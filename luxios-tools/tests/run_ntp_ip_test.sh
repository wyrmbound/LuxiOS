#!/bin/bash
# Extracts ntpParseIpv4 from sysmodules/rosalina/source/ntp.c and tests it on the host.
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
typedef uint8_t u8; typedef uint32_t u32;
EOF
tr -d '\r' < "$SRC/sysmodules/rosalina/source/ntp.c" | sed -n '/^bool ntpParseIpv4(/,/^}/p' > "$WORK/extracted.inc"
clang -std=gnu11 -O1 -fsanitize=undefined -fsanitize-trap=undefined -I"$WORK" "$HERE/ntp_ip_test.c" -o "$WORK/ntp_ip_test.exe"
"$WORK/ntp_ip_test.exe"
rm -rf "$WORK"
