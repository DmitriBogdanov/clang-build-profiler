// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "frontend/text.hpp"

#include <fstream>

#include "frontend/generic.hpp"
#include "utility/colors.hpp"


namespace {

void serialize(cbp::output::string_state& state, const cbp::tree& tree) {
    // Indent
    for (std::size_t i = 0; i < state.depth; ++i) state.format("|  ");

    // Serialize node
    const auto abs_total = cbp::time::to_ms(tree.total);
    const auto abs_self  = cbp::time::to_ms(tree.self);
    const auto rel_total = cbp::time::to_percentage(tree.total, state.timeframe);
    const auto rel_self  = cbp::time::to_percentage(tree.self, state.timeframe);

    constexpr std::size_t max_name_width = 117;

    const std::string name =
        tree.name.size() < max_name_width ? tree.name : tree.name.substr(0, max_name_width) + "...";

    constexpr auto fmt = "> {} ({} ms, {:.2f}%) | self ({} ms, {:.2f}%)\n";

    state.format(fmt, name, abs_total, rel_total, abs_self, rel_self);

    ++state.depth;
    for (const auto& child : tree.children) serialize(state, child);
    --state.depth;
}

} // namespace

void cbp::output::text(const cbp::profile& profile, const std::filesystem::path& output_directory) try {
    // Ensure proper directory structure
    std::filesystem::remove_all(output_directory);
    std::filesystem::create_directories(output_directory);
    
    // Serialize results to a string
    cbp::output::string_state state{profile};
    serialize(state, profile.tree);

    // Write to the text file
    std::ofstream{output_directory / "report.txt"} << state.str;

} catch (std::exception& e) { throw cbp::exception{"Could not output profile results as text, error:\n{}", e.what()}; }