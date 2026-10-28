/*
*   This file is part of LuxiOS, a fork of Luma3DS.
*   Copyright (C) 2026 LuxiOS contributors
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// LuxiFlap: a small flappy game starring Luxi, opened with the Konami code (see menuThreadMain).
// Everything between the "logic" markers is plain C with no 3DS calls, so luxios-tools/tests can run it on a PC.

#include <3ds.h>
#include "menus/luxi_flap.h"
#include "luxi_flap_sprite.h"
#include "menu.h"
#include "draw.h"
#include "ifile.h"
#include "fmt.h"

// --- LuxiFlap logic begin ---

#define FLAP_W              320
#define FLAP_H              240
#define FLAP_HUD_H          16      // score band at the top
#define FLAP_GROUND_Y       222     // ground from this row down
#define FLAP_LUXI_X         64
#define FLAP_LUXI_SIZE      24
#define FLAP_HITBOX_INSET   4       // forgiving hitbox: the mane's spikes don't count
#define FLAP_PILLAR_W       28
#define FLAP_GAP_H          76
#define FLAP_PILLAR_SPACING 150
#define FLAP_NUM_PILLARS    3       // 3 * 150 > 320 + 28, so a wrapped pillar always re-enters off screen
#define FLAP_SPEED          3       // px per frame, at 30 frames per second

// Luxi's vertical position and speed are in 1/256 px
#define FLAP_GRAVITY        154     // 0.6 px/frame^2
#define FLAP_FLAP_VY        (-7 * 256)
#define FLAP_MAX_VY         (10 * 256)

#define KONAMI_TIMEOUT_MS   1500    // max pause between two presses of the code

typedef enum
{
    FLAP_READY = 0,
    FLAP_PLAYING,
    FLAP_OVER,
} FlapPhase;

typedef struct
{
    s32 x;      // left edge, px
    s32 gapY;   // top of the gap, px
    bool scored;
} FlapPillar;

typedef struct
{
    FlapPhase phase;
    s32 y;      // top of Luxi, 1/256 px
    s32 vy;     // 1/256 px per frame
    FlapPillar pillars[FLAP_NUM_PILLARS];
    u32 score;
    u32 scroll; // px scrolled so far, for the ground stripes
    u32 rng;
} FlapGame;

typedef struct
{
    u32 idx;        // how much of the code has been entered
    u32 lastMs;
    u32 prevHeld;
} KonamiState;

static u32 flapRandom(FlapGame *g)
{
    // xorshift32
    u32 x = g->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g->rng = x;
    return x;
}

static s32 flapNewGapY(FlapGame *g)
{
    const s32 lo = FLAP_HUD_H + 16, hi = FLAP_GROUND_Y - 16 - FLAP_GAP_H;
    return lo + (s32)(flapRandom(g) % (u32)(hi - lo + 1));
}

static void flapReset(FlapGame *g, u32 seed)
{
    g->phase = FLAP_READY;
    g->y = ((FLAP_HUD_H + FLAP_GROUND_Y - FLAP_LUXI_SIZE) / 2) * 256;
    g->vy = 0;
    g->score = 0;
    g->scroll = 0;
    g->rng = seed != 0 ? seed : 0x4C757869; // xorshift must not start at 0
    for (u32 i = 0; i < FLAP_NUM_PILLARS; i++)
    {
        g->pillars[i].x = FLAP_W + 60 + (s32)i * FLAP_PILLAR_SPACING;
        g->pillars[i].gapY = flapNewGapY(g);
        g->pillars[i].scored = false;
    }
}

static bool flapHits(const FlapGame *g)
{
    s32 top = g->y / 256 + FLAP_HITBOX_INSET;
    s32 bottom = g->y / 256 + FLAP_LUXI_SIZE - FLAP_HITBOX_INSET; // exclusive
    s32 left = FLAP_LUXI_X + FLAP_HITBOX_INSET;
    s32 right = FLAP_LUXI_X + FLAP_LUXI_SIZE - FLAP_HITBOX_INSET; // exclusive

    if (bottom > FLAP_GROUND_Y)
        return true;

    for (u32 i = 0; i < FLAP_NUM_PILLARS; i++)
    {
        const FlapPillar *p = &g->pillars[i];
        if (right <= p->x || left >= p->x + FLAP_PILLAR_W)
            continue;
        if (top < p->gapY || bottom > p->gapY + FLAP_GAP_H)
            return true;
    }

    return false;
}

// Advances one frame; flap is true when A was pressed since the last frame
static void flapStep(FlapGame *g, bool flap)
{
    if (g->phase == FLAP_READY)
    {
        if (!flap)
            return;
        g->phase = FLAP_PLAYING;
    }
    else if (g->phase != FLAP_PLAYING)
        return;

    if (flap)
        g->vy = FLAP_FLAP_VY;
    g->vy += FLAP_GRAVITY;
    if (g->vy > FLAP_MAX_VY)
        g->vy = FLAP_MAX_VY;
    g->y += g->vy;
    if (g->y < FLAP_HUD_H * 256) // the sky has a lid, like the original
    {
        g->y = FLAP_HUD_H * 256;
        g->vy = 0;
    }

    g->scroll += FLAP_SPEED;
    for (u32 i = 0; i < FLAP_NUM_PILLARS; i++)
    {
        FlapPillar *p = &g->pillars[i];
        p->x -= FLAP_SPEED;
        if (p->x + FLAP_PILLAR_W <= 0)
        {
            p->x += FLAP_NUM_PILLARS * FLAP_PILLAR_SPACING;
            p->gapY = flapNewGapY(g);
            p->scored = false;
        }
        if (!p->scored && p->x + FLAP_PILLAR_W <= FLAP_LUXI_X + FLAP_HITBOX_INSET)
        {
            p->scored = true;
            g->score++;
        }
    }

    if (flapHits(g))
        g->phase = FLAP_OVER;
}

// D-pad only: the circle pad drifts, and its noise shouldn't break or fake the code
static bool konamiFeed(KonamiState *k, u32 heldKeys, u32 nowMs)
{
    static const u32 sequence[] = { KEY_DUP, KEY_DUP, KEY_DDOWN, KEY_DDOWN, KEY_DLEFT, KEY_DRIGHT, KEY_DLEFT, KEY_DRIGHT, KEY_B };
    const u32 ignored = KEY_CPAD_UP | KEY_CPAD_DOWN | KEY_CPAD_LEFT | KEY_CPAD_RIGHT |
                        KEY_CSTICK_UP | KEY_CSTICK_DOWN | KEY_CSTICK_LEFT | KEY_CSTICK_RIGHT | KEY_TOUCH;

    u32 pressed = heldKeys & ~k->prevHeld & ~ignored;
    k->prevHeld = heldKeys;
    if (pressed == 0)
        return false;

    if (k->idx != 0 && nowMs - k->lastMs > KONAMI_TIMEOUT_MS)
        k->idx = 0;
    k->lastMs = nowMs;

    if (pressed == sequence[k->idx])
    {
        if (++k->idx == sizeof(sequence) / sizeof(sequence[0]))
        {
            k->idx = 0;
            return true;
        }
    }
    else if (pressed == KEY_DUP)
        k->idx = k->idx == 2 ? 2 : 1; // "Up Up Up" still ends in "Up Up"
    else
        k->idx = 0;

    return false;
}

// --- LuxiFlap logic end ---

_Static_assert(FLAP_LUXI_SIZE == LUXI_FLAP_SPRITE_SIZE, "regenerate luxi_flap_sprite.h at FLAP_LUXI_SIZE");

#define FLAP_RGB(r, g, b)   ((u16)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#define FLAP_TEAL           FLAP_RGB(0x2E, 0xC4, 0xB6)
#define FLAP_TEAL_DARK      FLAP_RGB(0x1E, 0x8C, 0x82)
#define FLAP_TEAL_LIGHT     FLAP_RGB(0x7F, 0xE8, 0xDE)
#define FLAP_GOLD           FLAP_RGB(0xF7, 0xC9, 0x48)
#define FLAP_GOLD_DARK      FLAP_RGB(0xD9, 0x9A, 0x1E)
#define FLAP_GOLD_DEEP      FLAP_RGB(0xB8, 0x7E, 0x12)

#define FLAP_SAVE_MAGIC     0x50464C58 // "XLFP"
#define FLAP_PANEL_X        96
#define FLAP_PANEL_Y        36

static KonamiState konamiState;
static u16 flapSky[FLAP_H];

bool LuxiFlap_FeedKonami(u32 heldKeys)
{
    return konamiFeed(&konamiState, heldKeys, (u32)(svcGetSystemTick() / (SYSCLOCK_ARM11 / 1000)));
}

static void flapInitSky(void)
{
    // Dusk gradient, lighter towards the ground so Luxi's dark mane stands out
    for (u32 y = 0; y < FLAP_H; y++)
    {
        u32 t = y * 256 / FLAP_H;
        u32 r = 0x2C + ((0x6A - 0x2C) * t >> 8);
        u32 g = 0x34 + ((0x7F - 0x34) * t >> 8);
        u32 b = 0x70 + ((0xC8 - 0x70) * t >> 8);
        flapSky[y] = FLAP_RGB(r, g, b);
    }
}

static inline u16 flapPillarColor(s32 dx, s32 y, s32 gapY)
{
    if (dx < 2 || dx >= FLAP_PILLAR_W - 2)
        return FLAP_TEAL_DARK;
    if ((y >= gapY - 4 && y < gapY) || (y >= gapY + FLAP_GAP_H && y < gapY + FLAP_GAP_H + 4))
        return FLAP_TEAL_LIGHT; // lip at the gap

    // A zig-zag gold bolt down the middle
    s32 phase = y & 15;
    s32 zig = phase < 8 ? phase - 4 : 12 - phase;
    s32 d = dx - (FLAP_PILLAR_W / 2 + zig);
    return (d >= -1 && d <= 1) ? FLAP_GOLD : FLAP_TEAL;
}

static inline u16 flapGroundColor(u32 x, u32 y, u32 scroll)
{
    if (y < FLAP_GROUND_Y + 2)
        return FLAP_GOLD;
    return (((x + scroll + y) / 6) & 1) ? FLAP_GOLD_DARK : FLAP_GOLD_DEEP;
}

// Draws the whole play field with each pixel written once, so nothing flickers
static void flapRender(const FlapGame *g)
{
    u16 *const fb = (u16 *)FB_BOTTOM_VRAM_ADDR;
    s32 luxiY = g->y / 256;

    for (s32 x = 0; x < FLAP_W; x++)
    {
        const FlapPillar *pillar = NULL;
        for (u32 i = 0; i < FLAP_NUM_PILLARS; i++)
        {
            if (x >= g->pillars[i].x && x < g->pillars[i].x + FLAP_PILLAR_W)
                pillar = &g->pillars[i];
        }

        bool luxiColumn = x >= FLAP_LUXI_X && x < FLAP_LUXI_X + FLAP_LUXI_SIZE;
        u32 sx = (u32)(x - FLAP_LUXI_X);
        u16 *column = fb + x * SCREEN_BOT_HEIGHT;

        for (s32 y = FLAP_HUD_H; y < FLAP_H; y++)
        {
            u16 c;
            s32 sy = y - luxiY;
            if (luxiColumn && sy >= 0 && sy < FLAP_LUXI_SIZE && ((luxiFlapMask[sy] >> sx) & 1))
                c = luxiFlapSprite[sy * FLAP_LUXI_SIZE + sx];
            else if (y >= FLAP_GROUND_Y)
                c = flapGroundColor((u32)x, (u32)y, g->scroll);
            else if (pillar != NULL && (y < pillar->gapY || y >= pillar->gapY + FLAP_GAP_H))
                c = flapPillarColor(x - pillar->x, y, pillar->gapY);
            else
                c = flapSky[y];
            column[SCREEN_BOT_HEIGHT - 1 - y] = c;
        }
    }
}

static void flapDrawHud(u32 score, u32 best)
{
    Draw_DrawFormattedString(8, 3, COLOR_TITLE, "LuxiFlap   Score %-5lu", score);
    Draw_DrawFormattedString(FLAP_W - 8 - 11 * SPACING_X, 3, COLOR_WHITE, "Best %-5lu", best);
}

static void flapDrawPanel(const char *title, const char *line1, const char *line2)
{
    u16 *const fb = (u16 *)FB_BOTTOM_VRAM_ADDR;
    const u32 w = 33 * SPACING_X + 12, h = 3 * SPACING_Y + 22; // 33 characters wide: ends at x = 306

    for (u32 x = FLAP_PANEL_X; x < FLAP_PANEL_X + w && x < FLAP_W; x++)
    {
        for (u32 y = FLAP_PANEL_Y; y < FLAP_PANEL_Y + h; y++)
        {
            bool border = x == FLAP_PANEL_X || x == FLAP_PANEL_X + w - 1 || y == FLAP_PANEL_Y || y == FLAP_PANEL_Y + h - 1;
            fb[x * SCREEN_BOT_HEIGHT + SCREEN_BOT_HEIGHT - 1 - y] = border ? FLAP_GOLD : COLOR_BLACK;
        }
    }

    u32 y = FLAP_PANEL_Y + 8;
    y = Draw_DrawString(FLAP_PANEL_X + 6, y, COLOR_TITLE, title) + SPACING_Y + 2;
    y = Draw_DrawString(FLAP_PANEL_X + 6, y, COLOR_WHITE, line1) + SPACING_Y;
    Draw_DrawString(FLAP_PANEL_X + 6, y, COLOR_WHITE, line2);
}

static void flapClearHud(void)
{
    u16 *const fb = (u16 *)FB_BOTTOM_VRAM_ADDR;
    for (u32 x = 0; x < FLAP_W; x++)
        for (u32 y = 0; y < FLAP_HUD_H; y++)
            fb[x * SCREEN_BOT_HEIGHT + SCREEN_BOT_HEIGHT - 1 - y] = COLOR_BLACK;
}

// Same folder the rest of LuxiOS uses: /luma on the SD card, or on CTRNAND if the config says so
static FS_ArchiveID flapArchive(void)
{
    s64 out;
    svcGetSystemInfo(&out, 0x10000, 0x203);
    return (bool)out ? ARCHIVE_SDMC : ARCHIVE_NAND_RW;
}

static u32 flapLoadBest(void)
{
    IFile file;
    u32 data[2] = { 0 };
    u64 total = 0;

    if (R_FAILED(IFile_Open(&file, flapArchive(), fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, "/luma/luxiflap.bin"), FS_OPEN_READ)))
        return 0;
    Result res = IFile_Read(&file, &total, data, sizeof(data));
    IFile_Close(&file);

    return (R_SUCCEEDED(res) && total == sizeof(data) && data[0] == FLAP_SAVE_MAGIC) ? data[1] : 0;
}

static void flapSaveBest(u32 best)
{
    IFile file;
    u32 data[2] = { FLAP_SAVE_MAGIC, best };
    u64 total = 0;

    if (R_FAILED(IFile_Open(&file, flapArchive(), fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, "/luma/luxiflap.bin"), FS_OPEN_CREATE | FS_OPEN_WRITE)))
        return;
    IFile_Write(&file, &total, data, sizeof(data), 0);
    IFile_Close(&file);
}

static u32 flapScanKeysDown(void)
{
    u32 keys = 0;

    Draw_Lock();
    if (isHidInitialized && !menuShouldExit)
    {
        hidScanInput();
        keys = hidKeysDown();
    }
    Draw_Unlock();

    return keys;
}

// Sleeps until the deadline, polling every few ms so quick taps between frames aren't missed
static u32 flapWaitFrame(u64 deadline)
{
    u32 down = 0;

    for (;;)
    {
        down |= flapScanKeysDown();
        s64 left = (s64)(deadline - svcGetSystemTick());
        if (left <= 0 || menuShouldExit)
            break;
        s64 ns = left * 1000000000LL / SYSCLOCK_ARM11;
        svcSleepThread(ns < 4 * 1000 * 1000LL ? ns : 4 * 1000 * 1000LL);
    }

    return down;
}

void LuxiFlap_Run(void)
{
    const u64 framePeriod = SYSCLOCK_ARM11 / 30;
    FlapGame game;
    u32 best = flapLoadBest();
    bool redraw = true;

    flapInitSky();
    flapReset(&game, (u32)svcGetSystemTick());

    Draw_Lock();
    flapClearHud();
    Draw_Unlock();

    u64 next = svcGetSystemTick();
    while (!menuShouldExit && !preTerminationRequested)
    {
        next += framePeriod;
        u32 down = flapWaitFrame(next);
        u64 now = svcGetSystemTick();
        if (now > next + framePeriod) // don't try to catch up after a stall (e.g. the console slept)
            next = now;

        if (down & (KEY_B | KEY_START))
            break;

        FlapPhase before = game.phase;
        u32 scoreBefore = game.score;

        if (game.phase == FLAP_OVER)
        {
            if (!(down & KEY_A))
                continue;
            flapReset(&game, (u32)now);
            redraw = true;
            before = game.phase;
        }
        else
            flapStep(&game, (down & KEY_A) != 0);

        bool newBest = false;
        if (game.phase == FLAP_OVER && before != FLAP_OVER && game.score > best)
        {
            best = game.score;
            newBest = true;
            flapSaveBest(best);
        }

        // READY and OVER are still frames, so their panels are drawn once on top instead of every frame
        if (game.phase == FLAP_PLAYING || redraw || game.phase != before)
        {
            Draw_Lock();
            flapRender(&game);
            if (redraw || game.score != scoreBefore || game.phase != before)
                flapDrawHud(game.score, best);
            if (game.phase == FLAP_READY)
                flapDrawPanel("LuxiFlap!", "A: flap. Fly through the gaps.", "B: back to the HOME Menu");
            else if (game.phase == FLAP_OVER)
            {
                char line[40];
                snprintf(line, sizeof(line), newBest ? "Score %lu. New best!" : "Score %lu. Best %lu", game.score, best);
                flapDrawPanel("Bonk!", line, "A: try again   B: quit");
            }
            Draw_FlushFramebuffer();
            Draw_Unlock();
            redraw = false;
        }
    }

    // Don't let the HOME Menu see the B/START that closed the game as a fresh press
    for (u32 i = 0; i < 50 && !menuShouldExit && scanHeldKeys() != 0; i++)
        svcSleepThread(10 * 1000 * 1000LL);
}
