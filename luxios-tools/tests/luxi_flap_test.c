// Host test for the LuxiFlap easter egg (sysmodules/rosalina/source/menus/luxi_flap.c): the Konami code detector,
// the game's physics/scoring, and its renderer (into a fake framebuffer). run_luxi_flap_test.sh extracts the code.
// With an output path argument, it also writes a preview frame of a game in progress as a PPM image.
#include <stdio.h>
#include <string.h>
#include "stubs.h"

#include "extracted.inc"

static int failures = 0;

static void check(int cond, const char *what)
{
    failures += !cond;
    printf("%s %s\n", cond ? "ok  " : "FAIL", what);
}

static const u32 code[] = { KEY_DUP, KEY_DUP, KEY_DDOWN, KEY_DDOWN, KEY_DLEFT, KEY_DRIGHT, KEY_DLEFT, KEY_DRIGHT, KEY_B };

// Presses and releases each key, 100 ms apart; "extraHeld" stays held throughout (e.g. a drifting circle pad)
static bool typeKeys(KonamiState *k, const u32 *keys, u32 n, u32 extraHeld, u32 *t)
{
    bool fired = false;
    for (u32 i = 0; i < n; i++)
    {
        fired |= konamiFeed(k, keys[i] | extraHeld, *t += 50);
        fired |= konamiFeed(k, extraHeld, *t += 50);
    }
    return fired;
}

static void testKonami(void)
{
    KonamiState k = { 0 };
    u32 t = 1000;
    check(typeKeys(&k, code, 9, 0, &t), "konami: the code fires");
    check(!typeKeys(&k, code, 8, 0, &t), "konami: 8 of 9 keys doesn't fire");
    check(!konamiFeed(&k, KEY_A, t += 50), "konami: A instead of B doesn't fire");
    konamiFeed(&k, 0, t += 50);

    memset(&k, 0, sizeof(k));
    const u32 upFirst[] = { KEY_DUP, KEY_DUP, KEY_DUP };
    typeKeys(&k, upFirst, 3, 0, &t);
    check(typeKeys(&k, code + 2, 7, 0, &t), "konami: Up Up Up Down... still fires");

    memset(&k, 0, sizeof(k));
    check(typeKeys(&k, code, 9, KEY_CPAD_LEFT | KEY_TOUCH, &t), "konami: circle pad drift / touch don't break it");

    memset(&k, 0, sizeof(k));
    const u32 withX[] = { KEY_DUP, KEY_DUP, KEY_DDOWN, KEY_X, KEY_DDOWN, KEY_DLEFT, KEY_DRIGHT, KEY_DLEFT, KEY_DRIGHT, KEY_B };
    check(!typeKeys(&k, withX, 10, 0, &t), "konami: another key in the middle resets it");

    memset(&k, 0, sizeof(k));
    typeKeys(&k, code, 4, 0, &t);
    t += 5000;
    check(!typeKeys(&k, code + 4, 5, 0, &t), "konami: a long pause resets it");

    memset(&k, 0, sizeof(k));
    bool fired = false;
    for (u32 i = 0; i < 9; i++) // rolling presses, each key still held when the next goes down
        fired |= konamiFeed(&k, code[i] | (i ? code[i - 1] : 0), t += 50);
    check(!fired, "konami: a held key isn't pressed twice (Up Up needs a release in between)");

    memset(&k, 0, sizeof(k));
    check(!konamiFeed(&k, KEY_DUP | KEY_DDOWN, t += 50), "konami: two keys at once don't count");
}

// Flaps when Luxi sinks below the middle of the next gap: a crude autopilot that should go on forever
static bool autopilot(const FlapGame *g)
{
    const FlapPillar *next = NULL;
    for (u32 i = 0; i < FLAP_NUM_PILLARS; i++)
    {
        const FlapPillar *p = &g->pillars[i];
        if (p->x + FLAP_PILLAR_W > FLAP_LUXI_X && (next == NULL || p->x < next->x))
            next = p;
    }
    s32 target = next ? next->gapY + FLAP_GAP_H / 2 + 10 : (FLAP_HUD_H + FLAP_GROUND_Y) / 2;
    return g->y / 256 + FLAP_LUXI_SIZE / 2 > target && g->vy >= 0;
}

