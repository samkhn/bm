# Test Output Flag Integration

from dataclasses import dataclass
from pathlib import Path
import subprocess
import sys


@dataclass
class Test:
    name: str
    test_output_file_arg: str
    test_output_format_arg: str
    want_stdout: list[str]
    want_file_name: str
    want_file_contents: list[str]


TEST_COUNT = 5
TESTS = [
    Test("TestNoOutputFlag", "", "", [""], "", [""]),
    Test(
        "TestEmptyOutputArgsFail",
        "--output_format=",
        "--output_file=",
        ["Want form --{option_name}={option_value}"],
        "",
        [""],
    ),
    Test(
        "TestEmptyOutputFormatOk",
        "",
        "--output_file=results",
        ["Generated results"],
        "results",
        ["Running benchmarks in"],
    ),
    Test("TestEmptyFileOk", "--output_format=Text", "", ["Format: Text"], "", [""]),
    Test(
        "TestTextToFileOutput",
        "--output_format=Text",
        "--output_file=results",
        ["Generated results"],
        "results",
        ["Running benchmarks in", "Format: Text", "BM_Example"],
    ),
]


def test_output_flags():
    if len(sys.argv) != 2:
        print(
            "ERROR: wrong number of args. " "Only one arg expected: path/to/executable"
        )
        return -1
    binary_under_test = sys.argv[1]
    print(f"Test Output Flag Integration. Using binary: {binary_under_test}")
    passed = 0
    for t in TESTS:
        if t.test_output_file_arg and not t.want_file_contents:
            print(
                f"{t.name} is an invalid test. If a file is being generated, it should have expected output"
            )
            continue
        test_call = [
            binary_under_test,
            t.test_output_file_arg,
            t.test_output_format_arg,
        ]
        test_run = subprocess.run(test_call, capture_output=True)
        got_stdout = test_run.stdout.decode()
        got_file = ""
        if t.want_file_name:
            want_result_file = Path(binary_under_test).parent / t.want_file_name
            try:
                got_file = want_result_file.read_text()
                want_result_file.unlink(missing_ok=True)
            except Exception as err:
                print(
                    f"Failed test {t.name}."
                    f" {test_call} did not generate file {want_result_file.resolve()}"
                    f" Opening it threw the exception: {err}"
                )
                continue
        missing = []
        for w in t.want_stdout:
            if w not in got_stdout:
                missing.append(f"from stdout: {w}")
        if got_file:
            for w in t.want_file_contents:
                if w not in got_file:
                    missing.append(f"from file: {w}")
        if missing:
            print(
                f"Failed test {t.name}."
                f" {test_call} got stdout: [{got_stdout}] and file: [{got_file}]."
                f" Missing: {missing}."
            )
        else:
            passed += 1
    print(f"Test Output Flag Integration. Passed {passed} out of {TEST_COUNT}")
    return 0


if __name__ == "__main__":
    test_output_flags()
