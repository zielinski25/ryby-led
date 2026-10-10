// ═══════════════════════════════════════════════════════════════════════════
//  test_location.cpp — [4.7.0 ASTRO] lokalizacja, poranek i RTC z panelu (Etap 6).
//  Kompiluje PRAWDZIWE fragmenty Ryby_LED_fi_S3.cpp (wyciągnięte przez
//  check_location.sh → loc_extract.inc): zmienne, funkcje NVS, morningStartFor,
//  przeliczAstroDzis, rtcglue/rtcRestoreAtBoot/rtcSaveFromSystem oraz endpointy
//  GET/POST /api/location i GET/POST /api/rtc. Zależności: loc_mock.h + astro.cpp
//  + rtc_ds13xx.cpp.
// ═══════════════════════════════════════════════════════════════════════════
#include <cstdlib>
#include "loc_mock.h"
#include "loc_extract.inc"

static int g_pass = 0, g_fail = 0;
#define CHECK(cond, name)                                              \
  do {                                                                 \
    if (cond) { g_pass++; }                                            \
    else { g_fail++; std::printf("FAIL: %s (linia %d)\n", name, __LINE__); } \
  } while (0)

// Stałe czasu (UTC): sobota 2026-10-10 08:00 i poniedziałek 2026-10-12 08:00.
static const time_t kSat = 1791619200;
static const time_t kMon = 1791792000;

static void postTo(const char* path, const char* body) {
  AsyncWebServerRequest req;
  auto& h = webserialServer.postH[path];
  size_t n = strlen(body);
  h(&req, (uint8_t*)body, n, 0, n);
}
static void postBody(const char* body) { postTo("/api/location", body); }
static void postRtc(const char* body) { postTo("/api/rtc", body); }

static void getLocation() {
  AsyncWebServerRequest req;
  webserialServer.getH["/api/location"](&req);
}
static void getRtc() {
  AsyncWebServerRequest req;
  webserialServer.getH["/api/rtc"](&req);
}

static bool near(double a, double b, double tol) { return fabs(a - b) <= tol; }

// "HH:MM" z pola JSON → minuty; -1 gdy brak.
static int parseHM(const std::string& body, const char* key) {
  std::string k = std::string("\"") + key + "\":\"";
  size_t p = body.find(k);
  if (p == std::string::npos) return -1;
  p += k.size();
  int h = 0, m = 0;
  if (sscanf(body.c_str() + p, "%d:%d", &h, &m) != 2) return -1;
  return h * 60 + m;
}

// Wpis BCD: 2026-10-10 08:00:00 (sobota). Rejestry DS: sek, min, godz, dow, dzień, mies, rok.
static void setRtcRegs(uint8_t sec, uint8_t min, uint8_t hour, uint8_t dow,
                       uint8_t day, uint8_t mon, uint8_t year) {
  Wire.regs[0] = sec; Wire.regs[1] = min; Wire.regs[2] = hour; Wire.regs[3] = dow;
  Wire.regs[4] = day; Wire.regs[5] = mon; Wire.regs[6] = year;
}

static void resetState() {
  g_nvs.clear();
  g_nvsInt.clear();
  g_nvsFail = false;
  g_resp = Resp{};
  latitude = 52.1345;
  longitude = 20.1418;
  sunsetRecalcRequested = false;
  sunsetMinutes = 1140;
  morningMode = 0;
  morningOffsetMin = 0;
  astroHasDawn = false; astroHasSunrise = false;
  astroDawnMin = 0.0; astroSunriseMin = 0.0;
  rtcChip = 0;
  rtcSaveRequested = false;
  snprintf(rtcStatusMsg, sizeof(rtcStatusMsg), "wylaczony");
  g_fakeNow = 0;
  Wire.present = true;
  Wire.nackWrite = false;
  Wire.writes = 0;
  memset(Wire.regs, 0, sizeof(Wire.regs));
}

