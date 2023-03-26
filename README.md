# BM

Header only, nanosecond resolution, C++ microbenchmark library.

## Status
In development. Very much not ready. Check NOTES.txt for TODOs I'm working on.

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

## Requires

- Intel or AMD x86 chipset
- Linux
- g++ or clang with C++11 and x86 intrinsics

## Development and Testing

NOTES.txt contains objectives, requirements, non-requirements, TODO and design notes.

Requires:
- CMake 3.5
- Python3

```bash
$ cmake -B build -G Ninja -S .
$ cmake --build build --target check_all
```

