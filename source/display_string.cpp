// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "display_string.hpp"

#include "colors.hpp"


class string_serializer {
    std::string str{};
    std::size_t depth{};

    cbp::config       config{};
    bool              colors{};
    cbp::microseconds timeframe{};

    constexpr static std::string_view indent = "|  ";

private:
    template <class... Args>
    void format(std::format_string<Args...> fmt, Args&&... args) {
        std::format_to(std::back_inserter(this->str), fmt, std::forward<Args>(args)...);
    }

    void colorize(std::string_view color) { this->format("{}", color); }

    template <cbp::tree::node node_type>
    void serialize_node(const node_type& node) {
        // Categorize the node (determines color and whether it should be hidden)
        std::string color_name;

        for (const auto& category : this->config.tree.categorize) {
            if (node.timing.duration_total > category.duration) {
                color_name = category.color;
                break;
            }
        }

        if (color_name.empty()) return; // node is hidden, took too little time

        const auto color  = this->colors ? cbp::color_from_name(color_name) : "";
        const auto darken = this->colors ? cbp::ansi::bright_black : "";
        const auto reset  = this->colors ? cbp::ansi::reset : "";

        // Cleanup the name
        const std::string name = std::string(node.name);
        
        // TODO: Cleanup, replace_prefix

        // Serialize indent
        this->format("{}", darken);
        for (std::size_t i = 0; i < this->depth; ++i) this->format("{}", string_serializer::indent);
        this->format("{}", reset);

        // Serialize node
        const auto abs_total = cbp::time::to_ms(node.timing.duration_total);
        const auto abs_self  = cbp::time::to_ms(node.timing.duration_self);
        const auto rel_total = cbp::time::to_percentage(node.timing.duration_total, this->timeframe);
        const auto rel_self  = cbp::time::to_percentage(node.timing.duration_self, this->timeframe);
        
        constexpr auto fmt = "{}> {} ({} ms, {}%) | self ({} ms, {}%){}\n";
        this->format(fmt, color, name, abs_total, rel_total, abs_self, rel_self, reset);

        // Recursively descend down the tree
        ++this->depth;
        cbp::tree::apply_down(node, [&](const auto& child) { this->serialize_node(child); });
        --this->depth;
    }

public:
    string_serializer(const cbp::config& config, bool colors, cbp::microseconds timeframe)
        : config(config), colors(colors), timeframe(timeframe) {}

    std::string serialize_tree(const cbp::tree::tree& tree) {
        this->str.clear();
        this->serialize_node(tree.targets);
        return this->str;
    }
};

std::string cbp::display::string::serialize(const cbp::profile& results, const cbp::config& config, bool colors) {
    const auto timeframe = results.tree.targets.timing.duration_total;

    string_serializer serializer{config, colors, timeframe};
    
    return serializer.serialize_tree(results.tree);
}