#include <iostream>

#include "bm.h"

static void BM_Sample(const BM::State &state) {}

BM_Register(BM_Sample);

int main() {
  if (BM::Internal::registered_benchmarks_.size() != 1) {
    std::cout << "Failed to register bm\n";
    return -1;
  }
  std::cout << "Passed\n";
  return 0;
}
