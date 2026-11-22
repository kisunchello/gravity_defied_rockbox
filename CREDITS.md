# Credits & lineage

## Original game
**Gravity Defied** by **Codebrew Software**. All rights to the original game,
its name, brand and artwork belong to them. This port is an unofficial,
non‑commercial fan project and is not affiliated with or endorsed by Codebrew.

## Upstream code this port is based on
- **[rgimad/gravity_defied_cpp](https://github.com/rgimad/gravity_defied_cpp)** —
  C++/SDL2 reimplementation of the J2ME original. The physics and game‑logic
  classes in this port (`GamePhysics`, `GameLevel`, `LevelLoader`, `MathF16`,
  fixed‑point math, `.mrg` loading) derive from it.
- **[evgenyzinoviev/gravitydefied](https://github.com/evgenyzinoviev/gravitydefied)**
  (AGDTR, "Android Gravity Defied: The Remake") — reference for the level data
  format, league/bike parameters (100 / 175 / 220 / 325cc) and behaviour.

## This repository adds
- Native **Rockbox plugin** integration via a thin C shim (`rbshim.c`) so the
  C++ game code stays free of `plugin.h`.
- A custom freestanding C++ runtime (`cppsupport.cpp`): static bump allocator,
  `operator new/delete`, no exceptions/RTTI/global‑ctors (Rockbox plugins have
  no `.init_array`).
- Main menu, splash screen, animated attract demo, league progression, per‑track
  top‑3 leaderboards with name entry, settings/progress persistence, a custom
  level‑pack ("Mods") browser, in‑game music control, and iPod‑6G clickwheel
  controls.
- A fix for a crash‑hang (divide‑by‑zero when two wheel nodes coincide in the
  continuous‑collision solver) and CPU/framerate tuning for the HDD‑based iPod.

## License
Code is under the **GNU GPL v2** (see `LICENSE`), matching Rockbox and the
GPL‑licensed upstream projects.
