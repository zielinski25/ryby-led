// ═══════════════════════════════════════════════════════════════════════════
//  test_location.cpp — [4.7.0 ASTRO] lokalizacja do zachodu z panelu (Etap 6).
//  Kompiluje PRAWDZIWE fragmenty Ryby_LED_fi_S3.cpp (wyciągnięte przez
//  check_location.sh → loc_extract.inc): zmienne, loadSiteLocation/saveSiteLocation
//  oraz endpointy GET/POST /api/location. Zależności: loc_mock.h + astro.cpp.
// ═══════════════════════════════════════════════════════════════════════════
#include "loc_mock.h"
#include "loc_extract.inc"

static int g_pass = 0, g_fail = 0;
#define CHECK(cond, name)                                              \
  do {                                                                 \
    if (cond) { g_pass++; }                                            \
    else { g_fail++; std::printf("FAIL: %s (linia %d)\n", name, __LINE__); } \
  } while (0)

static void postBody(const char* body, size_t index = 0, size_t total = 0) {
  if (total == 0) total = strlen(body);
  AsyncWebServerRequest req;
  auto& h = webserialServer.postH["/api/location"];
  h(&req, (uint8_t*)body, strlen(body), index, total);
}

static void getLocation() {
  AsyncWebServerRequest req;
  webserialServer.getH["/api/location"](&req);
}

static bool near(double a, double b, double tol) { return fabs(a - b) <= tol; }

static void resetState() {
  g_nvs.clear();
  g_nvsFail = false;
  g_resp = Resp{};
  latitude = 52.1345;
  longitude = 20.1418;
  sunsetRecalcRequested = false;
  sunsetMinutes = 1140;
}

static void testGetDefault() {
  resetState();
  getLocation();
  CHECK(g_resp.code == 200, "GET: kod 200");
  CHECK(g_resp.body.find("\"lat\":52.1345") != std::string::npos, "GET: domyslna szerokosc");
  CHECK(g_resp.body.find("\"lon\":20.1418") != std::string::npos, "GET: domyslna dlugosc");
  CHECK(g_resp.body.find("\"sunset\":\"19:00\"") != std::string::npos, "GET: zachod 19:00");
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
  std::string big(200, ' ');
  big = "{\"lat\":52,\"lon\":20,\"pad\":\"" + std::string(150, 'x') + "\"}";
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

int main() {
  registerLocation();  // rejestracja endpointów z Ryby (wyciągniętych)
  testGetDefault();
  testPostValid();
  testPostRejected();
  testPostTooLargeOrChunked();
  testLoadFromNvs();
  testRoundTrip();
  std::printf("location: %d PASS, %d FAIL\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
