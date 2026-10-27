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

#pragma once

void    Sleep__Init(void);
void    Sleep__HandleNotification(u32 notifId);
bool    Sleep__Status(void);

// LuxiOS sleep options. Session-only on purpose (reset on reboot), so a problem on some console can't persist
extern bool sleepCutWifiEnabled;    ///< Turn Wi-Fi off while the lid is closed (upstream request #1773)
extern bool sleepBlockLid;          ///< Ignore the lid/sleep switch: deny sleep requests (upstream request #2229)
extern bool sleepNowRequested;      ///< "Sleep now" chosen in the menu, done once the menu has closed
extern bool sleepNowPending;        ///< Our own sleep request is in progress (bypasses sleepBlockLid)

void    Sleep__OnShellClosed(void);
void    Sleep__OnShellOpened(void);
Result  Sleep__RequestNow(void);
