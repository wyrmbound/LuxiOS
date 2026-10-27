// Host test for Rosalina ntp.c ntpParseIpv4 (function body is extracted by run_ntp_ip_test.sh).
#include <stdio.h>
#include <string.h>
#include "memory.h"

#include "extracted.inc"

int main(void)
{
    static const struct { const char *in; bool ok; u32 want; } cases[] = {
        { "192.168.1.1", true, 0xC0A80101 },
        { "10.0.0.2\r\n", true, 0x0A000002 },
        { "  8.8.4.4  \n", true, 0x08080404 },
        { "\xEF\xBB\xBF" "1.2.3.4\n", true, 0x01020304 },   // Notepad UTF-8 BOM
        { "0.0.0.0", true, 0 },
        { "255.255.255.255", true, 0xFFFFFFFF },
        { "256.1.1.1", false, 0 },
        { "1.2.3", false, 0 },
        { "1.2.3.4.5", false, 0 },
        { "1..2.3", false, 0 },
        { ".1.2.3", false, 0 },
        { "1.2.3.4x", false, 0 },
        { "1.2.3.4 junk", false, 0 },
        { "1234.1.1.1", false, 0 },
        { "pool.ntp.org", false, 0 },
        { "", false, 0 },
        { "\n", false, 0 },
    };
    int failures = 0;
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        u32 ip = 0xDEADBEEF;
        bool ok = ntpParseIpv4(cases[i].in, (u32)strlen(cases[i].in), &ip);
        bool pass = ok == cases[i].ok && (!ok || ip == cases[i].want);
        failures += !pass;
        printf("%s \"%s\" -> %d %08x\n", pass ? "ok  " : "FAIL", cases[i].in, ok, ip);
    }
    printf("%d failures\n", failures);
    return failures != 0;
}
