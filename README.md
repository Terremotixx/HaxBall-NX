# HaxBall NX

A local arcade football game for Nintendo Switch. It includes football, futsal, arcade hockey, handball, and Four Goals. Supports up to eight local players, with CPU players filling any open spots. There is no online play.

## Build

Requires devkitPro with devkitA64, libnx, and SDL2 for Switch. From the devkitPro MSYS2 terminal, configure `DEVKITPRO` and `DEVKITA64`, then run:

```sh
make
```

The build output is `HaxBall-NX.nro`.

## Controls

Move with the stick. Press A to shoot. Holding A briefly lets you control and aim the ball while moving; release to shoot. Menus work with the stick or D-pad, and Joy-Con can be used horizontally.

Author: Terremotix.
