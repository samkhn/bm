#include <iostream>

#include "bm.hpp"

static void BM_Sample(const BM::State &state) {}

BM_Register(BM_Sample);

int main() {
  if (BM::RegisteredBenchmarks.size() != 1) {
    std::cout << "Test Register. Failed to register. Want success.\n";
    return -1;
  }
  std::cout << "Test Register. Passed\n";
  return 0;
}