// ── Lokalizacja (jak w 4.7.0 przed Etapem 6, plus GET z nowymi polami) ──────

static void testGetDefault() {
  resetState();
  getLocation();
  CHECK(g_resp.code == 200, "GET: kod 200");
  CHECK(g_resp.body.find("\"lat\":52.1345") != std::string::npos, "GET: domyslna szerokosc");
  CHECK(g_resp.body.find("\"lon\":20.1418") != std::string::npos, "GET: domyslna dlugosc");
  CHECK(g_resp.body.find("\"sunset\":\"19:00\"") != std::string::npos, "GET: zachod 19:00");
  CHECK(g_resp.body.find("\"timeSynced\":false") != std::string::npos, "GET: brak NTP");
  CHECK(g_resp.body.find("\"mrMode\":0") != std::string::npos, "GET: domyslny tryb stalyPoranek");
  CHECK(g_resp.body.find("\"mrOff\":0") != std::string::npos, "GET: domyslne przesuniecie 0");
  CHECK(g_resp.body.find("\"dawn\":\"--:--\"") != std::string::npos, "GET: brak switu bez czasu");
  CHECK(parseHM(g_resp.body, "morningNow") == 7 * 60, "GET: tryb stały → morningNow = WD 07:00");
}

static void testPostValid() {
  resetState();
  postBody("{\"lat\":50.0647,\"lon\":19.9450}");
  CHECK(g_resp.code == 200, "POST poprawne: kod 200");
  CHECK(near(latitude, 50.0647, 1e-4), "POST poprawne: lat ustawiona");
  CHECK(near(longitude, 19.9450, 1e-4), "POST poprawne: lon ustawiona");
  CHECK(sunsetRecalcRequested, "POST poprawne: flaga przeliczenia zachodu");
  CHECK(g_nvs.count("astro_loc/lat") == 1 && near(g_nvs["astro_loc/lat"], 50.0647, 1e-4),
        "POST poprawne: lat zapisana w NVS");
  CHECK(g_nvs.count("astro_loc/lon") == 1 && near(g_nvs["astro_loc/lon"], 19.9450, 1e-4),
        "POST poprawne: lon zapisana w NVS");
  CHECK(g_nvsInt["astro_loc/mrMode"] == 0 && g_nvsInt["astro_loc/mrOff"] == 0,
        "POST poprawne: tryb i przesuniecie zapisane w NVS");
}

static void testPostRejected() {
  resetState();
  postBody("{\"lat\":95,\"lon\":20}");
  CHECK(g_resp.code == 400, "POST lat=95: kod 400");
  CHECK(near(latitude, 52.1345, 1e-9), "POST lat=95: stara wartosc zachowana");
  CHECK(!sunsetRecalcRequested && g_nvs.empty(), "POST lat=95: brak zapisu i flagi");

  resetState();
  postBody("{\"lat\":52,\"lon\":200}");
  CHECK(g_resp.code == 400, "POST lon=200: kod 400");

  resetState();
  postBody("{\"lat\":\"52.1\",\"lon\":20}");
  CHECK(g_resp.code == 400, "POST lat jako tekst: kod 400");

  resetState();
  postBody("{\"lat\":52.1}");
  CHECK(g_resp.code == 400, "POST brak lon: kod 400");

  resetState();
  postBody("{\"lat\":nan,\"lon\":20}");
  CHECK(g_resp.code == 400, "POST nan: kod 400");
}

static void testPostTooLargeOrChunked() {
  resetState();
  std::string big = "{\"lat\":52,\"lon\":20,\"pad\":\"" + std::string(150, 'x') + "\"}";
  postBody(big.c_str());
  CHECK(g_resp.code == 413, "POST > 128 B: kod 413");
  CHECK(near(latitude, 52.1345, 1e-9), "POST > 128 B: lokalizacja nietknieta");

  // Pierwszy z dwóch kawałków: handler nie odpowiada (czeka na ostatni).
  resetState();
  const char* part = "{\"lat\":50,";
  AsyncWebServerRequest req;
  webserialServer.postH["/api/location"](&req, (uint8_t*)part, strlen(part), 0, 20);
  CHECK(g_resp.sends == 0, "POST kawalek 1/2: brak odpowiedzi");
  CHECK(near(latitude, 52.1345, 1e-9), "POST kawalek 1/2: nic nie zapisano");
}

