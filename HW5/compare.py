import difflib
import os
import argparse

SRC_DIR = "test"
REF_DIR = "test/output_example"

def compare_single_file(filename, verbose=False):
    if not filename.endswith(".3.irp"):
        filename += ".3.irp"
    src_path = os.path.join(SRC_DIR, filename)
    ref_path = os.path.join(REF_DIR, filename)

    if not os.path.exists(src_path):
        print(f"[❌] 缺失源文件: {src_path}")
        return False
    if not os.path.exists(ref_path):
        print(f"[❌] 缺失参考文件: {ref_path}")
        return False

    with open(src_path, 'r') as f1, open(ref_path, 'r') as f2:
        src_lines = f1.readlines()
        ref_lines = f2.readlines()

    diff = list(difflib.unified_diff(ref_lines, src_lines,
                                     fromfile=ref_path,
                                     tofile=src_path,
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

    # 获取源文件夹中的所有.3.irp文件
    src_files = [f for f in os.listdir(SRC_DIR) if f.endswith(".3.irp")]
    
    for filename in sorted(src_files):
        # 检查参考文件夹中是否存在同名文件
        if not os.path.exists(os.path.join(REF_DIR, filename)):
            continue
            
        success = compare_single_file(filename, verbose)
        total += 1
        if success:
            passed += 1

    print(f"\n✅ {passed} / {total} 测试通过")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="比较 test 和 test/output_example 下的 .3.irp 文件差异")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--file", help="指定文件名，如 'xxx.3.irp'")
    group.add_argument("--all", action="store_true", help="对所有存在的参考文件进行比较")
    parser.add_argument("--verbose", action="store_true", help="展示详细差异")
    args = parser.parse_args()

    if args.all:
        compare_all(verbose=args.verbose)
    else:
        compare_single_file(args.file, verbose=args.verbose)
