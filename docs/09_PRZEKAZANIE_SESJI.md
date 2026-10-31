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
