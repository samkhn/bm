# Test Output Flag Integration

from dataclasses import dataclass
import re
import subprocess
import sys


@dataclass
class Test:
    name: str
    want_regexp_stdout: list[str]


# WARNING: This test may be very flaky. Keep an eye out.
TEST_COUNT = 1
TESTS = [
    Test("TestNoOutputFlag",
         [
          "BM_VecPush",
          "CPU Time [1-9]\\d* reference cycles",
          "Wall Time [0-9]\\d* milliseconds",
         ],),
]


def test_output_flags():
    if len(sys.argv) != 2:
        print("ERROR: wrong number of args. "
              "Only one arg expected: path/to/executable")
        return -1
    binary_under_test = sys.argv[1]
    print(f"Test Time Integration. Using binary: {binary_under_test}")
    passed = 0
    for t in TESTS:
        test_call = [binary_under_test]
        test_run = subprocess.run(test_call, capture_output=True)
        got_stdout = test_run.stdout.decode()
        missing = []
        for w in t.want_regexp_stdout:
            search = re.findall(w, got_stdout)
            if not search:
                missing.append(w)
        if missing:
            print(f"stdout missing {missing}")
        else:
            passed += 1
    print(f"Test Time Integration. Passed {passed} out of {TEST_COUNT}")
    return 0


if __name__ == "__main__":
    test_output_flags()
