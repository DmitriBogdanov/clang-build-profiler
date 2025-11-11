# Configuration file

[<- to README.md](..)

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
version: "0.1.0"

# The main profiling output
tree:

  # Node color & pruning thresholds, for example:
  #   'gray  :  90' => color durations >  90 ms gray
  #   'white : 150' => color durations > 150 ms white
  #   'yellow: 300' => color durations > 300 ms yellow
  #   'red   : 800' => color durations > 800 ms red
  #   Nodes without a color are hidden, to disable all pruning set 'gray' to '0'
  categorize:
    gray  :  90
    white : 150
    yellow: 300
    red   : 800
  
  # Automatically detect & simplify standard header includes, for example:
  #   'false' => '/usr/lib/llvm-21/include/c++/v1/filesystem'
  #   'true'  => '<filesystem>'
  detect_standard_headers: true

  # Automatically detect & simplify project header includes, for example:
  #   'false' => '/home/.../clang-build-profiler/include/utility/replace.hpp'
  #   'true'  => 'include/utility/replace.hpp'
  detect_project_headers:  true
  
  # Create alias for some filepaths, for example:
  #   '- from: "include/external/boost/"' => includes paths beginning with "include/external/boost/"
  #   '  to  : "boost/"                 '    will be shortened to "boost/"
  replace_filepath:
    - from: "include/"
      to  : ""
    - from: "source/"
      to  : ""
```

To serialize this config for modification we can run:

```sh
clang-build-profiler --write-config
```

Which will write a copy of default `.clang-build-profiler` unless one is already present.

> [!Tip]
> Setting `tree.categorize.gray` to `0` will disable all pruning, showing the full tree of includes and template instantiations. In most cases this amount of information is excessive and will produce multiple MBs of raw data, values above `30` are a reasonable default to ignore.