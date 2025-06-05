import difflib
import os
import argparse

TMP_DIR = "test/tmp"
REF_DIR = "test/output_example"

COLOR_RESET = "\033[0m"
COLOR_GREEN = "\033[32m"
COLOR_RED = "\033[31m"
COLOR_YELLOW = "\033[33m"

def compare_single_file(filename, verbose=False):
    if not filename.endswith(".4-ssa-opt.quad"):
        filename += ".4-ssa-opt.quad"

    tmp_path = os.path.join(TMP_DIR, filename)
    ref_path = os.path.join(REF_DIR, filename)

    if not os.path.exists(tmp_path):
        print(f"{COLOR_RED}Missing generated file:{COLOR_RESET} {tmp_path}")
        return False
    if not os.path.exists(ref_path):
        print(f"{COLOR_RED}Missing reference file:{COLOR_RESET} {ref_path}")
        return False

    with open(tmp_path, 'r') as f1, open(ref_path, 'r') as f2:
        tmp_lines = f1.readlines()
        ref_lines = f2.readlines()

    diff = list(difflib.unified_diff(ref_lines, tmp_lines,
                                     fromfile=ref_path,
                                     tofile=tmp_path,
                                     lineterm=''))

    if not diff:
        print(f"{COLOR_GREEN}PASS:{COLOR_RESET} {filename} has no differences")
        return True
    else:
        print(f"{COLOR_YELLOW}DIFFER:{COLOR_RESET} {filename} has differences")
        if verbose:
            for line in diff:
                print(line)
        return False

def compare_all(verbose=False):
    passed = 0
    total = 0

    for filename in sorted(os.listdir(REF_DIR)):
        if not filename.endswith(".4-ssa-opt.quad"):
            continue
        success = compare_single_file(filename, verbose)
        total += 1
        if success:
            passed += 1

    print(f"\n{COLOR_GREEN}Passed:{COLOR_RESET} {passed} / {total} tests")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Compare .4-ssa-opt.quad files between tmp and output_example directories")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--file", help="Specify a file name, such as 'hw8test00' or 'bubblesort'")
    group.add_argument("--all", action="store_true", help="Compare all reference files")
    parser.add_argument("--verbose", action="store_true", help="Show detailed differences")
    args = parser.parse_args()

    if args.all:
        compare_all(verbose=args.verbose)
    else:
        compare_single_file(args.file, verbose=args.verbose)
