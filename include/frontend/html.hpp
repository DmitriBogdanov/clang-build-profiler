// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// Output serialization for '--output=html'.
// _________________________________________________________________________________

#pragma once

#include <filesystem>

#include "backend/profile.hpp"


namespace cbp::output {
    
void html(const cbp::profile& profile, const std::filesystem::path& output_directory);
    
}