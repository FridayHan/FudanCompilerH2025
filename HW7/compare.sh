#!/usr/bin/env bash

TEST_DIR="test"
REF_DIR="test/output_example"
has_diff=0

# 新增模式和单文件支持
mode="simple"
single_file=""

while getopts "vF:" opt; do
    case $opt in
        v)
            mode="verbose"
            ;;
        F)
            single_file="$OPTARG"
            ;;
        *)
            echo "用法: $0 [-v] [-F basename]"
            exit 1
            ;;
    esac
done

# 根据是否指定 single_file 来组装待比较文件列表
if [[ -n $single_file ]]; then
    # 确保 single_file 包含 -ssa.quad 后缀
    if [[ ! "$single_file" =~ -ssa.quad$ ]]; then
        single_file="$single_file.4-ssa.quad"
    fi
    files=("$TEST_DIR/$single_file")
    show_ok_count=0
else
    files=("$TEST_DIR"/*-ssa.quad)
    show_ok_count=1
fi

ok_count=0

for file in "${files[@]}"; do
    [ -e "$file" ] || { echo "文件未找到: $file"; has_diff=1; continue; }
    base=$(basename "$file")
    ref="$REF_DIR/$base"

    if [ ! -f "$ref" ]; then
        echo "Reference missing: $ref"
        has_diff=1
        continue
    fi

    diff_output=$(diff -u "$ref" "$file")
    if [[ -n $diff_output ]]; then
        has_diff=1
        if [[ $mode == "verbose" ]]; then
            echo "==== Diff: $base ===="
            echo "$diff_output"
        else
            echo "DIFF: $base"
        fi
    else
        echo "OK: $base"
        ((ok_count++))
    fi
done

if [[ $show_ok_count -eq 1 ]]; then
    echo "通过的测试数: $ok_count"
fi

exit $has_diff
