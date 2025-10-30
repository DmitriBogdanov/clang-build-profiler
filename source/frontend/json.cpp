// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "frontend/json.hpp"

#include "frontend/formatter.hpp"
#include "utility/colors.hpp"
#include "utility/json.hpp"


// constexpr auto write_options = glz::opts{.prettify = true, .indentation_width = 4};
constexpr auto write_options = glz::opts{};

void cbp::output::json(const cbp::profile& profile) try {

    // Ensure proper directory structure
    std::filesystem::remove_all(".cbp/");
    std::filesystem::create_directories(".cbp/");

    // Serialize the JSON dump of the profile
    std::string buffer;
    if (const glz::error_ctx err = glz::write_file_json<write_options>(profile, ".cbp/profiling.json", buffer))
        throw cbp::exception{"Could not serialize JSON, error:\n{}", glz::format_error(err)};

    // std::string buffer;
    // if (const glz::error_ctx err = glz::write_json(profile, buffer))
    //     throw cbp::exception{"Could not serialize JSON, error:\n{}", glz::format_error(err)};
    // std::ofstream{".cbp/profiling.json"} << buffer;

} catch (std::exception& e) {
    throw cbp::exception{"Could not output profile results to the terminal, error:\n{}", e.what()};
}