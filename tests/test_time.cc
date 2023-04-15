#include "bm.hpp"

static void BM_VecPush(BM::Controller &c) {
  std::vector<int> v;
  for (auto _ : c) {
    for (int i = 0; i < 10; ++i) {
      v.push_back(i);
    };    
  }
}

BM_Register(BM_VecPush);

BM_Main();
