// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// Logic for serializing profiling results to a string.
// _________________________________________________________________________________

#pragma once

#include "profile.hpp"


namespace cbp::display::string {

[[nodiscard]] std::string serialize(const profile& results, const cbp::config& config, bool colors = true);

}