# Practical external (Vanta)

Windows x64 application with a C++ host and embedded WebView2 interface. Source is in `Apps/Basketball/Native`; retain the directory layout because it includes `Apps/Basketball/build-offsets.hpp`.

This repository contains source, UI assets, build scripts, diagnostics and regression tests. Generated programs, personal settings, browser/account data, captures, session credentials, archives and downloaded dependencies are excluded. The older project remains available in Git history.

## Double-click setup on Windows

Extract the entire GitHub ZIP, then use these files in its top folder:

1. **1 - Setup Vanta.bat** automatically downloads and installs missing prerequisites, then prepares the SDK and Python build packages.
2. **2 - Build Vanta.bat** also runs automatic setup, then builds/tests the app, builds the networking helper and installs both into `Apps/Basketball`. Close this copy of Vanta before rebuilding. Your settings are preserved.
3. **3 - Start Vanta.bat** opens the built app. Local-preview login: **1 / 1**. Join the game, complete the checks, click Continue and enable the features you want.

The batch files work when launched from another folder and leave the build window open so errors can be read. Setup reuses existing dependencies. Missing Visual Studio **2022 Build Tools** with C++/Windows SDK, **64-bit Python 3.12 build package**, **WebView2 Runtime**, and **WinDivert 2.2.2 x64 networking files** are downloaded from their official publishers. Compatible installed Python is reused, including registered custom/all-user installations. Otherwise setup extracts the official CPython NuGet build package into `Native/third_party/python/3.12.10`, verifies its pinned SHA-256 and Python publisher signatures, and checks SSL/venv/ensurepip support. This requires no Python MSI installation, launcher, PATH changes or administrator access. The SDK and Python packages are downloaded as needed too. Initial setup needs an internet connection and enough disk space for the C++ tools; installation can take several minutes.

Allow the normal Windows installer approval prompt if it appears. Setup verifies publisher signatures before running installers, pins the networking ZIP/driver SHA-256, requires a valid Windows driver signature and checks x64 images, and never restarts Windows automatically. If a restart is required, restart when ready and run the same BAT file again. Cancelled installations or blocked downloads stop setup with a readable error. Installer error 1603 is reported as a failure, separately from cancellation; the project-local Python package avoids the Python MSI 1603/2502/2503 failure path. No security settings are changed. Downloads and installed dependencies stay out of Git.

WinDivert signing sponsors differ between releases, so networking verification uses exact release fingerprints plus Windows signature validation. A matching fingerprint does not override a rejected signature or Windows Security detection. Signature failures now include the Windows status and publisher. If Windows Security blocks the official archive or driver, leave it blocked; setup does not restore quarantine, add exclusions, disable protection or download from a mirror. A reviewed upstream replacement is required before retrying a blocked artifact.

For a read-only prerequisite check, run `powershell -File Apps/Basketball/Native/scripts/build-easy.ps1 -CheckOnly`. You can also supply existing approved x64 `WinDivert.dll` and `WinDivert64.sys` in `Apps/Basketball/Native/third_party/windivert/bin`, or reuse a complete `Apps/Basketball/network_decoder/runtime` folder.

## Build with commands

Requirements: Visual Studio 2022 Build Tools with Desktop development with C++ and the Windows SDK, Microsoft Edge WebView2 Runtime, and Python 3.12 for the optional networking helper.

From `Apps/Basketball/Native`:

1. Run `powershell -File scripts/setup-dependencies.ps1`. This fetches the pinned WebView2 SDK and creates `.venv` with the pinned Python dependencies. Use `-PythonPath <python.exe>` if the Python launcher is unavailable.
2. Run `scripts/build-webui.cmd` to build `build/Vanta-Basketball.exe` and its release catalog. This does not replace the installed application.
3. Run `scripts/build.cmd` to build, run the native regression checks and install `Apps/Basketball/Vanta.exe` with its matching `vanta-release.json`.
4. Start `Apps/Basketball/Start Vanta.cmd`. The local-preview login is **1 / 1**; no online licensing/account service is configured in this app.

Optional networking helper:

```powershell
scripts/build-network-decoder-lab.cmd
.venv/Scripts/python.exe scripts/build-network-host.py
```

The helper can be built without driver binaries. To supply an approved WinDivert 2.2 x64 runtime, use `--windivert-dir <folder>` containing `WinDivert.dll` and `WinDivert64.sys`, or place those files in `third_party/windivert/bin` before building. The packaged helper loads them from its own directory and reports a clear error if either file is missing. The double-click setup downloads missing drivers from the official WinDivert release; driver binaries are not stored in this repository. Dependency licenses must accompany any distribution.

Copy the complete `build/network_decoder/runtime` folder to `Apps/Basketball/network_decoder/runtime`. Python is bundled in that runtime; end users do not need Python. Keep `Vanta.exe`, its release catalog and the full networking folder together.

## Current application

