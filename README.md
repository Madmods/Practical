# PracticalComplete — source map

`dist/PracticalComplete.luau` is **generated**. Edit files in `src/`, then run:

```
powershell -ExecutionPolicy Bypass -File build.ps1
```

`build.ps1 -Verify <file>` also checks the build is byte-identical to another file (it was identical to the original `PracticalComplete.luau` when split).

## How it fits together (read this first)

- The files are slices of ONE script, joined in name order. They are **not** separate modules:
  `engine/01`–`09` and each `ui/*` group share one scope, so a local defined in one file can be used in a later file.
- `engine/10`–`23` are self-contained `do (function() ... end)() end` blocks. They read shared engine locals
  (`settings`, `api`, `keyboard`, `model`, `attr`, `rootOf`, `flat`, `featureCleanup`, ...) and register their own cleanup
  by wrapping `featureCleanup`. Each can be worked on alone.
- The UI (`ui/*`) is a single Lua chunk stored inside `loadstring([====[ ... ]====])`:
  `ui/00_open_loadstring` opens it, `ui/ZZ_close_loadstring` closes it. Never put `]====]` inside a ui file.
  The UI talks to the engine only through `getgenv().PracticalSuiteAPI` (built in `engine/09`), and
  `ui/G0_engine_bridge` is where UI controls get mapped onto API calls.
- Cleanup chain: every block does `local previous=featureCleanup; featureCleanup=function() ...; previous() end`.
  Blocks run in file order, so cleanup runs in reverse.

## Cheap way to use ChatGPT on this

Paste only: this README's "How it fits together" + the one file you are changing (+ the API/helper names it uses).
To change a UI control, the usual set is `ui/40_pages_data` (the control definition) and `ui/G0_engine_bridge`
(`booleanMap` / `numberMap` / `routeControl`) plus the engine file that implements it.
Tell it to return the whole changed file, then rebuild.

## engine/ (loaded first, in this order)

| File | Lines | What it does | Main shared names defined |
|---|---|---|---|
| 01_setup_bindings | 59 | env, services, key/mouse binding helpers, saved timing profiles | `env P Run Input bindingDown ping keyboard gui finite` |
| 02_debug_panel_widgets | 132 | old on-screen debug panel, the `settings` table, setting-row builder | `create panel row settings guardKey setting settingRows liveHud` |
| 03_autotime_peak_predictor | 217 | Auto Time: reads Bat meter, predicts peak, sends release; feedback/records | `on model save cleanup begin result featureCleanup` |
| 04_panel_features_presets | 85 | panel toggles, Balanced/Relaxed response presets, drag | `saveTuning featureToggle boostUntil` |
| 05_defense_shared_helpers | 121 | defense state vars + helpers | `attr holdsBall rootOf flat goalPosition defenseCharacters chooseHandler defensiveInputs recordDefense` |
| 06_guard_input_movement | 306 | guard key handling, movement hooks (`attachMovement`), `guardDirection`, dribble boost | `patch restorePatches manualTakeover guardDirection attachMovement` |
| 07_assists_landing_visuals | 337 | floor ribbons, landing prediction, live contest, auto steal/block/rebound loop | `visualFolder floorPoint ribbon updateLanding updateAssists` |
| 08_guard_state_solo_test | 177 | guard state file, local solo rehearsal dummy | `startSoloTest stopSoloTest` |
| 09_suite_api_profiles | 190 | **`PracticalSuiteAPI`**, profiles, match report, tools page, cleanup | `api numericLimits applyProfile exportMatchReport` |
| 09b_hud_dock | ~50 | **HUD dock**: stacks screen cards (shot readout, pass panel, court warning, solo box) beside the ball ("Follow ball") or in a corner; cards register with `settings._hudRegister(frame, order)` | `api.SetHudCorner` |
| 10_shot_assist | 144 | release deferral / minimum-meter hold, contest outline | |
| 11_court_tools | 230 | ball recovery, passing lanes, screen awareness, combo builder (`api.PlayCombo`) | |
| 12_flight_coverage | 57 | stabilized ball-flight tracking, opponent coverage outline | |
| 13_contest_ceiling | 103 | estimated contest ceiling, contest-based release | |
| 14_drive_cutoff_intercept | 190 | drive cutoff, screen routing, pass interception (R key) | |
| 15_auto_join | 48 | auto-join selected spot (+ removed dunk-stopper stub) | |
| 16_receiver_denial | 299 | off-ball guard / matchup selection | |
| 17_pass_targets | 284 | pass-target ranking and visuals, catch-and-shoot readout | |
| 18_anti_alley_oop | 402 | lob tracking, backboard jump, interception | |
| 19_visuals_v2 | 550 | visual style V2 (Theme, Geometry, effects, adapter) | |
| 20_saved_combos | 88 | named combos + hotkeys, saved to `PracticalSavedCombos-v1.json` | |
| 21_smart_pass | 233 | smart pass (learns overhead/chest/bounce) | |
| 22_catch_and_shoot | 198 | auto shoot off pass | |
| 23_smart_dribble | 747 | measured dribble profiles, positional planner, smart dribbler, ball protection | |
| 24_defender_alerts | ~110 | edge-of-screen badges for nearby opponents you can't see (display only) | `settings.defenderAlerts` |
| 25_rebound_early | ~30 | `_reboundEarlySpot`: rim spot to head for while a shot is in the air; called from `engine/07` (`updateAssists`) | `settings.reboundEarly` |

