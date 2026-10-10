#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════════
#  Kompiluje (-fsyntax-only) blok [4.5.0 HIST-LONG] (Etap 3, G8) wyciągnięty z
#  Ryby_LED_fi_S3.cpp: endpoint /api/history/long oraz hak w saveHistoryPoint()
#  — przeciwko atrapom hl_mock.h i PRAWDZIWEMU histlong.h.
#  Dodatkowo pilnuje, że trasy /api/history* są dopasowywane przez
#  AsyncURIMatcher::exact (goły string = prefiks → kolizja z /api/history/*).
#  Nie zastępuje testu na płytce (docs/05, sekcja 5).
# ═══════════════════════════════════════════════════════════════════════════
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="${SRC:-$HERE/../../src}"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

python3 - "$SRC/Ryby_LED_fi_S3.cpp" "$WORK" <<'PY'
import sys, pathlib, re
ryby = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8")
out  = pathlib.Path(sys.argv[2])

# 1) Endpoint /api/history/long (do bloku log-critical).
a = ryby.index('webserialServer.on(AsyncURIMatcher::exact("/api/history/long")')
b = ryby.index('// [4.4.0 CRIT-LOG] Etap 5 — log krytyczny: status', a)
(out / "endpoint.inc").write_text(ryby[a:b], encoding="utf-8")

# 2) Hak w saveHistoryPoint() — od komentarza do domykającego nawiasu bloku.
a = ryby.index('// [4.5.0 HIST-LONG] Etap 3 (G8) — długa historia')
m = ryby.index('histlong::addSample((uint32_t)_hlNow, _hs);', a)
b = ryby.index('\n  }\n', m) + len('\n  }\n')
(out / "hook.inc").write_text(ryby[a:b], encoding="utf-8")

# 3) Wpięcia: init, clear w handlerze, kotwice exact, brak gołych prefiksów.
needles = [
  'histlong::init(littlefsReady);',
  'if (littlefsReady) histlong::clear();',
  'webserialServer.on(AsyncURIMatcher::exact("/api/history"), HTTP_GET',
  'webserialServer.on(AsyncURIMatcher::exact("/api/history/clear"), HTTP_POST',
  '#include "histlong.h"',
  '"v4.5.0+build.267"',
]
for n in needles:
    assert n in ryby, "brak wpięcia: " + n
bare = re.findall(r'webserialServer\.on\("(/api/history[^"]*)"', ryby)
assert not bare, "gołe (prefiksowe) trasy /api/history: %r" % bare
print("extract OK")
PY

cat > "$WORK/check.cpp" <<'CPP'
#include "hl_mock.h"
#include "histlong.h"
void registerHl() {
#include "endpoint.inc"
}
void hookHl() {
  int pwmPct[5] = {0, 0, 0, 0, 0};
  float totalPowerW = 0.0f;
  (void)pwmPct; (void)totalPowerW;
#include "hook.inc"
}
CPP

g++ -std=gnu++17 -fsyntax-only -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -I"$HERE/stubs" -I"$HERE" -I"$SRC" -I"$WORK" "$WORK/check.cpp"
echo "OK: blok HIST-LONG (endpoint /api/history/long, hak saveHistoryPoint, AsyncURIMatcher::exact) kompiluje się (syntax-only)."
