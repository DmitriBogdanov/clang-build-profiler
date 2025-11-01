// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "utility/prettify.hpp"

#include <filesystem>

#include "utility/filepath.hpp"
#include "utility/replace.hpp"


// Prettifier implementation initially inspired by identifier cleanup from https://github.com/jeremy-rifkin/cpptrace
// (licensed MIT), it was rewritten to fit a more modern style, its replacement rules were significantly extended
// and regex simplified to make things faster since standard library is notoriously slow at evaluating complex regex.

// --- Compiler format normalization ---
// -------------------------------------

void normalize_angle_brackets(std::string& identifier) {
    // Normalize angle brackets: "> >" -> ">>"
    cbp::replace_all_dynamically(identifier, "> >", ">>");
}

void normalize_pointer_spacing(std::string& identifier) {
    // Normalize "type *" / "type &" to "type*" / "type&"
    cbp::replace_all(identifier, " *", "*");
    cbp::replace_all(identifier, " &", "&");
}

void normalize_comma_spacing(std::string& identifier) {
    // Normalize commas: "," or " ," -> ", "
    cbp::replace_all(identifier, " ,", ","); // removes spaces on the left
    cbp::replace_all(identifier, ", ", ","); // removes spaces on the right
    
    cbp::replace_all(identifier, ",", ", "); // adds spaces on the right
}

void normalize_classes(std::string& identifier) {
    // Normalize "class whatever" -> "whatever"
    cbp::replace_all(identifier, boost::regex{R"(\b(class|struct)\s+)"}, "");
    // normalizes MSVC relative to the other compilers
}

void normalize_anonymous_namespace(std::string& identifier) {
    // Normalize "`anonymous namespace'" -> "(anonymous namespace)"
    cbp::replace_all(identifier, "`anonymous namespace'", "(anonymous namespace)");
    // normalizes MSVC relative to the other compilers
}

std::string cbp::prettify::normalize(std::string identifier) {
    normalize_angle_brackets(identifier);
    normalize_pointer_spacing(identifier);
    normalize_comma_spacing(identifier);
    normalize_classes(identifier);
    normalize_anonymous_namespace(identifier);

    return identifier;
}

// --- Implementation quirks deobfuscation ---
// -------------------------------------------

void deobfuscate_std_namespace(std::string& identifier) {
    // Replace "std::_something::" -> "std::"
    cbp::replace_all(identifier, boost::regex{R"(std(::_[a-zA-Z0-9_]+)?::)"}, "std::");
    // usually this removes stuff like "std::__1::", "std::__cxx11::" and etc.
}

void deobfuscate_abi_suffixes(std::string& identifier) {
    // Remove ABI suffixes like "[abi:ne210103]"
    cbp::replace_all(identifier, boost::regex{R"(\[abi:[a-zA-Z0-9]+\])"}, "");
}

std::string cbp::prettify::deobfuscate(std::string identifier) {
    deobfuscate_std_namespace(identifier);
    deobfuscate_abi_suffixes(identifier);

    return identifier;
}

// --- Collapse template alias ---
// -------------------------------

void collapse_default_traits(std::string& identifier) {
    cbp::replace_all_template(identifier, ", std::allocator<", "");
    cbp::replace_all_template(identifier, ", std::default_delete<", "");
    cbp::replace_all_template(identifier, ", std::char_traits<", "");
    // this is technically a lossy conversion, but 99% of the time what we want is to remove default
    // allocator / deleter / char_traits, most instances where allocators are explicitly passed to a
    // library (e.g. 'some_class<std::allocator<int>>') will not be collapsed since they lack comma
}

void collapse_string(std::string& identifier) {
    // Replace "std::basic_sting<char_type>" -> "std::string_type"
    cbp::replace_all(identifier, "std::basic_string<char>", "std::string");
    cbp::replace_all(identifier, "std::basic_string<wchar_t>", "std::wstring");
    cbp::replace_all(identifier, "std::basic_string<char8_t>", "std::u8string");
    cbp::replace_all(identifier, "std::basic_string<char16_t>", "std::u16string");
    cbp::replace_all(identifier, "std::basic_string<char32_t>", "std::u32string");
    cbp::replace_all(identifier, "std::basic_string_view<char>", "std::string_view");
    cbp::replace_all(identifier, "std::basic_string_view<wchar_t>", "std::wstring_view");
    cbp::replace_all(identifier, "std::basic_string_view<char8_t>", "std::u8string_view");
    cbp::replace_all(identifier, "std::basic_string_view<char16_t>", "std::u16string_view");
    cbp::replace_all(identifier, "std::basic_string_view<char32_t>", "std::u32string_view");
}

