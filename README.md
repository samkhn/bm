# BM

Header only, nanosecond resolution, C++ microbenchmark library.

## Status
In development. Very much not ready. Check NOTES.txt for TODOs.

## Sample

```cpp
#include "bm.h"

#include <string>

static void BM_KMP(const BM::State& state) {
	std::string space;
	std::string query;
	for (auto _ : state) {
		KnuthMorrisPrattSearch(space, query);
	}
}
BM_Register(BM_KMP);
BM_Main();
```

Then, compile and run.

```bash
$ g++ -I/path/to/headers -I/path/to/bm.h benchmark_library.cc -O3 -o bm
$ ./bm --output_format=csv --output_file=benchmark.csv
```

## Requires

- Intel or AMD x86 chipset
- Linux (if you wish to add Windows, please add the appropriate header and include directives in bm.h and send a PR).
- g++ or clang with at least C++11 and x86 intrinsics

## Development and Testing

NOTES.txt contains objectives, requirements, non-requirements, TODO and design notes.

Requires:
- CMake at least 3.5
- Python at least 3.5

```bash
$ cmake -B build -G Ninja -S .
$ cmake --build build --target check_all
```

### Flag testing

If you are testing a flag, please add a Python script that passes the flag and gets expected output. Check check\_flags\_integration.py as an example.
