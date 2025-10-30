// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "utility/prettify.hpp"

#include "utility/replace.hpp"


// Prettifier implementation initially inspired by identifier cleanup from https://github.com/jeremy-rifkin/cpptrace
// (licensed MIT), it was rewritten to fit a more modern style, its replacement rules were significantly extended
// and regex simplified to make things faster since standard library is notoriously slow at evaluating complex regex.

std::string cbp::prettify::normalize(std::string identifier) {
    // Normalize angle brackets: "> >" -> ">>"
    replace_all_dynamically(identifier, "> >", ">>");

    // Normalize pointer & reference spacing
    replace_all(identifier, std::regex{R"(\s*\*)"}, "*");
    replace_all(identifier, std::regex{R"(\s*&)"}, "&");

    // Normalize commas: "," or " ," -> ", "
    replace_all(identifier, std::regex{R"(\s*,\s*)"}, ", ");

    // Normalize "class whatever" -> "whatever"
    replace_all(identifier, std::regex{R"(\b(class|struct)\s+)"}, "");
    // normalizes MSVC relative to the other compilers

    // Normalize "`anonymous namespace'" -> "(anonymous namespace)"
    replace_all(identifier, "`anonymous namespace'", "(anonymous namespace)");
    // normalizes MSVC relative to the other compilers

    return identifier;
}

std::string cbp::prettify::deobfuscate(std::string identifier) {
    // Replace "std::_something::" -> "std::"
    replace_all(identifier, std::regex{R"(std(::_[a-zA-Z0-9_]+)?::)"}, "std::");
    // usually this removes stuff like "std::__1::", "std::__cxx11::" and etc.

    // Remove ABI suffixes like "[abi:ne210103]"
    replace_all(identifier, std::regex{R"(\[abi:[a-zA-Z0-9]+\])"}, "");

    return identifier;
}

std::string cbp::prettify::collapse(std::string identifier) {
    // Remove "std::allocator<whatever>"
    replace_all_template(identifier, std::regex{R"(, std::allocator<)"}, "");
    // it is technically a lossy conversion, but 99% of the time what we want is to remove default allocator

    // Remove "std::default_delete<whatever>"
    replace_all_template(identifier, std::regex{R"(, std::default_delete<)"}, "");
    // it is technically a lossy conversion, but 99% of the time what we want is to remove default deleter

    // Replace "std::basic_something<char>" -> "std::something"
    replace_all(identifier, "std::basic_string<char>", "std::string");
    replace_all(identifier, "std::basic_string_view<char>", "std::string_view");
    replace_all(identifier, "std::basic_regex<char>", "std::regex");

    // '<format>' template simplifications
    replace_all(identifier, "std::basic_format_string<char>", "std::format_string");
    replace_all(identifier, "std::basic_format_parse_context<char>", "std:format_parse_context");
    replace_all(identifier, "std::basic_format_args<std::format_context>", "std::format_args");
    // TODO: refine and complete this section, some templates require non-trivial work to simplify

    // Replace "std::ratio<1, 10^N>" with standard ratios
    replace_all(identifier, "std::ratio<1, 1000000000000>", "std::pico");
    replace_all(identifier, "std::ratio<1, 1000000000>", "std::nano");
    replace_all(identifier, "std::ratio<1, 1000000>", "std::micro");
    replace_all(identifier, "std::ratio<1, 1000>", "std::milli");
    replace_all(identifier, "std::ratio<1000, 1>", "std::kilo");
    replace_all(identifier, "std::ratio<1000000, 1>", "std::mega");
    replace_all(identifier, "std::ratio<1000000000, 1>", "std::giga");
    replace_all(identifier, "std::ratio<1000000000000, 1>", "std::tera");

    // Replace "std::chrono::duration<rep, ratio>" with standard duration units
    replace_all(identifier, "std::chrono::duration<long long, std::nano>", "std::chrono::nanoseconds");
    replace_all(identifier, "std::chrono::duration<long long, std::micro>", "std::chrono::microseconds");
    replace_all(identifier, "std::chrono::duration<long long, std::milli>", "std::chrono::milliseconds");
    replace_all(identifier, "std::chrono::duration<long long>", "std::chrono::seconds");
    replace_all(identifier, "std::chrono::duration<long, std::ratio<60>>", "std::chrono::minutes");
    replace_all(identifier, "std::chrono::duration<long, std::ratio<3600>", "std::chrono::hours");
    replace_all(identifier, "std::chrono::duration<int, std::ratio<86400>>", "std::chrono::days");
    replace_all(identifier, "std::chrono::duration<int, std::ratio<604800>>", "std::chrono::weeks");
    replace_all(identifier, "std::chrono::duration<int, std::ratio<2629746>>", "std::chrono::months");
    replace_all(identifier, "std::chrono::duration<int, std::ratio<31556952>>", "std::chrono::years");

    // Shorten explicitly expanded transparent functors
    replace_all(identifier, "std::plus<void>", "std::plus<>");
    replace_all(identifier, "std::minus<void>", "std::minus<>");
    replace_all(identifier, "std::multiplies<void>", "std::multiplies<>");
    replace_all(identifier, "std::divides<void>", "std::divides<>");
    replace_all(identifier, "std::modulus<void>", "std::modulus<>");
    replace_all(identifier, "std::negate<void>", "std::negate<>");
    replace_all(identifier, "std::equal_to<void>", "std::equal_to<>");
    replace_all(identifier, "std::not_equal_to<void>", "std::not_equal_to<>");
    replace_all(identifier, "std::greater<void>", "std::greater<>");
    replace_all(identifier, "std::less<void>", "std::less<>");
    replace_all(identifier, "std::greater_equal<void>", "std::greater_equal<>");
    replace_all(identifier, "std::less_equal<void>", "std::less_equal<>");

    // TODO: Simplification for lambdas like
    // "something<(lambda at
    // /home/georgehaldane/Documents/PROJECTS/CPP/clang-build-profiler/proj/include/external/argparse/argparse.hpp:701:12)>"
    // -> "something<(lambda at argparse.hpp:701)>"

    return identifier;
}

std::string cbp::prettify::full(std::string identifier) {
    return collapse(deobfuscate(normalize(std::move(identifier)))); // order matters
}
