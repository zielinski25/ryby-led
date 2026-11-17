#!/usr/bin/env python3
"""Compile and exercise the candidate's actual DS scheduler with simulated buses.
Host-only: not firmware build, not physical DS18B20 or control-loop verification.
"""
import pathlib, subprocess, sys, tempfile
s=pathlib.Path(sys.argv[1]).read_text(encoding='utf-8')
a=s.index('enum DsPlanId : uint8_t {'); b=s.index('\nvoid ustawInterwalPomiaru()',a)
code=s[a:b]
def section(a,b):
 start=s.index(a); return s[start:s.index(b,start)]
quality_code=(section('static const uint16_t TELEMETRY_STALE_DS18B20_S', '\nstatic void telemetryMarkAttempt')+
              section('static uint16_t telemetryAgeSeconds', '\nstatic bool telemetrySimulationActive')+
              'static bool telemetrySimulationActive(TelemetryChannel, uint32_t) { return true; }\n'+
              section('static uint8_t telemetryQuality', '\n// Automatyka mieszadla'))
head=r'''
#include <cstdint>
#include <cmath>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
using std::isfinite;
#define DLOGF(...) ((void)0)
#define min(a,b) ((a)<(b)?(a):(b))
static uint32_t clockMs=0;
unsigned long millis(){ return clockMs; }
uint64_t telemetryMonotonicMs(){ return clockMs; }
enum TelemetryChannel : uint8_t {TELEMETRY_CH_ZEWN,TELEMETRY_CH_BOJLER,TELEMETRY_CH_OGZ,TELEMETRY_CH_OGZ_SR,TELEMETRY_CH_POWROT,TELEMETRY_CH_PANEL,TELEMETRY_CH_POKOJ,TELEMETRY_CH_TROCINY,TELEMETRY_CH_RH,TELEMETRY_CH_PRESSURE,TELEMETRY_CH_DYM,TELEMETRY_CH_COUNT};
enum TelemetrySourceStatus {TELEMETRY_STATUS_NO_ATTEMPT,TELEMETRY_STATUS_INVALID,TELEMETRY_STATUS_REJECTED,TELEMETRY_STATUS_ACCEPTED,TELEMETRY_STATUS_SIMULATED,TELEMETRY_STATUS_STALE};
struct TelemetrySourceMeta {uint64_t lastAttemptMs=0,lastGoodMs=0; bool hasGoodValue=false,lastGoodSimulated=false; TelemetrySourceStatus lastStatus=TELEMETRY_STATUS_NO_ATTEMPT;};
TelemetrySourceMeta g_telemetrySourceMeta[TELEMETRY_CH_COUNT];
void telemetryMarkAttempt(TelemetryChannel c,uint64_t n){auto &m=g_telemetrySourceMeta[c];m.lastAttemptMs=n;m.lastStatus=TELEMETRY_STATUS_NO_ATTEMPT;}
void telemetryMarkInvalid(TelemetryChannel c){g_telemetrySourceMeta[c].lastStatus=TELEMETRY_STATUS_INVALID;}
/* INSERT_ACTUAL_QUALITY */
struct DS18B20 {bool online=true, ready=true; unsigned requests=0; uint32_t lastRequest=0; bool request(){requests++;lastRequest=clockMs;return online;} bool available(){return ready && (uint32_t)(clockMs-lastRequest)>=750;}};
DS18B20 sensors1,sensors2,sensors3;
int dsPowerCycleStan=0; const int DS_PC_IDLE=0; unsigned failures=0;
void dsZglosBladOdczytu(){failures++;}
bool pump=false; bool pompaJestWlaczona(){return pump;}
bool trybSymulacji=false;
#define SIM(name) bool symulacja##name##Aktywna=false; unsigned long symulacja##name##Do=0;
SIM(Ogrz) SIM(Powrotu) SIM(Trocin) SIM(Panelu) SIM(Bojlera) SIM(Zewn) SIM(Pokoju)
bool symulacjaTrwa(bool &active,unsigned long &until,const char*) {if(active && until && clockMs>=until) active=false; return active;}
float temperaturaOgrzewania=0,temperaturaOgrzewania_Powrot=0,temperaturaOgrzewania_Trociny=0,temperaturaPaneluSlonecznego=0,temperaturaBojlera=0,temperaturaNaZewnatrz=0,temperaturaPomieszczenia=0;
float symulowanaTemperaturaOgrzewania=10,symulowanaTemperaturaPowrot=10,symulowanaTemperaturaTrociny=10,symulowanaTemperaturaPanelu=10,symulowanaTemperaturaBojlera=10,symulowanaTemperaturaNaZewnatrz=-5,symulowanaTemperaturaPomieszczenia=10;
uint8_t czujnik_temp_C_O[8],czujnik_temp_C_O_powrot[8],czujnik_temp_C_O_trociny[8],czujnik_temp_Panel_Plus[8],czujnik_temp_Bojlera[8],czujnik_temp_zewnetrznej[8],czujnik_temp_Wewnatrz[8];
struct FiltrWiarygodnosci{};
FiltrWiarygodnosci filtrCO,filtrPowrot,filtrTrociny,filtrPanel,filtrBojler,filtrZewnetrzna,filtrWewnatrz;
enum MagistralaCzujnika {BUS_1,BUS_2,BUS_3};
std::vector<uint32_t> reads[TELEMETRY_CH_COUNT]; float values[TELEMETRY_CH_COUNT];
float odczytajTemperature(uint8_t*,float,float,float sim,FiltrWiarygodnosci&,MagistralaCzujnika,bool &active,unsigned long&,const char*,TelemetryChannel c){
 auto &m=g_telemetrySourceMeta[c]; bool synthetic=trybSymulacji||active;
 m.lastAttemptMs=clockMs;m.lastGoodMs=clockMs;m.hasGoodValue=true;m.lastGoodSimulated=synthetic;m.lastStatus=synthetic?TELEMETRY_STATUS_SIMULATED:TELEMETRY_STATUS_ACCEPTED;
 reads[c].push_back(clockMs); return synthetic ? sim : values[c];
}
'''
main=r'''
void run(uint32_t end) { while(clockMs<end) {clockMs+=500; Czujnik_DS();} }
void clear(){clockMs=0; pump=false;trybSymulacji=false;dsPowerCycleStan=0;failures=0;
 for(auto &s:dsPlan)s={};
 for(auto &b:dsBus)b={};
 for(auto &m:g_telemetrySourceMeta)m={};
 for(auto &r:reads)r.clear();
 for(float &v:values)v=20;
 sensors1=DS18B20{};sensors2=DS18B20{};sensors3=DS18B20{};
 symulacjaZewnAktywna=false;symulacjaZewnDo=0;
}
int main(){
 clear(); pump=true; Czujnik_DS();
 assert(sensors1.requests==1 && sensors2.requests==1 && sensors3.requests==1);
 clockMs=500; Czujnik_DS(); assert(reads[TELEMETRY_CH_OGZ].empty());
 clockMs=1000; Czujnik_DS(); assert(reads[TELEMETRY_CH_OGZ].size()==1);
 assert(temperaturaOgrzewania==20 && dsPlanInterval(DS_CO,clockMs)==60000);
 run(60500); assert(reads[TELEMETRY_CH_OGZ].size()==1);
 run(61000); assert(reads[TELEMETRY_CH_OGZ].size()==2);
 assert(reads[TELEMETRY_CH_ZEWN].size()==1 && reads[TELEMETRY_CH_PANEL].size()==2);
 run(121000); assert(reads[TELEMETRY_CH_ZEWN].size()==2 && reads[TELEMETRY_CH_BOJLER].size()==3);
 clear(); pump=true; values[TELEMETRY_CH_TROCINY]=30.0f;
 Czujnik_DS(); run(1000); assert(dsPlanInterval(DS_CO,clockMs)==2816);
 run(4000); assert(reads[TELEMETRY_CH_OGZ].size()>=2);
 assert(reads[TELEMETRY_CH_TROCINY].size()==1); // bus converts, but no reread
 assert(sensors2.requests>sensors3.requests);
 pump=false; assert(dsPlanInterval(DS_CO,clockMs)==15000);
 g_telemetrySourceMeta[TELEMETRY_CH_TROCINY].lastStatus=TELEMETRY_STATUS_INVALID;
 assert(dsPlanInterval(DS_CO,clockMs)==15000);
 clear(); sensors2.online=false; Czujnik_DS();
 assert(g_telemetrySourceMeta[TELEMETRY_CH_OGZ].lastStatus==TELEMETRY_STATUS_INVALID);
 assert(reads[TELEMETRY_CH_OGZ].empty() && failures==1);
 clear(); sensors2.ready=false; Czujnik_DS(); run(3000);
 assert(!dsBus[1].pending && failures==1 && reads[TELEMETRY_CH_OGZ].empty());
 clear(); Czujnik_DS(); dsPowerCycleStan=1; clockMs=500; Czujnik_DS();
 assert(!dsBus[0].pending && !dsBus[1].pending && !dsPlan[DS_CO].tried);
 dsPowerCycleStan=0; clockMs=1000; Czujnik_DS(); run(2000); assert(reads[TELEMETRY_CH_OGZ].size()==1);
 clear(); Czujnik_DS(); clockMs=500; symulacjaZewnAktywna=true;symulacjaZewnDo=4000;Czujnik_DS();
 assert(reads[TELEMETRY_CH_ZEWN].size()==1 && temperaturaNaZewnatrz==-5);
 run(5000); assert(reads[TELEMETRY_CH_ZEWN].size()==2 && temperaturaNaZewnatrz==20);
 dsPlan[DS_CO].tried=true;dsPlan[DS_CO].lastMs=0xfffffff0u;
 assert(dsPlanDue(DS_CO,0x000003e8u,1016,0));
 puts("OK: rzeczywisty harmonogram DS — interwaly, prog, pompa, awaria, timeout, reset, symulacja, wrap");
}
'''
with tempfile.TemporaryDirectory() as d:
 cpp=pathlib.Path(d)/'test.cpp';exe=pathlib.Path(d)/'test';cpp.write_text(head.replace('/* INSERT_ACTUAL_QUALITY */', quality_code)+code+main,encoding='utf-8')
 subprocess.run(['c++','-std=c++17','-Wall','-Wextra',str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