void collapse_regex(std::string& identifier) {
    // Replace "std::basic_regex<char_type>" -> "boost::regex_type"
    cbp::replace_all(identifier, "std::basic_regex<char>", "boost::regex");
    cbp::replace_all(identifier, "std::basic_regex<wchar_t>", "std::wregex");
}

void collapse_ratio(std::string& identifier) {
    // Replace "std::ratio<1, 10^N>" with standard ratios
    cbp::replace_all(identifier, "std::ratio<1, 1000000000000>", "std::pico");
    cbp::replace_all(identifier, "std::ratio<1, 1000000000>", "std::nano");
    cbp::replace_all(identifier, "std::ratio<1, 1000000>", "std::micro");
    cbp::replace_all(identifier, "std::ratio<1, 1000>", "std::milli");
    cbp::replace_all(identifier, "std::ratio<1000, 1>", "std::kilo");
    cbp::replace_all(identifier, "std::ratio<1000000, 1>", "std::mega");
    cbp::replace_all(identifier, "std::ratio<1000000000, 1>", "std::giga");
    cbp::replace_all(identifier, "std::ratio<1000000000000, 1>", "std::tera");
}

void collapse_chrono(std::string& identifier) {
    // Normalize "std::chrono::unit::duration" to "std::chrono::duration"
    cbp::replace_all(identifier, "std::chrono:nanoseconds::duration", "std::chrono::duration");
    cbp::replace_all(identifier, "std::chrono:microseconds::duration", "std::chrono::duration");
    cbp::replace_all(identifier, "std::chrono:milliseconds::duration", "std::chrono::duration");
    cbp::replace_all(identifier, "std::chrono:seconds::duration", "std::chrono::duration");
    cbp::replace_all(identifier, "std::chrono:minutes::duration", "std::chrono::duration");
    cbp::replace_all(identifier, "std::chrono:hours::duration", "std::chrono::duration");
    cbp::replace_all(identifier, "std::chrono:days::duration", "std::chrono::duration");
    cbp::replace_all(identifier, "std::chrono:weeks::duration", "std::chrono::duration");
    cbp::replace_all(identifier, "std::chrono:months::duration", "std::chrono::duration");
    cbp::replace_all(identifier, "std::chrono:years::duration", "std::chrono::duration");
    // this is an LLVM typedef, which can prevent other <chrono> simplifications from working

    // Replace "std::chrono::duration<rep, ratio>" with standard duration units
    cbp::replace_all(identifier, "std::chrono::duration<long long, std::nano>", "std::chrono::nanoseconds");
    cbp::replace_all(identifier, "std::chrono::duration<long long, std::micro>", "std::chrono::microseconds");
    cbp::replace_all(identifier, "std::chrono::duration<long long, std::milli>", "std::chrono::milliseconds");
    cbp::replace_all(identifier, "std::chrono::duration<long long>", "std::chrono::seconds");
    cbp::replace_all(identifier, "std::chrono::duration<long, std::ratio<60>>", "std::chrono::minutes");
    cbp::replace_all(identifier, "std::chrono::duration<long, std::ratio<3600>>", "std::chrono::hours");
    cbp::replace_all(identifier, "std::chrono::duration<int, std::ratio<86400>>", "std::chrono::days");
    cbp::replace_all(identifier, "std::chrono::duration<int, std::ratio<604800>>", "std::chrono::weeks");
    cbp::replace_all(identifier, "std::chrono::duration<int, std::ratio<2629746>>", "std::chrono::months");
    cbp::replace_all(identifier, "std::chrono::duration<int, std::ratio<31556952>>", "std::chrono::years");
}

void collapse_format(std::string& identifier) {
    cbp::replace_all(identifier, "std::basic_format_string<char>", "std::format_string");
    cbp::replace_all(identifier, "std::basic_format_parse_context<char>", "std:format_parse_context");
    cbp::replace_all(identifier, "std::basic_format_args<std::format_context>", "std::format_args");
    // TODO: refine and complete this section, some templates require non-trivial work to simplify
}

