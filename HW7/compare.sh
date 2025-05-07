#!/usr/bin/env bash

TEST_DIR="test"
REF_DIR="test/output_example"
has_diff=0

# ANSI 颜色代码
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # 无颜色

mode="simple"
single_file=""
ok_count=0

while getopts "vF:" opt; do
    case $opt in
        v)
            mode="verbose"
            ;;
        F)
            single_file="$OPTARG"
            ;;
        *)
            echo -e "${RED}用法: $0 [-v] [-F basename]${NC}"
            exit 1
            ;;
    esac
done

if [[ -n $single_file ]]; then
    if [[ ! "$single_file" =~ -ssa.quad$ ]]; then
        single_file="$single_file.4-ssa.quad"
    fi
    files=("$TEST_DIR/$single_file")
    show_ok_count=0
else
    files=("$TEST_DIR"/*-ssa.quad)
    show_ok_count=1
fi

for file in "${files[@]}"; do
    [ -e "$file" ] || { echo -e "${RED}文件未找到: $file${NC}"; has_diff=1; continue; }
    base=$(basename "$file")
    ref="$REF_DIR/$base"

    if [ ! -f "$ref" ]; then
        echo -e "${RED}Reference missing: $ref${NC}"
        has_diff=1
        continue
    fi

    diff_output=$(diff -u "$ref" "$file")
    if [[ -n $diff_output ]]; then
        has_diff=1
        if [[ $mode == "verbose" ]]; then
            echo -e "${RED}==== Diff: $base ====${NC}"
            echo "$diff_output"
        else
            echo -e "${RED}DIFF: $base${NC}"
        fi
    else
        echo -e "${GREEN}OK: $base${NC}"
        ((ok_count++))
    fi
done

if [[ $show_ok_count -eq 1 ]]; then
    echo -e "${GREEN}总共通过 $ok_count 个测试${NC}"
fi

exit $has_diff
