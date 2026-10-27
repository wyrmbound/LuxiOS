#!/bin/bash
# Extracts Cheat_IsCodeLine and Cheat_GetCode from sysmodules/rosalina/source/menus/cheats.c and tests them on the host.
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
for fn in "bool Cheat_IsCodeLine" "u64 Cheat_GetCode"; do
    tr -d '\r' < "$SRC/sysmodules/rosalina/source/menus/cheats.c" | sed -n "/^static $fn(/,/^}/p" >> "$WORK/extracted.inc"
done
clang -std=gnu11 -O1 -fsanitize=undefined -fsanitize-trap=undefined -I"$WORK" "$HERE/cheat_line_test.c" -o "$WORK/cheat_line_test.exe"
"$WORK/cheat_line_test.exe"
rm -rf "$WORK"
