# Tekken Revolution Reborn Qt Trainer (Standalone)

![TRR_256.png](assets/images/TRR_256.png)

Standalone Qt trainer for Tekken Revolution on RPCS3, based on existing CT offsets. 

Important! This is not a fully playable game but a POC that potentially could lead to an offline playable game but that requires a lot of reverse engineering and additional development. This is just a front-end to make it relatively easy to explore the possibilities. The cheats seem to work on both the EU and US versions of the game: `NPEB01406` and `NPUB31250`.                

Note: `build-msvc` is the canonical build/deploy folder for this setup.             

## Features
- Auto-attach to `rpcs3.exe`
- Attach fallback to `rpcs3-avx2.exe` when needed
- Configurable RPCS3 paths for `0.0.13`, `Latest`, and `Custom` executables
- Configurable game boot targets for `NPEB01406`, `NPUB31250`, and `NPJB00404`
- One-click `Start Game` and `Restart Game` flow (`kill all existing RPCS3 processes -> 500ms wait -> relaunch same target`, launched with `--no-gui`)
- `Start RPCS3` also pre-cleans existing RPCS3 processes before launching
- `Start RPCS3` picker supports `0.0.13`, `Latest`, and `Custom`
- `Start Game` picker supports `NPEB01406`, `NPUB31250`, and `NPJB00404`
- `Reset RPCS3` performs a soft reset by restarting the emulator process and clearing trainer attach state
- `Terminate RPCS3` performs a hard terminate of active RPCS3 processes and clears trainer attach state
- Built-in file logging with a `Show Logs` button for launch/debug diagnostics
- Extra utility buttons: `Snapshot`, `TR Manual`, `E3 2013`, and `Build/Tutorial` (in-app tutorial window with README tabs, text search, section index, and optional `README.pdf` launch)
- In `Build/Tutorial`, keyboard shortcuts are available: `Ctrl+F` focuses search, `F3` finds next, and `Shift+F3` finds previous.
- Resolve dynamic battle pointer from `0x3200D26BC`
- Character and stage selection
- Save/load presets to JSON
- Live monitor for key in-memory values (IDs, stage, timer, counters, state)
- Round-transition guard mode (temporarily pauses writes around KO/round change)
- Lua-compatible write/verify flow with retries on apply
- One-click stability profiles: Conservative, Balanced, Aggressive
- Optional force settings:
  - Game mode (`+0x178`, editable)
  - HP/UI value (`+0x2AC`, editable decimal/hex)
  - Infinite round (`+0x2B4`, editable)
  - Round timer (`+0x2A0`) with seconds presets (`Infinite`, `30`, `60`, `90`, `Custom`)
  - Round counters (`+0x290`, `+0x29C`)
  - Player control states (`base+0x12DA338`, `base+0x12DC7D8`)
- Mode reset pulse option (`4 -> target`) for in-fight mode switching
- Continuous lock mode (every 250ms) to keep values stable across rounds
- Optional post-apply stabilizer loop (~10 seconds) separate from continuous lock
- Auto-disable stage lock after match start to reduce transition instability
- Read Live Values button to pull mode/HP/infinite/timer values back into input fields
- Dedicated `Runtime` dialog for lock/guard/state controls
- Dedicated `Value Writes` dialog for mode/HP/infinite/timer controls
- `Advanced Memory` dialog for advanced reads/writes (positions, animation speed, global stage, game state values)

## Build (Windows)
Prerequisites:

- CMake 3.16+
- Visual Studio 2022 Build Tools
- Qt 6.11.1 MSVC 2022 64-bit

From this folder (trr-qt-trainer):

```powershell
powershell -ExecutionPolicy Bypass -File ".\build_and_deploy_msvc.ps1"
```

Executable:

- `build-msvc/trr_qt_trainer.exe`

The executable is built as a Windows GUI app (no console window).

Run directly (without deployment):

```powershell
& ".\build-msvc\trr_qt_trainer.exe"
```

For a portable folder, deploy the app from the build output with `windeployqt`.

## Deploy (Windows)

From this folder:

```powershell
powershell -ExecutionPolicy Bypass -File ".\deploy_msvc.ps1"
```

Manual equivalent:

```powershell
& "C:/Qt/6.11.1/msvc2022_64/bin/windeployqt.exe" --release --compiler-runtime "build-msvc/trr_qt_trainer.exe"
```

