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

#include "utility/json.hpp"
#include "utility/time.hpp"


// The main profiling data can be represented as a tree of nodes, reflecting exactly how we show it in the GUI:
//
// > Targets (1000 ms, 100%) | self (0 ms, 0%)                // 'tree_type::targets'
// |  > target_1 (500 ms, 50%) | self (0 ms, 0%)              // 'tree_type::target'
// |  |  ...                                                  // ...
// |  > target_2 (500 ms, 50%) | self (0 ms, 0%)              // 'tree_type::target'
// |  |  > main.cpp (500 ms, 50%) | self (0 ms, 0%)           // 'tree_type::translation_unit'
// |  |  |   ...                                              // ...
// |  |  > other.cpp                                          // 'tree_type::translation_unit'
// |  |  |  > Parsing (300 ms, 30%) | self (0 ms, 0%)         // 'tree_type::parsing'
// |  |  |  |  > header.h (300 ms, 30%) | self (300 ms, 30%)  // 'tree_type::source_parsing'
// |  |  |  > Templates (200 ms, 20%) | self (100 ms, 10%)    // 'tree_type::instantiation'
// |  |  |  |  > header.h (100 ms, 10%) | self (100 ms, 10%)  // 'tree_type::source_instantiation'
//
// The child nodes are stored densely without a backlink, this means no pointer stability, but tighter packing.
// We could pack things even more using a flattened tree representation, but this makes certain operations
// extremely difficult and is generally more error-prone since we need to preserve certain invariants, which
// are inherently upheld in the usual recursive case.

namespace cbp {

// Node categorization that allows us to determine which node is being inspected,
// some parts of the tree need special handling (e.g. 'parse' node need their names
// trimmed & simplified like filepaths, while 'instantiate' nodes need to collapse
// template names for readability)
enum class tree_type : std::uint16_t {
    // clang-format off
    targets                     = 1 << 0,
        target                  = 1 << 1,
            translation_unit    = 1 << 2,
                parsing         = 1 << 3,
                    parse       = 1 << 4,
                instantiation   = 1 << 5,
                    instantiate = 1 << 6,
                llvm_codegen    = 1 << 7,
                optimization    = 1 << 8,
                native_codegen  = 1 << 9,
    // clang-format on

    compilation_stage = parsing | instantiation | llvm_codegen | optimization | native_codegen,
    node              = parse | instantiate
    // bitflag groups, we intentionally don't enumerate these for reflection
};

[[nodiscard]] constexpr tree_type operator&(tree_type a, tree_type b) noexcept {
    return static_cast<tree_type>(std::to_underlying(a) & std::to_underlying(b));
}

[[nodiscard]] constexpr tree_type operator|(tree_type a, tree_type b) noexcept {
    return static_cast<tree_type>(std::to_underlying(a) | std::to_underlying(b));
}

// Node categorization used for coloring and pruning, warmer colors correspond to nodes that took more time
enum class tree_category : std::uint8_t { none, gray, white, yellow, red };

struct tree {
    cbp::tree_type     type;
    cbp::tree_category category = cbp::tree_category::none;
    std::string        name     = {};
    cbp::microseconds  total    = {};
    cbp::microseconds  self     = {};
    std::vector<tree>  children = {};

    auto operator<=>(const tree& other) const {
        return other.total <=> this->total;
        // makes tree nodes orderable by total duration, note the reversed order of arguments,
        // which means nodes with greater total duration will compare "less" and appear at the front
    }

    template <std::invocable<const cbp::tree&> Func>
    void for_all(Func func) const {
        func(*this);
        for (const auto& child : this->children) child.for_all(func);
    }

    template <std::invocable<cbp::tree&> Func>
    void for_all(Func func) {
        func(*this);
        for (auto& child : this->children) child.for_all(func);
    }

    template <std::invocable<const cbp::tree&> Func>
    void for_all_children(Func func) const {
        for (const auto& child : this->children) child.for_all(func);
    }

    template <std::invocable<cbp::tree&> Func>
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

template <>
struct glz::meta<cbp::tree_category> {
    using enum cbp::tree_category;

    static constexpr auto value = glz::enumerate(none, gray, white, yellow, red);
};
