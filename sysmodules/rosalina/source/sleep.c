/*
*   This file is part of Luma3DS
*   Modified for LuxiOS (2026). See the LuxiOS git history for the changes.
*   Copyright (C) 2016-2018 Aurora Wright, TuxSH
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
*
*   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
*       * Requiring preservation of specified reasonable legal notices or
*         author attributions in that material or in the Appropriate Legal
*         Notices displayed by works containing it.
*       * Prohibiting misrepresentation of the origin of that material,
*         or requiring that modified versions of such material be marked in
*         reasonable ways as different from the original version.
*/

#include <3ds.h>
#include "plugin/plgldr.h"
#include "plugin/plgloader.h"
#include "sleep.h"
#include "minisoc.h"
#include "utils.h"
#include "task_runner.h"

static bool         g_isSleeping = false;
static LightEvent   g_onWakeUpEvent;

void    IR__HandleSleep(bool isSleeping); ///< See in input_redirection.c

void    Sleep__Init(void)
{
    // No need anymore, handled by ServiceManager
    // srvSubscribe(0x214); ///< Sleep entry
    // srvSubscribe(0x213); ///< Sleep exit
    LightEvent_Init(&g_onWakeUpEvent, RESET_STICKY);
}

void    Sleep__HandleNotification(u32 notifId)
{
    if (notifId == 0x214) ///< Sleep entry
    {
        LightEvent_Clear(&g_onWakeUpEvent);
        g_isSleeping = true;
        // IR patch creates sleep issue, so we need to handle it
        // IR__HandleSleep(g_isSleeping);
        // Plugins do not receive 0x214 notifications, so we send it via our custom service
        if (PLG__GetConfigMemoryStatus() == PLG_CFG_RUNNING)
            PLG__NotifyEvent(PLG_SLEEP_ENTRY, false);
    }
    else if (notifId == 0x213) ///< Sleep exit
    {
        g_isSleeping = false;
        LightEvent_Signal(&g_onWakeUpEvent);
        // IR patch creates sleep issue, so we need to handle it
        // IR__HandleSleep(g_isSleeping);
        // Plugins actually receives 0x213 notifications, but since we send sleep entry, let's do sleep exit as well
        if (PLG__GetConfigMemoryStatus() == PLG_CFG_RUNNING)
            PLG__NotifyEvent(PLG_SLEEP_EXIT, false);
    }
}

bool sleepCutWifiEnabled = false;
bool sleepBlockLid = false;
bool sleepNowRequested = false;
bool sleepNowPending = false;

static bool g_wifiCutForSleep = false;

static inline bool isWirelessEnabled(void)
{
    return *(vu8 *)((0x10140000 | (1u << 31)) + 0x180) != 0; // same register as "Toggle Wireless"
}

// Lid closed (0x214, published by NS before it requests sleep): turn Wi-Fi off if asked to. One quick IPC, and on any
// failure Wi-Fi simply stays on. Not when Rosalina will deny the sleep anyway (network features, or lid blocked)
void    Sleep__OnShellClosed(void)
{
    if (!sleepCutWifiEnabled || miniSocEnabled || sleepBlockLid || g_wifiCutForSleep)
        return;
    if (!isWirelessEnabled() || !isServiceUsable("nwm::EXT"))
        return;
    if (R_FAILED(nwmExtInit())) // never nwmExtExit() after a failed init (libctru refcount)
        return;
    if (R_SUCCEEDED(NWMEXT_ControlWirelessEnabled(false)))
        g_wifiCutForSleep = true;
    nwmExtExit();
}

static void Sleep__RestoreWifiTask(void *argdata)
{
    (void)argdata;
    // NWM may still be resuming: retry for up to ~2 s. Runs on the task runner, never blocking notification handling
    for (u32 i = 0; i < 40; i++)
    {
        if (isServiceUsable("nwm::EXT") && R_SUCCEEDED(nwmExtInit()))
        {
            Result res = NWMEXT_ControlWirelessEnabled(true);
            nwmExtExit();
            if (R_SUCCEEDED(res) || isWirelessEnabled())
                return;
        }
        svcSleepThread(50 * 1000 * 1000LL);
    }
}

// Lid opened (0x213, also published when sleep was denied): give Wi-Fi back if we turned it off
void    Sleep__OnShellOpened(void)
{
    if (!g_wifiCutForSleep)
        return;
    g_wifiCutForSleep = false;
    TaskRunner_RunTask(Sleep__RestoreWifiTask, NULL, 0);
}

// "Sleep now" (upstream request #1905): ask NS to put the system to sleep like closing the lid does. The foreground
// app still gets its usual sleep query and may refuse, in which case nothing happens. Wake on HOME and lid events,
// the same MCU wake mask NS uses for lid sleep. Must be called with the menu closed (the game must not be frozen)
Result  Sleep__RequestNow(void)
{
    PtmWakeEvents wakeEvents = { .pdn_wake_events = 0, .mcu_interupt_mask = BIT(2) | BIT(5) | BIT(6) };

    sleepNowPending = true;
    Result res = APT_SleepSystem(&wakeEvents);
    if (R_FAILED(res))
        sleepNowPending = false;
    return res;
}

bool    Sleep__Status(void)
{
    if (g_isSleeping)
    {
        LightEvent_Wait(&g_onWakeUpEvent);
        return true;
    }
    return false;
}
