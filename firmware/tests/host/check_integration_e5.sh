#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════════
#  Kompiluje (-fsyntax-only) blok [4.4.0 CRIT-LOG] (Etap 5) wyciągnięty z
#  Ryby_LED_fi_S3.cpp oraz helper otaCritLine z ota_github.cpp — przeciwko
#  atrapom e5_mock.h i PRAWDZIWEMU critlog.h/critlog.cpp.
#  Łapie literówki, złe typy, brakujące deklaracje, błędne sygnatury
#  (logCritical, sendTelegramCriticalDocument, endpointy HTTP).
#  Nie zastępuje testu na płytce.
# ═══════════════════════════════════════════════════════════════════════════
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="${SRC:-$HERE/../../src}"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

python3 - "$SRC/Ryby_LED_fi_S3.cpp" "$SRC/ota_github.cpp" "$WORK" <<'PY'
import sys, pathlib
ryby = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8")
ota  = pathlib.Path(sys.argv[2]).read_text(encoding="utf-8")
out  = pathlib.Path(sys.argv[3])

# 1) logCritical + sendTelegramCriticalDocument (do logDailySummary).
a = ryby.index("// [4.4.0 CRIT-LOG] Etap 5 planu upgrade")
b = ryby.index("void logDailySummary() {", a)
(out / "core.inc").write_text(ryby[a:b], encoding="utf-8")

# 2) Endpointy /api/log-critical-* (do /api/fs-list).
a = ryby.index('webserialServer.on("/api/log-critical-status"')
# [4.7.0 ASTRO] sekcja lokalizacji stoi przed /api/fs-list — kończymy przed nią.
b = ryby.index('//  API: LOKALIZACJA DO ZACHODU', a)
(out / "endpoints.inc").write_text(ryby[a:b], encoding="utf-8")

# 3) Helper otaCritLine z ota_github.cpp.
a = ota.index("static void otaCritLine(const char* msg) {")
b = ota.index("\n}\n", a) + 3
(out / "ota_helper.inc").write_text(ota[a:b], encoding="utf-8")

# 4) Wywołania muszą istnieć w plikach źródłowych (kontrola spójności wpięcia).
for needle in ['logCritical("TSL"', 'critlog::init(littlefsReady);', 'logCritical("BOOT"',
               'logCritical("DS-DIAG"', 'case TG_LOG_CRITICAL:', 'sendTelegramCriticalDocument();',
               '"critlog"', '"ota"', '/log_krytyczny']:
    assert needle in ryby, "brak wpięcia: " + needle
for needle in ['otaCritLine("Task OTA', 'otaCritLine(b.c_str())', 'otaCritLine("esp_ota_mark']:
    assert needle in ota, "brak wpięcia OTA: " + needle
print("extract OK")
PY

cat > "$WORK/check.cpp" <<EOF
#include "e5_mock.h"
#include "critlog.h"
#include "core.inc"
void registerE5Endpoints() {
#include "endpoints.inc"
}
#include "ota_helper.inc"
void useE5() {
  logCritical("TEST", "msg");
  bool b = sendTelegramCriticalDocument();
  (void)b;
  otaCritLine("x");
  registerE5Endpoints();
}
EOF

g++ -std=gnu++17 -fsyntax-only -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -I"$HERE/stubs" -I"$HERE" -I"$SRC" -I"$WORK" "$WORK/check.cpp"
echo "OK: blok CRIT-LOG (logCritical, Telegram, endpointy, helper OTA) kompiluje się (syntax-only)."
