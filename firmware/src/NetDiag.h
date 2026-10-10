// ============================================================
// NetDiag.h — moduł diagnostyki sieci/routera (DODATEK, opcjonalny)
// Wrzesień 2026 — wersja 3, WŁASNY PLIK LOGU (osobno od log_a/log_b)
// na życzenie z czatu: "wolałbym osobny plik". Architektura Core0/
// Core1 z wersji 2 (v42 FIX-ARCH: "zero alokacji sieciowych na
// Core 1") bez zmian — patrz punkt 1) niżej.
//
// CEL: test routera/łącza przez 48h z inteligentnym (godzinowym)
// logowaniem, żeby nie zaśmiecać LittleFS. Po 48h NIE wyłącza się -
// tylko przechodzi w rzadszy tryb "lekki" (co 5 min zamiast co 60s).
//
// ARCHITEKTURA (WAŻNE, przeczytaj przed wgraniem):
//
//  1) loop() (Core 1) NIE wykonuje żadnego połączenia sieciowego -
//     tylko sprawdza timery i ustawia flagi (netDiagPending,
//     netDiagSummaryPending), DOKŁADNIE tak jak istniejący wzorzec
//     fbSendPending/fbCmdPending/fbCfgPending w sekcji FIREBASE-v2.
//     Realne WiFiClient.connect() dzieje się WYŁĄCZNIE w tgTaskFn
//     (Core 0), tam gdzie już dziś żyje cała reszta sieci (Firebase,
//     Telegram) - zgodnie z regułą [v42] "zero alokacji sieciowych
//     na Core 1" z komentarza nad blokiem FIREBASE-v2 w loop().
//
//  2) Testy używają WYŁĄCZNIE adresów IP (gateway + 1.1.1.1 + 8.8.8.8),
//     NIGDY nazw hostname -> zero dodatkowych wywołań DNS/hostByName,
//     żeby nie ożywić assertu z v239 (SNTP + DNS non-reentrant w lwIP).
//
//  3) CELOWO nie używam biblioteki ping/ICMP - ICMP na arduino-esp32
//     też siedzi na surowych gniazdach lwIP, a w projekcie z taką
//     historią wyścigów w tym module wolałem nie dokładać kolejnego
//     potencjalnie nie-reentrantnego wywołania. Zwykłe połączenie TCP
//     (port 80/443) i tak pokazuje "czy się da połączyć" i ile trwało.
//
//  4) Test w tgTaskFn jest zagatowany przez ITER_HAS_BUDGET() - tak
//     jak fbSendPending/fbCmdPending/fbCfgPending - więc nie zjada
//     budżetu iteracji ponad miarę. Umyślnie NIE jest w sekcji
//     chronionej przez _netCircuitOpen (circuit breaker Firebase/
//     Telegram) - to osobny, niezależny mechanizm, nie chcę mieszać
//     jego statystyk fail-streak z testem routera.
//
//  5) Twardy limit czasu samego connect() = NETDIAG_TCP_TIMEOUT_MS
//     (800ms) - z marginesem w 10000ms budżetu iteracji tgTaskFn.
//
//  6) [v3] ZAPIS DO WŁASNEGO PLIKU (/netdiag.txt), NIE przez logPrintf():
//     - Pisze WYŁĄCZNIE _netDiagWriteLine() (statyczna funkcja niżej),
//       wołana z tgTaskFn (Core 0) - ten sam rdzeń co i tak już robił
//       netDiagRunPendingTest()/netDiagWritePendingSummary(), więc
//       reguła "zero alokacji sieciowych na Core 1" z punktu 1) nie
//       jest tu w ogóle w grze (to zapis na flash, nie sieć).
//     - Własny, mały, twardy limit rozmiaru NETDIAG_LOG_MAX_BYTES
//       (96 KB) - NIEZALEŻNY od LOG_GUARD_LIMIT (7 MB, log_a+log_b)
//       i od fsSizeGuard()/fsGuardPending z głównego pliku. Netdiag
//       w ogóle nie wchodzi w rachunek g_logFileSize/g_logFileSizeA,
//       więc test routera nigdy nie wywoła rotacji głównego logu.
//     - Gdy plik przekroczy limit: USUŃ + zacznij od nowa (zamiast
//       rename). Celowo, bo historia TEGO projektu (patrz komentarz
//       o log_tmp.txt w głównym pliku) już raz pokazała, że
//       LittleFS.rename() może zostawić pusty/częściowy plik przy
//       niefortunnym timingu - dla diagnostyki-side-car to ryzyko
//       niewarte oszczędności miejsca, prościej i bezpieczniej jest
//       po prostu zacząć nowy plik.
//     - Ten sam heap-guard co logToFile() w głównym pliku (próg
//       8192 B największego wolnego bloku przed LittleFS.open() -
//       zapobiega assertowi lfs_file_close przy sfragmentowanym
//       heapie) i esp_task_wdt_reset() po zapisie - identyczna
//       ostrożność, inny plik.
//     - Każda linia ma własny znacznik czasu [HH:MM:SS] (przez
//       getLocalTimePL(), tę samą funkcję co reszta projektu) -
//       plik jest czytelny sam z siebie, bez potrzeby zerkania do
//       log_a/log_b po kontekst czasowy.
//
//  7) ZALECENIE: przed wrzuceniem do produkcyjnego urządzenia
//     przetestuj moduł osobno, na czystym szkicu, i popatrz przez
//     Serial czy timing Ci pasuje.
//
// INTEGRACJA - patrz zmieniony Ryby_LED_fi_S3.cpp (4 miejsca,
// oznaczone komentarzem "// [NETDIAG]"). BEZ ZMIAN względem v2 -
// ta wersja modyfikuje WYŁĄCZNIE ten plik (NetDiag.h):
//   a) #include "NetDiag.h"          - przy pozostałych include
//   b) netDiagInit();                - na końcu setup()
//   c) netDiagLoop();                - w loop() (Core 1, tylko flagi)
//   d) 2 bloki if(...Pending...)     - w tgTaskFn (Core 0, realny test)
//
// LOGI: WŁASNY plik /netdiag.txt (nie log_a.txt/log_b.txt, nie
// tag=NETDIAG w głównym logu). Podgląd/pobranie przez istniejące
// endpointy panelu WWW (port 8080), dokładnie tak samo jak dla
// każdego innego pliku na LittleFS:
//   - podgląd (ost. N linii): /api/fs-view?file=/netdiag.txt&lines=50
//   - podgląd całości:        /api/fs-view?file=/netdiag.txt
//   - pobranie pliku:         /api/fs-download?file=/netdiag.txt
// ============================================================

