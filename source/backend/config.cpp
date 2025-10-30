// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "backend/config.hpp"

#include <fstream>
#include <regex>

#include "external/fkYAML/node.hpp"

#include "utility/colors.hpp"
#include "utility/exception.hpp"


// More or less the fastest way of reading a text file, implementation taken from
// 'utl::json': https://github.com/DmitriBogdanov/UTL/blob/master/include/UTL/json.hpp
[[nodiscard]] std::string read_file_to_string(const std::string& path) {
    using namespace std::string_literals;

    std::ifstream file(path, std::ios::ate | std::ios::binary); // open file and immediately seek to the end
    // opening file as binary allows us to skip pointless newline re-encoding
    if (!file.good()) throw cbp::exception("Could not open file {{ {} }}", path);

    const auto file_size = file.tellg(); // returns cursor pos, which is the end of file
    file.seekg(std::ios::beg);           // seek to the beginning
    std::string chars(file_size, 0);     // allocate string of appropriate size
    file.read(chars.data(), file_size);  // read into the string
    return chars;
}

cbp::config cbp::config::from_string(std::string_view str) try {
    const fkyaml::node root = fkyaml::node::deserialize(str);

    cbp::config config;

    if (root.contains("tree")) {
        const auto& tree = root.at("tree");

        if (tree.contains("categorize")) {
            const auto& categorize = tree.at("categorize");

            if (categorize.contains("gray"))
                config.tree.categorize.red = cbp::milliseconds{categorize.at("gray").as_int()};
            if (categorize.contains("white"))
                config.tree.categorize.red = cbp::milliseconds{categorize.at("white").as_int()};
            if (categorize.contains("yellow"))
                config.tree.categorize.red = cbp::milliseconds{categorize.at("yellow").as_int()};
            if (categorize.contains("red"))
                config.tree.categorize.red = cbp::milliseconds{categorize.at("red").as_int()};
        }

        if (tree.contains("replace_prefix")) {
            config.tree.replace_prefix.clear();

            for (const auto& node : tree.at("replace_prefix").as_seq())
                config.tree.replace_prefix.push_back({.from = node.at("from").as_str(), .to = node.at("to").as_str()});
        }
    }

    return config;

} catch (std::exception& e) { throw cbp::exception{"Could not parse config error:\n{}", e.what()}; }

cbp::config cbp::config::from_file(std::string_view path) {
    return cbp::config::from_string(read_file_to_string(std::string(path)));
}

// Function for validating the config & making user-friendly error messages
std::optional<std::string> cbp::config::validate() const {

    // Validate version
    if (!std::regex_match(this->version, std::regex{R"(^\d*\.\d*\.\d*)"})) {
        constexpr auto fmt = "'version' has a value {{ {} }}, which doesn't the schema <major>.<minor>.<patch>";
        return std::format(fmt, this->version);
    }

    // Validate tree categorization
    const std::int64_t threshold_1 = this->tree.categorize.gray.count();
    const std::int64_t threshold_2 = this->tree.categorize.white.count();
    const std::int64_t threshold_3 = this->tree.categorize.yellow.count();
    const std::int64_t threshold_4 = this->tree.categorize.red.count();

    const bool order_is_correct =
        (threshold_1 < threshold_2) && (threshold_2 < threshold_3) && (threshold_3 < threshold_4);
        
    if (!order_is_correct) return "'tree.categorize' contains durations in the incorrect order.";

    return std::nullopt;
}
