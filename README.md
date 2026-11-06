# savekit

A small versioned binary serialization library in C++20.

> Work in progress. File format docs, design notes and benchmarks will go here.

## Building

Requires CMake 3.20+ and a C++20 compiler (GCC, Clang or MSVC). GoogleTest is
downloaded automatically on first configure.

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

### Options

| Option                       | Default | Meaning                                         |
|------------------------------|---------|-------------------------------------------------|
| `SAVEKIT_BUILD_TESTS`        | `ON`    | Build the GoogleTest suite                      |
| `SAVEKIT_BUILD_TOOLS`        | `ON`    | Build `savekit-dump`                            |
| `SAVEKIT_ENABLE_SANITIZERS`  | `OFF`   | AddressSanitizer + UBSan (GCC/Clang on Linux/macOS) |
| `SAVEKIT_WARNINGS_AS_ERRORS` | `OFF`   | `-Werror` / `/WX` (on in CI)                    |

## Layout

```
include/savekit/   public headers
src/               library implementation
tools/dump/        savekit-dump CLI
tests/             GoogleTest unit tests
.github/workflows/ CI: GCC + Clang with sanitizers, MSVC
```
