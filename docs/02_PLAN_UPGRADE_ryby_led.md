# PLAN UPGRADE — Ryby LED do poziomu Centrali Pieca (i dalej)

Data: 2026-10-09 · Autor: agent Arena.ai
Podstawa: `docs/01_RESEARCH_porownanie_ryby_vs_centrala.md`

## STATUS REALIZACJI (aktualizowane na bieżąco)

| Etap | Status | Release | Uwagi |
|---|---|---|---|
| 0. Repo + wersjonowanie | ✅ GOTOWY | 4.0.0 | commit `8e4776e`: struktura `firmware/docs/`, CHANGELOG.md, build_gate.ps1, usunięty zip/.pre-v253 |
| 1. OTA GitHub + release'y | ✅ KOD ZBUDOWANY + RELEASE v4.1.0 OPUBLIKOWANY | 4.1.0 | commity `cb16f00` + `037cd7a` (FIX-ORDER); build po stronie użytkownika OK, release v4.1.0 z `firmware.bin`+`.elf` publiczny; czeka: flash USB w domu + test OTA |
| 2. Bezpieczeństwo Firebase | 🟡 W TOKU (część 1/2 gotowa) | 4.1.1 | sekrety przeniesione do `src/secrets.h` (poza repo, `.gitignore`); do zrobienia po stronie użytkownika: nowy Database Secret w Firebase Console → wpisać do `secrets.h` → build → flash → **odwołanie starego sekretu**; potem zmiana CMD_TOKEN + hasła espota; część 2 (NVS+UserAuth) później |
| 3. Spool V2 + telemetria | 🟡 KOD + TESTY HOSTOWE GOTOWE | 4.2.0 | ring PSRAM + Spool V2 (CRC, commit) + sessionNonce + replay FIFO + `/api/telemetry/status`; 200 testów hostowych OK; DoD na płytce (zanik zasilania w trakcie wysyłki) do wykonania — procedura: `docs/03_ETAP3_SPOOL_V2.md`; brak jeszcze `/api/history/long` i panelu |
| 4. Panel: CORS/CORS-PN + karta OTA + tryb zdalny | ⬜ | — | CORS bazowy już jest (DefaultHeaders `*`); brakuje `Allow-Private-Network` i OPTIONS |
| 5. Log krytyczny + Telegram | ⬜ | — | |
| 6. Astronomia + RTC | ⬜ | — | |
| 7. Panel LVGL | ⬜ opcjonalny | — | |

### ⚠️ PILNE (poza kolejnością) — sekrety w publicznym zipie

W `firmware/src/Ryby_LED_fi_S3.cpp` są na stałe wpisane:
- `FIREBASE_SECRET "RhKVp49q..."` — **Database Secret: pełny RW na całej bazie
  RTDB, nigdy nie wygasa**. Zip z tym kodem wisiał w publicznym repo — każdy mógł
  go pobrać. **Natychmiastowa rotacja w Firebase Console** (Project Settings →
  Service accounts → Database secrets) unieważni stary sekret; firmware po
  rotacji straci łączność Firebase do czasu wpisania nowego sekretu
  (docelowo Etap 2 przenosi sekrety do NVS + UserAuth).
- `CMD_TOKEN "Akwarium2026!"` i hasło espota `AkwPanel2026!` — również do
  zmiany przy okazji.

Konwencja: każdy etap ma **cel**, **zakres** (co zmieniamy), **kryterium
zaliczenia** (Definition of Done) i **ryzyka**. Etapy P0 można robić od razu,
bez zmian w logice świecenia (zero ryzyka dla akwarium).

Zasada nadrzędna (przyjęta z praktyki obu projektów):
**każda zmiana = wpis w changelogu z Co / Dlaczego / czego NIE ruszono**,
fix ma tag `[TAG]`, wersja idzie w górę semver, przed release build-gate.

---

## ETAP 0 — Porządek w repozytorium i wersjonowaniu (fundament, ~0,5 dnia)

**Cel:** repo przestaje być „zipem z kopią zapasową”, staje się projektem
o strukturze Centrali; wersje przestają się rozjeżdżać.

**Zakres:**
1. Wypakować `ryby.zip` do struktury katalogów i usunąć zip oraz martwy plik
   `Ryby_LED_fi_S3.cpp.pre-v253` z repo (historię zachowuje git):
   ```
   firmware/
     platformio.ini
     partitions.csv
     boards/esp32-s3-n16r8.json
     sdkconfig.defaults            (wydzielić custom_sdkconfig do osobnego pliku)
     src/Ryby_LED_fi_S3.cpp
     src/terminal_html.cpp .h
     src/NetDiag.h
     src/log_printf_wrap.c
     scripts/build_gate.ps1        (z Etapu 1)
   panel/                          (panel v15 / zdalny — patrz Etap 4)
   docs/                           (ten dokument + plany PLAN_*.md z src/)
   CHANGELOG.md
   README.md
   version.txt
   ```
