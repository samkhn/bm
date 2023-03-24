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
#include <x86intrin.h>

#include <functional>
#include <iostream>
#include <vector>

namespace BM {

class State {};

typedef void(Function)(const BM::State &);

class Benchmark {
 public:
  ::BM::Function *f;
};

namespace Internal {

static std::vector<::BM::Benchmark *> registered_benchmarks_;

::BM::Benchmark *Register(::BM::Function *bm_f) {
  ::BM::Benchmark *bm = new ::BM::Benchmark;
  if (!bm_f) {
    std::cerr << "No benchmark passed\n";
    return bm;
  }
  bm->f = bm_f;
  ::BM::Internal::registered_benchmarks_.push_back(bm);
  return bm;
}

// WARNING: rdtsc counts reference cycles, not actual CPU core cycles.
// Run You can check this with sudo dmesg | grep tsc and it'll print the TSC
// frequency and the actual CPU frequency
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

#define BM_REGISTER_(bm) BM::Internal::Register(bm);

#define BM_NAME(bm) benchmark_id_##__LINE__##bm

#define BM_Register(bm) static BM::Benchmark *BM_NAME(bm) = BM_REGISTER_(bm)

#define BM_Main()                   \
  int main(int argc, char **argv) { \
    BM::Initialize(&argc, argv);    \
    BM::Run();                      \
    BM::ShutDown();                 \
    return 0;                       \
  }

#endif  // _BM_H_
