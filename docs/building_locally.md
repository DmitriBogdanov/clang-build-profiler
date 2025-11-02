# Building the project locally

[<- to README.md](..)

## Building the project

### Prerequisites

This project uses [CMake](https://cmake.org) with [presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) as a main way of managing platform-dependent configuration.

Since using the tool in question inherently requires an installation of `clang++`, the same compiler is used for all the build presets.

On most systems LLVM provides both system packages and an [automatic install script](https://apt.llvm.org/):

```sh
wget https://apt.llvm.org/llvm.sh &&
chmod +x llvm.sh                  &&
sudo ./llvm.sh all
```

For Windows, both `cmake` and `clang++` toolchain can be set up through [Visual Studio Installer](https://visualstudio.microsoft.com/downloads/).

### Building

Clone the repo:

```bash
git clone https://github.com/DmitriBogdanov/clang-build-profiler.git &&
cd "clang-build-profiler/"
```

Configure **CMake**:

```bash
cmake --preset=clang
```

Build the project:

```bash
cmake --build --preset=clang
```

The binary can be found at `build/clang-build-profiler`.

## Building the docs

### Prerequisites

This repo uses [MkDocs](https://www.mkdocs.org/) with [Material](https://squidfunk.github.io/mkdocs-material/) theme to build the website version of documentation.

`python3` can be downloaded from the [official website](https://www.python.org/downloads/) or using a package manager:

```sh
sudo apt update &&
sudo apt install python3
```

After this, to install necessary dependencies run:

```sh
pip install mkdocs-material          &&
pip install mkdocs-material[imaging] &&
pip install markdown-callouts        &&
pip install mkdocs-awesome-nav
```

### Building

To build the documentation locally and view it immediately run:

```sh
mkdocs serve --open
```

To build a version for distribution run:

```sh
mkdocs build
```