void collapse_iostream(std::string& identifier) {
    // Collapse <fstream> streams
    cbp::replace_all(identifier, "std::basic_ifstream<char>", "std::ifstream");
    cbp::replace_all(identifier, "std::basic_ifstream<wchar_t>", "std::wifstream");
    cbp::replace_all(identifier, "std::basic_ofstream<char>", "std::ofstream");
    cbp::replace_all(identifier, "std::basic_ofstream<wchar_t>", "std::wofstream");
    cbp::replace_all(identifier, "std::basic_fstream<char>", "std::fstream");
    cbp::replace_all(identifier, "std::basic_fstream<wchar_t>", "std::wfstream");
    // Collapse <fstream> buffers
    cbp::replace_all(identifier, "std::basic_filebuf<char>", "std::filebuf");
    cbp::replace_all(identifier, "std::basic_filebuf<wchar_t>", "std::wfilebuf");
    // Collapse <istream> streams
    cbp::replace_all(identifier, "std::basic_istream<char>", "std::istream");
    cbp::replace_all(identifier, "std::basic_istream<wchar_t>", "std::wistream");
    // Collapse <ostream> streams
    cbp::replace_all(identifier, "std::basic_ostream<char>", "std::ostream");
    cbp::replace_all(identifier, "std::basic_ostream<wchar_t>", "std::wostream");
    // Collapse <sstream> streams
    cbp::replace_all(identifier, "std::basic_istringstream<char>", "std::istringstream");
    cbp::replace_all(identifier, "std::basic_istringstream<wchar_t>", "std::wistringstream");
    cbp::replace_all(identifier, "std::basic_ostringstream<char>", "std::ostringstream");
    cbp::replace_all(identifier, "std::basic_ostringstream<wchar_t>", "std::wostringstream");
    cbp::replace_all(identifier, "std::basic_stringstream<char>", "std::stringstream");
    cbp::replace_all(identifier, "std::basic_stringstream<wchar_t>", "std::wstringstream");
    // Collapse <sstream> buffers
    cbp::replace_all(identifier, "std::basic_stringbuf<char>", "std::stringbuf");
    cbp::replace_all(identifier, "std::basic_stringbuf<wchar_t>", "std::wstringbuf");
}

std::string cbp::prettify::collapse(std::string identifier) {
    collapse_default_traits(identifier);

    collapse_string(identifier);
    collapse_regex(identifier);
    collapse_ratio(identifier); // should be before chrono
    collapse_chrono(identifier);
    collapse_format(identifier);
    collapse_iostream(identifier);

    return identifier;
}

// --- Shorten verbose forms ---
// -----------------------------

void shorten_transparent_functors(std::string& identifier) {
    cbp::replace_all(identifier, "std::plus<void>", "std::plus<>");
    cbp::replace_all(identifier, "std::minus<void>", "std::minus<>");
    cbp::replace_all(identifier, "std::multiplies<void>", "std::multiplies<>");
    cbp::replace_all(identifier, "std::divides<void>", "std::divides<>");
    cbp::replace_all(identifier, "std::modulus<void>", "std::modulus<>");
    cbp::replace_all(identifier, "std::negate<void>", "std::negate<>");
    cbp::replace_all(identifier, "std::equal_to<void>", "std::equal_to<>");
    cbp::replace_all(identifier, "std::not_equal_to<void>", "std::not_equal_to<>");
    cbp::replace_all(identifier, "std::greater<void>", "std::greater<>");
    cbp::replace_all(identifier, "std::less<void>", "std::less<>");
    cbp::replace_all(identifier, "std::greater_equal<void>", "std::greater_equal<>");
    cbp::replace_all(identifier, "std::less_equal<void>", "std::less_equal<>");
    // 'void' is a default value, we can safely omit this
}

void shorten_lambda_paths(std::string& identifier) {
    constexpr std::string_view match = "(lambda at ";

    std::size_t i = 0;

    while ((i = identifier.find(match, i)) != std::string::npos) { // locate substring
        // Select path to replace
        const std::size_t path_begin = i + match.size();
        const std::size_t path_end   = identifier.find(':', path_begin);

        if (path_end == std::string::npos) return; // path name doesn't terminate, better leave it as is

        std::string path = identifier.substr(path_begin, path_end);

        const std::size_t path_size       = path.size();
        const std::string path_normalized = cbp::normalize_filepath(std::move(path));

        // Perform the replacement
        identifier.replace(path_begin, path_size, path_normalized);
        i += path_normalized.size();
    }
}

std::string cbp::prettify::shorten(std::string identifier) {
    shorten_transparent_functors(identifier);
    shorten_lambda_paths(identifier);

    return identifier;
}

// --- Full simplification ---
// ---------------------------

std::string cbp::prettify::full(std::string identifier) {
    return shorten(collapse(deobfuscate(normalize(std::move(identifier))))); // order matters
}
