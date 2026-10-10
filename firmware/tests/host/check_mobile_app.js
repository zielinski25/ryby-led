#!/usr/bin/env node
// check_mobile_app.js — aplikacja mobilna panel/mobile (4.7.2+build.272). Bez zależności, bez przeglądarki.
//
// Sprawdza:
//  1) logikę app-logic.js (komendy, ts, PWM, normalizacja statusu, stan połączenia, formatowanie);
//  2) kontrakt z firmware: każda komenda z aplikacji istnieje w dispatcherze firmware;
//  3) kontrakt statusu: pola czytane przez aplikację są zapisywane przez firmware;
//  4) bezpieczeństwo UI: brak innerHTML/eval, brak sekretów w kodzie, PWA-pliki na miejscu.
//
// Uruchomienie: node firmware/tests/host/check_mobile_app.js

const fs = require("fs");
const path = require("path");
const assert = require("assert");

const ROOT = path.resolve(__dirname, "..", "..", "..");
const APP = path.join(ROOT, "panel", "mobile");
const FW = fs.readFileSync(path.join(ROOT, "firmware", "src", "Ryby_LED_fi_S3.cpp"), "utf8");
const HTML = fs.readFileSync(path.join(APP, "index.html"), "utf8");
const L = require(path.join(APP, "app-logic.js"));

let pass = 0, fail = 0;
function check(cond, label) {
  if (cond) { pass++; console.log("PASS: " + label); }
  else { fail++; console.log("FAIL: " + label); }
}
function eq(actual, expected, label) {
  let ok = false;
  try { assert.deepStrictEqual(actual, expected); ok = true; } catch (_) { ok = false; }
  check(ok, label + (ok ? "" : " (jest " + JSON.stringify(actual) + ", oczekiwano " + JSON.stringify(expected) + ")"));
}
function throwsMsg(fn, re, label) {
  let msg = null;
  try { fn(); } catch (e) { msg = e.message; }
  check(msg !== null && re.test(msg), label + (msg === null ? " (brak wyjątku)" : " (" + msg + ")"));
}

// ───── 1) komendy i ts ─────
eq(L.commandText("power_on"), "power_on", "komenda bez argumentów");
eq(L.commandText({ type: "pwm", values: [10, 20, 30, 40, 50] }), "pwm 10 20 30 40 50", "pwm: 5 wartości");
eq(L.commandText({ type: "autosave", values: [1, 2, 3, 4, 5] }), "autosave 1 2 3 4 5", "autosave: 5 wartości");
eq(L.commandText({ type: "pwm", values: [-5, 2000, "x", 7.6, null] }), "pwm 0 1023 0 8 0", "pwm: zakres 0–1023, śmieci → 0, zaokrąglenie");
throwsMsg(() => L.commandText("format_disk"), /nieznana komenda/, "nieznana komenda odrzucona");
throwsMsg(() => L.commandText({ type: "pwm", values: [1, 2] }), /5 wartości/, "pwm z 2 wartościami odrzucony");
throwsMsg(() => L.commandText({ type: "x" }), /nieznana akcja/, "nieznany typ akcji odrzucony");

eq(L.nextTs(1000, 0), 1000, "ts = czas gdy brak poprzedniego");
eq(L.nextTs(1000, 5000), 5001, "ts rośnie nawet gdy zegar stoi (firmware odrzuca STALE)");
eq(L.nextTs(9000, 5000), 9000, "ts = czas gdy zegar przed poprzednim wysłanym");

const body = L.buildCommand("led_100", "TOK", 1234);
eq(body, { cmd: "led_100", token: "TOK", ts: 1234 }, "body zgodne z kontraktem {cmd,token,ts}");
throwsMsg(() => L.buildCommand("led_100", "", 1), /CMD_TOKEN/, "brak tokenu → błąd (nic nie wysyłamy)");
throwsMsg(() => L.buildCommand("led_100", "T", 0), /ts musi być dodatni/, "ts=0 → błąd");

