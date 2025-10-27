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

#include "external/argparse/argparse.hpp"

#include "analyze.hpp"
#include "config.hpp"
#include "display_string.hpp"
#include "exception.hpp"
#include "version.hpp"
#include "json.hpp" // TEMP:


template <class... Args>
void exit_failure(std::format_string<Args...> fmt, Args&&... args) {
    std::println(fmt, std::forward<Args>(args)...);
    std::exit(EXIT_FAILURE);
}

template <class... Args>
void exit_success(std::format_string<Args...> fmt, Args&&... args) {
    std::println(fmt, std::forward<Args>(args)...);
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
            exit_success("{}", cli.help().str());
        });

    cli                                       //
        .add_argument("-v", "--version")      //
        .flag()                               //
        .help("Displays application version") //
        .action([&](const auto&) {            //
            exit_success("{}", version);
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

    auto& artifact_group = cli.add_mutually_exclusive_group();

    artifact_group                              //
        .add_argument("-b", "--build")          //
        .default_value(std::string{"build/"})   //
        .help("Selects CMake build directory"); //

    artifact_group                                  //
        .add_argument("-d", "--directory")          //
        .help("Selects build artifacts directory"); //

    artifact_group                                  //
        .add_argument("-f", "--file")               //
        .help("Selects specific translation unit"); //

    try {
        cli.parse_args(argc, argv);
    } catch (std::exception& e) {
        exit_failure("{}\n\n Run `clang-report --help` to see the full usage guide.", e.what());
    }

    // Parse config
    const std::string config_path = cli.get<std::string>("--config");

    const cbp::config config =
        std::filesystem::exists(config_path) ? cbp::config::from_file(config_path) : cbp::config{};
    
    if (const auto err = config.validate()) exit_failure("Config validation error:\n{}", err.value());
        
    // Handle specific translation unit
    if (cli.is_used("--file")) {
        const std::string path = cli.get<std::string>("--file");

        std::println("Analyzing translation unit {{ {} }}...", path);

        cbp::profile profile;
        profile.tree   = cbp::analyze_translation_unit(path);
        profile.config = config;

        std::println("{}", cbp::display::string::serialize(profile, true));
        
        cbp::write_file_json("tree.json", profile.tree);

        exit_success("Finished.");
    }
    // Handle artifact directory
    else if (cli.is_used("--directory")) {
        const std::string path = cli.get<std::string>("--directory");

        std::println("Analyzing build directory {{ {} }}...", path);

        // TODO:

        exit_failure("Not implemented yet.");
    }
    // Handle CMake build
    else {
        const std::string path = cli.get<std::string>("--build");

        std::println("Analyzing CMake build {{ {} }}...", path);

        // TODO:

        exit_failure("Not implemented yet.");
    }
} catch (cbp::exception& e) {
    std::println("Terminated due to exception:\n{}", e.what());
    // we use a custom exception class with more debug info & colored formatting
} catch (std::exception& e) {
    std::println("Terminated due to unhandled exception:\n{}", e.what());
    // there should be no other exceptions unless we run into an 'std::bad_alloc'
}
