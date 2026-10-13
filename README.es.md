# Counter-Strike 1.6 para PSP

[English](README.md) | **Español**

Port de Counter-Strike 1.6 a la PSP sobre el motor [Xash3D FWGS](https://github.com/FWGS/xash3d-fwgs),
partiendo del port de PSP de [Crow-bar](https://github.com/Crow-bar/xash3d-fwgs).
Se juega contra bots, hasta 5 contra 5, solo o con un amigo por ad-hoc, y anda en PSP y en PPSSPP.

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
| R | Disparar |
| L (toque corto) | Ataque secundario (zoom, silenciador, ráfaga, puñalada) |
| L + ✕ | Saltar |
| L + ○ | Agacharse mientras lo mantenés |
| L + □ / L + △ | Arma anterior / tirar arma |
| L + ↑ / L + R | Granadas / cuchillo |
| Cruceta ↓ | Agacharse (un toque y quedás agachado, otro y te parás) |
| Cruceta ← / → | Siguiente arma / usar y recargar |
| Cruceta ↑ | Cambiar de equipo |
| SELECT | Menú de compra (incluye "9. Autobuy", según `autobuy.txt`) |
| START | Menú |

En los menús de texto: cruceta ↑/↓ mueve el cursor, ✕ elige, ○ cierra.
Los botones se cambian en `cstrike/userconfig.cfg`. La velocidad de cámara, invertir la vista, la
velocidad y zona muerta del analógico, la ayuda de apuntado y la mira están en Options → Controls, y
el nombre del jugador en Options → Player name (teclado en pantalla: cruceta y ✕). La configuración se guarda al salir de cada página de opciones y con Options → Save settings.

## Multijugador ad-hoc

Dos PSP juegan la misma partida (con los bots de quien la crea):

1. El que crea: Counter-Strike, marcar **Ad-hoc**, elegir mapa y hasta 6 bots, Start Game.
2. La otra PSP: **Join Ad-hoc** en el menú principal. Busca la partida y se conecta.

En PPSSPP, para probarlo con dos instancias en la misma PC: Ajustes → Red, activar redes, dirección
del servidor ad-hoc `127.0.0.1`, relé de paquetes desactivado, una MAC distinta en cada instancia y
el servidor PRO ad-hoc integrado activado solo en una de las dos.

## Cambios principales respecto de los proyectos originales

- **Memoria**: heap fijo de 40 MB; un solo modelo por equipo (terror / gign) y `player.mdl` reemplazado
  por el modelo CT; texturas compartidas entre modelos y sin mipmaps en armas y sprites; texturas de
  armas, mapa, cielo y efectos a media resolución (HUD y menús completos); sonidos pasados a 8 bits /
  22 kHz y cargados al primer uso; malla de navegación de los bots leída por partes y cargada solo si
  hay bots; carga de modelos sin copias intermedias; menos historial de entidades, efectos
  temporales, partículas y haces; sin radar; los cadáveres desaparecen después de la animación de
  muerte; mallas de navegación cargadas sin los "encounter spots" (~1,5-2 MB por mapa). Dust2 5v5
  deja ~7 MB libres.
- **Memoria baja**: si en una partida queda poca memoria, se liberan los sonidos que no están sonando
  (se vuelven a cargar cuando hacen falta); si no alcanza, el juego vuelve al menú con un mensaje en
  vez de congelar la consola.
- **Render**: recorte por software de los triángulos del arma en primera persona; arreglo de las caras
  ocultas con `cl_righthand 1`; mira visible con `hud_scale` < 1; límite de 45 fps que duerme en vez
  de esperar en un bucle (ahorra batería); los sprites del mapa a más de 1500 unidades no se dibujan
  (el buffer de profundidad de 16 bits los dejaba ver a través de las paredes); sin la ventanita del
  modo espectador.
- **Bots**: con 8 bots o más, su lógica completa corre 8 veces por segundo en vez de 10.
- **Juego**: menús de texto con cursor para el gamepad (selección de equipo, compra), autobuy en el
  menú de compra, menú de partida con mapa, cantidad de bots y dificultad (Easy / Medium), ayuda de
  apuntado, página de controles para PSP, configuración que se guarda en el momento, textos de ayuda
  traducidos, recargas arregladas (el servidor se compila con `CLIENT_WEAPONS`), título del EBOOT
  "Counter-Strike".
- **Ad-hoc**: los sockets del motor funcionan sobre el ad-hoc de la PSP (PDP), con direcciones armadas
  a partir de la MAC (`engine/platform/psp/net_psp.c`); crear y unirse desde los menús.
- **Cambio de mapa**: desconectar termina el nivel como un cambio de mapa (la librería del juego está
  dentro del ejecutable y queda cargada) y libera los datos del nivel, así se puede jugar otro mapa
  sin reiniciar el juego.

## Problemas conocidos

- Mapas grandes pueden no entrar en memoria, sobre todo como segundo o tercer mapa de la sesión. Con
  muchos bots en un mapa pesado el juego puede volver al menú con el mensaje "Not enough memory":
  usá menos bots.
- El ad-hoc está probado en PPSSPP; se agradecen pruebas en PSP reales.
- Con muchos bots (4v4, 5v5) bajan los fps en los momentos con mucha acción.

## Créditos y licencias

- Xash3D engine: Unkle Mike y el equipo [FWGS](https://github.com/FWGS/xash3d-fwgs). Port a PSP: [Crow-bar](https://github.com/Crow-bar/xash3d-fwgs). Ver `README-xash3d.md`.
- Cliente: [cs16-client](https://github.com/Velaron/cs16-client) (a1batross, Velaron y colaboradores), ver su `LICENSE`.
- Servidor y bots: [ReGameDLL_CS](https://github.com/s1lentq/ReGameDLL_CS) (s1lentq y colaboradores), GPLv3, ver su `LICENSE`.
- Counter-Strike y Half-Life son marcas de Valve Corporation. Este proyecto no está afiliado a Valve.
