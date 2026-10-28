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

#pragma once

#include <3ds/types.h>

// LuxiFlap, the LuxiOS easter egg: Up Up Down Down Left Right Left Right B (D-pad) opens it, A flaps.

// Feed the currently held keys every few ms; returns true once the code has just been completed
bool LuxiFlap_FeedKonami(u32 heldKeys);

// Runs the game on the bottom screen until the player quits. Call between menuEnter() and menuLeave()
void LuxiFlap_Run(void);
