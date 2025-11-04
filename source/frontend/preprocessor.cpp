// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "frontend/preprocessor.hpp"

#include <filesystem>

#include "utility/filepath.hpp"
#include "utility/lookup.hpp"
#include "utility/prettify.hpp"
#include "utility/replace.hpp"

cbp::tree_category category_from_time(cbp::microseconds total, const cbp::config& config) {
    return (total >= config.tree.categorize.red)      ? cbp::tree_category::red
           : (total >= config.tree.categorize.yellow) ? cbp::tree_category::yellow
           : (total >= config.tree.categorize.white)  ? cbp::tree_category::white
           : (total >= config.tree.categorize.gray)   ? cbp::tree_category::gray
                                                      : cbp::tree_category::none;
}

void categorize(std::vector<cbp::tree>& children, const cbp::config& config) {
    for (auto& child : children) child.category = category_from_time(child.total, config);
}

void prune(std::vector<cbp::tree>& children) {
    const auto predicate = [](const cbp::tree& child) { return child.category == cbp::tree_category::none; };
    const auto iter      = std::remove_if(children.begin(), children.end(), predicate);

    children.erase(iter, children.end());
}

void shorten_standard_headers(std::vector<cbp::tree>& children) {
    for (auto& child : children) {
        if (child.type == cbp::tree_type::parse) {
            const auto filename = cbp::trim_filepath(child.name);

            if (cbp::lookup::is_standard_header(filename)) {
                child.name = std::format("<{}>", filename);
                child.self = child.total; // since we remove the info about the children
                child.children.clear();
            }
        }
    }
}

void shorten_project_headers(std::vector<cbp::tree>& children, std::string_view working_directory) {
    for (auto& child : children) {
        if (child.type == cbp::tree_type::parse || child.type == cbp::tree_type::translation_unit) {
            cbp::replace_prefix(child.name, working_directory, "");
            cbp::replace_prefix(child.name, "/", "");
            cbp::replace_prefix(child.name, "\\", "");
        }
    }
}

void normalize_paths(std::vector<cbp::tree>& children) {
    for (auto& child : children)
        if (child.type == cbp::tree_type::parse || child.type == cbp::tree_type::translation_unit)
            child.name = cbp::normalize_filepath(std::move(child.name));
}

void prettify_instantiations(std::vector<cbp::tree>& children) {
    for (auto& child : children) {
        if (child.type != cbp::tree_type::instantiate) continue;

        child.name = cbp::prettify::full(std::move(child.name));
        // collapses instantiations, replaces alias, normalizes format and performs
        // a lot of other work to make expanded template instantiations more readable
    }
}

void replace_configured_prefixes(std::vector<cbp::tree>& children, const cbp::config& config) {
    for (auto& child : children)
        if (child.type == cbp::tree_type::parse || child.type == cbp::tree_type::translation_unit)
            for (const auto& replacement : config.tree.replace_filepath)
                cbp::replace_prefix(child.name, replacement.from, replacement.to);
}

void prettify_tree(cbp::tree& tree, const cbp::config& config, std::string_view working_directory) {
    categorize(tree.children, config); // should happen first
    prune(tree.children);              // uses categorization for pruning

    // Simplify target & translation unit names
    if (tree.type == cbp::tree_type::target) {
        const std::string target_path = tree.name;

        tree.name = cbp::trim_filepath(target_path); // CMake guarantees unique target names,
        cbp::replace_suffix(tree.name, ".dir", "");  // specifying the full directory is redundant

        for (auto& translation_unit : tree.children) {
            cbp::replace_prefix(translation_unit.name, target_path + "/", ""); // trims target root
            cbp::replace_suffix(translation_unit.name, ".json", "");           // trims trace extension suffix
        }
    }
    
    if (config.tree.detect_standard_headers) shorten_standard_headers(tree.children); // uses path & type for pruning
    if (config.tree.detect_project_headers) shorten_project_headers(tree.children, working_directory);

    normalize_paths(tree.children);
    prettify_instantiations(tree.children);
    replace_configured_prefixes(tree.children, config);

    for (auto& child : tree.children) prettify_tree(child, config, working_directory);
}

void prettify_root(cbp::tree& root, const cbp::config& config) {
    root.category = category_from_time(root.total, config);
}

void cbp::preprocess(cbp::profile& profile, std::string_view working_directory) try {

    prettify_tree(profile.tree, profile.config, working_directory);
    prettify_root(profile.tree, profile.config);

} catch (std::exception& e) { throw cbp::exception{"Could not preprocess profiling tree, error:\n{}", e.what()}; }