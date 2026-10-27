// Host test for arm9 config.c key combo helpers: parseKeyComboOption and menuComboToString
// (keyNames and both function bodies are extracted from config.c by run_combo_test.sh).
// Built with -DUB_PROBE, main() only calls menuComboToString once under full -fsanitize=undefined.
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "memory.h"

#include "extracted.inc"

#define NKEYS ((int)(sizeof(keyNames) / sizeof(keyNames[0])))

#ifdef UB_PROBE
int main(void)
{
    char out[256];
    menuComboToString(out, 1u); // loop always evaluates (1 << 31) for i == 31
    printf("ok   menuComboToString(1) -> %s\n", out);
    return 0;
}
#else

static int failures = 0;

static void check(int cond, const char *what)
{
    failures += !cond;
    printf("%s %s\n", cond ? "ok  " : "FAIL", what);
}

static void checkParse(const char *in, int wantRc, u32 want)
{
    u32 out = 0xDEADBEEF;
    int rc = parseKeyComboOption(&out, in);
    // On failure *out must be left at 0 (it is cleared up front)
    int ok = rc == wantRc && (rc == 0 ? out == want : out == 0);
    failures += !ok;
    printf("%s parse \"%s\" -> rc=%d %08x (want rc=%d %08x)\n", ok ? "ok  " : "FAIL", in, rc, out, wantRc,
           wantRc == 0 ? want : 0);
}

static void checkPrint(u32 combo, const char *want)
{
    char out[256];
    memset(out, 0x55, sizeof(out));
    menuComboToString(out, combo);
    int ok = strcmp(out, want) == 0;
    failures += !ok;
    printf("%s print %08x -> \"%s\" (want \"%s\")\n", ok ? "ok  " : "FAIL", combo, out, want);
}

static void checkRoundTrip(u32 combo)
{
    char out[256];
    menuComboToString(out, combo);
    u32 back = 0xDEADBEEF;
    int rc = parseKeyComboOption(&back, out);
    int ok = rc == 0 && back == combo;
    failures += !ok;
    printf("%s roundtrip %08x -> \"%s\" -> rc=%d %08x\n", ok ? "ok  " : "FAIL", combo, out, rc, back);
}

int main(void)
{
    check(NKEYS == 32, "keyNames has 32 entries");

    // Bits with a real name vs. the "?" placeholders
    u32 validMask = 0;
    for (int i = 0; i < NKEYS; i++)
        if (strcmp(keyNames[i], "?") != 0) validMask |= 1u << i;
    check(validMask == 0xFF10CFFFu, "valid key mask is 0xFF10CFFF");

    // Single keys
    checkParse("A", 0, 1u << 0);
    checkParse("Select", 0, 1u << 2);
    checkParse("ZR", 0, 1u << 15);
    checkParse("Touch", 0, 1u << 20);
    checkParse("CStick Right", 0, 1u << 24);
    checkParse("CPad Down", 0, 1u << 31);

    // Multi-key combos (Rosalina default L+Down+Select) and order independence / duplicates
    checkParse("L+Down+Select", 0, (1u << 9) | (1u << 7) | (1u << 2));
    checkParse("Select+Down+L", 0, (1u << 9) | (1u << 7) | (1u << 2));
    checkParse("A+A", 0, 1u);
    checkParse("CPad Up+CStick Left+ZL", 0, (1u << 30) | (1u << 25) | (1u << 14));

    // Case-insensitivity
    checkParse("select", 0, 1u << 2);
    checkParse("l+DOWN+sElEcT", 0, (1u << 9) | (1u << 7) | (1u << 2));
    checkParse("cstick right+touch", 0, (1u << 24) | (1u << 20));

    // Invalid input
    checkParse("", -1, 0);
    checkParse("A+", -1, 0);           // trailing '+'
    checkParse("A+B+", -1, 0);
    checkParse("+A", -1, 0);           // empty first name
    checkParse("A++B", -1, 0);         // empty name in the middle
    checkParse("+", -1, 0);
    checkParse("?", -1, 0);            // placeholder name must not select bits 12/13/16..19/21..23
    checkParse("A+?", -1, 0);
    checkParse("Foo", -1, 0);
    checkParse("A+Foo", -1, 0);
    checkParse("Selectt", -1, 0);
    checkParse("Selec", -1, 0);
    checkParse(" A", -1, 0);           // no whitespace trimming inside the parser
    checkParse("A ", -1, 0);
    checkParse("CStick", -1, 0);
    checkParse("CStick Rightxxxxx", -1, 0);       // 17 chars, truncated to 16 internally
    checkParse("CStick Rightxxxxxxxxxxxx+A", -1, 0);
    checkParse("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", -1, 0);

    // Printing
    checkPrint(0, "");
    checkPrint(1u, "A");
    checkPrint((1u << 9) | (1u << 7) | (1u << 2), "L+Down+Select");   // high bit first
    checkPrint(1u << 31, "CPad Down");
    checkPrint((1u << 31) | 1u, "CPad Down+A");

    // Every bit on its own: named bits round-trip, "?" bits print "?" which the parser rejects
    for (int i = 0; i < 32; i++)
    {
        u32 combo = 1u << i;
        if (validMask & combo)
            checkRoundTrip(combo);
        else
        {
            char out[256], what[64];
            menuComboToString(out, combo);
            u32 back;
            snprintf(what, sizeof(what), "bit %d prints \"?\" and is rejected", i);
            check(strcmp(out, "?") == 0 && parseKeyComboOption(&back, out) == -1, what);
        }
    }

    // Multi-key round trips, including every named key at once
    checkRoundTrip((1u << 9) | (1u << 7) | (1u << 2));
    checkRoundTrip((1u << 14) | (1u << 15) | (1u << 20) | (1u << 31));
    checkRoundTrip(validMask);

    // All 32 bits: longest possible output must fit the documented 160 bytes (incl. NUL)
    {
        char out[256];
        memset(out, 0x55, sizeof(out));
        menuComboToString(out, 0xFFFFFFFFu);
        size_t len = strlen(out);
        char what[96];
        snprintf(what, sizeof(what), "all 32 bits -> %u chars + NUL <= 160", (unsigned)len);
        check(len + 1 <= 160, what);
        check((unsigned char)out[len + 1] == 0x55, "no write past the terminating NUL");
        u32 back;
        check(parseKeyComboOption(&back, out) == -1, "all 32 bits string (contains \"?\") is rejected");
    }

    printf("%d failures\n", failures);
    return failures != 0;
}
#endif
