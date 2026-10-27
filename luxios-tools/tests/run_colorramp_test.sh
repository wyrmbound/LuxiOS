#!/bin/bash
# Extracts the blackbody table, interpolate_color and colorramp_get_white_point from
# sysmodules/rosalina/source/redshift/colorramp.c and tests them on the host.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="$(cd "$HERE/../.." && pwd)"
WORK="$(mktemp -d)"
export PATH=/clangarm64/bin:$PATH
FILE="$SRC/sysmodules/rosalina/source/redshift/colorramp.c"

cat > "$WORK/memory.h" <<'EOF'
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
typedef int8_t s8; typedef int16_t s16; typedef int32_t s32; typedef int64_t s64;
EOF
# Table body only (without the declaration line and the closing "};")
tr -d '\r' < "$FILE" | sed -n '/^static const float blackbody_color\[\] = {/,/^};/p' | sed '1d;$d' > "$WORK/table.inc"
echo "#define TABLE_N $(grep -o '[0-9][0-9]*\.[0-9][0-9]*' "$WORK/table.inc" | wc -l)" > "$WORK/table_n.h"
{
    echo "static void"
    tr -d '\r' < "$FILE" | sed -n '/^interpolate_color(/,/^}/p'
    tr -d '\r' < "$FILE" | sed -n '/^void colorramp_get_white_point(/,/^}/p'
} > "$WORK/extracted.inc"
clang -std=gnu11 -O1 -fsanitize=undefined -fsanitize-trap=undefined -I"$WORK" "$HERE/colorramp_test.c" -o "$WORK/colorramp_test.exe"
"$WORK/colorramp_test.exe"
rm -rf "$WORK"
