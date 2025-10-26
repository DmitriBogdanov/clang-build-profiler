// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
//
// ____________________________________ CONTENT ____________________________________
//
// Template prettification. Debug info usually contains templates in a fully
// expanded verbose form, so instead of 'std::string' we will get something like
// 'std::basic_string<char, std::char_traits<char>, std::allocator<char>'. Expanded
// form can also contain ABI-specific prefixes and suffixes, internal namespaces and
// etc. By applying a bunch of replacement and normalization rules we can prettify
// the symbols so they will mostly match their expected form.
// _________________________________________________________________________________

#include <string>


namespace cbp::symbol {

[[nodiscard]] std::string prettify(std::string identifier);

}
