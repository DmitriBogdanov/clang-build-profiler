// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// Functions for analyzing builds/directories/files, which handle the filesystem
// & parsing and invoke the actual analysis backend.
// _________________________________________________________________________________

#pragma once

#include "backend/tree.hpp"


namespace cbp {

cbp::tree analyze_build(std::string_view path);
cbp::tree analyze_target(std::string_view path);
cbp::tree analyze_translation_unit(std::string_view path);

} // namespace cbp
