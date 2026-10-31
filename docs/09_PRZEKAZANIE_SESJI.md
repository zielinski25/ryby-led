# 09 — Przekazanie sesji (Ryby LED + aplikacja Android)

Ten plik jest dla kolejnej sesji AI (lub dla Ciebie). Zawiera wszystko, co trzeba wiedzieć,
żeby podjąć pracę bez pytania o kontekst. Stan na dzień **2026-10-31**.

Język rozmów z właścicielem: **polski, prosto, bez żargonu**. Właściciel woli **polecenia do terminala
(PowerShell 5)** i **nie używa `&&`** w PowerShell 5 (łańcuch poleceń rozdziela nowymi liniami).

---

## 1. Co to jest projekt

- **Ryby LED** — sterownik oświetlenia akwarium na płytce **ESP32-S3** (firmware w `firmware/src/Ryby_LED_fi_S3.cpp`).
- Cel długoterminowy: rozbudowa do poziomu programu **„Centrala Pieca”** (logi, zdalne sterowanie,
  wyjście poza LAN przez Firebase, odporność na zanik zasilania).
- Plan etapów: `docs/02_PLAN_UPGRADE_ryby_led.md` (tabela „STATUS REALIZACJI”).
- Obecna wersja firmware: **v4.7.2+build.272** (`RYBY_FW_VERSION`). Wersja aplikacji Android: **4.7.2** (versionCode 4272).

## 2. Repo, branch, PR — gdzie co jest

| Co | Wartość |
|---|---|
| Repo | `zielinski25/ryby-led` (GitHub) |
| Branch roboczy | **`arena/92c66e20-ryby-led`** — NIE zmieniać nazwy, NIE pushować na inne branche |
| Ostatni commit | `6f8730c` (na origin, lokalnie zsynchronizowany) |
| PR | **#1, DRAFT, OPEN**: „Ryby LED: etapy 2–6, panel 4.7.2 i aplikacja na telefon (draft)” — https://github.com/zielinski25/ryby-led/pull/1 |
| `origin/main` | **NIE modyfikować.** Stary panel v13 z sekretami w historii. |
| Tagi | `107`, `v4.1.0` (release OTA) |

### Ważne: stan lokalnego Gita (naprawiony 2026-10-31)
Lokalny `HEAD` był kiedyś na commicie bazowym `c19203a`, mimo że commity `e00cace`/`6f8730c` były na GitHubie.
Pliki robocze były identyczne z `origin/arena/92c66e20-ryby-led`. Naprawiono przez:
`git fetch origin arena/92c66e20-ryby-led` oraz `git reset --mixed origin/arena/92c66e20-ryby-led`
(zmienia tylko indeks i HEAD, nie pliki). **Pierwsze polecenie w nowej sesji:**
```
git fetch origin arena/92c66e20-ryby-led
git status -sb
```
Oczekiwane: `## arena/92c66e20-ryby-led...origin/arena/92c66e20-ryby-led` i zero zmienionych plików.

---

## 3. Stan etapów (skrót z `docs/02`)

| Etap | Status | Uwagi |
|---|---|---|
| 0. Repo + wersjonowanie | ✅ gotowy | 4.0.0 |
| 1. OTA przez GitHub Releases | ✅ kod + release v4.1.0 | czeka: flash USB w domu + test OTA |
| 2. Bezpieczeństwo Firebase | 🟡 w toku | sekrety w `src/secrets.h` (poza repo). **Rotacja Database Secret — status NIEZNANY** (patrz §4) |
| 3. Spool V2 + telemetria | 🟡 kod + 200 testów hostowych | DoD na płytce: zanik zasilania 5 min w trakcie wysyłki → REPLAY (`docs/03`) |
| 4. Panel: CORS, OTA, tryb zdalny | 🟡 kod gotowy | niesprawdzone z LTE (`docs/07`, krok 4). Panel **v14** zostaje (v15 zamknięte) |
| 5. Log krytyczny + Telegram | 🟡 kod gotowy | test na płytce wg `docs/04`, sekcja 6 |
| 6. Astronomia + RTC | 🟡 kod gotowy | do zrobienia w domu: identyfikacja modułu RTC (nadruk), test wg `docs/06`, sekcja 5 |
| 7. Panel LVGL | ⬜ opcjonalny | poza zakresem |

Kryteria akceptacji etapu 3 (dla testu sprzętowego): odłączenie zasilania na 5 min w trakcie wysyłki →
REPLAY po powrocie; CRC odrzuca uszkodzone rekordy; PSRAM przed i po; **nie ruszać Ramp Arbiter ani MIN LUX**.

