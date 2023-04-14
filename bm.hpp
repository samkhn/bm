// BM, a small benchmarking library
//
// Example:
// static void BM_memcpy(BM::Controller &c) {
//   char *src = new char[c.Arg(0)];
//   char *dst = new char[c.Arg(0)];
//   memset(src, 'x', c.Arg(0));
//   for (auto _ : c) {
//     memcpy(dst, src, c.Arg(0));
//   }
//   delete[] src;
//   delete[] dst;
// }
// BM_Register(BM_memcpy)->Arg(8)->Arg(64)->Arg(512);
// BM_Main();
//
// Start by declaring your benchmark as a static method that takes
// BM::Controller and returns void.
//
// Within your static function, you can access the Controller parameter to
// access arguments.
// E.g. c.Arg(0) will be the first argument passed.
//
// The section you want timed should be inside a for loop that iterates through
// the controller parameter: for (auto _ : c) { function_to_benchmark(); }
//
// Once you have declared the function, you can register it and set parameters:
// BM_Register(Function)
//   ->Arg(n) passes n as a parameter.
//   ->ArgRange(a, b) goes from a to b. Assumes a < b.
//   ->ArgRange(a, b, jump) goes from a to b but jumps by jump each time.
//     Assumes a < b.
//   ->Threads(n)
// n, a, b, jump are all integers. This is purposefully restricted for
// simplicity and so that you can more easily graph your results and understand
// how the growth of n or a->b impacts the performance of the critical section.
//
// Finally, at the bottom of your file, please call BM_Main(). This should be
// called only once.
//
// Compile your binary and include bm.hpp in the include path.
//
// You can pass flags to control how tests are executed and how to generate
// output. All flags are optional:
//
// By default it'll print to stdout, for instance. Some flags to control output:
// --output_format=csv (default is text)
// --output_file=results.txt (default is empty)
//
// Some flags you can use to tune the benchmark:
// --benchmark_enable_random_interleaving=True (default is False)
// --benchmark_warmup=True (default is False)
// --benchmark_repetitions={unsigned int} (default is 1)
// --benchmark_min_time={unsigned float} (default is 0.1 seconds)
//
// If a malformed flag is passed, benchmarks will not run.
//
// BM includes some system level checks for source of hardware jitter:
// 1) CPU Power frequency scaling
// 1a) Intel turboboosting. This can impact your results by changing the CPU
//     frequency. You can use sysfs to disable this.
// 1b) BMing functions with a lot of AVX instructions. Intel chips might slow
//     down during intense SIMD instructions to prevent overheating.
//     Perhaps offload your SIMD instructions to a GPU/TPU?
// 1c) Linux CPUfreq governor. Calculates what the CPU frequency should be.
//     Try to emulate real-world settings. You can use sysfs to disable this.
// 2) Virtual address randomization. You might want to set this on/off depending
//    on what you need e.g. if you want your critical section to be performant
//    in the face of getting moved to a different chipset where the VM map might
//    cause a lot of page faults at first. You can use sysfs to disable this.
//    Also use --benchmark_enable_random_interleaving=True
// 3) Caching. The more your CPU runs your benchmark, the faster it'll
//    (probably) get. If you want to only time when your cache lines are hot use
//    --benchmark_warmup=True
//    --benchmark_repetitions={uint}
//    --benchmark_min_time={ufloat}
// 4) Kernel interrupts. Can't really stop this. Processor might interrupt your
//    BM. To reduce the change of this, pin the pid to a CPU. You can use sysfs
//    to pin it.
// It can be difficult to get accurate results. Hopefully this reduces the
// jitter. Let me know if you find more ways to reduce it.
//
// If you compile with optimization flags, the compiler might optimize sections
// out. Use BM::DoNotOptimize(var). Example:
// static void BM_VecPush(BM::Controller &c) {
//   vector<int> v;
//   v.reserve(c.Args(0));
//   BM::DoNotOptimize(v.data());
//   for (auto _ : c) {
//     v.push_back(10);
//   }
// }
//
// You can also call without any arguments to prevent optimizations in the
// entire scope:
// static void BM_VecPush(BM::Controller &c) {
//   vector<int> v;
//   v.reserve(c.Args(0));
//   for (auto _ : c) {
//     v.push_back(10);
//   }
//   BM::DoNotOptimize();
// }
//
// To prevent the re-ordering of instructions, use BM::DontReorder().
// TODO: Prevent re-ordering with memory_order_acquire_release.
//

#ifndef BM_H
#define BM_H

#include <sys/types.h>
#include <x86intrin.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace BM {

static const std::string kOutputFileFormatFlag = "output_format";
static const std::string kOutputFilePathFlag = "output_file";

// Testing only.
// --test_root_dir: By default, benchmarking library assumes system root is '/'.
static const std::string kTestRootDirFlag = "test_root_dir";
// If set, system root will be test_root_dir. Used in testing procfs and sysfs
// checks.

