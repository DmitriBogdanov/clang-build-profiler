// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// A custom exception class used throughout the codebase, it carries source location
// info and supports C++20 <format> strings in constructor, which makes diagnostics
// nicer. Chaining & rethrowing such exceptions can even accomplish a pseudo-stacktrace.
// _________________________________________________________________________________

#pragma once

#include <format>
#include <source_location>
#include <stdexcept>

#include "utility/filepath.hpp"


namespace cbp {

class exception : public std::runtime_error {

    // Note: ANSI color sequences are supported by most modern terminals
    constexpr static auto format = //
        "\033[31;1m"               // bold red
        "Error   ->"               // |
        "\033[0m"                  // reset
        " "                        //
        "\033[36m"                 // cyan
        "cbp::exception"           // |
        "\033[0m"                  // reset
        " thrown at "              //
        "\033[35m"                 // magenta
        "{}"                       // |
        "\033[0m"                  // reset
        ":"                        //
        "\033[35m"                 // magenta
        "{}"                       // |
        "\033[0m"                  // reset
        " in function "            //
        "\033[35m"                 // magenta
        "{}"                       // |
        "\033[0m"                  // reset
        "\n"                       //
        "\033[31;1m"               // bold yellow
        "Message ->"               // |
        "\033[0m"                  // reset
        " {}";                     //

public:
    // Required API
    exception(std::string_view message, std::source_location loc = std::source_location::current())
        : std::runtime_error(
              std::format(format, cbp::trim_filepath(loc.file_name()), loc.line(), loc.function_name(), message)) {}

    exception(const exception& other) noexcept : std::runtime_error(other) {}

    [[nodiscard]] const char* what() const noexcept { return std::runtime_error::what(); }

    // Constructors with fmt
    // clang-format off
    template <class T1>
    exception(std::format_string<T1> fmt, T1&& arg1,
              std::source_location loc = std::source_location::current())
        : exception(std::format(fmt, std::forward<T1>(arg1)), loc) {}

    template <class T1, class T2>
    exception(std::format_string<T1, T2> fmt, T1&& arg1, T2&& arg2,
              std::source_location loc = std::source_location::current())
        : exception(std::format(fmt, std::forward<T1>(arg1), std::forward<T2>(arg2)), loc) {}

    template <class T1, class T2, class T3>
    exception(std::format_string<T1, T2, T3> fmt, T1&& arg1, T2&& arg2, T3&& arg3,
              std::source_location loc = std::source_location::current())
        : exception(std::format(fmt, std::forward<T1>(arg1), std::forward<T2>(arg2), std::forward<T3>(arg3)), loc) {}

    template <class T1, class T2, class T3, class T4>
    exception(std::format_string<T1, T2, T3, T4> fmt, T1&& arg1, T2&& arg2, T3&& arg3, T4&& arg4,
              std::source_location loc = std::source_location::current())
        : exception(std::format(fmt, std::forward<T1>(arg1), std::forward<T2>(arg2), std::forward<T3>(arg3), std::forward<T4>(arg4)), loc) {}
    // clang-format on
};

} // namespace cbp
