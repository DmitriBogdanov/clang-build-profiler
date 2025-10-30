// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "frontend/mkdocs.hpp"

#include "frontend/formatter.hpp"
#include "utility/replace.hpp"


void cbp::output::mkdocs(const cbp::profile& profile) try {

    auto callback = [](cbp::formatter_state& state, const cbp::tree& tree) {
        // Indent
        for (std::size_t i = 0; i < state.depth; ++i) state.format("    ");

        // Serialize node
        const auto abs_total = cbp::time::to_ms(tree.total);
        const auto abs_self  = cbp::time::to_ms(tree.self);
        const auto rel_total = cbp::time::to_percentage(tree.total, state.timeframe);
        const auto rel_self  = cbp::time::to_percentage(tree.self, state.timeframe);

        const auto color_name = tree.category == cbp::tree_category::red      ? "red"
                                : tree.category == cbp::tree_category::yellow ? "yellow"
                                : tree.category == cbp::tree_category::white  ? "white"
                                                                              : "gray";

        const auto color = std::format("<span class=\"cbp-timing-{}\">", color_name);
        const auto reset = "</span>";

        std::string name = tree.name;
        cbp::replace_all(name, "<", "&lt;");
        cbp::replace_all(name, ">", "&gt;");
        cbp::replace_all(name, "*", "\\*;");
        cbp::replace_all(name, "_", "\\_;");
        
        if (name.size() % 256 == 0) name += ' ';
        // TEMP: Fix for 'libc++' 'std::format_to' bug, see
        // https://github.com/llvm/llvm-project/issues/160666
        // https://github.com/llvm/llvm-project/issues/154670

        std::string prefix = "???"; // TODO:

        const auto callout = "node";

        constexpr auto fmt = "{} {} \"{} {}({} ms, {}%) | self ({} ms, {}%){}\"\n";

        state.format(fmt, prefix, callout, name, color, abs_total, rel_total, abs_self, rel_self, reset);
    };

    const auto result = cbp::formatter{std::move(callback)}(profile);

    // Ensure proper directory structure
    std::filesystem::remove_all(".cbp/");
    std::filesystem::create_directories(".cbp/docs/");

    // Create files necessary for an MkDocs build
    std::ofstream(".cbp/docs/index.md") << std::format("# Profiling results\n\n{}", result);

} catch (std::exception& e) {
    throw cbp::exception{"Could not output profile results to the terminal, error:\n{}", e.what()};
}