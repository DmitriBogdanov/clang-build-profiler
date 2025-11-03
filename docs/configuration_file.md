# Configuration file

## Basics

Following the convention of other LLVM tools, `clang-build-profiler` uses a YAML config `.clang-build-profiler` placed into the project root.

This means, for a project using other things from the LLVM toolchain, the root directory might look something like this:

```
docs/
include/
source/
.clang-build-profiler
.clang-format
.clang-tidy
.clangd
.gitignore
```

The config contains project-specific settings such as coloring, name simplification rules and etc.

## Schema

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