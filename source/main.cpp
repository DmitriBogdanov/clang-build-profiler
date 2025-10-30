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

#include "backend/config.hpp"
#include "backend/invoke.hpp"
#include "backend/profile.hpp"
#include "frontend/mkdocs.hpp"
#include "frontend/terminal.hpp"
#include "utility/exception.hpp"
#include "utility/version.hpp"

// TEMP:
#include "utility/prettify.hpp"

[[nodiscard]] std::string normalize_path(std::filesystem::path path) {
    return path.lexically_normal().string();
    // Note: Normalizing paths leads to a nicer output, otherwise we can end
    //       up with names like 'lib/bin/../include/' instead of 'lib/include/'
}
// TEMP:


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
        .add_argument("-t", "--target")             //
        .help("Selects build artifacts directory"); //

    artifact_group                                  //
        .add_argument("-f", "--file")               //
        .help("Selects specific translation unit"); //

    cli                                                //
        .add_argument("-o", "--output")                //
        .choices("terminal", "mkdocs", "text", "json") //
        .default_value(std::string{"terminal"})        //
        .help("Selects profiling output format");      //

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

    // Set profile
    cbp::profile profile;

    profile.config = config;

    // Analyze the required file / target / build
    if (cli.is_used("--file")) {
        const std::string path = cli.get<std::string>("--file");

        std::println("\nAnalyzing translation unit {{ {} }}...\n", path);

        profile.tree = cbp::analyze_translation_unit(path);
    } else if (cli.is_used("--target")) {
        const std::string path = cli.get<std::string>("--target");

        std::println("\nAnalyzing target {{ {} }}...\n", path);

        profile.tree = cbp::analyze_target(path);
    } else {
        const std::string path = cli.get<std::string>("--build");

        std::println("\nAnalyzing CMake build {{ {} }}...\n", path);

        profile.tree = cbp::analyze_build(path);
    }

    // Prettify the results
    profile.tree.for_all([&](cbp::tree& tree) {
        // Note: This can be done better without a double pass

        // Prune nodes
        const auto it = std::remove_if(tree.children.begin(), tree.children.end(), [&](const cbp::tree& child) {
            const bool is_node   = std::to_underlying(child.type & cbp::tree_type::node);
            const bool can_prune = child.total < profile.config.tree.categorize.gray;
            return is_node && can_prune;
        });

        tree.children.erase(it, tree.children.end());

        // Categorize
        if (tree.total > profile.config.tree.categorize.red) tree.category = cbp::tree_category::red;
        else if (tree.total > profile.config.tree.categorize.yellow) tree.category = cbp::tree_category::yellow;
        else if (tree.total > profile.config.tree.categorize.white) tree.category = cbp::tree_category::white;
        else if (tree.total > profile.config.tree.categorize.gray) tree.category = cbp::tree_category::gray;

        // Prettify names
        // if (tree.type == cbp::tree_type::parse) tree.name = normalize_path(std::move(tree.name));
        // else if (tree.type == cbp::tree_type::translation_unit) tree.name = normalize_path(std::move(tree.name));
        // else if (tree.type == cbp::tree_type::instantiate) tree.name = cbp::prettify::full(std::move(tree.name));
    });

    // Serialize the output
    if (cli.get("--output") == "terminal") {
        cbp::output::terminal(profile);
    } else if (cli.get("--output") == "mkdocs") {
        cbp::output::mkdocs(profile);
    } else {
        exit_failure("Not implemented yet.");
    }
} catch (cbp::exception& e) {
    std::println("Terminated due to exception:\n{}", e.what());
    // we use a custom exception class with more debug info & colored formatting
} catch (std::exception& e) {
    std::println("Terminated due to unhandled exception:\n{}", e.what());
    // there should be no other exceptions unless we run into an 'std::bad_alloc'
}
