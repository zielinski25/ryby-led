# RYBY-LED vs CENTRALA PIECA — głęboki research porównawczy

Data: 2026-10-09
Zakres: pełna analiza porównawcza programu sterownika oświetlenia akwarium
(**Ryby LED**) względem programu sterownika pieca C.O. (**Centrala Pieca**)
na podstawie kompletnych kodów źródłowych obu projektów.

Źródła analizy:

| Projekt | Źródło | Stan |
|---|---|---|
| Ryby LED | `ryby.zip` dodany do tego repo (2026-10-09): `Ryby_LED_fi_S3.cpp` (22 812 linii), `terminal_html.cpp` (3 385 linii), `NetDiag.h`, `platformio.ini`, `partitions.csv` | firmware **v261** (2026-09-27), wersja publiczna w release: **107** (nieaktualna) |
| Centrala Pieca | repo `zielinski25/Sterownik-Pieca-C.O.` (zip „Cenatrala Pieca.zip”): `main_centrala.cpp` (31 182 linie), `dashboard_lvgl_poc.cpp` (7 273), `main.cpp` panel LCD (3 666), `Piec.html` (9 168 linii) | firmware **v3.35.1** (2026-10-05), semver |

Uwaga: kod Centrali zawiera **199 odwołań do projektu Ryby** — wiele mechanizmów
Centrali zostało przeniesionych 1:1 z Ryb (logowanie LittleFS, write-guard TCP,
keepalive, DFS-lock, FS-GUARD). Ryby są więc w wielu obszarach *źródłem* wzorców,
nie tylko uczniem. Celem tego dokumentu jest wyłapanie obszarów, w których to
Centrala odjechała do przodu (głównie: wydania/OTA, bezpieczeństwo Firebase,
trwała telemetria, proces wytwórczy).

---

## 1. RYBY LED — stan obecny (inwentaryzacja)

### 1.1 Platforma sprzętowa i build

- **ESP32-S3 N16R8** (16 MB flash, 8 MB OPI PSRAM) — ta sama płytka co Centrala
  (wspólny plik `boards/esp32-s3-n16r8.json`).
- PlatformIO z platformą **pioarduino 55.03.38-1**, framework Arduino,
  `lib_ldf_mode = chain`, własny `custom_sdkconfig`:
  - `CONFIG_LWIP_DHCP_GET_NTP_SRV=n` — fix crasha lwIP SNTP/DNS (v239, udokumentowany
    w `PLAN_WDROZENIOWY_lwip_sntp_crash.md`),
  - `CONFIG_SPIRAM_FETCH_INSTRUCTIONS=y` + `CONFIG_SPIRAM_RODATA=y` — fix WDT_TASK
    przy blokadzie flash-cache przez zapisy LittleFS (kod wykonywany z PSRAM).
- Partycje: dual-OTA (`app0`/`app1` po 3 MB), **coredump 64 KB** (od v113),
  LittleFS/SPIFFS ~9,7 MB.
- Dwa env-y: USB (`esp32-s3-n16r8`) i **OTA espota** (`esp32-s3-n16r8-ota`,
  hasło `AkwPanel2026!`, host `ryby-led-s3.local` / IP 192.168.100.108).

### 1.2 Funkcje domenowe (akwarium)

- **5 kanałów LED PWM** 10-bit / 25 kHz (GPIO 16/19/4/10/13): białe, FS,
  FS-białe, niebieskie, czerwone.
- **Power-balancer** z limitem zasilacza 90 W (`LED_MAX_POWER_W`) — przycinanie
  celów mocy (`capTargetsToPowerLimit`).
- **Czujniki**: 2× TSL2561 (oświetlenie pokojowe + nad wodą), DS18B20
  (temperatura wody) z pełną diagnostyką błędów (throttle, DS-STALL, walidacja).
- **Tryby**: AUTO (harmonogram + adaptacja) i MANUAL; suwak master + 5 suwaków
  kanałów; soft-start; **Ramp Arbiter** (v252) — jeden właściciel PWM,
  wykrywanie kolizji soft-start/adaptacja/harmonogram, logi BLOCK/PREEMPT.
- **Regulacja adaptacyjna** (EMA, czułość, czas rampy) + tryb **MIN LUX**
  (przerwa południowa utrzymywana z czujnika) z ręczną blokadą override
  do końca przerwy (v237/v238).
