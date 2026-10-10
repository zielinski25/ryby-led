// ═══════════════════════════════════════════════════════════════════════════
//  astro.h — [4.7.0 ASTRO] Etap 6 planu upgrade, pkt 1: efemerydy słoneczne offline
//
//  Wschód, zachód, świt cywilny i zmierzch cywilny z równań NOAA (dokładność
//  ~1 min dla szerokości Polski). Bez sieci, bez RTC — wystarcza czas z NTP.
//
//  STATUS: zachód słońca podłączony (obliczZachodSlonca → sunsetMinutes →
//  wieczorna rampa). Świt/wschód NIE podłączone (Ramp Arbiter i MIN LUX nie
//  są dotykane). Lokalizacja ustawiana w panelu i trzymana w NVS (docs/06, §4).
//  Testy: firmware/tests/host/test_astro.cpp.
// ═══════════════════════════════════════════════════════════════════════════
#pragma once

namespace astro {

struct Times {
  bool   hasDawn;     // świt cywilny (słońce −6°) — false: polarna noc/dzień
  bool   hasSunrise;  // wschód (−0,833°)
  bool   hasSunset;   // zachód (−0,833°)
  bool   hasDusk;     // zmierzch cywilny (−6°)
  double dawnMin;     // minuty od północy czasu LOKALNEGO (może wyjść poza [0,1440))
  double sunriseMin;
  double sunsetMin;
  double duskMin;
};

// Oblicza zdarzenia dla daty lokalnej y-m-d.
//   latDeg  — szerokość (+N), np. 50.0647 (Kraków)
//   lonDeg  — długość (+E), np. 19.9450
//   tzMin   — offset lokalny względem UTC w minutach (Polska latem: 120, zimą: 60)
// Zwraca false tylko przy złych danych wejściowych (miesiąc/dzień poza zakresem).
bool compute(int y, int m, int d, double latDeg, double lonDeg, int tzMin, Times& out);

// Minuty z zakresu [0,1440) dla wartości z Times (do porównań z harmonogramem).
double wrapMinutes(double min);

// Czy para (szerokość, długość) jest poprawną lokalizacją: skończona liczba,
// lat w [-90,90], lon w [-180,180]. Ustawienie z panelu przechodzi tę walidację.
bool validLocation(double latDeg, double lonDeg);

// Odczytuje pole liczbowe "key":<liczba> z ciała JSON (prosty parser bez zależności).
// Zwraca false, gdy klucza nie ma, wartość nie jest liczbą albo jest nieskończona.
bool jsonNumber(const char* body, const char* key, double& out);

}  // namespace astro
