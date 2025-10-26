// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// Time units and <chrono> utils. Clang traces are always recorded in microseconds,
// so we use them internally and convert to milliseconds for display.
// _________________________________________________________________________________

#pragma once

#include <chrono>
#include <cstdint>


namespace cbp::time {

using microseconds = std::chrono::microseconds; // used for internal timestamps
using milliseconds = std::chrono::milliseconds; // used for display

template <class Rep, class Period>
std::uint64_t to_ms(std::chrono::duration<Rep, Period> duration) {
    return std::chrono::duration_cast<milliseconds>(duration).count();
}

template <class Rep, class Period>
std::uint64_t to_percentage(std::chrono::duration<Rep, Period> duration, std::chrono::duration<Rep, Period> timeframe) {
    using ms              = std::chrono::duration<double, milliseconds::period>;
    const double fraction = ms(duration).count() / ms(timeframe).count();
    return static_cast<std::uint64_t>(fraction * 100);
}

} // namespace cbp::time

namespace cbp {

using time::microseconds;
using time::milliseconds;

} // namespace cbp
