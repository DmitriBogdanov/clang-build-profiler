// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// Logic for serializing profiling results to a string.
// _________________________________________________________________________________

#pragma once

#include "backend/profile.hpp"

#include "external/UTL/stre.hpp"


namespace cbp {

struct formatter_state {
    std::size_t       depth{};
    cbp::microseconds timeframe{};
    std::string       str{};

    template <class... Args>
    void format(std::format_string<Args...> fmt, Args&&... args) {
        std::format_to(std::back_inserter(this->str), fmt, std::forward<Args>(args)...);
    }

    template <class... Args>
    void vformat(std::string_view fmt, Args&&... args) {
        std::vformat_to(std::back_inserter(this->str), fmt, std::forward<Args>(args)...);
    }
};

template <class callback_type>
class formatter {
    callback_type callback;

public:
    formatter_state state{};

    formatter(callback_type callback) : callback(std::move(callback)) {}

    std::string operator()(const cbp::profile& profile) try {
        this->state = formatter_state{.timeframe = profile.tree.total};
        
        this->serialize(profile.tree, profile.config);
        
        return std::move(this->state.str);
        
    } catch (std::exception& e) {
        throw cbp::exception{"Could serialize profile results to a string, error:\n{}", e.what()};
    }

private:
    void serialize(const cbp::tree& tree, const cbp::config& config) {
        // Serialize the node
        this->callback(this->state, tree);

        // Serialize its children
        ++this->state.depth;
        for (const auto& child : tree.children) this->serialize(child, config);
        --this->state.depth;
    }
};


} // namespace cbp
