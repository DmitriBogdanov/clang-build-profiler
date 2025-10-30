// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "utility/replace.hpp"

#include "utility/exception.hpp"


void cbp::replace_all(std::string& str, std::string_view from, std::string_view to) {
    std::size_t i = 0;

    while ((i = str.find(from, i)) != std::string::npos) { // locate substring to replace
        str.replace(i, from.size(), to);                   // replace
        i += to.size();                                    // step over the replaced region
    }
}

void cbp::replace_all(std::string& str, const std::regex& from, std::string_view to) {
    str = std::regex_replace(str, from, std::string(to));
}

void cbp::replace_all_dynamically(std::string& str, std::string_view from, std::string_view to) {
    std::size_t i = 0;

    if (!from.empty() && from.substr(1).contains(to))
        throw cbp::exception{"Could not dynamically replace {{ {} }} to {{ {} }} in the string {{ {} }},"
                             " self-similar tokens are not allowed",
                             from, to, str};

    while ((i = str.find(from.data(), i, from.size())) != std::string::npos) { // locate substring to replace
        str.replace(i, from.size(), to);                                       // replace
        ++i;                                                                   // do NOT step over the replaced region
    }

    // Note 1: Advancing by '1' instead of 'to.size()' means we will "notice" if
    //         replacement leads to another replacement opportunity, for example,
    //         in our use case when folding angle brackets: "> > >" => ">> >" => ">>>"

    // Note 2: It is possible to come up with self-similar 'from' / 'to' which could lead to an
    //         infinite loop, for example "123" -> "0123", the check should guard against such patterns.
}

void cbp::replace_all_template(std::string& str, std::string_view from, std::string_view to) {
    if (from.empty() || from.back() != '<')
        throw cbp::exception{"Template replacement {{ {} }} to {{ {} }} is invalid", from, to};

    std::size_t i = 0;

    while ((i = str.find(from, i)) != std::string::npos) { // locate substring to replace
        // Expand replaced range until the closing '>' is found
        std::size_t match_start       = i;
        std::size_t match_end         = match_start + from.size();
        std::size_t angle_brace_count = 1;

        for (; match_end < str.size() && angle_brace_count; ++match_end) {
            if (str[match_end] == '<') ++angle_brace_count;
            else if (str[match_end] == '>') --angle_brace_count;
        } // we assume angle braces should math, otherwise the whole rest of the string will be replaced

        str.replace(i, match_end - match_start, to); // replace

        i += to.size(); // step over the replaced region
    }
}
