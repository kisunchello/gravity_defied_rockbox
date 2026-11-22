# Gravity Defied — Rockbox port (iPod Classic 6G)

A native [Rockbox](https://www.rockbox.org/) plugin port of the classic 2D
motorcycle‑trials game **Gravity Defied**, built for the **iPod Classic 6th gen
(`ipod6g`)**. Integer fixed‑point physics, 3D‑perspective tracks, leagues &
bikes (100–325cc), per‑track top‑3 leaderboards, a splash screen, an animated
attract demo behind the menu, custom `.mrg` level packs ("Mods"), and the
player's own music kept running underneath.

**Version 1.1**

> ⚠️ **Unofficial fan port.** This project is not affiliated with, endorsed by,
> or connected to Codebrew Software. "Gravity Defied" and all rights to the
> original game, its name and brand belong to their respective owners. This is a
> non‑commercial, hobby reimplementation for the Rockbox homebrew firmware. If
> you are a rights holder and want material removed, open an issue and it will be
> taken down.

---

## Status

Built and confirmed running on a real iPod Classic 6G (official Rockbox stable
build). Other Rockbox colour targets may compile but are untested — the UI and
controls are tuned for the iPod 6G clickwheel.

## Install (prebuilt — easiest)

You need a working [Rockbox](https://www.rockbox.org/) install on your iPod
Classic 6G first.

1. Download `Gravity Defied.rock` from the [Releases](../../releases) page
   (or `prebuilt/Gravity Defied.rock` in this repo).
2. Copy it to your iPod at `/.rockbox/rocks/games/`.
3. Copy the whole **`data/`** folder to `/.rockbox/rocks/games/gravity/`
   (so you end up with `/.rockbox/rocks/games/gravity/levels.mrg`, the fonts,
   the sprites, etc.).
4. On the iPod: **Files → .rockbox → rocks → games → Gravity Defied**, or find
   it under the Games plugin category.

Progress, settings and records are saved automatically inside the `gravity/`
data folder (`progress.dat`, `settings.dat`, `modprog.dat`) — nothing to set up.

## Controls

| Action | Button |
|---|---|
| Throttle / accelerate | SELECT (centre) |
| Brake / reverse | PLAY |
| Lean back / forward | LEFT / RIGHT, or the clickwheel |
| Menus: up / down | clickwheel, or LEFT=back / RIGHT=down |
| Select | centre / PLAY |
| Back / pause | MENU |

The clickwheel control scheme (lean+gas / lean only / off) is configurable in
**Options**.

## Custom level packs (Mods)

Drop any Gravity Defied `.mrg` level pack into
`/.rockbox/rocks/games/gravity/mods/` and it shows up under the **Mods** menu.
Mod tracks aren't scored on the main leaderboard, but per‑pack completion marks
are remembered.

## Build from source

See [`rockbox-integration/INSTALL.md`](rockbox-integration/INSTALL.md) for the
full toolchain + build steps. In short:

1. Get a Rockbox source checkout.
2. Copy `plugin/` into it as `apps/plugins/gravity/`.
3. Apply `rockbox-integration/register-plugin.patch` (adds the plugin to
   `CATEGORIES` and `SUBDIRS`).
4. Build the `ipod6g` target; the result is `apps/plugins/gravity/gravity.rock`.

## Credits & lineage

- Original game: **Codebrew Software** (all rights reserved to them).
- Based on the C++ reimplementation **[rgimad/gravity_defied_cpp](https://github.com/rgimad/gravity_defied_cpp)**.
- Physics / level format reference from the Android remake
  **[evgenyzinoviev/gravitydefied](https://github.com/evgenyzinoviev/gravitydefied)** (AGDTR).
- Rockbox plugin port, menus, leagues/leaderboards, 3D fix, controls and build
  glue: this repository.

See [`CREDITS.md`](CREDITS.md) for details.

## License

Rockbox plugins link the Rockbox plugin API and are distributed under the
**GNU General Public License v2** (see [`LICENSE`](LICENSE)). The ported game
code derives from GPL‑licensed upstream projects and is released here under the
same terms. This licence covers the **code** only — it does not grant any rights
to the original game's assets, level data, name or brand, which remain the
property of their respective owners.