---

## 4. PILNE — sekrety

W historii publicznego zipa były na stałe wpisane: `FIREBASE_SECRET` (Database Secret, pełny zapis do RTDB,
nie wygasa), `CMD_TOKEN` i hasło OTA. **Nie wolno wpisywać żadnych wartości do repo, czatu ani dokumentów.**

- Do zrobienia przez właściciela: **rotacja Database Secret w Firebase Console**, wpisanie nowego do
  `secrets.h` (lokalnie, poza repo), zmiana `CMD_TOKEN` i hasła OTA.
- Brak potwierdzenia, że rotacja została wykonana. **Zapytaj właściciela na początku sesji.**
- Skaner sekretów `firmware/tests/host/check_secrets.py` (krok 0 testów) pilnuje, żeby znane wartości nie wróciły.

---

## 5. Aplikacja Android (główny wątek ostatnich prac)

Właściciel wybrał **natywną aplikację Android (Kotlin + Jetpack Compose)** zamiast pliku HTML, bo ma wyglądać
profesjonalnie i spójnie z aplikacją Pieca (`Sterownik-Pieca-C.O.`, gałąź `arena/539ab1b6-sterownik-pieca-c-o`).
Telefon: **realme RMX3085**. Budowa: Windows + Android Studio lub skrypt PowerShell.

### 5.1 Struktura
- `android/` — projekt Gradle (Gradle 8.9, AGP, minSdk 26, targetSdk 35, package `com.rybyled.panel`).
- `android/app/src/main/java/com/rybyled/panel/`
  - `MainActivity.kt` — start aplikacji.
  - `core/Logic.kt` — komendy (`pwm`, `autosave`, `power_on/off`, `pump_on/off`, `mode_auto/manual`, `led_100`, `led_off`, `restart`), formatowanie wartości, stan połączenia (`NONE/ONLINE/STALE`, STALE po 150 s).
  - `core/RybyStatus.kt` — parsowanie JSON statusu z RTDB.
  - `core/Rtdb.kt` — REST do Firebase RTDB (HTTPS).
  - `core/AppPrefs.kt` — sekrety i adres bazy w SharedPreferences telefonu.
  - `ui/Theme.kt` — **paleta `Pal`, wymiary `Dimens`, style tekstu `Txt`**, skopiowane z Pieca (jedna zmiana: `TextDim2 = #7D98AD`, bo Piec ma `#5A7A99` = 3,97:1, poniżej WCAG AA).
  - `ui/App.kt` — ekrany: Główna, Światło, Pompa, Energia, Ustawienia; `GlowCard`, `Tile`, `Panel`, `SectionTitle`, przyciski, dialogi.
  - `ui/AppViewModel.kt` — stan, odświeżanie co 5 s, wysyłka komend.
- `android/build_apk.ps1` — skrypt PowerShell 5: sprawdza Javę, pobiera Android SDK, buduje APK, kopiuje na pulpit.
- `android/README.md` — instrukcja budowy (PowerShell i Android Studio).
- `docs/08_DESIGN_APLIKACJI.md` — research wyglądu: analiza Pieca, zasady ciemnego motywu, WCAG, tabela kontrastu, decyzje, otwarte pytania.

### 5.2 Commity aplikacji Android (kolejność)
`4aacac2` (port) → `024f581` (skrypt PS) → `b7f65ec` (skaner sekretów) → `e3eea86` (import `rememberSaveable`) →
`55ccfbf` (ciemny motyw, ikona adaptacyjna) → `e00cace` (wygląd jak Piec, `Theme.kt`, docs/08, test kontrastu,
zależność ikon) → **`6f8730c`** (dopracowany wygląd: gradienty, poblaski, ikony w kaflach, pasek mocy, nagłówki sekcji).

### 5.3 Ważne decyzje techniczne
- **`material-icons-extended`** jest w `app/build.gradle.kts` (wersja z BOM). Bez niej `Lightbulb`, `WaterDrop`, `Bolt`, `Thermostat`, `WbSunny`, `DateRange` nie kompilują się. Test sprawdza tę zależność.
- Nazwy używane przez test: `TAB_LABELS` (5 elementów), `sendPlain("…")`, `sendPwm`, `sendAutosave`. **Nie zmieniać bez aktualizacji testu.**
- Kontrast: test `check_android_app.js` sekcja 6c parsuje `object Pal` z `Theme.kt` i sprawdza 16 par tekst/tło ≥ 4,5:1. Dodając kolor tekstu, dopisz go do `TEXT_PAIRS`.
- Edge-to-edge: `statusBarsPadding()` w pasku górnym (Android 15).
- Hasła i sekrety są w SharedPreferences telefonu, nie w kodzie.

