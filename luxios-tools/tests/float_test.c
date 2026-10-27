// Host test for arm9 config.c encodedFloatToString (function body is extracted by run_float_test.sh).
#include <stdio.h>
#include <string.h>
#include "memory.h"

#define FLOAT_CONV_MULT 100000000ll
#define FLOAT_CONV_PRECISION 8u
int lsprintf(char *buf, const char *fmt, ...);
#define sprintf lsprintf

#include "extracted.inc"

int main(void)
{
    static const struct { s64 val; const char *want; } cases[] = {
        { 100000000ll, "1" },
        { 110000001ll, "1.10000001" },   // was saved as "1.01"
        { 123456789ll, "1.23456789" },   // was saved as "1"
        { 125000000ll, "1.25" },
        { 10000000ll,  "0.1" },
        { -150000000ll, "-1.5" },
        { 0, "0" },
        { -50000000ll, "-0.5" },
    };
    int failures = 0;
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char out[64];
        encodedFloatToString(out, cases[i].val);
        int ok = strcmp(out, cases[i].want) == 0;
        failures += !ok;
        printf("%s %lld -> %s (want %s)\n", ok ? "ok  " : "FAIL", (long long)cases[i].val, out, cases[i].want);
    }
    return failures != 0;
}
