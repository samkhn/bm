// BM, small benchmark library
// TODO: document public parts of the API.

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
// command line flags).
// Testing only.
// --test_root_dir: By default, benchmarking library assumes system root is '/'.
// If set, system root will be test_root_dir. Used in testing procfs and sysfs
// checks.
class Options {
 public:
  // Testing only flags
  bool testing_flag_set_;
  bool test_root_dir_set_;
  std::string test_root_dir_;
  Options()
      : testing_flag_set_(false),
        test_root_dir_set_(false),
        test_root_dir_("") {}

  // InsertCliFlag returns non zero value on error:
  // 1: Line doesn't match {--option_name=option_value}
  // 2: Option value doesn't match type for option name
  // 3: Could not find option flag for the type
  // On correctly processing the flag and storing it in option, returns 0.
  int InsertCliFlag(char *argv) {
    if (!argv && !(argv[0] == '-' && argv[1] == '-')) {
      return 1;
    }
    char *option_name = argv + 2;
    char *option_value = strchr(argv, '=');
    if (option_value == NULL) {
      return 1;
    }
    option_value++;
    if (!option_name && !(isalpha(*option_name) || isdigit(*option_name))) {
      return 1;
    }
    if (!option_value || *option_value == '\0') {
      return 1;
    }
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
      default: {
        if (closest_candidate) {
          std::cerr << "No flags matched for " << option_name << ". Maybe "
                    << closest_candidate << "?\n";
        }
        return 3;
      }
    }
    return 0;
  }
};

static BM::Options Config;

// TODO: move to internal namespace
// SystemCheck is a store of sysfs files BM checks at runtime before benchmarks.
// Depending on what values a machine's sysfs has, it'll make recommendations
// to help improve benchmark predictions.
class SystemCheck {
 public:
  std::string file_path;
  std::string want;
  std::string remedy;
};

static const std::vector<SystemCheck> kSysfsChecks{
  {"/sys/devices/system/cpu/intel_pstate/no_turbo", "1",
   "Warning: Chip power frequency scaling is on. Recommend turning it off "
   "for more accurate results."},
};

// State holds threading and argument info for benchmarks.
class State {};

typedef void(Function)(const BM::State &);

class Benchmark {
 public:
  BM::Function *f;
};

static std::vector<BM::Benchmark *> RegisteredBenchmarks;

BM::Benchmark *Register(BM::Function *bm_f) {
  BM::Benchmark *bm = new BM::Benchmark;
  if (!bm_f) {
    std::cout << "Failed to register benchmark: No benchmark passed\n";
    return bm;
  }
  bm->f = bm_f;
  BM::RegisteredBenchmarks.push_back(bm);
  return bm;
}

void Initialize(int argc, char **argv) {
  int status = 0;
  for (int i = 1; i < argc; ++i) {
    status = BM::Config.InsertCliFlag(argv[i]);
    switch (status) {
      case 1: {
        std::cout << "Error with flag. Got " << argv[i]
                  << ". Want form --{option_name}={option_value}\n";
        break;
      }
      case 2: {
        std::cout << "Error with flag " << argv[i]
                  << ". Parsed flag value does not match flag's declared type\n";
        break;
      }
      case 3: {
        std::cout << "Error with flag " << argv[i] << "Unknown option_name\n";
        break;
      }
      default: {
        continue;
      }
    }
  }
  for (int i = 0; i < kSysfsChecks.size(); ++i) {
    std::string p;
    if (Config.test_root_dir_set_) {
      p = Config.test_root_dir_;
      if (p.back() == '/') {
        p.pop_back();
      }
    }
    p += kSysfsChecks[i].file_path;
    // TODO: Refactor: testing_flag_set_ is used during testing to see if flags
    // are properly set. Perhaps we want to replace with some sort of global
    // --log_error or --log_verbosity=TESTING.
    if (Config.testing_flag_set_) {
      std::cout << "Checking sysfs@" << p << "\n";
    }
    std::ifstream sys_file;
    sys_file.open(p, std::ios::in);
    if (!sys_file.is_open()) {
      if (Config.testing_flag_set_) {
        std::cout << "Failed to open " << p << "\n";
      }
      continue;
    }
    std::string sys_file_contents;
    sys_file >> sys_file_contents;
    if (sys_file_contents != kSysfsChecks[i].want) {
      std::cout << kSysfsChecks[i].remedy;
    }
    sys_file.close();
  }
}

void Run() {}

void ShutDown() {}

}  // namespace BM

// TODO: Args and Threads for BM. Will likely need to add void methods to
// Benchmark class (or whatever BM::Register returns).
// TODO: BM_Register(SomeBenchmark)->Args(a)
// TODO: BM_Register(SomeBenchmark)->ArgRange(a, b)
// TODO: BM_Register(SomeBenchmark)->ArgRange(a, b, jump)
// TODO: BM_Register(SomeBenchmark)->Threads(a)
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
