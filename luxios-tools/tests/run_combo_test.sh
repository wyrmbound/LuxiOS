#!/bin/bash
# Extracts keyNames, parseKeyComboOption and menuComboToString from arm9/source/config.c and tests them on the host.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="$(cd "$HERE/../.." && pwd)"
WORK="$(mktemp -d)"
export PATH=/clangarm64/bin:$PATH

cat > "$WORK/memory.h" <<'EOF'
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
typedef int8_t s8; typedef int16_t s16; typedef int32_t s32; typedef int64_t s64;
EOF
{
    tr -d '\r' < "$SRC/arm9/source/config.c" | sed -n '/^static const char \*keyNames\[\] = {/,/^};/p'
    for fn in "int parseKeyComboOption" "void menuComboToString"; do
        tr -d '\r' < "$SRC/arm9/source/config.c" | sed -n "/^static $fn(/,/^}/p"
    done
} > "$WORK/extracted.inc"

FLAGS="-std=gnu11 -O1 -fsanitize=undefined -fsanitize-trap=undefined -I$WORK"
FAILED=0

# Functional checks. shift-base is excluded here only so the probe below can report it separately
clang $FLAGS -fno-sanitize=shift-base "$HERE/combo_test.c" -o "$WORK/combo_test.exe"
"$WORK/combo_test.exe" || FAILED=1

# UB probe with the full sanitizer set: menuComboToString evaluates (1 << 31) on a signed int
clang $FLAGS -DUB_PROBE "$HERE/combo_test.c" -o "$WORK/combo_probe.exe"
if ! "$WORK/combo_probe.exe"; then
    echo "FAIL menuComboToString traps under -fsanitize=undefined (signed 1 << 31 in 'combo & (1 << i)')"
    FAILED=1
fi

rm -rf "$WORK"
exit $FAILED
