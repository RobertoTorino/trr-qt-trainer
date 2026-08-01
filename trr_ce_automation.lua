-- Tekken Revolution Reborn - Cheat Engine automation helper
-- Run this from Cheat Engine: Table -> Show Cheat Table Lua Script, paste and execute,
-- or assign it to a menu item.

local BASE_ADDR = 0x300000000
local THIS_POINTER_ADDR = 0x3200D26BC
local POINTER_RETRY_COUNT = 30
local POINTER_RETRY_DELAY_MS = 200
local VERIFY_RETRY_COUNT = 8
local VERIFY_RETRY_DELAY_MS = 40
local STABILIZE_CYCLES = 40
local STABILIZE_DELAY_MS = 250
local LOCK_INTERVAL_MS = 250
local MODE_RESET_PULSE_MS = 120
local TRANSITION_GUARD_PAUSE_MS = 2800
local ROUND_TIMER_TICKS_PER_SECOND = 60

local OFF_P1_ID = 0x170
local OFF_P2_ID = 0x174
local OFF_GAME_MODE = 0x178
local OFF_ROUND_TIMER = 0x2A0
local OFF_HP_BAR = 0x2AC
local OFF_INFINITE_ROUND = 0x2B4
local OFF_STAGE_ID = 0x2B8

local ADDR_P1_STATE = BASE_ADDR + 0x12DA338
local ADDR_P2_STATE = BASE_ADDR + 0x12DC7D8
local ADDR_P1_POS_X = BASE_ADDR + 0x12D9F00
local ADDR_P1_POS_Y = BASE_ADDR + 0x12D9F04
local ADDR_P1_POS_Z = BASE_ADDR + 0x12D9F08
local ADDR_P1_ANIMATION_SPEED = BASE_ADDR + 0x12DA59C
local ADDR_P2_POS_X = BASE_ADDR + 0x12DC3A0
local ADDR_P2_POS_Y = BASE_ADDR + 0x12DC3A4
local ADDR_P2_POS_Z = BASE_ADDR + 0x12DC3A8
local ADDR_GAME_STATE = BASE_ADDR + 0x12E9194
local ADDR_GAME_STATE_READ = BASE_ADDR + 0x2013F5B8
local ADDR_GLOBAL_STAGE_ID = BASE_ADDR + 0x200D703C

local CHARACTER_IDS = {
  { label = "Paul Phoenix", id = 0x000 },
  { label = "Marshall Law", id = 0x00A },
  { label = "King", id = 0x015 },
  { label = "Nina Williams", id = 0x020 },
  { label = "Hwoarang", id = 0x02A },
  { label = "Ling Xiaoyu", id = 0x034 },
  { label = "Christie Monteiro", id = 0x041 },
  { label = "Eddy Gordo (softlocks)", id = 0x04B },
  { label = "Jin Kazama", id = 0x04C },
  { label = "Julia Chang", id = 0x056 },
  { label = "Kuma", id = 0x05D },
  { label = "Bryan Fury", id = 0x064 },
  { label = "Heihachi Mishima", id = 0x06E },
  { label = "Kazuya Mishima", id = 0x06F },
  { label = "Lee Chaolan", id = 0x079 },
  { label = "Steve Fox", id = 0x07D },
  { label = "Mokujin", id = 0x088 },
  { label = "Jack-6", id = 0x08B },
  { label = "Asuka Kazama", id = 0x099 },
  { label = "Devil Jin", id = 0x0A8 },
  { label = "Feng Wei", id = 0x0B3 },
  { label = "Armor King", id = 0x0BA },
  { label = "Lili", id = 0x0BE },
  { label = "Sergei Dragunov", id = 0x0CB },
  { label = "Bob", id = 0x0D5 },
  { label = "Zafina (softlocks)", id = 0x0D9 },
  { label = "Miguel", id = 0x0DA },
  { label = "Leo", id = 0x0E1 },
  { label = "Lars", id = 0x0EB },
  { label = "Alisa", id = 0x0F5 },
  { label = "Jinpachi", id = 0x102 },
  { label = "Ogre", id = 0x103 },
  { label = "Jun", id = 0x105 },
  { label = "Kinjin", id = 0x10E },
  { label = "Eliza", id = 0x126 },
  { label = "Eliza v2", id = 0x12C }
}

local STAGE_IDS = {
  { label = "02 Eternal Paradise", id = 0x02 },
  { label = "03 Historic Town Square", id = 0x03 },
  { label = "04 Condor Canyon", id = 0x04 },
  { label = "05 Arctic Dream", id = 0x05 },
  { label = "08 Moonlit Wilderness", id = 0x08 },
  { label = "0B Sakura Schoolyard", id = 0x0B },
  { label = "0C Tempest", id = 0x0C },
  { label = "0D Winter Palace", id = 0x0D },
  { label = "0E Hall of Judgement", id = 0x0E },
  { label = "0F Naraku", id = 0x0F },
  { label = "18 Darkness", id = 0x18 },
  { label = "22 Practice (walls)", id = 0x22 },
  { label = "23 Practice (no walls)", id = 0x23 },
  { label = "28 Fireworks Over Barcelona", id = 0x28 },
  { label = "2A Riverside Promenade", id = 0x2A },
  { label = "2B Tropical Rainforest", id = 0x2B },
  { label = "2C Moai Excavation", id = 0x2C },
  { label = "2D Extravagant Underground", id = 0x2D },
  { label = "2E Tulip Festival", id = 0x2E }
}

local GAME_MODE_KNOWN = {
  { value = 1, label = "1 Versus | Continuous Fight" },
  { value = 2, label = "2 Interactive Splash Demo (0x2)" },
  { value = 3, label = "3 Unknown (0x3)" },
  { value = 4, label = "4 Round reset/wake workaround" },
  { value = 5, label = "5 Practice mode (stable)" }
}