- **Harmonogram** w EEPROM (sekcja `EEPROM - HARMONOGRAM`).
- Szybkie akcje: `/api/quick/pump_on|off`, `camera_on|off`, `led_test`,
  `led_off`, `restart` (pompka, kamera, test LED).

### 1.3 Architektura oprogramowania

- **Podział rdzeni**: cała sieć (Firebase, Telegram, NetDiag) wyłącznie na
  Core 0 (`tgTaskFn`, stos w PSRAM przez `xTaskCreateStaticPinnedToCore`),
  logika/UI na Core 1; reguła „zero alokacji sieciowych na Core 1” (od v42).
- **Kolejki międzyrdzeniowe z backpressure**: config refresh → immutable
  snapshot → queue → PERSIST_ACK → dopiero wtedy `cfgTs` i DELETE
  (v257/v258/v261 — dwufazowe, trwałe potwierdzanie konfiguracji).
- **Firebase RTDB** (`FirebaseClient 2.2.9`): ścieżki `/aquarium/status`,
  `/aquarium/commands` (kolejka first-key, tryb Turbo 1 s/10 s po walidacji
  komendy), `/aquarium/config` (`config_refresh`).
  Autoryzacja: **LegacyToken (Database Secret)** + `fbSSLClient.setInsecure()`.
- **Telegram bot**: `/start /help /sendlogs /logi /status /temp /energia /lux
  /swiatlo /adapt /historia /czujniki /harmonogram /schedule /notif
  /powiadomienia /testdaily`.
- **Panel WWW** (PROGMEM, `terminal_html.cpp`): zakładki **Dashboard /
  Ustawienia / Terminal (WebSocket `/ws` + konsola komend) / Wykresy
  (temperatura, lux, adaptacja) / Logi**; motyw ciemny, fonty Outfit/Space Mono,
  pełny responsive (7 reguł @media — wzorzec później skopiowany do Centrali).

### 1.4 Niezawodność i diagnostyka (mocna strona projektu)

- **Logi**: bufor kołowy w PSRAM + flush do LittleFS (`log_a.txt`/`log_b.txt`),
  rotacja przy 79 %, FS-GUARD (97 %), circular trim do 256 KB, format
  `lvl= tag= klucz=wartosc`, filtry kategorii w panelu, DIAG-FLASHSTALL
  (pomiar czasu zapisów flash).
- **Coredump**: partycja + **CRASH-ARCHIVE (v240)** — archiwizacja raw obrazu
  do `/coredumps`, trwały seq w NVS, retencja 20 plików, **automatyczna wysyłka
  do Telegrama** po starcie z retry.
- **WDT**: `esp_task_wdt_init` z obsługą `ESP_ERR_INVALID_STATE` →
  `esp_task_wdt_reconfigure`, karmienie w pętlach blokujących.
- **NTP/SNTP**: `configTzTime`, udokumentowany i załatany wyścig SNTP↔DNS (v239).
- **NetDiag.h**: 48-godzinny test routera/łącza do osobnego pliku logu,
  testy wyłącznie po IP (bez DNS), zagatowany budżetem iteracji.
