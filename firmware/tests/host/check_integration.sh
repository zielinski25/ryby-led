#!/usr/bin/env bash
# Kompiluje (-fsyntax-only) blok [4.2.0 SPOOL] i endpoint /api/telemetry/status
# wyciągnięte z Ryby_LED_fi_S3.cpp — przeciwko atrapom ryby_mock.h.
# Łapie literówki, złe typy i błędy sygnatur, których nie widać bez toolchainu ESP32.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="${SRC:-$HERE/../../src}"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

python3 - "$SRC/Ryby_LED_fi_S3.cpp" "$WORK" <<'PY'
import sys, pathlib
src = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8")
out = pathlib.Path(sys.argv[2])
# Blok: od nagłówka sekcji SPOOL do fbAsyncProcessResult (włącznie z funkcjami glue).
start_marker = "//  [v4.2.0 SPOOL] Etap 3 planu upgrade"
i = src.index(start_marker)
i = src.rfind("// ═══", 0, i)
end = src.index("static void fbAsyncProcessResult() {")
(out / "glue.inc").write_text(src[i:end], encoding="utf-8")
# Endpoint diagnostyczny.
a = src.index('webserialServer.on("/api/telemetry/status"')
b = src.index("});", a) + 3
(out / "endpoint.inc").write_text(src[a:b], encoding="utf-8")
PY

cat > "$WORK/check.cpp" <<EOF
#include "ryby_mock.h"
#include "telemetry_spool.h"
#include "glue.inc"
struct AsyncWebServerRequest { void send(int, const char*, const char*) {} };
struct WebSerialMock {
  template <typename F> void on(const char*, int, F&& f) { AsyncWebServerRequest r; f(&r); }
} webserialServer_obj;
#define HTTP_GET 1
#define webserialServer webserialServer_obj
void registerTelemetryEndpoints() {
#include "endpoint.inc"
}
void useAll() { tsBoot(); tsSpillTick(); fbAsyncStartTelemetryReplay(); fbAsyncHandleTelemetryResult(); }
EOF

g++ -std=gnu++17 -fsyntax-only -Wall -Wextra -Wno-unused-function \
    -I"$HERE/stubs" -I"$HERE" -I"$SRC" -I"$WORK" "$WORK/check.cpp"
echo "OK: blok SPOOL + endpoint /api/telemetry/status kompilują się (syntax-only)."
