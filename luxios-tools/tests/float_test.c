// Host test for arm9 config.c float helpers: encodedFloatToString and parseDecFloatOption
// (function bodies are extracted from config.c by run_float_test.sh).
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "memory.h"

#define FLOAT_CONV_MULT 100000000ll
#define FLOAT_CONV_PRECISION 8u
int lsprintf(char *buf, const char *fmt, ...);
#define sprintf lsprintf

#define strchrnul test_strchrnul
static char *strchrnul(const char *s, int c)
{
    while (*s && *s != c) s++;
    return (char *)s;
}

#include "extracted.inc"

static int failures = 0;

static void checkPrint(s64 val, const char *want)
{
    char out[64];
    encodedFloatToString(out, val);
    int ok = strcmp(out, want) == 0;
    failures += !ok;
    printf("%s print %lld -> %s (want %s)\n", ok ? "ok  " : "FAIL", (long long)val, out, want);
}

static void checkParse(const char *in, int wantRc, s64 want)
{
    s64 out = 12345;
    int rc = parseDecFloatOption(&out, in, -10 * FLOAT_CONV_MULT, 10 * FLOAT_CONV_MULT);
    int ok = rc == wantRc && (rc != 0 || out == want);
    failures += !ok;
    printf("%s parse \"%s\" -> rc=%d %lld (want rc=%d %lld)\n", ok ? "ok  " : "FAIL", in, rc, (long long)out, wantRc, (long long)want);
}

int main(void)
{
    checkPrint(100000000ll, "1");
    checkPrint(110000001ll, "1.10000001");   // was saved as "1.01"
    checkPrint(123456789ll, "1.23456789");   // was saved as "1"
    checkPrint(125000000ll, "1.25");
    checkPrint(10000000ll,  "0.1");
    checkPrint(-150000000ll, "-1.5");
    checkPrint(0, "0");
    checkPrint(-50000000ll, "-0.5");          // sign used to be lost

    checkParse("1.5", 0, 150000000ll);
    checkParse("-1.5", 0, -150000000ll);      // was parsed as +0.5
    checkParse("-1", 0, -100000000ll);        // was parsed as +1
    checkParse("-0.25", 0, -25000000ll);
    checkParse("+2", 0, 200000000ll);
    checkParse(".5", 0, 50000000ll);
    checkParse("-.5", 0, -50000000ll);
    checkParse("--1", -1, 0);
    checkParse("+-1", -1, 0);
    checkParse("-", -1, 0);
    checkParse("", -1, 0);
    checkParse("1.2x", -1, 0);
    checkParse("11", -1, 0);                  // out of range

    // Integer parser: empty values used to parse as 0, and long numbers wrapped around
    static const struct { const char *in; int wantRc; s64 want; } ints[] = {
        { "", -1, 0 }, { "-", -1, 0 }, { "+", -1, 0 }, { "0", 0, 0 }, { "42", 0, 42 }, { "-7", 0, -7 },
        { "999999999999999999", 0, 999999999999999999LL }, { "18446744073709551617", -1, 0 },
    };
    for (size_t i = 0; i < sizeof(ints) / sizeof(ints[0]); i++) {
        s64 v = 12345;
        int rc = parseDecIntOptionImpl(&v, ints[i].in, strlen(ints[i].in), INT64_MIN, INT64_MAX);
        int ok = rc == ints[i].wantRc && (rc != 0 || v == ints[i].want);
        failures += !ok;
        printf("%s int \"%s\" -> rc=%d %lld\n", ok ? "ok  " : "FAIL", ints[i].in, rc, (long long)v);
    }

    // Round trip: what we save must parse back to the same value
    static const s64 rt[] = { 110000001ll, -50000000ll, -150000000ll, 0, 99999999ll, -100000000ll };
    for (size_t i = 0; i < sizeof(rt) / sizeof(rt[0]); i++) {
        char buf[64];
        encodedFloatToString(buf, rt[i]);
        s64 back = 0;
        int ok = parseDecFloatOption(&back, buf, -10 * FLOAT_CONV_MULT, 10 * FLOAT_CONV_MULT) == 0 && back == rt[i];
        failures += !ok;
        printf("%s roundtrip %lld -> %s -> %lld\n", ok ? "ok  " : "FAIL", (long long)rt[i], buf, (long long)back);
    }

    printf("%d failures\n", failures);
    return failures != 0;
}
