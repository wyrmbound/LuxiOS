// Host test for Rosalina utils.c dateTimeToString (function body is extracted by run_date_test.sh).
// Expected strings were computed with Python's datetime.
#include <stdio.h>
#include <string.h>
#include "memory.h"

int lsprintf(char *buf, const char *fmt, ...);
#define sprintf lsprintf

#include "extracted.inc"

int main(void)
{
    static const struct { u64 ms; const char *want; } cases[] = {
        { 4002051723456ull, "2026-10-27_01-02-03.456" },
        { 4294967295000ull, "2036-02-07_06-28-15.000" },
        { 4294967296000ull, "2036-02-07_06-28-16.000" }, // 2^32 s since 1900: used to wrap to 1900
        { 4765132799999ull, "2050-12-31_23-59-59.999" },
        { 6311390400000ull, "2099-12-31_12-00-00.000" },
    };
    int failures = 0;
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char out[64];
        dateTimeToString(out, cases[i].ms, true);
        int ok = strcmp(out, cases[i].want) == 0;
        failures += !ok;
        printf("%s %s (want %s)\n", ok ? "ok  " : "FAIL", out, cases[i].want);
    }
    return failures != 0;
}
