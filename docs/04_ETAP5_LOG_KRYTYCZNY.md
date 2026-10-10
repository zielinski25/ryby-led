# Etap 5 — Log krytyczny + menu Telegram (4.4.0+build.266)

Plan: `docs/02_PLAN_UPGRADE_ryby_led.md`, sekcja ETAP 5. Wzór: `log_krytyczny.txt`
i menu bota z Centrali Pieca (v3.34.0 / v3.35.0).

## 1. Log krytyczny (`critlog`)

Moduł `firmware/src/critlog.h/.cpp` (namespace `critlog`). Osobny plik LittleFS,
którego nie rusza rotacja `log_a` / `log_b`:

| Plik | Rola |
|---|---|
| `/log_krytyczny.txt` | bieżący, dopisywany |
| `/log_krytyczny_old.txt` | archiwum po rotacji (nadpisywane) |

- Limit: 256 KB na plik. Po przekroczeniu bieżący plik zmienia nazwę na `_old` i startuje nowy.
  Łącznie na flash najwyżej 512 KB (SPIFFS ma ~9,7 MB).
- Linia: `YYYY-MM-DD HH:MM:SS lvl=CRIT tag=<TAG> msg="<opis>"`. Gdy NTP nie zsynchronizowany,
  zamiast daty jest `uptime=<s>`. Linie dłuższe niż 1000 B są ucinane.
- Zdarzenia zapisywane do pliku (tylko miejsca krytyczne, każde z progiem, żeby nie spamować):

| Tag | Kiedy |
|---|---|
| `BOOT` | start po `PANIC/CRASH` (ślad w coredumpie), `WDT_*`, `BROWNOUT` |
| `TSL` | czujniki światła niedostępne po 5 nieudanych próbach (I2C) |
| `DS-DIAG` | czujnik wody DS18B20 FAIL przy starcie |
| `OTA-GH` | nieudany start taska OTA, błąd HTTP OTA, `esp_ota_mark_app_valid_cancel_rollback` |

Zwykły log (`logPrintln`) dostaje te same linie, więc niczego nie gubi.

Nie ma alarmu przegrzania: Ryby nie ma progu temperatury wody, który by go wyzwalał.
Jeśli go dodamy, wpisujemy go do tabeli.

## 2. Endpointy HTTP (LAN, port 8080)

- `GET /api/log-critical-status` → `{"exists":true,"size":N,"max":262144,"old":false}`
- `GET /api/log-critical-download` → plik jako załącznik (`text/plain`)
- `GET /api/log-critical-download?old=1` → archiwum po rotacji

## 3. Telegram

- Komenda `/log_krytyczny` (alias `/krytyczny`): wysyła plik jako dokument (multipart,
  4 KB bloki, osobne połączenie TLS z `Connection: close`). Gdy plik pusty albo go nie ma,
  bot odpisuje komunikatem.
- **Menu pogrupowane (4.6.0+build.268).** Płaska siatka 2-kolumnowa, nagłówki sekcji to przyciski
  `noop` (klik nic nie robi). Każdy raport jest **1 klik**, bez podmenu (warunek DoD):

  | Sekcja | Przyciski (callback → komenda) |
  |---|---|
  | ⚙️ STEROWANIE | 💡 LED (`ledtog`), 🤖 Tryb (`trybtog`), 🔕 Powiad. (`notiftog`), 🔄 Restart ESP (`restart`) |
  | 📊 STATUS | 📊 Status (`status` → `/status`), ⚡ Energia (`energy`) |
  | 🌡️ CZUJNIKI | 🌡️ Temperatury (`temp` → `/temp`), 🔆 Lux & Światło (`light` → `/lux`), 🌊 Historia czujników (`sensorhist`) |
  | ⏰ HARMONOGRAM | ⏰ Harmonogram (`schedule` → `/harmonogram`), 🧠 Adaptacja (`adapt`) |
  | 🩺 DIAGNOSTYKA | 🚨 Log krytyczny (`critlog` → `/log_krytyczny`) |
  | 📋 LOGI | 📋 Pobierz logi (`logs`), 🗑️ Wyczyść logi (`clrlogs`) |
  | 🔁 OTA | 🔁 Aktualizacja OTA (`ota` → `/update`) |

  Sekcja STEROWANIE jest dodatkiem do planu (plan wymienia 6 sekcji raportów); przyciski sterujące
  nie są raportami, więc nie mieszają się z nimi. `/update` i `/ota` działają jak dotąd.
  Test formalny: `firmware/tests/host/check_tg_menu.py` (krok 7 `run_tests.sh`).

## 4. Panel WWW (`panel/akwarium-firebase-panel-v14.html`)

Zakładka **Logi** ma kartę „Log krytyczny”: rozmiar, „Pobierz”, „Archiwum”. Overlay
„niedostępne przez Firebase” znika tylko przy połączeniu LAN. Zdalnie nie ma wywołań
do ESP.

## 5. Testy

`bash firmware/tests/host/run_tests.sh`:

- `test_critlog.cpp`: 424 asercje, ASan + UBSan. Zapis, dopisywanie po restarcie,
  ucinanie długich linii, rotacja, nadpisanie archiwum, brak FS.
- `check_integration_e5.sh`: kompiluje `-fsyntax-only` blok `logCritical` +
  `sendTelegramCriticalDocument` + endpointy z `Ryby_LED_fi_S3.cpp` oraz helper
  `otaCritLine` z `ota_github.cpp` przeciwko atrapom `e5_mock.h` i prawdziwemu `critlog.cpp`.
  Sprawdza też, czy wywołania (`logCritical("TSL"`, `case TG_LOG_CRITICAL`, …) są
  wpięte w miejsca w kodzie.
- Panel: testy jsdom (karta logu w LAN i zdalnie).

**Nie testowano na ESP32.** Kompilacja pełnego firmware i test Telegrama wymagają płytki.

## 6. Test na płytce (DoD)

DoD z planu: z czatu da się pobrać log krytyczny i odpalić OTA; menu ma ≤1 klik do każdego raportu.

1. Build `esp32-s3-n16r8` bez błędów (PlatformIO na Windows).
2. Flash, potem w Telegramie: `/log_krytyczny` → dokument przychodzi (przy pustym logu:
   „brak zdarzeń krytycznych”).
3. Wymuszony błąd (np. odłącz jeden czujnik TSL) → po 5 próbach w pliku pojawia się linia
   `tag=TSL`. `GET /api/log-critical-status` pokazuje `exists:true`.
4. Restart z menu (lub przycisk RESET) → po starcie zwykły restart nie dopisuje wpisu `BOOT`
   (tylko PANIC/WDT/BROWNOUT).
5. Menu → „🔁 Aktualizacja OTA” → bot potwierdza start, a status idzie w `/api/ota-status`.
6. Panel w LAN: zakładka Logi → karta pokazuje rozmiar; „Pobierz” ściąga plik.

## 7. Rollback

Moduł `critlog` nie zmienia formatu innych plików. Cofnięcie do 4.3.0 usuwa menu
i endpointy, a pliki `log_krytyczny*.txt` zostają na LittleFS i nic im nie grozi.
