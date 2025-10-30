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

void cbp::replace_all_template(std::string& str, const std::regex& from, std::string_view to) {
    std::smatch match;
    std::size_t i = 0;

    while (std::regex_search(str.cbegin() + i, str.cend(), match, from)) {
        // Find matching >
        const std::size_t match_begin = i + match.position();
        std::size_t       end         = match_begin + match.length();
        for (int c = 1; end < str.size() && c > 0; ++end) {
            if (str[end] == '<') ++c;
            else if (str[end] == '>') --c;
        }

        str.replace(match_begin, end - match_begin, to.data(), to.size()); // replace
        i = match_begin + to.size();                                       // step over the replaced region
    }
}