### 5.4 Co użytkownik widział
- Pierwsza wersja (przed `e00cace`): uznana za „beznadziejną grafikę”. Użytkownik wybrał kierunek **„jak Piec, ale dopracowany”**.
- Po `6f8730c` użytkownik **jeszcze nie potwierdził** uruchomienia najnowszej wersji ani nie przysłał zrzutu ekranu.

### 5.5 Co zostało do ustalenia z właścicielem (nieodpowiedziane)
1. Czy cyjanowy akcent z Pieca jest OK (obecnie tak).
2. Czy większe teksty niż w Piecu są akceptowalne (obecnie tak: etykiety 12–13 sp, treść 15 sp).
3. Czy ikony Material wystarczą, czy ręcznie rysowane ikony jak w Piecu (osobny projekt).
4. Czy na ekranie Przegląd mają być temperatury wody/powietrza z innych czujników (zależy od firmware).
5. Ikona aplikacji: ikona adaptacyjna ryby z `55ccfbf` — czy zaakceptowana (nie była oceniana).

### 5.6 Jak właściciel pobiera i uruchamia (PowerShell 5, sprawdzone)
```
Remove-Item -Recurse -Force "$env:USERPROFILE\Desktop\ryby-nowy"
Invoke-WebRequest -Uri "https://github.com/zielinski25/ryby-led/archive/refs/heads/arena/92c66e20-ryby-led.zip" -OutFile "$env:USERPROFILE\Desktop\ryby-nowy.zip"
Expand-Archive -Path "$env:USERPROFILE\Desktop\ryby-nowy.zip" -DestinationPath "$env:USERPROFILE\Desktop\ryby-nowy" -Force
```
Projekt w Android Studio: `C:\Users\bakoma\Desktop\ryby-nowy\ryby-led-arena-92c66e20-ryby-led\android`
→ **Sync Now** → **Run** (telefon podłączony USB, debugowanie USB włączone).
Alternatywa: `powershell -ExecutionPolicy Bypass -File .\android\build_apk.ps1` (APK na pulpicie).

**Uwaga:** ścieżka `Expand-Archive` z `codeload.github.com` też działała, ale ZIP z `github.com/.../archive/refs/heads/...` jest zalecany.
Przy pierwszym rozpakowaniu na pulpit mogła nadpisać się stara kopia folderu `ryby-led-arena-92c66e20-ryby-led` (ta sama treść brancha, bez szkody).

---

## 6. Testy i weryfikacja

Uruchamiaj z katalogu repo (Linux/sandbox):
```
bash firmware/tests/host/run_tests.sh      # oczekiwany EXIT 0
node firmware/tests/host/check_android_app.js   # 140 PASS, 0 FAIL
node firmware/tests/host/check_mobile_app.js    # 116 PASS (logika ryby-mobile.html)
```
Ostatni pełny wynik: `WYNIK: 200 PASS, 0 FAIL`; `secrets: 89 plików, 0 trafień`; `critlog 424`, `histlong 47`,
`astro 197`, `rtc 59`, `location 92`, `day card 26`, `panel cmd 47`, `mobile app 116`, `android app 140`.

**Ograniczenia sandboxa:** nie ma Android SDK ani dostępu do Maven/Gradle (dozwolone hosty: github.com,
codeload.github.com, api.github.com, registry.npmjs.org, pypi.org, files.pythonhosted.org). **Kotlin nie jest
kompilowany w sandboxie** — błędy kompilacji wykrywa dopiero Android Studio u właściciela. Testy Androida to
testy kontraktu (tekst kodu), nie kompilacja.

---

## 7. Zasady pracy (obowiązkowe)

- Pracować **tylko** na `arena/92c66e20-ryby-led`. Push: `git push origin arena/92c66e20-ryby-led`.
- Nie zmieniać `origin/main`. Nie usuwać repo ani `.git`.
- Każda zmiana firmware: wpis w `CHANGELOG.md` (Co / Dlaczego / czego NIE ruszono), semver, tag `[TAG]` w fixie.
- Nie ruszać logiki świecenia bez potrzeby (Ramp Arbiter, MIN LUX).
- Sekrety nigdy w repo, czacie ani w dokumentach. Testy skanują znane wartości.
- Przed merge PR nie wykonywać `--auto`. Merge/close tylko z jawnym numerem PR (`gh pr merge <nr> --merge`).
- Zawsze sprawdzić `git status` przed commitem; commitować tylko pliki zmienione w tej pracy.

