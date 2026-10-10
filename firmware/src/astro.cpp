// ═══════════════════════════════════════════════════════════════════════════
//  astro.cpp — [4.7.0 ASTRO] patrz astro.h. Równania NOAA Solar Calculator
//  (uproszczone Meeus), bez zależności poza <math.h>.
// ═══════════════════════════════════════════════════════════════════════════
#include "astro.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

namespace astro {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double ZENITH_SUNRISE = 90.833;  // refrakcja + promień tarczy
constexpr double ZENITH_CIVIL   = 96.0;    // −6° (świt/zmierzch cywilny)

inline double rad(double deg) { return deg * kPi / 180.0; }
inline double deg(double r)   { return r * 180.0 / kPi; }

// Julian Day o 0:00 UT dla daty (algorytm Meeus).
double jd0Of(int y, int m, int d) {
  if (m <= 2) { y -= 1; m += 12; }
  int A = y / 100;
  int B = 2 - A + A / 4;
  return floor(365.25 * (y + 4716)) + floor(30.6001 * (m + 1)) + d + B - 1524.5;
}

// Deklinacja Słońca [rad] i równanie czasu [min] dla danego JD.
void solar(double jd, double& decl, double& eqTimeMin) {
  double T = (jd - 2451545.0) / 36525.0;
  double L0 = fmod(280.46646 + T * (36000.76983 + T * 0.0003032), 360.0);
  double M  = 357.52911 + T * (35999.05029 - 0.0001537 * T);
  double e  = 0.016708634 - T * (0.000042037 + 0.0000001267 * T);
  double Mr = rad(M);
  double C  = sin(Mr) * (1.914602 - T * (0.004817 + 0.000014 * T))
            + sin(2 * Mr) * (0.019993 - 0.000101 * T)
            + sin(3 * Mr) * 0.000289;
  double trueLong = L0 + C;
  double omega = 125.04 - 1934.136 * T;
  double appLong = trueLong - 0.00569 - 0.00478 * sin(rad(omega));
  double eps0 = 23.0 + (26.0 + (21.448 - T * (46.815 + T * (0.00059 - T * 0.001813))) / 60.0) / 60.0;
  double eps = eps0 + 0.00256 * cos(rad(omega));
  decl = asin(sin(rad(eps)) * sin(rad(appLong)));

  double y = tan(rad(eps) / 2.0);
  y *= y;
  double L0r = rad(L0);
  double eq = y * sin(2 * L0r)
            - 2 * e * sin(Mr)
            + 4 * e * y * sin(Mr) * cos(2 * L0r)
            - 0.5 * y * y * sin(4 * L0r)
            - 1.25 * e * e * sin(2 * Mr);
  eqTimeMin = deg(eq) * 4.0;
}

// Zdarzenie przy danym zenicie. Zwraca false, gdy Słońce nie przecina tego zenitu tego dnia.
// Wynik: minuty UT od 0:00 UT daty (może być <0 lub >1440 przy długościach ~180°).
bool event(double jd0, double latDeg, double lonDeg, double zenithDeg, bool rise, double& outUtcMin) {
  double latR = rad(latDeg);
  double t = 720.0 - 4.0 * lonDeg;  // startowe przybliżenie: południe słoneczne
  for (int it = 0; it < 3; it++) {
    double decl, eqT;
    solar(jd0 + t / 1440.0, decl, eqT);
    double cosH = cos(rad(zenithDeg)) / (cos(latR) * cos(decl)) - tan(latR) * tan(decl);
    if (cosH > 1.0 || cosH < -1.0) return false;
    double H = deg(acos(cosH));
    t = rise ? (720.0 - 4.0 * (lonDeg + H) - eqT)
             : (720.0 - 4.0 * (lonDeg - H) - eqT);
  }
  outUtcMin = t;
  return true;
}

}  // namespace

bool compute(int y, int m, int d, double latDeg, double lonDeg, int tzMin, Times& out) {
  if (m < 1 || m > 12 || d < 1 || d > 31) return false;
  if (latDeg < -90.0 || latDeg > 90.0 || lonDeg < -180.0 || lonDeg > 180.0) return false;

  double jd0 = jd0Of(y, m, d);
  double u;
  out = Times{false, false, false, false, 0, 0, 0, 0};

  if (event(jd0, latDeg, lonDeg, ZENITH_CIVIL, true, u)) {
    out.hasDawn = true;
    out.dawnMin = u + tzMin;
  }
  if (event(jd0, latDeg, lonDeg, ZENITH_SUNRISE, true, u)) {
    out.hasSunrise = true;
    out.sunriseMin = u + tzMin;
  }
  if (event(jd0, latDeg, lonDeg, ZENITH_SUNRISE, false, u)) {
    out.hasSunset = true;
    out.sunsetMin = u + tzMin;
  }
  if (event(jd0, latDeg, lonDeg, ZENITH_CIVIL, false, u)) {
    out.hasDusk = true;
    out.duskMin = u + tzMin;
  }
  return true;
}

double wrapMinutes(double min) {
  double r = fmod(min, 1440.0);
  if (r < 0) r += 1440.0;
  return r;
}

bool validLocation(double latDeg, double lonDeg) {
  if (!isfinite(latDeg) || !isfinite(lonDeg)) return false;
  return latDeg >= -90.0 && latDeg <= 90.0 && lonDeg >= -180.0 && lonDeg <= 180.0;
}

bool jsonNumber(const char* body, const char* key, double& out) {
  if (!body || !key) return false;
  char pat[64];
  if (snprintf(pat, sizeof(pat), "\"%s\"", key) >= (int)sizeof(pat)) return false;
  const char* p = strstr(body, pat);
  if (!p) return false;
  p += strlen(pat);
  while (*p == ' ' || *p == '\t') p++;
  if (*p != ':') return false;
  p++;
  while (*p == ' ' || *p == '\t') p++;
  // Liczba musi zaczynać się cyfrą lub minusem; strtod nie może przyjąć "nan"/"inf".
  if (!(*p == '-' || (*p >= '0' && *p <= '9'))) return false;
  char* end = nullptr;
  double v = strtod(p, &end);
  if (end == p || !isfinite(v)) return false;
  out = v;
  return true;
}

}  // namespace astro
