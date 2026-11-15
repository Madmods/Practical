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

First-session setup: enable QB aim once. If prompted to rejoin, keep Vanta and Roblox open. After the game loads and the connection checks pass, press Continue: QB aim resumes automatically in the same Roblox process. A prepared join listener keeps its authenticated setup in memory until Continue, avoiding a second helper launch and scan. Setup still expires after two minutes; turning QB aim off or closing Vanta cancels it and any pending resume.

Setup consumes key-scan results in short slices instead of waiting for the whole scan, while preserving the scan's total time and byte limits. Previously validated schemas are reused for their authenticated process/connection. A new connection may still require a join schema; this is not a promise of zero rejoins or a fixed startup time. The HUD says Setting up until transport is ready, separately from whether a reachable throw has been found.

Enable QB aim in the Football Fusion 3 workspace. Equip a football, wait for a ready target, and click normally. Calculated power works without waiting for the wheel. Manual game-power mode requires a valid visible display; hidden copies left by an earlier ball are ignored.

- Q locks or unlocks the selected receiver. Z cycles the pass mode; the pass card has previous/next buttons. Modes are Auto route, Bullet, Lob, Mag, Slant and Touch. Mag is an elevated touch preset; it does not expand catch range.
- Auto route searches powers from 15 through 95 in five-point steps, both ballistic arcs through 85 degrees, and reachable catch heights. Estimated-clear paths always outrank contested paths; among clear paths, higher power, shorter flight and lower elevation are preferred. Searches run at 10 Hz, with the retained trajectory recomputed for current positions between searches. Time-based limits ease angle and offset changes when the intermediate solution retains clearance. R/F still selects manual elevation with calculated power.
- **Beam mode · adaptive power** is optional and off by default. It searches downward from 95, refines the first clear interval in one-point steps, and chooses the highest tested power with an estimated-clear path. Both ballistic roots are available through 85 degrees: a lower-power high arc can replace an interceptable low beam. The old 35-degree restriction and strongest-contested-throw fallback are removed. The helper preserves the selected arc and power and rechecks coverage from the actual event origin. Changing pass type or using R/F exits Beam. The force95 setting key remains for compatibility.
- R raises the angle and F lowers it, switching to manual angle with calculated power. Power updates continuously within the game's 1–95 range; unreachable angles are reported. Cycling the pass mode restores automatic angle and calculated power. Hotkeys require the game to be focused and do not activate while typing.
- With Calculate power off, the game's visible power and the advanced Direct/Lob setting determine the throw until a pass-mode or R/F action enables calculated power again.
- Smart fit checks other active players and practice bots along the entire flight using estimated running, jumping and diving reach. If no candidate exceeds the clearance buffer, assistance is withheld. The helper also refuses a rewrite if actual-origin revalidation becomes contested. Neither layer cancels manual clicks: a click without an assisted plan remains a normal game throw. Stationary receivers have no movement lead and at most four studs of horizontal placement; moving receivers can get up to eight with adjustment time. Jump catches reduce lateral reach and require reaction/ascent time. Dive reach is adjustable. These reach estimates cannot guarantee interception-free passes, particularly where receiver and defender catch regions overlap.
- **No clear window** means Smart fit found no sufficiently clear candidate or coverage is incomplete. It does not display a ready trajectory. Turn Smart fit off to use unguarded assistance, or wait for separation; normal manual clicks remain available. **Open estimate** is a prediction, not a guarantee.
- Preview and transmitted throws both account for the five-stud forward launch offset. The packet calculation keeps the original event origin and solves the trajectory from the resulting launch position.
- The QB panel shows a trajectory side profile, power/elevation/flight/clearance values and pass-mode buttons. The field overlay uses a single thin curve, a small catch marker and one compact receiver/status card. Tall reach poles, ground rings, the receiver-to-catch connector and duplicate catch labels are removed. Display-only curve easing is marked Updating arc until it settles; it never changes the transmitted aim. Nearby catcher marks are compact, and the coverage detail reports nearest distance. Cursor selection requires 160 ms on a new receiver while the previous candidate remains available; Q locks immediately. Receivers can be selected up to 1,000 studs away, subject to physical throw range.
- **Live ball trajectories** is independent of QB aim. It reads client-visible Football positions in Workspace and nearby descendants, with bounded scans and up to 64 tracked balls. Raw assembly velocity is not trusted: script-driven footballs may retain their launch velocity or be anchored while moving. Successive position samples estimate current flight velocity; static objects with stale launch velocity stay hidden. Solid cyan paths show observed motion; dashed paths forecast 0.6 seconds only after measured movement and when configured gravity is available. Catch/reparent/removal, observed impact, sustained rest/level rolling, or timeout clears the trail. Landing detection is an estimate, not a map raycast. Unstreamed balls cannot be displayed. Tracking does not start the packet helper.

## Validation and collaboration

`build.cmd` runs the native tests. `powershell -File scripts/check-webui.ps1` runs an isolated WebView test without live input hooks. With the Python environment active, run `python -m unittest discover -s build/network_decoder -p test_*.py` for the decoder tests; run the network helper build first so the test directory contains the scanner DLL.

Clone this repository to a fresh directory, create a branch for your changes, and use pull requests into `main`. Commit source and tests, not builds, local settings or captures. Third-party code retains its own license notices; the WinDivert header's license notice and the UI fonts' OFL are included.
