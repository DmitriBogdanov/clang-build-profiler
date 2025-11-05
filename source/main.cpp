// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// Program entry point. Handles CLI args and invokes the analyzer.
// _________________________________________________________________________________

#include "external/UTL/time.hpp"
#include "external/argparse/argparse.hpp"
#include "external/fmt/color.h"
#include "external/fmt/format.h"

#include "backend/config.hpp"
#include "backend/invoke.hpp"
#include "frontend/html.hpp"
#include "frontend/json.hpp"
#include "frontend/mkdocs.hpp"
#include "frontend/preprocessor.hpp"
#include "frontend/terminal.hpp"
#include "frontend/text.hpp"
#include "utility/exception.hpp"
#include "utility/version.hpp"


constexpr auto style_step    = fmt::fg(fmt::color::dark_blue) | fmt::emphasis::bold;
constexpr auto style_hint    = fmt::fg(fmt::color::yellow) | fmt::emphasis::bold;
constexpr auto style_error   = fmt::fg(fmt::color::indian_red) | fmt::emphasis::bold;
constexpr auto style_path    = fmt::fg(fmt::color::saddle_brown);
constexpr auto style_enum    = fmt::fg(fmt::color::teal);
constexpr auto style_command = fmt::fg(fmt::color::purple) | fmt::emphasis::bold;

utl::time::Stopwatch stopwatch;

template <class... Args>
void exit_failure(fmt::format_string<Args...> fmt, Args&&... args) {
    fmt::println("Execution failed with with code {}, elapsed time: {}", EXIT_FAILURE, stopwatch.elapsed_string());
    fmt::println(fmt, std::forward<Args>(args)...);

    std::exit(EXIT_FAILURE);
}

template <class... Args>
void exit_failure_quiet(fmt::format_string<Args...> fmt, Args&&... args) {
    fmt::println(fmt, std::forward<Args>(args)...);

    std::exit(EXIT_FAILURE);
}

template <class... Args>
void exit_success(fmt::format_string<Args...> fmt, Args&&... args) {
    fmt::println("Execution finished, elapsed time: {}", stopwatch.elapsed_string());
    fmt::println(fmt, std::forward<Args>(args)...);

    std::exit(EXIT_SUCCESS);
}

