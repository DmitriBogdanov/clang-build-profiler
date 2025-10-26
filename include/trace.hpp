// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// A struct that holds an in-memory representation of the clang trace.
// _________________________________________________________________________________

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "json.hpp"


namespace cbp::trace {
    
// --- Trace data ---
// ------------------

// Clang traces are stored in the chrome tracing format, full specification can be found here:
// https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU
//
// In practice we are only concerned with a small subset of this specification.

struct event {
    std::string                 name{};     // usually fits into SSO
    std::string                 type{};     // always contains a single char
    std::uint64_t               thread{};   // most of the compilation is really single-threaded
    microseconds                time{};     // stored in us
    std::optional<microseconds> duration{}; // stored in us
    glz::generic                args{};     // schema varies based on event name and compiler flags
};

struct trace {
    std::vector<event> events{};
    microseconds       start_time{};
};

// --- Trace functions ---
// -----------------------

// template <class Predicate>
// [[nodiscard]] std::vector<event> extract_events(std::vector<event>& events, Predicate&& predicate) {
// }

// struct timeframe {
//     microseconds time;
//     microseconds duration;
// };

// [[nodiscard]] timeframe get_timeframe(const std::vector<event>& events);

} // namespace cbp::trace

// Rename reflected fields so we can use readable names in code, while the trace itself uses short names
template <>
struct glz::meta<cbp::trace::event> {
    constexpr static auto modify = glz::object( //
        "name", &cbp::trace::event::name,       //
        "ph", &cbp::trace::event::type,         //
        "tis", &cbp::trace::event::thread,      //
        "ts", &cbp::trace::event::time,         //
        "dur", &cbp::trace::event::duration,    //
        "args", &cbp::trace::event::args        //
    );                                          //
};

template <>
struct glz::meta<cbp::trace::trace> {
    constexpr static auto modify = glz::object(           //
        "traceEvents", &cbp::trace::trace::events,        //
        "beginningOfTime", &cbp::trace::trace::start_time //
    );                                                    //
};
