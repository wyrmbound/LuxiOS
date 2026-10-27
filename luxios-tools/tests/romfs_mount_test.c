// Host test for the update-RomFS mount detection in sysmodules/loader/source/patcher.c (patchLayeredFs).
// run_romfs_mount_test.sh extracts, byte for byte, the updateRomFsMounts table, the nbCandidates line and the
// two "//Locate update RomFS" loops into locate.inc, plus memsearch from sysmodules/loader/source/memory.c.
#include <stdio.h>
#include <string.h>
#include "memory.h"

#include "memsearch.inc"

// Wraps the extracted loops; returns the chosen index and the mount name the patcher would use
static u32 locateUpdateRomFs(u8 *code, u32 size, const char **mountOut)
{
#include "locate.inc"
    *mountOut = updateRomFsMounts[updateRomFsIndex];
    return updateRomFsIndex;
}

enum { RO2 = 0, ROM2 = 1, REX = 2, PATCH = 3, EXT = 4, ROM = 5 };

static int failures = 0;
static u8 buf[256];

// Fills the whole backing buffer (including bytes past `size`) with a non-NUL filler
static void reset(void)
{
    memset(buf, 'x', sizeof(buf));
}

static void put(u32 off, const char *bytes, u32 len)
{
    memcpy(buf + off, bytes, len);
}

static void check(const char *what, u32 size, u32 want)
{
    const char *mount = NULL;
    u32 got = locateUpdateRomFs(buf, size, &mount);
    int ok = got == want;
    failures += !ok;
    printf("%s %s -> %u \"%s\" (want %u)\n", ok ? "ok  " : "FAIL", what, got, mount, want);
}

int main(void)
{
    reset();
    put(13, "\0rex:", 5);
    check("first pass: \"\\0rex:\"", 64, REX);

    reset();
    put(13, "\0rom2:/patch", 12);
    check("first pass: \"\\0rom2:\" followed by a path", 64, ROM2);

    reset();
    put(16, "rom2:\0", 6);
    check("second pass: aligned \"rom2:\\0\" at 16", 64, ROM2);

    reset();
    put(0, "ro2:\0", 5);
    check("second pass: aligned \"ro2:\\0\" at offset 0", 64, RO2);

    for (u32 off = 17; off <= 19; off++)
    {
        char what[64];
        reset();
        put(off, "rom2:\0", 6);
        snprintf(what, sizeof(what), "unaligned \"rom2:\\0\" at %u not matched", off);
        check(what, 64, ROM);
    }

    reset();
    put(5, "\0patch:", 7);
    put(32, "ro2:\0", 5);
    check("both styles: first pass (patch) beats second pass (ro2)", 64, PATCH);

    reset();
    put(3, "\0ext:", 5);
    put(40, "\0ro2:", 5);
    check("two first-pass names: lower candidate index wins", 64, RO2);

    reset();
    put(8, "patch:\0", 7);
    put(24, "rom2:\0", 6);
    check("two aligned second-pass names: lower candidate index wins", 64, ROM2);

    reset();
    put(5, "rex:\0", 5);
    put(24, "rex:\0", 5);
    check("second pass skips an unaligned hit and finds a later aligned one", 64, REX);

    reset();
    put(9, "ro2:\0", 5);
    put(20, "ext:\0", 5);
    check("unaligned earlier candidate, aligned later candidate", 64, EXT);

    reset();
    check("nothing found -> falls back to rom:", 64, ROM);

    reset();
    put(10, "\0rom:", 5);
    put(20, "rom:\0", 5);
    check("only \"rom:\" present -> rom: (fallback)", 64, ROM);

    reset();
    put(10, "\0ro2", 4);
    put(20, "rex\0", 4);
    check("names without ':' are not matched", 64, ROM);

    // End of buffer. Bytes past `size` are set so that an over-read would produce a (wrong) match.
    reset();
    put(27, "\0ext:", 5);
    check("first pass: \"\\0ext:\" ends exactly at the buffer end", 32, EXT);

    reset();
    put(27, "\0ext", 4);
    buf[31] = ':';
    check("first pass: ':' just past the end is not read", 31, ROM);

    reset();
    put(24, "ext:\0", 5);
    check("second pass: aligned \"ext:\\0\" ends exactly at the buffer end", 29, EXT);

    reset();
    put(28, "rex:", 4);
    buf[32] = '\0';
    check("second pass: \"rex:\" at the end, NUL just past the end is not read", 32, ROM);

    reset();
    put(21, "patch:\0", 7);
    put(32, "patch:", 6);
    buf[38] = '\0';
    check("second pass: unaligned hit then aligned one cut off by the end", 38, ROM);

    printf("%d failures\n", failures);
    return failures != 0;
}
