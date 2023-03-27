// BM, small benchmark library
//
// Example:
// File: bm.cc
// static void BM_KMP(const BM::State& state) {
//   std::string space;
//   std::string query;
//   for (auto _ : state) {
//     KnuthMorrisPrattSearch(space, query);
//   }
// }
// BM_Register(BM_KMP);
// BM_Main();
//
// CLI:
// $ g++ -I/path/to/headers bm.cc -O3 -o bm
// $ ./bm --output_format=CSV --output_file=benchmark.csv
//

#ifndef _BM_H_
#define _BM_H_

#include <x86intrin.h>

#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace BM {

static const std::string kTestRootDirFlag = "test_root_dir";
// Options captures global settings for running the Benchmark (passed in via
// command line flags). These include end-user and testing flags:
// TODO: --warmup={True|False}. Default is true. BM will spend some time warming
// up the codepath.
// TODO: --enable_random_interleaving={True|False}. Default is true. To prevent
// a CPU cache line from getting used to a particular codepath, BM will move the
// BM onto different chips.
// TODO: --min_repetitions={positive integer}. Default is 10. BM will repeat
// benchmark at least min_repetitions times to get statistically significant
// results.
// TODO: --min_time={positive float}. Default is 1. BM will spend at least
// min_time seconds on the benchmark.
// Testing only.
// --test_root_dir: By default, benchmarking library assumes system root is '/'.
// If set, system root will be test_root_dir. Used in testing procfs and sysfs
// checks.
class Options {
 public:
  Options()
      : testing_flag_set_(false),
        test_root_dir_set_(false),
        test_root_dir_("") {}

  // Returns non zero value on error:
  // 1: Line doesn't match {--option_name=option_value}
  // 2: Option value doesn't match type for option name
  // 3: Could not find option flag for the type
  int InsertCliFlag(char *argv) {
    if (!argv && !(argv[0] == '-' && argv[1] == '-')) return 1;
    char *option_name = argv + 2;
    char *option_value = strchr(argv, '=');
    if (option_value == NULL) return 1;
    option_value++;
    if (!option_name && !(isalpha(*option_name) || isdigit(*option_name)))
      return 1;
    if (!option_value || *option_value == '\0') return 1;
    const std::string *closest_candidate = nullptr;
    switch (*option_name) {
      case 't': {
        // Because testing flags are optional, we won't set closest_candidate
        if (strncmp(option_name, kTestRootDirFlag.c_str(),
                    kTestRootDirFlag.size()) == 0) {
          // NOTE: we don't check whether the file exists here. We'll do error
          // checking on that when we try to open the file/directory.
          test_root_dir_ = option_value;
          test_root_dir_set_ = true;
          testing_flag_set_ = true;
          std::cout << "TEST FLAG SET: --test_root_dir=" << test_root_dir_
                    << "\n";
        }
        break;
      }
      default:
        if (closest_candidate) {
          std::cerr << "No flags matched for " << option_name << ". Maybe "
                    << closest_candidate << "?\n";
        }
        return 3;
    }
    return 0;
  }
  bool testing_flag_set_;
  bool test_root_dir_set_;
  std::string test_root_dir_;
};

struct SystemCheck {
  std::string file_path;
  std::string want;
  std::string remedy;
};

static const std::vector<SystemCheck> kSysfsChecks{
    {"/sys/devices/system/cpu/intel_pstate/no_turbo", "1",
     "Warning: Chip power frequency scaling is on. Recommend turning it off "
     "for more accurate results."},
};

class State {};

typedef void(Function)(const BM::State &);

class Benchmark {
 public:
  BM::Function *f;
};

static BM::Options options_;
static std::vector<BM::Benchmark *> registered_benchmarks_;

BM::Benchmark *Register(BM::Function *bm_f) {
  BM::Benchmark *bm = new BM::Benchmark;
  if (!bm_f) {
    std::cerr << "No benchmark passed\n";
    return bm;
  }
  bm->f = bm_f;
  BM::registered_benchmarks_.push_back(bm);
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

void Initialize(int argc, char **argv) {
  int status = 0;
  for (int i = 1; i < argc; ++i) {
    status = BM::options_.InsertCliFlag(argv[i]);
    switch (status) {
      case 1: {
        std::cout << "Error with flag. Got " << argv[i]
                  << ". Want form --{option_name}={option_value}\n";
        break;
      }
      case 2: {
        std::cout
            << "Error with flag " << argv[i]
            << ". Parsed flag value does not match flag's declared type\n";
        break;
      }
      case 3: {
        std::cout << "Error with flag " << argv[i] << "Unknown option_name\n";
        break;
      }
      default:
        continue;
    }
  }
  for (int i = 0; i < kSysfsChecks.size(); ++i) {
    // if test_root_dir is set, prepend to path in check and set
    // print_all_warnings_to_high. let user knwo we're goign to print everything
    // if file doesnt exist, ignore. but if testing, print that you couldn't
    // find the file read that file if it isn't what we want, print warning
    std::string p;
    if (options_.test_root_dir_set_) {
      p = options_.test_root_dir_;
      if (p.back() == '/') p.pop_back();
    }
    p += kSysfsChecks[i].file_path;
    if (options_.testing_flag_set_) std::cout << "Checking sysfs@" << p << "\n";
    std::ifstream sys_file;
    sys_file.open(p, std::ios::in);
    if (sys_file.is_open()) {
      std::string v;
      sys_file >> v;
      if (v != kSysfsChecks[i].want) std::cout << kSysfsChecks[i].remedy;
      sys_file.close();
    } else {
      if (options_.testing_flag_set_)
        std::cout << "Failed to open " << p << "\n";
    }
  }
}

void Run() {}

void ShutDown() {}

}  // namespace BM

#define BM_REGISTER_(bm) BM::Register(bm);

#define BM_NAME(bm) benchmark_id_##__LINE__##bm

#define BM_Register(bm) static BM::Benchmark *BM_NAME(bm) = BM_REGISTER_(bm)

#define BM_Main()                   \
  int main(int argc, char **argv) { \
    BM::Initialize(argc, argv);     \
    BM::Run();                      \
    BM::ShutDown();                 \
    return 0;                       \
  }

#endif  // _BM_H_