// ───── 2) normalizacja statusu ─────
const raw = {
  mode: "AUTO", power: true, pumpOn: false, temps: [24.5, "25.1", null],
  luxRoom: 320, luxNadWoda: 410.4, pwm: [100, 200, 2000, -1, 50], pct: 40,
  powerNowW: 12.3, powerLimitW: 60, powerNowCh: [1, 2, 3, 4, 5],
  energyTodayWh: 1234.5, energyWeekWh: 5000, energyMonthWh: 20000,
  ledOnMinutesToday: 125, ledOnMinutesWeek: 600, ledOnMinutesMonth: 2400,
  kwhPrice: 0.9, dayHistory: [1, 2, 3], hourPowerWh: [0.5, 1.5],
  pumpSlots: [{ start: 480, end: 540 }], pumpSlotCount: 1,
  schedule: { morningWD: 420, morningWE: 480, middayOff: 780, eveningBefore: -30, eveningOff: 1260 },
  fadeMinutes: 30, rampSec: 60, minLuxEnabled: true, minLuxActive: false, minLuxTarget: 1000,
  inLightingWindow: true, isNight: false, rampActive: false, adaptEnabled: true,
  wifi_rssi: -58, uptime: 90061, updatedAt: 777000
};
const s = L.normalizeStatus(raw);
eq(s.mode, "AUTO", "mode AUTO");
eq(s.power, true, "power true");
eq(s.temps, [24.5, 25.1, null], "temps: string → liczba, null zostaje null");
eq(s.pwm, [100, 200, 1023, 0, 50], "pwm: zakres 0–1023 po normalizacji");
eq(s.luxRoom, 320, "luxRoom z pola luxRoom");
eq(s.energy, { today: 1234.5, week: 5000, month: 20000 }, "energia z pól *Wh");
eq(s.ledMin.today, 125, "czas LED dziś");
eq(s.pumpSlots, [{ start: 480, end: 540 }], "przedziały pompy");
eq(s.updatedAt, 777000, "updatedAt zachowany (millis z ESP)");

const empty = L.normalizeStatus({});
eq(empty.mode, null, "pusty status: mode null");
eq(empty.pwm, [0, 0, 0, 0, 0], "pusty status: pwm bezpieczne zera");
eq(empty.temps, [null, null, null], "pusty status: temps null");
eq(empty.energy, { today: 0, week: 0, month: 0 }, "pusty status: energia 0");
eq(empty.pumpSlots, [], "pusty status: brak przedziałów");
eq(L.normalizeStatus(null).power, false, "null zamiast statusu nie wywala normalizacji");

const legacy = L.normalizeStatus({ lux: 111 });
eq(legacy.luxRoom, 111, "starszy firmware bez luxRoom: używany lux");

// ───── 3) stan połączenia (updatedAt się nie zmienia = ESP nie pisze) ─────
let tr = L.trackUpdate(null, 100, 1000);
eq(tr, { updatedAt: 100, changedAt: 1000 }, "pierwszy odczyt ustawia changedAt");
tr = L.trackUpdate(tr, 100, 50000);
eq(tr.changedAt, 1000, "ten sam updatedAt nie przesuwa changedAt");
tr = L.trackUpdate(tr, 160, 61000);
eq(tr.changedAt, 61000, "nowy updatedAt przesuwa changedAt");
eq(L.connectionState(tr, 61000 + L.STALE_MS), "online", "dokładnie próg → nadal online");
eq(L.connectionState(tr, 61000 + L.STALE_MS + 1), "stale", "po progu → opóźnione");
eq(L.connectionState(null, 5), "none", "brak trackera → none");
eq(L.connectionState({ updatedAt: null, changedAt: null }, 5), "none", "brak danych → none");
check(L.STALE_MS >= 120000 && L.STALE_MS <= 180000, "próg opóźnienia ~2.5× okres zapisu statusu (60 s)");

// ───── 4) formatowanie ─────
eq(L.fmtTemp(24.56), "24.6 °C", "temperatura 1 miejsce po przecinku");
eq(L.fmtTemp(null), "—", "brak temperatury → —");
eq(L.fmtWh(850), "850 Wh", "Wh poniżej 1000");
eq(L.fmtWh(1500), "1.50 kWh", "Wh powyżej 1000 → kWh");
eq(L.fmtMinutes(125), "2 h 05 min", "minuty → h/min");
eq(L.fmtMinutes(45), "45 min", "minuty poniżej godziny");
eq(L.fmtHHMM(480), "08:00", "HH:MM z minut");
eq(L.fmtHHMM(1439), "23:59", "HH:MM koniec doby");
eq(L.fmtHHMM(1440), "—", "HH:MM poza zakresem → —");
eq(L.fmtUptime(90061), "1 d 1 h", "uptime dni+godziny");
eq(L.fmtUptime(754), "12 min", "uptime minuty");
eq(L.costPln(1500, 0.9), 1.35, "koszt: 1.5 kWh × 0.90 zł");
eq(L.costPln(1500, null), null, "koszt bez ceny z ESP → null");
eq(L.pwmToPct(1023), 100, "PWM 1023 = 100%");
eq(L.pwmToPct(null), 0, "PWM brak = 0%");

