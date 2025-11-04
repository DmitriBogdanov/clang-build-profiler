# Technical limitations

[<- to README.md](..)

## Transitive include attribution

It is possible that multiple headers `h1.hpp` and `h2.hpp` transitively include the same header `heavy.hpp`.

Since include guards prevent headers from being parsed twice, profiling results will attribute the time of `heavy.hpp` to either `h1.hpp` or `h2.hpp`, depending on which one got parsed first.

This means that if we want to eliminate `heavy.hpp` from the include tree, we might need to tweak code & generate the profile multiple times in order to locate all the headers responsible for bringing it in.

## Implementation parsing time

Since `-ftime-trace` only exposes parsing events for headers, we have no real way to deduce the parsing time of `.cpp` itself.

In practice this is not a particularly important limitation since the overwhelming majority of parsing is going to be spent on includes.

## Generalizing template alias prettification

Debug info in C++ usually contains templates in their fully expanded form, for example, a seemingly short template like `std::string` when fully expanded will look more like `std::basic_string<char, std::char_traits<char>, std::allocator<char>>`. In some cases this representation may also include additional implementation-specific subtypes and typedefs.

For the standard library templates, `clang-build-profiler` contains an extensive number of simplification rules:

- Implementation-specific spacing and formatting is normalized
- Implementations-specific namespaces (like `std::__cxx11::`) are collapsed
- Known expansions (like `std::ratio<1, 1000>`, `std::chrono::duration<long long, std::ratio<1, 1000000>>`, `std::basic_ifstream<char>` and etc.) are collapsed to their shorter form (`std::milli`, `std::chrono::milliseconds`, `std::ifstream` and etc.)
- Known default arguments are collapsed (like `std::less<void>` which is equivalent to `std::less<>`)
- ABI suffixes are removed

This greatly aids in terms of readability, however doing the same logic in general case is not really possible without analyzing the actual compiler AST, which is why 3rd party libraries and some standard implementations might still contain needlessly expanded templates.

## Granular codegen data

Since `-ftime-trace` does not expose any events that could help us attribute code generation time to a particular class / function / file, this information is only available in terms of a general statistic.

## Preprocessing time

Since `-ftime-trace` does not separate preprocessing from the general parsing, any time spend in the preprocessor (which can be quite significant for some heavy recursive macros) will be attributed to the `Parsing` section.
