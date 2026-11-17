#!/usr/bin/env python3
"""Host test of candidate's actual quality/threshold functions."""
import pathlib, subprocess, sys, tempfile
s=pathlib.Path(sys.argv[1]).read_text(encoding='utf-8')
def between(a,b):
 i=s.index(a);return s[i:s.index(b,i)]
constants=between('static const uint16_t TELEMETRY_STALE_DS18B20_S', '\nstatic void telemetryMarkAttempt')
age=between('static uint16_t telemetryAgeSeconds', '\nstatic bool telemetrySimulationActive')
quality=between('static uint8_t telemetryQuality', '\n// Automatyka mieszadla')
pre=r'''
#include <stdint.h>
#include <cassert>
#define min(a,b) ((a)<(b)?(a):(b))
enum TelemetryChannel : uint8_t {TELEMETRY_CH_ZEWN,TELEMETRY_CH_BOJLER,TELEMETRY_CH_OGZ,TELEMETRY_CH_OGZ_SR,TELEMETRY_CH_POWROT,TELEMETRY_CH_PANEL,TELEMETRY_CH_POKOJ,TELEMETRY_CH_TROCINY,TELEMETRY_CH_RH,TELEMETRY_CH_PRESSURE,TELEMETRY_CH_DYM,TELEMETRY_CH_COUNT};
enum TelemetrySourceStatus {TELEMETRY_STATUS_NO_ATTEMPT,TELEMETRY_STATUS_INVALID,TELEMETRY_STATUS_REJECTED,TELEMETRY_STATUS_ACCEPTED,TELEMETRY_STATUS_SIMULATED,TELEMETRY_STATUS_STALE};
struct TelemetrySourceMeta {uint64_t lastAttemptMs=0,lastGoodMs=0;bool hasGoodValue=false,lastGoodSimulated=false;TelemetrySourceStatus lastStatus=TELEMETRY_STATUS_NO_ATTEMPT;};
TelemetrySourceMeta g_telemetrySourceMeta[TELEMETRY_CH_COUNT];
static bool simulation=false;
static bool telemetrySimulationActive(TelemetryChannel,uint32_t){return simulation;}
'''
main=r'''
int main(){
 assert(telemetryStaleThresholdS(TELEMETRY_CH_OGZ)==75);
 assert(telemetryStaleThresholdS(TELEMETRY_CH_TROCINY)==75);
 assert(telemetryStaleThresholdS(TELEMETRY_CH_ZEWN)==135);
 assert(telemetryStaleThresholdS(TELEMETRY_CH_RH)==135);
 assert(telemetryStaleThresholdS(TELEMETRY_CH_PRESSURE)==195);
 assert(telemetryQuality(TELEMETRY_CH_OGZ,0)==2);
 auto &m=g_telemetrySourceMeta[TELEMETRY_CH_OGZ];
 m.hasGoodValue=true;m.lastGoodMs=1000;m.lastStatus=TELEMETRY_STATUS_ACCEPTED;
 assert(telemetryQuality(TELEMETRY_CH_OGZ,76000)==0);
 assert(telemetryQuality(TELEMETRY_CH_OGZ,77000)==1);
 m.lastStatus=TELEMETRY_STATUS_INVALID;
 assert(telemetryQuality(TELEMETRY_CH_OGZ,2000)==3);
 m.lastStatus=TELEMETRY_STATUS_REJECTED;
 assert(telemetryQuality(TELEMETRY_CH_OGZ,2000)==3);
 m.lastStatus=TELEMETRY_STATUS_NO_ATTEMPT;
 assert(telemetryQuality(TELEMETRY_CH_OGZ,2000)==3);
 m.lastStatus=TELEMETRY_STATUS_SIMULATED;m.lastGoodSimulated=true;
 assert(telemetryQuality(TELEMETRY_CH_OGZ,2000)==1);
 simulation=true; assert(telemetryQuality(TELEMETRY_CH_OGZ,2000)==0);
}
'''
with tempfile.TemporaryDirectory() as d:
 cpp=pathlib.Path(d)/'test.cpp'; exe=pathlib.Path(d)/'test';cpp.write_text(pre+constants+age+quality+main,encoding='utf-8')
 subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
 print('OK: rzeczywiste progi swiezosci, blad, symulacja')