local ROUND_TIME_PRESETS = {
  { label = "Infinite", infinite_value = 1, timer_value = nil },
  { label = "30 Seconds", infinite_value = 0, timer_value = 30 },
  { label = "60 Seconds", infinite_value = 0, timer_value = 60 },
  { label = "90 Seconds", infinite_value = 0, timer_value = 90 },
  { label = "Custom", infinite_value = nil, timer_value = nil }
}

local function split_u32_be(value)
  local b1 = math.floor(value / 0x1000000) % 0x100
  local b2 = math.floor(value / 0x10000) % 0x100
  local b3 = math.floor(value / 0x100) % 0x100
  local b4 = value % 0x100
  return b1, b2, b3, b4
end

local function read_u32_be(addr)
  local t = readBytes(addr, 4, true)
  if t == nil or #t < 4 then
    return nil
  end

  return (t[1] * 0x1000000) + (t[2] * 0x10000) + (t[3] * 0x100) + t[4]
end

local function write_u32_be(addr, value)
  local b1, b2, b3, b4 = split_u32_be(value)
  return writeBytes(addr, b1, b2, b3, b4)
end

local function read_f32_be(addr)
  local t = readBytes(addr, 4, true)
  if t == nil or #t < 4 then
    return nil
  end

  return byteTableToFloat({ t[4], t[3], t[2], t[1] })
end

local function write_f32_be(addr, value)
  local t = floatToByteTable(value)
  if t == nil or #t < 4 then
    return false
  end

  return writeBytes(addr, t[4], t[3], t[2], t[1])
end

local function resolve_battle_ptr()
  local ptr_offset = read_u32_be(THIS_POINTER_ADDR)
  if ptr_offset == nil or ptr_offset == 0 then
    return nil
  end

  return BASE_ADDR + ptr_offset
end

local function resolve_battle_ptr_with_retry(retry_count, delay_ms)
  for _ = 1, retry_count do
    local ptr = resolve_battle_ptr()
    if ptr ~= nil then
      return ptr
    end
    sleep(delay_ms)
  end

  return nil
end

local function ensure_rpcs3_attached()
  local attached = false
  pcall(function()
    attached = openProcess("rpcs3.exe")
  end)

  if not attached then
    pcall(function()
      attached = openProcess("rpcs3-avx2.exe")
    end)
  end

  return attached
end

local form = createForm(false)
form.Caption = "TRR CE Automation"
form.Width = 560
form.Height = 920

local status_label = createLabel(form)
status_label.Left = 16
status_label.Top = 16
status_label.Caption = "Status: Not Attached"
status_label.Width = 520

local p1_label = createLabel(form)
p1_label.Left = 16
p1_label.Top = 52
p1_label.Caption = "Player 1 Character"

local p1_combo = createComboBox(form)
p1_combo.Left = 16
p1_combo.Top = 72
p1_combo.Width = 250

local p2_label = createLabel(form)
p2_label.Left = 286
p2_label.Top = 52
p2_label.Caption = "Player 2 Character"

local p2_combo = createComboBox(form)
p2_combo.Left = 286
p2_combo.Top = 72
p2_combo.Width = 250

local stage_label = createLabel(form)
stage_label.Left = 16
stage_label.Top = 112
stage_label.Caption = "Stage"

local stage_combo = createComboBox(form)
stage_combo.Left = 16
stage_combo.Top = 132
stage_combo.Width = 520

local p1_state_checkbox = createCheckBox(form)
p1_state_checkbox.Left = 16
p1_state_checkbox.Top = 170
p1_state_checkbox.Caption = "Set P1 to Controller (value 0)"
p1_state_checkbox.Checked = true

local p2_state_checkbox = createCheckBox(form)
p2_state_checkbox.Left = 286
p2_state_checkbox.Top = 170
p2_state_checkbox.Caption = "Set P2 to CPU (value 1)"
p2_state_checkbox.Checked = true

local mode_checkbox = createCheckBox(form)
mode_checkbox.Left = 16
mode_checkbox.Top = 198
mode_checkbox.Caption = "Write Game Mode"
mode_checkbox.Checked = true

local mode_edit = createEdit(form)
mode_edit.Left = 286
mode_edit.Top = 196
mode_edit.Width = 70
mode_edit.Text = "1"

local mode_preset_combo = createComboBox(form)
mode_preset_combo.Left = 362
mode_preset_combo.Top = 196
mode_preset_combo.Width = 174

local mode_desc_label = createLabel(form)
mode_desc_label.Left = 286
mode_desc_label.Top = 218
mode_desc_label.Caption = "Mode: 1 Versus | Continuous Fight"
mode_desc_label.Width = 250

local hp_checkbox = createCheckBox(form)
hp_checkbox.Left = 16
hp_checkbox.Top = 246
hp_checkbox.Caption = "Write HP Bar"
hp_checkbox.Checked = true

local hp_edit = createEdit(form)
hp_edit.Left = 286
hp_edit.Top = 244
hp_edit.Width = 250
hp_edit.Text = "0x4000000"

local round_checkbox = createCheckBox(form)
round_checkbox.Left = 16
round_checkbox.Top = 274
round_checkbox.Caption = "Write Infinite Round"
round_checkbox.Checked = true

local round_edit = createEdit(form)
round_edit.Left = 286
round_edit.Top = 272
round_edit.Width = 70
round_edit.Text = "1"

local round_time_checkbox = createCheckBox(form)
round_time_checkbox.Left = 16
round_time_checkbox.Top = 302
round_time_checkbox.Caption = "Write Round Timer"
round_time_checkbox.Checked = false

local round_time_edit = createEdit(form)
round_time_edit.Left = 286
round_time_edit.Top = 300
round_time_edit.Width = 70
round_time_edit.Text = "30"

