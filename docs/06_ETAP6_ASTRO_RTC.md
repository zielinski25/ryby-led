# Etap 6 — astronomia, zegar odporny i karta „Dzień”

Plan: `docs/02_PLAN_UPGRADE_ryby_led.md`, ETAP 6 (P2). Ten dokument opisuje stan po 4.7.0+build.270.

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

## 1. Astronomia — moduł, zachód, świt i wschód (4.7.0)

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
- **Świt i wschód są podłączone do rampy porannej** przez tryb w panelu (sekcja 4).
  Zmierzch cywilny jest wyliczany, ale nic go nie używa.

## 2. Zegar odporny (RTC) — KOD GOTOWY (4.7.0+build.270), wybór modułu w panelu

Bez routera i po dłuższym restarcie czas jest nieznany, więc harmonogram nie ma podstawy.
Wybrano opcję **(a) RTC na I²C**, ale bez zewnętrznej biblioteki: sterownik własny
`firmware/src/rtc_ds13xx.h/.cpp` (namespace `rtc13`), bo RTClib nie jest w projekcie.

**Moduł:** właściciel ma moduł DS, ale nie wie, czy to DS1307, czy DS3231. Najpewniej
rozstrzyga nadruk na układzie. Oba obsługuje ten sam sterownik; wybór w panelu
(`chip` 0 / 1307 / 3231). Domyślnie **wyłączony**, więc brak modułu nie zmienia pracy sterownika.
Zakup nowego modułu nie jest zatwierdzony.

**Sterownik (`rtc13`):**
- rejestry 0x00–0x06 (sek, min, godz, dzień tygodnia, dzień, miesiąc, rok), BCD;
- CH (bit 7 sekundy) = 1 → zegar zatrzymany → odczyt odrzucony;
- godzina w trybie 12 h (bit 6) obsługiwana; rok 2000–2099;
- czas w module przechowywany jako UTC (epoka), przeliczenie na kalendarz bez strefy;
- DS3231: zapis czyści OSF (rejestr 0x0F, bit 7);
- dzień tygodnia zapisywany jako `wday+1` (1 = niedziela … 7 = sobota), odczyt go nie używa.

**Przepływ w firmware:**
- start: jeśli RTC włączony **i** czas systemowy < 1700000000 (brak NTP) → odczyt z RTC,
  `settimeofday`, przeliczenie zachodu (`przeliczZachodTeraz`) i świtu/wschodu. Nie wołamy
  `configTzTime` drugi raz; strefa z setup pozostaje.
  Odczyt odrzucany, gdy epoka < 1700000000 (np. nieskalibrowany DS1307 od 2000-01-01).
- po udanym NTP (blok `timeSynced = true` w `loop()`) oraz na żądanie z panelu
  (`POST /api/rtc` ustawia flagę, zapis robi `loop()`) → `writeUtc` czasu systemowego.
- I²C: `Wire` (SDA 21, SCL 20), ta sama magistrala co TSL2561. Odczyt i zapis tylko w `loop()`,
  handlery HTTP nie dotykają I²C.
- Panel: karta „Zegar RTC” (wybór modułu, stan, czas systemowy ustawiony, Zapisz/Odśwież).

**Testy hostowe:** `test_rtc.cpp` (59 asercji; fałszywa magistrala: BCD, CH, OSF, zakres roku,
NACK), `check_location.sh` (boot z RTC, odrzucenie 2000-01-01, CH=1, brak modułu, zapis
z NACK, round-trip RTC → restart → odczyt, endpointy `/api/rtc`).
**Nie testowane:** prawdziwa magistrala i moduł (test na płytce, sekcja 5).

## 3. Karta „Dzień” w panelu (4.7.0, GOTOWE w kodzie, test hostowy)

Strona **Wykresy**, karta „Dzień — krzywa świateł i harmonogram” (nad kartą LUX).
Oś doby 00:00–24:00:
- krzywa PWM średniego kanałów (żółta) z `/api/history`, tylko „dziś” (próbki po ostatnim
  przejściu przez północ),
- paski MIN LUX (góra) i adaptacji (dół) z kolumn 12 i 13 historii,
- tło: okno rampy wieczornej (od zachodu + `EVENING_ON_BEFORE_SUNSET_MIN` do `EVENING_OFF_START`),
- pionowe znaczniki: start poranka (WD/WE), przerwa południowa, zachód, koniec rampy, „teraz”
  (z `localTime` w `/api/status`),
- pasek pompki z `pumpSlots`.
Pod wykresem: tekst z godzinami (zachód, rampa, poranek, przerwa, liczba próbek).

Firmware nie wymagał zmian: `/api/status` już zwraca `schedule{}` (w tym `sunsetMin`,
`eveningBefore`, `eveningOff`) i `pumpSlots`. Test: `check_day_card.js` (26 asercji, Node),
wyciąga funkcje karty z `terminal_html.cpp` i uruchamia je na atrapie DOM.
Ograniczenie: poranek pokazany jest jako start harmonogramu, bez długości rampy porannej.
Dane historii obejmują 24 h; karta pokazuje tylko okres od ostatniej północy.

