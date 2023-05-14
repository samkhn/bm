# BM

Header only, nanosecond resolution, C++ microbenchmark library.

## Status

Prints mean, variance and std deviation of critical sections with rdtsc. However, doesn't allow for passing arguments to critical section via Controller, doesn't allow for threading via Controller, doesn't yet support lfence and DoNotOptimize(). Will add these as I or you need them.

## Sample

```cpp
#include "bm.hpp"

#include <string>

static void BM_KMP(BM::Controller& c) {
  std::string space;
  std::string query;
  for (auto _ : c) {
    KnuthMorrisPrattSearch(space, query);
  }
}
BM_Register(BM_KMP);
BM_Main();
```

Then, compile and run.

```bash
g++ -I/path/to/headers -I/path/to/bm.h benchmark_library.cc -O3 -o bm
./bm --output_format=csv --output_file=benchmark.csv
```

## Requires

- Intel or AMD x86 chipset with rdtsc instruction available
- Linux with sysfs pseudofilesystem
- g++ or clang with at least C++11 and x86 intrinsics

## Development and Testing

NOTES.txt contains objectives, requirements, non-requirements, TODO and design notes.

Requires:

- CMake at least 3.5
- Python at least 3.5

```bash
mkdir -p build
cmake -Bbuild -GNinja -S .
cmake --build build --target check-all
```

### Flag testing

If you are testing a flag, please add a Python script that passes the flag and gets expected output. Check check\_flags\_integration.py as an example.

### Format and linting

Requires python3 black, clang-tidy and clang-format for C++. Run

```bash
# pip install -r requirements.txt  # if you haven't already
cmake --build build --target lint
```

Note: we ignore errors about exception throwing for static/global variables. Exception safety is a non-requirement. Check NOTES.txt for more design notes.
