cmake_minimum_required(VERSION 3.21.0...3.22.1)
# should work for all newer versions, specified range is the based in the highest version tested in practice

project(
    clang-build-profiler
    VERSION 0.1.0
    DESCRIPTION "C++ build report generator based on clang time traces."
    HOMEPAGE_URL "https://github.com/DmitriBogdanov/clang-build-report"
)

# include(FetchContent)
# FetchContent_Declare(
#   saucer
#   GIT_REPOSITORY https://github.com/saucer/saucer
#   GIT_TAG        v7.0.1
# )
# FetchContent_MakeAvailable(saucer)

add_executable(clang-build-profiler)

target_include_directories(
    clang-build-profiler PRIVATE
    "include/"
    "include/external/"
)

target_sources(
    clang-build-profiler PRIVATE
    "source/main.cpp"
    "source/backend/analyze.cpp"
    "source/backend/config.cpp"
    "source/backend/invoke.cpp"
    "source/frontend/mkdocs.cpp"
    "source/frontend/terminal.cpp"
    "source/utility/colors.cpp"
    "source/utility/demangle.cpp"
    "source/utility/prettify.cpp"
    "source/utility/replace.cpp"
    "source/utility/version.cpp"
)

target_compile_features(clang-build-profiler PRIVATE ${CR_COMPILE_FEATURES})
target_compile_options (clang-build-profiler PRIVATE ${CR_COMPILER_FLAGS}  )
target_link_options    (clang-build-profiler PRIVATE ${CR_LINKER_FLAGS}    )
