// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// Function that builds a merged tree summary for analyzing shared impact of
// headers / templates on all translation units.
// _________________________________________________________________________________

#pragma once

#include "backend/tree.hpp"

namespace cbp {

struct merge_summary {
    cbp::tree stages;
};

cbp::merge_summary create_merge_summary(const cbp::tree& tree);

} // namespace cbp