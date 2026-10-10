#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════════
#  Kompiluje (-fsyntax-only) blok [4.5.0 HIST-LONG] (Etap 3, G8) wyciągnięty z
#  Ryby_LED_fi_S3.cpp: endpoint /api/history/long oraz hak w saveHistoryPoint()
#  — przeciwko atrapom hl_mock.h i PRAWDZIWEMU histlong.h.
#  Dodatkowo pilnuje, że trasy /api/history* są dopasowywane przez
#  AsyncURIMatcher::exact (goły string = prefiks → kolizja z /api/history/*).
#  [4.7.0] Sprawdza też obliczZachodSlonca(): wyciągnięte z Ryby ciało
#  kompiluje się z astro.cpp i trafia w zachód z astral (UTC, ±2 min).
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
  '#define RYBY_FW_VERSION "v',
]
for n in needles:
    assert n in ryby, "brak wpięcia: " + n
bare = re.findall(r'webserialServer\.on\("(/api/history[^"]*)"', ryby)
assert not bare, "gołe (prefiksowe) trasy /api/history: %r" % bare

# 4) [4.7.0 ASTRO] obliczZachodSlonca — definicja (nie deklaracja) do '\n}\n'.
for n in ['#include "astro.h"', 'astro::compute(rok, miesiac, dzien, szerokosc, dlugosc, 0, t)']:
    assert n in ryby, "brak wpięcia astro: " + n
a = ryby.index('int obliczZachodSlonca(int rok, int miesiac, int dzien, float szerokosc, float dlugosc) {')
b = ryby.index('\n}\n', a) + len('\n}\n')
(out / "sunset.inc").write_text(ryby[a:b], encoding="utf-8")
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

# Zachód słońca: prawdziwe ciało obliczZachodSlonca + astro.cpp, wynik vs astral 3.2.
cat > "$WORK/sunset_check.cpp" <<'CPP'
#include "astro.h"
#include <math.h>
#include <stdio.h>
#include "sunset.inc"

// Wartości UTC zachodu z astral 3.2 dla lokalizacji z Ryby (52,1345 / 20,1418).
int main() {
  struct { int y, m, d; double utcRef; } cases[] = {
    {2026, 1, 5, 882.18}, {2026, 3, 25, 1020.80}, {2026, 6, 25, 1144.03},
    {2026, 10, 10, 956.22}, {2026, 12, 15, 866.85},
  };
  int bad = 0;
  for (auto& c : cases) {
    int got = obliczZachodSlonca(c.y, c.m, c.d, 52.1345f, 20.1418f);
    if (fabs((double)got - c.utcRef) > 2.0) {
      printf("FAIL zachod %04d-%02d-%02d: %d vs %.2f\n", c.y, c.m, c.d, got, c.utcRef);
      bad++;
    }
  }
  if (obliczZachodSlonca(2026, 6, 21, 69.6496f, 18.9560f) != -1) { printf("FAIL polar -1\n"); bad++; }
  return bad == 0 ? 0 : 1;
}
CPP

g++ -std=gnu++17 -Wall -Wextra -Wno-unused-function -I"$SRC" -I"$WORK" \
    "$SRC/astro.cpp" "$WORK/sunset_check.cpp" -o "$WORK/sunset_check"
"$WORK/sunset_check"
echo "OK: obliczZachodSlonca (astro::compute) zgodne z astral ±2 min."

echo "OK: blok HIST-LONG (endpoint /api/history/long, hak saveHistoryPoint, AsyncURIMatcher::exact) kompiluje się (syntax-only)."