2. Zsynchronizować `version.txt` z firmware: przyjąć mapowanie
   `v261 → 4.0.0` (semver; numer wewnętrzny zostaje jako build metadata:
   `4.0.0+build.261`). Od tej pory: PATCH = fix, MINOR = funkcja,
   MAJOR = przełom (np. migracja Firebase z Etapu 2).
3. Założyć `CHANGELOG.md` — szablon wpisu 1:1 z Centrali:
   `[vX.Y.Z TAG] data / Co / Dlaczego / Logika sterowania i algorytmy:
   (nietknięte|zmienione)`. Przenieść tam istniejący changelog z nagłówka
   `.cpp` (nagłówek skrócić do ostatnich ~10 wersji).
4. Dodać `.gitignore` (`.pio/`, `*.elf` lokalne itp.) oraz `README.md`
   z opisem struktury (wzór README Centrali).

**DoD:** `pio run` przechodzi ze ścieżki `firmware/`; w repo nie ma plików
`.zip` ani kopii zapasowych; `version.txt` == wersja w kodzie.

**Ryzyka:** brak (zmiany organizacyjne). Uwaga: ścieżki `-I src` w
`platformio.ini` pozostają ważne po przeniesieniu (env buduje się względem
katalogu platformio.ini).

---

## ETAP 1 — Pipeline wydań + OTA przez GitHub Releases (P0, ~2–3 dni)

**Cel:** Ryby dostają dokładnie ten mechanizm aktualizacji, który ma Centrala:
urządzenie samo pobiera nowy firmware z GitHub Releases; zero espota „ręcznie
z laptopa” na co dzień.

**Zakres:**
1. **Port OTA-GitHub z Centrali** (`main_centrala.cpp`):
   - `GET https://api.github.com/repos/zielinski25/ryby-led/releases/latest`
     (HTTPClient + User-Agent), parsowanie `tag_name` i `browser_download_url`
     assetu `firmware.bin`,
   - pobranie z obsługą przekierowań 302 (`HTTPUpdate`/WiFiClientSecure),
   - `Update.begin/write/end` + weryfikacja + `esp_ota_mark_app_valid_cancel_rollback()`
     po starcie (partycje dual-OTA już są!),
   - porównywanie wersji semver przed flashem (nie cofać bez flagi),
   - wyzwalacze — wszystkie trzy, jak w Centrali:
     a) karta „Aktualizacja firmware (OTA & GitHub)” w panelu WWW,
     b) komenda Telegram `/update`,
     c) komenda Firebase `update` w `/aquarium/commands`.
2. **Skrypt build-gate** (`firmware/scripts/build_gate.ps1`, wzór:
   `run_v33120_build_gate.ps1` Centrali): kompilacja obu env + kontrola
   spójności wersji (`RYBY_FW_VERSION` vs `version.txt` vs CHANGELOG top)
   + sprawdzenie rozmiaru bin < 3 MB (partycja!). Uruchamiany przed każdym
   release.
3. **Proces release** (udokumentować w `docs/RELEASE.md`):
   - tag semver → GitHub Release z assetami: **`firmware.bin` + `firmware.elf`**
     (elf konieczny do addr2line — lekcja z v239, gdzie user musiał ręcznie
     szukać ELF!) + skopiowany CHANGELOG wpisu,
   - aktualizacja `version.txt`.

**DoD:** flash testowy: wgranie release N, potem wystawienie release N+1 →
urządzenie aktualizuje się samo z karty panelu i z `/update`; po restarcie
`/api/status.fwVersion` pokazuje nową wersję; rollback działa (test celowo
uszkodzonym binem — odmowa flasha).

**Ryzyka:** brak coredump-partycji nie dotyczy (jest). Uwaga na limit 3 MB
partycji app — elf/bin budować z aktualnym `custom_sdkconfig`
(PSRAM_FETCH_INSTRUCTIONS zmienia rozmiar!).

---

## ETAP 2 — Bezpieczeństwo Firebase: TLS + UserAuth (P0, ~2–4 dni)

**Cel:** zamknięcie najpoważniejszej luki: `FIREBASE_SECRET` (Database Secret,
nigdy nie wygasa, pełny RW na całej bazie) siedzi w kodzie źródłowym, który
krąży jako zip, a TLS jest nieweryfikowany (`setInsecure()`).

**Zakres (port COMMITÓW H1/H2 Centrali):**
1. **Sekrety poza kodem**: `FIREBASE_SECRET`/tokeny → NVS (Preferences),
   konfigurowane kartą panelu (jak karta Telegram: `/api/telegram/save`) —
   usuwamy definicje z `.cpp` i z `platformio.ini`. **Rotacja sekretów
   w konsoli Firebase natychmiast po wdrożeniu** (stary sekret jest w zipie!).