static void testLoadFromNvs() {
  resetState();
  g_nvs["astro_loc/lat"] = 50.0647f;
  g_nvs["astro_loc/lon"] = 19.9450f;
  loadSiteLocation();
  CHECK(near(latitude, 50.0647, 1e-4), "load: lat z NVS");
  CHECK(near(longitude, 19.9450, 1e-4), "load: lon z NVS");

  resetState();
  g_nvs["astro_loc/lat"] = 200.0f;  // zła wartość w NVS → domyślna
  g_nvs["astro_loc/lon"] = 20.0f;
  loadSiteLocation();
  CHECK(near(latitude, 52.1345, 1e-9), "load: zla wartosc → domyslna lat");

  resetState();
  loadSiteLocation();  // brak wpisów
  CHECK(near(latitude, 52.1345, 1e-9) && near(longitude, 20.1418, 1e-9),
        "load: brak wpisow → domyslne");

  resetState();
  g_nvsFail = true;  // NVS niedostępne → nic nie zmieniamy, bez crasha
  loadSiteLocation();
  CHECK(near(latitude, 52.1345, 1e-9), "load: NVS niedostepne → domyslna");
  g_nvsFail = false;
}

static void testRoundTrip() {
  resetState();
  postBody("{\"lat\":-33.8688,\"lon\":151.2093}");  // zapis
  double la = latitude, lo = longitude;
  latitude = 0; longitude = 0;                      // „restart”
  loadSiteLocation();                               // odczyt z NVS
  CHECK(near(latitude, la, 1e-4) && near(longitude, lo, 1e-4), "round-trip: NVS → restart → odczyt");
}

// ── Poranek: tryb startu rampy wybierany w panelu ───────────────────────────

static void testMorningFixedDefault() {
  resetState();
  g_fakeNow = kMon;  // poniedziałek
  CHECK(morningStartFor(false) == 7 * 60, "poranek stały: dzien roboczy 07:00");
  g_fakeNow = kSat;
  CHECK(morningStartFor(true) == 8 * 60, "poranek stały: weekend 08:00");
}

static void testMorningDawnMode() {
  resetState();
  g_fakeNow = kSat;
  postBody("{\"lat\":52.1345,\"lon\":20.1418,\"mrMode\":1,\"mrOff\":-30}");
  CHECK(g_resp.code == 200, "POST tryb 1 (swit -30): kod 200");
  CHECK(morningMode == 1 && morningOffsetMin == -30, "POST tryb 1: stan ustawiony");
  CHECK(g_nvsInt["astro_loc/mrMode"] == 1 && g_nvsInt["astro_loc/mrOff"] == -30,
        "POST tryb 1: zapis w NVS");
  CHECK(astroHasDawn, "POST tryb 1: swit policzony na dzis");

  getLocation();
  int dawn = parseHM(g_resp.body, "dawn");
  int now  = parseHM(g_resp.body, "morningNow");
  CHECK(dawn >= 5 * 60 + 30 && dawn <= 7 * 60 + 30, "tryb 1: swit w rozsadnym zakresie (Wola, pazdziernik)");
  int expect = ((dawn - 30) % 1440 + 1440) % 1440;
  CHECK(now >= 0 && std::abs(now - expect) <= 1, "tryb 1: morningNow = swit - 30 min");

  // Tryb dotyczy obu typów dni: dzień roboczy i weekend dają ten sam start.
  g_fakeNow = kMon;
  int wd = morningStartFor(false);
  int we = morningStartFor(true);
  CHECK(wd == we, "tryb 1: dzien roboczy i weekend wspolny start");
  CHECK(std::abs(wd - expect) <= 1, "tryb 1: morningStartFor zgodne z GET");
}

