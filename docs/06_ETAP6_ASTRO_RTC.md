# Etap 6 — astronomia, zegar odporny i karta „Dzień”

Plan: `docs/02_PLAN_UPGRADE_ryby_led.md`, ETAP 6 (P2). Ten dokument opisuje stan po 4.7.0.

## 0. Co już było w kodzie przed 4.7.0 (ustalone odczytem)

- `obliczZachodSlonca(rok, mies, dzien, szer, dlug)` liczyła zachód uproszczonym wzorem
  (~0,9856·t, bez poprawek). Porównanie z `astral` 3.2 dla lokalizacji z kodu
  (52,1345 / 20,1418) pokazało **systematyczne odchylenie do −4,4 min** (zima/wiosna).
- `sunsetMinutes` jest wyliczane przy starcie i w cyklu dobowym z NTP (dwa miejsca w kodzie),
  z przesunięciem strefy (`timezoneOffsetMinutes`, zmiana czasu z `isDaylightSaving`).
  Brak NTP → sztywne 19:00 (jak dotąd).
- Wieczorna rampa startuje z `eveningOnStart = sunsetMinutes + EVENING_ON_BEFORE_SUNSET_MIN`.
  Przesunięcie jest już ustawialne w panelu i trzymane w EEPROM (domyślnie −60 min).
  **Rampa wieczorna jest więc już względna do zachodu.** Nie ma potrzeby nowego trybu.
- Współrzędne to stałe w kodzie: `latitude = 52.1345`, `longitude = 20.1418`.
  (Plan wcześniej zakładał Kraków 50,0647 / 19,9450 — to było błędne założenie.)

## 1. Astronomia — moduł i podłączenie zachodu (4.7.0)

- `firmware/src/astro.h/.cpp`: `astro::compute(y, m, d, lat, lon, tzMin, Times&)`.
  Zwraca świt cywilny (−6°), wschód i zachód (−0,833°), zmierzch cywilny, w minutach
  od północy czasu lokalnego. Flagi `has*` mówią, czy zdarzenie istnieje (polarny dzień/noc).
- Algorytm: NOAA (deklinacja, równanie czasu, 3 iteracje czasu zdarzenia).
- **Podłączone do zachodu:** `obliczZachodSlonca` wywołuje `astro::compute` (tzMin = 0,
  wynik w minutach UTC, jak dotąd; `-1` gdy brak zachodu). Sygnatura i kontrakt bez zmian,
  więc `sunsetMinutes`, strefa, zmiana czasu i rampa wieczorna działają jak wcześniej.
  Efekt: zachód zgodny z `astral` ±2 min zamiast odchylenia do −4,4 min.
- Testy:
  - `test_astro.cpp`: 22 pary data×miejsce wobec `astral` 3.2 (`astro_ref_gen.py`),
    w tym lokalizacja domowa 52,1345 / 20,1418. Tolerancja 2 min. 186 PASS, 0 FAIL (ASan + UBSan).
  - `check_integration_hl.sh`: wyciągnięte z pliku Ryby ciało `obliczZachodSlonca` kompiluje się
    z `astro.cpp` i trafia w wartości UTC z `astral` (±2 min), plus polarna noc → −1.
- **Świt, wschód i zmierzch nie są podłączone.** Rampa poranna nadal ma stałą godzinę.
  Ewentualne podłączenie to osobna decyzja (sekcja 4).

## 2. Zegar odporny (RTC) — CZEKA NA IDENTYFIKACJĘ MODUŁU

Dziś harmonogram zależy wyłącznie od NTP. Bez routera i po dłuższym restarcie czas jest
nieznany. Opcje z planu:
- **(a) RTC na I2C** (RTClib obsługuje DS1307 i DS3231). Rekomendowane.
- (b) „ostatni znany czas + drift” z logowaniem niepewności — bez dokupowania.

Właściciel ma moduł DS, ale nie wie, czy to DS1307, czy DS3231, i nie jest zamontowany.
Rozróżnienie: najpewniej po nadruku na układzie RTC (napis „DS1307” lub „DS3231”).
Parametry z kart katalogowych: DS1307 ma zewnętrzny kwarc 32,768 kHz i bez kompensacji
temperatury (typowo ±20 ppm); DS3231 ma wbudowany kwarc z kompensacją (±2 ppm).
Obie wersje obsługuje ta sama biblioteka RTClib, więc wybór nie blokuje integracji.
**Nie zaimplementowano.** Wymaga: identyfikacji modułu, zgody na integrację i wolnego pinu I2C.

## 3. Karta „Dzień” w panelu — NIE ZROBIONE

Podgląd krzywej świateł na dziś (harmonogram + adaptacja + MIN LUX na osi czasu).
Dane: `/history.csv` (dziś, co 5 min) i harmonogram. Wymaga sprawdzenia, czy panel ma
dostęp do harmonogramu (EEPROM) przez Firebase lub LAN.

## 4. Lokalizacja z panelu (GOTOWE w 4.7.0) i rampy poranne (DO DECYZJI)

**Lokalizacja:** ustawiana w panelu LAN (karta „Lokalizacja – zachód słońca”), zapisywana
na ESP w NVS (namespace `astro_loc`, klucze `lat`/`lon`). Domyślnie 52,1345 / 20,1418.
- `GET /api/location` → `{"ok":true,"lat":..,"lon":..,"sunset":"HH:MM","timeSynced":bool}`
- `POST /api/location`, body `{"lat":52.1345,"lon":20.1418}` → `{"ok":true}`; zła wartość → 400
  (poprzednia zachowana), body > 127 B → 413.
- Po zapisie zachód przeliczany w następnym obiegu `loop()` (flaga `sunsetRecalcRequested`).
- Test hostowy: `check_location.sh` (endpointy i NVS wyciągnięte z Ryby, atrapy NVS/serwera).

**Rampy poranne — otwarte:** rampa poranna nadal startuje o stałej godzinie. Czy ma iść
według świtu/wschodu z przesunięciem (jak wieczorna od zachodu)? To zmienia start rampy
porannej, więc wymaga decyzji właściciela.
Fallback przy braku czasu (NTP/RTC) pozostaje jak dziś: 19:00 dla zachodu.

## 5. DoD (w domu, po flashu 4.7.0)

- Log `lvl=INFO tag=NTP msg="Zachod slonca lokalnie"` pokazuje godzinę zgodną z `astral`/timeanddate
  dla 52,1345 / 20,1418 (±2 min) w dniu testu.
- Zachowanie rampy wieczornej bez zmian poza przesunięciem zachodu o ≤4 min.
- Brak NTP → 19:00 jak w 4.6.0.
- Panel: zmiana lokalizacji zapisuje się i po restarcie ESP zachód liczony z nowych wartości.

## 6. Rollback

Zachód: przywrócić poprzednią `obliczZachodSlonca` (wzór uproszczony) albo wgrać 4.6.0.
Moduł `astro` pozostaje w repo jako nieużywany, jeśli rollback jest częściowy.
