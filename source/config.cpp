// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "config.hpp"

#include <fstream>
#include <stdexcept>

#include "external/fkYAML/node.hpp"

#include "exception.hpp"


// More or less the fastest way of reading a text file, implementation taken from
// 'utl::json': https://github.com/DmitriBogdanov/UTL/blob/master/include/UTL/json.hpp
[[nodiscard]] std::string read_file_to_string(const std::string& path) {
    using namespace std::string_literals;

    std::ifstream file(path, std::ios::ate | std::ios::binary); // open file and immediately seek to the end
    // opening file as binary allows us to skip pointless newline re-encoding
    if (!file.good()) throw std::runtime_error("Could not open file {"s + path + "."s);

    const auto file_size = file.tellg(); // returns cursor pos, which is the end of file
    file.seekg(std::ios::beg);           // seek to the beginning
    std::string chars(file_size, 0);     // allocate string of appropriate size
    file.read(chars.data(), file_size);  // read into the string
    return chars;
}

cbp::config cbp::config::from_string(std::string_view str) {
    const fkyaml::node root = fkyaml::node::deserialize(str);

    cbp::config config;

    if (root.contains("tree")) {
        const auto& tree = root.at("tree");

        if (tree.contains("enabled")) config.tree.enabled = tree.at("enabled").as_bool();

        // TODO: parse thresholds

        if (tree.contains("replace_prefix")) {
            for (const auto& node : tree.at("replace_prefix").as_seq()) {
                std::string from = node.at("from").as_str();
                std::string to   = node.at("to").as_str();

                cbp::config::prefix_replacement replacement = {.from = std::move(from), .to = std::move(to)};

                config.tree.replace_prefix.push_back(std::move(replacement));
            }
        }
    }

    return config;
}

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
