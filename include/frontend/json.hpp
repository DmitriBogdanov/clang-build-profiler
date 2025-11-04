// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// Output serialization for '--output=json'.
// _________________________________________________________________________________

#pragma once

#include <filesystem>

#include "backend/profile.hpp"


namespace cbp::output {
    
void json(const cbp::profile& profile, const std::filesystem::path& output_directory);
    
}