# Etap 3 (dopięcie) — historia długa `/api/history/long` + wykres tygodniowy

**Wersja:** 4.5.0+build.267 · **Gap:** G8 (`docs/01`) · **Plan:** `docs/02`, Etap 3, pkt 4
**Status:** KOD + TESTY HOSTOWE GOTOWE. Test na płytce — sekcja 5 (w domu).

Spool V2 (`docs/03`) zostaje bez zmian. Ten dokument opisuje brakujący fragment
Etapu 3: długą historię do wykresów tygodniowych oraz zakładkę „Wykresy” w panelu.

---

## 1. Dlaczego osobny plik

`/history.csv` (24 h, co 5 min) ma czas `HH:MM` **bez daty**, okno 288 wierszy
i kompaktowanie przez `histStartLine`. Nie da się z niego zbudować tygodnia.
Dlatego dochodzi osobny plik z kubełkami 30 min i znacznikiem `unix`.
Stary `history.csv` i jego endpointy działają bez zmian.

## 2. Format (`/history_long.csv`)

Jeden wiersz na kubełek 30 min = średnia z próbek 5-minutowych wpadających w kubełek.

```
ts,t_woda,t_plyta1,t_plyta2,lux_pokoj,lux_woda,pwm0,pwm1,pwm2,pwm3,pwm4,moc_W,tryb,minLux,adapt,probki
1800000,24.0,25.1,25.3,100,200,20,20,20,20,20,40.0,AUTO,0,0,6
```

| Pole | Znaczenie |
|---|---|
| `ts` | początek kubełka, unix (wyrównany do 1800 s) |
| `t_*`, `lux_*`, `pwm*`, `moc_W` | średnie z próbek w kubełku (`pwm` zaokrąglone) |
| `tryb` | `AUTO`/`MAN` z **ostatniej** próbki kubełka |
| `minLux`, `adapt` | flagi z ostatniej próbki (`1`/`0`) |
| `probki` | ile próbek weszło do kubełka (zwykle 6) |

Pusta komórka = brak poprawnych odczytów w kubełku. Odczyty poza zakresem
(`-127` z DS18B20, NaN, ujemne lux) **nie** wchodzą do średniej, ale liczą się do `probki`.

## 3. Zachowanie

- Hak w `saveHistoryPoint()` (za guardem `historyClearPending`). Zapis tylko przy
  poprawnym czasie (`time() > 1700000000`, czyli po NTP). Bez czasu nic nie trafia do pliku.
- Rotacja: gdy bieżący plik przekroczy **40 KB** (~500 wierszy ≈ 10 dni), zmienia nazwę na
  `/history_long_old.csv` (poprzednie archiwum jest nadpisywane). Łącznie ≤ ~80 KB ≈ 3 tygodnie.
- Bufor kubełka jest w RAM. Restart w połowie kubełka gubi jego ≤30 min.
- `/api/history/clear` kasuje oba pliki long oraz bufor kubełka.

## 4. Endpoint i kotwice tras (ważne)

`GET /api/history/long` — `text/csv`, odpowiedź **chunked**: archiwum, potem bieżący
plik, nagłówek tylko raz. Cała historia nie trafia do RAM (≈80 KB). Jeden klient naraz;
drugi dostaje `503 {"error":"busy_or_no_fs"}`. Kursor wygasa po 10 s bez odczytu
(klient się rozłączył), więc kolejne żądanie nie utyka.

**Kolizja prefiksu naprawiona:** w ESPAsyncWebServer goły string w `on("...")` to
**prefiks**. `GET /api/history` przykrywał `/api/history/clear` i `/api/history/long`.
Teraz trzy trasy używają `AsyncURIMatcher::exact(...)` (ESPAsyncWebServer `^3.10.3`,
`src/ESPAsyncWebServer.h`). Ta sama pułapka jest w Centrali (FIX 2.6.0.4).
Test statyczny `check_integration_hl.sh` pilnuje, że żadna trasa `/api/history*`
nie wróci do wersji prefiksowej.

## 5. Panel — zakładka „Wykresy” (tylko LAN)

Karta „Historia tygodniowa (co 30 min)”:
- cztery wykresy liniowe (SVG): temperatura wody, lux nad wodą, moc LED, temperatura płyty 1;
  przełączniki widoczności; zakres dat, min/max/ostatnia wartość; przerwy w danych zostają jako przerwa;
- przycisk „⬇ CSV” (pobieranie wprost z ESP);
- zdalnie (Firebase) overlay „niedostępne przez Firebase”, bez wywołań do ESP.

Wykresy są rysowane z samych liczb (SVG `path`), więc tekst z CSV nie trafia do HTML.

## 6. Testy hostowe (wykonane)

| Test | Wynik |
|---|---|
| `test_histlong.cpp` (ASan + UBSan): kubełki, średnie, pomijanie błędnych odczytów, flagi, rotacja, strumień bez powtórnego nagłówka, kursor (7-bajtowe kawałki), timeout, clear, restart | 47 PASS, 0 FAIL |
| `check_integration_hl.sh`: endpoint + hak w `Ryby_LED_fi_S3.cpp` kompilują się z atrapami; trasy `exact` | OK (syntax-only) |
| jsdom panelu: parser CSV, render z przerwami, brak HTML z CSV, toggle, LAN/zdalnie | OK |
| `run_tests.sh` (wszystkie 6 kroków) | exit 0 |

**Nie testowano na ESP32** (brak toolchainu w sandboxie). Kompilacja PlatformIO — w domu.

## 7. DoD na płytce (w domu)

1. Build + flash 4.5.0 (USB). Po starcie i synchronizacji NTP poczekać ~1 h.
2. `GET http://<IP>:8080/api/history/long` → nagłówek + ≥2 wiersze, `ts` rosnące, `probki` ≈ 6.
3. Zakładka „Wykresy” w panelu (LAN): wykresy się rysują, zakres dat ma sens.
4. Zdalnie (LTE/Firebase): overlay „niedostępne przez Firebase”, w Konsoli przeglądarki brak żądań do IP ESP.
5. Dwa równoległe `GET /api/history/long` (np. dwie karty): jedna odpowiedź 200, druga 503 — bez zawieszenia ESP.
6. Heap: `fH` w logu przed i po odczycie ~80 KB (odpowiedź chunked nie może podbić pamięci).
7. `POST /api/history/clear` → `GET /api/history/long` zwraca pusty wynik (bez nagłówka).
8. Rotację można zweryfikować bez 10 dni: sprawdzić, że po przekroczeniu 40 KB pojawia się `history_long_old.csv` (np. po kilku dniach w `GET /api/fs-list`).

## 8. Rollback

4.4.0 ignoruje pliki `history_long*.csv` (nie są czytane). Wgranie 4.4.0 nie wymaga kasowania
LittleFS. Panel bez zakładki tygodniowej: przy starym ESP endpoint zwraca 404, karta pokazuje błąd.

## 9. Znane ograniczenia (świadome)

- **Okno utraty:** ≤30 min (bufor kubełka w RAM).
- **Brak NTP → brak zapisu.** Po restarcie bez sieci historia długa nie rośnie, aż wróci czas.
- **Jeden klient naraz** (`503` dla drugiego).
- **Retencja ~3 tygodnie** przy 40 KB na plik. Dłuższa wymaga podniesienia `histlong::MAX_BYTES`
  i sprawdzenia budżetu LittleFS (`partitions.csv`: ~9,7 MB).
- Odczyt archiwum i zapis bieżącego pliku mogą nakładać się w czasie (LittleFS, jeden zapis na 30 min) —
  na płytce obserwować w DoD pkt 2 i 5.
