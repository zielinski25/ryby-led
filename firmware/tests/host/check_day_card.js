// ═══════════════════════════════════════════════════════════════════════════
//  check_day_card.js — [4.7.0 ASTRO] karta „Dzień” w panelu (Etap 6 pkt 3).
//  Wyciąga z terminal_html.cpp funkcje karty (dayPad, dayHHMM, dayMin,
//  drawDayCurve) i uruchamia je na atrapie DOM. Sprawdza: tekst harmonogramu,
//  okno rampy wieczornej, „dziś” po przejściu przez północ, znaczniki
//  MIN LUX / adaptacji / pompki, brak pól w starym firmware (domyślne wartości).
//  Uruchomienie: node firmware/tests/host/check_day_card.js
// ═══════════════════════════════════════════════════════════════════════════
'use strict';
const fs = require('fs');
const path = require('path');

const src = fs.readFileSync(path.join(__dirname, '../../src/terminal_html.cpp'), 'utf8');
const a = src.indexOf('R"RAWHTML(') + 'R"RAWHTML('.length;
const b = src.indexOf(')RAWHTML"', a);
const html = src.slice(a, b);
const start = html.indexOf('// ── KARTA „DZIEŃ”');
const end = html.indexOf('function loadCharts() {');
if (start < 0 || end < 0) { console.error('FAIL: nie znaleziono bloku karty Dzień'); process.exit(1); }
const block = html.slice(start, end);

// Atrapa DOM: elementy z id; tylko to, czego używa drawDayCurve.
const els = {};
function mkEl(id) {
  return { id, parentElement: { clientWidth: 340 }, innerHTML: '', textContent: '',
           setAttribute(k, v) { this[k] = v; } };
}
els['chart-day'] = mkEl('chart-day');
els['day-info'] = mkEl('day-info');
const document = { getElementById: (id) => els[id] || null };

// Uruchomienie bloku w zakresie z atrapą document; zwraca funkcje do testów.
const factory = new Function('document', block + '\nreturn { drawDayCurve, dayHHMM, dayMin };');
const { drawDayCurve, dayHHMM, dayMin } = factory(document);

let pass = 0, fail = 0;
function check(cond, name) {
  if (cond) pass++; else { fail++; console.log('FAIL: ' + name); }
}

// Historia: „wczoraj” 23:50 → „dziś” 00:00..06:00 co 5 min. Po przejściu przez północ
// obowiązują tylko próbki dzisiejsze (od 00:00).
function row(hhmm, pwm, ml, ad) {
  const c = [hhmm, '20.0', '19.0', '18.0', '120', '90', pwm, pwm, pwm, pwm, pwm, '0', ml ? '1' : '0', ad ? '1' : '0', '200'];
  return c.join(',');
}
const csvLines = [row('23:50', 10, 0, 0), row('23:55', 10, 0, 0)];
for (let m = 0; m <= 360; m += 5) {
  const hh = String(Math.floor(m / 60)).padStart(2, '0');
  const mm = String(m % 60).padStart(2, '0');
  // MIN LUX w 02:00–02:30, adaptacja w 04:00–04:15.
  csvLines.push(row(hh + ':' + mm, 50, m >= 120 && m < 150, m >= 240 && m < 255));
}
const csv = csvLines.join('\n');

const status = {
  localTime: '09:30:00',
  pumpSlots: [{ start: 600, end: 660 }],
  schedule: {
    morningWD: 360, morningWE: 480, middayOff: 780,
    eveningBefore: -60, eveningOff: 1320, sunsetMin: 1079,  // zachód 17:59
  },
};

els['chart-day'].innerHTML = '';
drawDayCurve(status, csv);
const info = els['day-info'].textContent;
const svg = els['chart-day'].innerHTML;

check(dayHHMM(1079) === '17:59', 'dayHHMM zachodu');
check(dayHHMM(-30) === '23:30', 'dayHHMM ujemne minuty zawijają się');
check(dayMin('09:30:00') === 570, 'dayMin z sekundami');
check(isNaN(dayMin('--:--:--')), 'dayMin dla "--:--:--" → NaN');

check(info.includes('Zachód 17:59'), 'info: zachód 17:59 (' + info + ')');
check(info.includes('rampa wiecz. 16:59–22:00'), 'info: okno rampy 16:59–22:00');
// Zależnie od dnia tygodnia: dzień roboczy → 06:00, weekend → 08:00.
check(/poranek (06:00 \(dzień rob\.\)|08:00 \(weekend\))/.test(info), 'info: poranek wg dnia tygodnia (' + info + ')');
check(info.includes('próbek dziś: 73'), 'info: 73 próbki dzisiejsze po przejściu przez północ (' + info + ')');
check(!info.includes('23:50') && !svg.includes('23:50'), 'próbki z poprzedniego dnia odrzucone');

check(svg.includes('<path d="M'), 'krzywa PWM narysowana');
check(svg.includes('stroke="#ffd32a"'), 'linia PWM w kolorze żółtym');
check(svg.includes('fill="rgba(168,85,247,.85)"'), 'pasek MIN LUX obecny');
check(svg.includes('fill="rgba(0,212,245,.55)"'), 'pasek adaptacji obecny');
check(svg.includes('fill="rgba(0,212,245,.7)"'), 'pasek pompki obecny');
check(svg.includes('fill="rgba(255,159,67,.10)"'), 'tło rampy wieczornej obecne');
check(svg.includes('zach. 17:59'), 'etykieta zachodu');
check(svg.includes('>teraz<'), 'etykieta „teraz” (localTime 09:30)');
check(svg.includes('>00:00<') && svg.includes('>24:00<'), 'osie 00:00 i 24:00');

// Przejście przez północ w oknie rampy: rampa 23:00 → 01:00 rysuje dwa segmenty.
els['chart-day'].innerHTML = '';
drawDayCurve({ localTime: '--:--:--', schedule: { sunsetMin: 1140, eveningBefore: -60, eveningOff: 60 } }, csv);
const wrapSvg = els['chart-day'].innerHTML;
const wrapRects = (wrapSvg.match(/fill="rgba\(255,159,67,\.10\)"/g) || []).length;
check(wrapRects === 2, 'rampa przez północ: 2 segmenty (jest ' + wrapRects + ')');
check(!wrapSvg.includes('>teraz<'), 'brak localTime → brak znacznika „teraz”');

// Stary firmware bez pól schedule: domyślne 19:00 zachód, bez poranka, bez crasha.
els['chart-day'].innerHTML = '';
drawDayCurve({}, csv);
check(els['day-info'].textContent.includes('Zachód 19:00'), 'brak schedule → zachód domyślnie 19:00');
check(els['chart-day'].innerHTML.includes('<path'), 'brak schedule → krzywa nadal rysowana');

// Pusta historia: bez próbek, bez krzywej, bez wyjątku.
els['chart-day'].innerHTML = '';
drawDayCurve(status, '');
check(els['day-info'].textContent.includes('próbek dziś: 0'), 'pusta historia → 0 próbek');
check(!els['chart-day'].innerHTML.includes('<path'), 'pusta historia → brak krzywej');

console.log('day card: ' + pass + ' PASS, ' + fail + ' FAIL');
process.exit(fail === 0 ? 0 : 1);