If `windeployqt` reports missing DLLs, use the full exe path above and run it from a VS 2022 developer shell, or install the Visual C++ 2015-2022 x64 redistributable. The two most common causes are:

- Deploying the wrong executable path, such as a stale MinGW build.
- Not having the MSVC runtime available for `--compiler-runtime` to copy.

## One-click build and deploy

```powershell
powershell -ExecutionPolicy Bypass -File ".\build_and_deploy_msvc.ps1"
```

The script bootstraps the MSVC environment through `VsDevCmd.bat`, so it does not depend on CMake detecting a registered Visual Studio instance.

## Run deployed build

```powershell
powershell -ExecutionPolicy Bypass -File ".\run_msvc.ps1"
```

## Usage
1. Open `RPCS3 Config` and set at least one emulator path (`RPCS3 0.013`, `RPCS3 LATEST`, or `RPCS3 CUSTOM`).
2. In `RPCS3 Config`, set one or more game targets (`NPEB01406 EBOOT.BIN`, `NPUB31250 EBOOT.BIN`, `NPJB00404 EBOOT.BIN`).
3. Run `build-msvc/trr_qt_trainer.exe` (Run as Administrator).
4. Click `Start Game` to pick a configured title, or start RPCS3 manually and click `Attach RPCS3`.
5. Optional: use `Start RPCS3` to pick `0.0.13`, `Latest`, or `Custom` launch.
6. Optional: use `Snapshot`, `TR Manual`, and `E3 2013` utility buttons.
7. If pointer is unresolved, enter splash/demo once, then click `Refresh Pointer`.
8. Pick P1/P2/stage and keep `Lock values continuously` enabled.
9. Click `Apply Once` to prime values; lock mode continues applying automatically.
10. Optional: open `Runtime` and `Value Writes` dialogs for focused controls.
11. Optional: open `Advanced Memory` for advanced position/animation/global-stage reads/writes.
12. Optional: click `Restart Game` any time to relaunch the same configured target.
13. Optional: click `Reset RPCS3` to restart the active emulator session.
14. Optional: click `Terminate RPCS3` to force-stop active RPCS3 processes.
15. Optional: click `Save Preset` to store current setup, and `Load Preset` later.
16. Watch `Live Monitor` to verify values during transitions.
17. Keep `Pause writes around round transitions` enabled (default) and tune `Pause ms` if needed.

### One-click profiles
- Conservative: strongest stability defaults. No continuous character/stage lock, stage auto-disable on match start, guard pause 3600ms, mode=5, HP/UI on, infinite round on, timer on (60s), counters lock off.
- Balanced: practical default. No continuous character/stage lock, stage auto-disable on match start, guard pause 2400ms, timer lock off, counters lock off.
- Aggressive: maximum forcing. Continuous character/stage lock on, stage auto-disable off, guard off, mode reset pulse on, mode=1, HP/UI on, infinite round on, timer on (90s), counters lock on.

By default, lock mode re-applies only state/timer/counter values. Character/stage writes are one-shot unless you enable `Also lock character/stage selection continuously`.
If enabled, `Auto-disable stage lock once match starts` will stop stage writes automatically as soon as a non-zero round timer is detected.

`Apply Once` now follows the same Lua behavior: it writes selected values and verifies each write before enabling optional stabilizer/lock runtime writes.

## Notes on the black-screen-after-one-round issue (game still runs)
The app defaults to locking values continuously to avoid game state drifting after a round end.

Immediatelely after the fight ends press `START` this prevents the black screen and you will be returned to the start screen.

If black screen still occurs, try:
- Keep `Write game mode` (mode `5`), `Write HP/UI field`, and `Write infinite round` enabled.
- Enable `Write round timer` with a non-zero value.
- Disable stage forcing after the match starts (if your build is sensitive to stage writes).
- Increase transition `Pause ms` (for example from 2400 to 3600).

## CT-derived extra offsets included
From `battle-struct` in your CT:

- `0x170` p1 character id
- `0x174` p2 character id
- `0x178` game state
- `0x290` counter up (likely rounds/player counter)
- `0x29C` counter up (likely rounds/player counter)
- `0x2A0` round timer
- `0x2AC` UI flags (`0x10000000` = UI on)
- `0x2B4` infinite round time
- `0x2B8` stage id

---

