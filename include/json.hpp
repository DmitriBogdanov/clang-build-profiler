// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// Wraps <glaze/json.hpp> library include, enables <chrono> parsing/serialization,
// adds a simpler read/write API with errors through exceptions so can have a
// uniform error handling style throughout the codebase.
// _________________________________________________________________________________

#pragma once

#include <utility>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces" // false positive
#endif

#include "glaze/json.hpp" // IWYU pragma: export

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "exception.hpp"
#include "time.hpp"


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
    static void op(cbp::microseconds& value, is_context auto&& ctx, auto&& b, auto&& ix) noexcept {
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
    static void op(cbp::milliseconds& value, is_context auto&& ctx, auto&& b, auto&& ix) noexcept {
        serialize<JSON>::op<opts>(value.count(), ctx, b, ix);
    }
};

} // namespace glz


// --- Read/write wrappers ---
// ---------------------------

namespace cbp {

template <glz::read_supported<glz::JSON> T, auto opts = glz::opts{.error_on_unknown_keys = false}>
[[nodiscard]] std::expected<T, std::string> try_read_file_json(std::string_view path) {
    T                    value;
    std::string          buffer;
    const glz::error_ctx err = glz::read_file_json<opts>(value, path, buffer);

    if (err) {
        std::string context = std::format("Could not read JSON at {{ {} }}, error:\n{}", path, glz::format_error(err));
        return std::unexpected{std::move(context)};
    }

    return value;
}

template <glz::read_supported<glz::JSON> T, auto opts = glz::opts{.error_on_unknown_keys = false}>
[[nodiscard]] T read_file_json(std::string_view path) {
    auto result = try_read_file_json<T, opts>(path);

    if (result) return result.value();
    else throw cbp::exception{result.error()};
}

template <glz::read_supported<glz::JSON> T>
[[nodiscard]] T read_file_jsonc(std::string_view path) {
    return read_file_json<T, glz::opts{.comments = true, .error_on_unknown_keys = false}>(path);
}

template <glz::write_supported<glz::JSON> T, auto opts = glz::opts{}>
void write_file_json(std::string_view path, T&& value) {
    std::string          buffer;
    const glz::error_ctx err = glz::write_file_json<opts>(std::forward<T>(value), path, buffer);

    if (err) throw cbp::exception{"Could not write JSON at {{ {} }}, error:\n{}", path, glz::format_error(err)};
}

template <glz::write_supported<glz::JSON> T>
void write_file_jsonc(std::string_view path, T&& value) {
    write_file_json<T, glz::opts{.prettify = true, .indentation_width = 4}>(path, std::forward<T>(value));
}

} // namespace cbp
