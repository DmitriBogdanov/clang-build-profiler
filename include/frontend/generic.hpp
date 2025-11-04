// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// Generic logic & state for serializing profiling results to a string.
// _________________________________________________________________________________

#pragma once

#include "backend/profile.hpp"


namespace cbp::output {
    
    struct string_state {
        std::size_t       depth{};
        cbp::microseconds timeframe{};
        std::string       str{};
        
        string_state(const cbp::profile& profile) : timeframe(profile.tree.total) {}
        
        template <class... Args>
        void format(std::format_string<Args...> fmt, Args&&... args) {
            std::format_to(std::back_inserter(this->str), fmt, std::forward<Args>(args)...);
        }
        
        template <class... Args>
        void vformat(std::string_view fmt, Args&&... args) {
            std::vformat_to(std::back_inserter(this->str), fmt, std::forward<Args>(args)...);
        }
    };
    
}
