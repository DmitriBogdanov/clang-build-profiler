// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "frontend/terminal.hpp"

#include "frontend/formatter.hpp"
#include "utility/colors.hpp"


void cbp::output::terminal(const cbp::profile& profile) try {

    auto callback = [](cbp::formatter_state& state, const cbp::tree& tree) {
        // Indent
        state.format(ansi::bright_black);
        for (std::size_t i = 0; i < state.depth; ++i) state.format("|  ");
        state.format(ansi::reset);

        // Serialize node
        const auto abs_total = cbp::time::to_ms(tree.total);
        const auto abs_self  = cbp::time::to_ms(tree.self);
        const auto rel_total = cbp::time::to_percentage(tree.total, state.timeframe);
        const auto rel_self  = cbp::time::to_percentage(tree.self, state.timeframe);

        const auto color = tree.category == cbp::tree_category::red      ? ansi::red
                           : tree.category == cbp::tree_category::yellow ? ansi::yellow
                           : tree.category == cbp::tree_category::white  ? ansi::white
                                                                         : ansi::bright_black;
        const auto reset = ansi::reset;

        constexpr std::size_t max_name_width = 117;

        const std::string name =
            tree.name.size() < max_name_width ? tree.name : tree.name.substr(0, max_name_width) + "...";

        constexpr auto fmt = "{}> {} ({} ms, {}%) | self ({} ms, {}%){}\n";

        state.format(fmt, color, name, abs_total, rel_total, abs_self, rel_self, reset);
    };

    const auto result = cbp::formatter{std::move(callback)}(profile);

    std::println("{}", result);

} catch (std::exception& e) {
    throw cbp::exception{"Could not output profile results to the terminal, error:\n{}", e.what()};
}