// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// Struct representation of the YAML config and its parsing/serialization.
// _________________________________________________________________________________

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "time.hpp"
#include "version.hpp"


namespace cbp {

struct config {

    // --- Subclasses ---
    // ------------------

    struct category {
        milliseconds duration = {};
        std::string  color    = {};
    };

    struct prefix_replacement {
        std::string from = {};
        std::string to   = {};
    };

    struct tree_section {
        bool enabled = true;

        std::vector<category> categorize = {
            category{.duration = milliseconds{300}, .color = "red"   },
            category{.duration = milliseconds{150}, .color = "yellow"},
            category{.duration = milliseconds{50},  .color = "white" },
            category{.duration = milliseconds{0},   .color = "gray"  }
        };

        std::vector<prefix_replacement> replace_prefix = {};
    };

    // --- Members ---
    // ---------------

    std::string version = version::format_semantic();

    tree_section tree;

    constexpr static auto default_path = ".clang-build-profiler";

    // --- Parsing/serialization ---
    // -----------------------------

    static config from_string(std::string_view str);
    static config from_file(std::string_view path);

    std::string to_string() const;
    void        to_file(std::string_view path) const;

    std::optional<std::string> validate() const;
};

} // namespace cbp
