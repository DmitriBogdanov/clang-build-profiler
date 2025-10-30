// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "utility/filepath.hpp"

#include <filesystem>


std::string_view cbp::trim_filepath(std::string_view path) {
    const std::size_t last_slash = path.find_last_of("/\\");

    if (last_slash != std::string_view::npos && last_slash + 1 < path.size()) return path.substr(last_slash + 1);
    return path;
}

std::string cbp::normalize_filepath(std::string path) {
    return std::filesystem::path{std::move(path)}.lexically_normal().string();
    // removes ".." backtracking from the path, backtracing is usually introduced by relative paths
    // from the compiler binary such as, for example, "/usr/lib/llvm-21/bin/../include/"
}