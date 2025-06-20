#!/bin/bash

# 定义两个文件夹路径
folder1="./test/"
folder2="./test/output_example/"

# 创建用于存储结果的数组
declare -a identical_files
declare -a different_files

# 遍历第一个文件夹中的所有.4.quad文件
for file1 in "$folder1"*.4.quad; do
    # 获取文件名（不含路径）
    filename=$(basename "$file1")
    
    # 检查第二个文件夹中是否存在同名文件
    file2="$folder2$filename"
    if [ -f "$file2" ]; then
        # 使用diff比较文件内容
        if diff -q "$file1" "$file2" > /dev/null; then
            identical_files+=("$filename")
        else
            different_files+=("$filename")
        fi
    fi
done

# 输出结果
echo "相同的文件:"
printf '%s\n' "${identical_files[@]}"
echo -e "\n不同的文件:"
printf '%s\n' "${different_files[@]}"

# 统计信息
echo -e "\n统计:"
echo "相同的文件数量: ${#identical_files[@]}"
echo "不同的文件数量: ${#different_files[@]}"
