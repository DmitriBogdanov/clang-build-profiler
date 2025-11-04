// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "frontend/terminal.hpp"

#include <print>

#include "frontend/generic.hpp"
#include "utility/colors.hpp"


namespace {

void serialize(cbp::output::string_state& state, const cbp::tree& tree) {
    // Indent
    state.format(cbp::ansi::bright_black);
    for (std::size_t i = 0; i < state.depth; ++i) state.format("|  ");
    state.format(cbp::ansi::reset);

    // Serialize node
    const auto abs_total = cbp::time::to_ms(tree.total);
    const auto abs_self  = cbp::time::to_ms(tree.self);
    const auto rel_total = cbp::time::to_percentage(tree.total, state.timeframe);
    const auto rel_self  = cbp::time::to_percentage(tree.self, state.timeframe);

    const auto color = tree.category == cbp::tree_category::red      ? cbp::ansi::red
                       : tree.category == cbp::tree_category::yellow ? cbp::ansi::yellow
                       : tree.category == cbp::tree_category::white  ? cbp::ansi::white
                                                                     : cbp::ansi::bright_black;
    const auto reset = cbp::ansi::reset;

    constexpr std::size_t max_name_width = 117;

    const std::string name =
        tree.name.size() < max_name_width ? tree.name : tree.name.substr(0, max_name_width) + "...";

    constexpr auto fmt = "{}> {} ({} ms, {:.2f}%) | self ({} ms, {:.2f}%){}\n";

    state.format(fmt, color, name, abs_total, rel_total, abs_self, rel_self, reset);

    ++state.depth;
    for (const auto& child : tree.children) serialize(state, child);
    --state.depth;
}

} // namespace

void cbp::output::terminal(const cbp::profile& profile) try {
    // Serialize results to a string
    cbp::output::string_state state{profile};
    serialize(state, profile.tree);

    // Print to the terminal
    std::println("{}", state.str);

} catch (std::exception& e) {
    throw cbp::exception{"Could not output profile results to the terminal, error:\n{}", e.what()};
}