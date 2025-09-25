#!/bin/bash
PASS=0
FAIL=0

INPUT_DIR="tests/input"
OUTPUT_DIR="build/output"

mkdir -p "$OUTPUT_DIR"

shopt -s nullglob
for infile in "$INPUT_DIR"/input*.txt; do
    testname=$(basename "$infile" .txt)

    encrypted="$OUTPUT_DIR/${testname}_encrypted.out"
    decrypted="$OUTPUT_DIR/${testname}_decrypted.out"

    prefix="${testname:0:3}"

    # XOR
    export LD_LIBRARY_PATH=build/xor_lib
    ./encrypt 10 < "$infile" > "$encrypted"
    ./encrypt 10 < "$encrypted" > "$decrypted"
    if diff -q "$infile" "$decrypted" > /dev/null; then
        echo "[PASS] $testname XOR"
        PASS=$((PASS+1))
    else
        echo "[FAIL] $testname XOR"
        FAIL=$((FAIL+1))
    fi

    # ROT
    export LD_LIBRARY_PATH=build/rot_lib
    ./encrypt 1 < "$infile" > "$encrypted"
    ./encrypt -1 < "$encrypted" > "$decrypted"
    if diff -q "$infile" "$decrypted" > /dev/null; then
        echo "[PASS] $testname ROT"
        PASS=$((PASS+1))
    else
        echo "[FAIL] $testname ROT"
        FAIL=$((FAIL+1))
    fi

    # Caesar
    export LD_LIBRARY_PATH=build/caesar_lib
    ./encrypt 10 < "$infile" > "$encrypted"
    ./encrypt -10 < "$encrypted" > "$decrypted"
    if diff -q "$infile" "$decrypted" > /dev/null; then
        echo "[PASS] $testname Caesar"
        PASS=$((PASS+1))
    else
        echo "[FAIL] $testname Caesar"
        FAIL=$((FAIL+1))
    fi
done

echo "=== Summary ==="
echo "PASS: $PASS"
echo "FAIL: $FAIL"