// TODO: --benchmark_enable_random_interleaving=True (default is False)
// TODO: --benchmark_warmup=True (default is False)
// TODO: --benchmark_repetitions={unsigned int} (default is 1)
// TODO: --benchmark_min_time={unsigned float} (default is 0.1 seconds)
// TODO: CSV and JSON output
struct Options {
  // The name of the compiled binary that uses bm. Usually argv[0].
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
  // On correctly processing the flag and storing it in Options, returns 0.
  uint32_t InsertCliFlag(char *argv) {
    if (!argv || !(argv[0] == '-' && argv[1] == '-')) {
      return 1;
    }
    if (argv && *argv == '\0') {
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

// Config stores values set via command line flags.
// Config is correctly set by calling BM::Initialize(argc, argv).
// TODO(REFACTOR): Add a Validate() method to Config to do all checks at once.
//  Maybe move closest candidate check to here instead of in CLI parsing?
static BM::Options Config;

// SystemCheck is a store of sysfs files BM checks at runtime before benchmarks.
// Depending on what values a machine's sysfs has, it'll make recommendations
//  to help improve benchmark predictions.
struct SystemCheck {
  std::string file_path_;
  std::string want_;
  std::string remedy_;
};

// TODO: add check for AMD x86 chip equivalent
// TODO: add check for Linux scaling governor
// TODO: add check for randomizing virtual address
static const std::vector<BM::SystemCheck> kSysfsChecks{
    {"/sys/devices/system/cpu/intel_pstate/no_turbo", "1",
     "Warning: Chip power frequency scaling is on. Recommend turning it off "
     "for more accurate results."},
};

struct Count {
  uint64_t cpu_time_ = 0;
  uint64_t wall_time_ = 0;
  uint64_t iterations_ = 0;
};

namespace Internal {

// Because this isn't available in C++11...
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&...args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

}  // namespace Internal

// State is an object that injects and extracts information from a benchmarked
// function.
// TODO: AddState method appends to the linked list
class Controller {
 public:
  Controller *next_ = nullptr;
  Count counters_;
  Controller() = default;

  // Make controller and iterable type.
  class iterator {
   public:
    iterator() = default;
    explicit iterator(const Controller *state) : current_(state) {
      // sample rdtsc for total time purpose
    }
    iterator operator++() {
      // We iterate if we have met the following conditions
      // 1 or min iteration count < total iters < 1e9
      // wall time > 5 * minimum time
      // cpu time > minimum time
      // time sample equals running average
      current_ = current_->next_;
      return *this;
    }

    bool operator!=(const iterator &other) {
      return other.current_ != this->current_;
    }

    // TODO: nullptr check
    const Controller &operator*() const { return *current_; }

    ~iterator() {
      // sample rdtsc here and diff to beginning timer
    }

   private:
    const Controller *current_;
  };
  iterator begin() const { return iterator(this); }
  iterator end() const { return iterator(); }
};

typedef void(Function)(BM::Controller &);

struct Benchmark {
  Benchmark(BM::Function *f, std::string &name) : f_(f), name_(name) {}
  std::string name_ = "";
  BM::Function *f_ = nullptr;
  std::unique_ptr<BM::Controller> controller_ =
      BM::Internal::make_unique<BM::Controller>();
};

static std::vector<std::unique_ptr<BM::Benchmark>> RegisteredBenchmarks;

static int32_t Register(std::string bm_name, BM::Function *bm_f) {
  // if (!bm_name) {
  //   std::cout << "Failed to register benchmark: No name passed\n";
  //   return -1;
  // }
  if (!bm_f) {
    std::cout << "Failed to register benchmark: No benchmark passed\n";
    return -1;
  }
  BM::RegisteredBenchmarks.push_back(
      BM::Internal::make_unique<BM::Benchmark>(bm_f, bm_name));
  return 0;
}

static void Initialize(int argc, char **argv) {
  uint32_t status = 0;
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
    path.append(kSysfsChecks[i].file_path_);
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
    if (sys_file_contents != kSysfsChecks[i].want_) {
      std::cout << kSysfsChecks[i].remedy_ << '\n';
    }
    sys_file.close();
  }
}

static std::unordered_map<std::string, BM::Count> Results;

static void Run() {
  for (const auto &b : RegisteredBenchmarks) {
    if (!b) continue;
    b->f_(*b->controller_);
    Results[b->name_] = b->controller_->counters_;
  }
}

static void ShutDown() {
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
  for (auto result_it = Results.begin(); result_it != Results.end();
       ++result_it) {
    out << "Name" << delim << result_it->first << '\n'
        << "CPU Time" << delim << result_it->second.cpu_time_ << '\n'
        << "Wall Time" << delim << result_it->second.wall_time_ << '\n'
        << "Iterations" << delim << result_it->second.iterations_ << '\n';
  }
  if (!Config.output_file_path_.empty()) {
    std::cout << "Generated " << Config.output_file_path_ << ". ";
  }
}

}  // namespace BM

// TODO: Args and Threads for BM.
// TODO: BM_Register(SomeBenchmark)->Args(a)
// TODO: BM_Register(SomeBenchmark)->ArgRange(a, b)
// TODO: BM_Register(SomeBenchmark)->ArgRange(a, b, jump)
// TODO: BM_Register(SomeBenchmark)->Threads(a)
//  Threads will require the construction of a ThreadManager that gets passed to
//  BM::State during construction inside Run()

// We use BM_NAME just so we can call BM::Register
#define BM_NAME(bm) bm##__LINE__

#define BM_Register(bm) static int BM_NAME(bm) = BM::Register(#bm, bm)

#define BM_Main()                   \
  int main(int argc, char **argv) { \
    BM::Initialize(argc, argv);     \
    BM::Run();                      \
    BM::ShutDown();                 \
    return 0;                       \
  }

#endif  // BM_H
