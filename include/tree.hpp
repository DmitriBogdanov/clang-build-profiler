// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// A struct that holds the main in-memory representation of the profiling results.
// _________________________________________________________________________________

#pragma once

#include <string>
#include <vector>

#include "json.hpp"
#include "time.hpp"


// The main profiling data can be represented as a tree of nodes, reflecting exactly how we show it in the GUI:
//
// > Targets (1000 ms, 100%) | self (0 ms, 0%)                // 'node_type::targets'
// |  > target_1 (500 ms, 50%) | self (0 ms, 0%)              // 'node_type::target'
// |  |  ...                                                  // ...
// |  > target_2 (500 ms, 50%) | self (0 ms, 0%)              // 'node_type::target'
// |  |  > main.cpp (500 ms, 50%) | self (0 ms, 0%)           // 'node_type::translation_unit'
// |  |  |   ...                                              // ...
// |  |  > other.cpp                                          // 'node_type::translation_unit'
// |  |  |  > Parsing (300 ms, 30%) | self (0 ms, 0%)         // 'node_type::parsing'
// |  |  |  |  > header.h (300 ms, 30%) | self (300 ms, 30%)  // 'node_type::source_parsing'
// |  |  |  > Templates (200 ms, 20%) | self (100 ms, 10%)    // 'node_type::instantiation'
// |  |  |  |  > header.h (100 ms, 10%) | self (100 ms, 10%)  // 'node_type::source_instantiation'
//
// We store this tree as a flattened array of nodes.
// Parent-child relations can be deduced from the node order & 'node.depth' values.
//
// Flattened approach proved to both easier to work with and more performant than a more traditional tree structure,
// through some care is necessary to not break the invariants, for example:
//
//    | A correct tree            | Broken invariant (!)             | Broken invariant (!)             |
//    | ------------------------- | -------------------------------- | -------------------------------- |
//    | > root      (depth = 0)   |   > root      (depth = 0)        |   > root      (depth = 1) <- [!] |
//    |   > child_1 (depth = 1)   |     > child_1 (depth = 2) <- [!] |     > child_1 (depth = 2)        |
//    |   > child_2 (depth = 1)   |     > child_2 (depth = 2)        |     > child_2 (depth = 2)        |

// TODO: Update the comment

namespace cbp {

enum class tree_type {
    // clang-format off
    targets,
        target,
            translation_unit,
                parsing,
                    parse,
                instantiation,
                    instantiate,
                llvm_codegen,
                optimization,
                native_codegen
    // clang-format on
};

struct tree {
    tree_type         type;
    std::string       name{};
    microseconds      total{};
    microseconds      self{};
    std::vector<tree> children{};

    auto operator<=>(const tree& other) const {
        return other.total <=> this->total;
        // makes tree nodes orderable by total duration, note the reversed order of arguments,
        // which means nodes with greater total duration will compare "less" and appear at the front
    }

    template <std::invocable<const tree&> Func>
    void for_all(Func func) const {
        func(*this);
        for (const auto& child : this->children) child.for_all(func);
    }

    template <std::invocable<tree&> Func>
    void for_all(Func func) {
        func(*this);
        for (auto& child : this->children) child.for_all(func);
    }

    template <std::invocable<const tree&> Func>
    void for_all_children(Func func) const {
        for (const auto& child : this->children) child.for_all(func);
    }

    template <std::invocable<tree&> Func>
    void for_all_children(Func func) {
        for (auto& child : this->children) child.for_all(func);
    }
};

} // namespace cbp

// Define enum reflection for JSON serialization
template <>
struct glz::meta<cbp::tree_type> {
    using enum cbp::tree_type;

    static constexpr auto value =
        glz::enumerate(targets, target, translation_unit, parsing, parse, instantiation, instantiate);
};
