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

// void replace_all(std::string& str, const std::regex& from, std::string_view to) {
//     str = std::regex_replace(str, from, std::string(to));
// }

void replace_all(std::string& str, const std::regex& from, std::string_view to) {
    std::smatch match;
    std::size_t i = 0;

    while (std::regex_search(str.cbegin() + i, str.cend(), match, from)) { // locate substring to replace
        str.replace(i + match.position(), match.length(), to);             // replace
        i += match.position() + to.size();                                 // step over the replaced region
    }
}

void replace_all_dynamically(std::string& str, std::string_view from, std::string_view to) {
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
// (licensed MIT), it was rewritten to fit a more modern style, its replacement rules were significantly extended
// and regex simplified to make things faster since standard library is notoriously slow at evaluating complex regex.

std::string cbp::symbol::prettify(std::string symbol) {

    // Normalize angle brackets: "> >" -> ">>"
    replace_all_dynamically(symbol, "> >", ">>");

    // Normalize pointer & reference spacing
    replace_all(symbol, std::regex{R"(\s*\*)"}, "*");
    replace_all(symbol, std::regex{R"(\s*&)"}, "&");

    // Normalize commas: "," or " ," -> ", "
    replace_all(symbol, std::regex{R"(\s*,\s*)"}, ", ");

    // Normalize "class whatever" -> "whatever"
    replace_all(symbol, std::regex{R"(\b(class|struct)\s+)"}, "");
    // this normalizes MSVC relative to the other compilers

    // Normalize "`anonymous namespace'" -> "(anonymous namespace)"
    replace_all(symbol, "`anonymous namespace'", "(anonymous namespace)");
    // this normalizes MSVC relative to the other compilers

    // Replace "std::_something::" -> "std::"
    replace_all(symbol, std::regex{R"(std(::_[a-zA-Z0-9_]+)?::)"}, "std::");
    // usually this removes stuff like "std::__1::", "std::__cxx11::" and etc.

    // Remove "std::allocator<whatever>"
    replace_all_template(symbol, std::regex{R"(, std::allocator<)"}, "");
    // it is technically a lossy conversion, but 99% of the time what we want is to remove default allocator

    // Remove "std::default_delete<whatever>"
    replace_all_template(symbol, std::regex{R"(, std::default_delete<)"}, "");
    // it is technically a lossy conversion, but 99% of the time what we want is to remove default deleter

    // Replace "std::basic_something<char>" -> "std::something"
    replace_all(symbol, "std::basic_string<char>", "std::string");
    replace_all(symbol, "std::basic_string_view<char>", "std::string_view");
    replace_all(symbol, "std::basic_regex<char>", "std::regex");

    // '<format>' template simplifications
    replace_all(symbol, "std::basic_format_string<char>", "std::format_string");
    replace_all(symbol, "std::basic_format_parse_context<char>", "std:format_parse_context");
    replace_all(symbol, "std::basic_format_args<std::format_context>", "std::format_args");
    // TODO: refine and complete this section, some templates require non-trivial work to simplify

    // Replace "std::ratio<1, 10^N>" with standard ratios
    replace_all(symbol, "std::ratio<1, 1000000000000>", "std::pico");
    replace_all(symbol, "std::ratio<1, 1000000000>", "std::nano");
    replace_all(symbol, "std::ratio<1, 1000000>", "std::micro");
    replace_all(symbol, "std::ratio<1, 1000>", "std::milli");
    replace_all(symbol, "std::ratio<1000, 1>", "std::kilo");
    replace_all(symbol, "std::ratio<1000000, 1>", "std::mega");
    replace_all(symbol, "std::ratio<1000000000, 1>", "std::giga");
    replace_all(symbol, "std::ratio<1000000000000, 1>", "std::tera");

    // Replace "std::chrono::duration<rep, ratio>" with standard duration units
    replace_all(symbol, "std::chrono::duration<long long, std::nano>", "std::chrono::nanoseconds");
    replace_all(symbol, "std::chrono::duration<long long, std::micro>", "std::chrono::microseconds");
    replace_all(symbol, "std::chrono::duration<long long, std::milli>", "std::chrono::milliseconds");
    replace_all(symbol, "std::chrono::duration<long long>", "std::chrono::seconds");
    replace_all(symbol, "std::chrono::duration<long, std::ratio<60>>", "std::chrono::minutes");
    replace_all(symbol, "std::chrono::duration<long, std::ratio<3600>", "std::chrono::hours");
    replace_all(symbol, "std::chrono::duration<int, std::ratio<86400>>", "std::chrono::days");
    replace_all(symbol, "std::chrono::duration<int, std::ratio<604800>>", "std::chrono::weeks");
    replace_all(symbol, "std::chrono::duration<int, std::ratio<2629746>>", "std::chrono::months");
    replace_all(symbol, "std::chrono::duration<int, std::ratio<31556952>>", "std::chrono::years");

    // Shorten explicitly expanded transparent functors
    replace_all(symbol, "std::plus<void>", "std::plus<>");
    replace_all(symbol, "std::minus<void>", "std::minus<>");
    replace_all(symbol, "std::multiplies<void>", "std::multiplies<>");
    replace_all(symbol, "std::divides<void>", "std::divides<>");
    replace_all(symbol, "std::modulus<void>", "std::modulus<>");
    replace_all(symbol, "std::negate<void>", "std::negate<>");
    replace_all(symbol, "std::equal_to<void>", "std::equal_to<>");
    replace_all(symbol, "std::not_equal_to<void>", "std::not_equal_to<>");
    replace_all(symbol, "std::greater<void>", "std::greater<>");
    replace_all(symbol, "std::less<void>", "std::less<>");
    replace_all(symbol, "std::greater_equal<void>", "std::greater_equal<>");
    replace_all(symbol, "std::less_equal<void>", "std::less_equal<>");

    // TODO: Simplification for lambdas like
    // "something<(lambda at
    // /home/georgehaldane/Documents/PROJECTS/CPP/clang-build-profiler/proj/include/external/argparse/argparse.hpp:701:12)>"
    // -> "something<(lambda at argparse.hpp:701)>"

    // Remove ABI suffixes like "[abi:ne210103]"
    replace_all(symbol, std::regex{R"(\[abi:[a-zA-Z0-9]+\])"}, "");

    return symbol;
}
