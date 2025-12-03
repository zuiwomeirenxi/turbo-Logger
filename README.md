# TurboLog

A minimal high-performance logging component: fixed-size `LogBuffer` for staging log messages efficiently.

## LogBuffer

- Template parameter `SIZE`: compile-time capacity in bytes.
- Internal storage: `std::array<char, SIZE>`.
- Methods:
  - `append(const char* buf, size_t len)`: copies `len` bytes if they fit; otherwise no-op by default. Define `TURBOLOG_SAFE_APPEND` to copy up to available space.
  - `avail()`: remaining space.
  - `data()`: pointer to contiguous data.
  - `length()`: current length.
  - `reset()` / `bzero()`: reset the write pointer without zeroing memory.
- Copy disabled: copy ctor and assignment deleted.

## Build & Run (CMake)

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
./turbolog_example
```

## Quick Compile Without CMake

```bash
g++ -std=c++17 -Iinclude examples/main.cpp -o turbolog_example && ./turbolog_example
```
