// Host test for rosalina cheats.c code-line parsing: Cheat_IsCodeLine and Cheat_GetCode
// (function bodies are extracted from sysmodules/rosalina/source/menus/cheats.c by run_cheat_line_test.sh).
#include <stdio.h>
#include <string.h>
#include "memory.h"

#include "extracted.inc"

static int failures = 0;

static void checkLine(const char *in, bool wantCode, u64 want)
{
    bool isCode = Cheat_IsCodeLine(in);
    u64 code = isCode ? Cheat_GetCode(in) : 0;
    int ok = isCode == wantCode && (!isCode || code == want);
    failures += !ok;
    printf("%s \"%s\" -> %d %016llx (want %d %016llx)\n", ok ? "ok  " : "FAIL", in, isCode,
           (unsigned long long)code, wantCode, (unsigned long long)want);
}

int main(void)
{
    // Valid lines
    checkLine("00000000 00000000", true, 0);
    checkLine("12345678 9ABCDEF0", true, 0x123456789ABCDEF0ull);
    checkLine("FFFFFFFF FFFFFFFF", true, 0xFFFFFFFFFFFFFFFFull);
    checkLine("DD000000 00000104", true, 0xDD00000000000104ull);   // key code line
    checkLine("abcdef01 23456789", true, 0xABCDEF0123456789ull);   // lowercase hex
    checkLine("aBcDeF01 2345678f", true, 0xABCDEF012345678Full);   // mixed case

    // Wrong length
    checkLine("", false, 0);
    checkLine("1234567 89ABCDEF", false, 0);        // 7 + 8
    checkLine("12345678 9ABCDEF", false, 0);        // 8 + 7
    checkLine("12345678 9ABCDEF01", false, 0);      // 8 + 10
    checkLine("123456789ABCDEF0", false, 0);        // no separator
    checkLine("12345678 9ABCDEF0 ", false, 0);      // trailing space (caller strips whitespace first)
    checkLine(" 12345678 9ABCDEF0", false, 0);      // leading space

    // Separator problems / extra spaces
    checkLine("12345678  9ABCDEF", false, 0);       // two spaces, still 17 chars
    checkLine("12345678  9ABCDEF0", false, 0);      // two spaces
    checkLine("12345678\t9ABCDEF0", false, 0);      // tab separator
    checkLine("1234567809ABCDEF0", false, 0);       // '0' where the space belongs
    checkLine("1234 678 9ABCDEF0", false, 0);
    checkLine("12345678 9ABC EF0", false, 0);

    // Non-hex characters in either half
    checkLine("1234567G 9ABCDEF0", false, 0);
    checkLine("12345678 9ABCDEFG", false, 0);
    checkLine("g2345678 9abcdef0", false, 0);
    checkLine("12345678 9abcdefx", false, 0);
    checkLine("0x345678 9ABCDEF0", false, 0);
    checkLine("12345678 -ABCDEF0", false, 0);
    checkLine("[Cheat name here]", false, 0);       // 17 chars, not a code
    checkLine("12345678:9ABCDEF0", false, 0);

    // Each position of each half: a single bad char must reject
    for (int pos = 0; pos < 17; pos++)
    {
        if (pos == 8) continue;
        char line[18];
        memcpy(line, "12345678 9ABCDEF0", 18);
        line[pos] = 'Z';
        checkLine(line, false, 0);
    }

    // Embedded NUL: strnlen stops at it, so the line is too short
    {
        char line[18];
        memcpy(line, "12345678 9ABCDEF0", 18);
        line[12] = '\0';
        bool isCode = Cheat_IsCodeLine(line);
        failures += isCode;
        printf("%s embedded NUL -> %d (want 0)\n", isCode ? "FAIL" : "ok  ", isCode);
    }

    // Each nibble lands in the right place
    for (int pos = 0; pos < 17; pos++)
    {
        if (pos == 8) continue;
        char line[18];
        memcpy(line, "00000000 00000000", 18);
        line[pos] = 'f';
        int nibble = pos < 8 ? 15 - pos : 16 - pos;
        checkLine(line, true, 0xFull << (4 * nibble));
    }

    printf("%d failures\n", failures);
    return failures != 0;
}