---

## 8. Pliki poza repo (utracone między sesjami)

Te pliki były w `/tmp` i **nie są w repo**. Nowa sesja może je odtworzyć z opisu:
- `/tmp/piec_ref` — klon `Sterownik-Pieca-C.O.` (do odczytu). Źródła wyglądu: `ui/components/Hmi.kt`, `ui/components/Chrome.kt`, `ui/theme/Color.kt`, `widget_preview_full.png`.
- `/tmp/chk` — sandbox E2E (`e2e.mjs`, `mock_rtdb.mjs`, puppeteer-core@23).
- `/tmp/contrast.py` — obliczenia kontrastu (wzór WCAG; wyniki są w `docs/08`).
- `/tmp/pr_body.md` — treść opisu draft PR #1.
- `/tmp/ryby-backup` — kopia zapasowa z 2026-10-31 przed naprawą lokalnego Gita (można usunąć).

---

## 9. Otwarte punkty (priorytet)

1. **Rotacja sekretów Firebase** (§4) — PILNE, status nieznany.
2. **Pierwsze uruchomienie najnowszej aplikacji Android** przez właściciela + zrzut ekranu Główna.
3. Odpowiedzi na pytania z §5.5.
4. Test OTA i flash w domu (etap 1), test zaniku zasilania (etap 3), test LTE (etap 4), log krytyczny na płytce (etap 5), RTC (etap 6).
5. PR #1 nadal **draft** — decyzja właściciela, kiedy oznaczyć jako gotowy.
6. `docs/02` nie ma jeszcze wiersza o aplikacji Android w tabeli etapów — do uzupełnienia.

---

## 10. Pierwsze kroki w nowej sesji

1. `git fetch origin arena/92c66e20-ryby-led` → `git status -sb` (ma być czysto, zsynchronizowane).
2. Przeczytać ten plik, `docs/02` (status), `docs/08` (wygląd Androida).
3. Zapytać właściciela: rotacja sekretów zrobiona? czy uruchomił najnowszą wersję i jak wygląda?
4. Uruchomić `bash firmware/tests/host/run_tests.sh` — oczekiwany EXIT 0.
5. Dopiero potem zmiany. Każdy commit krótko opisać po polsku.

---

## 11. Indeks wszystkich prac (commity na branchu)

Pełna lista: `git log --oneline` na `arena/92c66e20-ryby-led`. Poniżej najważniejsze commity pogrupowane po obszarach.

### 11.1 Badania i plan
- `36e1dd1` — research porównawczy Ryby vs Centrala Pieca + plan upgrade (etapy 0–7) → `docs/01`, `docs/02`.
- `3876736` — `RELEASE.md` (proces wydawania) + status etapów 0–1.

### 11.2 Etap 0 — porządek w repo i wersjonowanie
- `8e4776e` — struktura `firmware/`, `docs/`, `CHANGELOG.md` (semver), `build_gate.ps1`, usunięcie starego zipa.

### 11.3 Etap 1 — OTA przez GitHub Releases
- `cb16f00` — OTA przez GitHub Releases (port z Centrali Pieca), release **v4.1.0** (tag `v4.1.0`).
- `037cd7a` — poprawki kompilacji (IDF 5.5, deklaracje w przodzie).
- Czeka: flash USB w domu i test OTA.

### 11.4 Etap 2 — bezpieczeństwo Firebase
- `3914604` — sekrety przeniesione do `src/secrets.h` (poza repo), szablon `secrets.example.h`.
- `6ef7dcd` — OTA i token Telegrama z `secrets.h`, `RYBY_OTA_PASSWORD` w środowisku, skaner sekretów w testach.
- `3a731a9` — skrypt `firmware/scripts/tls_dump_chain.ps1` (odczyt łańcucha TLS, poza repo).
- `b7f65ec` — naprawa skanera sekretów w teście Androida (usunięte znane wartości).
- Otwarte: **rotacja Database Secret** w Firebase, zmiana `CMD_TOKEN` i hasła OTA (§4).

### 11.5 Etap 3 — trwała telemetria
- `c3fd1c9` — TelemetryRing w PSRAM, Spool V2 (CRC), `sessionNonce`, replay FIFO, `/api/telemetry/status` (wersja 4.2.0).
- `d5303b9` — historia długa `/api/history/long` (kubełki 30 min) i wykres tygodniowy w panelu (4.5.0).
- Otwarte: test na płytce (zanik zasilania 5 min w trakcie wysyłki) wg `docs/03`.

