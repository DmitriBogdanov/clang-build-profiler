// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// A struct representing a total of all profiling results.
// _________________________________________________________________________________

#pragma once

#include "backend/tree.hpp"
#include "backend/config.hpp"


namespace cbp {

struct profile {
    cbp::tree   tree;
    cbp::config config;

    // TODO: summary
};

} // namespace cbp