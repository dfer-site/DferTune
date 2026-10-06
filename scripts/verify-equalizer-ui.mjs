#!/usr/bin/env node

import fs from 'node:fs';

const gui = fs.readFileSync('overlay/source/gui_main.cpp', 'utf8');
const browser = fs.readFileSync('overlay/source/gui_browser.cpp', 'utf8');
const playlist = fs.readFileSync('overlay/source/gui_playlist.cpp', 'utf8');
const tuner = fs.readFileSync('overlay/source/elm_equalizer.cpp', 'utf8');
const tunerHeader = fs.readFileSync('overlay/source/elm_equalizer.hpp', 'utf8');
const commands = fs.readFileSync('ipc/ipc_cmd.h', 'utf8');
const tuneHeader = fs.readFileSync('ipc/tune.h', 'utf8');
const codec = fs.readFileSync('DferTune/source/impl/codec_equalizer.cpp', 'utf8');
const player = fs.readFileSync('DferTune/source/impl/music_player.cpp', 'utf8');
const makefile = fs.readFileSync('Makefile', 'utf8');
const tesla = fs.readFileSync('overlay/lib/libryazhahand/libtesla/include/tesla.hpp', 'utf8');

const failures = [];

const handler = gui.match(
  /bool EqualizerGui::handleInput[\s\S]*?\n}\n\n\/\/ =============================================================================/,
)?.[0] ?? '';

if (!handler) {
  failures.push('EqualizerGui::handleInput was not found');
} else if (/KEY_LEFT[\s\S]*?(goBack|swapTo|changeTo)/.test(handler)) {
  failures.push('LEFT must not navigate away from the EQ editor');
} else if (!/keysDown & \(KEY_LEFT \| KEY_RIGHT\)[\s\S]*?return true/.test(handler)) {
  failures.push('EQ page must consume horizontal keys instead of treating LEFT as page-back');
}

for (const command of ['TuneIpcCmd_GetEqualizerSettings', 'TuneIpcCmd_SetEqualizerSettings']) {
  if (!commands.includes(command))
    failures.push(`missing IPC command: ${command}`);
}

if (!/export API_VERSION\s*:?=\s*8/.test(makefile))
  failures.push('root API_VERSION must be 8');

const modernKeys = [
  '5-band EQ', 'Equalizer', 'Target', 'Game/System', 'Preset', 'Flat', 'Bass',
  'Vocal', 'Rock', 'Bright', 'Custom', 'Reset all', 'Sound', 'Live processing',
  'Live tuner', 'Changes apply instantly', 'A edit/B done · L/R band · ↑/↓ gain',
  'Music DSP · Game/System output', 'Choose source', 'Current game',
  'Per-title controls', 'Playback', 'Rules and language', 'Startup', 'System events',
  '100 Hz', '300 Hz', '1 kHz', '3 kHz', '10 kHz',
];
for (const file of fs.readdirSync('overlay/lang').filter(name => name.endsWith('.json'))) {
  const translations = JSON.parse(fs.readFileSync(`overlay/lang/${file}`, 'utf8'));
  for (const key of modernKeys) {
    if (typeof translations[key] !== 'string' || translations[key].trim() === '')
      failures.push(`${file}: missing modern UI translation for ${JSON.stringify(key)}`);
  }
}

for (const token of ['CompactListItem', 'CompactToggleListItem', 'CompactCategoryHeader',
                     'EqualizerTuner', 'setBandGain',
                     'TUNE_EQUALIZER_TARGET_MUSIC', 'TUNE_EQUALIZER_TARGET_SYSTEM']) {
  if (!gui.includes(token))
    failures.push(`missing compact EQ control: ${token}`);
}

if (gui.includes('m_band_sliders') || gui.includes('m_band_items') || gui.includes('eqText'))
  failures.push('legacy per-row or RU/EN-only EQ UI is still present');

for (const token of ['BandCount = 5', 'TrackTop', 'TrackBottom', 'handleTuningInput',
                     'onTouch', 'requestGain', 'Changes apply instantly']) {
  if (!tuner.includes(token) && !tunerHeader.includes(token) && !gui.includes(token))
    failures.push(`five-fader live tuner contract missing: ${token}`);
}

if (!/direction\s*=\s*\(keysHeld\s*&\s*KEY_UP\)[\s\S]*?KEY_DOWN/.test(tuner)
    || /direction\s*=\s*\(keysHeld\s*&\s*KEY_RIGHT\)/.test(tuner))
  failures.push('vertical fader gain must be controlled by UP/DOWN, not LEFT/RIGHT');
if (!/if \(!m_editing\)[\s\S]*?KEY_A[\s\S]*?m_editing = true/.test(tuner)
    || !/keysDown & \(KEY_A \| KEY_B\)[\s\S]*?m_editing = false/.test(tuner))
  failures.push('tuner must release UP/DOWN for list navigation outside explicit edit mode');

for (const [name, source] of [['settings', gui], ['browser', browser], ['playlist', playlist]]) {
  if (/new tsl::elm::(?:ListItem|ToggleListItem|CategoryHeader|SilentListItem)\b/.test(source))
    failures.push(`${name}: legacy full-size menu constructor remains`);
}

if (!/typedef struct \{\s*u8 enabled;\s*u8 target;\s*s8 gains_db\[TUNE_EQUALIZER_BAND_COUNT\];\s*u8 reserved;\s*\} TuneEqualizerSettings;/.test(tuneHeader))
  failures.push('equalizer IPC snapshot must keep its explicit 8-byte v8 layout');
for (const token of ['I2cDevice_Alc5639', 'RegEqControl1', 'RegEqControl2',
                     'CoefficientSlots', 'NeutralGainCoefficient',
                     'ReferenceGainCoefficient']) {
  if (!codec.includes(token))
    failures.push(`missing independent codec EQ component: ${token}`);
}
if (!/target == TUNE_EQUALIZER_TARGET_MUSIC/.test(player)
    || !/codec_equalizer::Apply\(system_enabled/.test(player)
    || !/g_equalizer\.SetSettings\(software\)/.test(player))
  failures.push('music/system targets are not routed to separate DSP backends');

const inputHandler = (source, className) => source.match(
  new RegExp(`bool ${className}::handleInput[\\s\\S]*?\\n}\\n`),
)?.[0] ?? '';
if (/KEY_LEFT[\s\S]*?goBack/.test(inputHandler(gui, 'LanguageGui'))
    || /KEY_LEFT[\s\S]*?goBack/.test(inputHandler(gui, 'StartupSettingsGui')))
  failures.push('child settings pages must use B, not LEFT, for back navigation');
if (/const bool goLeft[\s\S]{0,180}KEY_LEFT/.test(browser)
    || /const bool goLeft[\s\S]{0,180}KEY_LEFT/.test(playlist))
  failures.push('browser/playlist LEFT input still conflicts with back navigation');
if (!/SettingsGui::handleInput[\s\S]*?HidNpadButton_B[\s\S]*?swapTo<MainGui>/.test(gui))
  failures.push('Settings B must return to the player instead of closing the overlay');

if (!tesla.includes('blockShoulderJump')
    || !/blockShoulderJump\.store\(tunerFocused/.test(gui))
  failures.push('focused tuner must suppress Tesla global L/R edge jumps');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Verified vertical five-band live tuner, IPC v8, and isolated navigation.');
