// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "utility/lookup.hpp"

#include <string>
#include <unordered_set>


bool cbp::lookup::is_standard_header(std::string_view name) {
    // Note: 'std::string_view' keys are fine as long as we create the set from literals or
    //        ensure that the string, from which the view is originating outlives the set
    
    static std::unordered_set<std::string_view> standard_headers{
        // Multi-purpose headers
        "cstdlib", "execution",
        // Language support library
        "cfloat", "climits", "compare", "contracts", "coroutine", "csetjmp", "csignal", "cstdarg", "cstddef", "cstdint",
        "exception", "initializer_list", "limits", "new", "source_location", "stdfloat", "typeindex", "typeinfo",
        "version",
        // Concepts library
        "concepts",
        // Diagnostics library
        "cassert", "cerrno", "debugging", "stacktrace", "stdexcept", "system_error",
        // Memory management library
        "memory", "memory_resource", "scoped_allocator",
        // Metaprogramming library
        "ratio", "type_traits",
        // General utilities library
        "any", "bit", "bitset", "expected", "functional", "optional", "tuple", "utility", "variant",
        // Containers library
        "array", "deque", "flat_map", "flat_set", "forward_list", "hive", "inplace_vector", "list", "map", "mdspan",
        "queue", "set", "span", "stack", "unordered_map", "unordered_set", "vector",
        // Iterators library
        "iterator",
        // Ranges library
        "generator", "ranges",
        // Algorithms library
        "algorithm", "numeric",
        // Strings library
        "cstring", "string", "string_view",
        // Text processing library
        "cctype", "charconv", "clocale", "codecvt", "cuchar", "cwchar", "cwctype", "format", "locale", "regex",
        "text_encoding",
        // Numerics library
        "cfenv", "cmath", "complex", "linalg", "numbers", "random", "simd", "valarray",
        // Time library
        "chrono", "ctime",
        // Input/output library
        "cinttypes", "cstdio", "filesystem", "fstream", "iomanip", "ios", "iosfwd", "iostream", "istream", "ostream",
        "print", "spanstream", "sstream", "streambuf", "strstream", "syncstream",
        // Concurrency support library
        "atomic", "barrier", "condition_variable", "future", "hazard_pointer", "latch", "mutex", "rcu", "semaphore",
        "shared_mutex", "stop_token", "thread"};

    return standard_headers.contains(name);
}