# Supported CLI arguments

| Argument               | Type                | Default value           | Description                                                  |
| ---------------------- | ------------------- | ----------------------- | ------------------------------------------------------------ |
| `-h`, `--help`         | Unique flag         | -                       | Displays help message                                        |
| `-v`, `--version`      | Unique flag         | -                       | Displays application version                                 |
| `-w`, `--write-config` | Unique flag         | -                       | Creates config file corresponding to the default configuration |
| `-c`, `--config`       | Optional argument   | `.clang-build-profiler` | Specifies custom config path                                 |
| `-o`, `--output`       | Optional argument   | `terminal`              | Selects profiling output format                              |
| `-b`, `--build`        | Exclusive group [1] | `build/`                | Selects CMake build directory                                |
| `-t`, `--target`       | Exclusive group [1] | -                       | Selects build artifacts directory                            |
| `-f`, `--file`         | Exclusive group [1] | -                       | Selects specific translation unit                            |

## Examples

### View help

Verbose form:

```sh
clang-build-profiler --help
```

Short form:

```sh
clang-build-profiler -h
```

### View version

Verbose form:

```sh
clang-build-profiler --version
```

Short form:

```sh
clang-build-profiler -v
```

### Write default config

Verbose form:

```sh
clang-build-profiler --write-config
```

Short form:

```sh
clang-build-profiler -w
```

### Specify custom config path

Verbose form:

```sh
clang-build-profiler --config="configs/clang-build-profiler.yml"
```

Short form:

```sh
clang-build-profiler -c "configs/clang-build-profiler.yml"
```

### Specify output format

Verbose form:

```sh
clang-build-profiler --output=json
```

Short form:

```sh
clang-build-profiler -o json
```

### Specify trace file

Verbose form:

```sh
clang-build-profiler --file="build/main.cpp.json"
```

Short form:

```sh
clang-build-profiler -f "build/main.cpp.json"
```