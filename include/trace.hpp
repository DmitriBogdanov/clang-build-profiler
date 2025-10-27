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


namespace cbp {

// Clang traces are stored in the chrome tracing format, full specification can be found here:
// https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU
//
// In practice we are only concerned with a small subset of this specification.

struct trace {

    struct event {
        std::string                 name{};     // usually fits into SSO
        std::string                 type{};     // always contains a single char
        std::uint64_t               thread{};   // most of the compilation is really single-threaded
        microseconds                time{};     // stored in us
        std::optional<microseconds> duration{}; // stored in us
        glz::generic                args{};     // schema varies based on event name and compiler flags

        auto operator<=>(const event& other) const {
            return this->time <=> other.time; // makes events orderable by time
        }
    };

    std::vector<event> events{};
    microseconds       start_time{};
};

} // namespace cbp

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
struct glz::meta<cbp::trace> {
    constexpr static auto modify = glz::object(           //
        "traceEvents", &cbp::trace::trace::events,        //
        "beginningOfTime", &cbp::trace::trace::start_time //
    );                                                    //
};
