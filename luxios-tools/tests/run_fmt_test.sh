#!/bin/bash
# Builds fmt.c (arm9 and rosalina copies) for the host and runs fmt_test.c against each.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="$(cd "$HERE/../.." && pwd)"
WORK="$(mktemp -d)"
export PATH=/clangarm64/bin:$PATH

# Minimal stand-in for the firmware's memory.h / 3ds types
cat > "$WORK/memory.h" <<'EOF'
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
typedef int8_t s8; typedef int16_t s16; typedef int32_t s32; typedef int64_t s64;
EOF
mkdir -p "$WORK/3ds" && echo '#include "../memory.h"' > "$WORK/3ds/types.h"

RENAME="-Dsnprintf=lsnprintf -Dvsnprintf=lvsnprintf -Dsprintf=lsprintf -Dvsprintf=lvsprintf -fno-builtin"
for copy in arm9/source rosalina; do
    if [ "$copy" = rosalina ]; then f=$SRC/sysmodules/rosalina/source/fmt.c; h=$SRC/sysmodules/rosalina/include/fmt.h
    else f=$SRC/arm9/source/fmt.c; h=$SRC/arm9/source/fmt.h; fi
    cp "$f" "$WORK/fmt.c"; cp "$h" "$WORK/fmt.h"
    clang -std=gnu11 -O1 -fsanitize=undefined -fsanitize-trap=undefined -I"$WORK" $RENAME -c "$WORK/fmt.c" -o "$WORK/fmt.o"
    clang -std=gnu11 -O1 -fsanitize=undefined -fsanitize-trap=undefined "$HERE/fmt_test.c" "$WORK/fmt.o" -o "$WORK/fmt_test.exe"
    echo "== $copy =="
    "$WORK/fmt_test.exe"
done
rm -rf "$WORK"
