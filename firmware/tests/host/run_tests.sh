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
#
#  Uruchomienie:  bash firmware/tests/host/run_tests.sh
#  Wymaga: g++ (C++17), python3. Nie wymaga sieci.
# ═══════════════════════════════════════════════════════════════════
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="${SRC:-$HERE/../../src}"
BIN="${TMPDIR:-/tmp}/tspool-test-bin"
export TSPOOL_FS_ROOT="${TSPOOL_FS_ROOT:-${TMPDIR:-/tmp}/tspool-fakefs}"

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
echo "OK: wszystkie kontrole hostowe zaliczone."
