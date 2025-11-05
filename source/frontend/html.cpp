// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "frontend/html.hpp"

#include "frontend/generic.hpp"
#include "utility/embedded.hpp"
#include "utility/replace.hpp"


namespace {

constexpr std::size_t indent_base_px  = 10;
constexpr std::size_t indent_level_px = 20;

void serialize(cbp::output::string_state& state, const cbp::tree& tree) {
    // HTML indent
    const std::size_t indent_px = state.depth ? indent_level_px : indent_base_px;

    // Serialize node
    const auto abs_total = cbp::time::to_ms(tree.total);
    const auto abs_self  = cbp::time::to_ms(tree.self);
    const auto rel_total = cbp::time::to_percentage(tree.total, state.timeframe);
    const auto rel_self  = cbp::time::to_percentage(tree.self, state.timeframe);

    const auto color_name = tree.category == cbp::tree_category::red      ? "red"
                            : tree.category == cbp::tree_category::yellow ? "yellow"
                            : tree.category == cbp::tree_category::white  ? "white"
                                                                          : "gray";

    const auto color = fmt::format("<span class=\"cbp-timing-{}\">", color_name);
    const auto reset = "</span>";

    std::string name = tree.name;
    cbp::replace_all(name, "<", "&lt;");
    cbp::replace_all(name, ">", "&gt;");

    const std::string_view tag = tree.children.empty() ? "div" : "details";

    const std::string prefix = fmt::format("<{} style=\"margin-left:{}px\">", tag, indent_px);
    const std::string suffix = fmt::format("</{}>", tag);

    const auto source_indent = [&] {
        for (std::size_t i = 0; i < state.depth; ++i) state.format("    "); // indent for source readability
    };

    // Open '<detail>' or '<div>'
    source_indent();
    state.format("{}\n", prefix);

    // '<summary>'
    source_indent();

    constexpr auto fmt = "<summary>{} {}({} ms, {:.2f}%) | self ({} ms, {:.2f}%){}</summary>\n";
    state.format(fmt, name, color, abs_total, rel_total, abs_self, rel_self, reset);

    // Nested elements
    ++state.depth;
    for (const auto& child : tree.children) serialize(state, child);
    --state.depth;

    // Close '<detail>' or '<div>'
    source_indent();
    state.format("{}\n", suffix);
}

} // namespace

void cbp::output::html(const cbp::profile& profile, const std::filesystem::path& output_directory) try {
    // Ensure proper directory structure
    std::filesystem::remove_all(output_directory);
    std::filesystem::create_directories(output_directory);

    // Copy HTML resources
    cbp::clone_from_embedded("resources/html/report.html", output_directory / "report.html");

    // Serialize results to a string
    cbp::output::string_state state{profile};
    serialize(state, profile.tree);

    // Create files with results
    constexpr auto tree_section_header = "<!-- ---------------------- -->\n"
                                         "<!-- Profiling results tree -->\n"
                                         "<!-- ---------------------- -->\n"
                                         "\n"
                                         "<header>Profiling results</header>\n"
                                         "\n";

    std::ofstream(output_directory / "report.html", std::ios::app)
        << fmt::format("{}{}", tree_section_header, state.str);

} catch (std::exception& e) { throw cbp::exception{"Could not output profile results as HTML, error:\n{}", e.what()}; }