# Etap 6 — astronomia, zegar odporny i karta „Dzień”

Plan: `docs/02_PLAN_UPGRADE_ryby_led.md`, ETAP 6 (P2). Ten dokument opisuje stan po 4.7.0.

## 1. Astronomia — moduł (4.7.0, GOTOWE i przetestowane hostowo)

- `firmware/src/astro.h/.cpp`: `astro::compute(y, m, d, lat, lon, tzMin, Times&)`.
  Zwraca świt cywilny (−6°), wschód i zachód (−0,833°), zmierzch cywilny, w minutach
  od północy czasu lokalnego. Flagi `has*` mówią, czy zdarzenie istnieje (polarny dzień/noc).
- Algorytm: NOAA (deklinacja, równanie czasu, 3 iteracje czasu zdarzenia).
- Test: `test_astro.cpp`, 18 par data×miejsce wobec `astral` 3.2 (`astro_ref_gen.py`),
  tolerancja 2 min. Wynik: 141 PASS, 0 FAIL (ASan + UBSan).
- **Nie podłączony.** Moduł nie zmienia zachowania firmware.

## 2. Zegar odporny (RTC) — DECYZJA WŁAŚCICIELA

Dziś harmonogram zależy wyłącznie od NTP. Bez routera i po dłuższym restarcie czas jest
nieznany. Opcje z planu:
- **(a) RTC DS3231 na I2C** (~10 zł, jak w Centrali, RTClib). Rekomendowane.
- (b) „ostatni znany czas + drift” z logowaniem niepewności — bez dokupowania.

Nie zaimplementowano. Wymaga zgody na zakup (pytanie 3 w `docs/02`) i wolnego pinu I2C.

## 3. Karta „Dzień” w panelu — NIE ZROBIONE

Podgląd krzywej świateł na dziś (harmonogram + adaptacja + MIN LUX na osi czasu).
Dane: `/history.csv` (dziś, co 5 min) i harmonogram. Wymaga sprawdzenia, czy panel ma
dostęp do harmonogramu (EEPROM) przez Firebase lub LAN.

## 4. Podłączenie astronomii do harmonogramu — DO DECYZJI

Rampy start/koniec podążałyby za wschodem/zachodem. To zmienia czasy rampy, więc dotyka
harmonogramu. Ramp Arbiter (v252) i MIN LUX pozostają nietknięte tylko, jeśli zmiana
ogranicza się do przesunięcia punktów startu/końca. Do ustalenia:
1. Czy rampa ma iść według wschodu/zachodu w całości, czy tylko jako przesunięcie
   względem ustawionej godziny (np. ±N min)?
2. Skąd szerokość/długość: stała w kodzie (Kraków 50,0647 / 19,9450), konfiguracja w panelu
   (zapis do NVS/EEPROM) czy z Firebase?
3. Fallback: przy braku czasu (NTP/RTC) wracać do sztywnego harmonogramu (jak dziś).

## 5. DoD (po podłączeniu, w domu)

- Wartości `astro` na płytce zgodne z `astral`/timeanddate dla Krakowa (±2 min).
- Zmiana szerokości nie wymaga przeflashowania (jeśli wybrano konfigurację w panelu).
- Brak czasu → zachowanie identyczne jak w 4.6.0.

## 6. Rollback

Moduł `astro` nie jest używany, więc rollback to usunięcie `astro.cpp` (lub wgranie 4.6.0).
