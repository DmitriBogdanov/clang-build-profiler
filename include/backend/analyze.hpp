// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// Function that builds the main tree of profiling results from the trace.
// _________________________________________________________________________________

#pragma once

#include "backend/tree.hpp"
#include "backend/trace.hpp"


namespace cbp {

cbp::tree analyze_trace(cbp::trace trace, std::string_view name);

} // namespace cbp
