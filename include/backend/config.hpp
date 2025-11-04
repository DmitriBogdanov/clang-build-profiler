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

#include "utility/time.hpp"
#include "utility/version.hpp"


namespace cbp {

struct config {

    // --- Subclasses ---
    // ------------------

    struct categorization {
        cbp::milliseconds gray   = cbp::milliseconds{0};
        cbp::milliseconds white  = cbp::milliseconds{50};
        cbp::milliseconds yellow = cbp::milliseconds{150};
        cbp::milliseconds red    = cbp::milliseconds{300};
    };

    struct prefix_replacement_rule {
        std::string from = {};
        std::string to   = {};
    };

    struct tree_section {
        categorization categorize;

        bool detect_standard_headers = true;
        bool detect_project_headers  = true;

        std::vector<prefix_replacement_rule> replace_filepath = {};
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

    std::optional<std::string> validate() const;
};

} // namespace cbp
