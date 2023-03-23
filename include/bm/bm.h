// BM, small benchmark library
//
// Example:
//	static void BM_KMP(const BM::State& state) {
//		std::string space;
//		std::string query;
//		for (auto _ : state) {
//			KnuthMorrisPrattSearch(space, query);
//		}
//	}
//	BM_Register(BM_KMP);
//	BM_Main();

#ifndef _BM_H_
#define _BM_H_

#include <stdint.h>

// WARNING: rdtsc counts reference cycles, not actual CPU core cycles.
// Run You can check this with sudo dmesg | grep tsc and it'll print the TSC
// frequency and the actual CPU frequency

#include <x86intrin.h>

namespace BM {

namespace Internal {

uint64_t Now() {
  unsigned int lo, hi;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)hi << 32) | lo;
}

}  // namespace Internal

void Initialize(int *argc, char **argv) {}

void Run() {}

void ShutDown() {}

}  // namespace BM

#define BM_Main()                   \
  int main(int argc, char **argv) { \
    BM::Initialize(&argc, argv);    \
    BM::Run();                      \
    BM::ShutDown();                 \
    return 0;                       \
  }

#endif  // _BM_H_
