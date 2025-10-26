// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// ABI demangling. Clang traces store symbols in a mangled form, so we
// have to do some work to turn them back into a human-readable state.
// _________________________________________________________________________________

#pragma once

#include <string>


namespace cbp::symbol {

std::string demangle(const std::string& symbol);

} // namespace cbp
