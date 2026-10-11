# Counter-Strike 1.6 para PSP

[English](README.md) | **Español**

Port de Counter-Strike 1.6 a la PSP sobre el motor [Xash3D FWGS](https://github.com/FWGS/xash3d-fwgs),
partiendo del port de PSP de [Crow-bar](https://github.com/Crow-bar/xash3d-fwgs).
Se juega contra bots en servidor local, hasta 5 contra 5, y anda en PSP y en PPSSPP.

Este repositorio contiene **solo código y configuración**. No incluye ningún archivo del juego
(mapas, modelos, sonidos, texturas): hace falta una copia legal de Counter-Strike 1.6.

## Qué incluye

| Carpeta | Contenido |
|---|---|
| `engine/`, `ref_gu/`, `common/`, `public/`, `pm_shared/` | Motor Xash3D con el renderer de PSP (`ref_gu`) |
| `mainui/` | Menú principal, con la pantalla "Counter-Strike" (mapa + cantidad de bots) |
| `dllcs-cstrikeclient/cs16-client-main/` | Cliente de CS ([cs16-client](https://github.com/Velaron/cs16-client)) |
| `dllcs-cstrikeclient/cs16-client-main/3rdparty/ReGameDLL_CS/` | Servidor y bots ([ReGameDLL_CS](https://github.com/s1lentq/ReGameDLL_CS)) |
| `gamedata/cstrike/` | Configuración para PSP (`userconfig.cfg`, `listenserver.cfg`) y mallas de navegación de los bots (`maps/*.nav`) |

En la PSP no hay carga dinámica de librerías: el cliente y el servidor se enlazan dentro del
mismo ejecutable (prefijos de símbolos `cs16_` y `csmp_`, ver `scripts/waifulib/xshlib.py`).

## Compilar

Hace falta el [pspsdk](https://github.com/pspdev/pspdev) (GCC 9.3 o más nuevo) y Python 3. En Linux o WSL:

```sh
export PSPDEV=$HOME/pspdev
export PATH="$PATH:$PSPDEV/bin"
python3 ./waf configure -T fast --psp=prx,660,HW
python3 ./waf build
```

El resultado queda en `build/engine/EBOOT.PBP` y `build/engine/xash.prx`.

## Requisitos

- Una PSP con 64 MB de RAM (PSP-2000, 3000, Go o E1000). La PSP-1000 tiene 32 MB y no alcanza.
- Un custom firmware con **Force Extra Memory** activado (ARK-4 / PRO / ME: en el menú recovery o
  VSH, "Force Extra Memory" / "Use high memory layout"). **Sin eso el juego no arranca.**
  En PPSSPP no hace falta nada.
- Una copia legal de Counter-Strike 1.6.

## Instalar

1. Crear una carpeta en `PSP/GAME/` de la memory stick (por ejemplo `PSP/GAME/cstrike/`).
2. Copiar ahí `EBOOT.PBP` y `xash.prx`.
3. Copiar desde tu Counter-Strike 1.6 las carpetas `valve/` y `cstrike/`.
4. Copiar el contenido de `gamedata/cstrike/` de este repositorio encima de `cstrike/`.
5. Copiar `extras.pak` (el paquete de recursos de Xash3D FWGS: fuentes y botones del menú) en `valve/`.
   La carpeta `valve/` de Steam no lo trae y sin él el juego queda en pantalla negra.
   Viene incluido en el release.
6. Los bots necesitan `cstrike/BotProfile.db`. Viene con ReGameDLL_CS (`regamedll/extra/zBot/bot_profiles.zip` en su repositorio).

## Controles

| Botón | Acción |
|---|---|
| Analógico | Moverse |
| △ / ✕ / □ / ○ | Mirar arriba / abajo / izquierda / derecha |
| Cruceta ↑ / ↓ | Saltar / agacharse |
| Cruceta ← / → | Siguiente arma / usar y recargar |
| R / L | Disparar / ataque secundario |
| SELECT | Menú de compra (incluye "9. Autobuy", según `autobuy.txt`) |
| START | Menú |

En los menús de texto: cruceta ↑/↓ mueve el cursor, ✕ elige, ○ cierra.
Los botones se cambian en `cstrike/userconfig.cfg`. La velocidad de cámara, invertir la vista, la
velocidad y zona muerta del analógico, la ayuda de apuntado y la mira están en Options → Controls.
La configuración se guarda al salir de cada página de opciones y con Options → Save settings.

## Cambios principales respecto de los proyectos originales

- **Memoria**: heap fijo de 40 MB; un solo modelo por equipo (terror / gign) y `player.mdl` reemplazado
  por el modelo CT; texturas compartidas entre modelos y sin mipmaps en armas y sprites; texturas de
  armas, mapa, cielo y efectos a media resolución (HUD y menús completos); sonidos pasados a 8 bits /
  22 kHz y cargados al primer uso; malla de navegación de los bots leída por partes y cargada solo si
  hay bots; carga de modelos sin copias intermedias; menos historial de entidades, efectos
  temporales, partículas y haces; sin radar; los cadáveres desaparecen después de la animación de
  muerte. Dust2 5v5 deja ~7 MB libres.
- **Render**: recorte por software de los triángulos del arma en primera persona; arreglo de las caras
  ocultas con `cl_righthand 1`; mira visible con `hud_scale` < 1; límite de 45 fps que duerme en vez
  de esperar en un bucle (ahorra batería).
- **Juego**: menús de texto con cursor para el gamepad (selección de equipo, compra), autobuy en el
  menú de compra, menú de partida con mapa, cantidad de bots y dificultad (Easy / Medium), ayuda de
  apuntado, página de controles para PSP, configuración que se guarda en el momento, textos de ayuda
  traducidos, recargas arregladas (el servidor se compila con `CLIENT_WEAPONS`), título del EBOOT
  "Counter-Strike".

## Problemas conocidos

- Cambiar de mapa después de jugar uno puede quedarse sin memoria por fragmentación del heap.
  Reiniciar el juego antes de cambiar de mapa.
- Mapas grandes pueden no entrar en memoria.
- Con muchos bots (4v4, 5v5) bajan los fps en los momentos con mucha acción.

## Créditos y licencias

- Xash3D engine: Unkle Mike y el equipo [FWGS](https://github.com/FWGS/xash3d-fwgs). Port a PSP: [Crow-bar](https://github.com/Crow-bar/xash3d-fwgs). Ver `README-xash3d.md`.
- Cliente: [cs16-client](https://github.com/Velaron/cs16-client) (a1batross, Velaron y colaboradores), ver su `LICENSE`.
- Servidor y bots: [ReGameDLL_CS](https://github.com/s1lentq/ReGameDLL_CS) (s1lentq y colaboradores), GPLv3, ver su `LICENSE`.
- Counter-Strike y Half-Life son marcas de Valve Corporation. Este proyecto no está afiliado a Valve.
