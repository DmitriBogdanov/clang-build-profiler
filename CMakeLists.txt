cmake_minimum_required(VERSION 3.21.0...3.22.1)
# should work for all newer versions, specified range is the based in the highest version tested in practice

project(
    clang-build-profiler
    VERSION 0.1.0
    DESCRIPTION "C++ build report generator based on clang time traces."
    HOMEPAGE_URL "https://github.com/DmitriBogdanov/clang-build-report"
)

# Create target for embedded resources
include(cmake/CMakeRC.cmake)

cmrc_add_resource_library(
    clang-build-profiler-resources
    NAMESPACE cbp
    "resources/html/report.html"
    "resources/mkdocs/mkdocs.yml"
    "resources/mkdocs/docs/images/favicon.svg"
    "resources/mkdocs/docs/admonitions.css"
    "resources/mkdocs/docs/classes.css"
    "resources/mkdocs/docs/width.css"
)

target_compile_features(clang-build-profiler-resources PRIVATE ${CBP_COMPILE_FEATURES})
target_compile_options (clang-build-profiler-resources PRIVATE ${CBP_COMPILER_FLAGS}  )
target_link_options    (clang-build-profiler-resources PRIVATE ${CBP_LINKER_FLAGS}    )

# Create clang-build-profiler library
add_library(clang-build-profiler-lib)

target_include_directories(
    clang-build-profiler-lib PUBLIC
    "include/"
    "include/external/"
)

target_sources(
    clang-build-profiler-lib PUBLIC
    "source/backend/analyze.cpp"
    "source/backend/config.cpp"
    "source/backend/invoke.cpp"
    "source/frontend/html.cpp"
    "source/frontend/json.cpp"
    "source/frontend/mkdocs.cpp"
    "source/frontend/preprocessor.cpp"
    "source/frontend/terminal.cpp"
    "source/frontend/text.cpp"
    "source/utility/demangle.cpp"
    "source/utility/embedded.cpp"
    "source/utility/filepath.cpp"
    "source/utility/lookup.cpp"
    "source/utility/prettify.cpp"
    "source/utility/replace.cpp"
    "source/utility/version.cpp"
    "source/external/fmt/format.cc"
)

target_compile_features(clang-build-profiler-lib PRIVATE ${CBP_COMPILE_FEATURES})
target_compile_options (clang-build-profiler-lib PRIVATE ${CBP_COMPILER_FLAGS}  )
target_link_options    (clang-build-profiler-lib PRIVATE ${CBP_LINKER_FLAGS}    )

target_link_libraries(clang-build-profiler-lib PUBLIC clang-build-profiler-resources)

# Create the main executable
add_executable(clang-build-profiler "source/main.cpp")

target_compile_features(clang-build-profiler PRIVATE ${CBP_COMPILE_FEATURES})
target_compile_options (clang-build-profiler PRIVATE ${CBP_COMPILER_FLAGS}  )
target_link_options    (clang-build-profiler PRIVATE ${CBP_LINKER_FLAGS}    )

target_link_libraries(clang-build-profiler PRIVATE clang-build-profiler-lib)

# Build tests if project is top-level
if("${CMAKE_SOURCE_DIR}" STREQUAL "${PROJECT_SOURCE_DIR}")
    include(CTest)
    enable_testing()
    
    add_executable(clang-build-profiler-tests "tests/main.cpp")
    
    target_compile_features(clang-build-profiler-tests PRIVATE ${CBP_COMPILE_FEATURES})
    target_compile_options (clang-build-profiler-tests PRIVATE ${CBP_COMPILER_FLAGS}  )
    target_link_options    (clang-build-profiler-tests PRIVATE ${CBP_LINKER_FLAGS}    )
    
    target_link_libraries(clang-build-profiler-tests PRIVATE clang-build-profiler-lib)
endif()
