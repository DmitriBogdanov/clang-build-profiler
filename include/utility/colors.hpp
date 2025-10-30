// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// A listing of all ANSI color escape sequences. We use it to prettify terminal output.
// _________________________________________________________________________________

#pragma once

#include <string_view>


namespace cbp::ansi {

constexpr std::string_view black          = "\033[30m";
constexpr std::string_view red            = "\033[31m";
constexpr std::string_view green          = "\033[32m";
constexpr std::string_view yellow         = "\033[33m";
constexpr std::string_view blue           = "\033[34m";
constexpr std::string_view magenta        = "\033[35m";
constexpr std::string_view cyan           = "\033[36m";
constexpr std::string_view white          = "\033[37m";
constexpr std::string_view bright_black   = "\033[90m"; // also known as "gray"
constexpr std::string_view bright_red     = "\033[91m";
constexpr std::string_view bright_green   = "\033[92m";
constexpr std::string_view bright_yellow  = "\033[93m";
constexpr std::string_view bright_blue    = "\033[94m";
constexpr std::string_view bright_magenta = "\033[95m";
constexpr std::string_view bright_cyan    = "\033[96m";
constexpr std::string_view bright_white   = "\033[97m";

constexpr std::string_view bold_black          = "\033[30;1m";
constexpr std::string_view bold_red            = "\033[31;1m";
constexpr std::string_view bold_green          = "\033[32;1m";
constexpr std::string_view bold_yellow         = "\033[33;1m";
constexpr std::string_view bold_blue           = "\033[34;1m";
constexpr std::string_view bold_magenta        = "\033[35;1m";
constexpr std::string_view bold_cyan           = "\033[36;1m";
constexpr std::string_view bold_white          = "\033[37;1m";
constexpr std::string_view bold_bright_black   = "\033[90;1m";
constexpr std::string_view bold_bright_red     = "\033[91;1m";
constexpr std::string_view bold_bright_green   = "\033[92;1m";
constexpr std::string_view bold_bright_yellow  = "\033[93;1m";
constexpr std::string_view bold_bright_blue    = "\033[94;1m";
constexpr std::string_view bold_bright_magenta = "\033[95;1m";
constexpr std::string_view bold_bright_cyan    = "\033[96;1m";
constexpr std::string_view bold_bright_white   = "\033[97;1m";

constexpr std::string_view reset = "\033[0m";

} // namespace cbp::ansi

namespace cbp {

[[nodiscard]] std::string_view color_from_name(std::string_view name);

} // namespace cbp
