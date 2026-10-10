// ═══════════════════════════════════════════════════════════════════════════
//  check_panel_cmd.js — [4.7.1 PANEL-CMD] komendy zdalne z panelu v14 (Etap 4, tryb zdalny).
//  Regresja: panel wysyłał komendy na legacy /aquarium/cmd.json, a firmware (v246) czyta
//  WYŁĄCZNIE kolejkę /aquarium/commands. Sprawdza:
//    1) sendFireCmd: POST na /aquarium/commands.json, payload {cmd, token, ts(number)};
//    2) każda nazwa komendy użyta w panelu jest obsługiwana przez firmware
//       (dyspozytor wykonajKomendeFirebase), plus prefiks "pwm ";
//    3) firmware nadal czyta /aquarium/commands i nie czyta /aquarium/cmd.
//  Uruchomienie: node firmware/tests/host/check_panel_cmd.js  (z korzenia repo)
// ═══════════════════════════════════════════════════════════════════════════
'use strict';
const fs = require('fs');
const path = require('path');

const root = path.join(__dirname, '../../..');
const panel = fs.readFileSync(path.join(root, 'panel/akwarium-firebase-panel-v14.html'), 'utf8');
const fw = fs.readFileSync(path.join(root, 'firmware/src/Ryby_LED_fi_S3.cpp'), 'utf8');

let pass = 0, fail = 0;
function check(cond, name) {
  if (cond) pass++; else { fail++; console.log('FAIL: ' + name); }
}

// ── 1) Wyciągnięcie sendFireCmd + fbPost z panelu ──────────────────────────
const sStart = panel.indexOf('async function fbPost(');
const sEnd = panel.indexOf('async function sendPwmCmd');
check(sStart > 0 && sEnd > sStart, 'znaleziono fbPost i sendFireCmd w panelu');
const block = panel.slice(sStart, sEnd);

const calls = [];
const fakeFetch = async (url, opts) => {
  calls.push({ url, opts });
  return { ok: true, json: async () => ({ name: '-Nfake' }) };
};
const toasts = [];
const factory = new Function(
  'fetch', 'DB_URL', 'DB_SECRET', 'CMD_TOKEN', 'toast', 'gid', 'Date',
  block + '\nreturn { sendFireCmd, fbPost };'
);
const FakeDate = { now: () => 1791619200123 };
const { sendFireCmd } = factory(
  fakeFetch,
  'https://db.example.app',
  'SEKRET-TESTOWY',
  'TOKEN-TESTOWY',
  (msg, kind) => toasts.push([msg, kind]),
  () => ({ classList: { add() {}, remove() {} } }),
  FakeDate
);