### 11.6 Etap 4 — panel WWW (CORS, OTA, tryb zdalny)
- `4484c6c` — `akwarium-firebase-panel-v13.html` bez sekretów (sekrety z localStorage).
- `199a803` — CORS-PNA + OPTIONS, panel **v14** (tryb LAN/zdalnie, karta OTA, skan Wi-Fi) (4.3.0).
- `b9b1ae3` — komendy zdalne panelu do kolejki `/aquarium/commands` (4.7.1).
- `5a4e5fa` — zapis AUTO zdalnie jako komenda `autosave` (4.7.2).
- `8bf5596` — zakres `minLuxTarget` 500–8000 zgodny z firmware.
- `cc851d1` — parser `configType: luxSim` w firmware (symulacja LUX z panelu).
- `8d3b8bf` — checklista wdrożenia `docs/07`; panel v15 zamknięty, zostaje v14.
- Otwarte: test z telefonu na LTE (`docs/07`, krok 4).

### 11.7 Etap 5 — log krytyczny i Telegram
- `9fe22e7` — `log_krytyczny.txt` na LittleFS, `/api/log-critical-download`, komenda `/log_krytyczny`, automatyczny coredump (4.4.0).
- `bbbb14a` — pogrupowane menu Telegrama (sekcje + 1 klik do raportu) (4.6.0).
- Otwarte: test na płytce wg `docs/04`, sekcja 6.

### 11.8 Etap 6 — astronomia i RTC
- `506da8a`, `2f43d79` — moduł `astro` (NOAA, świt/wschód/zachód/zmierzch), 197 asercji, `docs/06`.
- `9665611` — lokalizacja z panelu (NVS, `GET/POST /api/location`).
- `4ad2c68` — karta „Dzień” w panelu (krzywa doby, harmonogram).
- `3fbab63` — poranna rampa (stała / świt / wschód + przesunięcie), zegar RTC DS1307/DS3231 (`rtc13`, 59 asercji), endpointy `/api/rtc` (4.7.0+build.270).
- Otwarte: identyfikacja modułu RTC (nadruk na płytce), test na płytce wg `docs/06`, sekcja 5.

### 11.9 Aplikacje na telefon
- `466e0f5` — pierwsza wersja jako PWA z katalogiem `panel/mobile` (test 122 PASS). **Usunięta** w `cf6fa06`.
- `cf6fa06` — jednoplikowa aplikacja `panel/ryby-mobile.html` (logika między `LOGIC-BEGIN` a `LOGIC-END`, test 116 PASS, E2E 36 PASS).
- Otwarte dla wersji HTML: adres HTTPS (GitHub Pages) i test na telefonie. **Wybrano wersję Android** (poniżej).

### 11.10 Aplikacja Android (natywna, Kotlin + Compose)
- `4aacac2` — port `ryby-mobile.html` do Kotlin + Compose (`android/`), test kontraktu `check_android_app.js`.
- `024f581` — `android/build_apk.ps1` (PowerShell 5, buduje APK bez Android Studio) + README.
- `e3eea86` — naprawa brakującego importu `rememberSaveable` (błąd z Android Studio).
- `55ccfbf` — ciemny motyw, nagłówek ze statusem, kafelki, ikona adaptacyjna ryby.
- `e00cace` — wygląd jak Piec (`ui/Theme.kt`: `Pal`, `Dimens`, `Txt`), test kontrastu WCAG AA (16 par), zależność `material-icons-extended`, `docs/08`.
- `6f8730c` — dopracowany wygląd: gradienty, poblaski, ikony w kaflach, pasek mocy, nagłówki sekcji.
- Status: kod gotowy, **kompilacja niesprawdzona** (brak SDK w sandboxie), właściciel jeszcze nie potwierdził wyglądu na telefonie.

### 11.11 Dokumentacja
- `d166ac0` — `docs/09_PRZEKAZANIE_SESJI.md` (ten plik).
- `docs/08_DESIGN_APLIKACJI.md` — research wyglądu (Piec, zasady ciemnego motywu, WCAG, decyzje).
- `docs/02_PLAN_UPGRADE_ryby_led.md` — tabela statusów etapów (do uzupełnienia o aplikację Android).

### 11.12 Gdzie są pliki poza repo
- Paczka z całym repo i historią: `ryby-led-pelne-2026-10-31.zip` na Dysku Google (ID `10i3hLCw9nQprBhWHV3QOfIo-AxTD9UD2`) i w folderze roboczym sesji. Paczka pochodzi z 2026-10-31, przed tym indeksem.
