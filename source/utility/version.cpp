// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "utility/version.hpp"

#include <format>


std::string cbp::version::format_semantic() { return std::format("{}.{}.{}", major, minor, patch); }

std::string cbp::version::format_full() {
    return std::format(                    //
        "{} version {}.{}.{} ({} {})\n{}", //
        program,                           //
        major, minor, patch,               //
        platform, architecture,            //
        copyright                          //
    );                                     //
}
