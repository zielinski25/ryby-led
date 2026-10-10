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
#  Uruchomienie:  bash firmware/tests/host/run_tests.sh
#  Wymaga: g++ (C++17), python3. Nie wymaga sieci.
# ═══════════════════════════════════════════════════════════════════
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="${SRC:-$HERE/../../src}"
BIN="${TMPDIR:-/tmp}/tspool-test-bin"
export TSPOOL_FS_ROOT="${TSPOOL_FS_ROOT:-${TMPDIR:-/tmp}/tspool-fakefs}"

echo "== [1/2] testy logiki Spool V2 (ASan + UBSan) =="
g++ -std=gnu++17 -g -O1 -Wall -Wextra \
    -fsanitize=address,undefined -fno-sanitize-recover=undefined \
    -I"$HERE/stubs" -I"$SRC" \
    "$HERE/test_spool.cpp" "$SRC/telemetry_spool.cpp" \
    -o "$BIN"
"$BIN"

echo
echo "== [2/2] kompilacja bloku integracyjnego z Ryby_LED_fi_S3.cpp =="
bash "$HERE/check_integration.sh"
echo
echo "OK: wszystkie kontrole hostowe zaliczone."
