# TRR Cheat Engine Automation

This replaces the manual memory-edit steps with a small Cheat Engine Lua frontend.

![TRR_256.png](assets/images/TRR_256.png)

## File

- `trr_ce_automation.lua`

## What it automates

- Attach to `rpcs3.exe` (fallback to `rpcs3-avx2.exe`)
- Resolve the dynamic `BattleManager` pointer from `0x3200D26BC`
- Set:
  - Player 1 character ID (`battlePointer + 0x170`)
  - Player 2 character ID (`battlePointer + 0x174`)
  - Stage ID (`battlePointer + 0x2B8`)
- Optional defaults:
  - Game mode (`battlePointer + 0x178`) with editable value
  - Round timer (`battlePointer + 0x2A0`) with editable value
  - HP bar (`battlePointer + 0x2AC`) with editable value
  - Infinite round (`battlePointer + 0x2B4`) with editable value
  - P1 state to controller (`base + 0x12DA338` = 0)
  - P2 state to CPU (`base + 0x12DC7D8` = 1)
- Verify writes by reading values back after apply (pass/fail in popup)
- Optional stabilization loop that reapplies values for about 10 seconds
- Optional continuous lock that keeps character/stage/state applied while running
- Live read button to pull current mode/HP/round from memory into UI fields
- Optional mode reset pulse (`4 -> target`) for better in-fight apply behavior
- Round-time presets: `Infinite`, `30 seconds`, `60 seconds`, `90 seconds`, `Custom`
- Transition guard: pauses writes around round transitions to reduce black-screen risk
- Advanced memory reads/writes:
  - P1/P2 position vectors
  - P1 animation speed
  - Global stage ID
  - Game state readbacks (`0x12E9194`, `0x2013F5B8`)

## Stable usage (safer approach only)

1. Start RPCS3 and launch Tekken Revolution 01.05.
2. Open Cheat Engine.
3. Open your table from `Cheat Engine Table for TR/rpcs3-NPEB01406.CT`.
4. In Cheat Engine, open `Table -> Show Cheat Table Lua Script`.
5. Paste `trr_ce_automation.lua` content and execute.
6. In the `TRR CE Automation` window click `Attach RPCS3`.
7. Let the game reach splash/demo (no pause required).
8. Pick P1/P2/Stage.
9. Optional: set `Game mode`, `HP bar`, and `Infinite round` values.
10. Optional: set `Round timer` directly or use the round-time preset combo.
11. Optional: enable `Apply mode using reset pulse (4 -> target)` when mode changes do not stick.
12. Optional: click `Read Live Values` to inspect current mode/HP/round/timer before applying.
13. Keep `Pause writes around round transitions` enabled.
14. Click `Apply Selection`.
15. Read the popup:
   - If all entries are `OK`, the values were applied and verified.
   - If any entry is `FAIL`, stay on splash/demo for 1-2 seconds and click `Apply Selection` again.
16. If continuous lock is active and you want to stop all background writes, click `Stop Lock`.

### Advanced memory workflow (optional)

1. Click `Read Advanced Values` to pull live position/state values.
2. Check only the advanced writes you want to apply.
3. Click `Apply Checked Advanced Values`.
4. Use this mode carefully: these writes are direct and bypass the safer preset flow.

Default startup values in the current Lua UI:
- P1 = `Lili`
- P2 = `Bob`
- Stage = `28 - Fireworks Over Barcelona`
- Game mode = `1`
- HP bar = `0x4000000`

## Presets

- `Preset: Stable Practice`
  - Mode=5, HP=0x10000000, Infinite Round=1, Stabilizer ON, Lock ON.
- `Preset: Char/Stage Only`
  - Mode OFF, HP OFF, Infinite Round OFF, Stabilizer ON, Lock ON.
- `Preset: Round Safe`
  - Mode OFF, HP OFF, Infinite Round OFF, Lock ON.
  - Use this as the default black-screen avoidance test preset.

## Round timer behavior

- `Infinite` preset:
  - Writes `Infinite round = 1`.
  - Round does not end by timer.
- `30 seconds`, `60 seconds`, `90 seconds` presets:
  - Write `Infinite round = 0` and `Round timer = 30/60/90`.
  - This gives finite rounds while keeping versus behavior.
- `Custom`:
  - Leaves timer selection manual.

