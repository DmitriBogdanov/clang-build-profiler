// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "prettify.hpp"

#include <cassert>
#include <regex>


void replace_all(std::string& str, std::string_view from, std::string_view to) {
    std::size_t i = 0;

    while ((i = str.find(from, i)) != std::string::npos) { // locate substring to replace
        str.replace(i, from.size(), to);                   // replace
        i += to.size();                                    // step over the replaced region
    }
}

void replace_all(std::string& str, const std::regex& from, std::string_view to) {
    std::smatch match;
    std::size_t i = 0;

    while (std::regex_search(str.cbegin() + i, str.cend(), match, from)) { // locate substring to replace
        str.replace(i + match.position(), match.length(), to);             // replace
        i += match.position() + to.size();                                 // step over the replaced region
    }
}

void replace_all_while_possible(std::string& str, std::string_view from, std::string_view to) {
    std::size_t i = 0;

    assert(from.empty() || !from.substr(1).contains(to));

    while ((i = str.find(from.data(), i, from.size())) != std::string::npos) { // locate substring to replace
        str.replace(i, from.size(), to);                                       // replace
        ++i;                                                                   // do NOT step over the replaced region
    }

    // Note 1: Advancing by '1' instead of 'to.size()' means we will "notice" if
    //         replacement leads to another replacement opportunity, for example,
    //         in our use case when folding angle brackets: "> > >" => ">> >" => ">>>"

    // Note 2: It is possible to come up with self-similar 'from' / 'to' which could lead to an
    //         infinite loop, for example "123" -> "123456", the assert should guard against such patterns.
}

void replace_all_template(std::string& str, const std::regex& from, std::string_view to) {
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

// Prettifier implementation initially inspired by symbol cleanup from https://github.com/jeremy-rifkin/cpptrace
// (licensed MIT), it was rewritten to fit a more modern style and its replacement rules were extended & simplified
// to make regex a little faster since standard library is notoriously slow at evaluating complex regex.

std::string cbp::symbol::prettify(std::string symbol) {  
    // Normalize angle brackets: "> >" -> ">>"
    replace_all_while_possible(symbol, "> >", ">>");
    
    // Normalize commas: "," or " ," -> ", "
    replace_all(symbol, std::regex{R"(\s*,\s*)"}, ", ");
    
    // Normalize "class whatever" -> "whatever"
    replace_all(symbol, std::regex{R"(\b(class|struct)\s+)"}, "");
    // this normalizes MSVC relative to the other compilers
    
    // Normalize "`anonymous namespace'" -> "(anonymous namespace)"
    replace_all(symbol, std::regex{"`anonymous namespace'"}, "(anonymous namespace)");
    // this normalizes MSVC relative to the other compilers

    // Replace "std::_something::" -> "std::"
    replace_all(symbol, std::regex{R"(std(::_[a-zA-Z0-9_]+)?::)"}, "std::");
    // usually this removes stuff like "std::__1::", "std::__cxx11::" and etc.
    
    // TODO: Test and perhaps remove the rules for 'std::__something::'
    
    // Replace "std::__something::basic_string" -> "std::string"
    replace_all_template(symbol, std::regex{R"(std(::[a-zA-Z0-9_]+)?::basic_string<char)"}, "std::string");

    // Replace "std::__something::basic_string_view" -> "std::string_view"
    replace_all_template(symbol, std::regex{R"(std(::[a-zA-Z0-9_]+)?::basic_string_view<char)"}, "std::string_view");

    // Remove "std::__something::allocator<whatever>"
    replace_all_template(symbol, std::regex{R"(,\s*std(::[a-zA-Z0-9_]+)?::allocator<)"}, "");
    // it is technically a lossy conversion, but 99% of the time what we want is to remove default allocator
    
    // Remove "std::__something::default_delete<whatever>"
    replace_all_template(symbol, std::regex{R"(,\s*std(::[a-zA-Z0-9_]+)?::default_delete<)"}, "");
    // it is technically a lossy conversion, but 99% of the time what we want is to remove default deleter

    // Remove ABI suffixes like "[abi:ne210103]"
    replace_all(symbol, std::regex{R"(\[abi:[a-zA-Z0-9]+\])"}, "");
    // these are usually encountered after function names

    return symbol;
}