static void testMorningSunriseMode() {
  resetState();
  g_fakeNow = kSat;
  postBody("{\"lat\":52.1345,\"lon\":20.1418,\"mrMode\":2,\"mrOff\":15}");
  CHECK(g_resp.code == 200, "POST tryb 2 (wschod +15): kod 200");
  CHECK(astroHasSunrise, "tryb 2: wschod policzony");
  int start = morningStartFor(false);
  int expect = ((int)floor(astro::wrapMinutes(astroSunriseMin) + 0.5) + 15) % 1440;
  CHECK(std::abs(start - expect) <= 1, "tryb 2: start = wschod + 15 min");
  CHECK(start > 6 * 60 && start < 9 * 60, "tryb 2: wschod pazdziernik w 06-09");
}

static void testMorningOffsetKeptWhenOmitted() {
  resetState();
  g_fakeNow = kSat;
  morningOffsetMin = -30;
  postBody("{\"lat\":52.1345,\"lon\":20.1418,\"mrMode\":2}");
  CHECK(g_resp.code == 200 && morningOffsetMin == -30, "POST bez mrOff: przesuniecie zachowane");
  CHECK(morningMode == 2, "POST bez mrOff: tryb ustawiony");
}

static void testMorningRejected() {
  resetState();
  morningMode = 1;
  postBody("{\"lat\":52.1345,\"lon\":20.1418,\"mrMode\":3}");
  CHECK(g_resp.code == 400, "POST mrMode=3: kod 400");
  CHECK(morningMode == 1 && g_nvsInt.empty(), "POST mrMode=3: stan i NVS nietkniete");

  resetState();
  postBody("{\"lat\":52.1345,\"lon\":20.1418,\"mrMode\":1.5}");
  CHECK(g_resp.code == 400, "POST mrMode=1.5: kod 400");

  resetState();
  postBody("{\"lat\":52.1345,\"lon\":20.1418,\"mrOff\":200}");
  CHECK(g_resp.code == 400, "POST mrOff=200: kod 400");

  resetState();
  postBody("{\"lat\":52.1345,\"lon\":20.1418,\"mrOff\":\"x\"}");
  CHECK(g_resp.code == 400, "POST mrOff tekst: kod 400");
  CHECK(!sunsetRecalcRequested, "POST odrzucony poranek: brak przeliczenia");
}

static void testLoadMorningFromNvs() {
  resetState();
  g_nvsInt["astro_loc/mrMode"] = 2;
  g_nvsInt["astro_loc/mrOff"] = -45;
  loadSiteLocation();
  CHECK(morningMode == 2 && morningOffsetMin == -45, "load poranek: tryb i przesuniecie z NVS");

  resetState();
  g_nvsInt["astro_loc/mrMode"] = 7;   // poza zakresem → stały
  g_nvsInt["astro_loc/mrOff"] = 999;  // poza zakresem → 0
  loadSiteLocation();
  CHECK(morningMode == 0 && morningOffsetMin == 0, "load poranek: wartosci spoza zakresu → domyslne");
}

// ── RTC: wybór modułu w panelu, odczyt przy starcie, zapis z NTP ───────────

static void testRtcGetDefault() {
  resetState();
  getRtc();
  CHECK(g_resp.code == 200, "GET /api/rtc: kod 200");
  CHECK(g_resp.body.find("\"chip\":0") != std::string::npos, "GET /api/rtc: domyslnie wylaczony");
  CHECK(g_resp.body.find("\"status\":\"wylaczony\"") != std::string::npos, "GET /api/rtc: status wylaczony");
  CHECK(g_resp.body.find("\"sysSynced\":false") != std::string::npos, "GET /api/rtc: brak czasu systemowego");
}

