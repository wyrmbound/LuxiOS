# LuxiOS hardware test checklist

LuxiOS builds cleanly and its parsers pass host tests. It has been **tested on an Old 2DS** (boot, the menu, power
off/reboot and everyday use with FBI and Anemone). It hasn't been tested on New 3DS/2DS models, and most sections below
still need someone to go through them.
Test in this order: each section assumes the ones above it passed.

**Before you start:** keep your current `boot.firm` (upstream Luma3DS) on your PC. If anything goes wrong, put
the SD card in your PC and copy it back. Luma can't brick a console; boot9strap always loads `boot.firm` from SD.

## 1. Boot (most important)

- [ ] Console boots to the HOME Menu with the LuxiOS `boot.firm`.
- [ ] Hold SELECT while powering on: the config menu says "LuxiOS ... configuration"; change an option, save, reboot, and check it stuck.
- [ ] Your existing settings (`/luma/config.ini`) survived the first boot.
- [ ] Launch a DS game and a GBA game (Virtual Console), or open System Settings → Other Settings → anything that reboots: no crash or power-off.
- [ ] Optional: copy `luma/splash.bin` and `luma/splashbottom.bin` to `/luma/`, enable Splash in the config menu: the Luxi splash shows.

## 2. The menu (every menu open goes through new code)

Open the menu (L+Down+Select) in **several different games**, including one 3D game.

- [ ] Opening and closing the menu doesn't freeze or glitch the game (open it for 10+ seconds too).
- [ ] Hold Y in the menu: you see the game's bottom screen; let go: the menu is back.
- [ ] Credits: Luxi is drawn in the top-right, titles are gold.
- [ ] Take screenshot: works; with L held only `_top.bmp`, with R held only `_bot.bmp`; files are in `/luma/screenshots/<TitleID>/`.
- [ ] Take a screenshot with 3D on (top screen 800px/3D).
- [ ] Power off from the menu, then cancel the HOME Menu power dialog: the menu still opens normally afterwards.
- [ ] Reboot from the menu.

## 3. Sleep

- [ ] Close and open the lid in a game: wakes normally. Repeat a few times.
- [ ] Close the lid in a game, wait a minute, open: the game continues.
- [ ] System configuration → Sleep options → A (sleep now): sleeps; HOME or opening/closing the lid wakes it.
- [ ] Sleep options → Y (ignore lid) on: closing the lid doesn't sleep; turn it off again.
- [ ] Sleep options → X (Wi-Fi off while asleep) on: close the lid, open it: Wi-Fi comes back (check in HOME Menu or Toggle Wireless).

## 4. Features you use

- [ ] Cheats: a cheat named `[*Something]` turns on by itself when the game starts; a failing (red) cheat shows the error on the title line.
- [ ] Process list → X on a game: it closes and you're back at HOME Menu.
- [ ] Main menu shows "Mods: ..." for a game with a `/luma/titles/<TitleID>/romfs` folder; the mod works.
- [ ] `/luma/ntp_server.txt` with an IPv4 address: Miscellaneous → Update time and date via NTP works (and finishes within a few seconds if the server doesn't answer).
- [ ] Time zone in that screen: try e.g. -03:30 and check the clock.
- [ ] Plugins (.3gx) still load, and survive going to HOME Menu and back, and sleep.
- [ ] InputRedirection: start, use, stop, start again; disconnect the PC client while holding a button: local buttons still work.
- [ ] System configuration → Toggle power to card slot: shows "Card inserted" and status; a game card still works.
- [ ] LuxiFlap: on the HOME Menu press Up Up Down Down Left Right Left Right B on the D-pad. The game opens on the bottom screen; A flaps, B quits back to the HOME Menu (which carries on where it was). The best score is still there after a reboot. L+Down+Select still opens the menu as usual.

## 5. Niche (only if you have them)

- [ ] A game card with the NoCrypto flag (upstream #1827).
- [ ] Region emulation (`locale.txt`) with an app that checks the system version, e.g. NetPass (upstream #2141).
- [ ] Download Play.

If something fails, note what you did, the error screen (photo), and any file in `/luma/errdisp/` or `/luma/dumps/`.
