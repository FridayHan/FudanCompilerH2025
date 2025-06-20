import difflib
import os
import argparse

SRC_DIR = "test"
REF_DIR = "test/output_example"

def compare_single_file(filename, file_type, verbose=False):
    """比较单个文件的差异"""
    if not filename.endswith(file_type):
        filename += file_type
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
    """比较所有文件"""
    passed_normal = 0
    passed_semant = 0
    total_normal = 0
    total_semant = 0

    # 获取源文件夹中的所有.2.ast和.2-semant.ast文件
    src_files_normal = [f for f in os.listdir(SRC_DIR) if f.endswith(".2.ast")]
    src_files_semant = [f for f in os.listdir(SRC_DIR) if f.endswith(".2-semant.ast")]
    
    # 比较普通AST文件
    print("\n=== 比较 .2.ast 文件 ===")
    for filename in sorted(src_files_normal):
        # 检查参考文件夹中是否存在同名文件
        if not os.path.exists(os.path.join(REF_DIR, filename)):
            continue
            
        success = compare_single_file(filename, ".2.ast", verbose)
        total_normal += 1
        if success:
            passed_normal += 1
    
    # 比较语义分析AST文件
    print("\n=== 比较 .2-semant.ast 文件 ===")
    for filename in sorted(src_files_semant):
        # 检查参考文件夹中是否存在同名文件
        if not os.path.exists(os.path.join(REF_DIR, filename)):
            continue
            
        success = compare_single_file(filename, ".2-semant.ast", verbose)
        total_semant += 1
        if success:
            passed_semant += 1

    print(f"\n✅ .2.ast: {passed_normal} / {total_normal} 测试通过")
    print(f"✅ .2-semant.ast: {passed_semant} / {total_semant} 测试通过")

def main():
    parser = argparse.ArgumentParser(description="比较AST文件差异")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--file", help="指定文件的基础名称，如 'xxx'")
    group.add_argument("--all", action="store_true", help="对所有存在的参考文件进行比较")
    parser.add_argument("--type", choices=["ast", "semant"], 
                       help="指定比较类型: ast(.2.ast), semant(.2-semant.ast)")
    parser.add_argument("--verbose", action="store_true", help="展示详细差异")
    args = parser.parse_args()

    if args.all:
        compare_all(verbose=args.verbose)
    else:
        if args.type == "ast" or args.type is None:
            compare_single_file(args.file, ".2.ast", verbose=args.verbose)
        
        if args.type == "semant" or args.type is None:
            compare_single_file(args.file, ".2-semant.ast", verbose=args.verbose)

if __name__ == "__main__":
    main()
