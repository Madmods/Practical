# Practical external

Windows x64 external application with a C++ host and WebView2 interface. The source lives in `Apps/Basketball/Native`; keep this directory layout because the engine includes `Apps/Basketball/build-offsets.hpp`.

This repository contains the external source, UI, resources, diagnostics, build scripts and tests. Generated applications, packet captures, personal settings, downloaded SDKs, and the old Lua project are excluded. Previous Git history is preserved.

## Build

Install Visual Studio 2022 Build Tools with the Desktop development with C++ workload and Windows SDK, the Microsoft Edge WebView2 Runtime, and Python 3.12 for the optional network helper.

From `Apps/Basketball/Native`:

1. Run `powershell -File scripts/setup-dependencies.ps1` to fetch the pinned WebView2 SDK and create a Python build environment.
2. Run `scripts/build.cmd` to build and test the native app. It installs `Vanta.exe` and its matching release catalog into `Apps/Basketball`.
3. For the optional network helper, run `scripts/build-network-decoder-lab.cmd`, then `.venv\Scripts\python.exe scripts/build-network-host.py`. Copy the contents of `build/network_decoder/runtime` to `Apps/Basketball/network_decoder/runtime`.
4. Start the app with `Apps/Basketball/Start Vanta.cmd`.

The optional packet integration requires a compatible WinDivert 2.2 x64 runtime supplied separately through your normal approved dependency setup. Point `VANTA_WINDIVERT_DLL` at its absolute DLL path, or place the runtime beside the helper executable. Driver binaries are not distributed or downloaded by this repository. Without them, the app builds, but packet functions remain unavailable. Do not disable security software to install dependencies.

The Roblox offsets and protocol schema are version dependent. The current offset profile targets `version-4ea8e85481e34f46`; unsupported versions must pass the existing compatibility checks before features are enabled.

## Quarterback controls

First-session setup: enable QB aim before rejoining if prompted. Keep Vanta and the Roblox process open. The bounded setup listener can continue across reconnection, refresh connection keys, and save the validated schema. After the game loads, press Continue and enable QB aim again. Turning QB aim off explicitly or closing Vanta stops setup. Setup expires after two minutes.

Enable QB aim in the Football Fusion 3 workspace. Equip a football, wait for a ready target, and click normally. Calculated power works without waiting for the wheel. Manual game-power mode requires a valid visible display; hidden copies left by an earlier ball are ignored.

- Q locks or unlocks the selected receiver. Z cycles the pass mode; the pass card has previous/next buttons. Modes are Auto route, Bullet, Lob, Mag, Slant and Touch. Mag is an elevated touch preset; it does not expand catch range.
- Auto route uses 95 power and the lower ballistic trajectory, with movement history for receiver lead and route labels. It does not add loft simply because distance increases. Other pass presets retain their elevations; R/F selects manual elevation with calculated power.
- R raises the angle and F lowers it, switching to manual angle with calculated power. Power updates continuously within the game's 1–95 range; unreachable angles are reported. Cycling the pass mode restores automatic angle and calculated power. Hotkeys require the game to be focused and do not activate while typing.
- With Calculate power off, the game's visible power and the advanced Direct/Lob setting determine the throw until a pass-mode or R/F action enables calculated power again.
- Smart fit compares nearby catch points for moving receivers against all other active players and practice bots, allowing estimated running, jumping and diving reach along the flight. Stationary receivers are targeted at their actual head with no fit displacement or retained movement lead. Dive reach is adjustable in Advanced. Green means an estimated opening, amber means contested, and incomplete means some catcher data could not be read. No other active catchers is reported separately. Predictions cannot guarantee a catch or prevent an interception.
- Preview and transmitted throws both account for the five-stud forward launch offset. The packet calculation keeps the original event origin and solves the trajectory from the resulting launch position.
- The trajectory, receiver markers and HUD can be toggled separately. Mouse selection follows the projected body with a minimum hit area and retention preference. Receivers can be selected up to 1,000 studs away; actual throw range still depends on the ballistic solution and power limit.

## Validation and collaboration

`build.cmd` runs the native tests. `powershell -File scripts/check-webui.ps1` runs an isolated WebView test without live input hooks. With the Python environment active, run `python -m unittest discover -s build/network_decoder -p test_*.py` for the decoder tests; run the network helper build first so the test directory contains the scanner DLL.

Clone this repository to a fresh directory, create a branch for your changes, and use pull requests into `main`. Commit source and tests, not builds, local settings or captures. Third-party code retains its own license notices; the WinDivert header's license notice and the UI fonts' OFL are included.

