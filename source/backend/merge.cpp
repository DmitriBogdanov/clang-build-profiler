// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "backend/merge.hpp"

namespace {

cbp::tree merge_trees(cbp::tree root_1, cbp::tree root_2) {
    // Merge root timings
    root_1.total += root_2.total;
    root_1.self += root_2.self;

    // Merge children
    // 1) Create mapping so we can merge in O(N) instead of O(N^2)
    std::unordered_map<std::string, std::size_t> mapping;
    for (std::size_t i = 0; i < root_1.children.size(); ++i) mapping[root_1.children[i].name] = i;

    // 2) Perform the merge
    for (std::size_t i = 0; i < root_2.children.size(); ++i) {
        auto& child_2 = root_2.children[i];

        // First tree already has such node => merge subtrees
        if (auto it = mapping.find(child_2.name); it != mapping.end()) {
            auto& child_1 = root_1.children[it->second];

            child_1 = merge_trees(std::move(child_1), std::move(child_2));
        }
        // otherwise this is a new node
        else {
            root_1.children.push_back(std::move(child_2));
        }
    }

    std::sort(root_1.children.begin(), root_1.children.end());

    return root_1;
}

// void expand_and_merge(cbp::tree& total_parsing, cbp::tree& total_instantiation, const cbp::tree& tree) {
//     if (tree.type == cbp::tree_type::parsing) {
//         total_parsing = merge_trees(std::move(total_parsing), tree);
//         return;
//     }

//     if (tree.type == cbp::tree_type::instantiation) {
//         total_instantiation = merge_trees(std::move(total_instantiation), tree);
//         return;
//     }

//     if (cbp::to_bool(tree.type & cbp::tree_type::compilation_stage)) return; // don't expand further

//     if (cbp::to_bool(tree.type & cbp::tree_type::node))
//         throw cbp::exception{"Tree expansion descended too far while looking for parsing and instantiation subtrees
//         to "
//                              "merge, this is likely caused by incorrect input tree format"};

//     for (const auto& child : tree.children) expand_and_merge(total_parsing, total_instantiation, child);
// }

template <cbp::tree_type target_tree_type>
void expand_and_merge_trees_for_stage(cbp::tree& stage_total, const cbp::tree& tree) {
    static_assert(cbp::to_bool(target_tree_type & cbp::tree_type::compilation_stage),
                  "Target tree must be a compilation stage");

    if (tree.type == target_tree_type) {
        stage_total = merge_trees(std::move(stage_total), tree);
        return;
    }

    if (cbp::to_bool(tree.type & cbp::tree_type::compilation_stage)) return; // another stage, don't expand further

    if (cbp::to_bool(tree.type & cbp::tree_type::node))
        throw cbp::exception{"Tree expansion descended too far while looking for parsing and instantiation subtrees to "
                             "merge, this is likely caused by incorrect input tree format"};

    for (const auto& child : tree.children) expand_and_merge_trees_for_stage<target_tree_type>(stage_total, child);
}

template <cbp::tree_type target_tree_type>
cbp::tree create_merged_tree_for_stage(const cbp::tree& tree) {
    cbp::tree stage_total{ .type = target_tree_type };
    expand_and_merge_trees_for_stage<target_tree_type>(stage_total, tree);
    return stage_total;
}

} // namespace


cbp::merge_summary cbp::create_merge_summary(const cbp::tree& tree) try {
    cbp::tree parsing        = create_merged_tree_for_stage<cbp::tree_type::parsing>(tree);
    cbp::tree instantiation  = create_merged_tree_for_stage<cbp::tree_type::instantiation>(tree);
    cbp::tree llvm_codegen   = create_merged_tree_for_stage<cbp::tree_type::llvm_codegen>(tree);
    cbp::tree optimization   = create_merged_tree_for_stage<cbp::tree_type::optimization>(tree);
    cbp::tree native_codegen = create_merged_tree_for_stage<cbp::tree_type::native_codegen>(tree);

    parsing.name        = "Parsing";
    instantiation.name  = "Template instantiation";
    llvm_codegen.name   = "LLVM IR generation";
    optimization.name   = "Optimization";
    native_codegen.name = "Machine code generation";

    cbp::merge_summary summary;

    summary.stages.name     = "Compilation stages";
    summary.stages.type     = cbp::tree_type::translation_unit;
    summary.stages.children = {std::move(parsing), std::move(instantiation), std::move(llvm_codegen),
                               std::move(optimization), std::move(native_codegen)};
    
    // Total time of all compilation stages in likely to be a bit below 100% since
    // it doesn't include the misc. time of attributed to translation units
    for (const auto& child : summary.stages.children) summary.stages.total += child.total;
                               
    return summary;

} catch (std::exception& e) {
    throw cbp::exception{"Could not construct parsing & instantiation summary, error:\n{}", e.what()};
}