local round_time_preset_combo = createComboBox(form)
round_time_preset_combo.Left = 362
round_time_preset_combo.Top = 300
round_time_preset_combo.Width = 174

local round_time_desc_label = createLabel(form)
round_time_desc_label.Left = 286
round_time_desc_label.Top = 322
round_time_desc_label.Caption = "Round Time: Custom"
round_time_desc_label.Width = 250

local stabilize_checkbox = createCheckBox(form)
stabilize_checkbox.Left = 16
stabilize_checkbox.Top = 350
stabilize_checkbox.Caption = "Stabilize Writes for ~10s After Apply | Recommended"
stabilize_checkbox.Checked = true

local lock_checkbox = createCheckBox(form)
lock_checkbox.Left = 16
lock_checkbox.Top = 378
lock_checkbox.Caption = "Lock Character | Stage While Running"
lock_checkbox.Checked = false

local mode_reset_checkbox = createCheckBox(form)
mode_reset_checkbox.Left = 16
mode_reset_checkbox.Top = 406
mode_reset_checkbox.Caption = "Apply Mode Using Reset Pulse | 4 -> Target"
mode_reset_checkbox.Checked = false

local transition_guard_checkbox = createCheckBox(form)
transition_guard_checkbox.Left = 16
transition_guard_checkbox.Top = 434
transition_guard_checkbox.Caption = "Pause Writes Around Round Transitions"
transition_guard_checkbox.Checked = true

local transition_guard_edit = createEdit(form)
transition_guard_edit.Left = 286
transition_guard_edit.Top = 432
transition_guard_edit.Width = 70
transition_guard_edit.Text = tostring(TRANSITION_GUARD_PAUSE_MS)

local transition_guard_label = createLabel(form)
transition_guard_label.Left = 362
transition_guard_label.Top = 434
transition_guard_label.Caption = "Pause ms"
transition_guard_label.Width = 80

local stage_autodisable_checkbox = createCheckBox(form)
stage_autodisable_checkbox.Left = 16
stage_autodisable_checkbox.Top = 462
stage_autodisable_checkbox.Caption = "Auto-Disable Stage Lock After Match Starts"
stage_autodisable_checkbox.Checked = true

local pointer_label = createLabel(form)
pointer_label.Left = 16
pointer_label.Top = 490
pointer_label.Caption = "Battle Pointer: Unresolved"
pointer_label.Width = 520

local hint_label = createLabel(form)
hint_label.Left = 16
hint_label.Top = 510
hint_label.Caption = "Let Splash Demo Load Then Click Apply Selection."
hint_label.Width = 520

local preset_stable_button = createButton(form)
preset_stable_button.Left = 16
preset_stable_button.Top = 538
preset_stable_button.Width = 250
preset_stable_button.Caption = "Preset Stable Practice"

local preset_charonly_button = createButton(form)
preset_charonly_button.Left = 286
preset_charonly_button.Top = 538
preset_charonly_button.Width = 250
preset_charonly_button.Caption = "Preset Char | Stage Only"

local preset_roundsafe_button = createButton(form)
preset_roundsafe_button.Left = 366
preset_roundsafe_button.Top = 606
preset_roundsafe_button.Width = 170
preset_roundsafe_button.Caption = "Preset Round Safe"

local attach_button = createButton(form)
attach_button.Left = 16
attach_button.Top = 570
attach_button.Width = 120
attach_button.Caption = "Attach RPCS3"

local refresh_button = createButton(form)
refresh_button.Left = 146
refresh_button.Top = 570
refresh_button.Width = 120
refresh_button.Caption = "Refresh Pointer"

local apply_button = createButton(form)
apply_button.Left = 276
apply_button.Top = 570
apply_button.Width = 120
apply_button.Caption = "Apply Selection"

local read_live_button = createButton(form)
read_live_button.Left = 406
read_live_button.Top = 570
read_live_button.Width = 130
read_live_button.Caption = "Read Live Values"

local stop_lock_button = createButton(form)
stop_lock_button.Left = 16
stop_lock_button.Top = 606
stop_lock_button.Width = 170
stop_lock_button.Caption = "Stop Lock"

local close_button = createButton(form)
close_button.Left = 186
close_button.Top = 606
close_button.Width = 130
close_button.Caption = "Close"

local advanced_label = createLabel(form)
advanced_label.Left = 16
advanced_label.Top = 646
advanced_label.Caption = "Advanced Memory"
advanced_label.Width = 520

local advanced_p1_checkbox = createCheckBox(form)
advanced_p1_checkbox.Left = 16
advanced_p1_checkbox.Top = 674
advanced_p1_checkbox.Caption = "Write P1 Position"

local advanced_p1_x = createEdit(form)
advanced_p1_x.Left = 176
advanced_p1_x.Top = 672
advanced_p1_x.Width = 110
advanced_p1_x.Text = "0"

local advanced_p1_y = createEdit(form)
advanced_p1_y.Left = 296
advanced_p1_y.Top = 672
advanced_p1_y.Width = 110
advanced_p1_y.Text = "0"

local advanced_p1_z = createEdit(form)
advanced_p1_z.Left = 416
advanced_p1_z.Top = 672
advanced_p1_z.Width = 120
advanced_p1_z.Text = "0"

local advanced_p2_checkbox = createCheckBox(form)
advanced_p2_checkbox.Left = 16
advanced_p2_checkbox.Top = 704
advanced_p2_checkbox.Caption = "Write P2 Position"

local advanced_p2_x = createEdit(form)
advanced_p2_x.Left = 176
advanced_p2_x.Top = 702
advanced_p2_x.Width = 110
advanced_p2_x.Text = "0"

