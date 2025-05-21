#!/bin/bash

REF_DIR="test/output_example/k9"
OUT_DIR="test"
PASS=0
TOTAL=0

echo "开始测试对比..."

for ref in $REF_DIR/*.4-xml.clr; do
    filename=$(basename "$ref")
    testname="${filename%.4-xml.clr}"

    YOUR_OUTPUT="$OUT_DIR/${testname}.4-xml.clr"

    if [[ ! -f "$YOUR_OUTPUT" ]]; then
        echo "❌ 缺失输出: $YOUR_OUTPUT"
        continue
    fi

    if diff -q "$ref" "$YOUR_OUTPUT" > /dev/null; then
        echo "✅ 通过: $filename"
        ((PASS++))
    else
        echo "❌ 不通过: $filename"
    fi

    ((TOTAL++))
done

echo "========================="
echo "通过数量: $PASS / $TOTAL"
echo "========================="
