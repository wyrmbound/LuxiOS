#!/bin/bash
# Extracts the LuxiFlap logic and renderer from rosalina's luxi_flap.c and tests them on the host.
# Optional argument: path of a PPM preview frame to write.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="$(cd "$HERE/../.." && pwd)"
FILE="$SRC/sysmodules/rosalina/source/menus/luxi_flap.c"
WORK="$(mktemp -d)"
export PATH=/clangarm64/bin:$PATH

mkdir -p "$WORK/3ds"
cat > "$WORK/3ds/types.h" <<'EOF'
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
typedef int8_t s8; typedef int16_t s16; typedef int32_t s32; typedef int64_t s64;
EOF
cat > "$WORK/stubs.h" <<'EOF'
#pragma once
#include <3ds/types.h>
#include "luxi_flap_sprite.h"
// Key bits as in libctru's hid.h
enum {
    KEY_A = 1u << 0, KEY_B = 1u << 1, KEY_SELECT = 1u << 2, KEY_START = 1u << 3,
    KEY_DRIGHT = 1u << 4, KEY_DLEFT = 1u << 5, KEY_DUP = 1u << 6, KEY_DDOWN = 1u << 7,
    KEY_R = 1u << 8, KEY_L = 1u << 9, KEY_X = 1u << 10, KEY_Y = 1u << 11,
    KEY_TOUCH = 1u << 20,
    KEY_CSTICK_RIGHT = 1u << 24, KEY_CSTICK_LEFT = 1u << 25, KEY_CSTICK_UP = 1u << 26, KEY_CSTICK_DOWN = 1u << 27,
    KEY_CPAD_RIGHT = 1u << 28, KEY_CPAD_LEFT = 1u << 29, KEY_CPAD_UP = 1u << 30, KEY_CPAD_DOWN = 1u << 31,
};
#define SCREEN_BOT_HEIGHT 240
#define SYSCLOCK_ARM11 268111856ULL
static u16 fakeFb[320 * 240];
#define FB_BOTTOM_VRAM_ADDR ((void *)fakeFb)
static inline u64 svcGetSystemTick(void) { return 0; }
EOF
{
    tr -d '\r' < "$FILE" | sed -n '/^\/\/ --- LuxiFlap logic begin ---/,/^\/\/ --- LuxiFlap logic end ---/p'
    # The drawing code up to (not including) the HUD, which needs the real font
    tr -d '\r' < "$FILE" | sed -n '/^_Static_assert/,/^static void flapDrawHud/p' | sed '$d'
} > "$WORK/extracted.inc"

clang -std=gnu11 -O1 -Wall -Wno-unused-function -Wno-unused-variable -fsanitize=undefined -fsanitize-trap=undefined \
    -I"$WORK" -I"$SRC/sysmodules/rosalina/include" "$HERE/luxi_flap_test.c" -o "$WORK/luxi_flap_test.exe"
"$WORK/luxi_flap_test.exe" "$@"
STATUS=$?
rm -rf "$WORK"
exit $STATUS
