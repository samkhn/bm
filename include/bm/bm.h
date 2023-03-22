// BM, small benchmark library
//
// Example:
// static void BM_KMP(const BM::State& state) {
// 	std::string space;
//  std::string query;
//  for (auto _ : state) {
//		KnuthMorrisPrattSearch(space, query);
//  }
// }
// BM_Register(BM_KMP);
// BM_Main();

#ifndef _BM_H_
#define _BM_H_

#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

namespace BM {

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
