// ═══════════════════════════════════════════════════════════════════════════
//  test_astro.cpp — testy modułu astro (4.7.0, Etap 6 pkt 1), ASan + UBSan.
//  Wartości referencyjne: astral 3.2 (firmware/tests/host/astro_ref_gen.py),
//  minuty od północy czasu lokalnego. Tolerancja 2 min (algorytm NOAA vs astral).
// ═══════════════════════════════════════════════════════════════════════════
#include "astro.h"

#include <cmath>
#include <cstdio>

static int g_pass = 0, g_fail = 0;
#define CHECK(cond, name)                                                     \
  do {                                                                        \
    if (cond) { ++g_pass; }                                                   \
    else { ++g_fail; std::printf("FAIL: %s  (linia %d)\n", name, __LINE__); } \
  } while (0)

struct Ref {
  const char* name;
  double lat, lon;
  int tzMin, y, m, d;
  double dawn, sunrise, sunset, dusk;
};

static const Ref REF[] = {
    {"Krakow", 50.0647, 19.9450, 120, 2026, 10, 10, 381.22, 414.28, 1079.30, 1112.30},
    {"Krakow", 50.0647, 19.9450, 60, 2026, 3, 21, 308.53, 341.42, 1074.27, 1107.25},
    {"Krakow", 50.0647, 19.9450, 120, 2026, 6, 21, 225.22, 270.88, 1253.17, 1298.83},
    {"Krakow", 50.0647, 19.9450, 60, 2026, 12, 21, 417.35, 456.70, 939.82, 979.15},
    {"Warszawa", 52.2297, 21.0122, 120, 2026, 10, 10, 377.68, 412.35, 1072.63, 1107.23},
    {"Warszawa", 52.2297, 21.0122, 60, 2026, 3, 21, 302.35, 336.83, 1070.40, 1104.98},
    {"Warszawa", 52.2297, 21.0122, 120, 2026, 6, 21, 204.28, 254.75, 1260.75, 1311.23},
    {"Warszawa", 52.2297, 21.0122, 60, 2026, 12, 21, 421.18, 463.27, 924.70, 966.78},
    {"Gdansk", 54.3520, 18.6466, 120, 2026, 10, 10, 387.90, 424.35, 1079.48, 1115.85},
    {"Gdansk", 54.3520, 18.6466, 60, 2026, 3, 21, 309.67, 345.92, 1080.32, 1116.70},
    {"Gdansk", 54.3520, 18.6466, 120, 2026, 6, 21, 193.95, 250.87, 1283.57, 1340.48},
    {"Gdansk", 54.3520, 18.6466, 60, 2026, 12, 21, 439.47, 484.82, 922.10, 967.43},
    {"Tromso", 69.6496, 18.9560, 120, 2026, 10, 10, 393.17, 454.98, 1045.58, 1107.15},
    {"Tromso", 69.6496, 18.9560, 60, 2026, 3, 21, 278.35, 339.82, 1085.02, 1146.98},
    {"Sydney", -33.8688, 151.2093, 660, 2026, 10, 10, 355.22, 380.97, 1144.10, 1169.92},
    {"Sydney", -33.8688, 151.2093, 660, 2026, 3, 21, 393.52, 418.92, 1145.37, 1170.73},
    {"Sydney", -33.8688, 151.2093, 600, 2026, 6, 21, 391.93, 420.18, 1013.58, 1041.85},
    {"Sydney", -33.8688, 151.2093, 660, 2026, 12, 21, 311.15, 340.88, 1205.18, 1234.90},
    {"Dom", 52.1345, 20.1418, 120, 2026, 10, 10, 381.13, 415.73, 1076.22, 1110.73},
    {"Dom", 52.1345, 20.1418, 60, 2026, 3, 21, 305.92, 340.32, 1073.87, 1108.38},
    {"Dom", 52.1345, 20.1418, 120, 2026, 6, 21, 208.57, 258.78, 1263.68, 1313.92},
    {"Dom", 52.1345, 20.1418, 60, 2026, 12, 21, 424.30, 466.25, 928.68, 970.63},
};

static bool near(double a, double b, double tol) { return std::fabs(a - b) <= tol; }

static void testReference() {
  const double TOL = 2.0;
  int n = 0;
  for (const Ref& r : REF) {
    astro::Times t{};
    char name[96];
    std::snprintf(name, sizeof(name), "%s %04d-%02d-%02d", r.name, r.y, r.m, r.d);
    CHECK(astro::compute(r.y, r.m, r.d, r.lat, r.lon, r.tzMin, t), name);
    CHECK(t.hasDawn && t.hasSunrise && t.hasSunset && t.hasDusk, name);
    CHECK(near(t.dawnMin, r.dawn, TOL), name);
    CHECK(near(t.sunriseMin, r.sunrise, TOL), name);
    CHECK(near(t.sunsetMin, r.sunset, TOL), name);
    CHECK(near(t.duskMin, r.dusk, TOL), name);
    ++n;
  }
  std::printf("  referencje: %d dat/miejsc\n", n);
}

