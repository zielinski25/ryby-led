#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════
#  Testy hostowe Spool V2 (Etap 3, v4.2.0) — bez ESP32, bez PlatformIO.
#
#  1) test_spool.cpp kompiluje PRAWDZIWE źródło telemetry_spool.cpp
#     z atrapami Arduino/LittleFS/esp_system (stubs/) + AddressSanitizer
#     i UBSan. Sprawdza: CRC, ring SPSC, format strony, commit, uszkodzone
#     rekordy, ucięte strony, replay, kasowanie, polityki spill, *.part.
#  2) check_integration.sh: wyciąga blok [4.2.0 SPOOL] i endpoint
#     /api/telemetry/status z Ryby_LED_fi_S3.cpp i kompiluje je z atrapami
#     globali Ryby + API FirebaseClient (sygnatury z v2.2.13).
#
#  3) test_histlong.cpp: historia długa 30 min (4.5.0): kubełki, CRC-less
#     uśrednianie, rotacja, strumieniowanie (kursor), timeout. ASan + UBSan.
#  4) check_integration.sh: blok [4.2.0 SPOOL] + endpoint /api/telemetry/status.
#  5) check_integration_e5.sh: blok CRIT-LOG (Etap 5, 4.4.0) i helper OTA.
#  6) check_integration_hl.sh: endpoint /api/history/long, hak saveHistoryPoint,
#     AsyncURIMatcher::exact dla /api/history*.
#  7) check_tg_menu.py: menu Telegrama (JSON, PL_CAP, callbacki, 1 klik do raportu).
#  8) test_astro.cpp: efemerydy NOAA (wschód/zachód/świt/zmierzch) vs astral 3.2.
#  9) test_rtc.cpp: sterownik RTC DS1307/DS3231 (BCD, CH, OSF, epoka) — fałszywa magistrala.
# 10) check_location.sh: lokalizacja, poranek i RTC z panelu (NVS, /api/location, /api/rtc).
# 11) check_day_card.js: karta „Dzień” w panelu (krzywa doby, harmonogram).
#
#  0) check_secrets.py: brak sekretów zaszytych w repo (4.7.1, Etap 2).
# 12) check_panel_cmd.js: komendy zdalne panelu trafiają do kolejki firmware (4.7.1).
# 13) check_mobile_app.js: aplikacja mobilna panel/ryby-mobile.html (logika, komendy, pola statusu, jeden plik).
# 14) check_android_app.js: natywna aplikacja Android (android/, Kotlin): kontrakt z firmware, bez kompilacji.
#
#  Uruchomienie:  bash firmware/tests/host/run_tests.sh
#  Wymaga: g++ (C++17), python3. Nie wymaga sieci.
# ═══════════════════════════════════════════════════════════════════
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="${SRC:-$HERE/../../src}"
BIN="${TMPDIR:-/tmp}/tspool-test-bin"
export TSPOOL_FS_ROOT="${TSPOOL_FS_ROOT:-${TMPDIR:-/tmp}/tspool-fakefs}"

echo "== [0] skan sekretów w śledzonych plikach (4.7.1, Etap 2) =="
python3 "$HERE/check_secrets.py"
echo
echo "== [1/6] testy logiki Spool V2 (ASan + UBSan) =="
g++ -std=gnu++17 -g -O1 -Wall -Wextra \
    -fsanitize=address,undefined -fno-sanitize-recover=undefined \
    -I"$HERE/stubs" -I"$SRC" \
    "$HERE/test_spool.cpp" "$SRC/telemetry_spool.cpp" \
    -o "$BIN"
"$BIN"

echo
echo "== [2/6] testy modułu critlog — log krytyczny (4.4.0, ASan + UBSan) =="
g++ -std=gnu++17 -g -O1 -Wall -Wextra \
    -fsanitize=address,undefined -fno-sanitize-recover=undefined \
    -I"$HERE/stubs" -I"$SRC" \
    "$HERE/test_critlog.cpp" "$SRC/critlog.cpp" \
    -o "${BIN}-critlog"
TSPOOL_FS_ROOT="${TSPOOL_FS_ROOT}-critlog" "${BIN}-critlog"

echo
echo "== [3/6] testy modułu histlong — historia długa (4.5.0, ASan + UBSan) =="
g++ -std=gnu++17 -g -O1 -Wall -Wextra \
    -fsanitize=address,undefined -fno-sanitize-recover=undefined \
    -I"$HERE/stubs" -I"$SRC" \
    "$HERE/test_histlong.cpp" "$SRC/histlong.cpp" \
    -o "${BIN}-histlong"
TSPOOL_FS_ROOT="${TSPOOL_FS_ROOT}-histlong" "${BIN}-histlong"

echo
echo "== [4/6] kompilacja bloku integracyjnego z Ryby_LED_fi_S3.cpp =="
bash "$HERE/check_integration.sh"
echo
echo "== [5/6] kompilacja bloku CRIT-LOG (Etap 5) + helper OTA =="
bash "$HERE/check_integration_e5.sh"
echo
echo "== [6/6] kompilacja bloku HIST-LONG (4.5.0) + AsyncURIMatcher::exact =="
bash "$HERE/check_integration_hl.sh"
echo
echo
echo "== [7/7] menu Telegrama: JSON, PL_CAP, callbacki, raporty w 1 klik (4.6.0) =="
python3 "$HERE/check_tg_menu.py"
echo
echo
echo "== [8/13] testy modułu astro — efemerydy słoneczne (4.7.0, ASan + UBSan) =="
g++ -std=gnu++17 -g -O1 -Wall -Wextra \
    -fsanitize=address,undefined -fno-sanitize-recover=undefined \
    -I"$SRC" \
    "$HERE/test_astro.cpp" "$SRC/astro.cpp" \
    -o "${BIN}-astro"
"${BIN}-astro"
echo
echo
echo "== [9/13] sterownik RTC DS1307/DS3231 (4.7.0, ASan + UBSan) =="
g++ -std=gnu++17 -g -O1 -Wall -Wextra \
    -fsanitize=address,undefined -fno-sanitize-recover=undefined \
    -I"$SRC" \
    "$HERE/test_rtc.cpp" "$SRC/rtc_ds13xx.cpp" \
    -o "${BIN}-rtc"
"${BIN}-rtc"
echo
echo
echo "== [10/13] lokalizacja, poranek i RTC z panelu: NVS, /api/location, /api/rtc (4.7.0) =="
bash "$HERE/check_location.sh"
echo
echo
echo "== [11/13] karta Dzień w panelu (4.7.0, Node) =="
if command -v node >/dev/null 2>&1; then
  node "$HERE/check_day_card.js"
else
  echo "POMINIĘTO: brak node (wymagany do check_day_card.js)"
fi
echo
echo "== [12/13] komendy zdalne panelu v14 → kolejka /aquarium/commands (4.7.1, Node) =="
node "$HERE/check_panel_cmd.js"
echo
echo "== [13/13] aplikacja mobilna panel/ryby-mobile.html: logika, kontrakt z firmware (4.7.2, Node) =="
node "$HERE/check_mobile_app.js"
echo
echo
echo "== [14/14] aplikacja Android android/: kontrakt z firmware i HTML (4.7.2, Node, statycznie) =="
node "$HERE/check_android_app.js"
echo
echo "OK: wszystkie kontrole hostowe zaliczone."
