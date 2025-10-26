cmake_minimum_required(VERSION 3.21.0...3.22.1)
# should work for all newer versions, specified range is the based in the highest version tested in practice

project(
    clang-build-profiler
    VERSION 0.1.0
    DESCRIPTION "C++ build report generator based on clang time traces."
    HOMEPAGE_URL "https://github.com/DmitriBogdanov/clang-build-report"
)

add_executable(
    clang-build-profiler
    "source/main.cpp"
    "source/analyze.cpp"
    "source/colors.cpp"
    "source/config.cpp"
    "source/demangle.cpp"
    "source/display_string.cpp"
    "source/prettify.cpp"
    "source/tree.cpp"
    "source/version.cpp"
)

target_include_directories(
    clang-build-profiler PRIVATE
    "include/"
    "include/external/"
)

target_compile_features(clang-build-profiler PRIVATE ${CR_COMPILE_FEATURES})
target_compile_options (clang-build-profiler PRIVATE ${CR_COMPILER_FLAGS}  )
target_link_options    (clang-build-profiler PRIVATE ${CR_LINKER_FLAGS}    )