static void testRtcPost() {
  resetState();
  postRtc("{\"chip\":1307}");
  CHECK(g_resp.code == 200, "POST /api/rtc 1307: kod 200");
  CHECK(rtcChip == 1307, "POST /api/rtc 1307: wybor zapisany");
  CHECK(rtcSaveRequested, "POST /api/rtc 1307: zadanie zapisu do RTC");
  CHECK(g_nvsInt["astro_loc/rtcChip"] == 1307, "POST /api/rtc 1307: zapis w NVS");

  resetState();
  postRtc("{\"chip\":3231}");
  CHECK(g_resp.code == 200 && rtcChip == 3231, "POST /api/rtc 3231: kod 200 i wybor");

  resetState();
  postRtc("{\"chip\":0}");
  CHECK(g_resp.code == 200 && rtcChip == 0 && !rtcSaveRequested, "POST /api/rtc 0: wylaczenie bez zapisu");

  resetState();
  postRtc("{\"chip\":1234}");
  CHECK(g_resp.code == 400, "POST /api/rtc 1234: kod 400");
  CHECK(rtcChip == 0 && !rtcSaveRequested && g_nvsInt.empty(), "POST /api/rtc 1234: stan nietkniety");
}

static void testRtcBootRestore() {
  resetState();
  rtcChip = 1307;
  setRtcRegs(0x00, 0x00, 0x08, 0x07, 0x10, 0x10, 0x26);  // 2026-10-10 08:00:00
  sunsetMinutes = 0;
  rtcRestoreAtBoot();
  CHECK(g_fakeNow == kSat, "start z RTC: czas systemowy z RTC");
  CHECK(std::string(rtcStatusMsg) == "czas z RTC (bez NTP)", "start z RTC: status");
  CHECK(sunsetMinutes == 1140, "start z RTC: zachod przeliczony (19:00 CEST)");
  CHECK(astroHasDawn, "start z RTC: swit na dzis");

  // Czas już poprawny (NTP/wcześniej) → RTC nie nadpisuje.
  resetState();
  rtcChip = 1307;
  g_fakeNow = kMon;
  setRtcRegs(0x00, 0x00, 0x08, 0x07, 0x10, 0x10, 0x26);
  rtcRestoreAtBoot();
  CHECK(g_fakeNow == kMon, "start z RTC: czas poprawny nie nadpisany");

  // RTC wyłączony → brak odczytu.
  resetState();
  rtcChip = 0;
  setRtcRegs(0x00, 0x00, 0x08, 0x07, 0x10, 0x10, 0x26);
  rtcRestoreAtBoot();
  CHECK(g_fakeNow == 0 && std::string(rtcStatusMsg) == "wylaczony", "RTC wylaczony: brak zmian");

  // Nieskalibrowany zegar (2000-01-01) → odrzucony.
  resetState();
  rtcChip = 1307;
  setRtcRegs(0x00, 0x00, 0x00, 0x06, 0x01, 0x01, 0x00);
  rtcRestoreAtBoot();
  CHECK(g_fakeNow == 0, "RTC 2000-01-01: odrzucony");
  CHECK(std::string(rtcStatusMsg) == "czas w RTC nieprawidlowy", "RTC 2000-01-01: status");

  // Zatrzymany oscylator (CH=1) → brak odczytu.
  resetState();
  rtcChip = 1307;
  setRtcRegs(0x80, 0x00, 0x08, 0x07, 0x10, 0x10, 0x26);
  rtcRestoreAtBoot();
  CHECK(g_fakeNow == 0 && std::string(rtcStatusMsg) == "brak odczytu (modul/zegar)",
        "RTC CH=1: brak odczytu");

  // Brak modułu na magistrali.
  resetState();
  rtcChip = 3231;
  Wire.present = false;
  rtcRestoreAtBoot();
  CHECK(g_fakeNow == 0 && std::string(rtcStatusMsg) == "brak odczytu (modul/zegar)",
        "brak modulu: status bledu, bez crasha");
}