2. **Weryfikacja TLS**: zastąpić `setInsecure()` certyfikatem CA / bundle
   (wzór H1 Centrali) + test „fałszywy serwer” (odmowa połączenia).
3. **Migracja LegacyToken → UserAuth** (H2): konto techniczne urządzenia,
   reguły RTDB izolujące `/aquarium/<deviceId>/…` per urządzenie; token
   z odświeżaniem (FirebaseClient już to umie — `getAuth` z UserAuth).
   Zostawić feature-flagę powrotu do LegacyToken na czas przejścia
   (NVSettings), tak jak zrobiła to Centrala.
4. Hasło espota (`AkwPanel2026!`) — również do NVS/build-secrets poza repo.

**DoD:** `git grep FIREBASE_SECRET` = pusto; przechwycony MITM-em ruch TLS
zostaje odrzucony; konto bez reguł nie zapisuje do `/aquarium/<deviceId>`.

**Ryzyka:** utrata autoryzacji = utrata zdalnego sterowania → feature-flaga
i procedura awaryjnego powrotu; testować na płytce, nie „na żywo” w nocy.

---

## ETAP 3 — Trwała telemetria: Spool V2 + TelemetryRing + sessionNonce (P1, ~4–6 dni)

**Cel:** dane akwarium (temp. wody, lux pokój/woda, PWM kanałów, moc W, stan
trybów) przeżywają restart i brak sieci tak, jak w Centrali — rekordy z CRC,
batche, ACK/REPLAY, recovery po zaniku zasilania.

**Zakres:**
1. Port `TelemetryRing` (SPSC, PSRAM) i **Persistent Spool V2** z Centrali
   (seria COMMIT-B/G, moduły R245–R470 — kod jest już przetestowany
   macierzowo, przenosimy 1:1 z adaptacją próbek do domeny akwarium).
2. `sessionNonce` (COMMIT-B) — efemeryczny identyfikator sesji.
3. Próbki domenowe: `temp_woda`, `lux_pokoj`, `lux_woda`, `pwm[5]`, `moc_W`,
   `tryb`, `power`, `minlux_active`, `uptime`, `wifi_rssi`.
4. Eksport: rozszerzenie `/api/history` + nowy **`/api/history/long`**
   (historia zdarzeniowa, wzór Centrali) pod wykresy tygodniowe w panelu.
5. Wysyłka do Firebase jako batch (mniej zapisów → mniejsze blokady,
   zgodne z filozofią Turbo-queue Ryb).

**DoD:** test „wyjmij zasilanie na 5 min w trakcie wysyłki” → po powrocie
batche są dosyłane (REPLAY), CRC odrzuca uszkodzone rekordy (test negatywny
jak R250 Centrali); pamięć: pomiar zajętości PSRAM przed/po (budżet!).

**Ryzyka:** PSRAM jest już zajęta przez logi + kod (`SPIRAM_FETCH_INSTRUCTIONS`)
— wymagany audyt pamięci przed startem; nie dotykać Ramp Arbitera i MIN LUX.

---

## ETAP 4 — Panel WWW: CORS, karta OTA, tryb zdalny poza LAN (P1, ~3–5 dni)

**Cel:** panel Ryb dostaje to, co Piec.html dostał w v3.31–v3.33.

**Zakres:**
1. **CORS na wszystkich endpointach** `/api/*` i `/aquarium/*`
   (`Access-Control-Allow-Origin: *`, `Access-Control-Allow-Private-Network`,
   handlery OPTIONS) — wzór v3.32.1 Centrali.
2. Karty panelu: „Aktualizacja firmware (OTA & GitHub)” (Etap 1),
   „Log krytyczny” (Etap 5), odświeżona karta Telegram/WiFi (już są).
3. **Tryb zdalny**: skoro ścieżki `/aquarium/commands|config|status` już
   istnieją — dodać do panelu warstwę „poza LAN → Firebase” dokładnie wg
   wzoru Piec.html (fetch lokalny, fallback na przekaźnik komend).
   *Pytanie do właściciela:* gdzie leży „panel v15”, o którym mówi komentarz
   v246? Jeśli istnieje poza repo — dołączyć go do `panel/`.
4. Uzupełnić `docs/` o strukturę panelu i instrukcję otwierania zdalnego.

**DoD:** panel otwarty z telefonu na LTE (poza WiFi domowym) steruje
oświetleniem przez przekaźnik Firebase; skan sieci i OTA działają z innego
originu (test bez błędów CORS).

---

## ETAP 5 — Log krytyczny + Telegram parity (P1/P2, ~2–3 dni)

