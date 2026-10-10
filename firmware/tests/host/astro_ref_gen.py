#!/usr/bin/env python3
# ═══════════════════════════════════════════════════════════════════════════
#  astro_ref_gen.py — generuje tabelę referencyjną dla test_astro.cpp
#  z biblioteki `astral` (PyPI, pip install astral). NIE jest wymagany w testach
#  hostowych — tylko do odtworzenia wartości wpisanych w test_astro.cpp.
#  Wypisuje wiersze C++: {name, lat, lon, tzMin, y, m, d, dawn, sunrise, sunset, dusk}
#  Czasy w minutach od północy czasu lokalnego; -1 = brak zdarzenia (polarna noc/dzień).
# ═══════════════════════════════════════════════════════════════════════════
import datetime

from astral import LocationInfo
from astral.sun import sun

PLACES = [
    ("Krakow", 50.0647, 19.9450, "Europe/Warsaw", 120),
    ("Warszawa", 52.2297, 21.0122, "Europe/Warsaw", 120),
    ("Gdansk", 54.3520, 18.6466, "Europe/Warsaw", 120),
    ("Tromso", 69.6496, 18.9560, "Europe/Oslo", 120),
    ("Sydney", -33.8688, 151.2093, "Australia/Sydney", 660),
    # Lokalizacja z kodu Ryby (latitude/longitude w Ryby_LED_fi_S3.cpp):
    ("Dom", 52.1345, 20.1418, "Europe/Warsaw", 120),
]
DATES = [
    (2026, 10, 10),
    (2026, 3, 21),
    (2026, 6, 21),
    (2026, 12, 21),
]


def minutes(dt):
    return dt.hour * 60 + dt.minute + dt.second / 60.0


for name, lat, lon, tzname, tzmin in PLACES:
    loc = LocationInfo(name, "", tzname, lat, lon)
    for (y, m, d) in DATES:
        day = datetime.date(y, m, d)
        vals = {}
        try:
            s = sun(loc.observer, date=day, tzinfo=tzname)
            vals["_none"] = False
            for k in ("dawn", "sunrise", "sunset", "dusk"):
                vals[k] = minutes(s[k])
        except ValueError:
            # astral: "Sun never reaches ..." → brak zdarzenia (polarna noc/dzień)
            vals["_none"] = True
            for k in ("dawn", "sunrise", "sunset", "dusk"):
                vals[k] = -1
        # Polarny dzień/noc: astral może dać część zdarzeń; porównujemy tylko to, co jest.
        # Offset lokalny z samych danych astral (zmiana czasu zależy od daty!).
        off = int(s["sunrise"].utcoffset().total_seconds() // 60) if not vals.get("_none") else tzmin
        print('    {"%s", %.4f, %.4f, %d, %d, %d, %d, %.2f, %.2f, %.2f, %.2f},' % (
            name, lat, lon, off, y, m, d,
            vals["dawn"], vals["sunrise"], vals["sunset"], vals["dusk"]))
