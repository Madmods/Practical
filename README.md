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

Enable QB aim in the Football Fusion 3 workspace. Equip a football, wait for a visible power wheel and a ready target, and click normally.

- Auto route adapts elevation to distance and route direction. Other presets are Bullet, Lob, Mag, Slant and Touch. Mag is an elevated touch preset; it does not expand catch range.
- With Calculate power enabled, power is solved for the angle on each update, within the game's 1–95 range. Turn Automatic angle off to set a fixed elevation; R raises it and F lowers it while the game is focused and the ball is equipped.
- With Calculate power off, the game's power and the advanced Direct/Lob setting determine the throw. R/F retain their normal game behavior.
- Smart fit compares nearby catch points and estimated defender reach along the flight. Green is an estimated opening, amber is contested, and unscouted means no usable defender information. Predictions cannot guarantee a catch or prevent an interception.
- The trajectory, receiver markers and HUD can be toggled separately. Mouse selection follows the receiver's projected body, with a small retention preference to reduce target switching.

## Validation and collaboration

`build.cmd` runs the native tests. `powershell -File scripts/check-webui.ps1` runs an isolated WebView test without live input hooks. With the Python environment active, run `python -m unittest discover -s build/network_decoder -p test_*.py` for the decoder tests; run the network helper build first so the test directory contains the scanner DLL.

Clone this repository to a fresh directory, create a branch for your changes, and use pull requests into `main`. Commit source and tests, not builds, local settings or captures. Third-party code retains its own license notices; the WinDivert header's license notice and the UI fonts' OFL are included.