#pragma once
#include <WiFi.h>
#include <LittleFS.h>
#include <time.h>   // struct tm dla własnego znacznika czasu w _netDiagWriteLine()

// Zdefiniowane w głównym .cpp (LittleFS.begin() w setup()), tylko
// deklarowane tutaj - #include "NetDiag.h" w Ryby_LED_fi_S3.cpp jest
// UMIEJSCOWIONY PRZED definicją littlefsReady (ta jest dużo niżej w
// tym samym pliku), więc bez tej deklaracji byłby to nieznany
// identyfikator w miejscu użycia w _netDiagWriteLine().
extern bool littlefsReady;

// ---- KONFIGURACJA ----
#define NETDIAG_TEST_DURATION_MS   (48UL*3600UL*1000UL)   // 48h fazy gęstej
#define NETDIAG_DENSE_INTERVAL_MS  (60UL*1000UL)           // test co 60s w fazie gęstej
#define NETDIAG_LIGHT_INTERVAL_MS  (5UL*60UL*1000UL)       // test co 5 min w fazie lekkiej (po 48h)
#define NETDIAG_LOG_SUMMARY_MS     (60UL*60UL*1000UL)      // podsumowanie do logu co 1h
#define NETDIAG_TCP_TIMEOUT_MS     800                      // twardy limit czasu próby połączenia (Core 0)

// [v3] Osobny plik + jego własny, mały limit rozmiaru (patrz punkt 6 wyżej).
#define NETDIAG_LOG_FILE           "/netdiag.txt"
#define NETDIAG_LOG_MAX_BYTES      (98304UL)               // 96 KB - twardy limit WYŁĄCZNIE tego pliku

struct NetDiagTarget {
  const char* nazwa;
  IPAddress   ip;
  uint16_t    port;
};

// UWAGA: GATEWAY ma tu IP=0.0.0.0 - uzupełniany dopiero w netDiagInit()/
// netDiagLoop(), bo poznajemy go dopiero po WiFi.begin().
static NetDiagTarget netDiagTargets[3] = {
  { "GATEWAY",     IPAddress(0, 0, 0, 0), 80  },
  { "WAN-CF",      IPAddress(1, 1, 1, 1), 443 },
  { "WAN-GOOGLE",  IPAddress(8, 8, 8, 8), 443 },
};