static void testOrderingAndDayLength() {
  // Kolejność: świt < wschód < zachód < zmierzch, w każdym dniu.
  for (const Ref& r : REF) {
    astro::Times t{};
    astro::compute(r.y, r.m, r.d, r.lat, r.lon, r.tzMin, t);
    CHECK(t.dawnMin < t.sunriseMin && t.sunriseMin < t.sunsetMin && t.sunsetMin < t.duskMin,
          "kolejnosc zdarzen");
  }
  // Długość dnia w Krakowie 21.06 ~ 16 h 22 min (astral: 982 min); tolerancja 3 min.
  astro::Times t{};
  astro::compute(2026, 6, 21, 50.0647, 19.9450, 120, t);
  CHECK(near(t.sunsetMin - t.sunriseMin, 982.0, 3.0), "dlugosc dnia Krakow 21.06");
  // Przesilenie zimowe krótsze niż letnie.
  astro::Times w{};
  astro::compute(2026, 12, 21, 50.0647, 19.9450, 60, w);
  CHECK(w.sunsetMin - w.sunriseMin < t.sunsetMin - t.sunriseMin, "zima krotsza niz lato");
}

static void testTimezoneShift() {
  astro::Times a{}, b{};
  astro::compute(2026, 10, 10, 50.0647, 19.9450, 120, a);
  astro::compute(2026, 10, 10, 50.0647, 19.9450, 60, b);
  CHECK(near(a.sunriseMin - b.sunriseMin, 60.0, 1e-9), "tzMin +60 przesuwa wschod o 60 min");
  CHECK(near(a.sunsetMin - b.sunsetMin, 60.0, 1e-9), "tzMin +60 przesuwa zachod o 60 min");
}

static void testPolar() {
  // Tromsø 21.06: słońce nie zachodzi (brak wschodu i zachodu).
  astro::Times s{};
  CHECK(astro::compute(2026, 6, 21, 69.6496, 18.9560, 120, s), "polar lato: compute ok");
  CHECK(!s.hasSunrise && !s.hasSunset, "polar lato: brak wschodu i zachodu");
  // Tromsø 21.12: słońce nie wschodzi.
  astro::Times z{};
  CHECK(astro::compute(2026, 12, 21, 69.6496, 18.9560, 60, z), "polar zima: compute ok");
  CHECK(!z.hasSunrise && !z.hasSunset, "polar zima: brak wschodu i zachodu");
}

static void testInvalidInput() {
  astro::Times t{};
  CHECK(!astro::compute(2026, 13, 1, 50, 19, 120, t), "zly miesiac");
  CHECK(!astro::compute(2026, 1, 0, 50, 19, 120, t), "zly dzien");
  CHECK(!astro::compute(2026, 1, 1, 91, 19, 120, t), "szerokosc > 90");
  CHECK(!astro::compute(2026, 1, 1, 50, 181, 120, t), "dlugosc > 180");
}

static void testWrap() {
  CHECK(near(astro::wrapMinutes(1500), 60, 1e-9), "wrap > 1440");
  CHECK(near(astro::wrapMinutes(-30), 1410, 1e-9), "wrap < 0");
  CHECK(near(astro::wrapMinutes(720), 720, 1e-9), "wrap bez zmian");
}

static void testLocationValidation() {
  CHECK(astro::validLocation(52.1345, 20.1418), "lok: dom poprawna");
  CHECK(astro::validLocation(-90, -180) && astro::validLocation(90, 180), "lok: granice");
  CHECK(!astro::validLocation(90.001, 0), "lok: lat > 90");
  CHECK(!astro::validLocation(-91, 0), "lok: lat < -90");
  CHECK(!astro::validLocation(0, 180.5), "lok: lon > 180");
  CHECK(!astro::validLocation(0, -181), "lok: lon < -180");
  CHECK(!astro::validLocation(NAN, 20), "lok: NaN lat");
  CHECK(!astro::validLocation(52, INFINITY), "lok: inf lon");
}

static void testJsonNumber() {
  double v = 0;
  CHECK(astro::jsonNumber("{\"lat\":52.1345,\"lon\":20.1418}", "lat", v) && near(v, 52.1345, 1e-9), "json: lat");
  CHECK(astro::jsonNumber("{\"lat\":52.1345,\"lon\":20.1418}", "lon", v) && near(v, 20.1418, 1e-9), "json: lon");
  CHECK(astro::jsonNumber("{ \"lat\" :  -33.5 }", "lat", v) && near(v, -33.5, 1e-9), "json: spacje, ujemna");
  CHECK(!astro::jsonNumber("{\"lon\":20}", "lat", v), "json: brak klucza");
  CHECK(!astro::jsonNumber("{\"lat\":\"52.1\"}", "lat", v), "json: liczba w cudzyslowie odrzucona");
  CHECK(!astro::jsonNumber("{\"lat\":nan}", "lat", v), "json: nan odrzucone");
  CHECK(!astro::jsonNumber("{\"lat\":inf}", "lat", v), "json: inf odrzucone");
  CHECK(!astro::jsonNumber("{\"lat\" 52}", "lat", v), "json: brak dwukropka");
  CHECK(!astro::jsonNumber(nullptr, "lat", v), "json: null body");
}

int main() {
  testLocationValidation();
  testJsonNumber();
  testReference();
  testOrderingAndDayLength();
  testTimezoneShift();
  testPolar();
  testInvalidInput();
  testWrap();
  std::printf("astro: %d PASS, %d FAIL\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
