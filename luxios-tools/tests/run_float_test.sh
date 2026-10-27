#!/bin/bash
# Extracts encodedFloatToString from arm9/source/config.c and tests it on the host.
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
tr -d '\r' < "$SRC/arm9/source/config.c" | sed -n '/^static int encodedFloatToString/,/^}/p' > "$WORK/extracted.inc"
cp "$SRC/arm9/source/fmt.c" "$SRC/arm9/source/fmt.h" "$WORK/"
RENAME="-Dsnprintf=lsnprintf -Dvsnprintf=lvsnprintf -Dsprintf=lsprintf -Dvsprintf=lvsprintf -fno-builtin"
clang -std=gnu11 -O1 -I"$WORK" $RENAME -c "$WORK/fmt.c" -o "$WORK/fmt.o"
clang -std=gnu11 -O1 -I"$WORK" "$HERE/float_test.c" "$WORK/fmt.o" -o "$WORK/float_test.exe"
"$WORK/float_test.exe"
rm -rf "$WORK"
