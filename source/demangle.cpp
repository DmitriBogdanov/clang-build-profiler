// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "demangle.hpp"


// --- Detect platform ---
// -----------------------

#define CBP_DEMANGLE_WITH_CXXABI // TODO: Implement demangle for Windows


// --- <cxxabi> demangling ---
// ---------------------------

#ifdef CBP_DEMANGLE_WITH_CXXABI

#include <cassert>
#include <cstdlib>
#include <memory>

#include <cxxabi.h>

#include "exception.hpp"

struct c_str_deleter {
    void operator()(char* c_str) { std::free(c_str); }
};

using c_str_wrapper = std::unique_ptr<char, c_str_deleter>;

std::string cbp::symbol::demangle(const std::string& symbol) {
    // we have to take either 'std::string' or 'const char*' due to the null-termination requirement

    // Mangled names shouldn't contain any spaces
    assert(!symbol.contains(" "));

    // Some platforms struggle to demangle "__Z" with two leading underscores, so we trim the excess
    const std::size_t offset = symbol.starts_with("__Z") ? 1 : 0;

    // We are responsible for cleaning up the 'char *' returned by the API
    int        status = 0;
    const auto result = c_str_wrapper{abi::__cxa_demangle(symbol.c_str() + offset, nullptr, nullptr, &status)};

    if (status) throw cbp::exception{"Could not demangle symbol {{ {} }} with cxxabi, status {}", symbol, status};

    return std::string{result.get()};
}

#endif

// --- WinAPI demangling ---
// -------------------------

#ifdef CBP_DEMANGLE_WITH_WINAPI

// TODO:

#endif