## ui/ (inside the `loadstring` string)

| File | Lines | What it does |
|---|---|---|
| 00_open_loadstring | 2 | `do (function()` + `loadstring([====[` |
| 10_foundation_theme_builders | 355 | header, lifecycle, config/colors, motion, theme registry, `UI.make`/`text` builders |
| 20_icons | 317 | stroke icon definitions + drawing |
| 30_effects | 185 | press/ripple/shimmer/pulse/groupFade/shadow |
| 40_pages_data | 363 | **`PAGES` — every control definition**, CTRL/SETTING maps, forward declarations |
| 50_window_root | 120 | ScreenGui, window frame, ambient glow |
| 60_popups_buttons | 383 | overlay/tooltip/dropdown, icon/ghost/primary buttons, logo |
| 70_titlebar_sidebar_tabs | 396 | title bar, search, sidebar nav, page header, tabs |
| 80_list_footer_toasts | 290 | scroll list, footer buttons, status line, toasts |
| 90_widgets | 667 | toggle, slider, segmented, keybind, swatches, checkbox, combo builder, library |
| A0_cards_sections | 411 | `buildPanel`, `buildSection`, `buildCard` |
| B0_profiles_configs | 404 | profile snapshot/restore, Configs page |
| C0_overview_pages_search | 453 | live stats, `renderPage`, search, `openPage` |
| D0_settings_actions | 258 | apply theme/motion/layout, `ACTIONS`, profile file save/load |
| E0_mini_window_drag | 354 | profile dropdown, save/revert/reset, mini bar, hidden notice, open/close, drag/resize |
| F0_keyboard | 62 | key handling, rebinding, Ctrl+K |
| G0_engine_bridge | 303 | **maps UI controls to `PracticalSuiteAPI` calls** (`booleanMap`, `numberMap`, `routeControl`) |
| H0_go | 12 | first render + open |
| ZZ_close_loadstring | 5 | `]====],"PracticalUI2")` + xpcall start |

## Known cleanup candidates (not applied, to keep the build identical to the original)

- `engine/09_suite_api_profiles.luau` ends the cleanup with `if customUICleanup then pcall(customUICleanup);customUICleanup=nil end` repeated 21 times; one is enough.
- `engine/20_saved_combos.luau` has a ~23-level nested `pcall` that parses a key name; it is a single lookup in effect.