## Initial set-up and prerequisites

### Fixing memory allocation on Windows
RPCS3 uses a lot of memory regions and sometimes gives you an error about allocating memory.
* In your Windows search menu type msconfig and open it, then click on the Boot tab.
* Click on Advanced Options and uncheck Maximum memory.
* Checkmark Number of processors and set it to the highest number. Click OK Apply then restart your PC.

### RPCS3
* After installation disable `Check for updates` on startup in settings.
* Install the PlayStation 3 4.40 Firmware, since this game was released for version 4.40.
* For the game settings use the custom configuration, described below.

### Running the game
* Install a copy of your legally owned Tekken Revolution game, you need the pkg file and the rap file, drag and drop them on the RPCS3 GUI.
* Drag and drop the updates in RPCS3 and do them one by one from update 1.01 through 1.05 in that order.
* Optional: copy the `custom_configs` folder (which contains config_NPEB01406.yml) to the config directory of RPCS3 to remove black borders.
* Edit the custom config and add your own Adapter, example: `Adapter: NVIDIA GeForce GTX 1650`.
* Right-click on the game icon in the UI and select: `Boot with custom configuration`. 
* When starting the Game, the game’s splash screen will appear, pressing Start will return you back into the game.
* After about 20 seconds of pressing nothing the game will enter demo mode.

### Extras
* You can use a DDS viewer to be able to read the PlayStation 3 Manuals: Windows_DDS_Texture_Viewer_v089b.


### List of stage id's
1. 02 - Eternal Paradise (Fiji)
2. 03 - Historic Town Square (Germany)
3. 04 - Condor Canyon (Colombia)
4. 05 - Arctic Dream (Finland)
5. 08 - Moonlit Wilderness (UK)
6. 0B - Sakura Schoolyard (Japan)
7. 0C - Tempest (Norway)
8. 0D - Winter Palace (Canada)
9. 0E - Hall of Judgement (Japan)
10. 0F - Naraku (Japan)
11. 18 - [darkness]
12. 22 - Practice (walls)
13. 23 - Practice (no walls)
14. 28 - Fireworks Over Barcelona (Spain)
15. 2A - Riverside Promenade (France)
16. 2B - Tropical Rainforest (Brazil)
17. 2C - Moai Excavation (Chile)
18. 2D - Extravagant Underground (Russia)
19. 2E - Tulip Festival (Netherlands)

### List of character id's
1. Paul Phoenix - pau – 000
2. Marshall Law - law – 00A
3. King - kin – 015
4. Nina Williams - nin – 020
5. Hwoarang - hwo – 02A
6. Ling Xiaoyu - xia – 034
7. Christie Monteiro - chr – 041
8. Eddy Gordo - edd – 04B (softlocks)
9. Jin Kazama - jin – 04C
10. Julia Chang - jul – 056
11. Kuma - kum – 05D
12. Bryan Fury - bry – 064
13. Heihachi Mishima - hei – 06E
14. Kazuya Mishima - kaz – 06F
15. Lee Chaolan - lee – 079
16. Steve Fox - ste – 07D
17. Mokujin - mok – 088
18. Jack-6 - jac – 08B
19. Asuka Kazama - asu – 099
20. Devil Jin - dvj – 0A8
21. Feng Wei - fen – 0B3
22. Armor King - amk – 0BA
23. Lili (Emilie De Rochefort) - lil – 0BE
24. Sergei Dragunov - dra – 0CB
25. Bob (Robert Richards) - bob – 0D5
26. Zafina - zaf – 0D9 (softlocks)
27. Miguel Caballero Rojo - mig – 0DA
28. Leo Kliesen - leo – 0E1
29. Lars Alexandersson - lar – 0EB
30. Alisa Bosconovitch - ali – 0F5
31. Jinpachi Mishima - jnp – 102
32. Ogre - ogr – 103
33. Jun Kazama - jun – 105
34. Kinjin - knm – 10E
35. Eliza - vmp – 126
36. Eliza (Version 2) - vmp ver2 – 12C

Note: "vmp" was the internal code for "Vampire" during development, which became Eliza, and "knm" corresponds to the unplayable boss Kinjin.

**All credits go to `dennisstanistan` for publishing his findings to get Tekken Revolution to run offline.**

---

## Caution
**You are only allowed to use a copy of your legally owned PlayStation 3 games.**
