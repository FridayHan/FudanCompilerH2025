import difflib
import os

def compare_files(name):
    # 自动拼接路径
    tmp_path = f"test/tmp/hw8test{name}.s"
    ref_path = f"test/output_example/k9/hw8test{name}.s"

    if not os.path.exists(tmp_path):
        print(f"[❌] 文件不存在: {tmp_path}")
        return
    if not os.path.exists(ref_path):
        print(f"[❌] 文件不存在: {ref_path}")
        return

    with open(tmp_path, 'r') as f1, open(ref_path, 'r') as f2:
        tmp_lines = f1.readlines()
        ref_lines = f2.readlines()

    diff = list(difflib.unified_diff(ref_lines, tmp_lines,
                                      fromfile=ref_path,
                                      tofile=tmp_path,
                                      lineterm=''))
    if not diff:
        print(f"[✅] hw8test{name}.s 没有差异")
    else:
        print(f"[⚠️] hw8test{name}.s 存在差异：")
        for line in diff:
            print(line)

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="比较 tmp 和 k9 下的 .s 文件差异")
    parser.add_argument("file", help="省略 hw8test 的编号，如 '00' 表示比较 hw8test00.s")
    args = parser.parse_args()

    compare_files(args.file)