struct NetDiagStats {
  uint32_t prob;     // ile prob w tym oknie (godzinie)
  uint32_t ok;       // ile udanych połączeń
  uint32_t sumMs;    // suma czasu połączenia (tylko udane)
  uint32_t minMs;
  uint32_t maxMs;
  int32_t  rssiSum;
  int32_t  rssiMin;
  int32_t  rssiMax;
};

static unsigned long   _netDiagStartMs   = 0;
static unsigned long   _netDiagLastTest  = 0;
static unsigned long   _netDiagLastLog   = 0;
static uint8_t         _netDiagTargetIdx = 0;
static NetDiagStats    _netDiagStats[3];

// Flagi Core1 -> Core0. Zwykłe bool/uint8_t - DOKŁADNIE ten sam,
// już zaufany w tym projekcie wzorzec co fbSendPending/fbCmdPending/
// fbCfgPending (ustawiane na Core1 w loop(), czyszczone na Core0
// w tgTaskFn). Bez tego pliku nic nie woła sieci z Core 1.
static volatile bool    netDiagPending        = false;
static volatile uint8_t netDiagPendingIdx     = 0;
static volatile bool    netDiagSummaryPending = false;

static void _netDiagResetStats() {
  for (int i = 0; i < 3; i++) {
    _netDiagStats[i].prob    = 0;
    _netDiagStats[i].ok      = 0;
    _netDiagStats[i].sumMs   = 0;
    _netDiagStats[i].minMs   = 0xFFFFFFFFUL;
    _netDiagStats[i].maxMs   = 0;
    _netDiagStats[i].rssiSum = 0;
    _netDiagStats[i].rssiMin = 999;
    _netDiagStats[i].rssiMax = -999;
  }
}

// [v3] Wywołaj WYŁĄCZNIE z tgTaskFn (Core 0) - dokładnie tak samo jak
// netDiagRunPendingTest()/netDiagWritePendingSummary() niżej, więc nie
// łamie reguły [v42] (to i tak nie jest sieć, tylko zapis na flash).
// Zero rzucania wyjątków/crashy przy braku FS - po prostu cichy return,
// tak jak logToFile() w głównym pliku robi dla log_b.txt.
static void _netDiagWriteLine(const char* line) {
  if (!littlefsReady) return;

  // Ten sam próg i to samo uzasadnienie co w logToFile() (główny plik,
  // ~linia 12370): LittleFS.open() na mocno sfragmentowanym heapie
  // potrafi wywołać assert lfs_file_close - lepiej ominąć zapis niż
  // zrestartować urządzenie przez test diagnostyczny.
  if (heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) < 8192) return;

  // Własny, mały limit rozmiaru - sprawdzany PRZED każdym zapisem.
  // Po przekroczeniu: usuń i zacznij od nowa (nie rename - patrz
  // uzasadnienie w komentarzu nagłówkowym, punkt 6).
  {
    File chk = LittleFS.open(NETDIAG_LOG_FILE, "r");
    if (chk) {
      size_t sz = chk.size();
      chk.close();
      if (sz >= NETDIAG_LOG_MAX_BYTES) {
        LittleFS.remove(NETDIAG_LOG_FILE);
      }
    }
  }

  File f = LittleFS.open(NETDIAG_LOG_FILE, "a");
  if (!f) return;
  f.print(line);
  f.close();

  esp_task_wdt_reset();  // ta sama ostrożność co logToFile() po zapisie na flash (Core 0)
}

// Wywołaj RAZ w setup(), na samym końcu (po WiFi + po sekcji SNTP z v239).
void netDiagInit() {
  netDiagTargets[0].ip = WiFi.gatewayIP();  // lokalny odczyt, zero DNS
  _netDiagStartMs  = millis();
  _netDiagLastTest = 0;   // pierwszy test odpali się od razu w netDiagLoop()
  _netDiagLastLog  = millis();
  _netDiagResetStats();
}