- Practical Basketball and Football Fusion 3 have separate workspaces and verified game/version admission. The current offset profile targets `version-2c6fd5e09aaf4cfa`.
- QBA provides Packet and Mouse / UI methods with a saved selector. Switching cancels pending work and turns QB aim off. With Packets selected, read-only networking prepares as soon as FF3 is detected, including with QB aim off, so it can observe the next join. Mouse mode uses the game power wheel.
- Automatic throws to confirmed open receivers running away use at least a 25-degree arc, with power and route interception solved together. Reach, field, coverage and actual-release checks still apply. Explicit manual settings retain their policy.
- Practice range has its own boundary policy. Ordinary matches retain field checks. The decoder accepts the verified UnionOperation CFrame acknowledgement format and maintains parser state across warm-up, fresh schemas and bounded recovery.
- Ball Mags uses a simple spherical display with fresh transform sampling. Receiving controls include Mags, Auto Catch and Auto Jump; defense includes Auto Rush. Collision edits have identity-bound restoration journals.
- The throw capsule keeps receiver context through aim, release, flight and result transitions. Released beam presentation survives brief observation gaps and clears on observed completion.
- FF3 uses Quarterback, Receiving, Defense, Movement, Kicking and Match tools in the sidebar. The Network page is removed; the QB transport helper remains a background service. Diagnostic transport keys remain in memory; captures and logs are not repository content.

Use the controls and information tooltips in the app. Features and offsets depend on the supported client build; unsupported clients remain blocked. Offline checks and local observations do not establish guaranteed catches or successful throws on every connection.

## Tests

From `Apps/Basketball/Native`:

```powershell
powershell -File scripts/test-prerequisites.ps1
scripts/test-ff3-qb.cmd
scripts/test-ff3-qb-native.cmd
scripts/test-ff3-ball-mags.cmd
scripts/test-ff3-play.cmd
scripts/test-ff3-mags-visual.cmd
powershell -File scripts/check-qb-hud.ps1
powershell -File scripts/check-webui.ps1 -Frosted
scripts/build-network-decoder-lab.cmd
scripts/test-network-decoder-lab.cmd
```

The WebView test modes use isolated storage and do not install live input hooks. Diagnostic/probe scripts can read or interact with a running game; review their purpose before running them. `diagnostics/trajectory` contains a separate synthetic planner experiment, not the default live planner.

## Shareable package

After installing a matching executable/catalog and building the helper with its approved runtime files:

```powershell
powershell -File scripts/package-friend.ps1 -OutputDirectory <destination>
```

This creates a ZIP with fresh settings, the complete runtime, setup instructions, dependency notices and a checksum manifest. Personal settings, logs and captures are excluded; every ZIP entry is verified against the staged package. Extract the entire folder before running it. WebView2 Runtime must be installed on the recipient's computer.

## Studio adapter

`Native/studio_lab` and `src/studio_connection.hpp` support a separate local Studio connection. Its generated connection token/configuration is private local data and is excluded from Git. See `studio_lab/README.txt`.

Third-party headers, fonts and dependencies retain their accompanying license notices. No project-wide license is added by this upload.

The build also produces `Native/build/Vanta-Basketball-UiTest.exe` for automated UI/HUD checks. This separate executable forces synthetic test mode at compile time, uses caller permissions and never enables real game assists. The production app embeds `requireAdministrator` with `uiAccess=false`; the build verifies both embedded manifests.

Packet Beam calculates its power and updates only the existing destination and power fields. Power is always a whole-number value from 1 to 95. This path uses no executor or Luau companion and does not move the cursor. Origin, argument count and both original live-match trailing numeric values are preserved byte for byte, even when their values change. Unrecognized argument layouts pass through unchanged. Mouse / UI retains its normal power-wheel input.

With Packets selected, setup now starts its read-only listener while Roblox is still loading, before player/world admission. It waits for owned game traffic before starting a key scan, so opening Vanta well before joining does not exhaust setup retries. Known unrelated experiences stop preparation. Startup diagnostics include timings measured from the first owned game packet; idle waiting is recorded separately. Gameplay admission and throw validation remain required before edits.

QB aim > Placement > Anti OOB limit controls the planned catch-point margin from 0 to 20 studs in half-stud steps (default 2). Zero allows no outside distance; increasing the limit only extends the outside allowance and never adds an inward inset. Saved negative limits from earlier builds load as zero. Preview, planning, and packet release checks use the same saved value. Receiver reach and legal receiver body positions still apply; practice range ignores field boundaries.

QB aim > Visuals > OOB boundary draws the measured full field perimeter in cyan and the selected outside allowance in violet, with a translucent strip and a compact distance legend. The corners use the same radial limit as the throw checks. This saved toggle starts enabled, works without a selected receiver or held football while QB aim is on, and stays hidden in practice, game menus, and unknown fields. It shares the existing camera-sampled overlay and does not alter throws.

## FF3 extended controls

The selected receiving preview includes standing/jump/dive reach, a takeoff location and an approach guide. Pocket escape is visual only. Twenty new modules are available for review, with two kicking modules explicitly deferred. Defaults, input bindings and ranges are defined in `Apps/Basketball/Native/src/ff3_assist_catalog.json`; the native build generates its embedded catalog and supplies the same definitions to the UI.

New feature masters start off on every launch. Normal inputs share ownership with existing Catch/Jump/Rush controls and yield to physical input. Temporary jump, contact-group and action-hitbox edits restore through an identity-bound journal. Missing hitboxes, unavailable collision filters, unknown action bindings and unconfirmed captain ownership are reported rather than replaced by guessed game objects. Physics and match behavior require live review; the current practice check is read-only. Kicker Aim and Auto Kick Walk remain disabled until the next requested kicking session.

Run `scripts\test-ff3-assists.cmd`, `scripts\test-ff3-play.cmd` and `scripts\check-webui.ps1` from Native for the relevant checks. `scripts\probe-ff3-features.cmd` records a read-only layout/capability snapshot, skipping chats and script contents. Private diagnostics remain under Native/build and do not belong in a public archive.
