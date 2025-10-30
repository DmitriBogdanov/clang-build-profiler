// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// Preprocessing of profiling results: tree pruning, color categorization,
// path prettification, template prettification and etc. 
// _________________________________________________________________________________

#pragma once

#include "backend/profile.hpp"


namespace cbp {

void preprocess(cbp::profile& profile);

}