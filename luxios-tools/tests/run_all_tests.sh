#!/bin/bash
# Runs every run_*_test.sh in this directory and prints a summary. Exits non-zero if any of them failed.
HERE="$(cd "$(dirname "$0")" && pwd)"
PASSED=()
FAILED=()

for script in "$HERE"/run_*_test.sh; do
    name="$(basename "$script")"
    echo "=== $name"
    if bash "$script"; then
        PASSED+=("$name")
    else
        FAILED+=("$name")
    fi
    echo
done

echo "=== Summary: ${#PASSED[@]} passed, ${#FAILED[@]} failed"
for name in "${FAILED[@]}"; do
    echo "FAILED: $name"
done
[ ${#FAILED[@]} -eq 0 ]
