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

#include "backend/profile.hpp"


namespace cbp::output {
    
void terminal(const cbp::profile& profile);
    
}