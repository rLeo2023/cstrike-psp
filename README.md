# Counter-Strike 1.6 for PSP

**English** | [Español](README.es.md)

A port of Counter-Strike 1.6 to the PSP, built on the [Xash3D FWGS](https://github.com/FWGS/xash3d-fwgs)
engine and based on [Crow-bar's](https://github.com/Crow-bar/xash3d-fwgs) PSP port.
You play against bots on a local server, up to 5 vs 5. It runs on real PSP hardware and on PPSSPP.

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
  menu, "Force Extra Memory" / "Use high memory layout"). **Without it the game doesn't start.**
  PPSSPP needs nothing special.
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
| D-pad ↑ / ↓ | Jump / crouch |
| D-pad ← / → | Next weapon / use and reload |
| R / L | Fire / secondary attack |
| SELECT | Buy menu (includes "9. Autobuy", from `autobuy.txt`) |
| START | Menu |

In text menus: D-pad ↑/↓ moves the cursor, ✕ selects, ○ closes.
The buttons are set in `cstrike/userconfig.cfg`. Look speed, invert look, analog stick speed and
deadzone, aim assist and crosshair are in Options → Controls. Settings are saved when you leave a
settings page and with Options → Save settings.

## Main changes from the original projects

- **Memory**: fixed 40 MB heap; one model per team (terror / gign), with `player.mdl` replaced by the
  CT model; textures shared between models and no mipmaps on weapons and sprites; weapon, world, sky
  and effect textures at half resolution (HUD and menus at full); sounds converted to 8 bit / 22 kHz
  and loaded on first use; bot navigation mesh streamed from disk and only loaded when there are
  bots; model loading without intermediate copies; smaller entity history, temp entity, particle and
  beam pools; no radar; corpses removed after the death animation. Dust2 5v5 keeps ~7 MB free.
- **Rendering**: software clipping of first-person weapon triangles; fixed back-face culling with
  `cl_righthand 1`; crosshair visible with `hud_scale` < 1; 45 fps cap that sleeps instead of
  busy-waiting (saves battery).
- **Gameplay**: text menus with a gamepad cursor (team selection, buying), autobuy in the buy menu,
  game menu with map, number of bots and bot difficulty (Easy / Medium), client-side aim assist,
  PSP controls page, settings saved on the spot, localized hint texts, fixed magazine reloads
  (the server is built with `CLIENT_WEAPONS`), EBOOT titled "Counter-Strike".

## Known issues

- Changing maps after playing one can run out of memory because of heap fragmentation.
  Restart the game before switching maps.
- Large maps may not fit in memory.
- With many bots (4v4, 5v5) the frame rate drops in busy moments.

## Credits and licenses

- Xash3D engine: Unkle Mike and the [FWGS](https://github.com/FWGS/xash3d-fwgs) team. PSP port: [Crow-bar](https://github.com/Crow-bar/xash3d-fwgs). See `README-xash3d.md`.
- Client: [cs16-client](https://github.com/Velaron/cs16-client) (a1batross, Velaron and contributors), see its `LICENSE`.
- Server and bots: [ReGameDLL_CS](https://github.com/s1lentq/ReGameDLL_CS) (s1lentq and contributors), GPLv3, see its `LICENSE`.
- Counter-Strike and Half-Life are trademarks of Valve Corporation. This project is not affiliated with Valve.
