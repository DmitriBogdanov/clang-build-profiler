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

namespace cbp {

struct tree {

    enum class node_type {
        targets,
        target,
        translation_unit,
        parsing,
        source_parsing,
        instantiation,
        source_instantiation
    };

    struct node {
        std::string  name{};
        microseconds time{};
        microseconds duration_total{};
        microseconds duration_self{};
        std::size_t  depth{};
        node_type    type;
    };

    std::vector<node> nodes;

    node&       root();
    const node& root() const;

    void validate_invariants() const;
};

} // namespace cbp