local advanced_p2_y = createEdit(form)
advanced_p2_y.Left = 296
advanced_p2_y.Top = 702
advanced_p2_y.Width = 110
advanced_p2_y.Text = "0"

local advanced_p2_z = createEdit(form)
advanced_p2_z.Left = 416
advanced_p2_z.Top = 702
advanced_p2_z.Width = 120
advanced_p2_z.Text = "0"

local advanced_animation_checkbox = createCheckBox(form)
advanced_animation_checkbox.Left = 16
advanced_animation_checkbox.Top = 734
advanced_animation_checkbox.Caption = "Write P1 Animation Speed"

local advanced_animation_edit = createEdit(form)
advanced_animation_edit.Left = 216
advanced_animation_edit.Top = 732
advanced_animation_edit.Width = 110
advanced_animation_edit.Text = "0x00001000"

local advanced_stage_checkbox = createCheckBox(form)
advanced_stage_checkbox.Left = 16
advanced_stage_checkbox.Top = 764
advanced_stage_checkbox.Caption = "Write Global Stage id"

local advanced_stage_edit = createEdit(form)
advanced_stage_edit.Left = 216
advanced_stage_edit.Top = 762
advanced_stage_edit.Width = 110
advanced_stage_edit.Text = "0x00000000"

local advanced_state_label = createLabel(form)
advanced_state_label.Left = 16
advanced_state_label.Top = 796
advanced_state_label.Caption = "Game State: n/a | Game State (Read): n/a"
advanced_state_label.Width = 520

local advanced_read_button = createButton(form)
advanced_read_button.Left = 16
advanced_read_button.Top = 826
advanced_read_button.Width = 250
advanced_read_button.Caption = "Read Advanced Values"

local advanced_apply_button = createButton(form)
advanced_apply_button.Left = 286
advanced_apply_button.Top = 826
advanced_apply_button.Width = 250
advanced_apply_button.Caption = "Apply Checked Advanced Values"

for i = 1, #CHARACTER_IDS do
  local entry = CHARACTER_IDS[i]
  p1_combo.Items.add(string.format("%s (0x%03X)", entry.label, entry.id))
  p2_combo.Items.add(string.format("%s (0x%03X)", entry.label, entry.id))
end

for i = 1, #STAGE_IDS do
  local entry = STAGE_IDS[i]
  stage_combo.Items.add(string.format("%s (0x%02X)", entry.label, entry.id))
end

for i = 1, #GAME_MODE_KNOWN do
  local entry = GAME_MODE_KNOWN[i]
  mode_preset_combo.Items.add(string.format("%s", entry.label))
end

for i = 1, #ROUND_TIME_PRESETS do
  local entry = ROUND_TIME_PRESETS[i]
  round_time_preset_combo.Items.add(entry.label)
end

p1_combo.ItemIndex = 22 -- Lili
p2_combo.ItemIndex = 24 -- Bob
stage_combo.ItemIndex = 13 -- 28 - Fireworks Over Barcelona
mode_preset_combo.ItemIndex = 0 -- mode 1
round_time_preset_combo.ItemIndex = 0 -- infinite

local cached_battle_pointer = nil
local runtime_timer = createTimer(form, false)
runtime_timer.Interval = LOCK_INTERVAL_MS
runtime_timer.Enabled = false
local runtime_stabilize_cycles_left = 0
local runtime_lock_enabled = false
local runtime_context = nil
local runtime_transition_pause_until_ms = 0
local runtime_last_round_timer = nil
local runtime_stage_lock_disabled = false
local parse_u32_input

local function describe_mode(value)
  for i = 1, #GAME_MODE_KNOWN do
    if GAME_MODE_KNOWN[i].value == value then
      return GAME_MODE_KNOWN[i].label
    end
  end

  return string.format("%d - Unknown", value)
end

local function update_mode_description_from_text()
  local value, _ = parse_u32_input(mode_edit.Text)
  if value == nil then
    mode_desc_label.Caption = "Mode: Invalid"
    return
  end

  mode_desc_label.Caption = "Mode: " .. describe_mode(value)
end

local function update_round_time_description_from_text()
  local inf_value, _ = parse_u32_input(round_edit.Text)
  local timer_value, _ = parse_u32_input(round_time_edit.Text)

  if inf_value == nil then
    round_time_desc_label.Caption = "Round Time: Invalid"
    return
  end

  if inf_value == 1 then
    round_time_desc_label.Caption = "Round Time: Infinite"
    return
  end

  if timer_value == nil then
    round_time_desc_label.Caption = "Round Time: Invalid"
    return
  end

  round_time_desc_label.Caption = string.format("Round Time: %d Seconds (%d Ticks)", timer_value, timer_value * ROUND_TIMER_TICKS_PER_SECOND)
end

local function apply_round_time_preset(index)
  local preset = ROUND_TIME_PRESETS[index + 1]
  if preset == nil then
    return
  end

  if preset.infinite_value ~= nil then
    round_edit.Text = tostring(preset.infinite_value)
  end

  if preset.timer_value ~= nil then
    round_time_checkbox.Checked = true
    round_time_edit.Text = tostring(preset.timer_value)
  elseif preset.infinite_value == 1 then
    round_time_checkbox.Checked = false
  end

  update_round_time_description_from_text()
end

local function now_ms()
  if getTickCount ~= nil then
    return getTickCount()
  end

  return math.floor(os.clock() * 1000)
end

parse_u32_input = function(value_text)
  if value_text == nil then
    return nil, "Empty Value"
  end

  local s = value_text:match("^%s*(.-)%s*$")
  if s == "" then
    return nil, "Empty Value"
  end

  local n = nil
  if s:sub(1, 2) == "0x" or s:sub(1, 2) == "0X" then
    n = tonumber(s:sub(3), 16)
  else
    n = tonumber(s, 10)
  end

  if n == nil then
    return nil, string.format("Invalid nNumber '%s'", s)
  end

  if n < 0 or n > 0xFFFFFFFF then
    return nil, string.format("Out of Range 0..0xFFFFFFFF (%s)", s)
  end

  return math.floor(n), "Ok"
