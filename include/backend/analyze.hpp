// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// Functions for analyzing builds/directories/files, which turn clang traces
// into some kind of in-memory structure that we can work with.
// _________________________________________________________________________________

#pragma once

#include "backend/tree.hpp"
#include "backend/trace.hpp"


namespace cbp {

cbp::tree analyze_trace(cbp::trace trace, std::string_view name);

} // namespace cbp
