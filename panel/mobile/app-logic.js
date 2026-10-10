/*
 * Ryby LED — aplikacja mobilna: logika bez DOM (czysta, testowalna w Node).
 *
 * Kontrakt z firmware (4.7.2+build.272):
 *   - odczyt:  GET  {DB}/aquarium/status.json?auth=SECRET   (firmware pisze co ~60 s)
 *   - komendy: POST {DB}/aquarium/commands.json?auth=SECRET  body {cmd, token, ts}
 *              firmware czyta kolejkę co ~14 s; ts MUSI rosnąć (inaczej STALE).
 *   - komendy z argumentami: "pwm a b c d e" (0–1023), "autosave a b c d e".
 *
 * Plik nie zna DOM ani sieci. Eksport: window.RybyLogic (przeglądarka) / module.exports (Node).
 */
(function (root) {
  "use strict";

  var DEFAULT_DB_URL = "https://akwarium-367be-default-rtdb.europe-west1.firebasedatabase.app";
  // Status zapisywany co 60 s; po 150 s bez zmiany updatedAt uznajemy, że ESP nie pisze.
  var STALE_MS = 150000;
  var PWM_MAX = 1023;

  // Komendy bez argumentów, które firmware obsługuje (dispatcher wykonajKomendeFirebase).
  var PLAIN_CMDS = [
    "power_on", "power_off", "mode_auto", "mode_manual",
    "pump_on", "pump_off", "led_100", "led_test", "led_off", "restart"
  ];

  function isFiniteNum(x) {
    return typeof x === "number" && isFinite(x);
  }

  function num(x) {
    if (typeof x === "string" && x.trim() !== "") x = Number(x);
    return isFiniteNum(x) ? x : null;
  }

  function clampPwm(v) {
    var n = Math.round(Number(v));
    if (!isFinite(n)) return 0;
    return Math.max(0, Math.min(PWM_MAX, n));
  }

  // 5 kanałów -> "a b c d e" (kolejność jak w firmware)
  function pwmArgs(arr) {
    var out = [];
    for (var i = 0; i < 5; i++) out.push(clampPwm(arr && arr[i]));
    return out.join(" ");
  }

  function pwmToPct(v) {
    var n = num(v);
    if (n === null) return 0;
    return Math.round((Math.max(0, Math.min(PWM_MAX, n)) / PWM_MAX) * 100);
  }

  // ts musi być rosnący względem ostatnio wysłanego (firmware odrzuca STALE)
  function nextTs(nowMs, lastTs) {
    var last = isFiniteNum(lastTs) ? lastTs : 0;
    return Math.max(Math.floor(nowMs), last + 1);
  }

  // Buduje body POST /aquarium/commands.json. Rzuca błąd dla nieznanej komendy lub pustego tokenu.
  function buildCommand(cmd, token, ts) {
    if (typeof cmd !== "string" || cmd === "") throw new Error("pusta komenda");
    if (!token) throw new Error("brak CMD_TOKEN — wpisz go w Ustawieniach");
    if (!isFiniteNum(ts) || ts <= 0) throw new Error("ts musi być dodatni");
    return { cmd: cmd, token: token, ts: ts };
  }

  // Zamienia nazwę akcji z UI na tekst komendy dla firmware.
  // action: string z PLAIN_CMDS, albo {type:"pwm", values:[5]}, albo {type:"autosave", values:[5]}
  function commandText(action) {
    if (typeof action === "string") {
      if (PLAIN_CMDS.indexOf(action) < 0) throw new Error("nieznana komenda: " + action);
      return action;
    }
    if (action && (action.type === "pwm" || action.type === "autosave")) {
      if (!action.values || action.values.length !== 5) throw new Error("wymagane 5 wartości PWM");
      return action.type + " " + pwmArgs(action.values);
    }
    throw new Error("nieznana akcja");
  }

  // Normalizuje surowy JSON /aquarium/status do płaskiego modelu dla UI.
  // Brakujące pola dostają bezpieczne wartości (null / 0 / []) — UI nie może wywalić się na starym firmware.
  function normalizeStatus(d) {
    d = d || {};
    var temps = Array.isArray(d.temps) ? d.temps : [];
    var pwm = Array.isArray(d.pwm) ? d.pwm : [];
    var sch = d.schedule || {};
    var slots = Array.isArray(d.pumpSlots) ? d.pumpSlots : [];
    var dayH = Array.isArray(d.dayHistory) ? d.dayHistory : [];
    var hourH = Array.isArray(d.hourPowerWh) ? d.hourPowerWh : [];
    var weekH = Array.isArray(d.weekHistory) ? d.weekHistory : [];
    var chW = Array.isArray(d.powerNowCh) ? d.powerNowCh : [];
    var luxRoom = num(d.luxRoom);
    if (luxRoom === null) luxRoom = num(d.lux);
    return {
      mode: d.mode === "MANUAL" ? "MANUAL" : (d.mode === "AUTO" ? "AUTO" : null),
      power: d.power === true,
      pumpOn: d.pumpOn === true,
      temps: [num(temps[0]), num(temps[1]), num(temps[2])],
      luxRoom: luxRoom,
      luxNadWoda: num(d.luxNadWoda),
      pwm: [0, 1, 2, 3, 4].map(function (i) { return clampPwm(pwm[i]); }),
      pct: num(d.pct),
      autoPWM: num(d.autoPWM),
      powerNowW: num(d.powerNowW),
      powerLimitW: num(d.powerLimitW),
      powerNowCh: [0, 1, 2, 3, 4].map(function (i) { return num(chW[i]) || 0; }),
      peakPowerWToday: num(d.peakPowerWToday),
      energy: {
        today: num(d.energyTodayWh) || 0,
        week: num(d.energyWeekWh) || 0,
        month: num(d.energyMonthWh) || 0
      },
      ledMin: {
        today: num(d.ledOnMinutesToday) || 0,
        week: num(d.ledOnMinutesWeek) || 0,
        month: num(d.ledOnMinutesMonth) || 0
      },
      kwhPrice: num(d.kwhPrice),
      dayHistory: dayH.map(function (x) { return num(x) || 0; }),
      weekHistory: weekH.map(function (x) { return num(x) || 0; }),
      hourPowerWh: hourH.map(function (x) { return num(x) || 0; }),
      pumpSlots: slots.map(function (s) {
        return { start: num(s && s.start), end: num(s && s.end) };
      }),
      schedule: {
        morningWD: num(sch.morningWD),
        morningWE: num(sch.morningWE),
        middayOff: num(sch.middayOff),
        eveningBefore: num(sch.eveningBefore),
        eveningOff: num(sch.eveningOff)
      },
      fadeMinutes: num(d.fadeMinutes),
      rampSec: num(d.rampSec),
      minLuxEnabled: d.minLuxEnabled === true,
      minLuxActive: d.minLuxActive === true,
      minLuxTarget: num(d.minLuxTarget),
      inLightingWindow: d.inLightingWindow === true,
      isNight: d.isNight === true,
      rampActive: d.rampActive === true,
      adaptEnabled: d.adaptEnabled === true,
      wifiRssi: num(d.wifi_rssi),
      uptimeS: num(d.uptime),
      updatedAt: num(d.updatedAt)
    };
  }

  // Śledzi, kiedy po raz ostatni zmienił się updatedAt (licznik millis z ESP, nie czas unix).
  // tracker = {updatedAt, changedAt} (ms wg zegara telefonu). Zwraca nowy tracker.
  function trackUpdate(tracker, updatedAt, nowMs) {
    var prev = tracker || { updatedAt: null, changedAt: null };
    if (updatedAt === null || updatedAt === undefined) return prev;
    if (prev.updatedAt === null || updatedAt !== prev.updatedAt) {
      return { updatedAt: updatedAt, changedAt: nowMs };
    }
    return prev;
  }

  // "online" = dane świeże; "stale" = dane są, ale ESP nie zapisał od dawna; "none" = brak danych.
  function connectionState(tracker, nowMs) {
    if (!tracker || tracker.changedAt === null) return "none";
    return (nowMs - tracker.changedAt) > STALE_MS ? "stale" : "online";
  }

  function fmtTemp(v) {
    var n = num(v);
    return n === null ? "—" : n.toFixed(1) + " °C";
  }

  function fmtLux(v) {
    var n = num(v);
    return n === null ? "—" : Math.round(n) + " lx";
  }

  function fmtWatts(v) {
    var n = num(v);
    return n === null ? "—" : n.toFixed(1) + " W";
  }

  // Wh -> "123 Wh" / "1.23 kWh"
  function fmtWh(v) {
    var n = num(v);
    if (n === null) return "—";
    if (Math.abs(n) >= 1000) return (n / 1000).toFixed(2) + " kWh";
    return Math.round(n) + " Wh";
  }

  // minuty -> "2 h 05 min"
  function fmtMinutes(v) {
    var n = num(v);
    if (n === null) return "—";
    n = Math.max(0, Math.round(n));
    var h = Math.floor(n / 60), m = n % 60;
    if (h === 0) return m + " min";
    return h + " h " + (m < 10 ? "0" : "") + m + " min";
  }

  // minuty od północy -> "HH:MM"
  function fmtHHMM(minutes) {
    var n = num(minutes);
    if (n === null || n < 0 || n > 1439) return "—";
    var h = Math.floor(n / 60), m = n % 60;
    return (h < 10 ? "0" : "") + h + ":" + (m < 10 ? "0" : "") + m;
  }

  // Koszt energii w PLN (cena z ESP; brak ceny -> null)
  function costPln(wh, pricePerKwh) {
    var w = num(wh), p = num(pricePerKwh);
    if (w === null || p === null) return null;
    return Math.round((w / 1000) * p * 100) / 100;
  }

  function fmtPln(v) {
    return v === null || v === undefined ? "—" : v.toFixed(2) + " zł";
  }

  // Uptime sekundy -> "3 d 4 h" / "12 min"
  function fmtUptime(seconds) {
    var s = num(seconds);
    if (s === null) return "—";
    s = Math.max(0, Math.floor(s));
    var d = Math.floor(s / 86400), h = Math.floor((s % 86400) / 3600), m = Math.floor((s % 3600) / 60);
    if (d > 0) return d + " d " + h + " h";
    if (h > 0) return h + " h " + m + " min";
    return m + " min";
  }

  var api = {
    DEFAULT_DB_URL: DEFAULT_DB_URL,
    STALE_MS: STALE_MS,
    PLAIN_CMDS: PLAIN_CMDS,
    clampPwm: clampPwm,
    pwmArgs: pwmArgs,
    pwmToPct: pwmToPct,
    nextTs: nextTs,
    buildCommand: buildCommand,
    commandText: commandText,
    normalizeStatus: normalizeStatus,
    trackUpdate: trackUpdate,
    connectionState: connectionState,
    fmtTemp: fmtTemp,
    fmtLux: fmtLux,
    fmtWatts: fmtWatts,
    fmtWh: fmtWh,
    fmtMinutes: fmtMinutes,
    fmtHHMM: fmtHHMM,
    costPln: costPln,
    fmtPln: fmtPln,
    fmtUptime: fmtUptime
  };

  if (typeof module !== "undefined" && module.exports) module.exports = api;
  root.RybyLogic = api;
})(typeof window !== "undefined" ? window : globalThis);
