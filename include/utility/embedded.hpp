// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// Version, build platform & copyright info.
// _________________________________________________________________________________

#pragma once

#include <string>
#include <filesystem>


namespace cbp {
    
void clone_from_embedded(const std::string& resource_path, const std::string& output_path);
void clone_from_embedded(const std::string& resource_path, const std::filesystem::path& output_path);
    
}