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

static const std::string kOutputFileFormatFlag = "output_format";
static const std::string kOutputFilePathFlag = "output_file";
static const std::string kTestRootDirFlag = "test_root_dir";

// Options captures global settings for running the Benchmark (passed in via
// command line flags).
// Testing only.
// --test_root_dir: By default, benchmarking library assumes system root is '/'.
// If set, system root will be test_root_dir. Used in testing procfs and sysfs
// checks.
class Options {
 public:
  std::string benchmark_binary_name_;
  enum class OutputFormat {
    kUnknown,
    kText,
  };
  static OutputFormat StrToOutputFormat(const char *output_format_string) {
    if (!output_format_string) {
      return OutputFormat::kUnknown;
    }
    if (strncmp("kT", output_format_string, 2)) {
      return OutputFormat::kText;
    }
    return OutputFormat::kUnknown;
  }
  static std::string OutputFormatToStr(const OutputFormat &format) {
    switch (format) {
      case Options::OutputFormat::kUnknown:
        return "Unknown";
      case Options::OutputFormat::kText:
        return "Text";
      default:
        break;
    }
    return "";
  }
  OutputFormat output_format_ = Options::OutputFormat::kUnknown;
  std::string output_file_path_ = "";

  // Testing only flags
  bool any_test_flag_set_ = false;
  std::string test_root_dir_ = "";

  Options() = default;

  // InsertCliFlag returns non zero value on error:
  // 1: Line doesn't match {--option_name=option_value}
  // 2: Option value doesn't match type for option name
  // 3: Could not find option flag for the type
  // On correctly processing the flag and storing it in option, returns 0.
  int InsertCliFlag(char *argv) {
    if (!argv && !(argv[0] == '-' && argv[1] == '-')) {
      return 1;
    }
    if (*argv == '\0') {
      return 0;
    }
    char *option_name = argv + 2;
    if (!option_name && !(isalpha(*option_name) || isdigit(*option_name))) {
      return 1;
    }
    char *option_value = strchr(argv, '=');
    if (option_value == NULL) {
      return 1;
    }

    option_value++;
    if (!option_value || *option_value == '\0') {
      return 1;
    }
    const std::string *closest_candidate = nullptr;
    switch (*option_name) {
      case 't': {
        // Because testing flags are optional, we won't set closest_candidate
        if (!strncmp(option_name, kTestRootDirFlag.c_str(),
                     kTestRootDirFlag.size())) {
          // NOTE: we don't check whether the file exists here. We'll do error
          // checking on that when we try to open the file/directory.
          // TODO: verify that assignment operator does a deep copy.
          test_root_dir_ = option_value;
          any_test_flag_set_ = true;
          std::cout << "FLAG SET: --test_root_dir=" << test_root_dir_ << '\n';
        }
        break;
      }
      case 'o': {
        // TODO(OPTIMIZATION): instead of strncmp, bump the char * by n
        // characters to reach the first differing character. Address jump is
        // slightly faster than 2 strncmp calls (O(1) vs O(n)).
        closest_candidate = &kOutputFileFormatFlag;
        if (!strncmp(option_name, kOutputFileFormatFlag.c_str(),
                     kOutputFileFormatFlag.size())) {
          output_format_ = StrToOutputFormat(option_value);
          break;
        }
        if (!strncmp(option_name, kOutputFilePathFlag.c_str(),
                     kOutputFilePathFlag.size())) {
          // TODO: verify that assignment operator does a deep copy.
          output_file_path_ = option_value;
          break;
        }
      }
      default: {
        if (closest_candidate) {
          std::cout << "No flags matched for " << option_name << ". Maybe "
                    << *closest_candidate << "?\n";
        }
        return 3;
      }
    }
    return 0;
  }
};

// TODO(REFACTOR): Add a Validate() method to Config to do all checks at once.
//  Maybe move closest candidate check to here instead of in CLI parsing?
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

