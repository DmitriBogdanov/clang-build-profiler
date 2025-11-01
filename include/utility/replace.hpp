// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// Substring & regex replacement functions, we need those in multiple places.
// _________________________________________________________________________________

#pragma once

#include <string>

#include "external/boost/regex.hpp"


namespace cbp {

void replace_all(std::string& str, std::string_view from, std::string_view to);

void replace_all(std::string& str, const boost::regex& from, std::string_view to);

void replace_all_dynamically(std::string& str, std::string_view from, std::string_view to);

void replace_all_template(std::string& str, std::string_view from, std::string_view to);

void replace_prefix(std::string& str, std::string_view from, std::string_view to);

void replace_suffix(std::string& str, std::string_view from, std::string_view to);

} // namespace cbp