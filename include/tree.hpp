// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// A struct that holds the main in-memory representation of the profiling results.
// Results are stored as a recursive tree of nodes, with each node having a timing
// attached. Summary and other profiling results are generated based on this tree.
// _________________________________________________________________________________

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "time.hpp"


// Most of the profile data resides in a giant tree of structures,
// which reflects exactly how we show it in the GUI:
//
// > Targets (1000 ms, 100%) | self (0 ms, 0%)                // 'targets_node'
// |  > target_1 (500 ms, 50%) | self (0 ms, 0%)              // 'target_node'
// |  |  ...                                                  // ...
// |  > target_2 (500 ms, 50%) | self (0 ms, 0%)              // 'target_node'
// |  |  > main.cpp (500 ms, 50%) | self (0 ms, 0%)           // 'translation_unit_node'
// |  |  |   ...                                              // ...
// |  |  > other.cpp                                          // 'translation_unit_node'
// |  |  |  > Parsing (300 ms, 30%) | self (0 ms, 0%)         // 'parsing_node'
// |  |  |  |  > header.h (300 ms, 30%) | self (300 ms, 30%)  // 'source_include_node'
// |  |  |  > Templates (200 ms, 20%) | self (100 ms, 10%)    // 'templates_node'
// |  |  |  |  > header.h (100 ms, 10%) | self (100 ms, 10%)  // 'template_include_node'
//
// Various nodes can be represented by various classes, but each node class 'T' satisfies following requirements:
//    1) 'T' has a member 'name'
//    2) 'T' has a member 'timing'
//    3) 'T' supports function 'apply_down(T&& node, Func&& f)' which invokes 'f(child)' for all 'node' children
// This allows us to concisely traverse & serialize the whole tree. An alternative approach would be
// to write it in OOP style with runtime polymorphism, but such approach is more error-prone
// and bloats the node size unnecessarily with v-table pointers.

namespace cbp::tree {

// --- Generic node data ---
// -------------------------

struct timing_data {
    microseconds time{};
    microseconds duration_total{};
    microseconds duration_self{};
};

template <class T>
concept node =
    std::same_as<decltype(T{}.timing), timing_data> && std::convertible_to<decltype(T{}.name), std::string_view>;

// --- Node types ---
// ------------------

struct source_include_node {
    std::string name{};
    timing_data timing{};

    std::vector<source_include_node> transitive_includes{};
};

struct template_include_node {
    std::string name{};
    timing_data timing{};

    std::vector<template_include_node> transitive_includes{};
};

struct parsing_node {
    static constexpr std::string_view name = "Parsing";

    timing_data timing{};

    std::vector<source_include_node> includes{};
};

struct templates_node {
    static constexpr std::string_view name = "Templates";

    timing_data timing{};

    std::vector<template_include_node> includes{};
};

struct translation_unit_node {
    std::string name{};
    timing_data timing{};

    parsing_node   parsing{};
    templates_node templates{};

    // Note: We could pack thing into a single tree structure, instead of having 2 essentially parallel
    //       trees containing different timings, but there is convenience in having the structure reflect
    //       the format it is actually shown in, so we will keep it unless performance rewrite is necessary.
};

struct target_node {
    std::string name{};
    timing_data timing{};

    std::vector<translation_unit_node> translation_units{};
};

struct targets_node {
    static constexpr auto name = "Targets";

    timing_data timing{};

    std::vector<target_node> targets{};
};

struct tree {
    targets_node targets; // root node of the tree
};

// --- Generic node functions ---
// ------------------------------

// Apply 'func' to all children of the 'node' (only one level down, no recursive expansion)
template <class Func>
void apply_down(const targets_node& node, Func&& func) {
    for (const auto& e : node.targets) func(e);
}
template <class Func>
void apply_down(const target_node& node, Func&& func) {
    for (const auto& e : node.translation_units) func(e);
}
template <class Func>
void apply_down(const translation_unit_node& node, Func&& func) {
    func(node.parsing);
    func(node.templates);
}
template <class Func>
void apply_down(const parsing_node& node, Func&& func) {
    for (const auto& e : node.includes) func(e);
}
template <class Func>
void apply_down(const templates_node& node, Func&& func) {
    for (const auto& e : node.includes) func(e);
}
template <class Func>
void apply_down(const source_include_node& node, Func&& func) {
    for (const auto& e : node.transitive_includes) func(e);
}
template <class Func>
void apply_down(const template_include_node& node, Func&& func) {
    for (const auto& e : node.transitive_includes) func(e);
}

// Apply 'func' to the 'node' and all of its children recursively
template <node node_type, class Func>
void apply_recursively(const node_type& node, Func&& func) {
    func(node);
    apply_down(node, [&](const auto& child){ apply_recursively(child, func); });
}

} // namespace cbp::tree