- **WiFi multi-network**: NVS lista sieci, scan/add/delete, `wifiPickBestNetwork`.
- API HTTP: ~30 endpointów (status, set-pwm, quick/*, wifi/*, telegram/*,
  fs-* (przeglądarka plików LittleFS), log-*, history, adaptive/save,
  minlux/save, save-auto, symulacja `/api/sim`).

### 1.5 Wersjonowanie i repozytorium (słaba strona projektu)

- Wersje wewnętrzne `vNNN` (obecnie **v261**), string `FW_VERSION` w kodzie;
  changelog tylko w nagłówku pliku `.cpp`.
- **`version.txt` = 107 i release GitHub `107` (2026-06-01) są mocno nieaktualne**
  względem v261 — od czerwca brak nowego release mimo ~150 wersji wewnętrznych.
- Repozytorium = dopiero co dodany `ryby.zip`; w środku także martwy balast
  `Ryby_LED_fi_S3.cpp.pre-v253` (1,2 MB kopii zapasowej).
- Brak: semver, CHANGELOG.md, struktury katalogów w repo, skryptu build-gate,
  release'ów z artefaktami (bin **+ elf** do addr2line).

---

## 2. CENTRALA PIECA — stan obecny (punkt odniesienia, v3.35.1)

### 2.1 To, co Centrala ma ponad / lepiej niż Ryby

1. **OTA przez GitHub Releases** — sprawdzenie najnowszego tagu przez
   `api.github.com`, pobranie `firmware.bin` (obsługa 302), `Update.*`,
   weryfikacja i `esp_ota_mark_app_valid_cancel_rollback()`. Wyzwalane:
   kartą panelu „Aktualizacja firmware (OTA & GitHub)”, komendami Telegram
   `/update` `/update_panel` oraz komendami Firebase `update`/`update_centrala`
   (v3.32.0). **Ryby mają tylko ArduinoOTA z hasłem.**
2. **Bezpieczeństwo Firebase — COMMITY H1/H2**: weryfikacja TLS
   (koniec `setInsecure()`), secret baseline, migracja na **UserAuth**
   z izolowanymi regułami per urządzenie. **Ryby: LegacyToken + setInsecure().**
3. **Trwała telemetria (COMMIT-B/G, seria R245–R470)**: `TelemetryRing` (SPSC,
   PSRAM) + **Persistent Spool V2** — rekordy z CRC, batche, ACK/REPLAY/DELETE,
   recovery prefiksu po zaniku zasilania, testy macierzowe fault/recovery
   (setki testów „R-LOCAL”). **Ryby nie mają odpowiednika trwałej telemetrii.**
4. **`sessionNonce`** (COMMIT-B) — efemeryczny identyfikator sesji telemetrii
   (`esp_random` + zegar monotoniczny).
5. **`log_krytyczny.txt`** — osobny chroniony log zdarzeń alarmowych
   + komenda Telegram `/log_krytyczny` + pobieranie z panelu (v3.35.0).
6. **Nowoczesny bot Telegram** (v3.34.0) — płaskie menu 2-kolumnowe, pogrupowane
   raporty (status/temperatury/serwa/diagnostyka) z emotikonami; komendy
   akcji, alarmów, logów, diagnostyki, trybu serw i OTA bezpośrednio z czatu.
7. **Dyscyplina CORS** (v3.31.19–v3.32.2) — `Access-Control-Allow-Origin: *`
   + `Allow-Private-Network` + handlery OPTIONS (preflight) na **wszystkich**
   endpointach API, tak by panel otwarty z innego originu/LTE działał bez błędów.
8. **Zdalny panel poza LAN** (v3.33.0) — panel WWW działa przez przekaźnik
   komend Firebase (`/piec/cmd`), bo przeglądarka poza LAN nie widzi IP
   urządzenia; dotyczy to `tg_test`, `tg_config`, `rtc_sync`, `update`.
9. **Historia długoterminowa** — bufor szczegółowy w PSRAM + druga, zdarzeniowa
   historia długa (`/api/history/long`) pod wykresy LVGL/WWW.
10. **Proces wytwórczy**: semver (`v3.35.1`), changelog w nagłówku z szablonem
    *Co / Dlaczego / czego NIE ruszono*, tagi `[FIX-*]`/`[AUDYT-PKT*]`,
    skrypt **build-gate** (`run_v33120_build_gate.ps1`) uruchamiany przed release.
11. **Drugi fizyczny panel dotykowy LVGL** (ESP32 + ILI9488 3,5" + XPT2046,
    LVGL 8.3) — cienki klient HTTP Centrali; ekran alarmowy, wykresy,
    nieblokująca warstwa sieciowa. *(element sprzętowy — opcjonalny dla Ryb)*.
12. Pogoda/prognoza (`/api/prognoza`) — domenowo nieistotne dla akwarium.

### 2.2 Obszary, w których Ryby już są na poziomie Centrali lub wyżej

| Obszar | Uwagi |
|---|---|
| Logowanie LittleFS (rotacja, FS-GUARD, trim) | Centrala portowała 1:1 z Ryb |
| TCP keepalive, `select()` write-guard, DFS-lock 240 MHz | wzorce z Ryb v82/v83/v150/v50 |
| Archiwizacja coredump → LittleFS → Telegram | Ryby v240 ≈ rozwiązanie Centrali |
| Podział Core0/Core1 + budżet iteracji + circuit breaker sieciowy | Ryby mają (`ITER_HAS_BUDGET`, `_netCircuitOpen`) |
| Panel WWW jako single-file z responsywnym designem | Ryby były wzorcem dla Piec.html |
| Dokumentacja planów (`PLAN_*.md`) | Ryby mają (lwip/sntp, rampa/suwak itp.) |
| Dwufazowa trwałość konfiguracji (queue + PERSIST_ACK) | Ryby v257-v261 — Centrala nie ma odpowiednika |
| Ramp Arbiter / power-balancer / MIN LUX | unikalna domena Ryb |

### 2.3 Macierz luk (GAP ANALYSIS)

| # | Obszar | Centrala | Ryby | Priorytet |
|---|---|---|---|---|
| G1 | OTA z GitHub Releases (+karta panelu, /update w TG, komenda FB) | ✅ | ❌ (tylko espota) | **P0** |
| G2 | Bezpieczeństwo Firebase (TLS verify, UserAuth, reguły per urządzenie) | ✅ (H1/H2) | ❌ (LegacyToken + `setInsecure()`) | **P0 — bezpieczeństwo** |
| G3 | Rotacja/ekspozycja sekretów (`FIREBASE_SECRET`, hasło espota w kodzie) | ✅ baseline | ❌ sekrety w źródle | **P0 — bezpieczeństwo** |
| G4 | Trwała telemetria (Spool V2 + TelemetryRing + sessionNonce) | ✅ | ❌ | P1 |
| G5 | `log_krytyczny` + `/log_krytyczny` w Telegramie | ✅ | ❌ | P1 |
| G6 | Dyscyplina CORS na wszystkich endpointach (+OPTIONS) | ✅ | do zweryfikowania/brak | P1 |
| G7 | Zdalna obsługa panelu przez Firebase poza LAN | ✅ | częściowo (ścieżki `/aquarium/*` są, brak panelu w repo) | P1 |
| G8 | Historia długoterminowa `/api/history/long` | ✅ | ❌ (tylko `/api/history`) | P2 |
| G9 | Semver + CHANGELOG.md + build-gate przed release | ✅ | ❌ (vNNN, brak) | P1 |
| G10 | Release'y z artefaktami: `firmware.bin` **+ `.elf`** (addr2line) + changelog | częściowo | ❌ (ostatni release: 107, czerwiec) | P1 |
| G11 | Nowoczesne menu/raporty Telegram | ✅ (v3.34) | menu podstawowe | P2 |
| G12 | Struktura repo (firmware/panel/docs, bez binarnego balastu) | częściowo | ❌ (zip + kopia .pre-v253) | P1 |
| G13 | Panel dotykowy LVGL (druga płytka) | ✅ | brak sprzętu | P3/opcjonalnie |
| G14 | Zegar: oprócz NTP źródło czasu odporne na brak routera (RTC) | RTC+RTClib | tylko NTP | P2 (harmonogram!) |
| G15 | Harmonogram astronomiczny (wschód/zachód słońca z efemeryd) zamiast sztywnych godzin | pogoda tylko informacyjnie | ❌ | P2 (wysoka wartość domenowa) |

---

## 3. Wnioski

1. **Ryby są projektem dojrzałym inżyniersko** (architektura dwurdzeniowa,
   kolejki z trwałością, archiwizacja crashy, dyscyplina logów) — w warstwie
   niezawodnościowej to Centrala czerpała z Ryb, nie odwrotnie.
2. Realna przewaga Centrali koncentruje się w **czterech obszarach**:
   (a) pipeline wydań i OTA przez GitHub, (b) bezpieczeństwo Firebase,
   (c) trwała telemetria Spool V2, (d) proces wytwórczy (semver, changelog,
   build-gate, artefakty release).
3. Najszybsza i najważniejsza do zamknięcia luka to **G2/G3** — Database Secret
   w kodzie źródłowym + wyłączona weryfikacja TLS to ryzyko przejęcia pełnej
   kontroli nad urządzeniem przez każdego, kto pozna sekret (publiczny zip!).
4. Wartość domenowa ponad sam parytet: harmonogram astronomiczny (G15)
   i odporność zegara na awarię sieci (G14) — akwarium żyje rytmem dnia.
5. Konkretny plan podniesienia Ryb do poziomu Centrali (i miejscami wyżej):
   patrz **`docs/02_PLAN_UPGRADE_ryby_led.md`**.
