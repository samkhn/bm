#include <stdint.h>

#include "bm.h"

static constexpr int kN = 64 * 512;
static uint8_t data[kN];

static void BM_CacheLineFlush(const BM::State &state) {
  volatile uint8_t *p_addr;
  data[0] = 1;
  _mm_clflush(data);
  for (auto _ : state) {
    addr = &data[i];
  }
}
BM_Register(BM_CacheLineFlush);

static void BM_CacheLineFlush(const BM::State &state) {
  volatile uint8_t *p_addr;
  data[0] = 1;
  for (auto _ : state) {
    addr = &data[i];
  }
}
BM_Register(BM_NoCacheLineFlush);

BM_Main();