(async () => {
  await sendFireCmd('power_on');
  check(calls.length === 1, 'sendFireCmd wykonuje dokładnie jedno żądanie');
  const c = calls[0] || { url: '', opts: {} };
  check(c.url === 'https://db.example.app/aquarium/commands.json?auth=SEKRET-TESTOWY',
        'ścieżka: /aquarium/commands.json (jest: ' + c.url + ')');
  check(c.url.indexOf('/aquarium/cmd.json') < 0, 'brak legacy /aquarium/cmd.json');
  check(c.opts.method === 'POST', 'metoda POST (push z kluczem czasowym)');
  let body = {};
  try { body = JSON.parse(c.opts.body); } catch (e) { body = {}; }
  check(body.cmd === 'power_on', 'payload.cmd = power_on');
  check(body.token === 'TOKEN-TESTOWY', 'payload.token = CMD_TOKEN');
  check(typeof body.ts === 'number' && body.ts === 1791619200123, 'payload.ts = liczba (ms), rosnąca');
  check(toasts.length === 1 && toasts[0][1] === 'ok', 'po wysłaniu toast OK');

  // ── 2) Nazwy komend z panelu muszą być obsługiwane przez firmware ───────
  const panelCmds = new Set();
  for (const m of panel.matchAll(/sendFireCmd\(\s*['"]([a-z_0-9]+)['"]\s*\)/g)) panelCmds.add(m[1]);
  for (const m of panel.matchAll(/sendFireCmd\(\s*this\.checked\s*\?\s*'([a-z_0-9]+)'\s*:\s*'([a-z_0-9]+)'/g)) {
    panelCmds.add(m[1]); panelCmds.add(m[2]);
  }
  check(panelCmds.size >= 8, 'panel używa co najmniej 8 różnych komend (jest ' + panelCmds.size + ')');

  const fwCmds = new Set();
  for (const m of fw.matchAll(/cmd == "([a-z_0-9]+)"/g)) fwCmds.add(m[1]);
  const hasPwm = /cmd\.startsWith\("pwm "\)/.test(fw);
  for (const cmd of panelCmds) {
    if (cmd === 'pwm') continue;
    check(fwCmds.has(cmd), 'firmware obsługuje komendę z panelu: ' + cmd);
  }
  check(hasPwm, 'firmware obsługuje "pwm ..." (sendPwmCmd)');
  check(/sendFireCmd\(\s*"pwm "/.test(panel) || /sendFireCmd\("pwm "\+/.test(panel),
        'panel wysyła pwm z prefiksem "pwm "');

  // ── 3) Firmware: czyta kolejkę /aquarium/commands, nie czyta /aquarium/cmd ──
  check(fw.includes('fbDatabase.get(fbAsyncClient, "/aquarium/commands"'),
        'firmware czyta /aquarium/commands');
  const readsLegacy = /fbDatabase\.get\([^)]*"\/aquarium\/cmd"/.test(fw);
  check(!readsLegacy, 'firmware nie czyta legacy /aquarium/cmd');

  // ── 4) AUTO (4.7.2): panel wysyła "autosave a b c d e", firmware je obsługuje ──
  check(fw.includes('cmd.startsWith("autosave ")'), 'firmware parsuje komendę "autosave "');
  check(/case AppCommandType::AUTO_SAVE_PWM:[\s\S]*?saveAutoBrightnessToEEPROM\(\)/.test(fw),
        'firmware: AUTO_SAVE_PWM zapisuje do EEPROM (jak /api/save-auto)');
  check(!/configType:\s*"autoSave"/.test(panel), 'panel nie wysyła już configType:"autoSave"');
  const sa0 = panel.indexOf('function saveToAutoFb(');
  const saBlock = panel.slice(sa0, panel.indexOf('\n}\n', sa0) + 3);
  const sentAuto = [];
  const saFactory = new Function('gid', 'confirm', 'sendFireCmd', 'fbPatch', saBlock + '\nreturn saveToAutoFb;');
  const saveToAutoFb = saFactory(
    (id) => ({ value: String(10 + Number(id.slice(2))) }),
    () => true,
    async (cmd) => { sentAuto.push(cmd); },
    async () => { throw new Error('fbPatch nie powinien być wołany'); }
  );
  await saveToAutoFb();
  check(sentAuto.length === 1 && sentAuto[0] === 'autosave 10 11 12 13 14',
        'saveToAutoFb → sendFireCmd("autosave 10 11 12 13 14") (jest: ' + JSON.stringify(sentAuto) + ')');

  // ── 5) Symulacja LUX: w firmware jej nie ma → panel nic nie wysyła ───────
  const lsStart = panel.indexOf('function luxSimUnsupported(');
  const lsEnd = panel.indexOf('// ── [v14] TRYB POŁĄCZENIA');
  const lsBlock = panel.slice(lsStart, lsEnd);
  const lsSent = [];
  const lsFactory = new Function('fbPatch', 'toast', 'gid', 'CMD_TOKEN', 'Date',
    lsBlock + '\nreturn { luxSimSet, luxSimSaveConst, luxSimSaveAuto };');
  const ls = lsFactory(
    async (path, body) => { lsSent.push(body); },
    () => {},
    () => ({ value: '300' }),
    'T', FakeDate
  );
  ls.luxSimSet('auto'); ls.luxSimSaveConst(); ls.luxSimSaveAuto();
  check(lsSent.length === 0, 'symulacja LUX: brak wysyłki do Firebase (jest ' + lsSent.length + ')');

  // ── 6) Wszystkie configType z panelu są parsowane przez firmware ─────────
  const fwTypes = new Set(['schedule', 'adapt', 'minlux', 'params', 'pump', 'telegram']);
  const panelTypes = new Set();
  for (const m of panel.matchAll(/configType\s*:\s*["']([A-Za-z]+)["']/g)) panelTypes.add(m[1]);
  // luxSim celowo pominięty: zapisy są zablokowane strażnikiem (sekcja 5), więc nie trafiają do firmware
  panelTypes.delete('luxSim');
  for (const t of panelTypes) {
    check(fwTypes.has(t), 'firmware parsuje configType "' + t + '" z panelu');
  }

  // ── 7) Zakresy minLuxTarget: panel = firmware (500..8000) ────────────────
  check(/id="cfg-minLuxTarget"\s+min="500"\s+max="8000"/.test(panel), 'panel: slider minLuxTarget min=500 max=8000');
  check(/target<500\|\|target>8000/.test(panel), 'panel: walidacja minLuxTarget 500..8000');
  check(/extractFloat\("minLuxTarget", f\) && f >= 500\.0f && f <= 8000\.0f/.test(fw), 'firmware: minLuxTarget 500..8000');

  console.log('panel cmd: ' + pass + ' PASS, ' + fail + ' FAIL');
  process.exit(fail === 0 ? 0 : 1);
})();
