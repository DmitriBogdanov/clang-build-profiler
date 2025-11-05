// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "utility/version.hpp"

#include "external/fmt/format.h"


std::string cbp::version::format_semantic() { return fmt::format("{}.{}.{}", major, minor, patch); }

std::string cbp::version::format_full() {
    return fmt::format(                    //
        "{} version {}.{}.{} ({} {})\n{}", //
        program,                           //
        major, minor, patch,               //
        platform, architecture,            //
        copyright                          //
    );                                     //
}