## Game mode notes (current findings)

- `1`:
  - Continuous fight behavior.
  - Can black-screen at round end on some transitions.
- `4`:
  - Useful as a reset/wake workaround.
  - Can recover character from downed/no-reset situations.
- `5`:
  - Practice mode (most stable known mode).

The mode list in the UI now also includes:
- `2 - Unknown (0x2)`
- `3 - Unknown (0x3)`

These are empirical results from this setup; unknown mode values are still being mapped.

## Continuous-play workflow (recommended)

1. Start from `Preset: Round Safe`.
2. Choose desired P1/P2/Stage.
3. Set round-time preset to `30 seconds` (or `60/90`) for normal round ends.
4. Enable `Write game mode` and set mode to `1`.
5. Keep `Pause writes around round transitions` enabled and `Auto-disable stage lock after match starts` enabled.
6. Enable `Apply mode using reset pulse (4 -> target)` and click `Apply Selection`.
7. If a fighter gets stuck down with no reset, set mode `4`, apply once, then mode `1`, apply again.
8. If black-screen happens after KO/end, switch back to `Preset: Round Safe` and avoid mode/HP/round writes for that session.

## Known-good validation test (deterministic)

Use this exact setup to confirm your pipeline is working:

1. Keep `Write game mode` checked and set value `5`.
2. Keep `Write HP bar` checked and set value `0x10000000`.
3. Keep `Write infinite round` checked and set value `1`.
4. Keep `Stabilize writes for ~10s` checked.
5. Keep `Set P1 to controller` checked.
6. Keep `Set P2 to CPU` checked.
7. Set:
   - P1 = `Armor King`
   - P2 = `Mokujin`
   - Stage = `23 - Practice (no walls)`
8. Click `Apply Selection` until popup shows all `OK`.

Expected result:

- You control P1.
- P2 is CPU.
- Stage is Practice (no walls).
- Infinite-round style behavior is active (practice defaults applied).

If those 4 points happen, your setup is stable and correct.

## Practice mode vs fighting mode (rounds)

- `Write game mode` checked with value `5`:
  - Script writes practice mode and applies your HP/infinite-round values.
  - This is the most stable and recommended test path.
- `Write game mode` unchecked:
  - Script writes character/stage/player-state only.
  - Game mode/round behavior is left to whatever the game currently has.
  - Use this if you want to experiment with non-practice behavior.

Note: practice mode value `5` is currently the validated stable mode in this package. You can test other mode values, but they may be unstable depending on state/timing.

## Troubleshooting

- P2 does not change:
  - Keep `Set P2 to CPU` checked.
  - Keep `Stabilize writes for ~10s` checked.
  - Keep `Lock character/stage while running` checked.
  - Apply while on splash/demo, then let the transition finish before changing anything else.
- Black screen after round (music still playing):
  - First test with `Preset: Round Safe`.
  - Then add one variable at a time (mode, then HP, then infinite round).
  - Prefer finite round-time presets (`30/60/90`) over infinite for versus tests.
  - If using mode `1`, enable `Apply mode using reset pulse (4 -> target)`.
  - Keep `Pause writes around round transitions` enabled.
  - Keep `Auto-disable stage lock after match starts` enabled.
  - If black screen repeats, keep mode/HP/round disabled for continuous sessions.
  - Game-mode softlocks usually require RPCS3 restart; stopping CE writes may not recover that state.

## Live changes vs restart

- You can change character/stage while the game is already running.
- With continuous lock on, select new values and click `Apply Selection` again; restart is not required.
- If the game enters a black-screen softlock state (music continues, no input response), restart RPCS3 is usually required because the game state itself is already stuck.

## Notes

- This assumes your current memory layout from the included package and Tekken Revolution 01.05.
- No data breakpoint and no RPCS3 pause is required for this flow.
- If pointer resolution fails, stay in splash/demo briefly and click `Apply Selection` again (it retries pointer resolution).
- Character/stage IDs are taken from your `README_TRR.txt` list.
- If any ID encoding differs on your setup, you can adjust IDs directly inside the Lua table maps.

## Next step (optional)

If you want this fully independent from Cheat Engine, the same logic can be moved to a standalone Qt app that uses `OpenProcess` + `ReadProcessMemory` + `WriteProcessMemory`.
