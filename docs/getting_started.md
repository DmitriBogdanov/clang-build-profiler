# Getting started

## Installation

Pre-compiled binaries can be found in [Releases](https://github.com/DmitriBogdanov/clang-build-profiler/releases) for following platforms:

- Windows x86-64
- Ubuntu x86-64
- MacOS x86-64
- FreeBSD x86-64

For platforms outside of the supported list, the build can be produced locally, see the [corresponding guide](building_locally.md).

In all cases, `clang-build-compiler` is a stand-alone executable with no additional dependencies.

For additional convenience it is advised to add `clang-build-compiler` to the `PATH`.

## Usage

### Basics

Compile your project using `clang++` with `-ftime-trace` flag enabled. The compiler will place JSON traces together with the object files.

To analyze a specific trace (which corresponds to a single translation unit) and output results to the terminal run:

```sh
clang-build-profiler --file=<path_to_json_trace>
```

To analyze all traces in a directory run:

```sh
clang-build-profiler --target=<path_to_directory>
```

To analyze an entire CMake build run:

```sh
clang-build-profiler --build=<path_to_cmake_build>
```

Output example:



> [!Tip]
> When invoked with no arguments, `clang-build-profiler` assumes `--build=build/` by default.

### Making the output pretty

Analyzing a large build directly in the terminal is rather impractical. Using `mkdocs` we can build a proper GUI with expandable sections.

`mkdocs` can be easily installed with `pip`:

```sh
pip install mkdocs-material          &&
pip install mkdocs-material[imaging] &&
pip install markdown-callouts        &&
pip install mkdocs-awesome-nav
```

After this we can analyze the build:

```sh
clang-build-profiler --output=mkdocs
```

And visualize it in the browser using `mkdocs`:

```sh
(cd .cbp && mkdocs serve --open)
```

Output example:

<img src="images/showcase_mkdocs.png">

> [!Tip]
> `pip` together with `python3` can be installed using `apt` or grabbed directly from the [official website](https://www.python.org/downloads/):
>
> ```sh
> sudo apt update &&
> sudo apt install python3
> ```

### Configuration file

Following the convention of other LLVM tools, `clang-build-profiler` uses a YAML config `.clang-build-profiler` placed into the project root.

This config contains project-specific settings such as coloring, name simplification rules and etc.

Below in a default config used by the program:

```yaml title="clang-build-profiler"
---
version: "0.1.0"

# The main profiling output
tree:
  # How to color the tree nodes
  categorize:
    gray  :  90 # duration >  50 ms => color gray
    white : 150 # duration > 100 ms => color white
    yellow: 300 # duration > 150 ms => color yellow
    red   : 800 # duration > 300 ms => color red
    
    # Note: nodes without a color are hidden, to disable all pruning set 'gray' to '0'
  
  # Regex replacement rules to reduce filename verbosity
  replace_prefix:
    - from: "/usr/lib/llvm-21/include/c++/v1/"
      to  : "std/"
    - from: "/home/georgehaldane/Documents/PROJECTS/CPP/clang-build-profiler/proj/"
      to  : ""

# Summary of header files that took the longest time in total to parse
parsing_summary:
  enabled : true
  quantity: 100

# Summary of templates that took the longest time in total to instantiate
instantiation_summary:
  enabled : true
  quantity: 100

# Details about the tool invocation - time, config and etc.
environment:
  enabled: true
```

To serialize this config for modification we can run:

```sh
clang-build-profiler --write-config
```

Which will write a copy of default `.clang-build-profiler` unless one is already present.

> [!Tip]
> Setting `tree.categorize.gray` to `0` will disable all pruning, showing the full tree of includes and template instantiations. In most cases this amount of information is excessive and will produce multiple MBs of raw data, values above `30` are a reasonable default to ignore.
