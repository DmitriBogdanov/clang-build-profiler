// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "utility/colors.hpp"

#include "utility/exception.hpp"


std::string_view cbp::color_from_name(std::string_view name) {
    // clang-format off
    if (name == "black"         ) return ansi::black         ;
    if (name == "red"           ) return ansi::red           ;
    if (name == "green"         ) return ansi::green         ;
    if (name == "yellow"        ) return ansi::yellow        ;
    if (name == "blue"          ) return ansi::blue          ;
    if (name == "magenta"       ) return ansi::magenta       ;
    if (name == "cyan"          ) return ansi::cyan          ;
    if (name == "white"         ) return ansi::white         ;
    if (name == "bright_black"  ) return ansi::bright_black  ;
    if (name == "gray"          ) return ansi::bright_black  ; // alias
    if (name == "grey"          ) return ansi::bright_black  ; // alias
    if (name == "bright_red"    ) return ansi::bright_red    ;
    if (name == "bright_green"  ) return ansi::bright_green  ;
    if (name == "bright_yellow" ) return ansi::bright_yellow ;
    if (name == "bright_blue"   ) return ansi::bright_blue   ;
    if (name == "bright_magenta") return ansi::bright_magenta;
    if (name == "bright_cyan"   ) return ansi::bright_cyan   ;
    if (name == "bright_white"  ) return ansi::bright_white  ;
    // clang-format on

    throw cbp::exception{"Unknown color name {{ {} }}", name};
}