# Test Flags Integration

from dataclasses import dataclass
import subprocess
import sys

@dataclass
class Test:
	name: str
	input_flags: list[str]
	want_output: str

TEST_COUNT = 5
TESTS = [
	Test('TestNoFlagsIsOkay', [''], ''),
	Test('TestInvalidFlagName', ['--=test'], 'Error with flag'),
	Test('TestSetInvalidTestRootDir', ['--test_root_dir'], 'Error with flag'),
	Test('TestSetTestRootWithNoDir', ['--test_root_dir='], 'Error with flag'),
	Test('TestSetTestRootDir', ['--test_root_dir=/path'], 'TEST FLAG SET')
]


def test_flags_binary():
	if (len(sys.argv) != 2):
		print("ERROR: wrong number of args. Only one arg expected: path/to/executable")
		return -1
	binary_under_test = sys.argv[1]
	print(f"Test Flags Integration. Using binary: {binary_under_test}")
	passed = 0
	for t in TESTS:
		test_call = [binary_under_test] + t.input_flags
		test_run = subprocess.run(test_call, capture_output=True)
		if not t.want_output in test_run.stdout.decode():
			print(f"Failed test {t.name}. {test_call} got [{test_run.stdout.decode()}]. Did not contain [{t.want_output}].")
		else:
			print(f"Passed test {t.name}.")
			passed += 1
	print(f"Test Flags Integration. Passed {passed} out of {TEST_COUNT}")
	return 0


if __name__ == '__main__':
	test_flags_binary()