## 4. Lokalizacja i rampa poranna z panelu (GOTOWE w 4.7.0+build.270)

**Lokalizacja:** karta „Lokalizacja – zachód słońca” w panelu LAN, zapis na ESP w NVS
(namespace `astro_loc`, klucze `lat`, `lon`). Domyślnie 52,1345 / 20,1418.
- `GET /api/location` → `{"ok","lat","lon","sunset":"HH:MM","timeSynced","mrMode","mrOff","morningNow","dawn","sunrise"}`
- `POST /api/location`, body `{"lat":..,"lon":..,"mrMode":0|1|2,"mrOff":-180..180}`
  (`mrMode`/`mrOff` opcjonalne; brak = bez zmian). Zła wartość → 400 (poprzednia zachowana),
  body > 127 B → 413.
- Po zapisie zachód przeliczany w następnym obiegu `loop()` (flaga `sunsetRecalcRequested`),
  a świt/wschód od razu (`przeliczAstroDzis`).

**Rampa poranna — tryb do wyboru w panelu** (decyzja właściciela: „to powinno być do wyboru w aplikacji”):

| `mrMode` | Start porannej rampy |
|---|---|
| 0 (domyślnie) | stała godzina z harmonogramu (WD/WE), jak dotąd |
| 1 | świt cywilny (−6°) + przesunięcie |
| 2 | wschód słońca (−0,833°) + przesunięcie |

- Przesunięcie: −180…180 min (pole nieaktywne dla trybu 0).
- Tryb dotyczy dnia roboczego i weekendu.
- Źródło: `morningStartFor(weekend)` — jedno miejsce, z którego czytają wszystkie dotychczasowe
  odczyty `MORNING_ON_START_*` (sześć miejsc). Ramp Arbiter i MIN LUX nietknięte.
- Gdy świt/wschód nie istnieje (polarna noc) albo brak czasu, start wraca do stałej z harmonogramu.
- `/api/status` → `schedule.morningEff` (efektywny start); karta „Dzień” pokazuje tę wartość.
- Wariant domyślny: **0 (stała)**, czyli zachowanie sprzed Etapu 6. Zmiana na świt wymaga ręcznego
  wyboru w panelu.

**Wartość rampy wieczornej bez zmian:** nadal względna do zachodu (`EVENING_ON_BEFORE_SUNSET_MIN`).

**Fallback przy braku czasu (NTP i RTC):** zachód 19:00 jak dotąd; start poranka = stała z harmonogramu.
Nadal otwarte: czy fallback dla świtu/wschodu ma być inny niż stała godzina.

**Testy hostowe:** `check_location.sh` (92 asercje: NVS, endpointy, tryby 0/1/2, przesunięcie,
odrzucenia, `morningStartFor` zgodne z GET), `test_astro.cpp` (`morningStartMinutes`: zawijanie
doby, zaokrąglenie, obcięcie ±180, brak zdarzenia → fallback), `check_day_card.js` (`morningEff`).

## 5. DoD (w domu, po flashu 4.7.0+build.270)

Kompilacja PlatformIO i ten test wymagają płytki. W sandboxie nie wykonano ani kompilacji ESP32,
ani testu sprzętowego (brak dostępu do rejestru PlatformIO).

- Log `lvl=INFO tag=NTP msg="Zachod slonca lokalnie"` pokazuje godzinę zgodną z `astral`/timeanddate
  dla 52,1345 / 20,1418 (±2 min) w dniu testu.
- Zachowanie rampy wieczornej bez zmian poza przesunięciem zachodu o ≤4 min.
- Brak NTP i brak RTC → 19:00 jak w 4.6.0.
- Panel: zmiana lokalizacji zapisuje się i po restarcie ESP zachód liczony z nowych wartości.
- Panel → Poranek: tryb 1 (świt) z przesunięciem −30 → „Start poranka dziś” = świt − 30 min;
  karta „Dzień” pokazuje ten sam czas. Powrót do trybu 0 przywraca stałą z harmonogramu.
- RTC (jeśli moduł jest zamontowany, najpierw odczyt nadruku: DS1307 czy DS3231):
  1. panel → „Zegar RTC” → wybrać moduł → Zapisz; w ciągu kilku sekund „Stan” = „zapisano czas do RTC”;
  2. odłączyć NTP (router offline) i zrestartować ESP → w logu `czas z RTC (bez NTP)`,
     `GET /api/rtc` → `sysSynced: true`, zachód liczony;
  3. moduł bez baterii / wyjęty → „Stan” = „brak odczytu (modul/zegar)”, sterownik działa dalej;
  4. DS1307 po pierwszym uruchomieniu (2000-01-01) → „czas w RTC nieprawidlowy”, czas nie zmieniony.

## 6. Rollback

Zachód: przywrócić poprzednią `obliczZachodSlonca` (wzór uproszczony) albo wgrać 4.6.0.
Moduł `astro` pozostaje w repo jako nieużywany, jeśli rollback jest częściowy.
