// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// Wraps <glaze/json.hpp> library include and enables <chrono> parsing/serialization.
// Also adds a simpler read/write API with errors through exceptions so can have a
// uniform error handling style throughout the codebase.
// _________________________________________________________________________________

#pragma once

#include <utility>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces" // false positive in 'glaze'
#endif

#include "external/glaze/json.hpp" // IWYU pragma: export

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "utility/exception.hpp"
#include "utility/time.hpp"


// --- <chrono> parsing/serialization support ---
// ----------------------------------------------

namespace glz {

// Clang traces always store time in microseconds so we add support for parsing/serializing integers
// directly into 'std::chrono::microseconds', this shouldn't incur any additional overhead,
// see https://stephenberry.github.io/glaze/custom-serialization/
template <>
struct from<JSON, cbp::microseconds> {
    template <auto opts>
    static void op(cbp::microseconds& value, is_context auto&& ctx, auto&& it, auto&& end) noexcept {
        cbp::microseconds::rep representation; // same as 'std::uint64_t'
        parse<JSON>::op<opts>(representation, ctx, it, end);
        value = cbp::microseconds{representation};
    }
};

template <>
struct to<JSON, cbp::microseconds> {
    template <auto opts>
    static void op(const cbp::microseconds& value, is_context auto&& ctx, auto&& b, auto&& ix) noexcept {
        serialize<JSON>::op<opts>(value.count(), ctx, b, ix);
    }
};

// Config files and GUI use duration in milliseconds, so we add support for them too
template <>
struct from<JSON, cbp::milliseconds> {
    template <auto opts>
    static void op(cbp::milliseconds& value, is_context auto&& ctx, auto&& it, auto&& end) noexcept {
        cbp::milliseconds::rep representation; // same as 'std::uint64_t'
        parse<JSON>::op<opts>(representation, ctx, it, end);
        value = cbp::milliseconds{representation};
    }
};

template <>
struct to<JSON, cbp::milliseconds> {
    template <auto opts>
    static void op(const cbp::milliseconds& value, is_context auto&& ctx, auto&& b, auto&& ix) noexcept {
        serialize<JSON>::op<opts>(value.count(), ctx, b, ix);
    }
};

} // namespace glz
