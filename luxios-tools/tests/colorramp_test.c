// Host test for rosalina redshift/colorramp.c: colorramp_get_white_point.
// run_colorramp_test.sh extracts the blackbody table and both functions. The table is placed inside a struct
// between NaN guard cells, so any read past either end of it shows up as a non-finite result (no ASan on this
// toolchain); -fsanitize=undefined also checks the array index in &blackbody_color[...].
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include "memory.h"
#include "table_n.h"   // #define TABLE_N <number of floats in blackbody_color>

static const struct {
    float pre[6];
    float table[TABLE_N];
    float post[6];
} guarded = {
    { NAN, NAN, NAN, NAN, NAN, NAN },
    {
#include "table.inc"
    },
    { NAN, NAN, NAN, NAN, NAN, NAN },
};
#define blackbody_color guarded.table

#include "extracted.inc"

#define NROWS (TABLE_N / 3)
static int failures = 0;

static const float *row(int r)
{
    if (r < 0 || r >= NROWS) { printf("FAIL test bug: row %d out of range\n", r); failures++; r = 0; }
    return &guarded.table[r * 3];
}

static int finite3(const float *c)
{
    return isfinite(c[0]) && isfinite(c[1]) && isfinite(c[2]);
}

static void checkTemp(int temp, const float *want, float tol)
{
    float out[3] = { -1, -1, -1 };
    colorramp_get_white_point(out, temp);
    int ok = finite3(out);
    for (int i = 0; i < 3; i++)
        ok = ok && fabsf(out[i] - want[i]) <= tol;
    failures += !ok;
    printf("%s %dK -> (%.8f, %.8f, %.8f) want (%.8f, %.8f, %.8f)\n", ok ? "ok  " : "FAIL", temp,
           out[0], out[1], out[2], want[0], want[1], want[2]);
}

static void mid(const float *a, const float *b, float *out)
{
    for (int i = 0; i < 3; i++) out[i] = 0.5f * a[i] + 0.5f * b[i];
}

int main(void)
{
    char what[96];
    snprintf(what, sizeof(what), "table has %d rows (1000K..25100K in 100K steps = 242)", NROWS);
    failures += !(TABLE_N % 3 == 0 && NROWS == 242);
    printf("%s %s\n", (TABLE_N % 3 == 0 && NROWS == 242) ? "ok  " : "FAIL", what);

    const float one[3] = { 1.0f, 1.0f, 1.0f };
    float m[3];

    checkTemp(1000, row(0), 0);
    mid(row(0), row(1), m);
    checkTemp(1050, m, 1e-6f);
    checkTemp(6500, row(55), 0);
    checkTemp(6500, one, 1e-6f);          // neutral white point
    checkTemp(25000, row(240), 1e-7f);
    mid(row(240), row(241), m);
    checkTemp(25050, m, 1e-6f);           // last interpolated step, reads row 241 (the final row)
    checkTemp(25099, row(241), 1e-4f);
    checkTemp(25100, row(NROWS - 1), 0);  // used to interpolate with the row past the end
    checkTemp(25150, row(NROWS - 1), 0);
    checkTemp(30000, row(NROWS - 1), 0);
    checkTemp(INT_MAX, row(NROWS - 1), 0);
    checkTemp(500, row(0), 0);            // clamped to 1000K
    checkTemp(0, row(0), 0);
    checkTemp(-1, row(0), 0);
    checkTemp(INT_MIN, row(0), 0);

    // Sweep: every temperature gives finite components in [0, 1]; above 25100 it is the last table entry
    int sweepBad = 0, firstBad = 0;
    for (int t = -100; t <= 40000; t++)
    {
        float out[3] = { -1, -1, -1 };
        colorramp_get_white_point(out, t);
        int ok = finite3(out);
        for (int i = 0; i < 3; i++) ok = ok && out[i] >= 0.0f && out[i] <= 1.0f;
        if (t >= 25100)
            for (int i = 0; i < 3; i++) ok = ok && out[i] == row(NROWS - 1)[i];
        if (!ok && !sweepBad++) firstBad = t;
    }
    failures += sweepBad != 0;
    printf("%s sweep -100..40000K: %d bad (first %d)\n", sweepBad ? "FAIL" : "ok  ", sweepBad, firstBad);

    printf("%d failures\n", failures);
    return failures != 0;
}
