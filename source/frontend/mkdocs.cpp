// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "frontend/mkdocs.hpp"

#include "frontend/formatter.hpp"
#include "utility/replace.hpp"


// Bring in embedded resource
#include "cmrc/cmrc.hpp"
CMRC_DECLARE(cbp);

void copy_embedded_file(cmrc::embedded_filesystem&   filesystem,    //
                        const std::string&           resource_path, //
                        const std::filesystem::path& output_path    //
) {
    const cmrc::file file = filesystem.open(resource_path);

    std::ofstream{output_path} << std::string{file.begin(), file.end()};
}

void copy_embedded_files(const std::filesystem::path& output_directory) {
    cmrc::embedded_filesystem filesystem = cmrc::cbp::get_filesystem();

    copy_embedded_file(filesystem, "resources/mkdocs/mkdocs.yml", output_directory / "mkdocs.yml");
    copy_embedded_file(filesystem, "resources/mkdocs/docs/admonitions.css", output_directory / "docs/admonitions.css");
    copy_embedded_file(filesystem, "resources/mkdocs/docs/classes.css", output_directory / "docs/classes.css");
    copy_embedded_file(filesystem, "resources/mkdocs/docs/width.css", output_directory / "docs/width.css");
}

// Output
void cbp::output::mkdocs(const cbp::profile& profile, const std::filesystem::path& output_directory) try {

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
        cbp::replace_all(name, "*", "\\*");
        cbp::replace_all(name, "_", "\\_");

        if (name.size() % 256 == 0) name += ' ';
        // TEMP: Fix for 'libc++' 'std::format_to' bug, see
        // https://github.com/llvm/llvm-project/issues/160666
        // https://github.com/llvm/llvm-project/issues/154670

        const auto prefix = tree.children.empty() ? "!!!" : "???";

        const auto callout_type = (tree.type == cbp::tree_type::targets)                        ? "targets"
                                  : (tree.type == cbp::tree_type::target)                       ? "target"
                                  : (tree.type == cbp::tree_type::translation_unit)             ? "translation-unit"
                                  : cbp::to_bool(tree.type & cbp::tree_type::compilation_stage) ? "compilation-stage"
                                                                                                : "node";

        constexpr auto fmt = "{} {} \"{} {}({} ms, {:.2f}%) | self ({} ms, {:.2f}%){}\"\n";

        state.format(fmt, prefix, callout_type, name, color, abs_total, rel_total, abs_self, rel_self, reset);
    };

    const auto result = cbp::formatter{std::move(callback)}(profile);

    // Ensure proper directory structure
    std::filesystem::remove_all(output_directory);
    std::filesystem::create_directories(output_directory / "docs");

    // Create files necessary for an MkDocs build
    std::ofstream(output_directory / "docs/index.md") << std::format("# Profiling results\n\n{}", result);

    // Copy MkDocs resources
    copy_embedded_files(output_directory);

} catch (std::exception& e) {
    throw cbp::exception{"Could not output profile results to the terminal, error:\n{}", e.what()};
}