# Test Sysfs Scan Integration

from dataclasses import dataclass
import os
from pathlib import Path
import subprocess
import sys
import tempfile


@dataclass
class Test:
    name: str
    sysfs_file: str  # if this string is empty, we won't create a file
    input: str
    want_output: str


TEST_COUNT = 2
TESTS = [
    Test(
        "TestSysFsIntelTurboOff",
        "sys/devices/system/cpu/intel_pstate/no_turbo",
        "1",
        "",
    ),
    Test(
        "TestSysFsIntelTurboOn",
        "sys/devices/system/cpu/intel_pstate/no_turbo",
        "0",
        "Chip power frequency scaling is on",
    ),
]


def test_sysfs_scan():
    if len(sys.argv) != 2:
        print(
            "ERROR: wrong number of args. " "Only one arg expected: path/to/executable"
        )
        return -1
    binary_under_test = sys.argv[1]
    print(f"Test SysFS Scan Integration. Using binary: {binary_under_test}")
    passed = 0
    test_tmp_dir = tempfile.gettempdir()
    for t in TESTS:
        test_file_loc = os.path.join(test_tmp_dir, t.sysfs_file)
        Path(test_file_loc).parent.mkdir(parents=True, exist_ok=True)
        f = open(test_file_loc, "w")
        f.write(t.input)
        test_call = [binary_under_test] + ["--test_root_dir=" + test_tmp_dir]
        test_run = subprocess.run(test_call, capture_output=True)
        if t.want_output not in test_run.stdout.decode():
            print(
                f"Failed test {t.name}."
                f" {test_call} got [{test_run.stdout.decode()}]."
                f" Did not contain [{t.want_output}]."
            )
        else:
            passed += 1
        f.close()
        os.remove(test_file_loc)
    print(f"Test SysFS Scan Integration. Passed {passed} out of {TEST_COUNT}")
    return 0


if __name__ == "__main__":
    test_sysfs_scan()
