# FlashBarCSandbox

A cross-platform desktop application built with C++ and Qt 6 Widgets.

## Requirements

- CMake 3.21 or newer
- A C++17 compiler
- Qt 6.2 or newer with the Widgets module

## Build

```sh
cmake -S . -B build
cmake --build build
```

Run `build/FlashBarCSandbox` on Linux. On Windows, run the generated
`FlashBarCSandbox.exe` from the build directory.

## Source layout

- `src/ui`: Qt windows and presentation code
- `src/domain`: Domain models and rules
- `src/repositories`: Repository interfaces
- `src/services`: Application use cases and service ports
- `src/infrastructure`: Repository and service implementations, plus external integrations# FlashBarC