static void testGame(void)
{
    FlapGame g;
    flapReset(&g, 12345);
    check(g.phase == FLAP_READY && g.score == 0, "game: starts ready");
    s32 y0 = g.y;
    for (int i = 0; i < 100; i++) flapStep(&g, false);
    check(g.phase == FLAP_READY && g.y == y0, "game: waits for A");

    flapStep(&g, true);
    check(g.phase == FLAP_PLAYING && g.vy < 0, "game: A starts it with a hop");

    int frames = 0;
    while (g.phase == FLAP_PLAYING && frames < 1000) { flapStep(&g, false); frames++; }
    check(g.phase == FLAP_OVER && g.score == 0 && frames < 60, "game: without flapping Luxi falls and it's over quickly");

    flapReset(&g, 1);
    flapStep(&g, true);
    for (int i = 0; i < 40; i++) flapStep(&g, true);
    check(g.y >= FLAP_HUD_H * 256, "game: flapping non-stop stops at the top, not in the score bar");

    bool gapsOk = true, spacingOk = true;
    for (u32 seed = 1; seed <= 20; seed++)
    {
        flapReset(&g, seed * 7919);
        flapStep(&g, true);
        frames = 0;
        while (g.phase == FLAP_PLAYING && frames < 20000)
        {
            flapStep(&g, autopilot(&g));
            frames++;
            for (u32 i = 0; i < FLAP_NUM_PILLARS; i++)
            {
                s32 gy = g.pillars[i].gapY;
                gapsOk &= gy >= FLAP_HUD_H && gy + FLAP_GAP_H <= FLAP_GROUND_Y;
                s32 d = g.pillars[(i + 1) % FLAP_NUM_PILLARS].x - g.pillars[i].x;
                spacingOk &= d == FLAP_PILLAR_SPACING || d == FLAP_PILLAR_SPACING - FLAP_NUM_PILLARS * FLAP_PILLAR_SPACING;
            }
        }
        if (seed == 1)
            printf("     autopilot (seed 1): %d frames, score %lu, %s\n", frames, (unsigned long)g.score, g.phase == FLAP_OVER ? "crashed" : "still flying");
        if (g.score < 100)
        {
            printf("     seed %lu: only %lu points\n", (unsigned long)seed, (unsigned long)g.score);
            check(0, "game: every random level can be flown through");
            break;
        }
    }
    check(gapsOk, "game: gaps stay between the score bar and the ground");
    check(spacingOk, "game: pillars stay evenly spaced as they wrap around");

    // Score counts once per pillar: pillars pass at FLAP_SPEED px/frame, FLAP_PILLAR_SPACING px apart
    flapReset(&g, 99);
    flapStep(&g, true);
    frames = 1;
    while (g.phase == FLAP_PLAYING && frames < 3000) { flapStep(&g, autopilot(&g)); frames++; }
    u32 expected = (u32)((frames * FLAP_SPEED - (FLAP_W + 60 + FLAP_PILLAR_W - FLAP_LUXI_X - FLAP_HITBOX_INSET)) / FLAP_PILLAR_SPACING + 1);
    check(g.score == expected || g.score + 1 == expected, "game: one point per pillar passed");
}

static void testRender(const char *ppmPath)
{
    FlapGame g;
    flapInitSky();
    flapReset(&g, 4242);
    flapStep(&g, true);
    for (int i = 0; i < 160 && g.phase == FLAP_PLAYING; i++) flapStep(&g, autopilot(&g));

    memset(fakeFb, 0xEE, sizeof(fakeFb));
    flapRender(&g);
    bool hudUntouched = true, fieldDrawn = true;
    for (u32 x = 0; x < FLAP_W; x++)
        for (u32 y = 0; y < FLAP_H; y++)
        {
            u16 c = fakeFb[x * SCREEN_BOT_HEIGHT + SCREEN_BOT_HEIGHT - 1 - y];
            if (y < FLAP_HUD_H) hudUntouched &= c == 0xEEEE;
            else fieldDrawn &= c != 0xEEEE;
        }
    check(fieldDrawn, "render: every play-field pixel is written");
    check(hudUntouched, "render: the score bar is left alone");

    if (ppmPath == NULL)
        return;
    FILE *f = fopen(ppmPath, "wb");
    fprintf(f, "P6\n%d %d\n255\n", FLAP_W, FLAP_H);
    for (u32 y = 0; y < FLAP_H; y++)
        for (u32 x = 0; x < FLAP_W; x++)
        {
            u16 c = y < FLAP_HUD_H ? 0 : fakeFb[x * SCREEN_BOT_HEIGHT + SCREEN_BOT_HEIGHT - 1 - y];
            u8 rgb[3] = { (u8)((c >> 11) << 3), (u8)(((c >> 5) & 63) << 2), (u8)((c & 31) << 3) };
            fwrite(rgb, 1, 3, f);
        }
    fclose(f);
    printf("     preview written to %s (score %lu)\n", ppmPath, (unsigned long)g.score);
}

int main(int argc, char **argv)
{
    testKonami();
    testGame();
    testRender(argc > 1 ? argv[1] : NULL);
    printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures != 0;
}
