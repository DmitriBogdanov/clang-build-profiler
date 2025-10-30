// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// Functions for operating of filepath strings, used for prettification.
// _________________________________________________________________________________

#pragma once

#include <string>
#include <string_view>


namespace cbp {

std::string_view trim_filepath(std::string_view path);

std::string normalize_filepath(std::string path);

} // namespace cbp