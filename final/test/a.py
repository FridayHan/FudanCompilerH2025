import os
import filecmp

# 两个目录
dir1 = 'semant'
dir2 = 'out'

# 获取两个目录下的文件名集合（只取文件，不取子目录）
files1 = set(f for f in os.listdir(dir1) if os.path.isfile(os.path.join(dir1, f)))
files2 = set(f for f in os.listdir(dir2) if os.path.isfile(os.path.join(dir2, f)))

# 取交集：即两个目录下同名的文件
common_files = files1 & files2

if not common_files:
    print("两个目录下没有同名文件。")
else:
    for filename in sorted(common_files):
        file1 = os.path.join(dir1, filename)
        file2 = os.path.join(dir2, filename)
        if filecmp.cmp(file1, file2, shallow=False):
            print(f"{filename} 相同")
        else:
            print(f"{filename} 不同")

