# Counter-Strike 1.6 for PSP

**English** | [Español](README.es.md)

A port of Counter-Strike 1.6 to the PSP, built on the [Xash3D FWGS](https://github.com/FWGS/xash3d-fwgs)
engine and based on [Crow-bar's](https://github.com/Crow-bar/xash3d-fwgs) PSP port.
You play against up to 13 bots (7 vs 7), alone or with a friend over ad-hoc. It runs on real PSP hardware, on a PS Vita with Adrenaline and on PPSSPP.

This repository contains **code and configuration only**. It does not include any game files
(maps, models, sounds, textures): you need a legal copy of Counter-Strike 1.6.

## Contents

| Folder | Contents |
|---|---|
| `engine/`, `ref_gu/`, `common/`, `public/`, `pm_shared/` | Xash3D engine with the PSP renderer (`ref_gu`) |
| `mainui/` | Main menu, with the "Counter-Strike" screen (map + number of bots) |
| `dllcs-cstrikeclient/cs16-client-main/` | CS client ([cs16-client](https://github.com/Velaron/cs16-client)) |
| `dllcs-cstrikeclient/cs16-client-main/3rdparty/ReGameDLL_CS/` | Server and bots ([ReGameDLL_CS](https://github.com/s1lentq/ReGameDLL_CS)) |
| `gamedata/cstrike/` | PSP configuration (`userconfig.cfg`, `listenserver.cfg`) and bot navigation meshes (`maps/*.nav`) |

The PSP has no dynamic library loading, so the client and the server are linked into the same
executable (symbol prefixes `cs16_` and `csmp_`, see `scripts/waifulib/xshlib.py`).

## Building

You need the [pspsdk](https://github.com/pspdev/pspdev) (GCC 9.3 or newer) and Python 3. On Linux or WSL:

```sh
export PSPDEV=$HOME/pspdev
export PATH="$PATH:$PSPDEV/bin"
python3 ./waf configure -T fast --psp=prx,660,HW
python3 ./waf build
```

The output is `build/engine/EBOOT.PBP` and `build/engine/xash.prx`.

## Requirements

- A PSP with 64 MB of RAM (PSP-2000, 3000, Go or E1000). The PSP-1000 has 32 MB and can't run it.
- A custom firmware with **Force Extra Memory** enabled (ARK-4 / PRO / ME: in the recovery or VSH
  menu, "Force Extra Memory" / "Use high memory layout"). **Without it the game can't load a map**
  and shows a message asking for it. PPSSPP needs nothing special.
- Or a PS Vita with Adrenaline (game folder `ux0:pspemu/PSP/GAME/CSTRIKE/`).
- A legal copy of Counter-Strike 1.6.

## Installing

1. Create a folder under `PSP/GAME/` on the memory stick (for example `PSP/GAME/cstrike/`).
2. Copy `EBOOT.PBP` and `xash.prx` into it.
3. Copy the `valve/` and `cstrike/` folders from your Counter-Strike 1.6 installation.
4. Copy the contents of this repository's `gamedata/cstrike/` over `cstrike/`.
5. Copy `extras.pak` (the Xash3D FWGS resource pack: menu fonts and buttons) into `valve/`.
   A Steam `valve/` folder doesn't have it and the game stays on a black screen without it.
   It is included in the release.
6. The bots need `cstrike/BotProfile.db`. It ships with ReGameDLL_CS (`regamedll/extra/zBot/bot_profiles.zip` in its repository).

## Controls

| Button | Action |
|---|---|
| Analog stick | Move |
| △ / ✕ / □ / ○ | Look up / down / left / right |
| R | Fire |
| L (short press) | Secondary attack (zoom, silencer, burst, knife stab) |
| L + ✕ | Jump |
| L + ○ | Crouch while held |
| L + □ / L + △ | Last weapon used / drop weapon |
| L + ↑ / L + R | Grenades / knife |
| L + SELECT | Change team |
| D-pad ↓ | Crouch (press once to stay down, again to stand up) |
| D-pad ← / → | Next weapon / use and reload |
| D-pad ↑ | Scoreboard (while held) |
| SELECT | Buy menu (includes "9. Autobuy", from `autobuy.txt`) |
| START | Menu |

In text menus: D-pad ↑/↓ moves the cursor, ✕ selects, ○ closes.
The buttons are set in `cstrike/userconfig.cfg`. Look speed, invert look, analog stick speed and
deadzone, aim assist and crosshair are in Options → Controls, and the player name in
Options → Player name (on-screen keyboard: D-pad and ✕). Settings are saved when you leave a
settings page and with Options → Save settings.

## Frame rate

Options → Video has two modes, both locked to the screen refresh so the picture never tears:

- **30 FPS** (default): a steady 30, easier on the battery.
- **60 FPS**: unlocks up to 60. Meant for PPSSPP, a PSP on the charger or games with few bots;
  with many bots it drops to 30 in busy moments.

A real PSP (or PS Vita) is slower than PPSSPP, which runs the game as fast as the PC allows: the
same frame takes about 2 to 2.5 times longer on the console. Measured on a PS Vita with
Adrenaline, de_dust2:

| | Real hardware |
|---|---|
| No bots, 30 FPS mode | steady 30 |
| Many bots, 30 FPS mode | ~23-26, lower in big fights |
| 60 FPS mode | ~30-40 at best: it can't reach 60 |

On real hardware 30 FPS mode is the one to use, and fewer bots keep it steadier.

## Ad-hoc multiplayer

Two PSPs play the same game (the host's bots included):

1. Host: Counter-Strike, check **Ad-hoc**, pick the map and up to 6 bots, Start Game.
2. The other PSP: **Join Ad-hoc** in the main menu. It finds the game and connects.

On PPSSPP, to try it with two instances on one PC: Settings → Networking, enable networking, set the
ad-hoc server address to `127.0.0.1`, turn the packet relay off, give each instance a different MAC
address, and enable the built-in PRO ad-hoc server on one of them only.

## Main changes from the original projects

- **Memory**: heap of up to 40 MB, sized to the memory the console has (a fixed 40 MB didn't fit on
  real PSPs); one model per team (terror / gign), with `player.mdl` replaced by the
  CT model; textures shared between models and no mipmaps on weapons and sprites; weapon, world, sky
  and effect textures at half resolution (HUD and menus at full); sounds converted to 8 bit / 22 kHz
  and loaded on first use; bot navigation mesh streamed from disk and only loaded when there are
  bots; model loading without intermediate copies; smaller entity history, temp entity, particle and
  beam pools; no radar; corpses removed after the death animation; bot navigation meshes loaded
  without the "encounter spots" (~1.5-2 MB per map); the second player model borrows the
  animations it has byte for byte in common with the first (~1.9 MB); network buffers only for
  real players, not for bots (~128 KB per bot); map polygons and collision bevels in one block
  each instead of ~5000 small ones; player and weapon models kept between maps; ~1.8 MB less
  static memory (display list allocated at start, smaller sound channels, loopback buffers and
  tables); model and map files read into the PSP's volatile memory while loading. Dust2 with
  many bots peaks at ~27 MB of the 40 MB heap.
- **Low memory**: when the heap runs low in a game, sounds that aren't playing are freed (they
  load again when needed); if that isn't enough the game goes back to the menu with a message
  instead of freezing the console.
- **Rendering**: software clipping of first-person weapon triangles; fixed back-face culling with
  `cl_righthand 1`; crosshair visible with `hud_scale` < 1; 30 / 60 fps modes locked to the
  screen refresh; map sprites farther than 1500 units aren't drawn (the 16-bit depth buffer let
  them show through walls); no spectator inset window; compact scoreboard that fits the screen.
- **Speed**: player animation math on the VFPU instead of software double precision (the PSP
  FPU has no doubles), players outside the view skipped before animating them, display list
  guarded against overflowing with many players on screen.
- **Bots**: up to 13 (7 vs 7; 15 ran at ~23 fps on real hardware); their full think runs 10 times
  a second, 8 with 8 or more bots and 6 with 10 or more,
  spread over the frames so they don't all think at once; reaction times follow the actual think
  rate.
- **Gameplay**: text menus with a gamepad cursor (team selection, buying), autobuy in the buy menu,
  game menu with map, number of bots and bot difficulty (Easy / Medium), client-side aim assist,
  PSP controls page, settings saved on the spot, localized hint texts, fixed magazine reloads
  (the server is built with `CLIENT_WEAPONS`), EBOOT titled "Counter-Strike".
- **Ad-hoc**: the engine's sockets run over PSP ad-hoc (PDP) with addresses made up from the MAC
  (`engine/platform/psp/net_psp.c`); host and join from the menus.
- **Map changes**: disconnecting ends the level like a map change (the game library is linked into
  the executable and stays loaded) and frees the level data, so another map can be started without
  restarting the game.

## Known issues

- Large maps may not fit in memory, especially as the second or third map of a session. With
  many bots on a heavy map the game may go back to the menu with a "Not enough memory" message:
  use fewer bots.
- Ad-hoc has been tested on PPSSPP; tests on real PSPs are welcome.
- With many bots the frame rate drops in busy moments, more so on real hardware (~23-26 fps with
  many bots) and in 60 FPS mode, which can't reach 60 on a real console.
- Sleep mode during a game hasn't been tested on a real console yet.
- The first time a weapon sound plays it is read from the memory stick, which can cause a short
  hitch.

## Credits and licenses

- Xash3D engine: Unkle Mike and the [FWGS](https://github.com/FWGS/xash3d-fwgs) team. PSP port: [Crow-bar](https://github.com/Crow-bar/xash3d-fwgs). See `README-xash3d.md`.
- Client: [cs16-client](https://github.com/Velaron/cs16-client) (a1batross, Velaron and contributors), see its `LICENSE`.
- Server and bots: [ReGameDLL_CS](https://github.com/s1lentq/ReGameDLL_CS) (s1lentq and contributors), GPLv3, see its `LICENSE`.
- Counter-Strike and Half-Life are trademarks of Valve Corporation. This project is not affiliated with Valve.
