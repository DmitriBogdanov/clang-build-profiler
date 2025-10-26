// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "config.hpp"

#include <fstream>
#include <regex>

#include "external/fkYAML/node.hpp"

#include "colors.hpp"
#include "exception.hpp"


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

        if (tree.contains("enabled")) config.tree.enabled = tree.at("enabled").as_bool();

        if (tree.contains("categorize")) {
            config.tree.categorize.clear();

            for (const auto& node : tree.at("categorize").as_seq()) {
                config.tree.categorize.push_back(
                    {.duration = cbp::milliseconds{node.at("duration").as_int()}, .color = node.at("color").as_str()});
            }
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

std::string cbp::config::to_string() const {
    fkyaml::node root;

    root["version"]         = this->version;
    root["tree"]["enabled"] = this->tree.enabled;

    auto& replace_prefix_node = (root["tree"]["replace_prefix"] = fkyaml::node::sequence());

    for (const auto& replacement : this->tree.replace_prefix) {
        fkyaml::node node;
        node["from"] = replacement.from;
        node["to"]   = replacement.to;

        replace_prefix_node.as_seq().emplace_back(std::move(node));
    }
    // Note: This is verbose. Is there a better way of serializing a sequence?

    return fkyaml::node::serialize(root);
}

void cbp::config::to_file(std::string_view path) const { std::ofstream(std::string(path)) << this->to_string(); }

// Function for validating the config & making user-friendly error messages
std::optional<std::string> cbp::config::validate() const {

    // Validate version
    if (!std::regex_match(this->version, std::regex{R"(^\d*\.\d*\.\d*)"})) {
        constexpr auto fmt = "'version' has a value {{ {} }}, which doesn't the schema <major>.<minor>.<patch>";
        return std::format(fmt, this->version);
    }

    // Validate tree categorization
    cbp::milliseconds prev_duration      = cbp::milliseconds::max();
    std::size_t       categorization_pos = 0;

    for (const auto& [duration, color] : this->tree.categorize) {
        // Validate ordering
        if (duration > prev_duration) {
            constexpr auto fmt = "'tree.categorize' contains entries in the wrong order, each entry should have a "
                                 "smaller 'duration' than the previous one, yet entry {} has duration {{ {} }}, which "
                                 "is greaten than the previous {{ {} }}";

            return std::format(fmt, categorization_pos, duration, prev_duration);
        } else {
            prev_duration = duration;
        }

        // Validate color
        try {
            [[maybe_unused]] const auto code = cbp::color_from_name(color);
        } catch (...) {
            constexpr auto fmt = "'tree.categorize' contains an invalid color {{ {} }} at position {}";
            return std::format(fmt, color, categorization_pos);
        }

        ++categorization_pos;
    }

    // Validate tree replacement prefixes
    // std::size_t replace_prefix_pos = 0;

    // for (const auto& [from, to] : this->tree.replace_prefix) {
    //     try {
    //         [[maybe_unused]] const auto regex = std::regex{from};
    //     } catch (...) {
    //         constexpr auto fmt = "'tree.replace_prefix' contains an invalid 'from' regex {{ {} }} at position {}";
    //         return std::format(fmt, from, replace_prefix_pos);
    //     }

    //     try {
    //         [[maybe_unused]] const auto regex = std::regex{to};
    //     } catch (...) {
    //         constexpr auto fmt = "'tree.replace_prefix' contains an invalid 'to' regex {{ {} }} at position {}";
    //         return std::format(fmt, to, replace_prefix_pos);
    //     }

    //     ++replace_prefix_pos;
    // }

    return std::nullopt;
}
