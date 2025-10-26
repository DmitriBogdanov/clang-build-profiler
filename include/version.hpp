// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// Version, build platform & copyright info.
// _________________________________________________________________________________

#pragma once

#include "UTL/predef.hpp"


namespace cbp {

struct version {
    constexpr static int major = 0;
    constexpr static int minor = 1;
    constexpr static int patch = 0;

    constexpr static auto program      = "clang-report";
    constexpr static auto platform     = utl::predef::platform_name;
    constexpr static auto architecture = utl::predef::architecture_name;

    constexpr static auto copyright = "Copyright (c) 2025 Dmitri Bogdanov";

    static std::string format_semantic();
    static std::string format_full();
};

} // namespace cbp
