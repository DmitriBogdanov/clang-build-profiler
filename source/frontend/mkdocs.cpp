// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "frontend/mkdocs.hpp"

#include <version>

#include "frontend/generic.hpp"
#include "utility/embedded.hpp"
#include "utility/replace.hpp"

namespace {

void serialize(cbp::output::string_state& state, const cbp::tree& tree) {
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

    const auto color = fmt::format("<span class=\"cbp-timing-{}\">", color_name);
    const auto reset = "</span>";

    std::string name = tree.name;
    cbp::replace_all(name, "<", "&lt;");
    cbp::replace_all(name, ">", "&gt;");
    cbp::replace_all(name, "*", "\\*");
    cbp::replace_all(name, "_", "\\_");

    const auto prefix = tree.children.empty() ? "!!!" : "???";

    const auto callout_type = (tree.type == cbp::tree_type::targets)                        ? "targets"
                              : (tree.type == cbp::tree_type::target)                       ? "target"
                              : (tree.type == cbp::tree_type::translation_unit)             ? "translation-unit"
                              : cbp::to_bool(tree.type & cbp::tree_type::compilation_stage) ? "compilation-stage"
                                                                                            : "node";

    constexpr auto fmt = "{} {} \"{} {}({} ms, {:.2f}%) | self ({} ms, {:.2f}%){}\"\n";

    state.format(fmt, prefix, callout_type, name, color, abs_total, rel_total, abs_self, rel_self, reset);

    ++state.depth;
    for (const auto& child : tree.children) serialize(state, child);
    --state.depth;
}

} // namespace

void cbp::output::mkdocs(const cbp::profile& profile, const std::filesystem::path& output_directory) try {
    // Ensure proper directory structure
    std::filesystem::remove_all(output_directory);
    std::filesystem::create_directories(output_directory / "docs" / "images");

    // Copy MkDocs resources
    cbp::clone_from_embedded("resources/mkdocs/mkdocs.yml", output_directory / "mkdocs.yml");
    cbp::clone_from_embedded("resources/mkdocs/docs/images/favicon.svg", output_directory / "docs/images/favicon.svg");
    cbp::clone_from_embedded("resources/mkdocs/docs/admonitions.css", output_directory / "docs/admonitions.css");
    cbp::clone_from_embedded("resources/mkdocs/docs/classes.css", output_directory / "docs/classes.css");
    cbp::clone_from_embedded("resources/mkdocs/docs/width.css", output_directory / "docs/width.css");

    // Serialize results to a string
    cbp::output::string_state tree_state{profile};
    serialize(tree_state, profile.tree);

    cbp::output::string_state summary_state{profile};
    serialize(summary_state, profile.summary.stages);

    // Create files with results
    std::ofstream(output_directory / "docs/index.md") << "# Profiling results\n\n"
                                                      << tree_state.str << "\n\n"
                                                      << "# Compilation summary\n\n"
                                                      << summary_state.str;

} catch (std::exception& e) {
    throw cbp::exception{"Could not output profile results as MkDocs, error:\n{}", e.what()};
}