template <class... Args>
void exit_success_quiet(fmt::format_string<Args...> fmt, Args&&... args) {
    fmt::println(fmt, std::forward<Args>(args)...);

    std::exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) try {
    // Handle CLI args
    const std::string version = cbp::version::format_full();

    argparse::ArgumentParser cli(cbp::version::program, version, argparse::default_arguments::none);

    cli.add_description("Human-readable report generator for `clang -ftime-trace` traces");

    cli.add_epilog("More detailed documentation can be found at https://dmitribogdanov.github.io/clang-report/");

    cli                                //
        .add_argument("-h", "--help")  //
        .flag()                        //
        .help("Displays help message") //
        .action([&](const auto&) {     //
            exit_success_quiet("{}", cli.help().str());
        });

    cli                                       //
        .add_argument("-v", "--version")      //
        .flag()                               //
        .help("Displays application version") //
        .action([&](const auto&) {            //
            exit_success_quiet("{}", version);
        });

    cli                                                                         //
        .add_argument("-w", "--write-config")                                   //
        .flag()                                                                 //
        .help("Creates config file corresponding to the default configuration") //
        .action([](const auto&) {                                               //
            const std::string path = cbp::config::default_path;
            // TODO: Serialize YAML here
            exit_success("Serialized a copy of default config to {{ {} }} ", path);
        });

    cli                                           //
        .add_argument("-c", "--config")           //
        .default_value(cbp::config::default_path) //
        .required()                               //
        .help("Specifies custom config path");    //

    cli                                             //
        .add_argument("-a", "--artifacts")          //
        .default_value(".cbp/")                     //
        .required()                                 //
        .help("Specifies custom output directory"); //

    auto& exclusive_group = cli.add_mutually_exclusive_group();

    exclusive_group                             //
        .add_argument("-b", "--build")          //
        .default_value(std::string{"build/"})   //
        .help("Selects CMake build directory"); //

    exclusive_group                                 //
        .add_argument("-t", "--target")             //
        .help("Selects build artifacts directory"); //

    exclusive_group                                 //
        .add_argument("-f", "--file")               //
        .help("Selects specific translation unit"); //

    cli                                                        //
        .add_argument("-o", "--output")                        //
        .required()                                            //
        .choices("mkdocs", "html", "terminal", "json", "text") //
        .default_value(std::string{"terminal"})                //
        .help("Selects profiling output format");              //

    try {
        cli.parse_args(argc, argv);
    } catch (std::exception& e) {
        fmt::println("{}", fmt::styled("Error parsing CLI arguments:", style_error));
        fmt::println("");
        fmt::println("{}", e.what());
        fmt::println("");
        fmt::println("Run {} to see the full usage guide.", fmt::styled("clang-report --help", style_command));
        exit_failure_quiet("");
    }

    // Parse config
    const std::string working_directory = std::filesystem::current_path().string();
    const std::string config_path       = cli.get<std::string>("--config");

    fmt::print(style_step, "Step 1/5: ");
    fmt::println("Working directory is {{ {} }}...", fmt::styled(working_directory, style_path));

    fmt::print(style_step, "Step 2/5: ");
    fmt::println("Parsing config {{ {} }}...", fmt::styled(config_path, style_path));

    const cbp::config config =
        std::filesystem::exists(config_path) ? cbp::config::from_file(config_path) : cbp::config{};

    if (const auto err = config.validate()) exit_failure("Config validation error:\n{}", err.value());

    // Set profile
    cbp::profile profile;

    profile.config = config;

    // Analyze the required file / target / build
    if (cli.is_used("--file")) {
        const std::string path = cli.get<std::string>("--file");

        fmt::print(style_step, "Step 3/5: ");
        fmt::println("Analyzing translation unit {{ {} }}...", fmt::styled(path, style_path));

        profile.tree = cbp::analyze_translation_unit(path);
    } else if (cli.is_used("--target")) {
        const std::string path = cli.get<std::string>("--target");

        fmt::print(style_step, "Step 3/5: ");
        fmt::println("Analyzing target {{ {} }}...", fmt::styled(path, style_path));

        profile.tree = cbp::analyze_target(path);
    } else {
        const std::string path = cli.get<std::string>("--build");

        fmt::print(style_step, "Step 3/5: ");
        fmt::println("Analyzing CMake build {{ {} }}...", fmt::styled(path, style_path));

        profile.tree = cbp::analyze_build(path);
    }

    // Prettify the results
    fmt::print(style_step, "Step 4/5: ");
    fmt::println("Preprocessing results...");

    cbp::preprocess(profile, working_directory);

    // Invoke the frontend
    const std::string selected_output = cli.get("--output");

    fmt::print(style_step, "Step 5/5: ");
    fmt::println("Invoking frontend for {{ {} }}...", fmt::styled(selected_output, style_enum));

    const std::filesystem::path output_directory_path = cli.get<std::string>("--artifacts");

    if (selected_output == "mkdocs") {
        cbp::output::mkdocs(profile, output_directory_path);

        fmt::print(style_hint, "Hint: ");
        fmt::print("To open the generated report in browser run ");
        fmt::print(style_command, "(cd {} && mkdocs serve --open)", output_directory_path.string());
        fmt::println("");

    } else if (selected_output == "html") {
        cbp::output::html(profile, output_directory_path);

        const auto report_path = output_directory_path / "report.html";

        fmt::print(style_hint, "Hint: ");
        fmt::print("To open the generated report in browser run ");
        fmt::print(style_command, "open {}", report_path.string());
        fmt::println("");

    } else if (selected_output == "terminal") {
        cbp::output::terminal(profile);
    } else if (selected_output == "json") {
        cbp::output::json(profile, output_directory_path);
    } else if (selected_output == "text") {
        cbp::output::text(profile, output_directory_path);

        const auto report_path = output_directory_path / "report.txt";

        fmt::print(style_hint, "Hint: ");
        fmt::print("To open the generated report in text editor run ");
        fmt::print(style_command, "open {}", report_path.string());
        fmt::println("");

    } else {
        exit_failure("Not implemented yet.");
    }

    exit_success("");

} catch (cbp::exception& e) {
    fmt::println("Terminated due to exception:\n{}", e.what());
    // we use a custom exception class with more debug info & colored formatting
} catch (std::exception& e) {
    fmt::println("Terminated due to unhandled exception:\n{}", e.what());
    // there should be no other exceptions unless we run into an 'std::bad_alloc'
}