static void testRtcSave() {
  resetState();
  rtcChip = 1307;
  g_fakeNow = kSat;
  rtcSaveFromSystem();
  CHECK(std::string(rtcStatusMsg) == "zapisano czas do RTC", "zapis: status OK");
  CHECK(Wire.regs[0] == 0x00 && Wire.regs[1] == 0x00 && Wire.regs[2] == 0x08,
        "zapis: godzina 08:00:00 w BCD");
  CHECK(Wire.regs[4] == 0x10 && Wire.regs[5] == 0x10 && Wire.regs[6] == 0x26,
        "zapis: 10.10.26 w BCD");

  // Zapis, a potem „restart” bez NTP → ten sam czas wraca z RTC.
  g_fakeNow = 0;
  rtcRestoreAtBoot();
  CHECK(g_fakeNow == kSat, "round-trip RTC: zapis → restart → odczyt");

  // Brak czasu systemowego → nic nie zapisujemy.
  resetState();
  rtcChip = 1307;
  rtcSaveFromSystem();
  CHECK(std::string(rtcStatusMsg) == "brak czasu systemowego" && Wire.writes == 0,
        "zapis bez czasu: brak zapisu do RTC");

  // Błąd zapisu (NACK danych) → status błędu.
  resetState();
  rtcChip = 1307;
  g_fakeNow = kSat;
  Wire.nackWrite = true;
  rtcSaveFromSystem();
  CHECK(std::string(rtcStatusMsg) == "blad zapisu do RTC (modul?)", "zapis z NACK: status bledu");

  // Brak modułu → błąd, bez crasha.
  resetState();
  rtcChip = 3231;
  g_fakeNow = kSat;
  Wire.present = false;
  rtcSaveFromSystem();
  CHECK(std::string(rtcStatusMsg) == "blad zapisu do RTC (modul?)", "zapis bez modulu: status bledu");

  // RTC wyłączony → nic.
  resetState();
  g_fakeNow = kSat;
  rtcSaveFromSystem();
  CHECK(Wire.writes == 0 && std::string(rtcStatusMsg) == "wylaczony", "RTC wylaczony: zapis pominiety");
}

static void testRtcPostThenSave() {
  // Panel wybiera moduł → loop() zapisuje czas (flaga rtcSaveRequested).
  resetState();
  g_fakeNow = kMon;
  postRtc("{\"chip\":1307}");
  CHECK(rtcSaveRequested, "panel: flaga zapisu ustawiona");
  if (rtcSaveRequested) { rtcSaveRequested = false; rtcSaveFromSystem(); }
  CHECK(std::string(rtcStatusMsg) == "zapisano czas do RTC", "panel: zapis z loop() udany");
  CHECK(Wire.regs[4] == 0x12, "panel: poniedzialek 12 w BCD (dzien)");
  getRtc();
  CHECK(g_resp.body.find("\"status\":\"zapisano czas do RTC\"") != std::string::npos,
        "GET /api/rtc: status po zapisie");
  CHECK(g_resp.body.find("\"sysSynced\":true") != std::string::npos, "GET /api/rtc: czas systemowy OK");
}

int main() {
  registerLocation();  // rejestracja endpointów z Ryby (wyciągniętych)
  testGetDefault();
  testPostValid();
  testPostRejected();
  testPostTooLargeOrChunked();
  testLoadFromNvs();
  testRoundTrip();
  testMorningFixedDefault();
  testMorningDawnMode();
  testMorningSunriseMode();
  testMorningOffsetKeptWhenOmitted();
  testMorningRejected();
  testLoadMorningFromNvs();
  testRtcGetDefault();
  testRtcPost();
  testRtcBootRestore();
  testRtcSave();
  testRtcPostThenSave();
  std::printf("location: %d PASS, %d FAIL\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
