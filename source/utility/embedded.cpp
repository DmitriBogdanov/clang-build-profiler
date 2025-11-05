// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "utility/embedded.hpp"

#include <fstream>

#include "cmrc/cmrc.hpp"


CMRC_DECLARE(cbp); // brings in embedded resources

void cbp::clone_from_embedded(const std::string& resource_path, const std::string& output_path) {
    static cmrc::embedded_filesystem filesystem = cmrc::cbp::get_filesystem();
    
    // Get resource from embedded files as a raw 'char' buffer
    const cmrc::file file = filesystem.open(resource_path);
    
    // Serialize it to an output file
    std::ofstream{output_path} << std::string{file.begin(), file.end()};
}

void cbp::clone_from_embedded(const std::string& resource_path, const std::filesystem::path& output_path) {
    cbp::clone_from_embedded(resource_path, output_path.string());
    // can be lossy on windows, but we don't really care about non-ascii paths
}