#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════════
#  [4.7.0 ASTRO] Etap 6 — lokalizacja, poranek i RTC z panelu.
#  Wyciąga z Ryby_LED_fi_S3.cpp:
#    - zmienne (ASTRO_LOC_NVS_NS, latitude/longitude, tryb poranka, rtcChip),
#    - funkcje NVS, poranka (morningStartFor, przeliczAstroDzis), zachodu i RTC
#      (rtcglue, rtcRestoreAtBoot, rtcSaveFromSystem),
#    - endpointy GET/POST /api/location oraz GET/POST /api/rtc (blok przed /api/fs-list),
#  kompiluje je z atrapami loc_mock.h + astro.cpp + rtc_ds13xx.cpp i uruchamia
#  test_location.cpp. Nie zastępuje testu na płytce (docs/06, sekcja 5).
# ═══════════════════════════════════════════════════════════════════════════
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="${SRC:-$HERE/../../src}"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

python3 - "$SRC/Ryby_LED_fi_S3.cpp" "$WORK/loc_extract.inc" <<'PY'
import sys, pathlib
ryby = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8")
out = pathlib.Path(sys.argv[2])

# 1) Zmienne (od ASTRO_LOC_NVS_NS do rtcSaveRequested).
a = ryby.index('static constexpr const char* ASTRO_LOC_NVS_NS')
b = ryby.index('volatile bool rtcSaveRequested = false;', a)
b = ryby.index('\n', b) + 1
globals_src = ryby[a:b]

# 2) Funkcje NVS, poranka, zachodu, RTC (do loadMinLuxModeFromEEPROM).
a = ryby.index('// [4.7.0 ASTRO] Ustawienia astro/RTC — NVS')
b = ryby.index('void loadMinLuxModeFromEEPROM() {', a)
funcs_src = ryby[a:b]

# 3) Endpointy: od nagłówka sekcji do początku /api/fs-list.
a = ryby.index('//  API: LOKALIZACJA, PORANEK I RTC')
a = ryby.rindex('// ═══', 0, a)
b = ryby.index('webserialServer.on("/api/fs-list"', a)
endpoints_src = ryby[a:b]

# Sprawdzenia wpięcia w setup()/loop() i w miejsca poranka — bez nich test byłby pusty.
for needle in [
    'loadSiteLocation();  // [4.7.0 ASTRO] lokalizacja z NVS',
    '(nowMs - lastSunsetRecalc >= recalcInterval || sunsetRecalcRequested) && timeSynced',
    'sunsetRecalcRequested = false;',
    'latitude, longitude',
    'rtcRestoreAtBoot(); }',
    'rtcSaveFromSystem();   // [4.7.0 ASTRO] NTP OK',
    'przeliczAstroDzis();   // [4.7.0 ASTRO] świt/wschód na dziś po NTP',
    'morningStartFor(isWeekend())',
    '\\"morningEff\\"',
]:
    assert needle in ryby, "brak wpięcia: " + needle

out.write_text(
    globals_src + "\n" + funcs_src +
    "void registerLocation() {\n" + endpoints_src + "}\n",
    encoding="utf-8")
print("extract OK")
PY

g++ -std=gnu++17 -g -O1 -Wall -Wextra -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter \
    -fsanitize=address,undefined -fno-sanitize-recover=undefined \
    -I"$HERE/stubs" -I"$HERE" -I"$SRC" -I"$WORK" \
    "$HERE/test_location.cpp" "$SRC/astro.cpp" "$SRC/rtc_ds13xx.cpp" -o "$WORK/test_location"

# Endpointy muszą być zarejestrowane przed testem (registerLocation wywołane w teście).
"$WORK/test_location"