// ───── 5) kontrakt z firmware: komendy ─────
for (const cmd of L.PLAIN_CMDS) {
  const re = new RegExp('cmd\\s*==\\s*"' + cmd + '"|cmd\\s*==\\s*"[a-z_]+"\\s*\\|\\|\\s*cmd\\s*==\\s*"' + cmd + '"');
  check(re.test(FW), 'firmware obsługuje komendę "' + cmd + '"');
}
check(FW.includes('cmd.startsWith("pwm ")'), 'firmware obsługuje "pwm a b c d e"');
check(FW.includes('cmd.startsWith("autosave ")'), 'firmware obsługuje "autosave a b c d e"');
check(FW.includes('"/aquarium/commands"') || FW.includes('/aquarium/commands'), 'firmware czyta kolejkę /aquarium/commands');

// Każdy data-cmd w HTML musi być na liście PLAIN_CMDS (lub to pwm/autosave z JS)
const dataCmds = [...HTML.matchAll(/data-cmd="([a-z_0-9]+)"/g)].map((m) => m[1]);
check(dataCmds.length >= 7, 'UI ma przyciski komend (' + dataCmds.length + ')');
for (const c of dataCmds) check(L.PLAIN_CMDS.includes(c), 'przycisk data-cmd="' + c + '" jest na liście komend');

// ───── 6) kontrakt z firmware: pola statusu ─────
const statusFields = [
  "mode", "power", "pumpOn", "temps", "luxRoom", "luxNadWoda", "pwm", "pct", "autoPWM",
  "powerNowW", "powerLimitW", "peakPowerWToday", "energyTodayWh", "energyWeekWh", "energyMonthWh",
  "ledOnMinutesToday", "ledOnMinutesWeek", "ledOnMinutesMonth", "kwhPrice", "dayHistory",
  "hourPowerWh", "pumpSlots", "schedule", "fadeMinutes", "rampSec", "minLuxEnabled", "minLuxActive",
  "minLuxTarget", "inLightingWindow", "rampActive", "adaptEnabled", "wifi_rssi", "uptime", "updatedAt"
];
for (const f of statusFields) {
  check(FW.includes('\\"' + f + '\\":') || FW.includes('"' + f + '\\":') || FW.includes('\\"' + f + '\\"'),
        'firmware zapisuje pole statusu "' + f + '"');
}

// ───── 7) bezpieczeństwo i PWA ─────
const appLogic = fs.readFileSync(path.join(APP, "app-logic.js"), "utf8");
check(!/\beval\s*\(|new Function\s*\(/.test(HTML + appLogic), "brak eval / new Function");
check(!/\.innerHTML\s*=/.test(HTML), "brak innerHTML (dane z bazy tylko przez textContent)");
check(!/AkwPanel|RhKVp49q|Akwarium2026|8709162940:AA/.test(HTML + appLogic), "brak znanych sekretów w aplikacji");
check(/name="viewport"[^>]*width=device-width/.test(HTML), "meta viewport (telefon)");
check(/rel="manifest" href="manifest\.webmanifest"/.test(HTML), "link do manifestu PWA");
check(/serviceWorker\.register\("sw\.js"\)/.test(HTML), "rejestracja service workera");
for (const f of ["manifest.webmanifest", "icon.svg", "sw.js", "app-logic.js", "index.html"]) {
  check(fs.existsSync(path.join(APP, f)), "plik aplikacji istnieje: " + f);
}
const manifest = JSON.parse(fs.readFileSync(path.join(APP, "manifest.webmanifest"), "utf8"));
check(manifest.display === "standalone" && manifest.start_url, "manifest: standalone + start_url");
check(/min-height:\s*4[4-9]px|min-height:\s*5\dpx/.test(HTML), "przyciski dotykowe ≥44 px");

console.log("\nmobile app: " + pass + " PASS, " + fail + " FAIL");
process.exit(fail === 0 ? 0 : 1);