end

local function update_pointer()
  cached_battle_pointer = resolve_battle_ptr_with_retry(POINTER_RETRY_COUNT, POINTER_RETRY_DELAY_MS)
  if cached_battle_pointer == nil then
    pointer_label.Caption = "Battle Pointer: Unresolved | Let Splash Demo Run Then Retry)"
    return false
  end

  pointer_label.Caption = string.format("Battle Pointer: 0x%X", cached_battle_pointer)
  return true
end

local function verify_u32_be(addr, expected)
  local actual = read_u32_be(addr)
  if actual == nil then
    return false, "Read Failed"
  end

  if actual ~= expected then
    return false, string.format("Expected 0x%X Got 0x%X", expected, actual)
  end

  return true, "ok"
end

local function write_and_verify_u32(addr, expected, label, checks)
  for _ = 1, VERIFY_RETRY_COUNT do
    local write_ok = write_u32_be(addr, expected)
    if write_ok then
      local verify_ok, verify_msg = verify_u32_be(addr, expected)
      if verify_ok then
        checks[#checks + 1] = string.format("%s: OK (%s)", label, verify_msg)
        return true
      end
    end
    sleep(VERIFY_RETRY_DELAY_MS)
  end

  local _, verify_msg = verify_u32_be(addr, expected)
  checks[#checks + 1] = string.format("%s: FAIL (%s)", label, verify_msg)
  return false
end

local function write_values_fast(battle_pointer, p1, p2, st, options)
  write_u32_be(battle_pointer + OFF_P1_ID, p1.id)
  write_u32_be(battle_pointer + OFF_P2_ID, p2.id)
  write_u32_be(battle_pointer + OFF_STAGE_ID, st.id)

  if options.write_mode then
    write_u32_be(battle_pointer + OFF_GAME_MODE, options.mode_value)
  end

  if options.write_hp then
    write_u32_be(battle_pointer + OFF_HP_BAR, options.hp_value)
  end

  if options.write_round then
    write_u32_be(battle_pointer + OFF_INFINITE_ROUND, options.round_value)
  end

  if options.write_round_time then
    write_u32_be(battle_pointer + OFF_ROUND_TIMER, options.round_time_value)
  end

  if options.write_p1_state then
    write_u32_be(ADDR_P1_STATE, 0)
  end

  if options.write_p2_state then
    write_u32_be(ADDR_P2_STATE, 1)
  end
end

local function read_live_values_into_ui()
  if not ensure_rpcs3_attached() then
    status_label.Caption = "Status: Failed to Attach"
    return false
  end

  if not update_pointer() then
    return false
  end

  local mode_value = read_u32_be(cached_battle_pointer + OFF_GAME_MODE)
  if mode_value ~= nil then
    mode_edit.Text = tostring(mode_value)
  end

  local hp_value = read_u32_be(cached_battle_pointer + OFF_HP_BAR)
  if hp_value ~= nil then
    hp_edit.Text = string.format("0x%X", hp_value)
  end

  local round_value = read_u32_be(cached_battle_pointer + OFF_INFINITE_ROUND)
  if round_value ~= nil then
    round_edit.Text = tostring(round_value)
  end

  local round_time_value = read_u32_be(cached_battle_pointer + OFF_ROUND_TIMER)
  if round_time_value ~= nil then
    local seconds = math.floor((round_time_value + (ROUND_TIMER_TICKS_PER_SECOND / 2)) / ROUND_TIMER_TICKS_PER_SECOND)
    round_time_edit.Text = tostring(seconds)
  end

  update_mode_description_from_text()
  update_round_time_description_from_text()

  local mode_txt = mode_value ~= nil and tostring(mode_value) or "n/a"
  local hp_txt = hp_value ~= nil and string.format("0x%X", hp_value) or "n/a"
  local infinite_txt = round_value ~= nil and tostring(round_value) or "n/a"
  local timer_raw_txt = round_time_value ~= nil and tostring(round_time_value) or "n/a"
  local timer_seconds_txt = round_time_value ~= nil and tostring(math.floor((round_time_value + (ROUND_TIMER_TICKS_PER_SECOND / 2)) / ROUND_TIMER_TICKS_PER_SECOND)) or "n/a"

  showMessage(string.format(
    "Live values read.\nMode=%s\nHP=%s\nInfinite Round=%s\nRound Timer Raw=%s\nRound Timer Seconds=%s",
    mode_txt,
    hp_txt,
    infinite_txt,
    timer_raw_txt,
    timer_seconds_txt
  ))

  return true
end

local function read_advanced_values_into_ui()
  if not ensure_rpcs3_attached() then
    status_label.Caption = "Status: Failed to Attach"
    return false
  end

  local p1_x = read_f32_be(ADDR_P1_POS_X)
  local p1_y = read_f32_be(ADDR_P1_POS_Y)
  local p1_z = read_f32_be(ADDR_P1_POS_Z)
  local p2_x = read_f32_be(ADDR_P2_POS_X)
  local p2_y = read_f32_be(ADDR_P2_POS_Y)
  local p2_z = read_f32_be(ADDR_P2_POS_Z)
  local animation_speed = read_u32_be(ADDR_P1_ANIMATION_SPEED)
  local game_state = read_u32_be(ADDR_GAME_STATE)
  local game_state_read = read_u32_be(ADDR_GAME_STATE_READ)
  local global_stage = read_u32_be(ADDR_GLOBAL_STAGE_ID)

  if p1_x == nil or p1_y == nil or p1_z == nil or p2_x == nil or p2_y == nil or p2_z == nil then
    status_label.Caption = "Status: Failed to Read Advanced Positions"
    return false
  end

  advanced_p1_x.Text = string.format("%.6g", p1_x)
  advanced_p1_y.Text = string.format("%.6g", p1_y)
  advanced_p1_z.Text = string.format("%.6g", p1_z)
  advanced_p2_x.Text = string.format("%.6g", p2_x)
  advanced_p2_y.Text = string.format("%.6g", p2_y)
  advanced_p2_z.Text = string.format("%.6g", p2_z)
  advanced_animation_edit.Text = animation_speed ~= nil and string.format("0x%08X", animation_speed) or "n/a"
  advanced_stage_edit.Text = global_stage ~= nil and string.format("0x%08X", global_stage) or "n/a"
  advanced_state_label.Caption = string.format(
    "Game state: %s | Game State | Read: %s",
    game_state ~= nil and string.format("0x%08X", game_state) or "n/a",
    game_state_read ~= nil and string.format("0x%08X", game_state_read) or "n/a"
  )
  status_label.Caption = "Status: Advanced Values Read"
  return true
end

local function parse_float_input(value_text, label)
  local value = tonumber(value_text)
  if value == nil then
    return nil, label .. " Must Be a Number"
  end
  return value, "Ok"
end

local function apply_advanced_values()
  if not ensure_rpcs3_attached() then
    status_label.Caption = "Status: Failed to Attach"
    return false
  end

  local writes_requested = false
  local all_ok = true

  if advanced_p1_checkbox.Checked then
    writes_requested = true
    local x, x_err = parse_float_input(advanced_p1_x.Text, "P1 X")
    local y, y_err = parse_float_input(advanced_p1_y.Text, "P1 Y")
    local z, z_err = parse_float_input(advanced_p1_z.Text, "P1 Z")
    if x == nil or y == nil or z == nil then
      showMessage(x_err ~= "ok" and x_err or (y_err ~= "ok" and y_err or z_err))
      return false
    end
    all_ok = write_f32_be(ADDR_P1_POS_X, x) and all_ok
    all_ok = write_f32_be(ADDR_P1_POS_Y, y) and all_ok
    all_ok = write_f32_be(ADDR_P1_POS_Z, z) and all_ok
  end

  if advanced_p2_checkbox.Checked then
    writes_requested = true
    local x, x_err = parse_float_input(advanced_p2_x.Text, "P2 X")
    local y, y_err = parse_float_input(advanced_p2_y.Text, "P2 Y")
    local z, z_err = parse_float_input(advanced_p2_z.Text, "P2 Z")
    if x == nil or y == nil or z == nil then
      showMessage(x_err ~= "ok" and x_err or (y_err ~= "ok" and y_err or z_err))
      return false
    end
    all_ok = write_f32_be(ADDR_P2_POS_X, x) and all_ok
    all_ok = write_f32_be(ADDR_P2_POS_Y, y) and all_ok
    all_ok = write_f32_be(ADDR_P2_POS_Z, z) and all_ok
  end

  if advanced_animation_checkbox.Checked then
    writes_requested = true
    local value, err = parse_u32_input(advanced_animation_edit.Text)
    if value == nil then
      showMessage("Invalid P1 Animation Speed: " .. err)
      return false
    end
    all_ok = write_u32_be(ADDR_P1_ANIMATION_SPEED, value) and all_ok
  end

  if advanced_stage_checkbox.Checked then
    writes_requested = true
    local value, err = parse_u32_input(advanced_stage_edit.Text)
    if value == nil then
      showMessage("Invalid Global Stage id: " .. err)
      return false
    end
    all_ok = write_u32_be(ADDR_GLOBAL_STAGE_ID, value) and all_ok
  end

  if not writes_requested then
    showMessage("Select at Least one advanced write option.")
    return false
  end

  status_label.Caption = all_ok and "Status: Advanced Values Applied" or "Status: Advanced Write Failed"
  return all_ok
end

local function write_character_stage_state_fast(battle_pointer, p1, p2, st, options, write_stage)
  write_u32_be(battle_pointer + OFF_P1_ID, p1.id)
  write_u32_be(battle_pointer + OFF_P2_ID, p2.id)

  if write_stage then
    write_u32_be(battle_pointer + OFF_STAGE_ID, st.id)
  end

  if options.write_p1_state then
    write_u32_be(ADDR_P1_STATE, 0)
  end

  if options.write_p2_state then
    write_u32_be(ADDR_P2_STATE, 1)
  end
end

local function stop_runtime_writes(update_status)
  runtime_stabilize_cycles_left = 0
  runtime_lock_enabled = false
  runtime_context = nil
  runtime_transition_pause_until_ms = 0
  runtime_last_round_timer = nil
  runtime_stage_lock_disabled = false
  runtime_timer.Enabled = false
  if update_status then
    status_label.Caption = "Status: Lock Stopped"
  end
end

runtime_timer.OnTimer = function()
  if runtime_context == nil then
    runtime_timer.Enabled = false
    return
  end

  local pointer_now = resolve_battle_ptr()
  if pointer_now ~= nil then
    cached_battle_pointer = pointer_now
  else
    pointer_now = cached_battle_pointer
  end

  if pointer_now == nil then
    return
  end

  local round_timer_now = read_u32_be(pointer_now + OFF_ROUND_TIMER)

  if runtime_context.options.transition_guard and round_timer_now ~= nil then
    if runtime_last_round_timer ~= nil and runtime_last_round_timer > 0 and round_timer_now == 0 then
      runtime_transition_pause_until_ms = now_ms() + runtime_context.options.transition_guard_pause_ms
    end
    runtime_last_round_timer = round_timer_now
  end

  if runtime_context.options.auto_disable_stage_lock and (not runtime_stage_lock_disabled) and round_timer_now ~= nil and round_timer_now > 0 then
    runtime_stage_lock_disabled = true
  end

  if now_ms() < runtime_transition_pause_until_ms then
    return
  end

  if runtime_stabilize_cycles_left > 0 then
    write_values_fast(pointer_now, runtime_context.p1, runtime_context.p2, runtime_context.st, runtime_context.options)
    runtime_stabilize_cycles_left = runtime_stabilize_cycles_left - 1
  end

  if runtime_lock_enabled then
    write_character_stage_state_fast(pointer_now, runtime_context.p1, runtime_context.p2, runtime_context.st, runtime_context.options, not runtime_stage_lock_disabled)
  end

  if runtime_stabilize_cycles_left <= 0 and not runtime_lock_enabled then
    runtime_timer.Enabled = false
  end
end

local function apply_preset_stable_practice()
  mode_checkbox.Checked = true
  mode_edit.Text = "5"
  hp_checkbox.Checked = true
  hp_edit.Text = "0x10000000"
  round_checkbox.Checked = true
  round_edit.Text = "1"
  round_time_checkbox.Checked = false
  round_time_preset_combo.ItemIndex = 0
  stabilize_checkbox.Checked = true
  lock_checkbox.Checked = true
  mode_reset_checkbox.Checked = false
  transition_guard_checkbox.Checked = true
  transition_guard_edit.Text = tostring(TRANSITION_GUARD_PAUSE_MS)
  stage_autodisable_checkbox.Checked = true
  update_mode_description_from_text()
  update_round_time_description_from_text()
  showMessage("Preset Applied: Stable Practice Mode\nMode=5, HP=0x10000000, Infinite Round=1, Stabilizer ON")
end

local function apply_preset_character_stage_only()
  mode_checkbox.Checked = false
  hp_checkbox.Checked = false
  round_checkbox.Checked = false
  round_time_checkbox.Checked = false
  round_time_preset_combo.ItemIndex = 4
  stabilize_checkbox.Checked = true
  lock_checkbox.Checked = true
  mode_reset_checkbox.Checked = false
  transition_guard_checkbox.Checked = true
  transition_guard_edit.Text = tostring(TRANSITION_GUARD_PAUSE_MS)
  stage_autodisable_checkbox.Checked = true
  update_mode_description_from_text()
  update_round_time_description_from_text()
  showMessage("Preset Applied: Character/Stage Only\nAll gameplay values OFF, only characters/stage/states will be written, Stabilizer ON")
end

local function apply_preset_round_safe()
  mode_checkbox.Checked = false
  hp_checkbox.Checked = false
  round_checkbox.Checked = false
  round_time_checkbox.Checked = false
  round_time_preset_combo.ItemIndex = 4
  stabilize_checkbox.Checked = true
  lock_checkbox.Checked = true
  mode_reset_checkbox.Checked = false
  transition_guard_checkbox.Checked = true
  transition_guard_edit.Text = tostring(TRANSITION_GUARD_PAUSE_MS)
  stage_autodisable_checkbox.Checked = true
  update_mode_description_from_text()
  update_round_time_description_from_text()
  showMessage("Preset Applied: Round Safe\nMode/HP/Infinite Round OFF, Continuous Lock ON")
end

attach_button.OnClick = function()
  if not ensure_rpcs3_attached() then
    status_label.Caption = "Status: Failed to attach (start RPCS3 first)"
    showMessage("Could not attach to rpcs3.exe / rpcs3-avx2.exe")
    return
  end

  status_label.Caption = "Status: Attached"
  update_pointer()
end

refresh_button.OnClick = function()
  if not ensure_rpcs3_attached() then
    status_label.Caption = "Status: Failed to attach"
    return
  end

  update_pointer()
end

apply_button.OnClick = function()
  if not ensure_rpcs3_attached() then
    showMessage("RPCS3 Process is not available")
    return
  end

  if not update_pointer() then
    showMessage("Battle Pointer is 0. Enter splash demo once then try again.")
    return
  end

  local p1 = CHARACTER_IDS[p1_combo.ItemIndex + 1]
  local p2 = CHARACTER_IDS[p2_combo.ItemIndex + 1]
  local st = STAGE_IDS[stage_combo.ItemIndex + 1]

  if p1 == nil or p2 == nil or st == nil then
    showMessage("Invalid Selection")
    return
  end

  local options = {
    write_mode = mode_checkbox.Checked,
    write_hp = hp_checkbox.Checked,
    write_round = round_checkbox.Checked,
    write_round_time = round_time_checkbox.Checked,
    write_p1_state = p1_state_checkbox.Checked,
    write_p2_state = p2_state_checkbox.Checked,
    mode_reset_pulse = mode_reset_checkbox.Checked,
    transition_guard = transition_guard_checkbox.Checked,
    transition_guard_pause_ms = TRANSITION_GUARD_PAUSE_MS,
    auto_disable_stage_lock = stage_autodisable_checkbox.Checked,
    mode_value = 0,
    hp_value = 0,
    round_value = 0,
    round_time_value = 0
  }

  if options.write_mode then
    local value, err = parse_u32_input(mode_edit.Text)
    if value == nil then
      showMessage("Invalid Game Mode Value: " .. err)
      return
    end
    options.mode_value = value
  end

  if options.write_hp then
    local value, err = parse_u32_input(hp_edit.Text)
    if value == nil then
      showMessage("Invalid HP Bar Value: " .. err)
      return
    end
    options.hp_value = value
  end

  if options.write_round then
    local value, err = parse_u32_input(round_edit.Text)
    if value == nil then
      showMessage("Invalid Infinite Round Value: " .. err)
      return
    end
    options.round_value = value
  end

  if options.write_round_time then
    local value, err = parse_u32_input(round_time_edit.Text)
    if value == nil then
      showMessage("Invalid Round Timer Value: " .. err)
      return
    end
    options.round_time_value = value * ROUND_TIMER_TICKS_PER_SECOND
  end

  if options.transition_guard then
    local value, err = parse_u32_input(transition_guard_edit.Text)
    if value == nil then
      showMessage("Invalid Transition Pause ms: " .. err)
      return
    end
    options.transition_guard_pause_ms = value
  end

  local checks = {}
  local all_ok = true

  all_ok = write_and_verify_u32(cached_battle_pointer + OFF_P1_ID, p1.id, "P1 id", checks) and all_ok
  all_ok = write_and_verify_u32(cached_battle_pointer + OFF_P2_ID, p2.id, "P2 id", checks) and all_ok
  all_ok = write_and_verify_u32(cached_battle_pointer + OFF_STAGE_ID, st.id, "Stage id", checks) and all_ok

  if options.write_mode then
    if options.mode_reset_pulse and options.mode_value ~= 4 then
      write_u32_be(cached_battle_pointer + OFF_GAME_MODE, 4)
      sleep(MODE_RESET_PULSE_MS)
    end
    all_ok = write_and_verify_u32(cached_battle_pointer + OFF_GAME_MODE, options.mode_value, "Game Mode", checks) and all_ok
  end

  if options.write_hp then
    all_ok = write_and_verify_u32(cached_battle_pointer + OFF_HP_BAR, options.hp_value, "HP Bar", checks) and all_ok
  end

  if options.write_round then
    all_ok = write_and_verify_u32(cached_battle_pointer + OFF_INFINITE_ROUND, options.round_value, "Infinite Round", checks) and all_ok
  end

  if options.write_round_time then
    all_ok = write_and_verify_u32(cached_battle_pointer + OFF_ROUND_TIMER, options.round_time_value, "Round Timer", checks) and all_ok
  end

  if options.write_p1_state then
    all_ok = write_and_verify_u32(ADDR_P1_STATE, 0, "P1 State", checks) and all_ok
  end

  if options.write_p2_state then
    all_ok = write_and_verify_u32(ADDR_P2_STATE, 1, "P2 State", checks) and all_ok
  end

  if not all_ok then
    status_label.Caption = "Status: write/verify failures"
    showMessage("One or more values failed verification. Check process, version (01.05), and timing.")
    return
  end

  local result_prefix = "Applied and verified"
  status_label.Caption = "Status: Applied and verified"

  runtime_context = {
    p1 = p1,
    p2 = p2,
    st = st,
    options = options
  }

  runtime_transition_pause_until_ms = 0
  runtime_last_round_timer = nil
  runtime_stage_lock_disabled = false

  runtime_stabilize_cycles_left = stabilize_checkbox.Checked and STABILIZE_CYCLES or 0
  runtime_lock_enabled = lock_checkbox.Checked
  runtime_timer.Interval = LOCK_INTERVAL_MS
  runtime_timer.Enabled = (runtime_stabilize_cycles_left > 0) or runtime_lock_enabled

  if stabilize_checkbox.Checked then
    checks[#checks + 1] = string.format("Stabilizer: ON (%d cycles)", STABILIZE_CYCLES)
  end

  if runtime_lock_enabled then
    checks[#checks + 1] = "Continuous lock: ON (character/stage/state, stage auto-disable supported)"
  end

  if options.transition_guard then
    checks[#checks + 1] = string.format("Transition guard: ON (%d ms)", options.transition_guard_pause_ms)
  end

  if options.write_mode and options.mode_reset_pulse and options.mode_value ~= 4 then
    checks[#checks + 1] = string.format("Mode reset pulse: ON (4 -> %d)", options.mode_value)
  end

  showMessage(string.format(
    "%s.\nP1=%s, P2=%s, Stage=%s\n\n%s",
    result_prefix,
    p1.label,
    p2.label,
    st.label,
    table.concat(checks, "\n")
  ))
end

preset_stable_button.OnClick = function()
  apply_preset_stable_practice()
end

preset_charonly_button.OnClick = function()
  apply_preset_character_stage_only()
end

preset_roundsafe_button.OnClick = function()
  apply_preset_round_safe()
end

mode_preset_combo.OnChange = function()
  local idx = mode_preset_combo.ItemIndex
  if idx >= 0 then
    local entry = GAME_MODE_KNOWN[idx + 1]
    if entry ~= nil then
      mode_edit.Text = tostring(entry.value)
      update_mode_description_from_text()
    end
  end
end

mode_edit.OnChange = function()
  update_mode_description_from_text()
end

round_time_preset_combo.OnChange = function()
  local idx = round_time_preset_combo.ItemIndex
  if idx >= 0 then
    apply_round_time_preset(idx)
  end
end

round_edit.OnChange = function()
  update_round_time_description_from_text()
end

round_time_edit.OnChange = function()
  update_round_time_description_from_text()
end

read_live_button.OnClick = function()
  if read_live_values_into_ui() then
    status_label.Caption = "Status: live values read"
  else
    status_label.Caption = "Status: failed to read live values"
    showMessage("Failed to read live values. Ensure RPCS3 is running and battle pointer is resolved.")
  end
end

advanced_read_button.OnClick = function()
  if not read_advanced_values_into_ui() then
    showMessage("Failed to read advanced values. Ensure game version 01.05 is running.")
  end
end

advanced_apply_button.OnClick = function()
  if not apply_advanced_values() then
    showMessage("One or more checked advanced values could not be applied.")
  end
end

stop_lock_button.OnClick = function()
  stop_runtime_writes(true)
end

close_button.OnClick = function()
  stop_runtime_writes(false)
  form.close()
end

form.Position = "poScreenCenter"
update_mode_description_from_text()
update_round_time_description_from_text()
form.show()
