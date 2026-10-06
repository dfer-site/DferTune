#!/usr/bin/env node

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const scriptDir = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(scriptDir, '..');

const read = (relativePath) => fs.readFileSync(path.join(root, relativePath), 'utf8');
const sources = {
  configHeader: read('common/config/config.hpp'),
  configSource: read('common/config/config.cpp'),
  player: read('DferTune/source/impl/music_player.cpp'),
  gui: read('overlay/source/gui_main.cpp'),
  stringsHeader: read('overlay/source/strings.hpp'),
  stringsSource: read('overlay/source/strings.cpp'),
  ipcCommands: read('ipc/ipc_cmd.h'),
  ipcHeader: read('ipc/tune.h'),
  ipcClient: read('ipc/tune.c'),
  service: read('DferTune/source/tune_service.cpp'),
  overlayMakefile: read('overlay/Makefile'),
};

const checks = [
  ['boot gate configuration', sources.configHeader, 'get_wait_for_home'],
  ['boot gate implementation', sources.player, 'g_awaiting_home'],
  ['qlaunch boot detection', sources.player, 'pmdmntGetProcessId(&qlaunch_pid, kHomeScreenTid)'],
  ['keyboard configuration', sources.configHeader, 'get_pause_on_keyboard'],
  ['keyboard applet detection', sources.player, 'AppletId_LibraryAppletSwkbd'],
  ['controller-sync configuration', sources.configHeader, 'get_pause_on_controller_sync'],
  ['controller-sync persistence', sources.configSource, '"pause_on_controller_sync"'],
  ['controller-sync applet detection', sources.player, 'AppletId_LibraryAppletController'],
  ['controller-sync UI toggle', sources.gui, 'PauseOnControllerSync'],
  ['controller-sync UI string', sources.stringsHeader, 'PauseOnControllerSync'],
  ['controller-sync fallback string', sources.stringsSource, 'Pause On Controller Sync'],
  ['lock-screen configuration', sources.configHeader, 'get_pause_on_lockscreen'],
  ['lock-screen power detection', sources.player, 'PdmPlayEventType_PowerStateChange'],
  ['overlap-safe context gating', sources.player, 'refreshContextPause'],
  ['manual-pause preservation', sources.player, 'g_user_paused'],
  ['dedicated startup settings GUI', sources.gui, 'StartupSettingsGui::createUI'],
  ['startup settings navigation', sources.gui, 'tsl::changeTo<StartupSettingsGui>()'],
  ['startup policy snapshot type', sources.configHeader, 'struct StartupPolicy'],
  ['startup IPC command', sources.ipcCommands, 'TuneIpcCmd_SetStartupPolicy'],
  ['startup IPC public API', sources.ipcHeader, 'tuneSetStartupPolicy'],
  ['startup IPC client dispatch', sources.ipcClient, 'TuneIpcCmd_SetStartupPolicy'],
  ['startup IPC server dispatch', sources.service, 'impl::SetStartupPolicy'],
  ['runtime policy implementation', sources.player, 'void SetStartupPolicy'],
  ['runtime boot-gate cancellation', sources.player, '!config::get_auto_play_startup()'],
  ['shared libryazhahand UI namespace', sources.overlayMakefile, 'UI_OVERRIDE_PATH := /config/ryazhahand/'],
  ['atomic live pause flags', sources.player, 'std::atomic<bool> g_should_pause'],
  ['atomic live run flag', sources.player, 'std::atomic<bool> g_should_run'],
  ['stable queue path snapshot', sources.player, 'Result rc = PlayTrack(current_path);'],
];

const failures = checks
  .filter(([, source, expected]) => !source.includes(expected))
  .map(([name, , expected]) => `${name}: missing ${JSON.stringify(expected)}`);

const settingsRegion = sources.gui.slice(
  sources.gui.indexOf('SettingsGui::createUI()'),
  sources.gui.indexOf('SettingsGui::update()'),
);
for (const obsoleteDirectSetter of [
  'config::set_auto_play_startup',
  'config::set_wait_for_home',
  'config::set_pause_on_keyboard',
  'config::set_pause_on_controller_sync',
  'config::set_pause_on_lockscreen',
]) {
  if (settingsRegion.includes(obsoleteDirectSetter))
    failures.push(`main SettingsGui still writes ${obsoleteDirectSetter} directly`);
}

for (const [name, source] of [
  ['DferTune config', sources.configHeader],
  ['DferTune config implementation', sources.configSource],
  ['DferTune GUI', sources.gui],
]) {
  if (source.includes('switch_2_style') || source.includes('useSwitch2Style') || source.includes('Switch2Style'))
    failures.push(`${name} still owns shared libryazhahand Switch 2 style`);
}

const presetRegionStart = sources.gui.indexOf('default_title_volume_slider->setValueChangedListener');
const presetRegionEnd = sources.gui.indexOf('m_list->addItem(default_title_volume_slider)', presetRegionStart);
if (presetRegionStart >= 0 && presetRegionEnd > presetRegionStart) {
  const presetRegion = sources.gui.slice(presetRegionStart, presetRegionEnd);
  if (presetRegion.includes('config::set_title_volume'))
    failures.push('preset volume handler still writes a per-title override');
}

if (failures.length > 0) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log(`Verified ${checks.length} implementation contracts for issue #38 and runtime startup settings.`);