// TODO: add check for AMD x86 chip equivalent.
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
  std::string name = "";
  uint64_t cpu_time = 0;
  uint64_t wall_time = 0;
  uint64_t iterations = 0;
};

static std::vector<BM::Benchmark *> RegisteredBenchmarks;

BM::Benchmark *Register(char *bm_name, BM::Function *bm_f) {
  BM::Benchmark *bm = new BM::Benchmark;
  if (!bm_f) {
    std::cout << "Failed to register benchmark: No benchmark passed\n";
    return bm;
  }
  bm->f = bm_f;
  bm->name = std::string(bm_name);
  BM::RegisteredBenchmarks.push_back(bm);
  return bm;
}

void Initialize(int argc, char **argv) {
  int status = 0;
  Config.benchmark_binary_name_ = argv[0];
  for (int i = 1; i < argc; ++i) {
    status = BM::Config.InsertCliFlag(argv[i]);
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
      default: {
        continue;
      }
    }
  }
  for (size_t i = 0; i < kSysfsChecks.size(); ++i) {
    std::string path;
    if (!Config.test_root_dir_.empty()) {
      path = Config.test_root_dir_;
      // We pop the last directory seperator because sysfs check paths always
      // start with root (/).
      if (path.back() == '/') {
        path.pop_back();
      }
    }
    path.append(kSysfsChecks[i].file_path);
    // TODO: Refactor: any_test_flag_set_ is used during testing to see if flags
    // are properly set. Perhaps we want to replace with some sort of global
    // --log_error or --log_verbosity=TESTING.
    if (Config.any_test_flag_set_) {
      std::cout << "Checking sysfs@" << path << '\n';
    }
    std::ifstream sys_file;
    sys_file.open(path, std::ios::in);
    if (!sys_file.is_open()) {
      if (Config.any_test_flag_set_) {
        std::cout << "Failed to open " << path << '\n';
      }
      continue;
    }
    std::string sys_file_contents;
    sys_file >> sys_file_contents;
    if (sys_file_contents != kSysfsChecks[i].want) {
      std::cout << kSysfsChecks[i].remedy << '\n';
    }
    sys_file.close();
  }
}

void Run() {}

void ShutDown() {
  std::streambuf *output_buffer;
  std::ofstream output_file;
  if (!Config.output_file_path_.empty()) {
    output_file.open(Config.output_file_path_);
    output_buffer = output_file.rdbuf();
  } else {
    output_buffer = std::cout.rdbuf();
  }
  std::ostream out(output_buffer);
  out << "Running benchmarks in " << Config.benchmark_binary_name_ << ". ";
  if (Config.output_format_ != Options::OutputFormat::kUnknown) {
    out << "Format: " << Options::OutputFormatToStr(Config.output_format_)
        << ". ";
  }
  std::string delim = " ";
  switch (Config.output_format_) {
    case (Options::OutputFormat::kText): {
      delim = " : ";
    }
    default: {
      break;
    }
  }
  // TODO: change to table
  // TODO: CSV, JSON format
  out << '\n';
  for (const auto &b : RegisteredBenchmarks) {
    out << "Name" << delim << b->name << '\n'
        << "CPU Time" << delim << b->cpu_time << '\n'
        << "Wall Time" << delim << b->wall_time << '\n'
        << "Iterations" << delim << b->iterations << '\n';
  }
  if (!Config.output_file_path_.empty()) {
    std::cout << "Generated " << Config.output_file_path_ << ". ";
  }
}

}  // namespace BM

// TODO: Args and Threads for BM. Will likely need to add void methods to
// Benchmark class (or whatever BM::Register returns).
// TODO: BM_Register(SomeBenchmark)->Args(a)
// TODO: BM_Register(SomeBenchmark)->ArgRange(a, b)
// TODO: BM_Register(SomeBenchmark)->ArgRange(a, b, jump)
// TODO: BM_Register(SomeBenchmark)->Threads(a)
#define BM_REGISTER_(bm) BM::Register(#bm, bm);

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
