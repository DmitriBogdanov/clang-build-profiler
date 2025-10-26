// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "display_string.hpp"

#include "external/UTL/stre.hpp"

#include "colors.hpp"
#include "exception.hpp"


class string_serializer {
    std::string str{};

    const cbp::config& config{};
    bool               colors{};
    cbp::microseconds  timeframe{};

    constexpr static std::string_view indent = "|  ";

private:
    template <class... Args>
    void format(std::format_string<Args...> fmt, Args&&... args) {
        std::format_to(std::back_inserter(this->str), fmt, std::forward<Args>(args)...);
    }

    void serialize_node(const cbp::tree::node& node) {
        // Categorize the node (determines color and whether it should be hidden)
        std::string color_name;

        for (const auto& category : this->config.tree.categorize) {
            if (node.duration_total >= category.duration) {
                color_name = category.color;
                break;
            }
        }

        if (color_name.empty()) return; // node is hidden, took too little time

        const auto color  = this->colors ? cbp::color_from_name(color_name) : "";
        const auto darken = this->colors ? cbp::ansi::bright_black : "";
        const auto reset  = this->colors ? cbp::ansi::reset : "";

        // Cleanup the name
        std::string name = node.name;

        for (const auto& replacement : this->config.tree.replace_prefix)
            name = utl::stre::replace_prefix(std::move(name), replacement.from, replacement.to);

        // Serialize indent
        this->format("{}", darken);
        for (std::size_t i = 0; i < node.depth; ++i) this->format("{}", string_serializer::indent);
        this->format("{}", reset);

        // Serialize node
        const auto abs_total = cbp::time::to_ms(node.duration_total);
        const auto abs_self  = cbp::time::to_ms(node.duration_self);
        const auto rel_total = cbp::time::to_percentage(node.duration_total, this->timeframe);
        const auto rel_self  = cbp::time::to_percentage(node.duration_self, this->timeframe);

        constexpr auto fmt = "{}> {} ({} ms, {}%) | self ({} ms, {}%){}\n";
        this->format(fmt, color, name, abs_total, rel_total, abs_self, rel_self, reset);
    }

public:
    string_serializer(const cbp::config& config, bool colors, cbp::microseconds timeframe)
        : config(config), colors(colors), timeframe(timeframe) {}

    std::string serialize_tree(const cbp::tree& tree) {
        this->str.clear();
        for (const auto& node : tree.nodes) this->serialize_node(node);
        return this->str;
    }
};

std::string cbp::display::string::serialize(const cbp::profile& profile, bool colors) try {

    if (profile.config.tree.enabled) {
        string_serializer serializer{profile.config, colors, profile.tree.root().duration_total};

        return serializer.serialize_tree(profile.tree);
    }

    return {}; // TEMP:
} catch (std::exception& e) { throw cbp::exception{"Could not serialize profile results, error:\n{}", e.what()}; }