#!/usr/bin/env python3
"""Host tests against actual snippets from the patched main_centrala.cpp.

Usage: python test_centrala_zew.py path/to/main_centrala.cpp
Requires a C++11 compiler. These are not ESP32, integration or hardware tests.
"""
import json
import pathlib
import subprocess
import sys
import tempfile

source = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8")
def between(start, end):
    return source[source.index(start):source.index(end, source.index(start))]

# Include the actual struct, static ABI checks, age helper and JSON serializer.
diag = between('static portMUX_TYPE g_tempDiagMux', '\nfloat cisnienieOdczytane =')
layout = between('struct TelemetrySample {', '\nstatic void telemetryFormatRecordId')
serializer = between('static void telemetryJsonAppend(', '\nstatic bool telemetryBuildBatch(')
sensors = between('void cisnienie() {', '\n// ════════════════════════════════════════════════════════\n// MIESZADŁO')
for required in [
    'out.tDhtDiagX10 = encodeTempDiag(czytajTempDiag(), -40.0f, 80.0f)',
    'out.tempDiagReserved = 0;',
    'out.qualityBits |= TEMP_DIAG_MARK;',
    'ja("\\\"t_zewn_2\\\":" + jsonTempDiag()',
    'dhtErr == DHT_OK && isfinite(dhtTempPomijana)',
]:
    assert required in source, required

header = r'''
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
using std::isfinite;
using std::isnan;
using portMUX_TYPE = int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x) ((void)(x))
#define portEXIT_CRITICAL(x) ((void)(x))
static int64_t clockUs = 0;
int64_t esp_timer_get_time() { return clockUs; }
struct String {
    std::string v;
    String(const char* s) : v(s) {}
    String(float x, int precision) { char b[64]; snprintf(b,sizeof(b),"%.*f",precision,x); v=b; }
};
constexpr uint8_t TELEMETRY_CH_COUNT = 11;
'''
sensor_stubs = r'''
unsigned long millis() { return (unsigned long)(clockUs / 1000); }
uint64_t telemetryMonotonicMs() { return (uint64_t)(clockUs / 1000); }
enum { TELEMETRY_CH_PRESSURE=1, TELEMETRY_CH_RH=2, DHT_OK=0, DHT_PIN=6, DHT22=22 };
void telemetryMarkAccepted(int, uint64_t, bool) {}
void telemetryMarkAttempt(int, uint64_t) {}
void telemetryMarkInvalid(int) {}
bool symulacjaTrwa(bool b, unsigned long, const char*) { return b; }
bool symulacjaCisnieniaAktywna=false, symulacjaWilgotnosciAktywna=false;
unsigned long symulacjaCisnieniaDo=0, symulacjaWilgotnosciDo=0;
float symulowaneCisnienie=1000, symulowanaWilgotnosc=50;
float cisnienieOdczytane=0, wilgotnoscOdczytana=0, wilgotnoscSkorygowana=0;
float temperaturaNaZewnatrz=10;
unsigned long czasOstatniegoPomiaru=0, interwalPomiaru=3000;
int strzalkaWilgotnosc=0;
void ustawInterwalPomiaru() {}
struct MockBmp { float pressure=99400, temperature=-5; int reads=0;
  float readPressure() { return pressure; }
  float readTemperature() { reads++; return temperature; }
} bmp;
float mockTemp=0, mockRh=50; uint8_t mockDhtErr=DHT_OK;
uint8_t read_dht(float &t, float &rh, int, int) { t=mockTemp; rh=mockRh; return mockDhtErr; }
'''
main = r'''
static void output(const TelemetrySample &s) {
  char buf[1024] = "{"; size_t used=1; bool first=true;
  assert(telemetryAppendPathForSample(s,buf,sizeof(buf),used,first));
  telemetryJsonAppend(buf,sizeof(buf),used,"}"); puts(buf);
}
int main() {
  assert(sizeof(TelemetrySample)==80);
  clockUs=1000000; zapiszTempDiag(0.0f); assert(czytajTempDiag()==0.0f);
  clockUs=136000000; assert(isfinite(czytajTempDiag()));
  clockUs=136000001; assert(!isfinite(czytajTempDiag()));
  TelemetrySample old{}; old.unixTs=1700000000; old.sampleSeq=1;
  old.qualityBits=0x003fffff; old.tDhtDiagX10=0; old.tempDiagReserved=100;
  assert(!telemetryHasTempDiag(old)); output(old);
  TelemetrySample next{}; next.unixTs=1700000000; next.sampleSeq=2;
  next.qualityBits=TEMP_DIAG_MARK | 0x12345; next.tDhtDiagX10=0; next.tempDiagReserved=-1;
  TelemetrySample replay{}; unsigned char spool[80];
  memcpy(spool,&next,80); memcpy(&replay,spool,80); assert(memcmp(&replay,&next,80)==0); output(replay);
  next.sampleSeq=3; next.tDhtDiagX10=TEMP_DIAG_MISSING; next.tempDiagReserved=851; output(next);
}
'''

with tempfile.TemporaryDirectory() as tmp:
    tmp = pathlib.Path(tmp)
    src = tmp / 'test.cpp'
    exe = tmp / 'test'
    src.write_text(header + diag + layout + serializer + main, encoding='utf-8')
    subprocess.run(['g++', '-std=c++11', '-Wall', '-Wextra', str(src), '-o', str(exe)], check=True)
    lines = subprocess.check_output([str(exe)], text=True).splitlines()
    old, new, invalid = [next(iter(json.loads(line).values())) for line in lines]
    assert 't_zewn_2' not in old and 't_zewn_3' not in old
    assert old['q'] == 0x003fffff
    assert new['t_zewn_2'] == 0.0 and 't_zewn_3' not in new and new['q'] == 0x12345
    assert 't_zewn_2' not in invalid and 't_zewn_3' not in invalid
    assert len(new['a']) == 11 and len(new['age']) == 11
    print('OK: ABI 80 B, spool, JSON, diag 135s')