// Wywołaj RAZ na obieg loop() (Core 1). ZERO sieci tutaj - tylko timery
// i flagi, zgodnie z regułą [v42] "zero alokacji sieciowych na Core 1".
void netDiagLoop() {
  if (WiFi.status() != WL_CONNECTED) return;  // brak WiFi loguje już istniejący kod

  unsigned long _now = millis();
  bool dense = (_now - _netDiagStartMs) < NETDIAG_TEST_DURATION_MS;
  unsigned long interval = dense ? NETDIAG_DENSE_INTERVAL_MS : NETDIAG_LIGHT_INTERVAL_MS;

  if (netDiagTargets[0].ip == IPAddress(0, 0, 0, 0)) {
    netDiagTargets[0].ip = WiFi.gatewayIP();  // dogrywka, gdyby był nieznany przy starcie
  }

  if (!netDiagPending && _now - _netDiagLastTest >= interval) {
    _netDiagLastTest  = _now;
    netDiagPendingIdx = _netDiagTargetIdx;
    _netDiagTargetIdx = (uint8_t)((_netDiagTargetIdx + 1) % 3);
    netDiagPending    = true;   // Core 0 (tgTaskFn) wykona faktyczny test TCP
  }

  if (!netDiagSummaryPending && _now - _netDiagLastLog >= NETDIAG_LOG_SUMMARY_MS) {
    _netDiagLastLog        = _now;
    netDiagSummaryPending  = true;  // Core 0 zapisze podsumowanie do pliku
  }
}

// Wywołaj WYŁĄCZNIE z tgTaskFn (Core 0), zagatowane przez ITER_HAS_BUDGET().
// Tu i tylko tu dzieje się faktyczne połączenie TCP (limit 800ms).
static void netDiagRunPendingTest() {
  uint8_t idx = netDiagPendingIdx;
  NetDiagTarget &t = netDiagTargets[idx];
  if (t.ip == IPAddress(0, 0, 0, 0)) return;  // gateway jeszcze nieznany - pomiń ten cykl

  WiFiClient c;
  unsigned long _t0 = millis();
  bool ok = c.connect(t.ip, t.port, NETDIAG_TCP_TIMEOUT_MS);
  unsigned long _dt = millis() - _t0;
  if (ok) c.stop();

  NetDiagStats &s = _netDiagStats[idx];
  s.prob++;
  if (ok) {
    s.ok++;
    s.sumMs += _dt;
    if (_dt < s.minMs) s.minMs = _dt;
    if (_dt > s.maxMs) s.maxMs = _dt;
  }
  int rssi = WiFi.RSSI();
  s.rssiSum += rssi;
  if (rssi < s.rssiMin) s.rssiMin = rssi;
  if (rssi > s.rssiMax) s.rssiMax = rssi;
}

// Wywołaj WYŁĄCZNIE z tgTaskFn (Core 0), zagatowane przez ITER_HAS_BUDGET().
// [v3] Zapisuje do WŁASNEGO pliku (/netdiag.txt) przez _netDiagWriteLine(),
// NIE przez logPrintf() - patrz uzasadnienie w komentarzu nagłówkowym pkt 6.
static void netDiagWritePendingSummary() {
  bool dense = (millis() - _netDiagStartMs) < NETDIAG_TEST_DURATION_MS;

  // Własny znacznik czasu - plik ma być czytelny sam z siebie, bez
  // potrzeby korelacji z log_a/log_b po kontekst czasowy.
  struct tm _ti;
  char _tsBuf[12];
  if (getLocalTimePL(&_ti)) {
    snprintf(_tsBuf, sizeof(_tsBuf), "%02d:%02d:%02d", _ti.tm_hour, _ti.tm_min, _ti.tm_sec);
  } else {
    snprintf(_tsBuf, sizeof(_tsBuf), "??:??:??");
  }

  for (int i = 0; i < 3; i++) {
    NetDiagStats &s = _netDiagStats[i];
    if (s.prob == 0) continue;  // nic do zalogowania w tym oknie

    float lossPct = 100.0f * (float)(s.prob - s.ok) / (float)s.prob;
    float avgMs   = s.ok ? (float)s.sumMs / (float)s.ok : 0.0f;
    float rssiAvg = (float)s.rssiSum / (float)s.prob;

    // WARN gdy >20% prob nieudanych w tym oknie, inaczej INFO
    char line[256];
    snprintf(line, sizeof(line),
      "[%s] lvl=%s cel=%s prob=%u ok=%u strata=%.1f%% "
      "rtt_min=%ums rtt_avg=%.0fms rtt_max=%ums rssi_min=%d rssi_avg=%.0f rssi_max=%d tryb=%s\n",
      _tsBuf,
      (lossPct > 20.0f) ? "WARN" : "INFO",
      netDiagTargets[i].nazwa, (unsigned)s.prob, (unsigned)s.ok, lossPct,
      (unsigned)(s.ok ? s.minMs : 0), avgMs, (unsigned)(s.ok ? s.maxMs : 0),
      (int)s.rssiMin, rssiAvg, (int)s.rssiMax,
      dense ? "GESTY" : "LEKKI"
    );
    _netDiagWriteLine(line);
  }
  _netDiagResetStats();
}