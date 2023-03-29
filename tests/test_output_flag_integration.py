# Test Output Flag Integration

from dataclasses import dataclass
import subprocess
import sys

@dataclass
class Test:
    name: str
    test_output_file_arg: str
    test_output_format_arg: str
    want_output: str

    
TEST_COUNT = 5
TESTS = [
    Test("TestNoOutputFlag", "", "", ""),
    Test("TestEmptyOutputArgsFail",
         "--output_format=", "--output_file=",
         "Want form --{option_name}={option_value}"),
    Test("TestEmptyOutputFormatOk", "", "--output_file=results",
         "Generated results"),
    Test("TestEmptyFileOk", "--output_format=Text", "", "Format: Text"),
    Test("TestTextToFileOutput",
         "--output_format=Text", "--output_file=results",
         "Format: Text. Generated results")
]


def test_output_flags():
    if (len(sys.argv) != 2):
        print("ERROR: wrong number of args. "
              "Only one arg expected: path/to/executable")
        return -1
    binary_under_test = sys.argv[1]
    print(f"Test Output Flag Integration. Using binary: {binary_under_test}")
    passed = 0
    for t in TESTS:
        test_call = [binary_under_test,
                     t.test_output_file_arg, t.test_output_format_arg]
        test_run = subprocess.run(test_call, capture_output=True)
        if t.want_output not in test_run.stdout.decode():
            print(f"Failed test {t.name}."
                  f" {test_call} got [{test_run.stdout.decode()}]."
                  f" Did not contain [{t.want_output}].")
        else:
            passed += 1
    print(f"Test Output Flag Integration. Passed {passed} out of {TEST_COUNT}")
    return 0


if __name__ == '__main__':
    test_output_flags()
