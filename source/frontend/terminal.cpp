// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "frontend/terminal.hpp"

#include "external/fmt/color.h"

#include "frontend/generic.hpp"
#include "utility/colors.hpp"


namespace {

void serialize(cbp::output::string_state& state, const cbp::tree& tree) {
    // Indent
    constexpr auto indent_color = fmt::color::gray;

    for (std::size_t i = 0; i < state.depth; ++i) fmt::print(fmt::fg(indent_color), "|  ");

    // Serialize node
    const auto abs_total = cbp::time::to_ms(tree.total);
    const auto abs_self  = cbp::time::to_ms(tree.self);
    const auto rel_total = cbp::time::to_percentage(tree.total, state.timeframe);
    const auto rel_self  = cbp::time::to_percentage(tree.self, state.timeframe);

    const auto color = tree.category == cbp::tree_category::red      ? fmt::color::indian_red
                       : tree.category == cbp::tree_category::yellow ? fmt::color::yellow
                       : tree.category == cbp::tree_category::white  ? fmt::color::white
                                                                     : fmt::color::gray;

    constexpr std::size_t max_name_width = 117;

    const std::string name =
        tree.name.size() < max_name_width ? tree.name : tree.name.substr(0, max_name_width) + "...";

    constexpr auto fmt = "> {} ({} ms, {:.2f}%) | self ({} ms, {:.2f}%)";

    fmt::print(fmt::fg(color), fmt, name, abs_total, rel_total, abs_self, rel_self);
    fmt::println("");
    
    ++state.depth;
    for (const auto& child : tree.children) serialize(state, child);
    --state.depth;
}

} // namespace

void cbp::output::terminal(const cbp::profile& profile) try {
    constexpr auto style_header = fmt::fg(fmt::color::dark_turquoise) | fmt::emphasis::bold;
    
    // Serialize results to the terminal
    fmt::println("");
    fmt::println("{}", fmt::styled("# Profiling results", style_header));
    fmt::println("");
    
    cbp::output::string_state state{profile};
    serialize(state, profile.tree);
    
    
    fmt::println("");

} catch (std::exception& e) {
    throw cbp::exception{"Could not output profile results to the terminal, error:\n{}", e.what()};
}