// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// A struct trepresenting a total of all profiling results.
// _________________________________________________________________________________

#pragma once

#include "config.hpp"
#include "tree.hpp"


namespace cbp {

struct profile {
    tree::tree tree;
    config     config;

    // TODO: summary
};

} // namespace cbp