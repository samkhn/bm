#include <iostream>

#include "bm.hpp"

static void BM_Sample(BM::Controller &state) {}

BM_Register(BM_Sample);

int main() {
  if (BM::Benchmarks.size() != 1) {
    std::cout << "Test Register. Failed to register. Want success.\n";
    return -1;
  }
  std::cout << "Test Register. Passed\n";
  return 0;
}
