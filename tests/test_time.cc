#include "bm.hpp"

static void BM_VecPush(BM::Controller &c) {
  std::vector<int> v;
  auto begin = c.begin();
  auto end = c.end();
  for (auto it = begin; it != end; ++it) {
    for (int i = 0; i < 1000; ++i) {
      v.push_back(i);
    };
  }
}

BM_Register(BM_VecPush);

BM_Main();
