#!/bin/bash
# Extracts the update-RomFS mount detection loops from sysmodules/loader/source/patcher.c (patchLayeredFs) and
# memsearch from sysmodules/loader/source/memory.c, and tests them on the host.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="$(cd "$HERE/../.." && pwd)"
WORK="$(mktemp -d)"
export PATH=/clangarm64/bin:$PATH
PATCHER="$SRC/sysmodules/loader/source/patcher.c"

cat > "$WORK/memory.h" <<'EOF'
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
typedef int8_t s8; typedef int16_t s16; typedef int32_t s32; typedef int64_t s64;
EOF
tr -d '\r' < "$SRC/sysmodules/loader/source/memory.c" | sed -n '/^u8 \*memsearch(/,/^}/p' > "$WORK/memsearch.inc"
{
    tr -d '\r' < "$PATCHER" | sed -n '/static const char \*updateRomFsMounts\[\] = {/,/};/p'
    echo "    u32 updateRomFsIndex;"
    tr -d '\r' < "$PATCHER" | grep 'const u32 nbCandidates = '
    # From the "//Locate update RomFS" comment up to (not including) the line that picks the mount
    tr -d '\r' < "$PATCHER" | sed -n '/\/\/Locate update RomFS/,/updateRomFsMount = updateRomFsMounts\[updateRomFsIndex\];/p' | sed '$d'
} > "$WORK/locate.inc"

# Make sure the extraction still finds what the test expects
for want in 'u8 \*memsearch(' ; do grep -q "$want" "$WORK/memsearch.inc" || { echo "FAIL extraction: memsearch not found"; exit 1; }; done
for want in 'updateRomFsMounts\[\] = {' 'nbCandidates = ' 'Locate update RomFS' 'Second pass' '& 3) == 0'; do
    grep -q "$want" "$WORK/locate.inc" || { echo "FAIL extraction: '$want' not found in patcher.c"; exit 1; }
done

clang -std=gnu11 -O1 -fsanitize=undefined -fsanitize-trap=undefined -I"$WORK" "$HERE/romfs_mount_test.c" -o "$WORK/romfs_mount_test.exe"
"$WORK/romfs_mount_test.exe"
rm -rf "$WORK"
