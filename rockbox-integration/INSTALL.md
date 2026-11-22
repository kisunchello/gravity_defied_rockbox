# Building the Gravity Defied plugin from source

The plugin builds **inside a Rockbox source tree** — it is not a standalone
program. These steps produce `gravity.rock` for the iPod Classic 6G (`ipod6g`).

## 1. Get Rockbox

```sh
git clone https://github.com/Rockbox/rockbox.git
cd rockbox
```

## 2. Drop the plugin in

Copy this repo's `plugin/` folder into the Rockbox tree as
`apps/plugins/gravity/`:

```sh
cp -r /path/to/gravity-defied-rockbox/plugin apps/plugins/gravity
```

## 3. Register the plugin

Apply the patch that adds `gravity` to the plugin `CATEGORIES` and `SUBDIRS`:

```sh
git apply /path/to/gravity-defied-rockbox/rockbox-integration/register-plugin.patch
```

(If `git apply` complains about an unrelated tree, the change is tiny — just add
a line `gravity,games` to `apps/plugins/CATEGORIES` and a line `gravity` under
`/* For all targets */` in `apps/plugins/SUBDIRS`.)

> The plugin's own `apps/plugins/gravity/gravity.make` adds a C++ build rule
> (Rockbox plugins are C by default). No other Rockbox file needs changing.

## 4. Cross‑compiler with C++ support

Rockbox needs its own GCC cross‑toolchain (not your system compiler). The stock
`tools/rockboxdev.sh` builds a **C‑only** compiler; this plugin has C++ sources,
so the toolchain must be built with `c,c++` enabled. Apply:

```sh
git apply /path/to/gravity-defied-rockbox/rockbox-integration/toolchain-cxx.patch
```

Then build the ARM toolchain (target letter `a` = arm):

```sh
./tools/rockboxdev.sh --target=a
# installs arm-elf-eabi-{gcc,g++} (9.5.0) — add its bin dir to PATH
```

On macOS you'll also need GNU `sed`/`coreutils` (`gsed`, `greadlink`) and should
configure the host build with `CC=clang CXX=clang++`.

## 5. Build

```sh
mkdir build-ipod && cd build-ipod
../tools/configure   # choose: iPod Classic/6G  ->  (N)ormal build
make -j$(nproc)      # -> apps/plugins/gravity/gravity.rock
```

`make zip` produces a full `rockbox.zip`, but you only need the single
`gravity.rock` (plus the `data/` files) to install over an existing Rockbox.

## 6. Install

Copy `apps/plugins/gravity/gravity.rock` to `/.rockbox/rocks/games/` on the
device, and the repo's `data/` folder to `/.rockbox/rocks/games/gravity/`.
(You can rename the `.rock` to anything, e.g. `Gravity Defied.rock` — the data
path is fixed and independent of the file name.)

## macOS note

Apple's `clang` can't build Rockbox; you need real GCC. A common setup is a
wrapper script around Homebrew GCC that injects
`-isysroot $(xcrun --show-sdk-path) -D_FORTIFY_SOURCE=0`. That host shim is
environment‑specific and intentionally **not** included in this repo.