**Zakres:**
1. **`log_krytyczny.txt`** (wzór v3.35.0 Centrali): osobny plik LittleFS na
   zdarzenia alarmowe (przegrzanie? błąd czujnika 10× z rzędu, awaria
   zasilania LED, crash) + endpoint `/api/log-critical-download` + komenda
   Telegram `/log_krytyczny` (wysyłka dokumentem).
2. **Modernizacja menu bota** (wzór v3.34.0 Centrali): płaskie menu
   2-kolumnowe z emoji, pogrupowane raporty (Status / Czujniki / Harmonogram /
   Diagnostyka / Logi / OTA), natychmiastowy dostęp do `/status`, `/temp`,
   `/lux`, `/harmonogram`, `/update`.
3. Zachować istniejącą automatyczną wysyłkę coredumpów (v240) — tego Ryby
   nauczyły Centralę, nie ruszamy.

**DoD:** z czatu da się pobrać log krytyczny i odpalić OTA; menu ma ≤1 klik
do każdego raportu.

---

## ETAP 6 — Wartość domenowa ponad parytet (P2, ~3–4 dni)

Rzeczy, których Centrala nie ma, a akwarium skorzysta — „level wyżej”:

1. **Harmonogram astronomiczny**: wschód/zachód + świt cywilny liczony
   z efemeryd (biblioteka `suncalc`-podobna, offline, bez API) dla szerokości
   Krakowa (konfigurowalnej) → rampy start/koniec podążają za prawdziwym dniem.
   Fallback: harmonogram sztywny (dzisiejszy) przy braku czasu.
2. **Odporność zegara**: dziś harmonogram jedzie wyłącznie z NTP — brak
   routera = brak czasu po dłuższym restarcie. Opcje: (a) RTC DS3231 na I2C
   (jak Centrala, biblioteka RTClib), (b) tryb „ostatni znany czas + drift”
   z logowaniem niepewności. Rekomendacja: (a), ~10 zł hardware.
3. **Karta „Dzień” w panelu**: podgląd krzywej świateł na dziś (harmonogram +
   adaptacja + MIN LUX na osi czasu) — unikalna funkcja Ryb, łatwa z już
   istniejących danych.

---

## ETAP 7 (opcjonalny) — Panel dotykowy LVGL przy akwarium (P3)

Jeśli będzie potrzeba fizycznego sterownika przy zbiorniku: port architektury
panelu LCD Centrali (cienki klient HTTP, LVGL 8.3, nieblokujący fetch) —
kod Centrali jest gotowym szablonem; hardware: ESP32 + TFT SPI + XPT2046.
Decyzja poza zakresem tego planu.

---

## Mapa priorytetów i szacunki

| Etap | Priorytet | Pracochłonność | Zależności |
|---|---|---|---|
| 0. Repo + wersjonowanie | P0 | 0,5 dnia | — |
| 1. OTA GitHub + build-gate + release'y | P0 | 2–3 dni | 0 |
| 2. Bezpieczeństwo Firebase | **P0 (najważniejszy)** | 2–4 dni | 0 |
| 3. Spool V2 + telemetria | P1 | 4–6 dni | 0, audyt PSRAM |
| 4. Panel: CORS + OTA + tryb zdalny | P1 | 3–5 dni | 1, 2 |
| 5. Log krytyczny + Telegram | P1/P2 | 2–3 dni | 0 |
| 6. Astronomia + RTC + karta dnia | P2 | 3–4 dni | 0 |
| 7. Panel LVGL | P3 | opcjonalnie | 4 |

Razem do pełnego parytetu (etapy 0–5): **~12–18 dni roboczych**.

## Zasady bezpieczeństwa zmian (obowiązkowe)

1. Nie dotykać w jednym release: Ramp Arbitera, MIN LUX, adaptacji i modułu
   Firebase naraz — jeden obszar = jeden release.
2. Każdy etap kończy się releasem z artefaktami (bin+elf) i wpisem CHANGELOG.
3. Przed etapem 3 — pomiar wolnej PSRAM (`heap_caps_get_free_size`) i budżet
   na spool (nie więcej niż Centrala: strony 4 KB, twarda kwota — wzór R251).
4. Zmiany sieciowe testować scenariuszem „router odłączony” (lekcja v239).
5. Po każdej migracji sekretów — rotacja starych wartości w konsoli Firebase.

## Otwarte pytania do właściciela

1. Gdzie jest źródło „panela v15” (wspomniany w v246 użytkownik komend
   `/aquarium/commands`)? → dołączyć do `panel/`.
2. Czy przejście na UserAuth Firebase może odbyć się z oknem dual-auth
   (stary+nowy sposób przez 1–2 tygodnie)?
3. Czy jest zgoda na dokupienie DS3231 (Etap 6.2)?
4. Czy release'y mają pozostać publiczne (publiczny repo = publiczne binaria)?
