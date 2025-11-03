// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "backend/invoke.hpp"

#include <filesystem>
#include <print>

#include "backend/analyze.hpp"
#include "utility/colors.hpp"


constexpr auto parse_options = glz::opts{.error_on_unknown_keys = false};

// Analyzing a single file corresponds to analyzing a single translation unit
cbp::tree cbp::analyze_translation_unit(std::string_view path) try {
    // Read the trace & forward it to the analyzer
    cbp::trace  trace;
    std::string buffer;

    if (const glz::error_ctx err = glz::read_file_json<parse_options>(trace, path, buffer))
        throw cbp::exception{"Could not parse trace from JSON, error: {}", glz::format_error(err)};

    return cbp::analyze_trace(std::move(trace), path);

} catch (std::exception& e) { throw cbp::exception{"Could not analyze file {{ {} }}, error:\n{}", path, e.what()}; }

// Analyzing a target <=> analyzing all traces in a directory & its subdirectories.
//
// This is the main function we use to analyze non-CMake targets as virtually any sane
// build system and method of compilation will result in a "build artifacts" directory
// that will have traces next to their object files.
cbp::tree cbp::analyze_target(std::string_view path) try {
    // Handle filesystem errors
    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path))
        throw cbp::exception{"Target path {{ {} }} does not point to a valid directory", path};

    // Create root node
    auto target_tree = cbp::tree{.type = cbp::tree_type::target, .name = std::string(path)};

    // Recursively iterate all JSON files in a 'path' directory
    for (const auto& entry : std::filesystem::recursive_directory_iterator{path}) {
        if (!entry.is_regular_file()) continue;
        if (!entry.path().has_extension()) continue;
        if (entry.path().extension() != ".json") continue;

        // Parse the trace or skip it if JSON doesn't match the expected schema
        const std::string filepath = entry.path().string();

        cbp::trace  trace;
        std::string buffer;

        if (const glz::error_ctx err = glz::read_file_json<parse_options>(trace, filepath, buffer)) {
            constexpr auto fmt =
                "{}Warning:{} File {{ {} }} in target {{ {} }}, doesn't match the trace schema, skipping...";
            std::println(fmt, ansi::yellow, ansi::reset, filepath, path);
            std::println("Parse error => {}", glz::format_error(err));
            continue;
        }

        // Analyze translation unit
        try {
            target_tree.children.push_back(cbp::analyze_trace(std::move(trace), filepath));
        } catch (std::exception& e) {
            throw cbp::exception{"Could not analyze file {{ {} }}, error:\n{}", filepath, e.what()};
        }
    }

    // Gather root node timing
    for (const auto& child : target_tree.children) target_tree.total += child.total;

    return target_tree;

} catch (std::exception& e) { throw cbp::exception{"Could not analyze target {{ {} }}, error:\n{}", path, e.what()}; }

// --- Analyze build ---
// ---------------------

cbp::tree cbp::analyze_build(std::string_view path) try {
    // Handle filesystem errors
    if (!std::filesystem::exists(path) && !std::filesystem::is_directory(path))
        throw cbp::exception{"Build path {{ {} }} does not point to a valid directory", path};

    const std::filesystem::path target_directories_path =
        std::filesystem::path{path} / "CMakeFiles" / "TargetDirectories.txt";

    if (!std::filesystem::exists(target_directories_path)) throw cbp::exception{"Could not locate file {{ {} }}", path};

    // Every CMake target has a corresponding directory, mentioned in the 'TargetDirectories.txt',
    // however it also contains a few internal CMake targets that we need to ignore. We want to
    // parse those target paths and then analyze them one-by-one.
    std::vector<std::string> target_directories;

    auto target_directories_file = std::ifstream{target_directories_path};

    std::string line;
    while (std::getline(target_directories_file, line)) {
        // Targets with no corresponding directories are internal CMake targets, ignore them
        if (std::filesystem::exists(line) && !std::filesystem::is_empty(line))
            target_directories.push_back(std::move(line));
    }

    // Create root node
    auto target_tree = cbp::tree{.type = cbp::tree_type::targets, .name = "Targets"};

    // Analyze targets
    for (const auto& target_directory : target_directories)
        target_tree.children.push_back(cbp::analyze_target(target_directory));

    // Gather root node timing
    for (const auto& child : target_tree.children) target_tree.total += child.total;

    return target_tree;

} catch (std::exception& e) { throw cbp::exception{"Could not analyze build {{ {} }}, error:\n{}", path, e.what()}; }