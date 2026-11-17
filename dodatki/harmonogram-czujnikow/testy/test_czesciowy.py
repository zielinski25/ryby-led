#!/usr/bin/env python3
"""Hostowe testy fragmentów częściowego kandydata. To nie test ESP ani pieca."""
import pathlib, subprocess, sys, tempfile
s = pathlib.Path(sys.argv[1]).read_text(encoding='utf-8')
def frag(a, b):
    start = s.index(a)
    return s[start:s.index(b, start)]
filter_code = frag('struct FiltrWiarygodnosci {', '\n// ════════════════════════════════════════════════════════\n// ODCZYT CZUJNIKÓW TEMPERATURY')
diag = frag('static portMUX_TYPE g_tempDiagMux', '\nfloat cisnienieOdczytane =')
sensors = frag('void cisnienie() {', '\n// ════════════════════════════════════════════════════════\n// MIESZADŁO')
assert 'delay(2000);' in s[s.index('// --- DHT22: retry+backoff'):s.index('// --- Karta SD')]
assert 'zastosujFiltrWiarygodnosci(filtr, surowyOdczyt, odczytPoprawny)' in s
prelude = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <string>
using std::isfinite; using std::isnan;
static uint64_t clockMs=0;
unsigned long millis() {return (unsigned long)clockMs;}
int64_t esp_timer_get_time() {return (int64_t)clockMs*1000;}
uint64_t telemetryMonotonicMs() {return clockMs;}
using portMUX_TYPE=int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x) ((void)(x))
#define portEXIT_CRITICAL(x) ((void)(x))
#define DLOGF(...) ((void)0)
struct String {
 std::string v;
 String(const char *s):v(s){} String(unsigned char x):v(std::to_string(x)){}
 String(float x,int):v(std::to_string(x)){}
};
String operator+(const char *a,const String &b){return String((std::string(a)+b.v).c_str());}
String operator+(const String &a,const char *b){return String((a.v+b).c_str());}
String operator+(const String &a,const String &b){return String((a.v+b.v).c_str());}
void debugPrint(const char*){} void zapiszLogKrytyczny(const String&){}
const uint8_t DS_POWERCYCLE_PROG_BLEDOW=3;
'''
stubs = r'''
static unsigned long dhtOstatniaProbaMs=0;
static bool dhtBylaProba=false;
enum { TELEMETRY_CH_PRESSURE=1, TELEMETRY_CH_RH=2, DHT_OK=0, DHT_PIN=6, DHT22=22 };
void telemetryMarkAccepted(int,uint64_t,bool){} void telemetryMarkAttempt(int,uint64_t){}
void telemetryMarkInvalid(int){}
bool symulacjaTrwa(bool b,unsigned long,const char*) {return b;}
bool symulacjaCisnieniaAktywna=false,symulacjaWilgotnosciAktywna=false;
unsigned long symulacjaCisnieniaDo=0,symulacjaWilgotnosciDo=0;
float symulowaneCisnienie=1000,symulowanaWilgotnosc=50;
float cisnienieOdczytane=0,wilgotnoscOdczytana=0,wilgotnoscSkorygowana=0;
float temperaturaNaZewnatrz=10;
unsigned long czasOstatniegoPomiaru=0,interwalPomiaru=600000;
int strzalkaWilgotnosc=0;
void ustawInterwalPomiaru(){}
struct MockBmp {int reads=0; float readPressure(){++reads;return 99400;}} bmp;
int dhtReads=0; float mockTemp=0,mockRh=50; uint8_t mockErr=0;
uint8_t read_dht(float &t,float &h,int,int) {++dhtReads;t=mockTemp;h=mockRh;return mockErr;}
'''
main = r'''
int main() {
 FiltrWiarygodnosci f{NAN,false,NAN,0,"zew"};
 assert(zastosujFiltrWiarygodnosci(f,-1,true)==-1 && f.telemetryAcceptedThisCall);
 assert(zastosujFiltrWiarygodnosci(f,-10,true)==-10 && f.telemetryAcceptedThisCall);
 assert(zastosujFiltrWiarygodnosci(f,-999,false)==-10 && !f.telemetryAcceptedThisCall);
 assert(zastosujFiltrWiarygodnosci(f,-25,true)==-25 && f.telemetryAcceptedThisCall);
 assert(zastosujFiltrWiarygodnosci(f,10,true)==-25 && !f.telemetryAcceptedThisCall);
 assert(zastosujFiltrWiarygodnosci(f,10,true)==10 && f.telemetryAcceptedThisCall);
 clockMs=10000; dhtBylaProba=true; dhtOstatniaProbaMs=10000;
 wilgotnosc(); assert(dhtReads==0);
 clockMs=11999; wilgotnosc(); assert(dhtReads==0);
 clockMs=12000; wilgotnosc(); assert(dhtReads==1 && czytajTempDiag()==0);
 clockMs=131999; wilgotnosc(); assert(dhtReads==1);
 clockMs=132000; mockTemp=-5; wilgotnosc(); assert(dhtReads==2 && czytajTempDiag()==-5);
 clockMs=267000; assert(isfinite(czytajTempDiag()));
 clockMs=267001; assert(!isfinite(czytajTempDiag()));
 clockMs=270000; mockErr=1; wilgotnosc(); assert(dhtReads==3 && !isfinite(czytajTempDiag()));
 clockMs=10000; cisnienie(); assert(bmp.reads==1);
 clockMs=189999; cisnienie(); assert(bmp.reads==1);
 clockMs=190000; cisnienie(); assert(bmp.reads==2);
 puts("OK: fragmenty DHT, BMP i filtr temperatur ujemnych (host)");
}
'''
with tempfile.TemporaryDirectory() as d:
    cpp=pathlib.Path(d)/'test.cpp'; exe=pathlib.Path(d)/'test'
    cpp.write_text(prelude+diag+stubs+filter_code+sensors+main,encoding='utf-8')
    subprocess.run(['c++','-std=c++17','-Wall','-Wextra',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
