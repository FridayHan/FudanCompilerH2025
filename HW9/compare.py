import difflib
import os
import argparse

TMP_DIR = "test/tmp"
REF_DIR = "test/output_example/k9"

def compare_single_file(filename, verbose=False):
    if not filename.endswith(".s"):
        filename += ".s"
    tmp_path = os.path.join(TMP_DIR, filename)
    ref_path = os.path.join(REF_DIR, filename)

    if not os.path.exists(tmp_path):
        print(f"[❌] 缺失生成文件: {tmp_path}")
        return False
    if not os.path.exists(ref_path):
        print(f"[❌] 缺失参考文件: {ref_path}")
        return False

    with open(tmp_path, 'r') as f1, open(ref_path, 'r') as f2:
        tmp_lines = f1.readlines()
        ref_lines = f2.readlines()

    diff = list(difflib.unified_diff(ref_lines, tmp_lines,
                                     fromfile=ref_path,
                                     tofile=tmp_path,
                                     lineterm=''))

    if not diff:
        print(f"[✅] {filename} 没有差异")
        return True
    else:
        print(f"[⚠️] {filename} 存在差异")
        if verbose:
            for line in diff:
                print(line)
        return False

def compare_all(verbose=False):
    passed = 0
    total = 0

    for filename in sorted(os.listdir(REF_DIR)):
        if not filename.endswith(".s"):
            continue
        success = compare_single_file(filename, verbose)
        total += 1
        if success:
            passed += 1

    print(f"\n✅ {passed} / {total} 测试通过")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="比较 tmp 和 k9 下的 .s 文件差异")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--file", help="指定文件名，如 'hw8test00.s' 或 'bubblesort.s'")
    group.add_argument("--all", action="store_true", help="对所有参考文件进行比较")
    parser.add_argument("--verbose", action="store_true", help="展示详细差异")
    args = parser.parse_args()

    if args.all:
        compare_all(verbose=args.verbose)
    else:
        compare_single_file(args.file, verbose=args.verbose)
