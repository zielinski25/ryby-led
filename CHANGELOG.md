# CHANGELOG — Ryby LED

Od 2026-10-09 obowiązuje numeracja **semver**. Mapowanie ustalone w Etapie 0
planu upgrade: wewnętrzne **v261 = 4.0.0+build.261**. Wpisy poniżej
(powstałe przed tą datą) zachowują oryginalną numerację wewnętrzną vNNN
jako nienaruszony zapis historyczny — przeniesione 1:1 z nagłówka
`firmware/src/Ryby_LED_fi_S3.cpp`.

---

## [4.7.0+build.269] ASTRO — 2026-10-10

- Co: Etap 6 planu upgrade, pkt 1 — **moduł efemeryd słonecznych** (`firmware/src/astro.h/.cpp`):
  wschód, zachód, świt i zmierzch cywilny z równań NOAA dla dowolnej szerokości/długości,
  offline, bez API.
- **Zachód słońca podłączony:** `obliczZachodSlonca` (wyliczający `sunsetMinutes` dla rampy
  wieczornej) używa `astro::compute`. Poprzedni wzór odchylał się do −4,4 min (zima/wiosna);
  nowy jest zgodny z `astral` ±2 min. Kontrakt bez zmian (minuty UTC, `-1` = brak zachodu),
  więc strefa, zmiana czasu, `EVENING_ON_BEFORE_SUNSET_MIN` i fallback 19:00 działają jak dotąd.
- **Lokalizacja z panelu:** szerokość/długość ustawiane w panelu LAN (karta „Lokalizacja –
  zachód słońca”), zapisywane na ESP w NVS (namespace `astro_loc`), bez przeflashowania.
  `GET /api/location` (lat, lon, zachód HH:MM, czy czas z NTP), `POST /api/location` z JSON
  `{"lat":..,"lon":..}`. Walidacja: lat −90..90, lon −180..180; zła wartość → 400, zachowana
  poprzednia. Zmiana → zachód przeliczany w następnym obiegu pętli. Brak wpisu w NVS → domyślne
  52,1345 / 20,1418. Test: `check_location.sh` (27 asercji, endpointy wyciągnięte z Ryby).
- **Nie podłączone:** świt i wschód (rampa poranna nadal stała godzina). Ramp Arbiter
  i MIN LUX nietknięte. Decyzje — `docs/06`, sekcja 4.
- Testy: `test_astro.cpp` (ASan+UBSan, 186 asercji): 22 pary data×miejsce (Kraków, Warszawa,
  Gdańsk, Tromsø, Sydney, dom 52,1345/20,1418) z tolerancją 2 min, kolejność zdarzeń, długość dnia,
  tzMin, polarny dzień/noc (Tromsø), walidacja wejścia. Generator wartości: `astro_ref_gen.py`.
  `check_integration_hl.sh`: prawdziwe ciało `obliczZachodSlonca` vs `astral` (±2 min).
- Wersja: 4.7.0+build.269.

---

## [4.6.0+build.268] MENU-GROUP — 2026-10-10

- Co: Etap 5 planu upgrade, pkt 2 — **pogrupowane menu Telegrama** (`docs/04`, sekcja 3).
  - Płaska siatka 2-kolumnowa z nagłówkami sekcji: ⚙️ STEROWANIE, 📊 STATUS, 🌡️ CZUJNIKI,
    ⏰ HARMONOGRAM, 🩺 DIAGNOSTYKA, 📋 LOGI, 🔁 OTA.
  - Nagłówki to przyciski `noop` (klik nic nie robi, tylko potwierdza callback).
  - Każdy raport nadal w **jednym** kliknięciu, bez podmenu (warunek DoD).
  - `PL_CAP` 1500 → 2600 (menu ~1,3 KB, bufor w PSRAM).
- Testy: `check_tg_menu.py` (krok 7 w `run_tests.sh`): wyciąga JSON z kodu, podstawia atrapy,
  sprawdza parsowanie, limit bufora, zgodność callbacków z dispatcherem, brak duplikatów,
  obecność wszystkich raportów i sekcji. Negatywnie: za mały `PL_CAP` i literówka w callbacku
  są wykrywane.
- Wersja: 4.6.0+build.268. Nie testowano w Telegramie na płytce (`docs/04`, sekcja 6).

---

## [4.5.0+build.267] HIST-LONG — 2026-10-10

- Co: dopięcie Etapu 3 planu upgrade (`docs/02`, pkt 4; gap G8 w `docs/01`), szczegóły w
  `docs/05_ETAP3_HISTORIA_DLUGA.md`: **historia długa** pod wykresy tygodniowe.
  - Nowy moduł `histlong` (`firmware/src/histlong.h/.cpp`): plik `/history_long.csv`
    (+ `/history_long_old.csv`), jeden wiersz na kubełek 30 min (średnia z próbek 5-min,
    `ts` unix, `probki`). Rotacja przy 40 KB. Odczyty poza zakresem (np. −127 z DS18B20)
    nie wchodzą do średniej. Zapis tylko po NTP.
  - Endpoint `GET /api/history/long` (`text/csv`, odpowiedź chunked, archiwum + bieżący,
    nagłówek raz, jeden klient naraz — drugi dostaje 503).
  - **Naprawa kolizji tras:** `GET /api/history` przykrywał `/api/history/clear`
    (goły string = prefiks). Trzy trasy `/api/history*` przechodzą na `AsyncURIMatcher::exact`.
  - `/api/history/clear` kasuje też pliki long.
  - Panel `akwarium-firebase-panel-v14.html`: zakładka **Wykresy** ma kartę „Historia tygodniowa”
    (4 wykresy SVG, przełączniki, pobieranie CSV). Tylko w LAN; zdalnie overlay.
- Testy: `test_histlong.cpp` (47 asercji, ASan+UBSan), `check_integration_hl.sh`
  (syntax-only endpointu i haka w `Ryby_LED_fi_S3.cpp`, sprawdzenie `exact`), testy jsdom
  panelu (wykres, CSV, LAN/zdalnie). **Nie testowano na ESP32** — patrz `docs/05`, sekcja 7.
- Niezmienione: Spool V2 (`telemetry_spool`), Ramp Arbiter, MIN LUX, coredump v240, log krytyczny.

---

## [4.4.0+build.266] CRIT-LOG — 2026-10-10

- Co: Etap 5 planu upgrade (`docs/02_PLAN_UPGRADE_ryby_led.md`, szczegóły w
  `docs/04_ETAP5_LOG_KRYTYCZNY.md`): **log krytyczny + menu Telegram**.
  - Nowy moduł `critlog` (`firmware/src/critlog.h/.cpp`): osobny plik
    `/log_krytyczny.txt` na LittleFS, nie rusza go rotacja `log_a`/`log_b`.
    Limit 256 KB, archiwum `_old`, łącznie ≤512 KB.
  - Zdarzenia krytyczne: start po PANIC/WDT/BROWNOUT (`BOOT`), utrata czujnika
    światła po 5 próbach (`TSL`), czujnik wody DS18B20 FAIL przy starcie (`DS-DIAG`),
    błędy OTA (`OTA-GH`).
  - Endpointy: `GET /api/log-critical-status`, `GET /api/log-critical-download[?old=1]`.
  - Telegram: komenda `/log_krytyczny` (alias `/krytyczny`) wysyła plik jako dokument;
    menu: przyciski **🔁 Aktualizacja OTA** i **🚨 Log krytyczny**; `/ota` jako alias `/update`.
  - Panel `akwarium-firebase-panel-v14.html`: karta „Log krytyczny” w zakładce Logi
    (rozmiar, pobieranie, archiwum), tylko w LAN.
- Testy: `test_critlog.cpp` (424 asercje, ASan+UBSan), `check_integration_e5.sh`
  (syntax-only bloku z `Ryby_LED_fi_S3.cpp` i helpera z `ota_github.cpp`),
  testy jsdom panelu. **Nie testowano na ESP32** — patrz `docs/04`, sekcja 6.
- Atrapa LittleFS (`stubs/LittleFS.h`): tryb `"a"` dopisuje (wcześniej nadpisywał).

## [4.3.0+build.265] PANEL-CORS — 2026-10-10

- Co: Etap 4 planu upgrade (`docs/02_PLAN_UPGRADE_ryby_led.md`): panel WWW obsługuje
  tryb LAN i zdalny.
  - **Firmware (CORS/PNA):** nagłówek `Access-Control-Allow-Private-Network: true`,
    `Access-Control-Max-Age: 600`, a preflight `OPTIONS` dla dowolnej ścieżki API zwraca 204
    (`webserialServer.onNotFound`). Wzór: Centrala Pieca v3.32.1/v3.32.2.
  - **Firmware (OTA):** `/api/ota-start` akceptuje `force=1` w query (`?force=1`) albo w body.
  - **Panel → `panel/akwarium-firebase-panel-v14.html`:**
    - badge trybu połączenia w nagłówku (LAN ✓ / ZDALNIE (Firebase));
    - karta „Aktualizacja firmware (OTA)”: lokalnie `POST /api/ota-start`, zdalnie komenda
      `update` przez Firebase;
    - karta „Skan sieci Wi-Fi” (tylko LAN);
    - sekrety (`DB_SECRET`, `CMD_TOKEN`) z `localStorage`, nie z pliku.
- Testy: firmware: fragment CORS/OPTIONS sprawdzony `-fsyntax-only` z atrapami
  ESPAsyncWebServer. Panel: `node --check` + testy jsdom (9 testów: tryby LAN/zdalnie,
  OTA, skan, escapowanie HTML). **Nie testowano** na ESP ani z telefonu na LTE.
- Bez zmian: sterowanie i odczyt nadal przez Firebase. Logika Ramp/MIN LUX nietknięta.

## [4.2.0+build.264] SPOOL — 2026-10-10

- Co: Etap 3 planu upgrade (`docs/02_PLAN_UPGRADE_ryby_led.md`, szczegóły w
  `docs/03_ETAP3_SPOOL_V2.md`): **trwała telemetria akwarium** w stylu Centrali.
  - `TelemetryRing` — bufor SPSC w PSRAM (288 próbek × 48 B = 13,8 KB), próbka
    co 5 min (ten sam cykl co wpis historii CSV).
  - **Persistent Spool V2** na LittleFS (`/spool/pNNNNNNNNNN.spl`): nagłówek 64 B,
    rekordy 48 B próbki + CRC32, stopka 64 B = commit. Strona przed publikacją
    jest weryfikowana (CRC nagłówka, stopki, payloadu i każdego rekordu).
  - `sessionNonce` (losowany przy każdym starcie) — klucze RTDB
    `<nonce>_<sampleSeq>`, więc ponowne wysłanie batcha jest bezpieczne.
  - Replay FIFO: `PATCH /aquarium/telemetry/v1`, max 12 rekordów na żądanie,
    jako operacja asynchroniczna w schedulerze Firebase (najniższy priorytet).
  - Nowy endpoint diagnostyczny `GET /api/telemetry/status` (ring, liczniki,
    strony na FS, sessionNonce, wolny PSRAM).
  - Spill do FS: gdy offline ≥ 60 s albo ring ≥ 6 próbek (30 min); polityka
    wolnego miejsca: najpierw kasowanie najstarszych stron (< 64 KB wolnego lub > 240 stron).
  - Przy starcie: usuwane osierocone `*.part` (przerwany zapis przy zaniku zasilania).
- Testy: `firmware/tests/host/run_tests.sh` — 200 testów logiki Spool V2 (ASan + UBSan)
  oraz kompilacja bloku integracyjnego z `Ryby_LED_fi_S3.cpp`. Test na płytce (zanik
  zasilania w trakcie wysyłki) — procedura w `docs/03`, do wykonania przed release.
- Dlaczego: dane akwarium (temperatury, lux, PWM, moc) mają przeżywać restart i brak
  sieci; po zaniku zasilania odtwarzane są strony zapisane na FS, a uszkodzone
  rekordy są odrzucane zamiast wysyłane.
- Logika sterowania: NIETKNIĘTA (Ramp Arbiter, MIN LUX, PWM, Core 1). Zmiany tylko
  w `saveHistoryPoint()` (producent), `tgTaskFn` (spill), schedulerze FB (replay).
- Ograniczenia: próbki zalegające w RAM (≤ 30 min) przepadają przy zaniku zasilania —
  zapisane strony już nie. `/api/history/long` i kanały wykresów tygodniowych — w kolejnym
  kroku Etapu 3. Retencja węzłów `/aquarium/telemetry` w RTDB — do ustalenia.
- Kolejność wdrożenia: najpierw flash **4.1.1** (sekrety, zbudowany i sprawdzony),
  dopiero potem **4.2.0** po teście na płytce.

---

## [4.1.1+build.263] SECRETS — 2026-10-09

- Co: Etap 2 planu upgrade (część 1): sekrety poza kodem. `FIREBASE_HOST`,
  `FIREBASE_SECRET` i `CMD_TOKEN` przeniesione z `Ryby_LED_fi_S3.cpp` do
  `src/secrets.h` (dodany do `.gitignore` — nigdy w repo); w repozytorium tylko
  szablon `secrets.example.h`. Osłona `__has_include` + `#error` z instrukcją,
  gdy pliku brak. Wersja `v4.1.1+build.263`.
- Dlaczego: stare wartości sekretów wyciekły w publicznie udostępnionym zipie
  źródeł. Kolejność rotacji: (1) nowy Database Secret w Firebase Console (stary
  jeszcze działa — urządzenie nie traci łączności), (2) wpis do `secrets.h`,
  (3) build + flash, (4) dopiero po starcie nowego firmware'u odwołanie starego
  sekretu w konsoli + zmiana `CMD_TOKEN` (wpisać nowy w panelu HTML).
- Logika sterowania: NIETKNIETA (zmiana wyłącznie organizacyjna).
- Część 2 Etapu 2 (NVS + UserAuth zamiast Legacy Database Secret): później.

---

## [4.1.0+build.262] OTA-GITHUB — 2026-10-09

- Co: Etap 1 planu upgrade (`docs/02_PLAN_UPGRADE_ryby_led.md`): **OTA przez
  GitHub Releases** — nowy, samodzielny moduł `firmware/src/ota_github.h/.cpp`
  (port 1:1 sprawdzonego mechanizmu z Centrali Pieca). Endpointy
  `GET /api/ota-status` i `POST /api/ota-start[?force=1]`; wyzwalacze: panel,
  komenda `/update` (Telegram), `update`/`update_firmware`/`ota` (Firebase
  `/aquarium/commands`), konsola WS (`ota`, `ota status`). Task OTA na Core 0
  (reguła architektury), stos 16 KB w DRAM, karmienie TWDT podczas flash,
  anty-rollback (`esp_ota_mark_app_valid_cancel_rollback` w setup),
  porównanie semver (downgrade tylko z `force=1`), restart przez istniejący
  mechanizm `restartRequestedAt` (czysty restart z PRE_RESET_UPDATE + flush
  logów). Release: tag semver + asset `firmware.bin` (+ zalecany `firmware.elf`
  do addr2line) w `zielinski25/ryby-led`.
- Dlaczego: dotąd jedyne OTA = espota z hasłem (wymaga laptopa w LAN); parytet
  z Centralą Pieca i zdalne aktualizacje bez espota.
- FIX-ORDER (kompilacja po reorganizacji V262): deklaracje wyprzedzające dla
  symboli zdefiniowanych niżej w pliku (`Komentarze`, `rampaAdaptacyjnaAktywna`,
  `rampaMinLuxPriorytet` przed Ramp Arbiter; `fbParseConfigSnapshot`,
  `enqueueFirebaseConfigSnapshot` przed sekcją fbAsync), przywrócony typ
  `PumpSlot` tablicy `pumpSlots`, prototyp `wykonajKomendeFirebase(cmd, ts, key)`
  zgodny z definicją, literówka `fbAsyncTimeoutCount` → `g_fbAsyncTimeoutCount`,
  include `ota_github.h` przeniesiony na początek pliku; w `ota_github.cpp`
  API IDF 5.5 (`esp_ota_get_state_partition`, `ESP_OTA_IMG_PENDING_VERIFY`).
  Bez zmian logiki sterowania.
- Logika sterowania i algorytmy: NIETKNIETE (LED/adaptacja/harmonogram/
  MIN LUX/rampy/Ramp Arbiter bez zmian; transport Firebase/Telegram/logi bez
  zmian — moduł w 100% samodzielny).
- Weryfikacja: kompilacja pełna po stronie użytkownika (sandbox agenta nie ma
  dostępu do CDN assetów GitHub — toolchain); składnia modułu zweryfikowana
  parserem. Przed flashem: `firmware/scripts/build_gate.ps1`.

---

## [4.0.0+build.261] REPO-RESTRUCTURE — 2026-10-09

- Co: Etap 0 planu upgrade (docs/02_PLAN_UPGRADE_ryby_led.md): repozytorium
  przebudowane ze struktury "ryby.zip" na firmware/ + docs/; ten plik
  CHANGELOG.md utworzony z pełnej historii z nagłówka .cpp (nagłówek
  skrócony do ostatnich 10 wersji); version.txt zsynchronizowany
  z firmware (107 -> 4.0.0); usunięto z repo kopię .pre-v253 i zip.
- Dlaczego: rozjechane wersje (release 107 vs firmware v261) i brak
  struktury blokowały pipeline wydań (Etap 1) i parytet z Centralą Pieca.
- Logika sterowania i algorytmy: NIETKNIETE (zmiany wyłącznie organizacyjne
  + wersja wyświetlana).

---

## Historia wewnętrzna (numeracja vNNN)

### v261 (2026-09-27) - CONFIG QUEUE BACKPRESSURE.
     Gdy fbConfigApplyQueue jest pełna, snapshot nie powoduje już
     kolejnych GET /aquarium/config. Core 0 przechowuje immutable
     snapshot i ponawia enqueue z backoffem, zachowując config_refresh
     pending do chwili przejęcia snapshotu przez Core 1.
### v259 (2026-09-27) - FIREBASE LEGACY PATH REMOVAL.
     Trzy stare, blokujące implementacje sendStatusToFirebase(),
     checkFirebaseCommands() i checkFirebaseConfig() zostały zastąpione
     lekkimi wrapperami ustawiającymi pending flagi. Aktywny transport
     Firebase pozostaje wyłącznie w native async schedulerze na Core 0.
### v258 (2026-09-27) - CONFIG DURABLE ACK / TWO-PHASE APPLY.
     v257 potwierdzał zastosowanie runtime zanim Core 0 zakończył trwały zapis.
     W v258 cały immutable FirebaseConfigSnapshot trafia do osobnej
     fbConfigPersistQueue. Core 0 wykonuje komplet EEPROM.commit() + zapis
     LittleFS, retry przy błędzie, i dopiero wtedy wysyła PERSIST_ACK.
     Core 1 aplikuje runtime dopiero po PERSIST_ACK; dopiero po kolejnym
     FirebaseConfigAck Core 0 zapisuje cfgTs i uzbraja DELETE config_refresh.
     Dzięki temu restart/awaria storage nie może potwierdzić konfiguracji
     Firebase jako wykonanej przed trwałą persystencją.
### v257 (2026-09-27) - CONFIG RUNTIME/STORAGE SPLIT.
     Firebase config_refresh jest parsowany na Core 0 do immutable snapshotu,
     snapshot trafia przez queue do Core 1, a potwierdzenie zastosowania wraca
     przez ACK. Dopiero po ACK Core 0 zapisuje cfgTs i usuwa config_refresh.
     Runtime EEPROM/LittleFS nie wykonuje już bezpośrednich commitów: wartości
     są kolejkowane do storageQueue, a zapis/commit realizuje wyłącznie Core 0.
     Dodano retry nieudanego EEPROM.commit() i jawne logowanie przepełnienia
     kolejek. onPowerChange()/onTrybChange() korzystają wyłącznie z RAM.
### v252 (2026-09-26) - RAMP-ARBITER: jeden właściciel zasobu PWM.
     Dodano Ramp Arbiter z wykrywaniem właściciela, blokadą startu automatycznych
     ramp oraz kontrolowanym wywłaszczeniem dla poleceń użytkownika. Arbiter
     rozwiązuje kolizję soft-start/adaptacja/harmonogram/transition i loguje
     BLOCK/PREEMPT/COLLISION_RESOLVED. Zachowane zostały istniejące mechanizmy
     ramp, ale nie mogą już równolegle prowadzić backupBrightnessComposite.
     W tej wersji nie zmieniono hardware/PWM API ani protokołu Firebase.
### v246 (2026-09-26) - FIREBASE-TURBO-QUEUE-ONLY: tylko /aquarium/commands.
     Polling komend/kolejki pozostaje co 1 s, ale status Firebase jest
     wysyłany co 10 s zamiast co 1 s. Podczas Turbo pomijany jest legacy
     fallback /aquarium/cmd, ponieważ panel v15 używa /aquarium/commands.
     Dzięki temu jedna iteracja nie wykonuje już dwóch GET-ów komendy
     przy 1-sekundowym Turbo. Celem jest ograniczenie blokad Firebase,
     timeoutów i wtórnych rozłączeń terminala LAN.
### v244 (2026-09-26) - FIX-FB-STATE-CALLBACK-RACE: Firebase Core 0 no longer
     executes onPowerChange()/onTrybChange() directly. It only changes the
     requested state; the existing Core 1 debug/change detector in loop()
     applies the callback exactly once. This removes the observed race where
     Core 1 saw the new `tryb/power` before Core 0 synchronized prevTrybGlobal,
     causing duplicate WYWOLANIE/ZAPIS/EEPROM/COUNTER/PRZEJSCIE side effects.
     Firebase path no longer performs duplicate EEPROM/updateLEDs work for
     power changes either.
### v243 (2026-09-26) - FIREBASE-COMMAND-QUEUE: robust first-key queue + config_refresh + 64-bit ts.
### v240 (2026-09-26) - CRASH-ARCHIVE: domkniecie lancucha crash -> Telegram.
     Dotychczas po restarcie z PANIC/WDT kod tylko czytal summary coredumpu
     i kasowal zrodlo przez esp_core_dump_image_erase(), przez co surowy plik
     nie nadawal sie do pozniejszego wyslania. Dodano archiwizacje raw obrazu
     do LittleFS (/coredumps), trwaly seq w NVS, retencje 20 plikow oraz
     automatyczna kolejke wysylki do Telegrama. Wysylka dziala po starcie
     taska Telegram i retryuje po awarii sieci. Coredump jest kasowany ze
     zrodla dopiero po poprawnym zweryfikowaniu pliku archiwum.

     Dodatkowo poprawiono Firebase Turbo: okno 60 s nadal jest odswiezane
     tylko przez prawidlowe, nowe komendy po walidacji tokenu i identyfikatora
     polecenia; sam uszkodzony payload nie aktywuje juz kosztownego turbo.
     Turbo pozostaje 1s komendy / 10s status, a po minucie ciszy wraca do wartosci normalnych.

### v239 (2026-09-05) - FIX-SNTP-FB-RACE: crash zdekodowany przez addr2line
     (użytkownik przesłał prawdziwy firmware.elf, potwierdzony jako ten
     sam v238 co crashował) na realnym teście "odłączony ruter":
     setup() -> fbInitialize() [17052] -> WiFi.hostByName() -> lwIP DNS ->
     assert() w udp_new_ip_type (udp.c:1278). W tym samym momencie SNTP
     (uruchomione configTzTime(), ~188 linii wcześniej) samo w tle próbuje
     kolejnego serwera NTP (sntp_try_next_server->sntp_dns_found) - dwa
     niezależne wywołania DNS w nie-reentrantnym module lwIP zderzają się.
     Nasila się przy złej sieci (ruter odłączony) - własna pętla czekania
     na NTP w setup() (10x1s) i tak się kończy, ale NIE zatrzymuje klienta
     SNTP, który dalej ponawia w tle, częściej niż przy dobrej sieci.
     TWARDE OGRANICZENIE (już z v228): configTzTime()/sntp_stop() wołane
     >1x w programie to osobna, znana przyczyna TEGO SAMEGO assertu
     (arduino-esp32 #10675) - więc nie wolno "zatrzymać SNTP na chwilę".
     Fix: 3s przerwa osadzająca (WDT-safe, po 100ms) między końcem sekcji
     NTP a pierwszym fbInitialize() - zmniejsza (NIE gwarantuje ze 100%
     pewnością, patrz Ryby_LED_S3_plan_fix_sntp_race.md) prawdopodobieństwo
     zazębienia w czasie. Prawdziwy pewny fix wymagałby łatki
     lwIP/arduino-esp32 (poza zasięgiem tego projektu).
     Backtrace WDT_TASK z tego samego logu (checkpoint=L:wifiCheck+sensors,
     uptime=12s) prawdopodobnie ta sama rasa, tylko bez trafienia w assert -
     niepotwierdzone z pewnością, ale spójne z hipotezą.

### v238 (2026-09-04) - FIX-MINLUX-LOCK-GAP: zdiagnozowane z realnego logu
     (aquarium_log.txt, klik 12:56:03) — po ustawieniu suwaka podczas
     przerwy rampa poprawnie dojeżdżała do celu w 30s (v236/v237 działały),
     ale zaraz potem LED gasły. Przyczyna: blokada z v237 w gałęzi "żadna
     rampa nie trwała" sprawdzała `minLuxModeActive` - a to bywa chwilowo
     `false`, gdy MIN LUX samo się właśnie wyłączyło (czujnik zgłosił
     "wystarczy światła"). Klik w tym oknie nie ustawiał blokady wcale.
     Chwilę później calculateMinLuxPWM() (czujnik odłączony -> niestabilny
     fallback) ponownie uznawał że trzeba dołożyć światła i bezwarunkowo
     reaktywował MIN LUX (minLuxModeActive=true, linia ~10431, bez
     sprawdzenia żadnej blokady) — nadpisując świeżo ustawioną wartość
     użytkownika z powrotem w dół.
     Fix: warunek ustawienia blokady zmieniony z `minLuxModeActive` na
     `minLuxModeEnabled && !inLightingWindowGlobal` - "MIN LUX w ogóle
     dotyczy tej przerwy", niezależnie od tego czy w danej sekundzie jest
     akurat aktywne czy chwilowo wyłączone przez siebie samo. Druga gałąź
     (przerwanie aktywnej rampy adaptacyjnej, bylaMinLux=rampaMinLuxPriorytet)
     bez zmian - tam sygnał już jest precyzyjny (rampa faktycznie trwała).

### v237 (2026-08-27) - FEATURE-MINLUX-MANUAL-LOCK: na życzenie — ręczna zmiana
     suwaka PODCZAS MIN LUX (przerwa południowa) ma się trzymać aż do końca
     przerwy, nie być nadpisana przez kolejny cykl przeliczenia MIN LUX
     (co ~30s, calculateMinLuxPWM() z czujnika, niezależnie od panelu).
     Nowa flaga minLuxManualOverrideUntilBreakEnd:
     - ustawiana w /set-pwm w OBU miejscach, gdzie suwak może dotknąć
       MIN LUX: (a) gałąź przerwania aktywnej rampy adaptacyjnej, tylko
       gdy to był konkretnie MIN LUX (bylaMinLux = rampaMinLuxPriorytet
       sprzed resetu, nie sama rampaAdaptacyjnaAktywna - ta ostatnia jest
       też używana przez inne, niezwiązane rampy); (b) gałąź "żadna rampa
       nie trwała" (v236 fix 2), gdy minLuxModeActive - to częstszy
       przypadek w praktyce, bo rampy MIN LUX trwają tylko kilka sekund,
       większość czasu MIN LUX "siedzi" na wartości między cyklami.
     - sprawdzana w applyMinLuxMode() jako wczesny return, obok już
       istniejących (softStartActive, rampScheduleActive) - MIN LUX
       pomija przeliczenie, ale minLuxModeActive zostaje true (koncepcyjnie
       nadal "rządzi" przerwą, tylko chwilowo nie nadpisuje PWM).
     - zwalniana automatycznie w trybAuto() dokładnie w momencie wykrycia
       końca przerwy (inLightingWindowGlobal false->true, tam gdzie flaga
       jest normalnie liczona z bieżącego czasu) - od tego momentu MIN LUX
       wraca do normalnego działania przy następnej przerwie, bez śladu
       starej blokady.

### v236 (2026-08-27) - FIX-RAMPA-SUWAK: dwie poprawki na suwak w trybie AUTO,
     diagnostyka Serial.printf z v235-DEBUG-RAMPA usunięta (posłużyła do
     namierzenia obu problemów, cel osiągnięty).
     (1) Rampa harmonogramu (OPADANIE/WZNOSZENIE) przerywana suwakiem:
         wartość suwaka teraz przechwytywana do pendingManualRampStart[]
         w /set-pwm, zanim kolejne wywołania trybAuto() zdążą nadpisać
         backupBrightnessComposite czymś innym. Wcześniej: użytkownik
         widział błysk na swojej wartości, po czym rampa i tak startowała
         z przypadkowej wartości sprzed nadpisania (patrz
         Ryby_LED_S3_plan_fix_suwak_rampa.md, zdiagnozowane z realnego
         zrzutu terminala: 51->177(błysk)->54, zamiast 51->177->rampa
         do 0 STARTUJĄC z 177).
     (2) Suwak gdy ŻADNA rampa nie trwa (gałąź "poza oknem świecenia /
         chroniony harmonogram"): ta gałąź nie miała w ogóle soft-startu —
         wartość szła prosto na sprzęt. Kod SAM to sobie diagnozował jako
         tag=DIAG-3 "SKOK BACKUP (bez rampy)" / tag=FLASH "skok hardware
         PWM wykryty" (potwierdzone w logu: seria natychmiastowych skoków
         76->237->424->80->655->104->0->223->0->43 podczas testowania
         suwaka). Dodany ten sam soft-start co w gałęzi rampy adaptacyjnej
         (30s, kapowany do limitu mocy FIX-FLASH3), bez ruszania istniejącej
         ochrony EEPROM/harmonogramu (v8/v9 FIX-PANEL-BREAK) - dołożony OBOK,
         nie zamiast.
     PRZY OKAZJI naprawione (znalezione podczas implementacji (2), ten sam
     mechanizm co (1)): gałąź rampy adaptacyjnej (MIN LUX) liczyła
     "currentVal" z backupBrightnessComposite PO tym jak już zostało
     nadpisane nową wartością suwaka (nadpisanie dzieje się w pętli na
     górze handlera, przed jakąkolwiek gałęzią) — start rampy wychodził
     równy celowi, czyli zero dystansu do przejechania, soft-start był
     bez efektu. Naprawione tym samym mechanizmem co (1) i (2): nowa
     tablica _preClickBBC[5] przechwytuje stan SPRZED zmiany na samej
     górze handlera /set-pwm, zanim cokolwiek go nadpisze.

v235-DEBUG-RAMPA (2026-08-27, wersja 2 — poprawiony zakres) - TYMCZASOWY
     BUILD DIAGNOSTYCZNY, NIE WERSJA DOCELOWA. Pierwsza wersja tego builda
     instrumentowała tylko gałąź `rampa harmonogramu` (bo taki przykład
     trafił w logu) — użytkownik sprostował: błysk widuje w trybie MIN LUX
     (`rampaAdaptacyjnaAktywna`+`rampaMinLuxPriorytet`), innej gałęzi tego
     samego handlera. Rozszerzone o wszystkie 4 punkty (szukaj znacznika
     [DEBUG-TEMP-RAMPA] w kodzie, usunąć po diagnozie):
     (1) SET-PWM-RAW — surowa wartość suwaka od razu przy wejściu, kanał 0,
         niezależnie od tego, która gałąź (harmonogram/adaptacyjna) potem
         zadziała.
     (2) SET-PWM (gałąź harmonogramu) — bbc0 zaraz po zapisie, ta sama co
         w wersji 1.
     (3) ADAPT-INIT (gałąź adaptacyjna/MIN LUX) — currentVal (z bbc0) i
         targetVal (z autoBbc0) PRZED kapowaniem mocy — sprawdzone w kodzie:
         oba fixy (FIX PANEL-AUTO sync + FIX-FLASH3 kap) już tu są, więc
         jeśli mimo to widać błysk, coś dalej w łańcuchu nie działa jak
         powinno — te dwie wartości to pokażą.
     (4) ADAPT-CAP — target PO kapowaniu mocy, dla porównania z (3).
     (5) trybAuto() na starcie, KAŻDE wywołanie — teraz też z
         rampaAdapt/minLuxPrio/softStart, nie tylko rampSchedActive.

### v235 (2026-08-23) - FIX-LOG-SEQ-ATOMIC: pierwszy log na v231+ pokazał
     drobne nie-monotoniczne cofnięcia w seq= (np. 181->175, różnice
     rzędu pojedynczych sztuk) — g_logSeq++ (volatile uint32_t) nie jest
     atomowe, a logToFile() woła się z obu rdzeni. Naprawione: std::atomic
     <uint32_t>, inkrement przez fetch_add() którego wynik (wartość SPRZED
     dodania +1) trafia bezpośrednio do zmiennej lokalnej _thisSeq — nie
     osobny odczyt g_logSeq po inkrementie, co eliminowałoby tylko część
     wyścigu (drugi rdzeń mógłby zdążyć zainkrementować między własnym
     fetch_add a własnym odczytem). Realnej utraty linii to nie dotyczyło
     (te dają duże skoki, nie ±kilka) — czysto kosmetyczna niedokładność
     w samym liczniku, teraz poprawiona.

### v234 (2026-08-22) - FIX-LOG-WATERLEAK-ONCHANGE: ostatni, dotąd błędnie
     uznawany za "zablokowany" punkt planu redukcji logów. Kod czujnika
     wody (GPIO6, analogRead) był cały czas w pliku (linia ~7527/17742) —
     przeoczony przy wcześniejszym sprawdzaniu wersji v230, mylnie uznany
     za nieznaleziony. Linia "odczyt" logowała się bezwarunkowo co 60s
     (potwierdzone: adc=0 state=0 zawsze w dotychczasowych logach — zero
     informacji w tej postaci). Zmiana: heartbeat "odczyt" co 1h zamiast
     60s. WAŻNE: częstotliwość SAMEGO SPRAWDZANIA (60s) i branże wykrycia/
     zaniku wody (WARN/INFO, logowane natychmiast przy zmianie) — bez
     zmian, to czujnik bezpieczeństwa i szybkość detekcji realnego
     przecieku nie może ucierpieć.
     Plan redukcji logów (Ryby_LED_S3_plan_redukcji_logow.md) — komplet.

### v233 (2026-08-22) - FIX-LOG-CB-COOLDOWN-SPAM: ostatni punkt z planu
     redukcji logów (rate-limit/dedup dla retry/cooldown). Usunięty
     log `tag=CB msg="Circuit open, skip FB+TG" cooldown=Xs`, który
     powtarzał się ~1x/s przez cały czas trwania cooldownu (do ~30
     niemal identycznych linii na incydent przy typowym 30s backoffie).
     Nie dedup/rate-limit, tylko usunięcie — informacja była w 100%
     redundantna: dokładny czas końca cooldownu (`cooldown_until=`)
     jest już logowany raz, w `NET_FAIL()` przy otwarciu obwodu.
     Zero zmian w samej pętli oczekiwania (WDT-safe, esp_task_wdt_reset()
     co 100ms) — usunięta wyłącznie linia logPrintf().
     Plan redukcji logów (Ryby_LED_S3_plan_redukcji_logow.md) — komplet
     punktów możliwych bez dodatkowych informacji: zrobione (v231-v233).
     Pozostaje tylko WATER-LEAK on-change, zablokowane brakiem lokalizacji
     jego kodu źródłowego.

### v232 (2026-08-22) - FIX-LOG-SCHED5-ONCHANGE: tag=SCHED5 (diagnostyka
     harm/curr z v72, cel: łapać skąd bierze się rozbieżność) logowany
     dotąd bezwarunkowo co 60s. Potwierdzone w wielu logach produkcyjnych:
     diff=0 i reszta stanu bez zmiany niemal zawsze — log bez informacji
     w tej postaci. UWAGA: on-change tylko po `diff` (jak pierwotnie
     planowane w Ryby_LED_S3_plan_redukcji_logow.md) zgubiłby kontekst
     (adapt/rampAdapt/trans/sSt/rampSch/inWin) PROWADZĄCY do rozbieżności,
     nie tylko jej moment — to sprzeczne z celem diagnostyki z v72. Zamiast
     tego: log przy zmianie KTÓREGOKOLWIEK z 8 pól (nie tylko diff), plus
     zapasowy heartbeat co 1h (pole hb=T/F pokazuje powód wpisu). Sprawdzanie
     stanu co 1s zamiast logowania co 60s — realna zmiana widoczna z
     opóźnieniem <1s zamiast do 60s, przy tym samym koszcie (7 lekkich pól).

### v231 (2026-08-22) - FIX-LOG-VOLUME + FIX-LOG-SEQ: z planu redukcji logów
     (Ryby_LED_S3_plan_redukcji_logow.md), tylko dwa z czterech punktów —
     pozostałe dwa świadomie ODRZUCONE po ponownym sprawdzeniu:
     (1) seq= — globalny licznik (g_logSeq) dopisywany do prefiksu KAŻDEJ
         linii w logToFile() (nie w logPrintf() — to zapis na flash jest
         tym, co może się urwać w połowie). Wykrywa utracone linie po
         restarcie/utracie zasilania w trakcie buforowania.
     (2) logPrintfNoFile() — kopia logPrintf() bez logToFile(), podmieniona
         na 3 (nie 4 — patrz niżej) rutynowych "OK": FB-CMD GET OK, FB-CMD
         DELETE OK, FB-CFG GET OK. Serial/WebSocket bez zmian, oszczędność
         tylko na flashu.
     ODRZUCONE po weryfikacji na tym pliku:
     - TG-POST "OK" NIE wyciszony — jego linia (path=...timeout=%s) jest
       WSPÓLNA dla sukcesu i błędu (timeout jako pole, nie osobna gałąź
       lvl=ERR jak w FB-CMD/FB-CFG). Wyciszenie ukryłoby też prawdziwe
       timeouty — dokładnie te dane, które posłużyły do rekonstrukcji
       incydentów sieciowych w poprzednich raportach.
     - Konsolidacja LED-CH (5 linii/kanał -> 1 linia) NIE zrobiona - już
       rozstrzygnięte w v210 (patrz CHANGELOG v210): to zmienna struktura
       per-kanał (nazwa+pwm+pct+gpio+opcjonalny "derating"), nie stały
       zestaw pól jak przy BACKUP-PWM. Świadomie odłożone tam, decyzja
       wciąż aktualna.
     - Konsolidacja HEAP+HEAP-b+HEAP-c (3 linie -> 1) NIE zrobiona —
       policzone na realnych wartościach z logu: połączona linia to
       ~547B, bufor logPrintf()/logToFile() to 384B. Silent truncation
       w vsnprintf() by ucinało dane, nie tylko skracało linię. Zgodne
       z powodem, dla którego HEAP-c w ogóle dostał osobną linię w v161
       (FIX-HEARTBEAT-TRUNC) — [HEAP]/[HEAP-b] już wtedy było "napięte".

### v230 (2026-08-22) - FIX-SHARED-STREAK: rozdzielenie _netFailStreak na
     _fbFailStreak/_tgFailStreak. Przyczyna: analiza aquarium_log__3_.txt,
     incydent 2026-08-19 00:58 — 2 kolejne TIMEOUT FB-CMD w 43s, ale circuit
     NIE otworzył się, bo udany TG-POST między nimi zerował wspólny
     _netFailStreak zanim streak dotarł do NET_FAIL_THRESHOLD=2. Jeden
     wspólny licznik dla Firebase i Telegrama maskował powtarzające się
     awarie pojedynczej usługi. Zgodnie z ugruntowaną praktyką circuit
     breaker pattern (osobny breaker per zależność) — rozdzielony sam
     licznik streak (8 miejsc: TG×2, FB-SEND×2, FB-CMD×2, FB-CFG×2).
     _netCircuitOpen/_netCooldownUntilMs/_netBackoffMs ZOSTAJĄ WSPÓLNE
     (celowo) — to ochrona budżetu iteracji/WDT (WARSTWA A+B, patrz v131),
     ryzyko globalne niezależne od tego, która usługa zawiodła. Zero zmian
     w pętlach oczekiwania (WDT-safety bez zmian), zero zmian w logice
     cooldown/half-open/ITER-BUDGET.

### v229 (2026-08-14) - FIX-RACE: audyt race conditions (patrz
     Ryby_LED_S3_analiza_WDT_race_conditions_2026-08-13.md +
     Ryby_LED_S3_polaczone_2026-08-13.md). Trzy zmiany, single-writer
     zamiast mutexów:
     (1) Rollover energii (energyTodayWh i cała rodzina) rozdzielony
         chirurgicznie od reszty logDailySummary(): logDailySummary()
         zostaje na Core 1 (loop()) i nadal zeruje statystyki.* (drugi
         writer tych pól - aktualizujUczenie()/uczSieTransmisji() - też
         jest na Core 1, więc to bezpieczne bez zmian). Sama mutacja
         energyTodayWh/ledOnMinutesToday/luxHoursTodayWater/itd. wydzielona
         do nowej applyEnergyRollover(), wołanej wyłącznie z tgTaskFn
         (Core 0) przez flagę energyRolloverPending - ten sam rdzeń co
         saveHistoryPoint() (histSavePending), który dolicza do tych
         samych zmiennych co ~5 min. "+=" i "=0" nigdy się już nie
         przeplatają między rdzeniami.
     (2) g_UNUSED_eepromCommitPending_DO_NOT_USE - dodano volatile (potwierdzony cross-core
         race Core 0/Core 1). TOCTOU między odczytem a zerowaniem w
         runDiagnostics() formalnie zostaje - pełne zamknięcie wymagałoby
         sekcji krytycznej we wszystkich ~15 miejscach ustawiających flagę,
         świadomie zostawione na docelowo.
     (3) historyClearPending - dodano volatile (3. kontekst wykonania:
         handler AsyncWebServer /api/history/clear).
     NIE dotyczy WDT_TASK/flash-freeze (v228, PSRAM-XIP) - to osobny,
     niezależny problem czystej współbieżności w RAM, bez związku
     z operacjami na flashu.

### v228 (2026-08-11) - HARDEN-NTP-DHCP-GUARD: kanarek kompilacyjny + log
     diagnostyczny dla fixu PANIC/CRASH w lwIP SNTP (assert "Required to
     lock TCPIP core functionality!" w `sntp_servermode_dhcp`) — pełny
     opis w PLAN_WDROZENIOWY_lwip_sntp_crash.md.
     KONTEKST: sam fix (`CONFIG_LWIP_DHCP_GET_NTP_SRV=n` w
     custom_sdkconfig, platformio.ini) wdrożono 2026-08-08 wyłącznie na
     poziomie configu builda — do tej pory w tym pliku .cpp nie było
     żadnego śladu tego faktu, więc kolejna osoba (albo ja za pół roku)
     czytająca changelog firmware nie miałaby jak się o nim dowiedzieć.
     RESEARCH (2026-08-11, PL/EN/DE/RU): dodatkowe wyszukiwanie
     potwierdziło, że wariant DHCP-wyzwalany (`sntp_servermode_dhcp`,
     sntp.c:770) to OSOBNE, udokumentowane zgłoszenie
     (esphome/issues#6591, grudzień 2024) — zamknięte jako "stale", BEZ
     żadnego fixu w kodzie. To co innego niż wariant "aplikacyjny"
     (`configTzTime()`/`sntp_setoperatingmode()`/`sntp_stop()` wołane
     wprost z kodu użytkownika), naprawiony przez PR #10529+#10725 w
     arduino-esp32 3.1.0. Niezależne potwierdzenie z niepowiązanego
     issue #10252 (Ethernet+NTP): komentarz w przykładowym kodzie
     ostrzega wprost, że włączenie `esp_sntp_servermode_dhcp(1)` tworzy
     pętlę awarii. WNIOSEK: sam upgrade frameworka (Krok 2 pierwotnego
     planu) NIE naprawiłby tego wariantu — `CONFIG_LWIP_DHCP_GET_NTP_SRV=n`
     to jedyny realnie dostępny fix, więc wart jest zabezpieczenia przed
     przypadkowym cofnięciem (merge, aktualizacja platformy, edycja
     platformio.ini bez pełnego czystego rebuildu).
     FIX: (1) kanarek kompilacyjny `#ifdef CONFIG_LWIP_DHCP_GET_NTP_SRV`
     tuż przed `configTzTime()` w setup() — jeśli flaga kiedykolwiek
     wróci do `y`, pojawi się `#warning` w logu builda PlatformIO;
     (2) log startowy `tag=NTP` potwierdzający aktualny stan flagi,
     trafiający do `aquarium_log.txt` — obserwowalny w terenie, bez
     potrzeby ręcznego sprawdzania sdkconfig na urządzeniu.
     Przy okazji potwierdzono (grep całego pliku): `configTzTime()` jest
     wołane dokładnie RAZ w całym programie (setup(), sekcja NTP) —
     wzorzec bezpieczny względem wariantu "aplikacyjnego" z #10675, więc
     żadna zmiana logiki synchronizacji czasu nie była potrzebna ani
     wprowadzona.
     Bilans `{}`/`()` sprawdzony względem pliku źródłowego — identyczna
     delta po obu stronach.
     DO ZWERYFIKOWANIA NA URZĄDZENIU: po flashu potwierdzić w logu
     startowym linię `tag=NTP msg="CONFIG_LWIP_DHCP_GET_NTP_SRV=wylaczone
     (fix aktywny)"` — jej brak albo odwrotna treść (`WLACZONE`) oznacza
     regresję configu i wymaga pełnego czystego rebuildu
     (`.pio/build/<env>` usunięte + build od zera).

### v227 (2026-08-02) - FIX-TLS-HANDSHAKE-TIMEOUT: znaleziono i naprawiono lukę
     strukturalną, którą wcześniej uznaliśmy za "ryzyko szczątkowe" bez
     dobrego rozwiązania — research znalazł rozwiązanie.
     KONTEKST: `log_combined__2_.txt` (15:28:15-15:28:35) pokazał crash WDT
     BEZ zwykłej sekwencji TIMEOUT→CircuitOpen — zamiast tego 14 sekund
     ciszy między `[TLS-DRAM]` (checkpoint tuż przed `tgClient.connect()`)
     a spodziewanym `[connect wynik]`, które nigdy się nie pojawiło. Komentarz
     z v108 zakładał, że `connect(ip,port,timeoutMs=5000)` ogranicza CAŁY
     handshake (TCP+TLS) — założenie fałszywe.
     RESEARCH: arduino-esp32 issues #481, #2896, #6077, #6528 — wielokrotnie
     potwierdzone, że TCP-level timeout w connect() NIE ogranicza fazy
     `mbedtls_ssl_handshake()`, która może wisieć bezterminowo przy
     nietypowym zachowaniu serwera/sieci w trakcie negocjacji TLS.
     WiFiClientSecure ma jednak DEDYKOWANĄ, osobną metodę na tę fazę:
     `setHandshakeTimeout(ms)` (potwierdzone w oficjalnej dokumentacji
     metod klasy). Ważny szczegół z issue #6077: `handshake_timeout`
     resetuje się do 0 (czyli bez limitu!) po KAŻDYM `stop()` — trzeba
     wywoływać przed każdą próbą connect(), nie raz przy starcie programu.
     FIX: `setHandshakeTimeout(4000)` dodane w 3 miejscach, każde tuż przed
     odpowiednim `connect()`: `tgEnsureConnected()`, `sendTelegramDocument()`
     (oba na `tgClient`), oraz `fbInitialize()` (na `fbSSLClient` — ten sam
     typ obiektu, FirebaseClient wywołuje jego connect() wewnętrznie, ale
     fbInitialize() to jedyne miejsce, gdzie faktycznie odtwarzamy
     połączenie po stop(), więc obejmuje wszystkie kolejne próby).
     Margines: TCP(do 5s) + TLS(do 4s) = do 9s w oknie WDT(10s) — 1s zapasu.
     Bilans `{}`/`()` sprawdzony względem pliku źródłowego — identyczna
     delta po obu stronach.
     DO ZWERYFIKOWANIA NA URZĄDZENIU: czy kolejny crash z sygnaturą
     "cisza po TLS-DRAM, brak connect wynik" jeszcze się zdarzy, czy
     `setHandshakeTimeout()` faktycznie przerywa zawieszony handshake.

### v226 (2026-08-01) - FIX-FB-IDLE-RECONNECT: przywrócono mechanizm wiekowego
     reconnectu dla `fbSSLClient`, usunięty w v105 na błędnym założeniu.
     KONTEKST: v105 usunął `fbClientLastUse`/`FB_IDLE_RECONNECT_MS` (wraz
     z całym starym `fbConnect()`) z uzasadnieniem "FirebaseClient z
     ESP_SSLClient prawidłowo wykrywa martwy socket" — eliminując
     rzekomo całą klasę problemów. Research (github.com/mobizt/
     FirebaseClient/discussions/99) obala to założenie: sam autor
     biblioteki FirebaseClient potwierdza, że to wciąż ten sam,
     znany bug w ESP32 WiFiClient (błędny `peek()`/`connected()` po
     zamknięciu połączenia przez serwer) — ESP_SSLClient dziedziczy po
     tej samej klasie, więc bug pozostaje. Jego oficjalna rekomendacja:
     wymuszać `.stop()` na połączeniu bezczynnym >2 minuty, zamiast
     polegać tylko na `.connected()`/keepalive. Pasuje to 1:1 do
     `[FB-BLOCK-DONE] total=9624ms` z log_combined__21_.txt — sygnatury
     niemal identycznej z pierwotnym bugiem z v104, który v105 miał
     rzekomo wyeliminować.
     FIX: przywrócono `fbClientLastUse`/`FB_IDLE_RECONNECT_MS=100000UL`
     (100s, konserwatywnie poniżej granicy "2 minuty" z rekomendacji
     mobizt) — dokładnie ten sam wzorzec, jaki od v32 ma `tgClient`
     (`tgClientLastUse`/`TG_IDLE_RECONNECT_MS=55000UL`) i nigdy stamtąd
     nie zniknął. WAŻNY SZCZEGÓŁ: `fbInitialize()` ma własny idempotentny
     early-return `if (fbAppReady && fbApp.ready()) return;` — samo
     dopisanie warunku idle przy wywołaniu `fbInitialize()` byłoby
     no-opem, bo `fbAppReady` wciąż =true (gniazdo nie jest jeszcze
     martwe, tylko stare). Dlatego przy przekroczeniu progu jawnie
     wymuszono `fbSSLClient.stop()` + `fbAppReady=false` PRZED
     wywołaniem `fbInitialize()`, dokładnie jak robi to `tgClient.stop()`
     w `tgEnsureConnected()`.
     ZMIANY (3 miejsca — sendStatusToFirebase, checkFirebaseCommands,
     checkFirebaseConfig): warunek idle przed bramą reinit + aktualizacja
     `fbClientLastUse` przy każdym potwierdzonym sukcesie (SET, GET, DEL).
     Nowy log `[FB-IDLE-RECONNECT] site=... idleMs=...` przy wyzwoleniu.
     Bilans `{}`/`()` sprawdzony względem pliku źródłowego — identyczna
     delta po obu stronach.
     DO ZWERYFIKOWANIA NA URZĄDZENIU: czy `[FB-BLOCK-DONE] total=`
     przestanie kiedykolwiek zbliżać się do 10s po dłuższych okresach
     bezczynności FB (np. gdy TG/inne operacje dominują iterację).

### v225 (2026-08-01) - FIX-FW-VERSION-DRIFT (znowu, trzeci raz): plik
     dostarczony przez użytkownika miał już kod v223/v224
     (DIAG-FSGUARD-DRIFT - g_fsGuardTriggerUsed, porównanie bajtów
     log_a+log_b w momencie wyzwolenia fsSizeGuard vs faktycznego
     przetwarzania w tgTaskFn), ale `#define FW_VERSION` wciąż mówił
     "v222" - ten sam dryf co v156 i v167 (2 poprzednie "ostateczne"
     naprawy). Tym razem tylko podbito makro do "v225 (2026-08-01)",
     zgodnie z nazwą pliku dostarczonego przez użytkownika - bez zmian
     funkcjonalnych. Fix v222 (WWW/OTA never-started) potwierdzony obecny
     i nienaruszony w tej wersji.

### v222 (2026-07-31) - FIX-WWW-NEVER-STARTED + FIX-OTA-NEVER-STARTED: zgłoszenie
     użytkownika "nie ładuje mi się strona WWW, ale przez Telegram mogę
     logi ściągnąć" - potwierdzone w `aquarium_log__2_.txt`: restart
     WDT_TASK o 22:19:40, po restarcie OBIE skonfigurowane sieci WiFi nie
     połączyły się na czas w setup() (7.5s+timeout), log wprost:
     `tag=SETUP msg="Brak WiFi w setup() po 2 probach, webserialServer i
     ArduinoOTA NIE wystartowaly w tym boocie (wystartuja po nastepnym
     restarcie z dostepnym WiFi)"`. WiFi później samo się połączyło w tle
     (dlatego Firebase/Telegram działały normalnie), ale serwer WWW i OTA
     NIE wystartowały mimo to - i nie wystartowały już NIGDY do
     ręcznego restartu, wbrew temu, co sugerował komunikat w logu.
     ROOT CAUSE: istniejący mechanizm odzyskiwania (FIX-v52-WWW-RECONNECT,
     WiFi monitor w loop(), ~linia 17330) rebinduje webserialServer TYLKO
     gdy `_wifiLostMs != 0` - czyli tylko dla scenariusza "WiFi było,
     zniknęło, wróciło". Gdy WiFi nie połączyło się WCALE w setup(),
     gałąź ustawiająca `_wifiLostMs` (przy `WiFi.status() != WL_CONNECTED`)
     nigdy nie wykonała się w stanie "był rozłączony" - w praktyce zależy
     to od tego, czy WiFi zdążyło się połączyć w tle PRZED pierwszym
     przebiegiem tego bloku (co 60s). Jeśli tak - `_wifiLostMs` zostaje 0
     na zawsze, `if (_wifiLostMs != 0)` jest fałszywy, serwer nigdy nie
     startuje. ArduinoOTA miał dokładnie tę samą lukę (log wspominał oba
     na raz), tylko nikt jej wcześniej nie zauważył, bo objaw (brak
     możliwości wgrania firmware przez OTA) ujawnia się dopiero przy
     PRÓBIE wgrania, nie natychmiast jak brak strony WWW.
     FIX: rozszerzono warunek na `if (_wifiLostMs != 0 || !wsReady)` -
     `wsReady` jednoznacznie oznacza "serwer faktycznie wystartował",
     niezależnie od tego, jaką drogą tu trafiliśmy. Osobny log dla nowej
     gałęzi (`msg="Polaczono pierwszy raz po nieudanym boocie"`), żeby nie
     mylić z prawdziwym `przerwa=Xs` liczonym od realnego `_wifiLostMs`.
     Dla ArduinoOTA: wydzielono cały blok setup (setHostname/setPassword/
     4 callbacki/begin(), ~40 linii) z `setup()` do osobnej funkcji
     `startOtaIfNeeded()` (nowa, ~przed `setup()`) + flaga `otaReady` -
     wywoływana z dwóch miejsc (setup() przy udanym połączeniu, WiFi
     monitor gdy połączenie przyszło później) zamiast kopiować 40 linii
     callbacków w dwóch miejscach, które trzeba by pamiętać synchronizować.
     Weryfikacja: bilans `{}`/`()`  sprawdzony względem pliku źródłowego
     przed edycją - identyczna delta po obu stronach (edycja domknięta).
     DO ZWERYFIKOWANIA NA URZĄDZENIU: przy kolejnym boocie bez WiFi w
     setup() (rzadki przypadek, wymaga awarii obu sieci naraz w oknie
     restartu) - czy strona WWW i OTA wystartują same, gdy WiFi wróci,
     bez potrzeby ręcznego restartu.

### v218 (2026-07-30) - LOGS-AI-STEP50: rozstrzygnięcie 3 pozostałych otwartych
     decyzji z briefingu, priorytet = czytelność dla AI (decyzja właściciela
     projektu, research: logfmt/canonical log line, brandur.org, Stripe eng
     blog - log ma nieść kontekst zdarzenia per linia, nie statyczną
     dokumentację ani wizualny pasek postępu).

     (c) Baner startowy setup() - USUNIĘTO 41 logPrintln z archiwalną
     historią wersji (v97-v170, ~linie 13398-13439 sprzed tej partii).
     Czysto archiwalna lista bez wartości zdarzeniowej - historia i tak
     żyje w tym CHANGELOGU (źródło prawdy), powielanie przy KAŻDYM boocie
     to czysty szum bez informacji o konkretnym uruchomieniu. Spłaszczone
     do jednej linii: `lvl=INFO tag=BOOT akcja=start version=%s wdt=10s`
     (dawne "Watchdog Timer: 10 sekund" złączone jako pole wdt=).

     Klaster "Łączenie z WiFi" (pytanie 5) - USUNIĘTO kropki postępu
     (2x pętla retry, do 15 kropek/próbę = do 30 linii szumu na boot),
     usunięto 2x puste logPrint("") (nie potrzebne bez paska kropek).
     `logPrint("Łączenie z WiFi")` -> `tag=WIFI-CONNECT akcja=start`
     (nowy tag, para z istniejącym `tag=BOOT msg="Polaczono z WiFi"` /
     `msg="WiFi timeout..."` - wynik już był logfmt od wcześniej).
     Usunięto wiodące "\n" z "1. proba WiFi nieudana" (było tylko po to,
     by zamknąć linię kropek, teraz zbędne).

     4 miejsca Serial.printf (pytanie 2, 8 wywołań łącznie: LOG-GUARD x2,
     WS x2, LOG-CLEAR x2, WDT-FIX x2) - przekonwertowano FORMAT stringów
     na logfmt, ale świadomie ZACHOWANO mechanizm - dalej bezpośredni
     Serial.printf, NIE logPrintf/logToFile(). To zachowuje zabezpieczenie
     przed rekurencją w logToFile() (precedens v195) przy jednoczesnym
     spełnieniu celu AI-czytelności (te linie i tak trafiają tylko na
     Serial, nigdy do pliku logu). Nowe tagi: `tag=LOG-GUARD site=`
     (logToFile|logFlushCore1, spójne z istniejącym `tag=FLASH-STALL
     site=logFlushCore1`), `tag=WS akcja=connect|disconnect`,
     `tag=LOG-CLEAR akcja=ok|fail`, `tag=WDT-FIX`.

     Walidacja: liczba linii pliku 20338->20295 (-43: -41 baner, -2 puste
     logPrint dots-guard, reszta edycje in-place). Bilans `{}`: 1847/1860
     (delta vs poprzednia wersja wynika wyłącznie z usuniętych linii
     bez klamr, zero nowych/usuniętych klamr strukturalnych - sprawdzone
     ręcznie, każda usunięta linia to czysty logPrintln(...) bez {}).
     Zero pozostałości starego formatu w kodzie poza tym wpisem CHANGELOG
     i historycznymi wzmiankami w CHANGELOG v98 (sprawdzone grepem).
     Nowe stringi w 100% ASCII (sprawdzone grepem non-ASCII).

     Stan projektu po tej partii: WSZYSTKIE świadome wyjątki i otwarte
     pytania z briefingu (1-5 + kategoria (c)) rozstrzygnięte. Pozostają
     tylko: pytanie 1 (tag=WIFI-STATUS vs tag=WiFi - scalić?), pytanie 3
     (prefiks [WARN]/[OK]/[ERR] vs pole lvl= w 100%), pytanie 4 (konwencja
     dla dekoracyjnego [%d]) - te trzy NIE zostały poruszone w tej sesji,
     wymagają osobnej decyzji. Poza nimi: projekt konwersji logfmt można
     uznać za funkcjonalnie ukończony.

### v221 (2026-07-30) - LOGS-AI-STEP53: naprawa driftu FW_VERSION - był
     zahardkodowany na "v171" (ostatni ręczny bump), mimo że CHANGELOG
     doszedł już do v220 - dokładnie ten sam typ driftu, który był raz
     "naprawiony" w v169 (FIX-FW-VERSION-DRIFT) i od tamtej pory ponownie
     się rozjechał, bo nikt nie podbija makra przy każdej partii. Podbite
     do "v220 (2026-07-30)" (~linia 6423), zgodnie z bieżącym stanem
     CHANGELOGU. Wpływa na: baner bootowy (`tag=BOOT ... version=%s`),
     panel WWW (Dashboard), `/api/status` (pole `fwVersion`) - wszystkie
     trzy czytają to samo makro, więc jedna zmiana synchronizuje wszystkie
     trzy miejsca naraz.

     UWAGA na przyszłość: FW_VERSION i numer CHANGELOG-u to formalnie
     dwa niezależne liczniki (jeden śledzi realne release'y firmware,
     drugi - partie konwersji logów) - nie ma automatycznego mechanizmu
     wiążącego je na stałe. Przy każdej kolejnej partii, która ma trafić
     na fizycznie wgrywane urządzenie, sprawdź czy FW_VERSION nie
     zostało w tyle względem CHANGELOGU (tak jak tutaj) i podbij ręcznie
     jeśli tak.

     Walidacja: zmiana jednego literału stringa, zero zmian strukturalnych
     (bilans `{}`/`()` bez zmian poza tekstem tego wpisu), liczba linii
     pliku +18 (sam wpis CHANGELOG + 1 zmieniona linia bez zmiany liczby
     linii).

### v220 (2026-07-30) - LOGS-AI-STEP52: pytanie 6 (ASCII cleanup) - ZAMKNIĘTE,
     transliteracja polskich znaków diakrytycznych i symboli spoza ASCII w
     55 wywołaniach log* (63 fizyczne linie) wykrytych świeżym rekonesansem
     po v219. Wyłącznie podmiana znaków w wolnym tekście `msg=`/opisach/
     wartościach - zero zmian w strukturze tag=/lvl=/key=value, zero zmian
     w nazwach tagów, zero zmian w %-argumentach formatu.

     Mapowanie: ą->a ć->c ę->e ł->l ń->n ó->o ś->s ź->z ż->z (+ wielkie
     odpowiedniki), ° usunięte (zawsze poprzedzało literał "C" - "%.1f°C"
     -> "%.1fC", spójne z jednostką stopni Celsjusza bez ozdobnika), Δ ->
     "delta" (słowo - ujednolica z już istniejącym polem `delta=` używanym
     w tag=MEM-FREE/TLS-DRAM/HEAP-MIN/SEKCJA5-DBG - wcześniej niespójne:
     inline Δ w DIAG-3/5/7 vs pole delta= gdzie indziej, teraz jednolite),
     ² -> "2" (I²C -> I2C, jedyne wystąpienie w log*, ~19398/19640),
     Ω -> "Ohm" (4.7kΩ -> 4.7kOhm, ~13598/18703), × -> "x" (%ds×5 ->
     %dsx5, ~19665).

     Kategorie dotknięte: bloki DIAG-3..DIAG-31 (30 linii, wolny tekst
     sugestii/opisu), DS-DIAG/DS-ADDR (7), DS18B20 throttle (3), wartości
     `WŁ`/`WYŁ` przekazywane jako %s w POMPA/PANEL-POWER/MIN-LUX/DIR/PANEL/
     ZAPIS (6 - stały się `WL`/`WYL`), EEPROM/FB-INIT (3), pozostałe
     pojedyncze (TRYBAUT itp. już miały delta= od wcześniej, nie dotknięte
     tą partią bo już były ASCII).

     Walidacja: zero non-ASCII w jakimkolwiek wywołaniu logPrintf/
     logPrintln/logPrint w całym pliku (sprawdzone rekonesansem po
     string-literałach, z wykluczeniem komentarzy `//` w argumentach
     wieloliniowych i bloku CHANGELOG - to nie są runtime stringi).
     Liczba linii pliku bez zmian (20393 przed/po - czysta podmiana
     znaków w istniejących liniach, zero dodanych/usuniętych linii poza
     tym wpisem CHANGELOG). Bilans `{}` 1850/1863 bez zmian, bilans `()`
     12989/13011 bez zmian (podmiana znaków nie dotyka nawiasów). Liczba
     cudzysłowów bez zmian względem stanu przed tą partią (podmiana
     znaków wewnątrz stringów, nie dodawanie/usuwanie cudzysłowów).

     STAN PROJEKTU PO v220: struktura logfmt kompletna I w 100% ASCII w
     runtime stringach log*. Trwałe, świadome wyjątki (NIE do ruszania
     bez nowej decyzji człowieka): tgPost (kanał Telegram, narracyjny,
     zawiera emoji/diakrytyki celowo), 4x Serial.printf LOG-GUARD/WS/
     LOG-CLEAR/WDT-FIX (mechanizm zachowany, ryzyko rekurencji w
     logToFile()), tag=WiFi vs tag=WIFI-STATUS (osobne, różne schematy
     pól). Projekt konwersji logfmt jest funkcjonalnie i formalnie
     ukończony - brak otwartych pytań.

### v219 (2026-07-30) - LOGS-AI-STEP51: zamknięcie pytań 1/3/4 z briefingu -
     BEZ ZMIAN W KODZIE, wyłącznie potwierdzenie/dokumentacja stanu
     faktycznego po świeżym rekonesansie (grep całego pliku).

     Pytanie 3 (lvl= vs [WARN]/[OK]/[ERR]) - ZAMKNIĘTE. Rekonesans:
     zero wywołań logPrintf/logPrintln/logPrint w pliku ma dziś prefiks
     [WARN]/[OK]/[ERR] (jedyne trafienia grepa na te ciągi to zwykłe
     komentarze kodu typu "// [OK] FIX-v33...", niezwiązane z logami).
     Migracja do pola lvl= jest w 100% ukończona już od wcześniejszych
     partii. Decyzja: lvl= jako jedyny obowiązujący standard, formalnie
     potwierdzone, nic do zrobienia.

     Pytanie 4 (dekoracyjne [%d]) - ZAMKNIĘTE. Rekonesans: 3 miejsca
     wskazane w briefingu jako oczekujące (harmonogram POMPA ~9492,
     lista kanałów LED ~10346) dostały już nazwane pole zamiast gołego
     indeksu w toku wcześniejszych partii: `tag=POMPA-SLOT slot=%d ...`,
     `tag=LED-CH kanal=%d ...`. Zero pozostałości wzorca `\[%d\]` w
     jakimkolwiek wywołaniu logPrintf w całym pliku (sprawdzone grepem
     po wywołaniach wieloliniowych). Decyzja: przyjęty wzorzec to
     "nazwane pole zamiast gołego indeksu" (slot=, kanal=) - spójny z
     resztą logfmt, konwencja idx= okazała się zbędna. Nic do zrobienia.

     Pytanie 1 (tag=WiFi vs tag=WIFI-STATUS) - ZAMKNIĘTE, decyzja:
     zostają rozdzielone. Rekonesans schematów pól: `tag=WiFi` (5x,
     linie ~7821/14002/17167/17174/17200) zawsze niesie `msg="..."`
     (próba połączenia / brak połączenia / hard-reconnect / ponowne
     połączenie) - narracja zdarzenia. `tag=WIFI-STATUS` (1x, ~12376)
     niesie `od=%d do=%d ip=%s freeH=%luB` - czysta zmiana stanu
     WiFi.status(), bez msg=. Dwa niekompatybilne schematy pól, ten sam
     typ rozróżnienia co już przyjęty precedens tag=MANUAL vs
     tag=MANUAL-SYNC vs tag=MANUAL-BACKUP (różne mechanizmy = osobne
     tagi, nie warianty akcja= jednego zdarzenia jak RAMPA/KAMERA).
     Scalenie wymusiłoby sztuczne ujednolicanie pól bez zysku.

     Walidacja: brak zmian w kodzie wykonawczym w tej partii (wyłącznie
     wpis CHANGELOG) - liczba linii pliku bez zmian poza tym wpisem,
     bilans `{}` bez zmian, zero nowych stringów runtime.

     STAN PROJEKTU PO v219: wszystkie otwarte pytania i świadome
     wyjątki z briefingu rozstrzygnięte. Trwałe, świadome wyjątki
     (decyzja człowieka, NIE do ruszania bez nowej decyzji): tgPost
     (kanał Telegram, narracyjny), 4x Serial.printf LOG-GUARD/WS/
     LOG-CLEAR/WDT-FIX (mechanizm zachowany ze względu na ryzyko
     rekurencji w logToFile(), format już logfmt), tag=WiFi vs
     tag=WIFI-STATUS (osobne, świadomie). Projekt konwersji logfmt
     jest funkcjonalnie ukończony.

### v217 (2026-07-30) - LOGS-AI-STEP49: konwersja 5 z 11 pozycji kategorii
     (d) "rozproszone" z briefingu (stan po v216) - te, które nie
     wymagały decyzji człowieka. 5 wywołań -> 5 nowych.

     Terminal: URL (~15322, po webserialServer.begin()): nowy
     `tag=BOOT msg="Terminal dostepny" url=`. Reużyty `tag=BOOT`
     (ten sam etap - faza uruchamiania serwerów po połączeniu WiFi,
     precedens 3 sąsiednich logów BOOT wyżej w setup()).

     MIĘKKI START MANUALNY zakończony (~18924, koniec pętli
     manualSoftStartActive w trybManual()): reużyty `tag=MANUAL`
     (ten sam tag loguje start tej samej sekwencji w onPowerChange(),
     ~19033, msg="miekki start po wlaczeniu zasilania") - teraz
     `msg="miekki start zakonczony"`, para start/koniec rozpoznawalna
     po treści `msg=`. Celowo NIE dodano pola `akcja=start|stop` do
     obu linii - wymagałoby to edycji już wcześniej skonwertowanej
     linii startowej z v212-erze, poza zakresem tej partii; zostawione
     jako możliwe drobne dopracowanie na przyszłość.

     TRYB MANUALNY: Zachowywane indywidualne wartości jasności.
     (~18985, ta sama funkcja trybManual(), gałąź hasChanged): nowy
     `tag=MANUAL-SYNC`, `msg="zachowywanie indywidualnych wartosci
     jasnosci"`. Celowo odrębny od `tag=MANUAL` (inny mechanizm -
     to log okresowy przy każdej zmianie kompozytu w pętli głównej,
     nie miękki start) i od `tag=MANUAL-BACKUP` (ten dotyczy backupu
     przy przełączeniu trybu AUTO<->MANUAL, nie bieżącej pętli).

     Kamera WŁĄCZONA/WYŁĄCZONA (~18998/~19006, wlaczKamera()/
     wylaczKamera()): scalone pod istniejący `tag=KAMERA` (już używany
     w loop() dla zdarzenia auto-wyłączenia po 30 min, ~16992) z nowym
     polem `akcja=start|stop` (precedens RAMPA/WIFI-GUARD - warianty
     tego samego zdarzenia pod jednym tagiem zamiast osobnych opisów).
     Uwaga: przy auto-wyłączeniu (30 min) powstaną teraz 2 linie -
     istniejąca `tag=KAMERA msg="Auto-wylaczenie po 30 minut"` (powód)
     + nowa `tag=KAMERA akcja=stop` (sama zmiana stanu) - zamierzone,
     ten sam wzorzec co WYWOLANIE+ZAPIS gdzie indziej w pliku (powód
     i skutek jako osobne linie tego samego zdarzenia).

     Walidacja: bilans `{}` bez zmian (1846/1859 - czyste podmiany
     stringów, zero nowych klamr). Liczba linii pliku bez zmian
     (20266 - edycje w miejscu). Delta cudzysłowów +8 (parzysta,
     zgodna z ręcznym wyliczeniem per-linia: +2/+2/+2/+2/+0). Zero
     pozostałości starego brzmienia ("Terminal: \"", "MIĘKKI START
     MANUALNY zakończony", "TRYB MANUALNY: Zachowywane", "Kamera:
     WŁĄCZONA", "Kamera: WYŁĄCZONA") w pliku poza komentarzami
     CHANGELOG (sprawdzone grepem). Nowe stringi w 100% ASCII
     (sprawdzone grepem non-ASCII na zmienionych liniach).

     NIE ruszone w tej partii (pozostałe z (d), wymagają decyzji
     człowieka - patrz otwarte pytania w briefingu): klaster
     "Łączenie z WiFi" + 4x kropka postępu + 2x pusty logPrint
     (~13831-13881, boot) - to pasek postępu do odczytu na Serialu
     na żywo, nie dyskretne zdarzenie logfmt; zestawione z tym samym
     pytaniem co kategoria (c) - konwertować mimo utraty sensu
     "paska", czy zostawić trwale jako wyjątek jak Serial.printf.
     `logPrintln(_cdBt)` (~13716, coredump backtrace) - sprawdzone:
     to już poprawny logfmt wewnątrz zmiennej (`lvl=ERR tag=COREDUMP
     msg="backtrace" adresy="..."`), grep flagował tylko dlatego że
     to zmienna a nie literał - nic do zrobienia, wpis można uznać za
     zamknięty.

     Stan "co zostało": (c) baner startowy setup() (~13318-13368,
     44 logPrintln z historią wersji) - nadal czeka na decyzję
     człowieka czy w ogóle konwertować (czysto archiwalna lista, nie
     zdarzenie runtime). Klaster "Łączenie z WiFi" j.w. Otwarte
     pytania 1-4 z briefingu - bez zmian, nadal nierozstrzygnięte.
     Poza (c) i klastrem WiFi-dots: kategoria (d) wyczerpana - świeży
     grep po tej partii (logPrintf/logPrintln/logPrint bez `tag=`,
     wykluczając tgPost, deklaracje funkcji i komentarze) zwraca tylko
     pozycje z (c) + klaster WiFi-dots + _cdBt (fałszywe trafienie,
     patrz wyżej).

### v216 (2026-07-30) - LOGS-AI-STEP48: konwersja partii (b) z listy "co
     zostało" w v215 - UCZENIE/Transmisja/KOREKTA/ADAPT/RAMPA ADAPTACYJNA
     (~10612-11984). 12 wywołań, 11 przekonwertowanych + 1 usunięty jako
     martwy duplikat.

     Zapamiętano MANUAL (~10612): nowy `tag=MANUAL-BACKUP` (odrębny od
     istniejącego `tag=MANUAL` - to inne zdarzenie: backup jasności przy
     przełączeniu trybu, nie "miękki start po włączeniu zasilania").

     UCZENIE: próbka (~11014): nowy `tag=UCZENIE`, pole `probek=`.

     Transmisja (~11038): nowy `tag=TRANSMISJA-UCZENIE`, pola
     `zmierzona=`/`uczona=` (bez znaku `%` w wartościach, zgodnie z
     konwencją key=value).

     KOREKTA MIN/MAX (~11102/~11117): scalone pod jednym nowym tagiem
     `tag=ADAPT-KOREKTA akcja=min|max` (precedens RAMPA/WIFI-GUARD -
     warianty tego samego zdarzenia pod jednym tagiem). `lvl=WARN`
     (zdarzenie graniczne - system musiał skorygować wynik adaptacji).
     Usunięty zbędny manualny prefiks `logTime().c_str()` w obu
     (precedens FIX-v39-3 - logToFile() i tak dopisuje `[HH:MM:SS ms=]`).

     ADAPT: wynik < MIN LUX (~11142): nowy `tag=ADAPT-CLAMP`, `lvl=WARN`.
     Celowo odrębny od ADAPT-KOREKTA - inny mechanizm (clamp do
     `minLuxCurrentPWM`, nie do `minLuxDlaRoslin`). Usunięty `logTime()`.

     Główna linia ADAPT (~11176, podsumowanie obliczAdaptacyjnaJasnosc):
     nowy `tag=ADAPT`, `lvl=INFO`. Celowo odrębny od `tag=ADAPT-RESULT`
     (ten drugi zostaje wyłącznie dla gałęzi tryb=0-brak-czujnika, inna
     struktura pól). Usunięty `logTime()`.

     RAMPA ADAPTACYJNA %d->%d (~11264): USUNIĘTY jako martwy duplikat -
     dublował dokładnie te same wartości (kanał 0, od=/do=) co
     `tag=RAMPA akcja=start` zalogowany kilkanaście linii wyżej w tej
     samej funkcji (zastosujRampeAdaptacyjna), po prostu odczytane z
     innych zmiennych (rampaAdaptacyjnaAktualne[0]/Cel[0] zamiast
     backupBrightnessComposite/nowiePWM[0], ale te same liczby w
     momencie wywołania). Zero utraty informacji.

     Adaptacja zapisana / Brak danych adaptacji (~11376/~11386):
     reużyty `tag=EEPROM` (już używany w tej samej funkcjonalnej
     rodzinie - odczyt/zapis adaptacji z EEPROM). "pierwszy start" to
     normalny stan przy pierwszym uruchomieniu, nie anomalia - `lvl=INFO`.

     Adaptacja odczytana + Próbek (~11408-11410): spłaszczone z 2 wywołań
     w 1 canonical log line (`tag=EEPROM msg="Adaptacja odczytana"
     probek=%u`) - to jedno zdarzenie (odczyt adaptacji), nie dwa.

     Usunięto adapt_stats.json (~11984, komenda "reset-adapt"): nowy
     `tag=RESET-ADAPT`. Nie reużyto `tag=EEPROM` - to usunięcie pliku
     LittleFS, nie operacja EEPROM, mimo że w tej samej komendzie.

     Walidacja: bilans `{}` 1844/1857 (oba -1 względem v215 - spójne,
     bo usunięty duplikat miał własny blok if(Komentarze){...}). Liczba
     cudzysłowów 8022 (+2 względem v215 - zgodne z ręcznym wyliczeniem
     per-edycja: -2 usunięty duplikat, +2 EEPROM-zapisana, +2 EEPROM-brak,
     -2 scalenie EEPROM-odczytana/Próbek, +2 RESET-ADAPT). Liczba linii
     20196 (-7 względem v215: -1 usunięcie logTime() z głównej linii
     ADAPT, -5 usunięty duplikat RAMPA ADAPTACYJNA, -1 scalenie
     EEPROM-odczytana/Próbek do jednej linii) - oba wyliczenia zgodne co
     do linii.

     Stan "co zostało": (c) baner startowy setup() (~13226+, ~44
     logPrintln z historią wersji) - nadal czeka na decyzję człowieka;
     (d) rozproszone pozycje z briefingu (coredump backtrace, "Łączenie
     z WiFi" x5, Terminal:, MIĘKKI START MANUALNY, TRYB MANUALNY,
     Kamera WŁ/WYŁ) - bez zmian, ~11 wywołań. Bez (c): zostaje ~11
     wywołań w (d) + ewentualne resztki potwierdzone świeżym grepem.
     Otwarte pytania 1-4 z briefingu - bez zmian, nadal nierozstrzygnięte.

### v215 (2026-07-30) - LOGS-AI-STEP47: konwersja 1 miejsca zidentyfikowanego
     w v214 jako kolejne do zrobienia - detaliczna linia w diagHeap()
     (~19547, blok DIAG-17-RESTART, wykonywana tuż po decyzji restartowej
     obu scenariuszy A/B) miała już logfmt key=value, ale brakowało
     `lvl=`/`tag=` (ten sam przypadek co SUMMARY w v214).

     Dopisany prefiks `lvl=ERR tag=DIAG-17-RESTART ` przed istniejącą
     treścią (`free=... maxAlloc=... frag=... crit=... uptime=...`).
     Reużyty `tag=DIAG-17-RESTART` - ta sama nazwa co dwa sąsiadujące
     wywołania w tej samej funkcji/tym samym bloku `if (restartNow)`
     (scenariusz A "restart ratunkowy" / scenariusz B "restart nocny"),
     więc to trzecia linia tego samego zdarzenia restartu, nie nowy tag.
     `lvl=ERR` spójne z sąsiadującymi wywołaniami (restart = zawsze
     poważna sytuacja, nie rutyna). Reszta pól (5) bez zmian, bez
     przestawiania kolejności ani paddingu.

     Celowo NIE scalono tej linii w jedno wywołanie z poprzedzającym ją
     logPrintf scenariusza A/B - wymagałoby to przepisania dwóch
     niezależnych gałęzi if/else if na wspólną ścieżkę, większe ryzyko
     niż jednoliniowa poprawka prefiksu; zostawione jako osobna decyzja
     na przyszłość, jeśli w ogóle uznana za wartą zrobienia.

     Walidacja: bilans `{}` bez zmian (1844/1857), liczba cudzysłowów bez
     zmian (8012 - tekst wstawiony do wnętrza istniejącego stringa, zero
     nowych `"`), liczba linii pliku bez zmian (20165 - edycja w miejscu,
     bez dodawania/usuwania linii). Świeży grep potwierdza: zero
     pozostałości starego brzmienia (`"free=%luB maxAlloc=%luB frag=`
     bez `tag=` przed nim) w pliku poza tym wpisem CHANGELOG.

     Stan "co zostało" bez zmian względem v213/v214 (patrz te wpisy):
     (b) UCZENIE/Transmisja/KOREKTA/ADAPT/RAMPA ADAPTACYJNA (~11338,
     ~11348, ~11946 i sąsiadujące - potwierdzone obecne w świeżym grepie
     wykonanym przy okazji tej partii); (c) baner startowy setup()
     (~13226+, ~50 logPrintln z historią wersji) - nadal osobna kategoria
     do decyzji; (d) rozproszone pozycje z v213 bez zmian. Następna
     partia: wybrać (b) jako najmniejszy spójny blok, albo poczekać na
     decyzję człowieka co do (c) i otwartych pytań 1-4 z briefingu.

### v214 (2026-07-30) - LOGS-AI-STEP46: konwersja partii (a) z listy "co
     zostało" w v213 - SUMMARY/Energia/harmonogram-etykieta (~9011,
     ~9038, ~9274-9284) - 3 miejsca -> 3 nowe/poprawione wywołania.

     SUMMARY (~9011, funkcja raportu dziennego): logfmt już był gęsty
     (`SUMMARY date=... key=value...`), ale brakowało pola `lvl=`/`tag=`
     - dopisany prefiks `lvl=INFO tag=SUMMARY ` przed istniejącą treścią,
     reszta bez zmian (19 pól bez zmian, walidowane liczbą argumentów).

     Energia dziś (~9038): stary format "Energia dziś: %.1f Wh | Tydzień:
     ... | Miesiąc: ..." -> `lvl=INFO tag=ENERGIA msg="raport dzienny"
     dzisWh= tydzienWh= miesiacWh=`. Reużyty `tag=ENERGIA` (precedens w
     tej samej funkcji: "Nowy tydzien"/"Nowy miesiac" reset licznikow).

     Harmonogram-etykieta (~9220-9284, funkcja loadScheduleFromEEPROM):
     banner `logPrintln("=== ODCZYT HARMONOGRAMU Z EEPROM ===")` + 5x
     wywołanie helpera `printTimeHM(label, minuty)` (jedna linia na pole)
     spłaszczone do jednej canonical log line pod reużytym
     `tag=HARMONOGRAM` (precedens: ostrzeżenia MIDDAY/MORNING wyżej w tej
     samej funkcji), pola `ranoRobocze=hh:mm ranoWeekend=hh:mm
     poludnieWyl=hh:mm wieczorPrzed=hh:mm wieczorWyl=hh:mm`. Pole
     "wieczorPrzed" liczone tym samym wzorem h=min/60 m=min%60 co
     pozostałe - zachowuje identyczne zachowanie liczbowe dla ujemnej
     wartości EVENING_ON_BEFORE_SUNSET_MIN jak oryginalny printTimeHM
     (bez próby "naprawy" - poza zakresem tej konwersji formatu).
     Usunięty surowy licznik minut ("%4d min") jako redundantny wobec
     pola hh:mm (ten sam precedens co usunięcie ti.tm_hour/min/sec w
     v213 - dane czasu w jednej reprezentacji wystarczą). Helper
     `printTimeHM()` (definicja + forward-deklaracja) usunięty w całości
     - był używany wyłącznie w tym jednym miejscu, po spłaszczeniu do
     inline logPrintf stał się martwym kodem.

     Walidacja: bilans `{}` 1843/1856 (było 1844/1857 - delta -1/-1
     spójna z usunięciem jednej funkcji o zbalansowanych nawiasach,
     printTimeHM). Delta cudzysłowów parzysta. Liczba linii pliku
     20116 (było 20125 - delta -9, spójna z usunięciem 8-liniowej
     definicji printTimeHM + 1-liniowej forward-deklaracji minus
     wcześniejsze puste linie odjęte przy okazji). Zero pozostałości
     starego formatu ("SUMMARY date=" bez `tag=`, "Energia dziś:",
     "ODCZYT HARMONOGRAMU Z EEPROM", `printTimeHM`) w kodzie poza
     komentarzami CHANGELOG (sprawdzone grepem). Świeży wieloliniowy
     grep logPrintf/logPrintln/logPrint bez `tag=` (z wyłączeniem
     tgPost) w kodzie po CHANGELOGU potwierdza zgodność z listą (a)-(d)
     z v213 plus jedno dodatkowe pominięcie nieopisane wcześniej -
     patrz sekcja "co zostało" w briefingu, punkt do zrobienia w
     kolejnej partii: ~19507 (`free=%luB maxAlloc=%luB frag=%s crit=%s
     uptime=%lus`, blok DIAG-17-RESTART) - ma już logfmt key=value, ale
     brakuje `lvl=`/`tag=`, podobny przypadek jak SUMMARY wyżej.

### v213 (2026-07-30) - LOGS-AI-STEP45: konwersja diagHealthReport() -
     "RAPORT ZDROWIA" (~linia 19819, wywoływany co 6h gdy Komentarze=true,
     wołany z ~19280). Ten blok nie był ujęty w rekonesansie "DUŻY
     POZOSTAŁY ZAKRES" opisanym w v212 - pominięcie wykryte świeżym
     grepem po logPrintf/logPrintln bez `tag=` (uwzględniającym wywołania
     wieloliniowe, nie tylko pojedynczą linię - poprzedni pobieżny grep
     dawał fałszywe trafienia/pominięcia).

     14-polowy wieloliniowy blok (osobne linie printf na tryb/power/noc,
     PWM backup, PWM auto, temp, lux, minlux/adapt/ucz, heap/wifi, rampy,
     zachód/ntp/uptime, plus puste linie-separatory) spłaszczony do jednej
     gęstej linii (canonical log line, wzorzec jak PODSUMOWANIE DNIA) pod
     nowym `tag=HEALTH`, `lvl=INFO` (raport okresowy, nie anomalia).
     Pola: tryb, power, noc, pwmB=v,v,v,v,v (5 kanałów CSV zamiast
     `[%4u %4u ...]`), pwmA=..., tempP1/tempP2/tempW, luxPok/luxWoda/
     luxCel, minlux/adapt/ucz, scale, heap, wifi, ss/tr/mss/mtr/rA/rS
     (flagi ramp), zachod=%02d:%02d, ntp, uptime=%lum.

     Usunięte: padding kolumnowy (`%-6s`/`%-3s`/`%-15s`/`%-4s` -
     niezgodny z zasadą "bez paddingu do wyrównania kolumn"), znak
     non-ASCII `°C` w etykiecie "W: %.1f°C" (zamieniony na zwykłe `tempW=`
     bez jednostki w tekście - jednostka wynika z nazwy pola), oraz
     `ti.tm_hour/tm_min/tm_sec` z listy argumentów - były redundantne
     względem znacznika czasu, który logToFile() i tak dopisuje do każdej
     linii (ten sam precedens co usunięcie manualnego `logTime()` w
     v212/FIX-v39-3). Zmienna `struct tm ti` zostaje w kodzie - nadal
     potrzebna jako bramka `if (!getLocalTimePL(&ti)) return;`.

     Walidacja: policzone `%`-specyfikatory (35) = liczba argumentów (35)
     w nowym wywołaniu. Bilans `{}` bez zmian (1843/1856). Zero trafień
     "RAPORT ZDROWIA" w pliku poza tym wpisem CHANGELOG (sprawdzone
     grepem). Świeży grep (wieloliniowy, nie pojedyncza linia) po
     logPrintf/logPrint/logPrintln bez `tag=` w kodzie po CHANGELOGU -
     pełna, poprawiona lista tego co zostało, patrz sekcja niżej.

     POPRAWIONY (względem v212) stan "co zostało" - świeży wieloliniowy
     grep, z wykluczeniem komentarzy i baneru startowego:
     (a) SUMMARY/Energia/harmonogram-etykieta: ~8959, ~8986, ~9172, ~9222;
     (b) UCZENIE/Transmisja/KOREKTA/ADAPT/RAMPA ADAPTACYJNA + sąsiadujące
     zapis/odczyt adapt_stats: ~10482, ~10884-11134, ~11246-11280, ~11854;
     (c) baner startowy setup() (~13125-13175, ~50 logPrintln z historią
     wersji) - osobna kategoria, do decyzji czy w ogóle konwertować;
     (d) rozproszone: ~13523 (coredump backtrace _cdBt), ~13638-13688
     ("Łączenie z WiFi" + kropki postępu + puste logPrint - dekoracyjne),
     ~15129 ("Terminal: URL"), ~18731/18792-18813 (MIĘKKI START MANUALNY/
     TRYB MANUALNY/Kamera WŁ/WYŁ). Poprzednie wskazanie zakresów
     "~13291, ~15141-15250, ~18816-19968" w v212 było niedokładne -
     część z nich to fałszywe trafienia (bloki DIAG-XX w rejonie
     18878-19344 i 19986-20030 już mają `tag=` w kolejnej linii stringa,
     pobieżny jednoliniowy grep tego nie widział), a RAPORT ZDROWIA
     (skonwertowany w tej partii) w ogóle nie był tam wymieniony.

### v212 (2026-07-30) - LOGS-AI-STEP44: konwersja klastra MIN LUX (3 wywołania,
     ~linie 9401-9479, funkcja applyMinLuxMode) + bloku STATUS LED I
     ZASILACZ (~linie 9974-9977) - łącznie 6 starych wywołań -> 4 nowe.

     MIN LUX (reużyty `tag=MIN-LUX`, precedens już istniał w tej samej
     funkcji dla gałęzi "Wejscie w okno swiecenia"/fallback czujnika):
     (1) gałąź WYŁĄCZONY/rampa-w-dół -> `msg="lux wystarczajace, rampa w
     dol do 0" lux= cel=`; (2) gałąź debug "Wystarczająco" (MIN LUX już
     nieaktywny, log okresowy) -> `msg="wystarczajaco" lux= cel=`;
     (3) gałąź "uzupełniam" (rampa w górę) -> `msg="uzupelniam" tryb=
     lux= cel= pwm=%d->%d czujnik=`. Wszystkie trzy `lvl=INFO` (rutynowa
     telemetria trybu MIN LUX, nie anomalia). Usunięty manualny prefiks
     `%s`+`logTime()` (precedens FIX-v39-3, ~linia 10749: logToFile()
     samo dopisuje znacznik czasu, podwójny prefiks był zbędny).
     ASCII-fikacja wartości pól: "Łagodnie"->"LAGODNIE",
     "nad_wodą"->"nad_woda" (bez polskich znaków w wartościach, zgodnie
     z konwencją "STAŁY"->"STALY" z v206).

     STATUS LED I ZASILACZ: dwa osobne `logPrintf` ("STAN LED: %s",
     "Zasilacz 24V: %s") + `logPrintln` dekoracyjnego separatora
     "-----------------------------" scalone w jedną linię (canonical
     log line - jedno zdarzenie, dwa pola ściśle powiązane, ten sam
     warunek `if`) pod nowym `tag=LED-STATUS` (`stan=WL|WYL
     zasilacz=WLACZONY|WYLACZONY`, `lvl=INFO`). Separator usunięty w
     całości - czysto dekoracyjny, zero danych.

     PRZY OKAZJI - sprostowanie do notatki z v211 ("NOWE ODKRYCIE"):
     log nagłówka "LED %d STAŁE" opisany tam jako wciąż w starym
     formacie w rzeczywistości już był skonwertowany w v210
     (`tag=LED-TEMP`, ~linia 9953 w bieżącej numeracji) - potwierdzone
     świeżym grepem, zero trafień starego wzorca w pliku. Notatka z v211
     była nieaktualna/błędna, nie odpowiadała stanowi kodu - nie ma tu
     nic do zrobienia, wpis w v211 można traktować jako zamknięty.

     Walidacja: bilans `{}` bez zmian (1842/1855), liczba linii pliku
     bez zmian (20021), delta cudzysłowów +2 (parzysta - z nowych
     `msg="..."` w MIN-LUX minus usunięty separator/scalenie w
     LED-STATUS), 0 pozostałości starego formatu ("MIN LUX: WYŁĄCZONY",
     "MIN LUX: Wystarczająco", "MIN LUX: %s uzupełniam", "STAN LED:",
     "Zasilacz 24V:", separator kreskowy) w wywołaniach logPrintf/
     logPrintln poza komentarzami CHANGELOG (sprawdzone grepem).

     NADAL NIEROZSTRZYGNIĘTE (bez zmian, patrz briefing): 1) tag=
     WIFI-STATUS vs tag=WiFi. 2) LOG-GUARD/WS/LOG-CLEAR/WDT-FIX
     (Serial.printf) - konwertować czy zostawić na stałe. 3) Prefiks
     [WARN]/[OK]/[ERR] vs pole lvl= - ujednolicenie odłożone.

     DUŻY POZOSTAŁY ZAKRES (świeży rekonesans grepem po tej partii,
     logPrintf/logPrintln bez `tag=`, poza deklaracjami funkcji i
     komentarzami): (a) klaster SUMMARY/Energia/harmonogram-etykieta
     (~linie 8897-9110) - SUMMARY już ma key=value ale bez `tag=`
     prefiksu, do decyzji czy dopisać `tag=SUMMARY` czy zostawić (już
     częściowo logfmt); (b) klaster UCZENIE/Transmisja/KOREKTA MIN/
     KOREKTA MAX/ADAPT/RAMPA ADAPTACYJNA (~linie 10822-11218, ta sama
     funkcja co MIN LUX skonwertowany w tej partii, sąsiaduje w
     kodzie - naturalny kandydat na następną partię); (c) baner
     startowy setup() z historią wersji (~linie 13063-13113, ~50 linii
     logPrintln z tekstem changelogu wypisywanym na starcie - inna
     kategoria niż runtime telemetry, do osobnej decyzji czy w ogóle
     konwertować); (d) rozproszone pojedyncze wywołania dalej w pliku
     (~13291, ~15141-15250, ~18816-19968) - nieprzejrzane w tej sesji.

### v211 (2026-07-30) - LOGS-AI-STEP43: konwersja odłożonego od v208 bloku
     diagnostycznego FLAGS/backup/rawPWM/hwPWM/prevHW/delta/manualB/autoB/
     temp (~linia 9815-9846, towarzyszy nagłówkowi FLASH/STATE-CHG) - 8
     wywołań. Zastosowany precedens z v210 (LED-TEMP): każde zdarzenie
     zostaje osobną linią (nie spłaszczam w jedną canonical line - to
     wciąż "stały zestaw 5 kanałów", ale 8 różnych pomiarów per-cykl,
     nie jedno zdarzenie). Nowe tagi (brak wcześniejszego precedensu):
     `tag=FLAGS-STATE` (przekazuje istniejący już logfmt string flagBuf
     bez zmian), `tag=BACKUP-PWM`, `tag=RAW-PWM`, `tag=HW-PWM`,
     `tag=HW-PWM-PREV`, `tag=FLASH-DELTA` (lvl=WARN, tylko przy
     flashDetected), `tag=MANUAL-PWM`, `tag=AUTO-PWM`, `tag=FLASH-TEMP`.
     Pola per-kanał `c0=..c4=` (nazewnictwo spójne z `tag=LED-CH kanal=`/
     `tag=RAMP-CH kanal=` - te iterują, tu kanały są rozpisane na jednej
     linii, więc `c0..c4` zamiast `kanal=`). Bez paddingu (`%4u`->`%u`),
     bez ozdobników `[...]`/`***BŁYSK***` - flaga błysku już niesie
     `lvl=WARN` na tagu.

     Walidacja: bilans `{}` bez zmian (1841/1854), liczba linii pliku bez
     zmian (19995), 0 pozostałości starego formatu ("FLAGS :", "backup :",
     "rawPWM :", "hwPWM :", "prevHW :", "delta :", "manualB:", "autoB :",
     "temp :") w wywołaniach logPrintf poza komentarzem CHANGELOG.

     NADAL NIEROZSTRZYGNIĘTE (bez zmian, patrz briefing): 1) tag=
     WIFI-STATUS vs tag=WiFi. 2) LOG-GUARD/WS/LOG-CLEAR/WDT-FIX
     (Serial.printf) - konwertować czy zostawić na stałe. 3) Prefiks
     [WARN]/[OK]/[ERR] vs pole lvl= - ujednolicenie odłożone.
### v210 (2026-07-30) - LOGS-AI-STEP42: nagłówek "LED STAŁE" (odkryty przy
     okazji v209, ~linia 9891) - rozstrzygnięte researchem (Brandur/
     Stripe canonical log line, zerolog #583): idea konsoliduje pola
     jednego zdarzenia w jedną gęstą linię, ale to zdarzenie (nagłówek
     z temperaturami) i pętla `tag=LED-CH` (5 linii per-kanał) to inny
     przypadek niż `PODSUMOWANIE DNIA` - liczba kanałów/pola per-kanał
     to zmienna struktura, nie stały zestaw pól jak w PODSUMOWANIU.
     Pełne scalenie nagłówka+pętli w jedną linię wymagałoby przebudowy
     pętli (dynamiczne klucze per-kanał) - większe ryzyko regresji w
     19.9k-liniowym pliku, ten sam typ zmiany co odłożony w v208 dump
     FLAGS/STATE-CHG. Decyzja: nagłówek dostaje własny `tag=LED-TEMP`
     (`tryb=STALE pwm= led1_c= led2_c= woda_c=`), zostaje osobną linią
     od pętli LED-CH - jedno zdarzenie = jedna linia, bez mieszania
     formatu nagłówka z formatem per-kanał pod wspólnym tagiem.

     PRZY OKAZJI: usunięty martwy kod - ręcznie liczony `timeBuf`/
     `timeinfo` (dawny prefiks "HH:MM:SS |" w starym formacie) był
     używany wyłącznie w tej jednej, teraz skonwertowanej linii;
     redundantny również dlatego, że `logToFile()` i tak dopisuje
     własny znacznik czasu `[HH:MM:SS ms=...]` do KAŻDEJ linii logu
     (patrz FIX-v128-LOG-TIMESTAMP-MS) - manualny prefiks nigdy nie
     był potrzebny.

     Walidacja: bilans `{}` bez zmian (1840/1853, identyczny z v209),
     liczba linii pliku: 19960 (z 19967 w v209: -7 martwy kod, brak
     zmiany w samej linii logu), delta cudzysłowów -4 (parzysta - z
     usuniętych literałów `"??:??:??"`/`"%02d:%02d:%02d"` martwego
     kodu), 0 pozostałości "STAŁE"/starego formatu w wywołaniach
     logPrintf poza komentarzem changelogowym v209 (opis historyczny,
     nietykany).

     NADAL NIEROZSTRZYGNIĘTE (bez zmian, patrz briefing): 1) tag=
     WIFI-STATUS vs tag=WiFi. 2) LOG-GUARD/WS/LOG-CLEAR/WDT-FIX
     (Serial.printf) - konwertować czy zostawić na stałe. 3) Prefiks
     [WARN]/[OK]/[ERR] vs pole lvl= - ujednolicenie odłożone.

### v209 (2026-07-30) - LOGS-AI-STEP41: konwencja dla dekoracyjnego `[%d]`
     (pytanie otwarte 4 z briefingu) - rozstrzygnięte researchem: logfmt/
     canonical-log-line (Brandur, Splunk) wprost odradza numer porządkowy
     w nawiasie bez nazwanego pola - liczy się jako "guesswork" dokładnie
     tego typu, jaki logfmt ma eliminować. Decyzja: gdy indeks ma jasną
     tożsamość domenową (kanał/slot), użyć nazwanego pola zgodnego z
     istniejącym precedensem, NIE generycznego `idx=`.

     1) Harmonogram POMPA (EEPROM-load dump, dawne "[%d] %02d:%02d -
     %02d:%02d", ~linia 8972): to ten sam log co już istniejący
     `tag=POMPA-SLOT` z panelu WWW (funkcja zapisu, ~linia 13909) -
     scalony pod ten sam tag zamiast tworzyć nowy/`idx=` (pola
     `slot=%d start=%02d:%02d koniec=%02d:%02d`, `lvl=INFO`).

     PRZY OKAZJI: w tym samym miejscu (funkcja zapisu z panelu, ~linia
     13907-13909) znaleziony i naprawiony pre-istniejący bug -
     podwójnie eskejpowane `\\n` (dosłowny tekst "\n" w logu zamiast
     znaku nowej linii) w dwóch wywołaniach `tag=POMPA`/`tag=POMPA-SLOT`
     - poprawione na pojedyncze `\n`. Bug niezwiązany z konwersją
     logfmt, ale dotyczył tego samego bloku kodu.

     2) Lista kanałów LED (dawne "[%d] %s : %3d (%.0f%%) GPIO:%d%s",
     ~linia 9841): brak tu wcześniejszego tagu-precedensu, więc nowy
     `tag=LED-CH` z polem `kanal=%d` (nazwa pola spójna z istniejącym
     `tag=RAMP-CH kanal=`), plus `nazwa=%s pwm=%d pct=%.0f%% gpio=%d`,
     opcjonalne ` derating=tak` gdy aktywny derating (dawniej tekst
     " DERATING" doklejony do formatu) -> `lvl=INFO` (rutynowa
     telemetria, nie anomalia). Przy okazji: usunięty padding spacjami
     w lokalnej tablicy `names[5]` ("biale    " -> "biale" itd.) i
     szerokość `%3d` na wartości liczbowej - zgodnie z zasadą projektu
     "bez paddingu do wyrównania kolumn" (ta tablica jest używana
     wyłącznie w tym jednym miejscu, zero ryzyka regresji gdzie indziej).

     3) `f.print("[FORCE-FLUSH]...")` w `logForceFlush()` - dopisane do
     zakresu tej partii na wyraźną decyzję człowieka. To bezpośredni
     zapis do pliku logu (nie przez `logPrintf`), ale BEZ ryzyka
     rekurencji przez `logToFile()` (funkcja pisze do już otwartego `f`
     wprost) - inna kategoria niż 4 świadomie pominięte `Serial.printf`
     (te zostają nietknięte). Nowy `tag=FORCE-FLUSH`, `lvl=INFO` (marker
     wywoływany przy KAŻDYM restarcie, nie tylko przy błędzie - patrz
     wywołania w OTA-success/OTA-error/HTTP-restart/heap-critical-restart).

     Walidacja: bilans `{}` bez zmian (1840/1853, identyczny z oryginałem
     - brak zmian strukturalnych), liczba linii pliku bez zmian (19901
     przed dopisaniem tego wpisu), delta cudzysłowów +2 (parzysta, z
     nowego `msg="..."` w FORCE-FLUSH), 0 pozostałości starego formatu
     (`[%d] %02d:%02d...`, `[%d] %s : %3d...`, `[FORCE-FLUSH]`) w
     wywołaniach logPrintf/logPrintln/f.print (sprawdzone grepem -
     jedyne pozostałe trafienia to komentarze CHANGELOG, nietykane).

     NADAL NIEROZSTRZYGNIĘTE (bez zmian względem v208, patrz briefing):
     1) tag=WIFI-STATUS vs tag=WiFi. 2) LOG-GUARD/WS/LOG-CLEAR/WDT-FIX
     (Serial.printf) - konwertować czy zostawić na stałe. 3) Prefiks
     [WARN]/[OK]/[ERR] vs pole lvl= - ujednolicenie odłożone.

     NOWE ODKRYCIE (do decyzji człowieka, NIE tknięte w tej partii): tuż
     nad blokiem LED-CH (~linia 9827) log `"%s | LED %d STAŁE | LED1:%.0f
     °C LED2:%.0f°C woda:%.0f°C\n"` (z `timeBuf` jako prefiks) jest wciąż
     w starym, nie-logfmt formacie (pipe-delimited, jednostki °C w
     tekście) i tworzy razem z pętlą LED-CH dwuczęściowy blok tego samego
     zdarzenia (nagłówek + 5 linii per-kanał) - podobny kształt do
     dawnego dumpu FLAGS/STATE-CHG z v208, odłożonego jako "większa
     zmiana strukturalna". Kandydat do spłaszczenia w jedną canonical
     log line razem z LED-CH, ale to osobna decyzja, nie robię przy
     okazji tej partii.
### v208 (2026-07-30) - LOGS-AI-STEP40: dokończenie rekonesansu z v207 —
     5 wywołań `logPrintf/logPrintln`. Nowe tagi: `tag=POWER-OFF`
     (lvl=INFO, zastępuje `[ZASILANIE WYŁ]`), `tag=FLASH` (lvl=WARN,
     skok hardware PWM — anomalia, nie rutyna; pole `nr=` zamiast
     `#%lu` w nawiasie), `tag=STATE-CHG` (lvl=INFO, zmiana
     power/supply). `tag=RAMP-CH` (x2, gałąź "interval liczony" i
     gałąź "roznica=0 pomijam kanał") z nowym polem `kanal=` (numer
     kanału rampy, dawniej `%d` w nawiasie tagu).

     ŚWIADOMIE POZA ZAKRESEM: 9 linii dumpu diagnostycznego
     (FLAGS/backup/rawPWM/hwPWM/prevHW/delta/manualB/autoB/temp,
     ~linie 9673-9704) towarzyszących blokowi FLASH/STATE-CHG — nie
     były na oryginalnej liście kandydatów z v207, nie mają emoji/
     starego tagu do zamiany (już czysty tekst "etykieta : wartości"),
     a spłaszczenie ich do jednej linii (canonical log line, jak przy
     PODSUMOWANIU DNIA) to większa zmiana strukturalna - do osobnej
     decyzji, nie robię przy okazji tej partii.

     Walidacja: bilans `{}` niezmieniony (1839/1852), liczba linii
     pliku bez zmian (19861 przed dopisaniem tego wpisu), delta
     cudzysłowów +8 (parzysta, zgodna z 5 zmienionymi wywołaniami),
     0 pozostałości starego formatu ("[ZASILANIE WYŁ]"/"[FLASH#.../
     "[STATE-CHG.../"[RAMP-CH...") poza komentarzami CHANGELOG.

     NADAL NIEROZSTRZYGNIĘTE (do decyzji człowieka, patrz briefing):
     1) tag=WIFI-STATUS vs tag=WiFi - scalać czy zostawić rozdzielone?
     2) LOG-GUARD/WS/LOG-CLEAR/WDT-FIX (Serial.printf) - konwertować
        mimo ryzyka rekurencji w logToFile(), czy zostawić na stałe?
     3) Prefiks poziomu [WARN]/[OK]/[ERR] vs pole lvl= - w praktyce
        już głównie lvl=, ale reszta starych [WARN]/[OK]/[ERR] (patrz
        "POZOSTAŁO" w nagłówkach wcześniejszych wersji) czeka na
        ujednolicenie.

     POZOSTAŁO z rekonesansu v207: "[%d] %02d:%02d - %02d:%02d"
     (harmonogram, ~linia 8931) i "[%d] %s : %3d (%.0f%%) GPIO:%d%s"
     (lista sekcji z nazwami, ~linia 9800) - oba to dekoracyjny numer
     porządkowy "[%d]" bez jasnego kandydata na tag=, ten sam typ co
     wcześniej świadomie odłożone dumpy per-slot POMPA/LED (patrz
     nagłówek dawnej wersji, sekcja "ŚWIADOMIE NIETKNIĘTE") - do
     wspólnej decyzji o konwencji zamiast konwertowania każdego z
     osobna.
### v207 (2026-07-30) - LOGS-AI-STEP39: sprzątanie rekonesansu po v206 —
     15 wywołań `logPrintf/logPrintln` (nie `Serial.printf`, te zostają
     nietknięte, patrz niżej). Reużyte tagi: `tag=DS18B20` (gałąź WARN,
     seria 10 błędów wody). Nowe tagi (wszystkie `lvl=INFO` chyba że
     zaznaczono): `tag=TG-TASK` (start tasku), `tag=TG-ALIVE` (heartbeat),
     `tag=WIFI-STATUS` (zmiana statusu WiFi.status() — celowo odrębny od
     `tag=WiFi` z v206, który dotyczy próby połączenia; scalenie
     odłożone, do decyzji), `tag=FB-BLOCK`/`tag=FB-BLOCK-DONE`,
     `tag=WIFI-GUARD` z nowym polem `akcja=start|stop` (precedens
     RAMPA/v206), `tag=AUTO` (log przerwy/nocy, prefiks `logTime()`
     zachowany bez zmian — poza zakresem tej partii), `tag=FIX-BREAK-v15`,
     `tag=SEKCJA5-DBG`, `tag=RAMP-ADAPT-START`, `tag=SECT5-FIX` ->
     `lvl=WARN` (log jednorazowy przy zadziałaniu kapu mocy — anomalia,
     nie rutyna), `tag=SCHED5`, `tag=TEMP`, `tag=MANUAL`, `tag=HEAP-MIN`
     (nowe minimum sterty — odrębne od heartbeatu `tag=HEAP` żeby nie
     mylić zdarzenia z cyklicznym logiem), `tag=HEAP` (heartbeat),
     `tag=HEAP-b`, `tag=HEAP-c`.

     ŚWIADOMIE NIETKNIĘTE: LOG-GUARD/WS/LOG-CLEAR/WDT-FIX (Serial.printf,
     nie logPrintf) — precedens v195, ryzyko rekurencji przez
     logToFile(). Zostają w starym formacie do osobnej decyzji.

     Walidacja: bilans `{}` niezmieniony względem v206 (delta +1/+1
     wynika z literału `` `{}` `` w tekście tego wpisu CHANGELOG, nie ze
     zmiany struktury kodu), liczba linii pliku bez zmian (19828),
     wszystkie edycje wyłącznie w treści literałów log.

     NOWY REKONESANS (poza pierwotną listą ~27, znaleziony grepem po tej
     partii — NIE konwertowane w tym kroku): `[%d] %02d:%02d...`
     (harmonogram, list okien czasowych), `[ZASILANIE WYŁ]`,
     `[FLASH#%lu @call%lu]`, `[STATE-CHG @call%lu]`, `[%d] %s : %3d...`
     (lista sekcji z nazwami), `[RAMP-CH%d]` (x2, kanał rampy).
### v206 (2026-07-30) - LOGS-AI-STEP38: klaster "WiFi-connect-attempt/TG-DNS-
     resolved/EEPROM-config-load (EMA/SENS/RAMP)/MIN-LUX-window/RAMPA-
     adaptacyjna-start-stop/PWR-LIMIT/SIM-LUX/ADAPT-RESULT/BEZPIECZNIK/
     ANOMALIA" (linie ~7173-10913, 14 wywołań). `tag=WiFi` (reużyty,
     próba połączenia z siecią) i `tag=TG-DNS` (reużyty, gałąź sukcesu -
     gałąź WARN była już skonwertowana wcześniej) -> oba `lvl=INFO`.
     `tag=EEPROM` (reużyty, precedens wielu wcześniejszych konwersji
     odczytu konfiguracji) dla trzech odczytów startowych: EMA filtr
     (`alpha=`), interwał czujnika (`sek=`), czas rampy PWM (`sek=`) ->
     `lvl=INFO`. `tag=MIN-LUX` (reużyty z v195) dla bare loga wyjścia z
     trybu przy wejściu w okno świecenia -> `lvl=INFO`. Trzy warianty
     "[RAMPA START]/[RAMPA STOP]" (dwa specyficzne dla applyMinLuxMode +
     jeden generyczny caller-based) -> zunifikowane pod reużyty
     `tag=RAMPA` (już istniał z `wywolal=` dla gałęzi zablokowanej) z
     nowym polem `akcja=start|stop` (precedens FIX-v52/v205 dla
     rozróżniania gałęzi jednym tagiem), pola `od=`/`do=`/`plan=` ->
     `lvl=INFO` (rutynowy start/koniec rampy, nie anomalia sama w sobie -
     w odróżnieniu od `tag=BEZPIECZNIK`/`tag=ANOMALIA` niżej). Nowy
     `tag=PWR-LIMIT` (loguje tylko przy zmianie skali mocy ≥1% - telemetria
     balancera, nie błąd -> `lvl=INFO`), nazewnictwo kanałów `mocB=/mocFS=/
     mocFSB=/mocN=/mocC=` spójne z `tag=SUMMARY` z kroku 2. Nowy
     `tag=SIM-LUX` (log cykliczny co 30s w trybie symulacji sprzętu,
     `lvl=INFO`), wartość "STAŁY" -> "STALY" (bez polskich znaków, konwencja
     z partii 29+). Nowy `tag=ADAPT-RESULT` (diagnostyka TRYB0-brak-
     czujnika, `lvl=INFO`, pole `tryb=0-brak-czujnika`). Nowy
     `tag=BEZPIECZNIK` (zadziałanie zabezpieczenia przy overshoot rampy)
     i reużyty `tag=ANOMALIA` (rampa zakończona natychmiast, <500ms) ->
     oba `lvl=WARN` (sama obecność wpisu to już nietypowa sytuacja,
     precedens LOOP-CP/LOOP-SLOW/v204).

     Walidacja: bilans `{}` bez zmian względem stanu przed tą partią
     (1836/1849, identyczny z oryginałem - edycje wyłącznie w treści
     literałów log, żadna zmiana strukturalna), liczba linii pliku bez
     zmian (19774) przed dopisaniem tego wpisu CHANGELOG, delta
     cudzysłowów +14 (parzysta, 7x nowy `msg=` dodany: TG-DNS/EMA/SENS/
     RAMP/WiFi/MIN-LUX/ANOMALIA), 0 pozostałych wystąpień starego formatu
     `[WiFi]`/`[TG-DNS]`/`[EMA]`/`[SENS]`/`[RAMP]`/`[MIN LUX]`/
     `[RAMPA STOP]`/`[RAMPA START]`/`[PWR-LIMIT]`/`[SIM-LUX]`/
     `[ADAPT-RESULT]`/`[BEZPIECZNIK]`/`[ANOMALIA]` w wywołaniach
     logPrintf/logPrintln/Serial.printf (sprawdzone grepem po edycji -
     jedyne pozostałe trafienia to komentarze CHANGELOG, nietykane z
     zasady). Filtr JS panelu WWW: do przejrzenia równolegle (patrz
     pkt 2.3 planu) - nie zweryfikowane w tym kroku, panel WWW nie był
     częścią przesłanych plików w tej turze.

     POZOSTAJE (rekonesans po tej partii): ~27 kandydatów w starym
     formacie nawiasowym - LOG-GUARD (x2, Serial.printf), WS (x2,
     Serial.printf), TG-TASK (bare wywołanie poza już-skonwertowaną
     rodziną z v196 - do zweryfikowania czy to duplikat tagu czy inne
     miejsce), TG-ALIVE, WIFI (wielka litera - odrębny od tag=WiFi,
     do decyzji czy scalić), LOG-CLEAR (x2, Serial.printf), FB-BLOCK,
     FB-BLOCK-DONE, WIFI-GUARD (x2), WDT-FIX (x2, Serial.printf),
     FIX-BREAK v15, %s [AUTO], SEKCJA5-DBG, RAMP-ADAPT-START, SECT5-FIX,
     SCHED5, DS18B20, TEMP, MANUAL, [INFO] [HEAP] (podwójny prefiks),
     HEAP, HEAP-b, HEAP-c.
### v205 (2026-07-30) - LOGS-AI-STEP37: moduł "KAMERA/RESTART/NTP-debug/
     WiFi hard-reconnect/FIX-v52 rebind/SECTION" (loop(), linie ~16465-
     16624). 7 wywołań w starym formacie (nawiasowym lub bez tagu wcale).
     `tag=KAMERA` (nowy, auto-wyłączenie po 30 min) i `tag=RESTART`
     (reużyty, już istniał z lvl=INFO dla przyczyny resetu na boot) ->
     oba `lvl=INFO` (zdarzenia rutynowe, nie anomalia). Debug "Czas NTP
     zsynchronizowany" (bez tagu, tylko emoji) -> reużyty `tag=NTP`
     (już istniał dla "Zachod slonca przeliczony"), `lvl=INFO`. WiFi
     hard-reconnect (`[WiFi] Hard-reconnect #%u...`) -> reużyty `tag=WiFi`
     `lvl=WARN` (ta sama gałąź anomalii co sąsiedni już-skonwertowany log
     "Brak polaczenia"), nowe pole `proba=%u`. `[FIX-v52]` (WWW server
     rebind/begin po odzyskaniu WiFi, 2 miejsca) -> tag zachowany jako
     identyfikator fixa (precedens FIX-v16/v198), `lvl=INFO`, nowe pole
     `akcja=rebind|begin` (rozróżnienie dwóch gałęzi zamiast osobnych
     tagów, precedens WYWOLANIE/v198) + `ip=%s:8080`. Makro `_SECTION_T`
     (`[SECTION] %s = %lums`, loguje TYLKO gdy sekcja >1000ms) -> `tag=
     SECTION lvl=WARN` (precedens LOOP-CP/LOOP-SLOW/v204: log obecny
     tylko przy przekroczeniu progu = już anomalia), nowe pole `label=%s`,
     jednostka `ms` usunięta z wartości (`ms=%lu` zamiast `%lums`).
     Usunięto emoji ⏱/⏰ z treści logów runtime. Walidacja: bilans `{}`
     identyczny z oryginałem (1835/1848), liczba linii pliku bez zmian
     (19752/19752), delta cudzysłowów +8 (parzysta, 4x nowy msg=), 0
     pozostałych `[KAMERA]`/`[RESTART]`/`[WiFi]`/`[FIX-v52]`/`[SECTION]`
     w tym module.
### v204 (2026-07-30) - LOGS-AI-STEP36: moduł "LOOP-CP/LOOP-SLOW/LOOP-STAT"
     (loop(), profiler czasu iteracji, linie ~16298-16328). 3 wywołania
     logPrintf w starym formacie nawiasowym `[TAG]`. `tag=LOOP-CP`
     (makro LOOP_CP, loguje TYLKO gdy sekcja >100ms) i `tag=LOOP-SLOW`
     (loguje TYLKO gdy iteracja >500ms) -> oba `lvl=WARN` (precedens
     TG-KA/v192: log obecny tylko przy przekroczeniu progu = już
     anomalia, nawet bez oryginalnego prefiksu poziomu). `tag=LOOP-STAT`
     (raport cykliczny co 5 min, zawsze) -> `lvl=INFO`. Usunięto
     dekoracyjny padding `%-22s %3lu` z LOOP-CP, nowe pole `label=%s`;
     dopisek `(ostatnie 5min)` z LOOP-STAT wydobyty do `okres=5min`;
     zdanie "cos blokuje loop()!" skrócone do `msg="cos blokuje loop"`.
     Jednostka `ms` usunięta z wartości liczbowych, zostaje w nazwie
     klucza (`ms=`). Walidacja: bilans `{}` 1834/1847 (delta +1/+1 wzgl.
     v203, zgodna - brak zmian strukturalnych), liczba linii pliku bez
     zmian (19737), 0 pozostałych `[LOOP-CP]`/`[LOOP-SLOW]`/`[LOOP-STAT]`.
### v203 (2026-07-30) - LOGS-AI-STEP35: moduły "ADAPT wczytano statystyki" i
     "HIST start-po-resecie/kompaktowanie-zaplanowane" (linie ~15262-
     15375, loadAdaptStats()/saveHistoryPoint()). 3 wywołania logPrintf
     bez oryginalnego nawiasu-poziomu, wszystkie -> lvl=INFO (rutynowe
     zdarzenia startowe/harmonogramowe, nie anomalia). `tag=ADAPT`
     reużyty (już istniał z lvl=WARN dla gałęzi uszkodzonego pliku w tej
     samej funkcji) - "Wczytano statystyki" -> `msg="Wczytano
     statystyki" korMin= korMax= pomiary=`. Nowy `tag=HIST` (odróżniony
     od już-skonwertowanego `tag=HIST-COMPACT`, bo to inna, wcześniejsza
     faza - odtworzenie stanu po restarcie i decyzja o zaplanowaniu
     kompaktowania, nie samo kompaktowanie): "start po resecie" ->
     `msg="Start po resecie" linecount= startline= live=`; "kompaktowanie
     zaplanowane" -> `msg="Kompaktowanie zaplanowane" startline=
     linecount=` (klucze `linecount=`/`startline=` malymi literami, spójne
     z już istniejącym `tag=HIST-COMPACT`). Walidacja: bilans `{}`
     identyczny z oryginałem (1833/1846), liczba linii pliku bez zmian
     (19719/19719), delta cudzysłowów +6 (parzysta, 3x nowy msg=), 0
     pozostałych wywołań logPrintf("[ADAPT]...")/logPrintf("[HIST]...").
### v202 (2026-07-30) - LOGS-AI-STEP34: moduł "Terminal/NTP wait/I2C reset/
     TSL init/SYSTEM GOTOWY/BACKOFF" (setup(), boot-time, linie ~14725-
     14980). Usunięto emoji ⏳/🌇 i format nawiasowy `[BACKOFF]`. Konwersje:
     pętla oczekiwania NTP -> `tag=NTP msg="Oczekiwanie na czas NTP"`;
     "Zachód słońca lokalnie" -> `tag=NTP zachod=%02d:%02d` (pole `zachod=`
     wg precedensu już istniejącego gdzie indziej w pliku, tag=NTP spójny
     z sąsiednim logiem "Czas UTC" tej samej gałęzi); "Reset I2C..." i
     "Inicjalizacja czujników TSL2561..." -> `tag=TSL-INIT` (ten sam tag,
     co już skonwertowane logi wykrycia czujników niżej w tej samej
     funkcji). Canonical-line: "SYSTEM GOTOWY" + osobny `sunsetMinutes =
     %02d:%02d` (2 logPrintf) scalone w JEDNĄ linię `tag=BOOT msg="System
     gotowy" zachod=%02d:%02d` (tag=BOOT spójny z innymi logami fazy
     boot/WiFi-connect w tym samym pliku). BACKOFF (crash-loop, wykonuje
     się tylko gdy streak>=2 - sama obecność to już anomalia, precedens
     WDT-HUNT/v201 i TG-KA/v192): `tag=BACKOFF lvl=WARN` na wykryciu
     (pola streak=/checkpoint=/reason=/opoznienie=), `tag=BACKOFF
     lvl=INFO` na zakończeniu (rutynowe potwierdzenie, nie nowa anomalia).
     Usunięto polskie znaki diakrytyczne z msg= (spójność grepowalności,
     konwencja z partii 29+). Walidacja: bilans {} bez zmian poza tym
     wpisem CHANGELOG; stary format (emoji/nawiasy) 0 wystąpień w zakresie
     linii 14725-14980; 9 oryginalnych logPrintf/logPrintln scalonych do
     8 linii logu (SYSTEM GOTOWY+sunsetMinutes -> 1 canonical line).

### v201 (2026-07-30) - LOGS-AI-STEP32: moduł "EEPROM readout + RESTART/
     PRE-RESET/WDT-HUNT/SETUP orphans/PSRAM" (setup(), boot-time). Dwa
     przykłady canonical/wide log line (zgodnie z RybyLED_logi_pod_AI.md
     pkt 2.1): (1) "=== ODCZYT JASNOŚCI LED Z EEPROM ===" + 5x logPrintf
     w pętli (sekcje) + 6 osobnych logPrintf (RAW/rampa/4x harmonogram) ->
     JEDNA linia `tag=EEPROM-READOUT` (pętla teraz buduje sek0..sek4= do
     lokalnego bufora, potem jeden logPrintf; usunięto zbędne %02d:%02d
     wyliczane z już obecnych minut); (2) 4 kolejne logPrintf "[PRE-RESET]
     Poprzedni cykl/heap/wifi/checkpoint" -> JEDNA linia `tag=PRE-RESET`
     ze wszystkimi polami. Reszta modułu 1:1: `tag=RESTART` (przyczyna
     restartu + EXTERNAL_PIN note, usunięto emoji ℹ), `tag=WDT-HUNT`
     (WARN - crash-streak to sama w sobie anomalia, drukuje się tylko
     streak>=2), `tag=PRE-RESET` "brak danych" (osobna gałąź, power-on),
     `tag=SETUP-ORPHAN` (x3: history_tmp.csv/wifi_list_tmp.txt/
     wifi_list_old.txt, ujednolicone pod jeden tag z polem `plik=` -
     precedens SSTART-INIT/v199 kontekst=; WARN bo sama obecność pliku-
     sieroty to już anomalia, jak TG-KA/v192), `tag=PSRAM`/`tag=PSRAM-TLS`
     (usunięto dopisek wersji "v129" z treści loga - to nie jest runtime-
     istotne, zostaje tylko w tym komentarzu CHANGELOG). Walidacja: bilans
     {} w kodzie identyczny z oryginałem (różnica +1/+1 tylko w tekście
     tego wpisu CHANGELOG, nie w kodzie); stary format 0 wystąpień;
     8+4=12 logPrintf scalone do 2 linii (redukcja ~10 linii/restart).

### v200 (2026-07-30) - LOGS-AI-STEP31: świeży pełny rekonesans (wcześniejsze
     szacunki ~275 kandydatów okazały się zawyżone przez wywołania
     wieloliniowe już skonwertowane w linii kontynuacji - realnie 166).
     Skonwertowano moduł Telegram-connect (17 wywołań, w pełni izolowany):
     TG-CONN (x2: próba połączenia + wynik connect()), MEM-FREE (x4: src=TG/
     TG-OOM/TG-TIMEOUT/TG-DOC - usunięto emoji 🆓), TLS-MUTEX (x1, czekanie
     >50ms na mutex - usunięto emoji ⏳, WARN jak sąsiedni już skonwertowany
     przypadek timeout), TLS-COOL (x1), TLS-DRAM (x1, rozbite heapSrc=TG_TLS
     state=%s zamiast sklejonego "heapSrc=TG_TLS %s"), PSRAM-TLS (x1, WARN -
     anomalia "Fix 4 nieaktywny"), TG-POST (x3: log zawsze + 2x MEM-FREE-
     sąsiadujące już policzone wyżej, "wymuszono disconnect"->WARN), TG-DEL
     (x1), TG-HIST-DEL (x3: start/iter/done, nowe pole faza=), POLL (x1,
     cbData= zamiast callback_data='%s'). MEM-FREE/TG-POST WARN tylko w
     gałęziach błędu (OOM/TIMEOUT/disconnect wymuszony), reszta INFO -
     zgodnie z konwencją "loguje tylko przy niepowodzeniu -> WARN" (precedens
     TG-KA/v192) tam gdzie dotyczy, INFO dla logów zawsze-obecnych. TG-CONN
     "connect()=%s | TCP+TLS=%lums | via=%s | fH=%luB%s" z warunkowym
     sufiksem " WOLNY!"/" BARDZO WOLNY!" -> stałe pole slow=nie/tak/bardzo
     (logfmt: pole zawsze obecne zamiast warunkowego tekstu w wartości).
     Walidacja: bilans {} identyczny z oryginałem (1830/1843), liczba
     cudzysłowów identyczna (2435/2435), stary format tych 10 tagów: 0
     wystąpień w aktywnych wywołaniach. Pozostaje ~149 kandydatów z
     pełnego rekonesansu (patrz PROGRESS.md dla podziału na moduły).

### v195 (2026-07-30) - LOGS-AI-STEP26: konwersja całej pozostałej puli [WARN]
     (36 wywołań, w tym 4 sąsiednie ERR/INFO skonwertowane przy okazji tego
     samego bloku) w JEDNEJ dużej partii, zamiast dzielenia na pod-partie
     per moduł jak dotychczas - decyzja podjęta na wyraźną prośbę o
     przyspieszenie. Zakres: TLS-MUTEX (x2, różne funkcje), TG-DNS,
     HARMONOGRAM (x2), MIN LUX (x2, nowy tag MIN-LUX), RAMPA/RAMP-DBG (x3),
     FLASH-STALL (x3, w tym 2x Serial.printf zamiast logPrintf -
     celowo pozostawione jako Serial.printf, patrz komentarz projektowy o
     unikaniu rekurencji w logToFile()), DFS, LittleFS/DIR (WARN+ERR+2xOK,
     reużyty tag DIR z DIAG/v173), FS-CAPACITY, WDT (x2)+PANIC, BOOT
     (WARN+WARN+OK), PSRAM, PANEL-AUTO (WARN+OK), SETUP (WARN+sąsiedni
     bare ERR), TSL-INIT (nowy tag - I2C gotowy + 2 pary czujnik OK/WARN,
     ODDZIELNY od tagu TSL już skonwertowanego w v186 dla innej funkcji -
     reinit i pierwsza inicjalizacja w setup() to różne miejsca w kodzie),
     ADAPT, FB-INIT (INFO+WARN+WARN, odwrócona kolejność tagów
     "[FB-INIT] [WARN]" potwierdzona z rekonesansu - poprawiona na
     standardowe lvl=WARN tag=FB-INIT), WS-CLEANUP-STALL, CICHA ZMIANA
     (x2, nowy tag CICHA-ZMIANA - spacja w oryginalnym tagu niedozwolona
     w wartości logfmt, zamieniona na myślnik), WiFi (Brak połączenia),
     EVENTS-STALL, TRYBAUT (x2).

     DECYZJA: sąsiednie ERR/OK/INFO w tym samym bloku if/else co edytowany
     WARN skonwertowane od razu razem z nim (LittleFS/DIR, BOOT, PANEL-AUTO,
     TSL-INIT, SETUP, FB-INIT) - ten sam wzorzec co "domknięcie sąsiedztwa"
     z TG-CMD/v188 i DOC-KA+DOC-SPEED/v191, tylko zastosowany szerzej naraz.
     Bare TRYBAUT-DBG (x2, tuż obok TRYBAUT) świadomie NIE skonwertowane -
     inny tag spoza zakresu [WARN], zostawiony na później.

     Nowe pola: tag=MIN-LUX (zamiast spacji w oryginalnym "[MIN LUX]"),
     tag=CICHA-ZMIANA (jw., "[CICHA ZMIANA]"), site= w FLASH-STALL (3x,
     reużyta konwencja z TG-POST/DNS-STALL v189/v190), czujnik=/addr= w
     TSL-INIT, proba=/host= w FB-INIT i TG-DNS.

     WALIDACJA: 0 wystąpień starego formatu "[WARN]" w wywołaniach
     logPrintf/logPrintln/Serial.printf (sprawdzone grepem po edycji).
     36 dopasowań nowych tag= wprowadzonych tą partią. Cudzysłowy parzyste
     we wszystkich 36+4 edytowanych liniach (sprawdzone programowo, 0 lub 2
     na linię). Nawiasy klamrowe { }: bez zmian względem stanu przed tą
     partią (edycje wyłącznie w treści literałów log, przed dodaniem tego
     wpisu CHANGELOG).
### v196 (2026-07-30) - LOGS-AI-STEP27: konwersja całej pozostałej puli [OK]
     (20 wywołań z tagiem [OK] + 4 sąsiednie skonwertowane przy okazji
     tego samego bloku: 2x bare/[ERR] przy NTP i TG-TASK, 1x bare
     [AWARIA] podniesiony do WARN) = 24 zmienione linie łącznie. Zakres:
     SOFT-START (1), PRZEJSCIE (x4 - AUTO/MANUALNE/harmonogram 10s/
     transition), ZAPIS (x2 - tryb+zasilanie do EEPROM), RAMPA-KONIEC
     (adaptacyjna, wywolal=), RAMPA (harmonogram, kierunek=), EEPROM
     (adaptacja odczytana), SETUP (konfiguracja zakończona), WiFi (x2 -
     połączono/ponownie połączony), PANEL-AUTO (x2), NTP (x3 - nowy tag,
     brak czasu ERR + czas UTC + zachód słońca), TG-TASK (x3 - nowy tag,
     2xOK+1xERR skonwertowane razem), TRYBAUT (powrót do normy), AWARIA
     (x2 - nowy tag, wyłączenie WARN + gotowość INFO).

     DECYZJE: PRZEJŚCIE AUTO/MANUALNE (~9490/9508) i osobna RAMPA
     PRZEJŚCIOWA 10s (~17606) to trzy różne miejsca w kodzie logujące
     koniec przejścia jasności - zunifikowane pod wspólny tag=PRZEJSCIE,
     z polem tryb=AUTO/MANUAL tam gdzie odróżnialne, bez tego pola przy
     transition 10s (nieodróżnialne z kontekstu). RAMPA-KONIEC (caller-
     based, wywolal=) i RAMPA (harmonogram, kierunek=) to różne funkcje
     z różnymi danymi - NIE scalone pod jeden tag. Linia ~17247 (RAMPA
     harmonogramu) ma manualny prefiks "%s", logTime().c_str() PRZED
     lvl=/tag= - duplikat znacznika czasu (logToFile() i tak dodaje
     własny timestamp, ten sam wzorzec co opisany w komentarzu FIX-v39-3
     przy linii ~10325) - ŚWIADOMIE NIETKNIĘTY, poza zakresem tej
     konwersji (tylko [TAG]->lvl=/tag= przekonwertowane, prefiks
     zostawiony jak był). Wartości "WZNOSZENIE"/"OPADANIE" zamienione na
     "gora"/"dol" (ASCII bez spacji - bezpieczne jako wartość logfmt).
     Wartości "WŁ"/"WYŁ" w ZAPIS zasilanie=%s POZOSTAWIONE z polskimi
     znakami - zgodnie z precedensem tag=DIR/stan=%s z v195
     (littlefsReady ? "WŁ" : "WYŁ"), dla spójności z już istniejącym
     skonwertowanym kodem.

     Nowe tagi: NTP, TG-TASK, AWARIA, RAMPA-KONIEC, PRZEJSCIE, SOFT-START,
     ZAPIS. Nowe pola: od=/do=/czas= (PRZEJSCIE/SOFT-START), wywolal=
     (RAMPA-KONIEC), kierunek=/proc= (RAMPA), cel=EEPROM (ZAPIS), godz=
     (NTP Czas UTC), zachod= (NTP), stos= (TG-TASK), proby=/przerwa=
     (WiFi), temp= (AWARIA).

     WALIDACJA: 0 wystąpień starego formatu "[OK]" w wywołaniach
     logPrintf/logPrintln/Serial.printf (sprawdzone grepem po edycji,
     jedyne pozostałe trafienie to komentarz kodu w linii ~6733,
     nietykany z zasady). 0 pozostałości wielolinijkowych (literał
     "[OK]..." na osobnej linii niż logPrintf/logPrintln). Cudzysłowy
     parzyste we wszystkich 24 edytowanych liniach (sprawdzone
     programowo). Nawiasy klamrowe { }: bez zmian względem stanu przed tą
     partią (ta sama globalna nierównowaga 1829/1842 co w pliku
     wejściowym - nie wprowadzona tą partią, istniała już wcześniej).
     Liczba linii pliku bez zmian (19637) przed dopisaniem tego wpisu
     CHANGELOG.
### v194 (2026-07-30) - LOGS-AI-STEP25: konwersja rodziny Firebase FB-SEND (4) +
     FB-CMD (4) + FB (7) = 15 wywołań logPrintf, wszystkie w
     sendStatusToFirebase()/checkFirebaseCommands(). Zakres: cały grep
     "[FB-SEND]"/"[FB-CMD]"/"[FB]" w wywołaniach log, poza jednym
     historycznym cytatem w komentarzu ~linia 2780 ("[FB-CMD] HTTP=200 |
     t=5726ms" - dawna diagnoza throughput, dokumentacja, nietknięta).

     FB-SEND (4, PUT /aquarium/status): "fbApp nie gotowy - skip" (ERR,
     early return przed wysyłką) i "ERROR: %s" (ERR, fbSendResult.isError())
     podniesione do ERR - obie miały już literalny sufiks " ERROR" w
     oryginalnym stringu (nieużywany jako tag, tylko jako słowo w treści),
     usunięty po ekstrakcji do lvl=. "TIMEOUT" (gałąź else, brak wyniku po
     12s) też ERR z tego samego powodu (literalny sufiks " ERROR", plus
     fbAppReady=false + zamknięcie gniazda w kodzie - realna ścieżka
     awaryjna). "OK" (fbSendResult.isResult()) - INFO, żaden sufiks ERROR
     w oryginale. Błąd biblioteki (fbSendResult.error().message()) trafia
     do nowego pola blad="%s" w cudzysłowie (tekst dynamiczny, może
     zawierać spacje - inaczej niż statyczne msg= widziane dotychczas).

     FB-CMD (4, GET/DELETE /aquarium/cmd): połączona gałąź błędu GET
     (!fbCmdResult.isResult() || isError(), ternary "TIMEOUT" albo
     komunikat biblioteki) - ERR, ta sama logika blad="%s" co FB-SEND.
     "GET OK" - INFO. "DELETE ERROR: %s" - ERR, blad="%s". "DELETE OK" -
     INFO. Wszystkie 4 to realne wyniki operacji sieciowych (sukces/
     porażka), nie zwykłe zdarzenia.

     FB (7, checkFirebaseCommands() - obsługa komend z panelu Firebase):
     "ODRZUCONO: błędny token" - PODNIESIONE do WARN (bezpieczeństwo:
     komenda z niepoprawnym tokenem odrzucona, analogicznie do "ODRZUCONO:
     bledny token" w FB-CFG/v184, ten sam wzorzec treści i decyzji).
     Pozostałe 6 (komenda: %s, Pompa WL/WYL, LED 100%/WYL, Restart za 1s)
     - INFO, świadome akcje operatora z panelu Firebase, goły tag bez
     oryginalnego prefiksu poziomu, konwencja "goły tag = INFO" jak w
     CMD-RUN/v187. Log "komenda: %s" zmieniony na klucz cmd=%s (spójne
     z cmd= wprowadzonym w CMD-RUN/v187).

     Nowe pola: blad="%s" (FB-SEND x2, FB-CMD x2 - komunikat błędu
     biblioteki FirebaseClient, w cudzysłowie bo tekst dynamiczny),
     czas=/rozmiar= (FB-SEND, zamiast t=/size= wplecionych w treść),
     cmd=%s (FB, zamiast "komenda: %s"). Separator "|" usunięty w 4
     wywołaniach FB-SEND/FB-CMD (niepotrzebny w logfmt, ten sam zabieg
     co przy ENERGIA/v185, FB-CFG/v184, SIM-UI/v193).

     Walidacja: linii bez zmian poza tym wpisem CHANGELOG (15 edycji
     in-place, bez zmiany liczby linii kodu). Stary format "[FB-SEND]"/
     "[FB-CMD]"/"[FB]" w wywołaniach logPrintf/logPrintln: 0 wystąpień
     (1 pozostała wzmianka "[FB-CMD]" wyłącznie w komentarzu ~linia 2780,
     historia, nieedytowana). Nowy format: 15 dopasowań "tag=FB-SEND"/
     "tag=FB-CMD"/"tag=FB " w kodzie, zgodnie z oczekiwaniem. Nawiasy
     klamrowe { }: 1827/1840 przed tym wpisem, bez zmian ze zmiany kodu
     (edycje wyłącznie w treści literałów log).
### v193 (2026-07-30) - LOGS-AI-STEP24: konwersja [TG] (4) + SIM-UI (4) = 8
     wywołań. FB-KA (zaproponowany jako kandydat po TG-KA/v192) okazał
     się NIE ISTNIEĆ jako realne wywołanie logu - to wyłącznie wzmianka
     w komentarzu projektowym ~linia 2979 obok TG-KA (odpowiednik dla
     Firebase nigdy nie został zaimplementowany jako osobny log, mimo że
     komentarz architektury go wspomina). Pominięty bez konwersji,
     zastąpiony w tej partii przez [TG]/SIM-UI z wcześniej
     zaproponowanej listy alternatyw.

     [TG] (4, bare "[TG]" - sendMessage()/sendInlineMenu()): POZIOM ERR
     (4/4) - wszystkie cztery to realne błędy przerywające funkcję
     (sendMessage nie potwierdził "ok":true, brak pamięci PSRAM na
     headerBuf/_pl - early return false, wysyłka inlineMenu nieudana) -
     goły tag podniesiony do ERR ze względu na treść/kontekst (każde
     wystąpienie jest w gałęzi błędu if(!ok)/if(!ptr)), analogicznie do
     TG-KA/v192 i wcześniejszych precedensów podnoszenia gołych tagów.

     SIM-UI (4, endpoint /api/sim - symulacja czujnika światła z panelu
     WWW): POZIOM INFO (4/4) - świadome akcje operatora (wł/wył/auto-sin/
     zmiana parametrów symulacji), żadna ścieżka błędu, goły tag bez
     oryginalnego prefiksu poziomu.

     NOWE POLA: lux= (SIM-UI, wartość stała), min=/max=/okres= (SIM-UI,
     zakres i okres trybu auto-sin oraz przy update parametrów) - ta
     sama konwencja "etykieta -> pole" co w poprzednich partiach.
     Separator "|" między polami usunięty (niepotrzebny w logfmt, ten
     sam zabieg co przy ENERGIA/v185, FB-CFG/v184).

     ŚWIADOMIE NIETKNIĘTE: 7 komentarzy kodu oznaczonych "[SIM-UI]"
     (~linie 14370-14403, 14461) - markery ułatwiające przyszłe
     usunięcie całego modułu symulacji ("Aby usunąć symulację: usuń ten
     blok..."), nie są literałami wywołań logu, nieedytowane.

     WALIDACJA: stary format "[TG]"/"[SIM-UI]" w wywołaniach logPrintf/
     logPrintln: 0 wystąpień; nowy format: 8/8 dopasowań tag=TG(msg=)/
     tag=SIM-UI; cudzysłowy per wywołanie: parzyste we wszystkich 8 (po
     2 - otwarcie/domknięcie msg=, wszystkie 8 mają pole msg=); delta
     cudzysłowów całego pliku (przed dodaniem tego wpisu CHANGELOG):
     +16, dokładnie 8 wywołań x 2; nawiasy klamrowe { } bez zmian w
     kodzie.

     POZOSTAŁO: dwie duże rozproszone pule ([WARN] ~32, [OK] ~23,
     [ERR] ~6, w tym odwrócona kolejność w [FB-INIT] [WARN] i
     [WiFi-CFG] [ERR]) oraz ~140 wywołań w dziesiątkach izolowanych
     modułów jeszcze nietkniętych (FB 7, HIST-COMPACT 4, FB-SEND 4,
     FB-CMD 4, TG-CFG 3, STREFA1/STREFA2, SETUP, RAMPA INIT, HEAP-DRIFT,
     FB-INIT, DIAG-FSBREAKDOWN i wiele mniejszych). Filtr JS panelu WWW
     nadal niezweryfikowany.

### v192 (2026-07-30) - LOGS-AI-STEP23: konwersja trzech małych partii naraz
     (na żądanie) - TG-KA (1), POMPA (7), AUTO (6), łącznie 14 wywołań.

     TG-KA (~linia 7090, tgEnsureConnected()): POZIOM PODNIESIONY z
     gołego tagu do WARN - loguje WYŁĄCZNIE gdy keepalive=FAIL (komentarz
     projektowy ~linia 2918: "log tylko gdy keepalive=FAIL, OK jest
     normą, nie informacją"), więc sama obecność loga już sygnalizuje
     anomalię - analogicznie do "ODRZUCONO: bledny token" w FB-CFG/v184
     i "NIEZNANA komenda" w CMD-RUN/v187 (goły tag podniesiony ze
     względu na treść/kontekst, nie automatycznie INFO). Różni się to
     od bliźniaczego DOC-KA/v191 (INFO), który loguje PRZY KAŻDEJ
     wysyłce niezależnie od wyniku.

     POMPA (7, harmonogram pompki): WARN (1) - "Brak NTP - pompa
     wyłączona" (~linia 9394), miał już oryginalny prefiks [WARN],
     zachowany bez zmiany wagi. INFO (6) - wszystkie pozostałe: brak
     danych EEPROM przy pierwszym starcie, załadowano/zapisano
     przedziały (nagłówki list), zmiana stanu WŁ/WYŁ, manual ON/OFF
     override z panelu WWW - wszystkie gołe tagi, zwykły oczekiwany
     przebieg.

     AUTO (6, tryb automatyczny LED po restarcie): INFO (6) - wszystkie
     gołe tagi, żadna ścieżka błędu - potwierdzenia stanu po restarcie
     (miękki start, mid-ramp restart, reset w/poza oknem świecenia) i
     potwierdzenia zapisu z panelu WWW.

     NOWE POLA: stan= (POMPA, WŁ/WYŁ), pwm= (AUTO, zapis LED), liczba=
     (POMPA, liczba przedziałów) - ta sama konwencja "etykieta ->
     pole" co w poprzednich partiach.

     ŚWIADOMIE ZACHOWANY BUG: linia ~13435 (POMPA, zapis z panelu WWW)
     ma w oryginale podwójny backslash "\\n" zamiast "\n" (prawdopodobnie
     pomyłka przy edycji w przeszłości - drukuje dosłowne "\n" zamiast
     nowej linii). Zachowane bez zmian - poprawianie logiki/bugów poza
     zakresem konwersji formatu logów (per zasada minimalnych zmian).

     ŚWIADOMIE NIETKNIĘTE: "[%d] %02d:%02d - %02d:%02d\n" (~linie
     8467/13437, per-slot dump wewnątrz bloków POMPA Załadowano/
     Zapisano) - "[%d]" to dekoracyjny numer porządkowy, nie
     identyfikator modułu/tagu, brak jasnego kandydata na tag= bez
     wymyślania nowego pola od zera; ten sam typ "[%d]" dump istnieje
     też przy liście kanałów LED (~linia 9336, inny moduł) - oba poza
     zakresem tej partii, kandydat do osobnej decyzji.

     WALIDACJA: stary format "[TG-KA]"/"[POMPA]"/"[WARN] [POMPA]"/
     "[AUTO]" w wywołaniach logPrintf/logPrintln: 0 wystąpień; nowy
     format: 14/14 dopasowań tag=TG-KA/POMPA/AUTO; cudzysłowy per
     wywołanie: parzyste we wszystkich 14 (13 z msg= mają po 2, 1 czysto
     key=value - stan=/czas= w POMPA - ma 0); delta cudzysłowów całego
     pliku (przed dodaniem tego wpisu CHANGELOG): +26, dokładnie suma
     cudzysłowów z 14 zmienionych linii; nawiasy klamrowe { } bez zmian
     w kodzie.

     POZOSTAŁO: dwie duże rozproszone pule ([WARN] ~32, [OK] ~23,
     [ERR] ~6, w tym odwrócona kolejność w [FB-INIT] [WARN] i
     [WiFi-CFG] [ERR]) oraz ~150 wywołań w dziesiątkach izolowanych
     modułów jeszcze nietkniętych (FB 7, TG 4, SIM-UI 4, HIST-COMPACT 4,
     FB-SEND 4, FB-CMD 4, TG-CFG 3, STREFA1/STREFA2, SETUP, RAMPA INIT,
     HEAP-DRIFT, FB-INIT, DIAG-FSBREAKDOWN i wiele mniejszych). Filtr
     JS panelu WWW nadal niezweryfikowany.

### v191 (2026-07-30) - LOGS-AI-STEP22: konwersja trójki TG-DOC (3 wywołania,
     ~linie 7530/7565/7633, sendTelegramDocument() - keepalive, brak
     pamięci na bufor, przepustowość streamingu). Pierwsza z zapowiedzianych
     mniejszych partii po pełnym rekonesansie pliku (238 wywołań w starym
     formacie, patrz podsumowanie w rozmowie) - jeden zwarty, jednofunkcyjny
     moduł, niskie ryzyko.

     POZIOM: INFO (2) - DOC-KA (status keepalive, drukowany zawsze,
     OK/FAIL jako wartość, nie jako obecność/brak logu) i DOC-SPEED
     (diagnostyka przepustowości, drukowana przy każdym wysłanym
     dokumencie, nie tylko przy problemie) - oba gołe tagi, zgodnie z
     konwencją "goły tag = INFO". ERR (1) - "Brak pamieci na bufor 4KB"
     (ps_malloc + malloc fallback oba zawiodły - abort wysyłki, realny
     błąd) - miał już oryginalny prefiks [ERR], zachowany bez zmiany
     wagi, em dash usunięty.

     DOC-KA już był niemal czystym logfmt (keepalive=%s fd=%d) - dodano
     wyłącznie lvl=/tag=, ta sama sytuacja co TG-CMD/v188 i
     DNS-STALL/v189.

     WALIDACJA: stary format w wywołaniach logPrintf: 0 wystąpień (2
     pozostałe wzmianki "[DOC-SPEED]" to komentarze dokumentacyjne
     ~linie 1252/1285, opisujące historyczne uzasadnienie dodania tego
     loga - nieedytowane, ten sam typ wyjątku co wcześniejsze banery);
     nowy format: 3/3 dopasowania tag=DOC-KA/TG-DOC/DOC-SPEED;
     cudzysłowy: TG-DOC ma 2 (msg=), DOC-KA/DOC-SPEED mają 0 (czyste
     key=value, bez msg=).

     POZOSTAŁO: z pełnego rekonesansu pliku - trzy duże rozproszone
     pule ([WARN] ~33, [OK] ~23, [ERR] ~6, w tym odwrócona kolejność
     tagów w [FB-INIT] [WARN] i [WiFi-CFG] [ERR]) oraz ~170 wywołań w
     dziesiątkach izolowanych modułów (POMPA 6, AUTO 6, FB 7, TG 4,
     SIM-UI 4, HIST-COMPACT 4, FB-SEND 4, FB-CMD 4, TG-CFG 3, STREFA1/2,
     RAMPA INIT, HEAP-DRIFT, FB-INIT, DIAG-FSBREAKDOWN i wiele
     mniejszych) - kolejne partie po zatwierdzeniu.

### v190 (2026-07-30) - LOGS-AI-STEP21: konwersja TG-SEND-STALL (2 wywołania,
     ~linie 7108/7127, tgPost() - stall przy print(headers)/print(body)
     mimo że writeguard przeszedł). Naturalne domknięcie sąsiedztwa z
     TG-POST/WRITEGUARD (v189) - ten sam mechanizm "domknij sąsiedztwo po
     głównej partii" co TG-CMD po CMD-RUN (v188) i DNS-STALL po
     DNS-ZERO-IP (v189).

     POZIOM: WARN (2/2) - żadne nie zmienione względem oryginału, oba
     miały już jawny prefiks [WARN] w kodzie źródłowym. Czysta konwersja
     strukturalna nawiasów [WARN] [TAG] -> lvl=WARN tag=TAG, bez decyzji
     o zmianie wagi.

     NOWE POLE: site=(headers|body) - już było w oryginale jako
     key=value, zachowane bez zmian.

     STRUKTURA: komentarz w nawiasie "(writeguard OK, ale send trwał
     długo)" przeniesiony do pola msg= (identyczna treść w obu
     wywołaniach, tylko site= się różni). Polskie znaki diakrytyczne w
     treści msg= zamienione na ASCII ("trwał długo" -> "trwal dlugo"),
     spójnie z resztą nowych pól logfmt.

     ŚWIADOMIE NIETKNIĘTE: komentarz dokumentacyjny w nagłówku funkcji
     (~linia 1637) - opisuje mechanizm progu w prozie, nie jest
     wywołaniem logu runtime.

     WALIDACJA: stary format "[WARN] [TG-SEND-STALL]" w wywołaniach
     logPrintf: 0 wystąpień; nowy format: 2/2 dopasowania "lvl=WARN
     tag=TG-SEND-STALL"; cudzysłowy per wywołanie parzyste w obu;
     nawiasy klamrowe { } bez zmian względem kodu.

     POZOSTAŁO: reszta puli [WARN] z rekonesansu v188 (TRYBAUT/
     TLS-MUTEX/RAMP-DBG/HARMONOGRAM/BOOT po 2 każdy, 9 pojedynczych
     tagów, 16 gołych [WARN]) - wymaga świeżego rekonesansu. Bare
     [DS18B20] (~linia 17551) nadal kandydat wyższej wagi do sprawdzenia
     osobno. [OK] (~26) i [ERR] (~7) nadal niesprawdzone. Filtr JS
     panelu WWW nadal niezweryfikowany.

### v189 (2026-07-30) - LOGS-AI-STEP20: konwersja pięciu trójek z puli
     [WARN] (rekonesans v188) - [TG-POST]/[DS18B20]/[DS-STALL]/
     [DNS-ZERO-IP]/[DNS-STALL], po 3 wywołania każdy, 15 łącznie.
     Pierwsza partia po rozbiciu [WARN] na pod-partie per moduł
     (decyzja z v188 - profil ryzyka bliższy [TSL] niż jednorodnym
     CMD-RUN/FS-GUARD).

     TG-POST (3, ~linie 6978/7004/7063, tgPost()): 2x WRITEGUARD
     (gniazdo niezapisywalne - raz przed nagłówkami, raz przed
     body) + 1x OOM (brak PSRAM na bufor odpowiedzi). Nowe pole
     site=(naglowki|body) - reużyta konwencja z DNS-ZERO-IP/
     DNS-STALL, nie wynaleziona od nowa. Po ekstrakcji site= treść
     msg= identyczna dla obu WRITEGUARD - potwierdza poprawność
     podziału. Em dash "—" usunięty z obu (jak w poprzednich
     partiach), OOM: końcowy "!" usunięty.

     DS18B20 (3, ~linie 17520/17532/17544, walidacja odczytu
     temp., throttle 5/30 min): błąd odczytu płyta1/płyta2/woda,
     zachowanie ostatniej dobrej wartości. Nowe pole
     czujnik=(plyta1|plyta2|woda) - ta sama technika co czujnik=
     w TSL/v186 i plyta= w DS-ADDR/v175. DECYZJA: warunkowy sufiks
     tekstowy " (log co 30min)"/"" (zależny od _errCountN>6)
     zamieniony na ZAWSZE OBECNE pole throttle=5min|30min - ten
     sam ternary-warunek w kodzie, tylko dwa inne literały jako
     argument %s (analogicznie do ternary lvl=WARN/INFO w
     DS-ADDR/v175), zamiast czasem-pustego sufiksu. Pole teraz
     zawsze obecne i zawsze grepowalne.

     DS-STALL (3, ~linie 17463/17468/17473, requestTemperatures(),
     pomiar czasu bus1/2/3 >200ms): nowe pole bus=1|2|3
     (analogicznie do plyta=/czujnik=). Dymka możliwych przyczyn
     z bus1 "(pull-up? EMI? kolizja ROM?)" zachowana dosłownie w
     msg= - oryginał miał ją TYLKO przy bus1 (bus2/bus3 nigdy jej
     nie miały), nie dopisano jej do pozostałych dwóch (zero
     nowych słów/interpretacji, zgodnie z dotychczasową zasadą).

     DNS-ZERO-IP (3, ~linie 6836/7322/15033, tgEnsureConnected/
     sendTelegramDocument/fbInitialize): treść identyczna we
     wszystkich trzech miejscach, różni je tylko site= (już było
     w oryginale). Jedyna zmiana: treść po site= przeniesiona do
     msg=, dodano lvl=WARN tag=.

     DNS-STALL (3, ~linie 6842/7328/15039, te same 3 funkcje): już
     czysty key=value w oryginale (ms=/ok=/cnt=/maxMs=, trzecie
     wywołanie ma dodatkowo attempt= - realna różnica z kodu, nie
     wymyślona) - dodano wyłącznie lvl=WARN tag= na początku, bez
     msg= (zbędny przy jasnych kluczach, ta sama zasada co przy
     DS-DIAG pin=/voltage=/devices= w v174).

     UZASADNIENIE POZIOMÓW: wszystkie 15 to lvl=WARN - żadne nie
     zmienione względem oryginału, wszystkie miały już jawny
     prefiks [WARN] (w przeciwieństwie do wcześniejszych partii z
     "goły tag = INFO"; to pierwsza partia złożona wyłącznie z
     już-WARN wywołań, nie re-litygowano żadnego poziomu).

     WALIDACJA: stary format tych 5 tagów w wywołaniach logPrintf/
     logPrintln: 0 wystąpień (2 pozostałe to komentarze
     dokumentacyjne v162/v164 - historia, nieedytowane). Nowy
     format: 15/15 dopasowań (3 na każdy z 5 tagów). Cudzysłowy w
     edytowanym bloku: 9 wywołań z msg= mają po 2 (parzyste), 6
     czysto key=value (3x DS18B20 linia-argumentów, 3x DNS-STALL)
     mają 0 - sprawdzone ręcznie. Delta cudzysłowów całego pliku:
     +18 (9 wywołań x 2), zgodnie z oczekiwaniem. Nawiasy klamrowe
     { }: 1823/1836 bez zmian (poza tym wpisem CHANGELOG). 18
     fizycznie zmienionych linii kodu (15 wywołań, z czego 3x
     DS18B20 to edycje dwuliniowe - format+argumenty).

     ODKRYCIE PRZY WALIDACJI: próba zweryfikowania arytmetyki
     52-15=37 z rekonesansu v188 pokazała, że pozostała populacja
     [WARN] jest bardziej niejednorodna niż prosty grep sugerował:
     część wywołań używa Serial.printf zamiast logPrintf (np.
     [FLASH-STALL] x2, ~linie 10557/10659), część ma ODWRÓCONĄ
     kolejność tagów "[FB-INIT] [WARN]" zamiast "[WARN] [TAG]"
     (~linie 15047/15052), jedno wystąpienie to string odpowiedzi
     HTTP panelu WWW ("[WARN] LittleFS wylaczone...", ~linia
     13416) - NIE log, analogiczny wyjątek do stringów Telegram z
     CMD-RUN/v187 - a komunikat Telegram o temperaturach (~linie
     7555-7562, zmienne zone1/zone2) używa tekstu "[WARN]" jako
     DANEJ w treści wiadomości, nie jako tagu logu - też poza
     zakresem z tego samego powodu co reszta Telegrama. Liczba
     "37" z v188 NIE jest więc wiarygodna bez świeżego,
     ostrożniejszego rekonesansu (Serial.printf + logPrintf/
     logPrintln, z wykluczeniem HTTP-response/Telegram/komentarzy).

     ŚWIADOMIE NIETKNIĘTE: [TG-SEND-STALL] (2, ~linie 6998/7017,
     sąsiaduje bezpośrednio z TG-POST WRITEGUARD, ale to osobna
     para z rekonesansu v188 - inny tag, inna partia). Bare
     [TG-POST] (2, ~linie 7110/7126 - log per-wywołanie +
     disconnect, nie miały [WARN], poza zakresem rekonesansu
     WARN). Bare [DS18B20] (~linia 17551, "10 błędów z rzędu" po
     10. kolejnym błędzie czujnika wody - WARTE UWAGI: treść
     sugeruje wyższą wagę niż throttle 5/30min otaczających WARN,
     ale bez [WARN] w oryginale i poza zakresem tej partii).
     Reszta [WARN]-podmodułów z v188 (TRYBAUT/TLS-MUTEX/RAMP-DBG/
     HARMONOGRAM/BOOT po 2, 9 pojedynczych, 16 gołych [WARN]) -
     wymagają świeżego przeliczenia (patrz ODKRYCIE wyżej) przed
     dalszym planowaniem większych partii. [OK] (~26) i [ERR]
     (~7) - nadal niesprawdzone dokładnie po TSL/CMD-RUN. Filtr JS
     panelu WWW (pkt 2.3 planu) - nadal niezweryfikowany (patrz
     v175).

     PROPONOWANA KOLEJNA PARTIA (do potwierdzenia): [TG-SEND-STALL]
     (2 wywołania, ~linie 6998/7017) - naturalne domknięcie
     sąsiedztwa z TG-POST (ten sam wzorzec co TG-CMD po CMD-RUN w
     v188), potwierdzone wciąż w starym formacie, zero ryzyka.
     Alternatywa: świeży, dokładniejszy rekonesans całej reszty
     [WARN] (uwzględniający Serial.printf/odwróconą kolejność/
     wykluczenie HTTP-Telegram, patrz ODKRYCIE wyżej) przed
     wyborem kolejnej większej pod-partii.

### v188 (2026-07-30) - LOGS-AI-STEP19: konwersja [TG-CMD] (1 wywołanie,
     ~linia 11885, log per-komenda tuż przed switchem CMD-RUN) +
     rekonesans grupy [WARN] w kodzie przed kolejną partią.

     TG-CMD: jedyne wywołanie, "domknięcie sąsiedztwa" z CMD-RUN/v187 -
     już niemal czysty logfmt (cmd=%d freeH=%luB maxAlloc=%luB), tylko
     dodano lvl=INFO tag=TG-CMD na początku, bez zmiany reszty treści.
     lvl=INFO: log wykonuje się przy KAŻDEJ komendzie (diagnostyka
     heap przed wykonaniem), nie tylko przy problemie.

     REKONESANS [WARN]: 52 aktywne wywołania w kodzie (nie 63 jak w
     szacunku z v185/v186 - część już skonwertowana przy okazji innych
     partii: DIAG w v173, DS-DIAG w v174, EEPROM w v182, ENERGIA w
     v185). W PRZECIWIEŃSTWIE do CMD-RUN (jeden zwarty moduł), [WARN]
     okazuje się rozsiane po ~20 RÓŻNYCH podmodułach, każdy z 1-3
     wystąpieniami: [TG-POST]/[DS18B20]/[DS-STALL]/[DNS-ZERO-IP]/
     [DNS-STALL] (po 3), [TRYBAUT]/[TLS-MUTEX]/[TG-SEND-STALL]/
     [RAMP-DBG]/[HARMONOGRAM]/[BOOT] (po 2), oraz 9 pojedynczych
     tagów (WS-CLEANUP-STALL/TG-DNS/SETUP/POMPA/PANEL-AUTO/
     FS-CAPACITY/FLASH-STALL/EVENTS-STALL/ADAPT). Dodatkowo 16 z 52 to
     GOŁE [WARN] bez żadnego drugiego tagu - inny wzorzec niż
     dotychczasowe partie. Ryzyko konwersji WSZYSTKICH 52 na raz w
     jednym przebiegu: wyższe niż CMD-RUN, bo każdy podmoduł ma inny
     kontekst/zmienne/format - bliższe profilowi ryzyka pierwotnie
     przypisanemu [TSL] (mieszanka wielu wzorców) niż jednorodnym
     partiom FS-GUARD/EEPROM/CMD-RUN.

     WALIDACJA: stary format "[TG-CMD]" nie występuje już w żadnym
     aktywnym wywołaniu (1/1 skonwertowane, pozostałe wzmianki
     wyłącznie w komentarzu ~linia 4276 i nagłówku CHANGELOG v187 -
     historia). 1 dopasowanie "tag=TG-CMD" w kodzie. Liczba linii bez
     zmian poza tym wpisem CHANGELOG (edycja in-place).

     PROPONOWANA KOLEJNA PARTIA (do potwierdzenia, na podstawie
     rekonesansu powyżej): zamiast jednego przebiegu przez wszystkie
     52 [WARN], rozbicie na mniejsze pod-partie per faktyczny moduł -
     zaczynając od trójek [TG-POST]/[DS18B20]/[DS-STALL]/
     [DNS-ZERO-IP]/[DNS-STALL] (izolowane, po 3 wywołania, niższe
     ryzyko niż od razu cała pula). Alternatywa: przeliczyć też [OK]
     (rekonesans z v186 dał ~26) i [ERR] (~7) w kodzie na nowo po
     TSL/CMD-RUN, żeby mieć pełny obraz przed wyborem kolejności.
     Filtr JS panelu WWW (pkt 2.3 planu) - nadal niezweryfikowany
     (patrz v175).

### v187 (2026-07-30) - LOGS-AI-STEP18: konwersja [CMD-RUN] (32 wywołania,
     ~linie 11815-11958, dyspozytor komend Telegram w tgTaskFn - jeden
     switch(cmd) z parami start/done dla każdej z 16 komend). Pierwsza
     partia po wyczerpaniu pierwotnie rozpoznanej puli izolowanych
     modułów DIAG-owych (v173-v186); wybrana jako najniższe ryzyko z
     pozostałych dużych grup, bo mimo rozmiaru to JEDEN zwarty blok
     kodu (jeden switch), a nie prefiks rozsiany po całym pliku jak
     [OK]/[ERR]/[WARN] (patrz rekonesans w CHANGELOG v186).

     ZAKRES: 16 par start/done (MENU, LOGS, STATUS, TEMP, ENERGY,
     LIGHT, ADAPT, SENSOR_HIST, SCHEDULE, LED_TOGGLE, TRYB_TOGGLE,
     CLR_CONFIRM, CLR_DO, RESTART_CONFIRM, NOTIF_TOGGLE - każda 2
     wywołania) + RESTART_DO (pojedyncze, bez pary) + default/NIEZNANA
     komenda (pojedyncze) = 15×2 + 1 + 1 = 32.

     NOWE POLA: cmd=<NAZWA_KOMENDY> (dotąd wplecione w treść, np. "MENU
     start" -> teraz cmd=MENU faza=start) i faza=start|done (dotąd
     osobne słowo "start"/"done" w zdaniu) - wprowadzone konsekwentnie
     na wszystkie 15 par od razu, bo wszystkie mają identyczny wzorzec
     "NAZWA start/done" (ta sama konwersja "etykieta -> pole", co przy
     DS-ADDR plyta=/FB-CFG cfg=/TSL czujnik=, tylko na raz na całym
     module, bo wzorzec był już jednolity w każdej gałęzi switcha).
     Klucz czas= zamiast gołego %lums (spójne z wartosc=/plik=/stan=/
     cena= z poprzednich partii).

     UZASADNIENIE POZIOMU (32 wywołania): lvl=INFO (31) - wszystkie
     pary start/done i RESTART_DO to świadome akcje operatora
     zainicjowane komendą Telegram (analogia do OTA/EEPROM/FB-CFG w
     poprzednich partiach - świadoma akcja, nie anomalia). lvl=WARN (1)
     - "NIEZNANA komenda" (default w switchu, wartość cmd spoza
     zdefiniowanych enumów) - PODNIESIONE z gołego [CMD-RUN] do WARN,
     bo treść ("ignoruję") sygnalizuje nieobsłużony/nieoczekiwany
     przypadek, analogicznie do DIAG-23 PWM-OUT-OF-RANGE w v173 i
     "ODRZUCONO: bledny token" w FB-CFG/v184 (goły tag podniesiony ze
     względu na treść, nie automatycznie INFO tylko dlatego że brak
     było nawiasu poziomu w oryginale).

     PRZY OKAZJI: usunięto em dash "—" z RESTART_DO (jedyne wystąpienie
     w tej partii), treść przeniesiona do pola msg=. Telegram-owe
     stringi wysyłane do użytkownika (np. "LED WŁĄCZONE [OK]", "Tryb:
     AUTO [OK]", "Logi zostały wyczyszczone [OK]", emoji 💡🤖🗑️🔔🔕) -
     CELOWO NIETKNIĘTE, to kanał narracyjny czytany przez człowieka na
     Telegramie (per plan pkt 1.2d/1.1d - ten sam zakres wyjęcia co
     wiadomość ds3Msg w DS-DIAG/v174).

     WALIDACJA: stary format "[CMD-RUN]" nie występuje już w żadnym
     aktywnym wywołaniu logPrintf/logPrintln w pliku (32/32
     skonwertowane). Wzmianki "[CMD-RUN]" pozostałe wyłącznie w
     nagłówkach CHANGELOG v181-v186 (historia) i w komentarzu
     dokumentacyjnym v80 (~linia 3178, opisującym pierwotny projekt
     tagu przy jego wprowadzeniu) - żadne nie jest kodem runtime. 32
     dopasowania "tag=CMD-RUN" w kodzie, zgodnie z oczekiwaniem.
     Cudzysłowy per wywołanie: sprawdzone, parzyste we wszystkich 32
     (30 czysto key=value bez msg= mają 0, 2 z msg= - RESTART_DO i
     default/NIEZNANA - mają po 2). Nawiasy klamrowe { }: bez zmian
     względem stanu przed tą partią (jedyne { } w edytowanym bloku to
     case TG_LED_TOGGLE/TG_TRYB_TOGGLE - nietknięte, oraz if/else
     wewnątrz CLR_DO i NOTIF_TOGGLE - nietknięte).

     ŚWIADOMIE NIETKNIĘTE: linia ~11808 ("[TG-CMD] cmd=%d freeH=...
     maxAlloc=..." tuż przed switchem) - INNY tag (TG-CMD, nie
     CMD-RUN), już częściowo logfmt, poza zakresem tej partii
     (skopowanej ściśle po wzorcu grep "[CMD-RUN]"). Reszta prefiksów:
     [WARN] (63), [OK] (~26 po odliczeniu już skonwertowanych w
     DS-ADDR/FS-GUARD/EEPROM/TSL), [ERR] (~7 po tym samym odliczeniu).
     Filtr JS panelu WWW (pkt 2.3 planu) - nadal niezweryfikowany
     (patrz v175).

     PROPONOWANA KOLEJNA PARTIA: [TG-CMD] (1 wywołanie, tuż obok
     CMD-RUN, ~linia 11808) jako natychmiastowy "domknij sąsiedztwo"
     krok - zero ryzyka, ale bardzo mały. Alternatywy: rekonesans
     [WARN] (63, największa pozostała grupa, nie sprawdzona jeszcze w
     kodzie pod kątem izolacji modułów) albo dokładne przeliczenie
     pozostałych czystych [OK]/[ERR] (~26/~7) rozsianych po pliku, by
     ocenić czy da się je zebrać w 1-2 partie mimo braku wspólnego
     modułu.

### v186 (2026-07-30) - LOGS-AI-STEP17: konwersja [TSL] (17 wywołań, ~linie
     9669-13092, moduł czujników światła TSL2561 - pokojowy + nad wodą).
     Ostatnia z pierwotnie rozpoznanej puli izolowanych modułów
     (v174-v185); jedyna oznaczona w v179/step14 jako "najwyższe ryzyko"
     (miesza [WARN]/[OK]/[ERR]/stary numer wersji [v71] razem, jedna
     linia miała DWA osadzone tagi poziomu w jednym wywołaniu).

     ZAKRES: odczyt+EMA+log co 30s dla obu czujników (pokojowy, nad
     wodą) z rozróżnieniem saturacji IR, log "0 lux" (throttle 5 min),
     log próby reinicjalizacji, cały blok auto-reinit I2C (begin() +
     ghost-fix przez getEvent() + finalny fail po 5 próbach), oraz 3
     logi zmiany trybu sensora z panelu WWW (endpoint sensorMode).

     DECYZJA STRUKTURALNA (podwójny zagnieżdżony [WARN]): linia 9669
     miała treść "[WARN] [TSL] ... [WARN] SATURACJA IR..." - prefiks
     poziomu wystąpił dwa razy w jednej linii (raz na początku, raz
     wpleciony w zdanie). Scalone do JEDNEGO pola lvl=WARN + msg=
     "SATURACJA IR, odczyt odrzucony" - sama treść komunikatu bez
     zmian, usunięta wyłącznie zduplikowana etykieta poziomu. Blok
     if/else (saturacja / normalny odczyt) pozostał nietknięty
     strukturalnie - konwertowane osobno per gałąź, bez łączenia w
     jedno wywołanie z ternary (inaczej niż DS-ADDR w v175), bo tu
     różnica między gałęziami to nie tylko lvl= ale i obecność pola
     msg=, więc scalenie dodałoby warunkowy format-string zamiast
     warunkowego argumentu - większe ryzyko niż analogiczny przypadek
     DS-ADDR. Identyczna decyzja zastosowana do bliźniaczego bloku
     TSL-WODA (oryginalnie BEZ osobnego prefiksu poziomu wcale - tylko
     wewnętrzny "[WARN] SATURACJA IR" w gałęzi saturacji, druga gałąź
     całkiem goła): przypisano lvl=WARN / lvl=INFO analogicznie do TSL,
     dla spójności między dwoma bliźniaczymi czujnikami.

     NOWE POLE: czujnik=pokojowy|woda w 6 wywołaniach bloku reinit I2C
     (dotąd rozróżnialne tylko przez tekst "Czujnik pokojowy:"/"Czujnik
     nad wodą:" w treści) - ten sam zabieg co plyta=1|2 przy DS-ADDR
     w v175.

     STARY PREFIKS WERSJI [v71]: 3 wywołania miały "[v71] [TSL]" zamiast
     poziomu (numer wersji firmware użyty jako identyfikator zamiast
     prawdziwego poziomu istotności - inny wzorzec niż [DIAG-N]/[OK]/
     [WARN]/[ERR] widziane dotychczas). Wszystkie 3 to potwierdzenia
     zmiany trybu sensora zainicjowanej z panelu WWW (analogia do
     "goły tag = INFO" z DS-ADDR/FS-GUARD/OTA/EEPROM) - przypisano
     lvl=INFO. Treść "OFFPokój"/"OFFPokój+Woda" (opisująca przejście
     OFF->tryb) uproszczona do ASCII "OFF->Pokoj"/"OFF->Pokoj+Woda" bez
     zmiany znaczenia.

     POZIOMY (17 wywołań): lvl=WARN (4) - saturacja IR x2 (pokojowy +
     woda, odczyt odrzucony ale auto-korygowany następnym cyklem) i
     "zwrócił 0 lux" x2 (throttle 5 min, już oryginalnie WARN u obu
     czujników). lvl=ERR (3) - "sensor NIEAKTYWNY" po ghost-fix
     nieudanym x2 (pokojowy/woda) i "Czujniki niedostępne po 5 próbach"
     (finalny fail, uruchamia throttle 10 min) - wszystkie 3 miały już
     oryginalny prefiks [ERR], zachowane bez zmiany wagi. lvl=INFO (10)
     - odczyt normalny x2, próba reinicjalizacji, reinit OK x2,
     ghost-fix aktywny x2, oraz 3x zmiana trybu z panelu (dawniej goły
     [v71] lub [OK]) - zwykły, oczekiwany przebieg pracy czujników.

     WALIDACJA: stary format "[TSL]"/"[TSL-WODA]"/"[v71] [TSL]" nie
     występuje już w żadnym aktywnym wywołaniu logPrintf/logPrintln w
     pliku (17/17 skonwertowane). Wzmianki "[TSL]" pozostałe wyłącznie
     w nagłówkach CHANGELOG v174-v185 (historia, nieedytowane) - 13
     dopasowań, żadne nie jest kodem. 17 dopasowań "tag=TSL"/
     "tag=TSL-WODA" w kodzie, zgodnie z oczekiwaniem. Cudzysłowy per
     wywołanie: sprawdzone, parzyste we wszystkich 17 (15 z msg= mają
     po 2, 2 czysto key=value - gałęzie INFO odczytu normalnego - mają
     0, bez dodawania zbędnego pola). Liczba linii bez zmian poza tym
     wpisem CHANGELOG (wszystkie 17 edycji log in-place, żadna zmiana
     struktury if/else nietknięta poza scaleniem podwójnego prefiksu
     opisanym wyżej). Nawiasy klamrowe { }: bez zmian względem stanu
     przed tą partią.

     ŚWIADOMIE NIETKNIĘTE: linia ~12090 (komentarz "v101:
     FIX-WDT-CONFIRMED..." wymieniający "TSL-reinit" jako nazwę
     naprawionego problemu) - historia dokumentacji, ten sam typ
     wyjątku co banery/cytaty z poprzednich partii. Linia ~14233
     ("Inicjalizacja czujników TSL2561..." w setup()) - log całkowicie
     bez tagu, poza zakresem tej partii (skopowanej ściśle po wzorcu
     grep tagów [TSL]/[TSL-WODA]/[v71][TSL]) - kandydat do osobnej
     partii "logi bez żadnego tagu", razem z linią ~7832 [ENERGIA]
     odnotowaną w v185. Linia ~18278 (tag=DIAG-15, "TSL ZAMROŻONY") -
     już skonwertowana w v173, inny tag (diagnostyka zamrożenia
     odczytu, nie moduł TSL sam w sobie), poza zakresem.
     To była ostatnia partia z pierwotnie rozpoznanej puli izolowanych
     modułów DIAG/DS-DIAG/DS-ADDR/FS-GUARD/OTA/COREDUMP/STACK/CB/DIR/
     EEPROM/COUNTER/FB-CFG/ENERGIA/TSL. Pozostały już tylko duże grupy
     rozproszone po całym pliku, nie odizolowane w jednym module:
     [WARN] (63), [OK] (33, częściowo już skonwertowane przy okazji
     innych partii - do przeliczenia na nowo), [CMD-RUN] (32), [ERR]
     (18, jw. - część już skonwertowana). Filtr JS panelu WWW (pkt 2.3
     planu) - nadal niezweryfikowany (patrz v175).

### v185 (2026-07-30) - LOGS-AI-STEP16: konwersja [ENERGIA] (11 wywołań,
     ~linia 7844-14483, liczniki zużycia energii - reset tygodniowy/
     miesięczny, zmiana ceny kWh z panelu, zapis/odczyt statystyk
     energy_stats.json). Jedenasta partia z puli poza-DIAG-owych
     prefiksów, wybrana per propozycja z v184 jako niższe ryzyko
     z dwóch pozostałych przeanalizowanych grup ([ENERGIA]/[TSL]).

     ZAKRES: reset licznika tygodniowego/miesięcznego w
     logDailySummary(), zmiana ceny kWh z panelu WWW (endpoint
     /set-kwh-price), potwierdzenie wczytania statystyk po
     synchronizacji NTP, oraz cała funkcja loadEnergyStats() -
     brak pliku (pierwsze uruchomienie), błąd otwarcia pliku, brak
     czasu NTP, stary format pliku bez pola 'yr', diagnostyka LOAD
     (wieloliniowa) i finalne podsumowanie wczytanych wartości.

     POZIOMY: lvl=ERR (2) - "Nie można otworzyć energy_stats.json"
     (realny błąd I/O mimo że plik wg LittleFS.exists() istnieje) i
     "Brak czasu NTP - nie wczytano statystyk" (funkcja wywołana
     przed potwierdzeniem NTP - naruszenie wymaganej kolejności
     wywołań, funkcja zwraca early) - obie ścieżki miały już
     oryginalny prefiks [ERR], zachowane bez zmiany wagi. lvl=WARN
     (2) - "Brak WiFi/NTP - statystyki nie wczytane" i "Plik bez
     pola yr (stary format) - pomijam" - obie miały już oryginalny
     prefiks [WARN], zachowane bez zmiany wagi. lvl=INFO (7) - reset
     tygodniowy/miesięczny, zmiana ceny kWh, "statystyki wczytane po
     NTP", "brak pliku - pierwsze uruchomienie" (oczekiwane przy
     pierwszym boocie, nie błąd), diagnostyka LOAD (dump przy każdym
     wczytaniu, nie sam w sobie problem) i finalne podsumowanie
     wczytanych wartości - wszystkie to zwykły, oczekiwany przebieg.

     PRZY OKAZJI: usunięto separatory "|" między polami key=value w
     2 wywołaniach (LOAD diagnostyka, podsumowanie Wczytano) -
     niepotrzebne w logfmt, gdzie spacja już rozdziela pola. Klucz
     "dziś=" zamieniony na ASCII "dzis=" (spójne z resztą nowych
     kluczy wprowadzonych w poprzednich partiach - wartosc/plik/stan/
     cfg/sloty - żaden nie zawierał polskich znaków diakrytycznych).
     Jednostka "PLN" usunięta z wartości ceny kWh (ta sama decyzja co
     usunięcie jednostki " lux" z Min LUX w v182 - jednostka nie jest
     częścią wartości logfmt), nowy klucz cena=.

     WALIDACJA: stary format "[ENERGIA]" nie występuje już w żadnym
     aktywnym wywołaniu logPrintf/logPrintln w pliku (11/11
     skonwertowane). Wzmianki "[ENERGIA]" pozostałe wyłącznie w
     nagłówkach CHANGELOG v174-v184 (historia, nieedytowane). 11
     dopasowań tag=ENERGIA w kodzie, zgodnie z oczekiwaniem.
     Cudzysłowy per wywołanie: sprawdzone, parzyste we wszystkich 11
     (w tym wywołanie wieloliniowe LOAD - 4 na pierwszej linii + 2 na
     linii kontynuacji, zgodnie z oczekiwaniem). Liczba linii bez
     zmian (18976, wszystkie 11 edycji in-place). Nawiasy klamrowe
     { }: bez zmian względem stanu przed tą partią.

     ŚWIADOMIE NIETKNIĘTE: linia ~7832 (logDailySummary(), tuż przed
     resetem tygodniowym) - "Energia dziś: %.1f Wh | Tydzień: %.1f Wh
     | Miesiąc: %.1f Wh" - log całkowicie bez tagu (ani [ENERGIA] ani
     żaden inny), poza zakresem tej partii bo partia była skopowana
     ściśle po wzorcu grep "[ENERGIA]"; kandydat do osobnej,
     niewielkiej partii "logi bez żadnego tagu" albo do konwersji
     przy okazji przyszłej zmiany w tej funkcji (per plan pkt 2.2
     krok 4). Linia ~14112 (emoji "🌇 Zachód słońca..." w tej samej
     funkcji co statystyki NTP) - inny moduł/tag, poza zakresem.
     Reszta prefiksów: [WARN] (63), [OK] (33), [CMD-RUN] (32),
     [ERR] (18), [TSL] (14) - kolejne partie po zatwierdzeniu tej.
     Filtr JS panelu WWW (pkt 2.3 planu) - nadal niezweryfikowany
     (patrz v175).

### v184 (2026-07-30) - LOGS-AI-STEP15: konwersja [FB-CFG] (11 wywołań,
     ~linia 15132-15448, checkFirebaseConfig() - pobranie i
     zastosowanie konfiguracji z Firebase RTDB). Dziesiąta partia
     z puli poza-DIAG-owych prefiksów, wybrana per propozycja z v183
     jako najniższe ryzyko z trzech pozostałych większych grup
     ([FB-CFG]/[ENERGIA]/[TSL]).

     ZAKRES: GET z Firebase RTDB (sukces/timeout/błąd), walidacja
     tokenu, identyfikacja typu configu, oraz 5 sekcji zapisu:
     harmonogram, adaptacja czujników, Min LUX, parametry systemu
     (cena kWh/rampa/EMA/interwał czujnika), harmonogram pompki,
     Telegram (token/chatId/enabled) + finalne potwierdzenie zapisu
     do EEPROM.

     POZIOMY: lvl=ERR (1) - GET z Firebase TIMEOUT lub błąd
     transportu; jedyna ścieżka w tej partii, która faktycznie
     przerywa przetwarzanie i wymusza reinit (fbAppReady=false,
     fbSSLClient.stop(), NET_FAIL()) - analogia do DIAG-17-RESTART/
     DS-DIAG FAIL z wcześniejszych partii. lvl=WARN (1) - odrzucony
     request z błędnym tokenem: walidacja nieudana, żądanie
     odrzucone bez zastosowania zmian - mimo że oryginał nie miał
     prefiksu poziomu, treść ("ODRZUCONO") sygnalizuje odrzuconą
     próbę zapisu, więc podniesione do WARN, analogicznie do
     DIAG-23 PWM-OUT-OF-RANGE w v173 (goły tag podniesiony ze
     względu na treść, nie automatycznie INFO). lvl=INFO (9) -
     reszta: GET OK, typ configu odebrany, oraz 6 potwierdzeń
     zastosowanych zmian (schedule/adapt/minlux/params/pump/
     telegram) + finalne "konfiguracja zastosowana i zapisana" -
     wszystkie to oczekiwany przebieg synchronizacji configu
     zainicjowanej z panelu WWW, nie anomalie.

     PRZY OKAZJI: dodano wspólny klucz cfg= dla wszystkich 6 sekcji
     configu (schedule/adapt/minlux/params/pump/telegram) w miejsce
     dotychczasowego zapisu "nazwa: klucz=wartość" - to ta sama
     konwersja "etykieta przed dwukropkiem → klucz=wartość" co przy
     DS-ADDR (plyta=) i EEPROM (Czujniki→msg=), tylko konsekwentnie
     zastosowana na cały moduł od razu (5 sekcji + typ configu).
     Nowy klucz sloty= (liczba przedziałów pompy zapisanych) w
     miejsce zdania "%d przedziałów zapisanych". Usunięto nawiasy
     wokół present= w logu telegram (już czyste key=value, nawiasy
     były tylko dekoracją interpunkcyjną).

     WALIDACJA: stary format "[FB-CFG]" nie występuje już w żadnym
     aktywnym wywołaniu logPrintf/logPrintln w pliku (11/11
     skonwertowane). Wzmianki "[FB-CFG]" pozostałe wyłącznie w
     nagłówkach CHANGELOG v174-v183 (historia, nieedytowane) i w
     jednym cytacie historycznym w CHANGELOG v39/v115 (linia ~1971,
     dokumentuje ówczesny format logu przy opisie buga - ten sam typ
     wyjątku co banery/cytaty z poprzednich partii, nieedytowane).
     11 dopasowań tag=FB-CFG w kodzie, zgodnie z oczekiwaniem.
     Cudzysłowy per wywołanie: sprawdzone, parzyste we wszystkich 11
     (4 wywołania z msg="..." mają po 2 pary, 7 czysto key=value bez
     msg= ma po 1 parze otwarcie/domknięcie samego literału).
     Liczba linii bez zmian (18915, wszystkie 11 edycji in-place).
     Nawiasy klamrowe { }: bez zmian względem stanu przed tą partią.

     ŚWIADOMIE NIETKNIĘTE: reszta prefiksów [WARN] (63), [OK] (33),
     [CMD-RUN] (32), [ERR] (18), [TSL] (14), [ENERGIA] (11) -
     kolejne partie po zatwierdzeniu tej. Filtr JS panelu WWW
     (pkt 2.3 planu) - nadal niezweryfikowany (patrz v175).

### v183 (2026-07-30) - LOGS-AI-STEP14: konwersja [COUNTER] (2 wywołania,
     ~linia 10428/10436, licznik zmian nastaw EEPROM - bump przy
     zmianie, odczyt przy starcie). Dziewiąta partia z puli
     poza-DIAG-owych prefiksów, wybrana per propozycja z v182 -
     najmniejsza pozostała partia, zerowe ryzyko regresji, oba
     wywołania już `if (Komentarze)`.

     ZAKRES: bumpChangeCounter() - zapis inkrementowanego licznika do
     EEPROM po każdej realnej zmianie nastaw (power/tryb/jasność/
     harmonogram); loadChangeCounterFromEEPROM() - odczyt licznika
     przy starcie urządzenia.

     POZIOMY: lvl=INFO (2) - oba gołe [COUNTER] bez oryginalnego
     prefiksu poziomu, zgodnie z konwencją "goły tag = INFO" z
     poprzednich partii (DS-ADDR v175, FS-GUARD v176, OTA v177,
     EEPROM v182). Zwykłe potwierdzenia zapisu/odczytu licznika,
     żadna ścieżka błędu w żadnym z dwóch wywołań.

     PRZY OKAZJI: ujednolicony klucz wartosc= dla obu wywołań (ta sama
     zmienna localChangeCounter przy zapisie i odczycie) w miejsce
     dwóch osobnych opisów słownych ("Licznik nastaw:" / "Wczytano
     licznik z EEPROM:") - ten sam zabieg co target= przy Min LUX
     w v182.

     WALIDACJA: stary format "[COUNTER]" nie występuje już w żadnym
     aktywnym wywołaniu logPrintf/logPrintln w pliku (2/2
     skonwertowane). Wzmianki "[COUNTER]" pozostałe wyłącznie w
     nagłówku CHANGELOG v182 (historia, nieedytowane). 2 dopasowania
     tag=COUNTER w kodzie, zgodnie z oczekiwaniem. Cudzysłowy per
     wywołanie: sprawdzone, parzyste w obu (msg="..."). Liczba linii
     bez zmian poza samym wpisem CHANGELOG (2 edycje in-place).

     POZOSTAŁO: reszta prefiksów z listy v174-v182 ([WARN] 63,
     [OK] 33, [CMD-RUN] 32, [ERR] 18, [TSL] 14, [FB-CFG] 11,
     [ENERGIA] 11) - kolejne partie po zatwierdzeniu tej. Filtr JS
     panelu WWW (pkt 2.3 planu) - nadal niezweryfikowany (patrz v175).

### v182 (2026-07-30) - LOGS-AI-STEP13: konwersja [EEPROM] (11 wywołań,
     ~linia 7889-10454, zapis/odczyt konfiguracji: harmonogram dnia,
     Min LUX, tryb logowania, Power/Tryb, parametry czujników).
     Ósma partia z puli poza-DIAG-owych prefiksów, wybrana per
     propozycja z v181.

     ZAKRES: świeży chip (reset harmonogramu do domyślnych), zapis/
     odczyt Min LUX (2), dane adaptacji uszkodzone (reset do
     domyślnych), zapis/odczyt trybu logowania (3 - w tym gałąź
     fallback gdy zapisana wartość poza zakresem), zapis/odczyt
     Power+Tryb (2), zapis/odczyt parametrów czujników światła (2).

     POZIOMY: lvl=WARN (2) - świeży chip (harmonogram zresetowany do
     wartości domyślnych, wymaga konfiguracji przez panel) i dane
     adaptacji uszkodzone (walidacja NaN/zakres nieudana, reset do
     domyślnych) - obie ścieżki już miały oryginalny prefiks [WARN],
     zachowane bez zmian wagi. lvl=INFO (9) - wszystkie pozostałe to
     zwykłe potwierdzenia zapisu/odczytu konfiguracji, bez oryginalnego
     prefiksu poziomu (gołe [EEPROM]), zgodnie z konwencją z
     poprzednich partii (DS-ADDR, FS-GUARD, OTA) że goły tag bez
     poziomu = INFO.

     PRZY OKAZJI: 2 wywołania miały już strukturę zbliżoną do logfmt
     (Power=%s,Tryb=%s / Tryb=%s,Aktywne=%s,Uczenie=%s) - tylko
     zmienione na małe litery (power=/tryb=/aktywne=/uczenie=) dla
     spójności z resztą pliku, bez dodawania msg= (klucze już
     samoopisujące). Ujednolicono target=%.0f między wersją zapisu i
     odczytu Min LUX - odczyt miał dodatkowo wplecioną jednostkę
     " lux" bezpośrednio w formacie, usunięta (jednostka nie jest
     częścią wartości logfmt). Żadnych nowych słów/interpretacji nie
     dodano do treści komunikatów poza tym - konwersja czysto
     strukturalna (nawiasy → lvl=/tag=, dwukropek-jako-separator →
     klucz=wartość albo msg=, "!" → przecinek), zgodnie z preferencją
     minimalnych zmian.

     WALIDACJA: stary format "[EEPROM]" nie występuje już w żadnym
     aktywnym wywołaniu logPrintf/logPrintln w pliku (11/11
     skonwertowane). Wzmianki "[EEPROM]" pozostałe wyłącznie w
     nagłówkach CHANGELOG v174-v181 (listy pozostałych prefiksów,
     historia, nieedytowane). 11 dopasowań tag=EEPROM w kodzie,
     zgodnie z oczekiwaniem. Cudzysłowy per wywołanie: sprawdzone,
     parzyste we wszystkich 10 wywołaniach z msg= (jedno wywołanie,
     power=/tryb=, nie ma pola msg= i celowo nie ma cudzysłowów -
     już samo w sobie key=value). Liczba linii bez zmian poza samym
     wpisem CHANGELOG (11 edycji in-place).

     ŚWIADOMIE NIETKNIĘTE: 2 wywołania [COUNTER] w bezpośrednim
     sąsiedztwie (~linia 10372/10380, licznik zmian nastaw) - inny
     tag, mała odrębna grupa poza zakresem tej partii.

     POZOSTAŁO: reszta prefiksów z listy v174-v181 ([WARN] 63, [OK] 33,
     [CMD-RUN] 32, [ERR] 18, [TSL] 14, [FB-CFG] 11, [ENERGIA] 11) i
     mniejsze grupy (m.in. [COUNTER] 2) - kolejne partie po
     zatwierdzeniu tej. Filtr JS panelu WWW (pkt 2.3 planu) - nadal
     niezweryfikowany (patrz v175).

### v181 (2026-07-30) - LOGS-AI-STEP12: konwersja [DIR] (3 wywołania,
     ~linia 12049/12896/13451). Siódma partia z puli poza-DIAG-owych
     prefiksów, wybrana per propozycja z v180 po korekcie zakresu
     (pierwotny szacunek z v179 mówił o 9 wywołaniach - w praktyce
     6 z nich poszło już razem z FS-GUARD w v176, co v179 sugerowało
     sprawdzić).

     ZAKRES: log startu inicjalizacji LittleFS w setup(), potwierdzenie
     zmiany stanu LittleFS z panelu WWW (API /api/log-settings), log
     usunięcia pliku przez panel WWW (endpoint zarządzania plikami).
     Wszystkie 3 to proste logi bez warunkowych poziomów ani makr -
     najniższe ryzyko spośród dotychczasowych partii.

     POZIOMY: lvl=INFO (3) - wszystkie trzy to potwierdzenia zwykłych,
     oczekiwanych zdarzeń (start inicjalizacji, świadoma zmiana
     ustawienia przez operatora z panelu, świadome usunięcie pliku
     przez operatora) - żadna ścieżka błędu nie była dotąd objęta
     tagiem [DIR] (błędy w tym samym bloku setup() mają już osobne
     prefiksy [WARN]/[ERR], poza zakresem tej partii; endpoint
     usuwania pliku nie logował dotąd ścieżki błędu przy request->send
     500 - bez zmian, poza zakresem).

     PRZY OKAZJI: zagnieżdżone prefiksy "[DIR] [PANEL]" i "[DIR] [FS]"
     (ten sam wzorzec podwójnego prefiksu co [FS-GUARD] z [DIR] w v176
     i [INFO] [STACK] w v179) scalone do jednego tag=DIR - kontekst
     źródła (panel vs setup) przeniesiony do treści msg= zamiast
     osobnego pola, bo to nie jest odrębny poziom ważności ani
     grepowalny podtyp zdarzenia jak FS-GUARD-CRITICAL, tylko opis.
     Nowe pole plik= dla nazwy pliku (zamiast wplecionej w zdanie po
     dwukropku).

     ŚWIADOMIE NIETKNIĘTE: komentarz nagłówka sekcji w setup()
     "// [DIR] LITTLEFS - SYSTEM PLIKÓW" (linia ~12047) - to komentarz
     strukturalny dzielący kod na sekcje, nie treść logu runtime, poza
     zakresem zmian (ten sam typ wyjątku co komentarze [FIX-vXXX] w
     całym pliku). 2 wzmianki "[DIR]" w nagłówkach CHANGELOG v175/v176
     (historia scalenia z FS-GUARD) - append-only, nieedytowane.

     WALIDACJA: stary format "[DIR]" nie występuje już w żadnym
     aktywnym wywołaniu logPrintf/logPrintln w pliku (3 pozostałe
     wystąpienia opisane wyżej - żadne nie jest kodem runtime).
     3 dopasowania tag=DIR, zgodnie z oczekiwaniem. Cudzysłowy per
     wywołanie: sprawdzone, parzyste we wszystkich 3. Liczba linii bez
     zmian poza samym wpisem CHANGELOG (3 edycje in-place).

     POZOSTAŁO: reszta prefiksów z listy v174-v180 ([WARN] 63, [OK] 33,
     [CMD-RUN] 32, [ERR] 18, [TSL] 14, [FB-CFG] 11, [ENERGIA] 11,
     [EEPROM] 11) - kolejne partie po zatwierdzeniu tej. Filtr JS
     panelu WWW (pkt 2.3 planu) - nadal niezweryfikowany (patrz v175).

### v180 (2026-07-30) - LOGS-AI-STEP11: konwersja [CB] (6 wywołań, ~linia
     5103-11374, circuit breaker sieci FB/TG z v131). Szósta partia
     z puli poza-DIAG-owych prefiksów, wybrana per propozycja z v179.

     ZAKRES: Circuit OPEN (próg streak przekroczony, backoff+cooldown
     ustawiony), Circuit RESET (streak wyzerowany po sukcesie),
     Half-open (próba pojedynczej operacji FB-CMD po cooldown),
     Circuit open/skip (pomijanie FB+TG podczas cooldown), oraz 2x
     ITER-BUDGET przekroczony (odłożenie FB na kolejną iterację / skip
     TG-POLL).

     POZIOMY: lvl=WARN (3) - Circuit OPEN (realna degradacja, próg
     NET_FAIL_THRESHOLD przekroczony, ten sam wzorzec progu co [STACK]
     w v179) oraz 2x ITER-BUDGET przekroczony (iteracja trwa dłużej
     niż budżet, coś odkładane na później - realne ostrzeżenie o
     obciążeniu, nie zwykły stan). lvl=INFO (3) - Circuit RESET
     (powrót do normy), Half-open (próba odzyskania, oczekiwany krok
     mechanizmu, nie problem), Circuit open/skip (konsekwencja stanu
     już zasygnalizowanego WARN-em przy otwarciu - nie nowa anomalia,
     tylko potwierdzenie że system czeka zgodnie z planem).

     PRZY OKAZJI: usunięto 5 różnych emoji w 6 wystąpieniach (🔴 ✅ ⚡
     ⏳ ⚠️x2, ten ostatni wielobajtowy z selektorem wariantu
     \xef\xb8\x8f) i 2x em dash "—" (zastąpione
     przecinkiem w treści msg=) - zgodnie z pkt 1.2a/1.3 planu.
     "streak byl=" (luźny zapis ze spacją w kluczu) zmapowany na
     streak_byl= (snake_case - spacja w kluczu łamałaby logfmt). Dla
     obu ITER-BUDGET wprowadzono nowy klucz czas= zamiast wplatania
     surowej liczby w nawiasie w treść zdania.

     ŚWIADOMIE NIETKNIĘTE: linia ~573 - cytat historyczny w komentarzu
     CHANGELOG (dawna diagnoza DIAG-TGDOC-THROUGHPUT) zawiera dosłowny
     fragment starego logu urządzenia "[CB] ITER-BUDGET: skip TG-POLL"
     - zapis tego, co faktycznie wydrukowało urządzenie w chwili tamtej
     diagnozy, nie żywy kod; ten sam typ wyjątku co banery poprzednich
     kroków (v122 STACK). Zmiana tego cytatu zafałszowałaby historię.

     WALIDACJA: stary format "[CB]" nie występuje już w żadnym aktywnym
     wywołaniu logPrintf w pliku (4 pozostałe wystąpienia to 3 wzmianki
     w nagłówkach CHANGELOG v177-v179 listujących pozostałe prefiksy +
     1 cytat historyczny opisany wyżej - żadne nie jest kodem).
     6 dopasowań tag=CB, zgodnie z oczekiwaniem. Cudzysłowy per
     wywołanie: sprawdzone, parzyste we wszystkich 6 (po 2 na linię,
     otwarcie+domknięcie msg=). Liczba linii bez zmian poza samym
     wpisem CHANGELOG (6 edycji in-place, żadna nie zmieniła układu
     wieloliniowych wywołań ani kontynuacji backslash w makrach
     NET_FAIL/NET_SUCCESS).

     POZOSTAŁO: reszta prefiksów z listy v174-v179 ([WARN] 63, [OK] 33,
     [CMD-RUN] 32, [ERR] 18, [TSL] 14, [FB-CFG] 11, [ENERGIA] 11,
     [EEPROM] 11) i mniejsze grupy - kolejne partie po zatwierdzeniu
     tej. Filtr JS panelu WWW (pkt 2.3 planu) - nadal niezweryfikowany
     (patrz v175).

### v179 (2026-07-30) - LOGS-AI-STEP10: konwersja [STACK] (4 wywołania,
     ~linia 18088-18121, monitoring HWM stosu tgTask i loop()).
     Piąta partia z puli poza-DIAG-owych prefiksów, wybrana per
     propozycja z v178.

     ZAKRES: nowe minimum HWM tgStack_free, alarm tgStack_free<3000B,
     nowe minimum HWM loopStack_free, alarm loopStack_free<2000B.

     DECYZJA WS. ZAGNIEŻDŻONEGO PREFIKSU: 2 z 4 wywołań miały
     podwójny prefiks "[INFO] [STACK]" (poziom + tag razem, ten sam
     wzorzec co [FS-GUARD] miało z [OK]/[ERR]/[WARN]/[DIR] w v176) -
     scalone do jednego pola lvl=, tag=STACK zostaje jednolity dla
     całego modułu.

     POZIOMY: lvl=INFO (2) - nowe minimum HWM (jednorazowy log
     informacyjny, wyjaśnienie w treści że HWM działa w jedną stronę
     i spadek nie oznacza wycieku - zgodne z oryginalnym [INFO]).
     lvl=WARN (2) - przekroczenie progu (3000B tgTask / 2000B
     loop()) - realne ryzyko stack overflow, dotychczas bez
     prefiksu poziomu wcale (goły [STACK]), teraz jawnie WARN.

     Klucze: poprzednie=/nowe=/delta= dla zdarzeń HWM (zamiast dwóch
     surowych liczb wplecionych w zdanie), tgStack_free=/prog= i
     loopStack_free=/prog= dla alarmów progowych.

     WALIDACJA: stary format "[STACK]" nie występuje już w żadnym
     aktywnym wywołaniu logPrintf/logPrintln w pliku (1 pozostałe
     wystąpienie to przykład w komentarzu CHANGELOG v122 - historia
     dokumentacji, ten sam typ wyjątku co banery wcześniejszych
     kroków). 4 dopasowania tag=STACK. Cudzysłowy per wywołanie
     parzyste we wszystkich 4.

     POZOSTAŁO: reszta prefiksów z listy v174-v178 ([WARN] 63,
     [OK] 33, [CMD-RUN] 32, [ERR] 18, [TSL] 14, [FB-CFG] 11,
     [ENERGIA] 11, [EEPROM] 11, [CB] 6 itd.) - kolejne partie po
     zatwierdzeniu tej. Filtr JS panelu WWW (pkt 2.3) - nadal
     niezweryfikowany (patrz v175).

### v178 (2026-07-30) - LOGS-AI-STEP9: konwersja [COREDUMP] (6 wywołań,
     ~linia 12055-12082, odczyt partycji coredump po restarcie
     PANIC). Czwarta partia z puli poza-DIAG-owych prefiksów, wybrana
     per propozycja z v177.

     ZAKRES: Task/PC/backtrace_depth summary, hexdump backtrace
     (budowany przez String, nie logPrintf - drugi tego typu
     przypadek po DS-DIAG FAIL z v174), hint dekodowania adresów
     przez addr2line, 2 ścieżki błędu odczytu partycji, oraz log
     "funkcja niedostępna w tym sdkconfig" (branch #else).

     POZIOMY: lvl=ERR (4) - Task/PC/backtrace summary, sam hexdump
     backtrace (kontynuacja tego samego zdarzenia), "obraz OK ale
     odczyt summary zawiódł", "obraz uszkodzony" - wszystkie 4
     raportują dane o FAKTYCZNIE zaistniałym crashu (PANIC), więc
     ERR, nie INFO, mimo że same w sobie nie "psują się w locie".
     lvl=INFO (2) - hint dekodowania (to porada narzędziowa, nie
     dana o stanie systemu) i log "CONFIG niedostępne, pomijam
     odczyt" (normalna konfiguracja builda, nie problem).

     DECYZJA STRUKTURALNA: hexdump backtrace budowany przez String
     (nie logPrintf) dostał pełny nagłówek "lvl=ERR tag=COREDUMP
     msg=\"backtrace\" adresy=\"..."" wewnątrz literału inicjującego
     _cdBt (zamiast dawnego "   [COREDUMP] backtrace:") + domykający
     cudzysłów doklejony PO pętli, przed logPrintln - jedyna zmiana
     struktury kodu w tej partii (pętla zbierająca adresy zostaje
     nietknięta).

     ŚWIADOMIE NIETKNIĘTE: linia ~11967 (tag=WARN) zawiera tekstową
     referencję "[COREDUMP] kilka linii niżej" - poza zakresem tej
     partii (inny tag, wejdzie do puli [WARN]). Referencja stanie
     się nieco niespójna (wskazuje na stary zapis tagu) do czasu
     konwersji [WARN] - odnotowane do poprawienia przy tamtej
     partii.

     WALIDACJA: stary format "[COREDUMP]" nie występuje już w żadnym
     aktywnym wywołaniu logPrintf/logPrintln/String (6/6 skonwertowane).
     6 dopasowań tag=COREDUMP. Cudzysłowy per wywołanie sprawdzone
     ręcznie, parzyste we wszystkich, w tym w String _cdBt (otwarcie
     w literale inicjującym, domknięcie w osobnej linii += "\"").

     POZOSTAŁO: reszta prefiksów z listy v174-v177 ([WARN] 63,
     [OK] 33, [CMD-RUN] 32, [ERR] 18, [TSL] 14, [FB-CFG] 11,
     [ENERGIA] 11, [EEPROM] 11, [STACK] 5, [CB] 6 itd.) - kolejne
     partie po zatwierdzeniu tej. Filtr JS panelu WWW (pkt 2.3) -
     nadal niezweryfikowany (patrz v175).

### v177 (2026-07-30) - LOGS-AI-STEP8: konwersja [OTA] (5 wywołań,
     ~linia 13654-13690, moduł ArduinoOTA). Trzecia partia z puli
     poza-DIAG-owych prefiksów, wybrana per propozycja z v176 -
     mały, odizolowany moduł bez zależności z innymi tagami.

     ZAKRES: onStart (rozpoczęcie uploadu), onEnd (koniec uploadu),
     onProgress (postęp co 20%), onError (błąd uploadu), oraz log
     gotowości po ArduinoOTA.begin() w setup().

     POZIOMY: lvl=INFO (4) - start/koniec/postęp uploadu i log
     gotowości to normalny, oczekiwany przebieg (upload OTA jest
     świadomą akcją operatora, nie anomalią). lvl=ERR (1) - onError,
     jedyna ścieżka faktycznego niepowodzenia (auth/begin/connect/
     receive/end failed).

     PRZY OKAZJI: usunięto emoji "▶" i em dash "—" z 2 linii
     (onStart, log gotowości) - zanieczyszczały znaki spoza ASCII/
     polskich (pkt 1.2a/1.3 planu). Zmienna lokalna "msg" (opis
     błędu) zmapowana na klucz opis= (nie msg=), żeby uniknąć
     dwuznaczności między nazwą zmiennej w kodzie a kluczem logfmt.

     WALIDACJA: liczba linii bez zmian (edycje in-place, 2 wywołania
     wieloliniowe - onProgress i log gotowości - zachowały swój
     układ). Stary format "[OTA]" nie występuje już w żadnym
     aktywnym wywołaniu logPrintf/logPrintln w pliku. 5 dopasowań
     tag=OTA, cudzysłowy per wywołanie parzyste.

     POZOSTAŁO: reszta prefiksów z listy v174/v175/v176 ([WARN] 63,
     [OK] 33, [CMD-RUN] 32, [ERR] 18, [TSL] 14, [FB-CFG] 11,
     [ENERGIA] 11, [EEPROM] 11, [COREDUMP] 6, [STACK] 5, [CB] 6 itd.)
     - kolejne partie po zatwierdzeniu tej. Filtr JS panelu WWW
     (pkt 2.3) - nadal niezweryfikowany (patrz v175).

### v176 (2026-07-30) - LOGS-AI-STEP7: konwersja [FS-GUARD] (14 wywołań +
     1 podprefiks [FS-GUARD-CRITICAL], ~linia 10984-11104 + 18544).
     Druga partia z puli poza-DIAG-owych prefiksów, wybrana per
     propozycja z v175 (moduł już miał mieszane [OK]/[ERR]/[WARN]/
     [DIR] razem z [FS-GUARD] - dobry kandydat do ujednolicenia
     w jednym przebiegu).

     ZAKRES: cała ścieżka utrzymania FS - próba wysyłki logów na
     Telegram przed czyszczeniem, fallback rotacji log_b->log_a,
     przycinanie history_old.csv i history.csv, podsumowanie po
     przycinaniu, marker krytyczny gdy dysk nadal pełny, oraz
     wyzwalacz wejścia w tryb przycinania (loop(), linia ~18544).

     POZIOMY (metoda z v173/v175 - kontekst funkcji, nie sama
     obecność prefiksu): lvl=INFO (9) - ścieżki sukcesu i "pominięto
     bo poniżej progu" (wysyłka TG+usunięcie plików, rotacja log_b,
     usunięcie/pominięcie history_old.csv, pominięcie/przycięcie
     history.csv, podsumowanie końcowe - to zwykły przebieg
     utrzymania, nie problem). lvl=WARN (2) - wejście w tryb
     przycinania (zdarzenie oczekiwane, ale sygnalizujące zbliżanie
     się do limitu) oraz nieudana wysyłka TG przed fallbackiem do
     starej rotacji (nie błąd krytyczny - system ma zaplanowany
     fallback). lvl=ERR (4, tag=FS-GUARD) - brak dostępu do
     logMutex, nieudane usunięcie plików po wysyłce TG, nieudana
     rotacja log_b->log_a, nieudany zapis history_tmp.csv - każdy
     to realne niepowodzenie operacji FS bez fallbacku dalej w tej
     samej gałęzi. lvl=ERR (1, tag=FS-GUARD-CRITICAL, tag zostaje
     odrębny jak w v173 dla DIAG-17-FRAG) - dysk nadal ≥97% pełny
     PO wszystkich 3 krokach czyszczenia, czyli plateau tuż przed
     ENOSPC; zachowano osobny tag zamiast złączenia z tag=FS-GUARD,
     żeby zostać łatwo grepowalnym jak dotąd.

     Klucze ujednolicone z resztą pliku: odzyskane=/rozmiar=/
     uzyte=/limit=/prog=/procent= w miejscu dotychczasowych
     wartości wplecionych w zdanie; remove=/rename= (już były
     zmienne, tylko przeniesione z nawiasu do key=value).

     WALIDACJA: liczba linii bez zmian (18551 -> 18551, wszystkie
     edycje in-place). Stary format "[FS-GUARD]" nie występuje już
     w żadnym aktywnym wywołaniu logPrintf/logPrintln w pliku (15
     dopasowań tag=FS-GUARD* - 14 zwykłych + 1 CRITICAL, zgodnie
     z oczekiwaniem). Cudzysłowy per wywołanie sprawdzone ręcznie -
     parzyste we wszystkich 15.

     POZOSTAŁO: reszta prefiksów z listy v174/v175 ([WARN] 63,
     [OK] 33, [CMD-RUN] 32, [ERR] 18, [TSL] 14, [FB-CFG] 11,
     [ENERGIA] 11, [EEPROM] 11 itd.) - kolejne partie po
     zatwierdzeniu tej. Filtr JS panelu WWW (pkt 2.3) - nadal
     niezweryfikowany (patrz v175).

### v175 (2026-07-30) - LOGS-AI-STEP6: konwersja [DS-ADDR] (5 wywołań,
     dump ROM adresów Płyta1/Płyta2 przy starcie, ~linia 11681-11694).
     Pierwsza "partia" z puli poza-DIAG-owych prefiksów odnotowanej
     jako POZOSTAŁO w v174 - robione stopniowo, partiami do
     zatwierdzenia, nie hurtowo (plan pkt 2.2 krok 4).

     ZAKRES: Płyta1 ROM (sukces + BRAK), Płyta2 ROM (sukces + BRAK),
     werdykt zgodności obu adresów (kolizja 1-Wire czy różne).

     POZIOMY: lvl=INFO (4) - odczyt ROM per płyta, sukces i BRAK
     traktowane tak samo jak w DS-DIAG/v174 (odczyt jednorazowy przy
     starcie, bez retry/alertu - nie mylić z finalnym FAIL z v174,
     które ma inną wagę bo uruchamia alert TG). lvl=WARN (1, warunkowe
     przez ternary w tym samym wywołaniu) - TYLKO gdy adresy
     identyczne (realna kolizja 1-Wire, potwierdzony problem
     sprzętowy); lvl=INFO gdy różne (stan prawidłowy).

     Klucze: plyta=1|2 (nowe pole, nie było w oryginale - rozróżnia
     dwa niemal identyczne wywołania bez zgadywania z treści),
     rom=<HEX>|BRAK, rom1_vs_rom2=IDENTYCZNE|rozne + msg= z werdyktem.
     Format spójny z tag=DS-ADDR (nie zmieniano tagu) i z konwencją
     pin=/voltage=/rom= z DS-DIAG.

     WALIDACJA: linie 18511 -> 18513 (+2, wyłącznie z rozbicia
     ternary werdyktu collision na osobną linię argumentu - reszta
     edycji in-place). Cudzysłowy w edytowanym bloku: parzyste,
     sprawdzone ręcznie per wywołanie (3 nowe pary z msg=\"...\").
     Stary format "[DS-ADDR]" nie występuje już w żadnym aktywnym
     wywołaniu logPrintf/logPrintln w pliku.

     POZOSTAŁO: reszta prefiksów z listy v174 ([WARN] 63,
     [OK] 33, [CMD-RUN] 32, [ERR] 18, [TSL] 14, [FS-GUARD] 14 itd.)
     - kolejne partie po zatwierdzeniu tej. Filtr JS panelu WWW
     (pkt 2.3) - nadal niezweryfikowany; przesłany
     akwarium-firebase-panel-v13.html to panel Firebase (zdalny),
     NIE zawiera filtra logów - jawnie odsyła do lokalnego panelu ESP
     (port 8080, /terminal), którego pliku nie mamy.

### v174 (2026-07-30) - LOGS-AI-STEP5: konwersja DS-DIAG (luka względem
     planu, zidentyfikowana przy przeglądzie z pełnym
     RybyLED_logi_pod_AI_plan.md - pkt 2.2.3 wymienia DIAG-XX i
     DS-DIAG razem jako zakres kroku 3, ale DS-DIAG zostało w
     całości w starym formacie przez kroki 3-4).

     ZAKRES: 9 wywołań logPrintf w aktywnej diagnostyce DS18B20
     wody w setup() (~linia 11538-11600: start diagnozy, pomiar
     pinu, reset_pulse, ROM x2, per-attempt trace x1 w pętli 3
     prób, werdykt FAIL/OK) + 1 wywołanie potwierdzające wysyłkę
     alertu Telegram w tgTaskFn (~linia 11085).

     Świadomie NIE dotknięto: String ds3Msg (~linia 11067) - treść
     wiadomości Telegram, kanał narracyjny czytany przez człowieka,
     poza zakresem per plan pkt 1.2d. Komentarz w CHANGELOG v94
     (~linia 1996, "WERYFIKACJA: DS18B20 OK...") i przykład formatu
     w komentarzu v67 (~linia 2771, "Log: [emoji] [DS-DIAG]...") -
     dokumentacja historyczna, ten sam typ wyjątku co baner v151
     pozostawiony w kroku 4.

     POZIOMY (wg realnej severity, metoda z v173 - sprawdzenie
     kontekstu/logiki funkcji, nie zgadywanie): lvl=INFO (8) - start
     diagnozy, pomiar pinu, reset_pulse, ROM (sukces i BRAK),
     per-attempt trace (log próby w pętli retry niezależnie od
     wynik=OK/FAIL - to ślad kroku pośredniego, nie werdykt końcowy),
     potwierdzenie wysyłki alertu TG, finalne "DS18B20 wody OK".
     lvl=ERR (1) - finalny FAIL po 3 nieudanych próbach: ustawia
     ds3BootFail=true i uruchamia ścieżkę alertu Telegram - analogia
     do DIAG-17-RESTART z v173 (strzela tuż przed akcją korygującą
     wysokiego ryzyka).

     DECYZJA STRUKTURALNA (wykracza poza mechaniczną konwersję z
     kroku 4): blok FAIL był 7-liniowym dumpem z osadzonymi \n i
     3-punktową checklistą - dokładnie przypadek "boxa"/wielolinijki
     z pkt 1.2b i 2.1 planu (canonical/wide log line), inny niż
     DIAG-N z kroku 4 (te już były 1-liniowe, wymagały tylko
     prefiksu). Spłaszczony do JEDNEJ linii: dane liczbowe (pin,
     poziom pinu, reset_pulse) zachowane jako key=value; checklista
     (bez własnych danych liczbowych, czysta porada dla człowieka)
     skondensowana do wolnego tekstu w nawiasie - zgodne z decyzją
     nt. "Sprawdź X" z v173.

     DROBNE PORZĄDKI PRZY OKAZJI: em dash (—) usunięty z 2 linii
     (zanieczyszczał liczbę znaków spoza ASCII/polskich - pkt
     1.2a/1.3 planu); klucz "ROM=" zmieniony na małe "rom=" dla
     spójności z resztą pól (pin=, voltage=, devices= już małymi
     literami); końcowe "-> %s" w linii per-attempt zamienione na
     właściwe pole "wynik=%s" (pełna zgodność z logfmt zamiast
     strzałki ASCII).

     WALIDACJA: liczba linii 18448 -> 18442 (-6, wyłącznie z
     kolapsu bloku FAIL - reszta edycji in-place bez zmiany liczby
     linii). Automatyczne przejście po wszystkich dopasowaniach
     logPrintf/logPrintln/Serial.printf w pliku, sprawdzające
     parzystość nieescapowanych cudzysłowów w pełnym zasięgu
     wywołania (balans nawiasów, nie tylko literał): 0 rzeczywistych
     naruszeń, 2 pozorne trafienia - oba to słowo "logPrintf("
     zacytowane w komentarzach CHANGELOG (ten sam typ pozornego
     trafienia co w v173). Delta nawiasów okrągłych -3/-3 w pełni
     wyjaśniona usuniętymi zagnieżdżonymi nawiasami w bloku FAIL i
     liniach ROM=BRAK/Diagnoza (nie utratą pary). Nawiasy klamrowe
     { } bez zmian (1817/1830) - struktura if/else nietknięta.

     POZOSTAŁO: poza-DIAG-owe/poza-DS-DIAG prefiksy typu
     [INFO]/[HEAP]/[ERR]/[FS-GUARD] itd. w reszcie pliku - nadal
     nietknięte (odnotowane już w v173 jako szerszy zakres niż plan
     zakładał na krok 3/4). Filtr JS panelu WWW (pkt 2.3 planu) -
     nadal niezweryfikowany, plik panelu nie jest częścią tego .cpp.

### v173 (2026-07-30) - LOGS-AI-STEP4: dokończenie kroku 3 - standalone
     [DIAG-N] bez prefiksu poziomu, świadomie odłożone w v172.

     ZNALEZISKO PRZED WYKONANIEM: plan (i komentarz v172 poniżej)
     zakładał "standalone = informacyjne, nie WARN/ERR". Sprawdzenie
     każdego z 10 wystąpień na tle jego WARN-owego "rodzeństwa" (ten
     sam numer DIAG-N) pokazało, że założenie było błędne dla
     większości - kilka standalone bloków jest W RZECZYWISTOŚCI
     bardziej krytyczne niż sąsiadujący WARN, nie mniej. Dowód z
     samego kodu: komentarz nad diagWaterTemp() nazywa próg >28°C
     "ostrzeżeniem" a próg >32°C ("WODA PRZEGRZANA") "alarmem" -
     przypisanie mu lvl=INFO byłoby cofnięciem severity, nie
     neutralnym mapowaniem. Analogicznie diagHeap(): "Alarm
     krytyczny (zawsze)" dla HEAP KRYTYCZNY i "Alarm fragmentacji"
     dla FRAGMENTACJA HEAP - oba nazwane w komentarzu jako
     poważniejsze niż sąsiadujący "Heap niski (<50kB)", który już
     ma lvl=WARN.

     Przypisane poziomy (10 wywołań, wg realnej severity z
     komentarzy/logiki funkcji, NIE wg samej obecności/braku
     prefiksu w oryginale):
       lvl=INFO (3): DIAG-11 (próg informacyjny 60-85%, poniżej
         WARN @85%); DIAG-13, DIAG-16 (potwierdzenie auto-naprawy
         tuż po zadziałaniu WARN w tym samym bloku - rezultat, nie
         nowy alarm).
       lvl=WARN (1): DIAG-23 (PWM overflow, auto-clamp do 1023 -
         anomalia wykryta i naprawiona w locie, system działa dalej;
         unikalny tag, brak WARN/ERR "rodzeństwa" do porównania).
       lvl=ERR (6): DIAG-17 (HEAP KRYTYCZNY - najwyższy z 3 poziomów
         heap wg komentarza funkcji); DIAG-17-FRAG (środkowy
         poziom, "WWW moze nie dzialac"); DIAG-17-RESTART ×2
         (strzela tuż przed wymuszonym ESP.restart() - z definicji
         krytyczne); DIAG-28 "Woda przegrzana" (>32°C, patrz wyżej);
         DIAG-29 NaN/Inf (uszkodzenie danych uczenia wymagające
         resetu do wartości domyślnej - gorsze niż zwykłe "poza
         zakresem" u sąsiadów WARN).

     DECYZJA "Sprawdź X" (druga sprawa odłożona w v172): NIE
     restrukturyzowano. Przejrzano wszystkie 10 wystąpień
     narracyjnych sugestii w już-skonwertowanych blokach WARN
     (DIAG-4, 11, 12, 18, 19, 22, 27×2, 28) - żadna nie zawiera
     ukrytych danych liczbowych/pomiarowych nadających się na pole
     key=value; to czysta porada dla człowieka ("Sprawdź
     harmonogram", "Sprawdź serwer NTP/DNS" itd.) bez naturalnego
     klucza. Pozostawione jako wolny tekst na końcu wywołania -
     zgodny z logfmt sposób łączenia pól strukturalnych z czytelną
     dla człowieka resztą (już obecny wzorzec z kroku 3), nie
     wymaga dalszej redukcji.

     WALIDACJA: liczba linii pliku niezmieniona (18389). Liczba
     cudzysłowów wzrosła dokładnie o 20 (10 wywołań × 2 dla
     msg="..." - zgodne z oczekiwaniem). Automatyczne przejście
     przez WSZYSTKIE 590 wywołań logPrintf/logPrintln w pliku,
     sprawdzające parzystość nieescapowanych cudzysłowów per-
     wywołanie (ten sam typ błędu co krytyczny bug z kroku 3) - 0
     rzeczywistych naruszeń (1 pozorne trafienie to słowo
     "logPrintf(" zacytowane w tym komentarzu, nie kod). Nawiasy
     ()/{} zbalansowane w skali całego pliku.

### v172 (2026-07-30) - LOGS-AI-STEP1+2+3: kroki 1-3 z RybyLED_logi_pod_AI_plan.md.

     DECYZJA (pytanie 1 z planu, pkt 3): rozstrzygnięte researchem, nie
     zgadywaniem. logfmt (pierwotne źródło: Heroku/brandur.org, dziś
     replikowane identycznie w bibliotekach Elixir/pino/CLI-parserach)
     ZAWSZE traktuje poziom logowania jako pole "level="/"lvl=", NIGDY
     jako prefiks w nawiasach - to jedno spójne pole key=value zamiast
     specjalnego wyjątku formatu dla samego poziomu. Odrzucono
     "[WARN]" na rzecz "lvl=WARN". Potwierdzone też: dla płaskich,
     jednopoziomowych danych (nasz przypadek) logfmt jest tańszy
     tokenowo niż JSON (mniej cudzysłowów/nawiasów na klucze) - zgodne
     z motywacją z pkt 1.3 planu.

     KROK 3 (pkt 2.2.3) - restrukturyzacja bloków DIAG-1..31: format
     "[WARN] [DIAG-N] TYTUŁ! reszta" -> "lvl=WARN tag=DIAG-N
     msg=\"TYTUŁ\" reszta". Zamienionych: 34 wywołań logPrintf (regex
     na wzorcu, zweryfikowane ręcznie każde z 34 dopasowań pod kątem
     false-positive przy skrótach z kropką typu "niezsynch." oraz przy
     wartościach %02d:%02d zawierających dwukropek). Kod diagnozy
     (DIAG-N) zachowany bez zmian jako osobne pole "tag=" - dalej
     grepowalny po starych logach archiwalnych, zgodnie z wymogiem
     planu. Świadomie NIE dotknięto: standalone "[DIAG-N]" bez
     prefiksu poziomu (np. ℹ [DIAG-11], [DIAG-17-RESTART],
     [DIAG-23]) - to inny przypadek (informacyjny, nie WARN/ERR),
     wymaga osobnej decyzji o poziomie w kolejnym kroku. Narracyjne
     zdania "Sprawdź X" pozostawione bez zmian w tej turze (osobna
     literał-kontynuacja w kilku blokach) - redukcja do samego
     tag=value dla TYCH fragmentów to kolejny, bardziej ryzykowny
     krok (wymaga decyzji per-blok co zachować jako dane, a co
     odrzucić jako czystą poradę dla człowieka).

     NAPRAWIONY W LOCIE BŁĄD KRYTYCZNY: pierwsza wersja tej
     transformacji generowała msg="TYTUŁ" z NIEZESCAPOWANYMI
     cudzysłowami wewnątrz already-otwartego literału C - to zamykało
     string C przedwcześnie i psuło kompilację. Wykryte przed zapisem,
     naprawione przez poprawne escapowanie (\" zamiast ").

     WALIDACJA: identyczna liczba linii (18349), nawiasów {}/(),
     średników przed/po. Liczba cudzysłowów wzrosła dokładnie o 68
     (34 wywołania × 2 cudzysłowy dla msg="...") - zgodne z
     oczekiwaniem, potwierdza brak przypadkowego rozerwania innych
     literałów.

     NAPRAWA 1b (w tej samej turze, po walidacji): pierwszy przebieg
     kroku 1 pomijał wywołania logPrintf, w których literał formatu
     był na OSOBNEJ linii niż "logPrintf(" (typowe dla dłuższych
     wywołań rozbitych na wiele linii dla czytelności). Skutek: ~51
     wywołań (głównie bloki DIAG-1..31) zachowało resztki emoji/FE0F
     (np. widoczny "[WARN]️" z niewidocznym selektorem wariantu zamiast
     czystego "[WARN]"). Naprawione nowym przebiegiem śledzącym PEŁNY
     zasięg wywołania przez balans nawiasów (nie tylko pierwszą linię).
     Przy okazji: strzałki Unicode (→←↔↑↓) w tych blokach zamienione na
     ASCII "->"/"<-" zamiast usunięte (zachowanie znaczenia separatora,
     np. "IDENTYCZNE → KOLIZJA" -> "IDENTYCZNE -> KOLIZJA", nie
     "IDENTYCZNEKOLIZJA"). Wykryto i naprawiono w locie 1 realną
     regresję z pierwszej wersji tej łatki: uniwersalne przycinanie
     wiodącej spacji w KAŻDYM literalu string omyłkowo zjadało spację
     niezwiązaną z emoji (3x " (log co 30min)" w logach DS18B20 błędu
     czujnika - spacja jest częścią normalnego formatowania konkatenacji
     %s, nie artefaktem po emoji) - poprawione tak, by przycinanie
     dotyczyło WYŁĄCZNIE literałów, które faktycznie zaczynały się od
     emoji/ramki w oryginale.

     WALIDACJA: identyczna liczba linii (18326), nawiasów {}/(),
     cudzysłowów i średników przed/po tej poprawce.

     KROK 1 (pkt 2.2.1) - global strip emoji/ramek: automatyczny
     przebieg po wszystkich wywołaniach logPrintf/logPrint/logPrintln
     (312 zmodyfikowanych linii z 606), usuwający emoji i znaki ramek
     ASCII-art (═ ║ ╔ ╚ ╠ ╣ itd.) z treści logów runtime. NIE dotyczy
     komentarzy ani wywołań narracyjnych (sendTelegramMessage/
     sendTelegramDocument/buildTelegram*Report/tgPost) - Telegram
     zostaje bez zmian, bo to kanał czytany bezpośrednio przez
     człowieka.

     NAPRAWIONA UTRATA INFORMACJI: 4x w pętli SOFT-START/PRZEJŚCIE
     (linie ~8112/8129/8147/8173) emoji 💧 było JEDYNYM identyfikatorem
     trzeciej wartości temperatury (LED1/LED2 miały tekstowe etykiety,
     woda nie miała żadnej poza emoji) - zamienione na "woda:%.0f°C",
     zgodnie z konwencją już używaną gdzie indziej w kodzie.
     Sprawdzone systematycznie (nie wyrywkowo): wszystkie pozostałe
     emoji w wywołaniach logujących mają już jawne etykiety tekstowe
     obok siebie (np. "🛡️ [GUARD]", "🌡️ [DS-ADDR]") - ich usunięcie
     nie traci żadnych danych, tylko dekorację.

     KROK 2 (pkt 2.2.2) - spłaszczenie PODSUMOWANIE DNIA do jednej
     linii canonical/logfmt (wzorzec "wide log line" ze Stripe, patrz
     pkt 1.3 planu). Funkcja logDailySummary() zamiast 15 osobnych
     logPrintf tworzących ramkę z ~600 B/dzień drukuje teraz JEDNĄ
     linię "SUMMARY key=value ..." (~200 B), zawierającą wszystkie
     te same dane co poprzednio + dodaną pełną datę (date=YYYY-MM-DD,
     wcześniej logTime() dawał tylko HH:MM:SS - bez daty grep po
     konkretnym dniu w log_a.txt/log_b.txt był niemożliwy). Pola:
     date, time, korMin, korMax, pomiarow, srRedukcjaPct, luxPokoj,
     luxWoda, tempP1, tempP2, tempWoda, transmisjaPct, probek, mocW,
     mocLimitW, mocB/FS/FSB/N/C (moc per kanał LED). Zweryfikowano
     ręcznie: 22 specyfikatory %, 22 argumenty - zgadza się.

     WALIDACJA (oba kroki): sprawdzono bilans nawiasów {}/() przed/po
     krokiem 1 (identyczny) oraz policzono nawiasy klamrowe funkcji
     logDailySummary() - zamyka się poprawnie w linii 7201, bez
     przesunięcia zakresu funkcji.

     NIE ZROBIONE W TYM KROKU (celowo, zgodnie z planem 2.2):
     - ujednolicenie [OK]/[WARN]/[ERR] vs LVL= (pkt 3 planu, decyzja
       odłożona do kroku 2-4 - jeszcze nierozstrzygnięta, patrz niżej).
     - restrukturyzacja DIAG-XX/DS-DIAG do tag=value (krok 3 planu).
     - filtr JS w panelu WWW: NIE wymaga zmian dla kroku 1 (same tagi
       nietknięte) - DLA KROKU 2 DO SPRAWDZENIA: jeśli filtr JS
       parsuje/rozpoznaje starą wielolinijkową ramkę PODSUMOWANIE DNIA
       (np. po tekście "PODSUMOWANIE DNIA" lub liniach "Korekty MIN"),
       trzeba go zaktualizować pod nowy tag "SUMMARY" - nie
       zweryfikowane w tym kroku, bo panel WWW nie był częścią
       przesłanych plików w tej turze.

### v171 (2026-07-30) - FEATURE-COREDUMP-SUMMARY: punkt 3 z
     RYBY_LED_dalsze_kroki_v169.md ("core dump zamiast zgadywania
     Stack overflow lub null pointer"), odblokowany dopiero teraz -
     brakowało realnego partitions.csv do weryfikacji miejsca.

     STAN WYJŚCIOWY (potwierdzony w przesłanym partitions.csv):
     partycja `coredump` (64 KB, offset 0x610000) została dodana
     JUŻ w [v113] - istnieje od dawna, ale do tej pory nic jej nie
     odczytywało. `CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH` (+ DATA_FORMAT_ELF,
     CHECKSUM_CRC32) jest domyślnie WŁĄCZONE w prekompilowanym
     sdkconfig frameworku Arduino-ESP32 dystrybuowanym przez
     PlatformIO (potwierdzone niezależnym źródłem - zrzut sdkconfig.h
     z paczki framework-arduinoespressif32-libs) - nie wymaga więc
     żadnej zmiany w sdkconfig.defaults, tylko obecności partycji
     `coredump`, którą już mamy.

     FIX: w setup(), zaraz po bloku [PRE-RESET]/[WDT-HUNT] (ten sam
     punkt, ta sama logika "co się działo tuż przed crashem"):
     esp_core_dump_image_check() sprawdza, czy w partycji jest
     poprawny obraz; jeśli tak - esp_core_dump_get_summary() daje
     PROGRAMOWO, bez podłączania komputera: nazwę zadania (tasku),
     które spanikowało, adres PC oraz backtrace (tablica adresów).
     Logowane jako nowa linia 💥 [COREDUMP], razem z gotową komendą
     xtensa-esp32s3-elf-addr2line do zamiany adresów na linie kodu.
     Po odczycie obraz jest kasowany (esp_core_dump_image_erase()) -
     inaczej ten sam dump logowałby się PONOWNIE przy każdym kolejnym
     restarcie, aż do następnego crasha.
     Cały blok jest dodatkowo owinięty w `#if CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH`
     (ten sam wzorzec co w oficjalnym nagłówku esp_core_dump.h) - jeśli
     z jakiegoś powodu w tym konkretnym buildzie flaga jest wyłączona,
     kod się nie skompiluje w środku (zamiast literować się na
     "unknown type name"), a w logu pojawi się jawna informacja
     "niedostępne w tym sdkconfig" zamiast cichej ciszy.

     DO ZWERYFIKOWANIA NA URZĄDZENIU: wywołać świadomy crash (np.
     istniejącą komendę DIAG-17 lub null-pointer w kodzie testowym),
     potwierdzić że po restarcie w logu pojawia się [COREDUMP] z
     nazwą tasku i że kolejny restart (bez nowego crasha) już tego
     NIE powtarza (bo esp_core_dump_image_erase() zadziałał).

### v170 (2026-07-30) - Trzy poprawki z dalszego śledztwa po v169
     (RYBY_LED_dalsze_kroki_v169.md: analiza aquarium_log.txt, sesje od
     10:07 do 16:24 następnego dnia, obejmuje oba restarty - PANIC linia
     9322, OTA linia 15226):

     (1) FIX-BOOT-LOGFLUSH: znaleziono w logu z urządzenia policzalny
     dowód, że linie zapisane w pierwszych ~10s po każdym boot/crashu
     (FS-CAPACITY, SYSTEM START, przyczyna restartu, diagnoza PANIC,
     [PRE-RESET]/[WDT-HUNT], sieroty) fizycznie lądują w pliku PO liniach
     zalogowanych ~10s później (np. "TG-TASK START" przy ms=10495 leżało
     w pliku PRZED "SYSTEM START"/"PANIC" z ms=1434-1460 tego samego
     boota), mimo że chronologicznie były pierwsze. ROOT CAUSE: setup()
     działa na Core 1 -> logToFile() idzie do g_logBuf[1], a jedyny
     konsument logFlushPending[1] (tgTaskFn, Core 0) startuje dopiero
     ~10s później (czeka na WiFi) - w tym oknie bufor Core 1 nie ma
     żadnego konsumenta. FIX: wymuszony, jednorazowy flush g_logBuf[1]
     zaraz po bloku PRE-RESET/sierot w setup(), zanim tgTaskFn w ogóle
     zostanie utworzony (zero ryzyka wyścigu o logMutex/bufor w tym
     miejscu - ten sam mutex+logFlushCore1() co przy normalnej obsłudze
     w tgTaskFn, tylko wywołany wcześniej).

     (2) FIX-DIAG-FSBREAKDOWN-FULLSCAN: DIAG-FSBREAKDOWN (v168) sumowało
     tylko twardą listę znanych plików - pliki spoza tej listy, ale
     realnie istniejące w kodzie (/history_tmp.csv, /wifi_list_tmp.txt,
     /wifi_list_old.txt, /last_reset.txt) wpadały w "różnica_vs_used",
     myląco sugerując czysty narzut LittleFS. FIX: pełny przelot
     katalogu root (ten sam wzorzec co istniejący endpoint /api/fs-list)
     sumuje rozmiar KAŻDEGO pliku faktycznie obecnego na dysku, loguje
     indywidualnie każdy plik >=4KB oraz największy znaleziony plik.

     (3) FIX-ORPHAN-REMOVE-UNCHECKED: sprzątanie sierot przy starcie
     (/history_tmp.csv, /wifi_list_tmp.txt, /wifi_list_old.txt) wołało
     LittleFS.remove() bez sprawdzenia wyniku - log twierdził "Usunięto"
     nawet, gdyby littlefs udokumentowanie odmówił usunięcia z powodu
     braku wolnych bloków blisko pełnego dysku (littlefs-project/littlefs
     #1007, #545) - dokładnie scenariusz, w którym te sieroty realnie
     powstają. FIX: logowany jest teraz rozmiar pliku PRZED usunięciem
     oraz realny wynik remove() (OK / NIEUDANE).

     Świadomie NIE zrobione w tej wersji (z RYBY_LED_dalsze_kroki_v169.md):
     sortowanie logu po `ms=` w obrębie sesji przy analizie - to
     rekomendacja analityczna, nie zmiana w firmware; core dump do flash -
     wymaga wglądu w partitions.csv, którego nie było w tej sesji.

### v169 (2026-07-29) - Trzy niezależne poprawki ze śledztwa DIAG-FSBREAKDOWN
     (v168) + analizy realnego loga z urządzenia (aquarium_log.txt) +
     testu na czystym FS (fs_diag_test):

     (1) DIAG-FSGROWTH: dodano `LittleFS.usedBytes()`/`totalBytes()` do
     heartbeatu, jako nowa, OSOBNA linia 💓[HEAP-c] (nie dopisana do już
     napiętych [HEAP]/[HEAP-b] — patrz [v161] FIX-HEARTBEAT-TRUNC, bufor
     logPrintf to stały char[384], cichnie ucina za długie linie). Loguje
     fsUsed/fsTotal/fsPct oraz deltę od poprzedniego odczytu heartbeatu.
     CEL: dotąd used/total logowane były tylko raz przy starcie
     (FS-CAPACITY) i przy wyzwoleniu FS-GUARD (rzadkie, godziny odstępu) —
     brak widoczności czy widmowy narzut z DIAG-FSBREAKDOWN faktycznie
     narasta w czasie/z restartami (hipoteza H2: bloki osierocone po
     przerwanych zapisach WDT/crash), czy jest stały. Teraz będzie to
     widać w trendzie między heartbeatami.

     (2) FIX-OTA-SRC-MISLABEL: potwierdzone wprost w log_combined z
     urządzenia — restart po zakończonym OTA poprawnie rozpoznawany jako
     `RESTART: przyczyna = SOFTWARE (3)` (bootResetReason, ESP-IDF), ale
     towarzyszący `[PRE-RESET] Poprzedni cykl: ... src=WDT(brak danych)`
     mylnie sugerował crash. ROOT CAUSE: `ArduinoOTA.onEnd()` nigdy nie
     wołał `PRE_RESET_UPDATE(...)` ani nie ustawiał `g_restartPending` —
     jedyne dwa miejsca, które to robiły (restart soft/HTTP, DIAG-17,
     ~linie 14683/17342 sprzed tej poprawki) nie obejmowały ścieżki OTA,
     bo bibliteka ArduinoOTA restartuje samodzielnie zaraz po onEnd(), bez
     jawnego ESP.restart() w tym pliku. Efekt: ostatni zapis do
     `g_preReset` przed restartem OTA pochodził z periodycznego
     heartbeatu tgTaskFn (co 2s, src=2="WDT(brak danych)" domyślnie) —
     stąd mylna etykieta. FIX: `onEnd()` teraz woła
     `g_restartPending=true` + `PRE_RESET_UPDATE(4)` (nowy src=4="OTA"),
     dokładnie ten sam wzorzec co pozostałe dwa jawne restarty.

     (3) DIAG-TGDOC-THROUGHPUT: zmierzono realny czas i przepustowość
     (KB/s) streamingu w `sendTelegramDocument()` — nowy log
     `[DOC-SPEED] sent=...B ms=... kBps=...`. KONTEKST: log z urządzenia
     pokazał `[WARN] [FLASH-STALL] site=fsGuard ms=28999` (cały blok
     fsGuardPending, w tym ta wysyłka, ~4.1 MB) + towarzyszące
     `[CB] ITER-BUDGET: skip TG-POLL` — czyli bot nie odpowiadał na
     komendy Telegram przez te ~29s, bo cały fsGuardPending (w tym
     sendTelegramDocument()) wykonuje się synchronicznie w tgTaskFn, tej
     samej pętli, która normalnie obsługuje TG-POLL. Research (fora
     ESP32/mbedTLS): software'owe TLS na ESP32 typowo osiąga rzędu
     kilkuset kbps do ~1 Mbps przepustowości uploadu — więc ~29s na
     ~4.1 MB (~145 KB/s) mieści się w oczekiwanym zakresie dla mbedTLS na
     tym sprzęcie, NIE jest samo w sobie dowodem błędu w tym kodzie.
     Właściwy problem to ARCHITEKTURALNY: cały ten blok blokuje tgTaskFn
     (i tym samym TG-POLL) na czas trwania wysyłki, więc bot "milczy"
     przez dziesiątki sekund za każdym razem, gdy FS-GUARD się wyzwoli.
     NIE jest to fix — sam pomiar, żeby przy kolejnym auto-sendzie było
     widać czy to "normalna" prędkość mbedTLS czy dodatkowa degradacja
     sieci. DO ROZWAŻENIA (nie zrobione w tej wersji): przeniesienie
     sendTelegramDocument() z tgTaskFn do osobnego, dedykowanego taska,
     żeby TG-POLL nie stał w miejscu podczas wysyłki logów.

     (4) FIX-FW-VERSION-DRIFT (TRZECI RAZ — po v156 i v167): baner
     startowy (`logPrintln("📦 Wersja firmware: v167...")`) był wciąż
     ręcznie kopiowanym literałem, mimo że `#define FW_VERSION` miał być
     "jedynym źródłem prawdy" od v156 — sam "fix" nigdy faktycznie nie
     podpiął bannera pod tę stałą, tylko ręcznie zsynchronizował wartości
     jednorazowo, więc drift był strukturalnie nieunikniony i wrócił.
     Naprawiono ostatecznie: baner teraz czyta `FW_VERSION` przez `%s`
     (logPrintf zamiast logPrintln z literałem) — nie ma już drugiego
     miejsca do zapomnienia.

     DO ZWERYFIKOWANIA NA URZĄDZENIU: (a) czy [HEAP-c] fsUsed rośnie
     skokowo w okolicach restartów PANIC/WDT (potwierdziłoby H2); (b) czy
     kolejny restart OTA poprawnie pokaże src=OTA zamiast WDT; (c) jaka
     realna wartość kBps wychodzi w [DOC-SPEED] przy kolejnym auto-sendzie.

### v168 (2026-07-28) - DIAG-FSBREAKDOWN: dokumenty log_a+log_b wysyłane
     automatycznie przez FS-GUARD (fsSizeGuard()/fsGuardPending) mają
     konsekwentnie ~4-4.7 MB, mimo że próg wyzwalający to FS_LIMIT =
     8 232 000 B (~8.23 MB), a pozostałe znane pliki (history.csv,
     energy_stats.json, adapt_stats.json, wifi_list.txt,
     telegram_config.json) są rzędu dziesiątek KB. Skąd różnica ~4 MB
     między FS_LIMIT a realnym rozmiarem wysłanych logów — nieznane.
     DODANO: trzy linie logPrintf "[DIAG-FSBREAKDOWN]" tuż w momencie
     wyzwolenia FS-GUARD (PRZED sendTelegramDocument()/rotacją/
     kasowaniem) — zrzut used/total oraz rozmiarów log_a, log_b,
     history.csv, history_old.csv, energy_stats.json, adapt_stats.json,
     wifi_list.txt, telegram_config.json i sumy vs used (różnica =
     narzut LittleFS lub coś nieuwzględnionego). Zero zmian logiki
     biznesowej — czysto diagnostyczny dodatek, jedna, jednorazowa
     grupa logów na wyzwolenie FS-GUARD (rzadkie zdarzenie, brak
     spamu). Do analizy przy następnym auto-sendzie.

### v167 (2026-07-26) - KA-TIGHTEN-TG + FIX-FW-VERSION-DRIFT (znowu): plik
     dostarczony przez użytkownika jako "v166" miał `#define FW_VERSION
     "v166"`, ale baner startowy (`logPrintln("📦 Wersja firmware:...")`)
     wciąż pokazywał tekst "v164" — to DOKŁADNIE ten sam dryf, który miał
     być raz na zawsze naprawiony w v156 (FIX-FW-VERSION-DRIFT, jedna
     stała FW_VERSION). Ktoś zaktualizował stałą, ale nie baner — dwa
     miejsca zamiast jednego źródła prawdy. Naprawiono ponownie.
     Do tego dołożono `KA-TIGHTEN-TG`: plik "v166" nie miał jeszcze
     zaciśniętego `applyTcpKeepalive(tgClient,...)` z (3,2,3) na (2,1,2)
     (fix z v165, opisany niżej) — dodano na tej właściwej, nowszej
     bazie, obok niezależnych fixów DAYROLL i JSON-DUP.

### v166 (2026-07-26) - FIX-v166-JSON-DUP: `dayBuf` (z domykającym `}`) był
     drukowany w całości, a zaraz potem ponownie jako `dayStr` (ten sam
     bufor, tylko bez ostatniego `}`) — `d0..d6` trafiało do pliku
     DWA razy, a JSON miał dodatkowy `}` w środku (obiekt zamykał się
     przedwcześnie po pierwszym `dayBuf`, kolejne klucze trafiały POZA
     już zamknięty obiekt = nieprawidłowy JSON). Dowód z
     `energy_stats.json`: `{...,"yr":126,"d0":0.00,...,"d6":0.00},
     "d0":0.00,...`. Fix: każdy bufor budowany BEZ własnego domykającego
     `}` i drukowany dokładnie raz; jedyny `}` na końcu pliku pochodzi
     z `wkBuf`.

### v165 (2026-07-25) - FIX-v165-DAYROLL: poprzednio rolowanie dnia
     (`energyTodayWh` → historia) wymagało trafienia DOKŁADNIE w minutę
     00:00 (`tm_hour==0 && tm_min==0`). Jeśli `loop()` zawiesił się w tym
     jednym 60-sekundowym oknie (reconnect WiFi, zapis LittleFS, WDT
     itp.), całe rolowanie dnia było pomijane — `energyTodayWh` nigdy się
     nie zerowało, więc `energyWeekWh`/`energyMonthWh` nie odklejały się
     od `energyTodayWh` (tydzień i miesiąc pokazywały to samo co dziś,
     mimo upływu wielu dni). Fix: wykrywanie wprost zmiany `tm_yday` —
     odpali się najpóźniej przy pierwszej iteracji `loop()` po zmianie
     dnia, niezależnie od godziny.

### v164 (2026-07-24) - FIX-DNS-ZERO-IP: znaleziono konkretną przyczynę
     masowej kaskady ~20 restartów WDT_TASK z rzędu (21:38-22:05,
     `log_combined__23_.txt`) + 1 PANIC/CRASH, po której użytkownik musiał
     ręcznie zresetować zasilanie, żeby urządzenie złapało WiFi.
     DIAGNOZA: sam WiFi łączył się poprawnie za każdym razem (status
     -1→3, IP przydzielone w ~30s) — problem był głębiej.
     `[FB-INIT] DNS pre-warm OK` w CAŁYM pliku poza tym jednym oknem
     zawsze poprawnie zwracał `34.107.226.223` — ale w oknie kaskady
     wielokrotnie zwracał **`0.0.0.0`** (pusta/nieprawidłowa odpowiedź
     DNS), a `WiFi.hostByName()` mimo to zwracał `true` (sukces)!
     Kod nigdy nie sprawdzał, czy zwrócony adres ma sens — więc próbował
     łączyć się z nieroutowalnym 0.0.0.0, co przyczyniało się do kolejnych
     zawieszeń i restartów w pętli, aż DNS w sieci lokalnej się wyklarował
     (albo do ręcznego power-cyklu, który to przyspieszył).
     To był realny, chwilowy problem z DNS w sieci lokalnej (nie stały
     błąd firmware) — ale firmware nie miał jak go poprawnie wykryć i się
     wycofać, więc traktował fałszywy sukces jako prawdziwy.
     FIX: dodano walidację `_resolved == IPAddress(0,0,0,0)` po każdym
     `WiFi.hostByName()` w trzech miejscach — `fbInitialize()`,
     `tgEnsureConnected()`, `sendTelegramDocument()` — jeśli zwrócony
     adres to 0.0.0.0, traktowane jak błąd DNS (retry / fallback / circuit
     breaker), nie jak sukces. Nowy log `[WARN] [DNS-ZERO-IP] site=...`
     przy wykryciu.
     DO ZWERYFIKOWANIA NA URZĄDZENIU: czy przy kolejnej chwilowej awarii
     DNS w sieci lokalnej urządzenie teraz poprawnie retry'uje/cofa się
     zamiast wpadać w wielokrotną kaskadę restartów.

### v163 (2026-07-24) - FIX-KA-TIGHTEN: pierwszy PRAWDZIWY FIX (nie diagnostyka)
     w tej serii od v148. `log_combined__21_.txt` pokazał crash WDT bez
     zwykłej sekwencji TIMEOUT→CircuitOpen→cooldown — zamiast tego serię
     wolnych, ale udanych wywołań FB/TG, kończącą się
     `[FB-BLOCK-DONE] total=9624ms`. To niemal identyczna sygnatura do
     już raz zdiagnozowanego i "naprawionego" buga z v104
     (FIX-WDT-FB-DEAD-TCP, `total=9613ms`) — martwe gniazdo Firebase
     podczas reuse. v105 usunął cały stary `fbConnect()` (307 linii) na
     rzecz biblioteki FirebaseClient/ESP_SSLClient, zakładając że ta
     "prawidłowo wykrywa martwy socket" — ale symptom wrócił.
     PRZEGLĄD KODU: `sendStatusToFirebase()`/`checkFirebaseCommands()`/
     `checkFirebaseConfig()` mają pętlę `while (!isResult() && <12000ms)`
     z `esp_task_wdt_reset()` MIĘDZY wywołaniami `fbApp.loop()` — ale
     własny komentarz z v111 mówi wprost: `fbApp.loop() → SSL_read() →
     recv()` może zablokować się BEZ LIMITU wewnątrz pojedynczego
     wywołania. v112 dodał TCP keepalive (idle=3s, intvl=2s, cnt=3) jako
     obronę — ale to daje **najgorszy przypadek wykrycia martwego gniazda
     = 3+2×3 = 9 sekund**, zostawiając tylko ~1s marginesu przed WDT (10s),
     jeśli SSL_read() zablokuje się tuż po świeżym resecie. Pasuje idealnie
     do zmierzonych total=9613-9624ms w obu incydentach.
     FIX: zacieśniono keepalive do idle=2s, intvl=1s, cnt=2 → najgorszy
     przypadek wykrycia = 2+1×2 = 4 sekundy, dużo bezpieczniejszy margines
     do WDT. Zastosowano we WSZYSTKICH 4 miejscach w kodzie (nie 3 —
     sendStatusToFirebase, checkFirebaseCommands GET, checkFirebaseCommands
     DEL, checkFirebaseConfig — changelog v112 już wtedy mówił o 4).
     RYZYKO: krótszy keepalive = trochę więcej ruchu sieciowego
     (nieistotne przy tej częstotliwości wywołań) i teoretycznie odrobinę
     większa szansa fałszywego ECONNRESET na chwilowo wolnym, ale żywym
     łączu — akceptowalny kompromis wobec ryzyka WDT crasha.
     DO ZWERYFIKOWANIA NA URZĄDZENIU: czy `[FB-BLOCK-DONE] total=` nadal
     zbliża się do 10s, czy teraz kończy się wcześniej (ECONNRESET +
     NET_FAIL zamiast crasha).

### v162 (2026-07-23) - DIAG-DNSSTALL: po v161 (fix cichego ucinania linii
     heartbeat) log_combined__18_.txt pokazał KOMPLETNE dane — i mimo to
     PIĄTY z rzędu inny checkpoint (`[L:wsFlush]`, po pompa/events/
     DAILY-SUMMARY/histFlag) przy identycznym momencie crasha (cooldown=0s),
     a WSZYSTKIE PIĘĆ liczników (tgSend/wsCleanup/flash/events/fsGuard)
     pokazało zero tuż przed. To mocny dowód, że winowajca NIE jest w
     żadnej sekcji loop() (Core 1) — checkpoint różni się za każdym razem,
     bo Core 1 po prostu przypadkowo zamarza RAZEM z czymś na Core 0
     (tgTask), w momencie próby reconnect po cooldownie.
     Przegląd tgEnsureConnected() ujawnił: KAŻDY krok (mutex, TLS cooldown,
     connect()+TLS) ma esp_task_wdt_reset() w środku pętli oczekiwania
     LUB jest już osobno zmierzony i zalogowany (np. "[TG-CONN]
     connect()=... TCP+TLS=Xms" — zawsze drukowane, z ostrzeżeniem
     "BARDZO WOLNY!" >5000ms) — OPRÓCZ jednego: `WiFi.hostByName()`.
     To wywołanie ma reset PRZED i PO, ale ZERO ochrony W TRAKCIE — dokładnie
     ten sam wzorzec, co pierwotny problem z tgClient.printf()/print()
     (v94), który ostatecznie okazał się zawsze czysty po zmierzeniu
     (v155, TG-SEND-STALL). Komentarz dev-a przy DNS zakłada "może trwać
     ~1-4s" jako normę, ale NIGDY nie było to realnie zmierzone.
     CO ZOSTAŁO DODANE: zmierzono WiFi.hostByName() we WSZYSTKICH trzech
     miejscach wywołania: tgEnsureConnected() (~linia 5511),
     sendTelegramDocument() (~linia 6003, nowa ścieżka z v159, wywoływana
     wewnątrz fsGuardPending — dokładnie w warunkach złej sieci), oraz
     fbInitialize() (~linia 13514, w pętli retry 2x). Wspólne liczniki
     g_dnsStallCount/g_dnsStallMaxMs, próg 2000ms (powyżej dolnej granicy
     udokumentowanej "normy"), log [WARN] [DNS-STALL] site=<miejsce> przy
     przekroczeniu, dodane do heartbeatu 💓[HEAP-b] (dnsStallCnt/dnsStallMaxMs).
     NIE jest to fix — zachowanie DNS bez zmian, sam pomiar.
     DO ZWERYFIKOWANIA NA URZĄDZENIU: przy kolejnym crashu z dowolnym z
     pięciu checkpointów sprawdzić czy [DNS-STALL] pojawił się w tym samym
     oknie. Jeśli tak → winowajca WRESZCIE potwierdzony po 5 kolejnych
     próbach (events→fsGuard→dns). Jeśli nie → wyczerpano wszystkie
     rozsądne kandydaty na ścieżce connect+send+DNS; kolejny krok to
     migracja tgPost()/tgEnsureConnected() na HTTPClient mimo wcześniej
     udokumentowanego ryzyka resztkowego (patrz plan WDT, decyzja
     "MIGRACJA ODRZUCONA" z 2026-07-20) — albo pogodzenie się z tym, że to
     rzadkie (<1/dzień), samonaprawiające się zdarzenie bez dalszej
     możliwej diagnostyki software'owej.

### v161 (2026-07-23) - FIX-HEARTBEAT-TRUNC: prawdziwy, realny bug znaleziony
     po tym jak użytkownik potwierdził "to jest v160" mimo że
     log_combined__17_.txt nie pokazywał pola `fsGuardStallCnt` WCALE.
     PRZYCZYNA: `logPrintf()` (~linia 9336) używa STAŁEGO bufora
     `char buffer[384]` + `vsnprintf()`. Pojedyncza linia 💓[HEAP] ze
     wszystkimi polami dodanymi w v155-v160 (tgSendStall, wsCleanupStall,
     flashStall, eventsStall, fsGuardStall, heapSrc) ma po rozwinięciu
     ~448 bajtów — 64 bajty ZA DUŻO. `vsnprintf` ucina PO CICHU, bez
     żadnego ostrzeżenia czy błędu, więc każda linia HEAP w KAŻDYM logu
     od v158 wzwyż kończyła się w tym samym miejscu ("eventsSta...") i
     NIGDY nie pokazywała fsGuardStallCnt/fsGuardStallMaxMs/heapSrc —
     dane były poprawnie liczone w kodzie, po prostu nigdy nie trafiały
     do logu. Kolejne wersje (v158, v159, v160) dokładały pola bez
     sprawdzenia całkowitej długości linii względem bufora.
     FIX: rozbito jedną linię 💓[HEAP] na dwie — 💓[HEAP] (oryginalne pola
     do tlsMutexPeakWait + heapSrc, ~248B) i 💓[HEAP-b] (wszystkie liczniki
     DIAG-*-STALL, ~212B) — obie z dużym zapasem poniżej 384B. Zero zmian
     w samych licznikach/logice pomiarów, tylko w tym, JAK są wypisywane.
     LEKCJA: przy kolejnych dopisywanych polach do tej linii sprawdzić
     długość względem bufora 384B, zamiast zakładać że się zmieści.

### v160 (2026-07-22) - DIAG-FSGUARDSTALL: rozszerzenie instrumentacji flash na
     blok fsGuardPending, zastosowane na PRAWDZIWEJ bieżącej bazie (v159
     z FIX-LOG-SEND-TRIGGER — wcześniejsza wersja tego samego pomiaru z
     poprzedniej odpowiedzi została zbudowana na nieaktualnym v157/v158 i
     nie uwzględniała nowego sendTelegramDocument()).
     KONTEKST: log_combined__15_.txt pokazał TRZECI różny checkpoint
     (`[DAILY-SUMMARY]`, po `[L:pompa]` w v157 i `[L:events]` w v158) przy
     IDENTYCZNYM momencie crasha (dokładnie cooldown=0s circuit breakera).
     Diagnostyki v155-158 wciąż nie złapały niczego w heartbeacie tuż przed
     żadnym z tych crashy — checkpoint zmienia się za każdym razem, co
     sugeruje, że winowajca NIE siedzi w konkretnej sekcji loop(), tylko
     gdzieś, co zamraża OBA rdzenie naraz.
     NOWY, SILNIEJSZY KANDYDAT (dzięki v159 FIX-LOG-SEND-TRIGGER):
     `sendTelegramDocument()` jest teraz wywoływane WEWNĄTRZ bloku
     fsGuardPending, czyli dokładnie wtedy, gdy FS się zapełnia — co samo
     w sobie koreluje z długim działaniem/dużym ruchem w logach, w tym
     seriami TIMEOUT. Wysyłka pliku przez TLS pod złą siecią może trwać
     dużo dłużej niż same remove()/rename() (dla których i tak istnieje
     sprzeczność w komentarzach: v88 "~300-400ms" vs fsGuardPending "~2ms").
     CO ZOSTAŁO DODANE: zmierzono czas trwania CAŁEGO bloku fsGuardPending
     — sendTelegramDocument() + Krok 1-3 razem — millis() przed/po, ten
     sam wzorzec co poprzednie DIAG-*-STALL. Próg 150ms
     (FLASH_STALL_THRESHOLD_MS, wspólny). Log [WARN] [FLASH-STALL]
     site=fsGuard przy przekroczeniu + liczniki
     g_fsGuardStallCount/g_fsGuardStallMaxMs w heartbeacie.
     NIE jest to fix — zachowanie fsGuardPending (w tym nowej wysyłki na
     Telegram) bez zmian, sam pomiar.
     DO ZWERYFIKOWANIA NA URZĄDZENIU: przy kolejnym crashu z dowolnym z
     trzech checkpointów (pompa/events/DAILY-SUMMARY) sprawdzić, czy
     [FLASH-STALL] site=fsGuard pojawił się w tym samym oknie. Jeśli tak —
     sprawdzić dodatkowo, czy w tym momencie działo się jednocześnie
     sendTelegramDocument() (tgEnabled + FS > FS_LIMIT), żeby odróżnić
     "wolna wysyłka sieciowa" od "wolny remove()/rename()".

### v159 (2026-07-22) - FIX-LOG-SEND-TRIGGER: zmiana wyzwalacza wysyłki logów na Telegram
     Poprzednio: logDailySummary() o 00:00 zawsze wrzucała TG_LOGS do kolejki, niezależnie
     od zapełnienia LittleFS. Logi NIGDY nie były kasowane po udanej wysyłce — jedynym
     mechanizmem zwalniania miejsca była rotacja fsSizeGuard() (log_b -> log_a, log_a
     kasowany bezpowrotnie) przy przekroczeniu FS_LIMIT (~79%), całkowicie niezależna
     od tego, czy i kiedy cokolwiek trafiło na Telegram.
     Teraz: usunięto wysyłkę o północy z logDailySummary() (reszta funkcji — statystyki
     dzienne/tygodniowe/miesięczne — bez zmian). W tgTaskFn, w bloku fsGuardPending
     (wyzwalanym przez fsSizeGuard() gdy FS > FS_LIMIT, czyli realnie się zapełnia),
     dodano próbę sendTelegramDocument() PRZED starą rotacją:
       - sukces wysyłki -> log_a+log_b kasowane w całości (ten sam wzorzec co ręczne
         "Wyczyść logi" / logClearPending) - więcej odzyskanego miejsca niż sama rotacja.
       - porażka wysyłki (brak WiFi, tgEnabled=false, Telegram nie odpowiada) lub
         porażka samego kasowania po wysyłce -> _autoSendOk=false -> leci dokładnie
         ten sam stary system rotacji/nadpisywania (Krok 1-3), bez żadnych zmian.
     Manualna komenda /sendlogs (TG_LOGS) działa bez zmian - to osobna ścieżka.
### v158 (2026-07-21) - DIAG-EVENTSSTALL: diagnostyka nowego wątku "[L:events]"
     z log_combined__12_.txt (03:08:04, WDT_TASK, poprzedni cykl
     uptime=14121s). Ten sam zewnętrzny wzorzec co poprzednie crashe (TG
     dead-socket → FB TIMEOUT → Circuit OPEN → cooldown 29s→0s → crash
     tuż po "Half-open: probuje FB-CMD test"), ale checkpoint tym razem
     to [L:events], NIE [L:pompa] — i co ważne, ŻADEN z trzech liczników
     diagnostycznych z v155-157 nie pokazał spike'a w heartbeacie tuż
     przed crashem (tgSendStallCnt=0, wsCleanupStallCnt=0,
     flashStallMaxMs=243ms — dwa rzędy wielkości poniżej progu WDT). Więc
     żadna z trzech dotąd zmierzonych hipotez nie tłumaczy TEGO crasha.
     NIE jest to fix, tylko pomiar — identyczny wzorzec co
     DIAG-TGSENDSTALL (v155) / DIAG-WSCLEANUP (v156) / DIAG-FLASHSTALL (v157).
     ANALIZA KODU: [L:events] to checkpoint LOOP_CP zapisywany tuż przed
     sekcją "6️⃣ EVENTY - ZMIANY STANÓW" w loop() (Core 1), obejmującą
     warunkowe wywołanie onSectionsChange() (przy zmianie `sections`).
     Przegląd onSectionsChange()/saveFadeToEEPROM()/saveScheduleToEEPROM()
     nie znalazł oczywistej blokady — wszystkie zapisy EEPROM są odroczone
     przez g_UNUSED_eepromCommitPending_DO_NOT_USE (brak synchronicznego I/O w tym miejscu).
     CO ZOSTAŁO DODANE: zmierzono czas trwania KAŻDEGO wywołania
     onSectionsChange() — millis() przed/po, ten sam wzorzec co
     FLASH-STALL/TG-SEND-STALL/WS-CLEANUP-STALL. Próg
     EVENTS_STALL_THRESHOLD_MS=150 (spójny z FLASH_STALL_THRESHOLD_MS).
     Po przekroczeniu: log [WARN] [EVENTS-STALL] (ms=, sections=) +
     liczniki g_eventsStallCount/g_eventsStallMaxMs, raportowane też
     w heartbeat 💓[HEAP] (nowe pola eventsStallCnt/eventsStallMaxMs).
     DO ZWERYFIKOWANIA NA URZĄDZENIU (nie da się potwierdzić statycznym
     przeglądem kodu): obserwować log przy następnym crashu z checkpointem
     [L:events] — czy [EVENTS-STALL] pojawia się w tym samym oknie.
     Jeśli tak → winowajca potwierdzony, projektować fix. Jeśli nie →
     hipoteza wykluczona, [L:events] wymaga dalszego badania od zera
     (możliwe że blokada jest w samym warunku `sections != lastSections`
     lub w kodzie PRZED checkpointem "events", nie po nim — patrz plan
     WDT, sekcja "Nowe dane z urządzenia").

### v157 (2026-07-20) - DIAG-FLASHSTALL: diagnostyka wątku "[L:pompa]"
     z planu poprawek WDT (Priorytet 0/1, "zbadać osobno" — dwukrotny
     WDT-crash w module pompy, log 18, ~29x w pętli). NIE jest to fix,
     tylko pomiar — identyczny wzorzec co DIAG-TGSENDSTALL (v155) i
     DIAG-WSCLEANUP (v156).
     ANALIZA KODU: [L:pompa] to checkpoint LOOP_CP zapisywany TUŻ PRZED
     wywołaniem updatePump() (~linia 14523/14527). updatePump() sam w
     sobie jest w pełni nieblokujący — digitalWrite() (bezpośredni zapis
     rejestru), arytmetyka getLocalMinutes()/isDaylightSaving(), pętla
     bounded po slotach pompy, żadnego I/O, żadnego mutexa. Skoro ostatni
     checkpoint to "pompa", a funkcja wywoływana po nim nie ma nic co
     mogłoby zablokować się na >10s (próg WDT) — winowajcy szukano
     w miejscu, gdzie go nie ma.
     RESEARCH: dokumentacja ESP-IDF ("Concurrency Constraints for Flash
     on SPI1", docs.espressif.com) potwierdza, że operacja zapisu/erase
     na flash SPI1 wymaga wyłączenia cache instrukcji na OBU rdzeniach
     jednocześnie (nie tylko na rdzeniu wykonującym operację) — drugi
     rdzeń jest wstrzymywany (spinlock) na czas trwania operacji. Zmierzony
     przykład z issue #3782 (micropython/micropython, ESP32) pokazuje
     ~80ms zawieszenia CAŁEGO systemu (w tym drugiego rdzenia) na
     pojedynczy zapis strony 4KB. logToFile()/logFlushCore1() w tym
     firmware piszą do LittleFS WYŁĄCZNIE na Core 0 (tgTask) — ale loop()
     z updatePump() biegnie na Core 1. Jeśli zapis LittleFS na Core 0
     (bufor do 8KB, ewentualnie dłużej przy fragmentacji/GC na prawie
     pełnym FS — patrz Priorytet 3a, [FS-GUARD]) zbiegnie się w czasie
     z iteracją loop() na Core 1, LOOP_CP nie zdąży zapisać checkpointu
     "diag" mimo że updatePump() już się wykonał — a black-box
     PRE_RESET_CP_L nadal pokaże "pompa" jako ostatni checkpoint, myląco
     wskazując na funkcję, która sama w sobie jest niewinna.
     CO ZOSTAŁO DODANE: zmierzono czas trwania KAŻDEGO zapisu LittleFS
     (LittleFS.open+write+close) w dwóch miejscach: logToFile() (Core 0,
     bezpośredni zapis) i logFlushCore1() (Core 0, flush bufora Core 1)
     — millis() przed/po, ten sam wzorzec co TG-SEND-STALL/WS-CLEANUP-STALL.
     Próg FLASH_STALL_THRESHOLD_MS=150 (powyżej udokumentowanych ~80ms/4KB,
     margines na wolniejsze/starsze moduły flash). Po przekroczeniu: log
     [WARN] [FLASH-STALL] (site=logToFile|logFlushCore1, ms=, bytes=) +
     liczniki g_flashStallCount/g_flashStallMaxMs, raportowane też
     w heartbeat 💓[HEAP] (obok tgSendStallCnt i wsCleanupStallCnt).
     UWAGA IMPLEMENTACYJNA: log stallu leci przez Serial.printf(), NIE
     przez logPrintf()/logToFile() — jesteśmy fizycznie wewnątrz funkcji
     zapisującej do LittleFS, rekurencyjne wywołanie kolejnego zapisu
     tuż po zmierzonym stallu tylko zafałszowałoby pomiar i pogłębiło
     badany problem.
     ZERO zmiany zachowania: brak nowego timeoutu, brak nowej logiki —
     zapis LittleFS woła się identycznie jak w v156, tylko owinięty
     pomiarem.
     >>> WSKAZÓWKA DLA PRZYSZŁEJ ANALIZY LOGÓW (AI lub człowiek): <<<
     Jeśli [FLASH-STALL] pojawia się w logu W TYM SAMYM OKNIE CZASOWYM
     co [L:pompa]/WDT-crash (porównać ms= z obu wpisów) — to potwierdza
     hipotezę cross-core flash stall jako realną przyczynę i trzeba
     zaprojektować fix (np. throttling/batchowanie zapisów LittleFS,
     albo przeniesienie logToFile() poza okno czasowe najbliższe
     updatePump()). Jeśli [FLASH-STALL] występuje, ale NIGDY w oknie
     crashy pompy — hipoteza wykluczona, [L:pompa] wymaga dalszego
     badania od zera (np. realny sprzętowy problem z GPIO/relay pompy).
     Jeśli [FLASH-STALL] nie występuje wcale — mechanizm cache-disable
     nie jest w praktyce wystarczająco długi na tym FS, żeby być
     problemem, mimo że teoretycznie istnieje.

v156-poprawka (2026-07-20) - FIX-FW-VERSION-DRIFT: #define FW_VERSION
     (~linia 3614, źródło pola "fwVersion" w /api/status i na Dashboardzie
     od v142) oraz hardkodowany banner startowy w setup() (~linia 10542,
     log "📦 Wersja firmware: ...") nadal wskazywały "v155", mimo że plik
     zawierał już pełen changelog i kod v156 (DIAG-WSCLEANUP niżej).
     Efekt: Dashboard i log startowy raportowałyby fałszywie starszą
     wersję niż faktycznie wgrany firmware — myląco przy analizie logów
     z urządzenia (dokładnie ten typ niespójności, przed którym miał
     chronić FW_VERSION jako "jedno miejsce do zmiany", patrz komentarz
     v142 przy tej stałej). FIX: oba miejsca zaktualizowane na "v156
     (2026-07-20)"; stary wpis v155 przesunięty w bannerze startowym do
     listy pozycji historycznych poniżej nagłówka. Zero zmian
     funkcjonalnych — czysto kosmetyczna korekta metadanych.

### v156 (2026-07-20) - DIAG-WSCLEANUP: diagnostyka Priorytetu 5b z planu
     poprawek WDT — checkpoint czasowy wewnątrz wywołania
     wsTerminal.cleanupClients() (patrz komentarz [v125] "podejrzany #1"
     przy tym wywołaniu, ~linia 13941). NIE jest to fix, tylko pomiar —
     identyczny wzorzec co DIAG-TGSENDSTALL w v155.
     KONTEKST: log 17 sugerował, że główną przyczyną blokad >10s jest
     sieć (TG dead socket, patrz Priorytet 0/1), nie WebSocket/cleanup —
     ale to nigdy nie zostało bezpośrednio zmierzone, tylko wywnioskowane.
     RESEARCH: przejrzano źródło AsyncWebSocket::cleanupClients()
     (me-no-dev/ESPAsyncWebServer, plik AsyncWebSocket.cpp) — funkcja to
     pojedyncze przejście listy `_clients` (std::list), sprawdzenie
     shouldBeDeleted() na każdym i ewentualny erase()/close() najstarszego
     klienta gdy jest ich za dużo. Przy typowej liczbie klientów WS w tym
     projekcie (1-2, panel WWW) czas powinien być pomijalny (<<1ms) —
     ALE close()/erase() może zejść w kod AsyncTCP (LwIP), gdzie teoretycznie
     możliwe są opóźnienia przy niestabilnej sieci (zob. też GitHub
     me-no-dev/ESPAsyncWebServer #1334 — WDT na tasku async_tcp pod dużym
     obciążeniem, inny mechanizm niż tu, ale potwierdza, że stos WS/TCP
     potrafi się zablokować pod pewnymi warunkami). Komentarz [v125]
     "podejrzany #1 — potencjalne 200-500ms" nie miał nigdy pomiaru na
     żywym urządzeniu — tylko podejrzenie.
     CO ZOSTAŁO DODANE: zmierzono czas trwania KAŻDEGO wywołania
     wsTerminal.cleanupClients() (millis() przed/po, ten sam wzorzec co
     TG-SEND-STALL). Próg WS_CLEANUP_STALL_THRESHOLD_MS=200 (dolna granica
     z podejrzenia [v125] "200-500ms" — celowo niżej niż próg 2000ms przy
     TG-SEND-STALL, bo cleanupClients() z natury powinien być rzędu <1ms;
     200ms i więcej to już wyraźna anomalia warta zalogowania).
     Gdy przekroczony: log [WARN] [WS-CLEANUP-STALL] (ms=), plus dwa nowe
     liczniki: g_wsCleanupStallCount (ile razy) i g_wsCleanupStallMaxMs
     (najdłuższy zaobserwowany), raportowane też w heartbeat 💓[HEAP]
     (obok tgSendStallCnt/tgSendStallMaxMs z v155 — ten sam wzorzec).
     ZERO zmiany zachowania: brak nowego timeoutu, brak nowej logiki —
     cleanupClients() woła się identycznie jak w v155, tylko owinięte
     pomiarem.
     >>> WSKAZÓWKA DLA PRZYSZŁEJ ANALIZY LOGÓW (AI lub człowiek): <<<
     Jeśli [WS-CLEANUP-STALL] pojawia się w logu — to dowód, że
     cleanupClients() realnie bywa wolne i Priorytet 5b ("wsCleanup()
     audyt" w planie) trzeba podnieść z powrotem z priorytetu ⚪ na
     wyższy, z docelowym fixem (np. ograniczenie częstotliwości wywołań
     przy wykrytej niestabilności sieci, albo przeniesienie na Core 0
     obok reszty operacji sieciowych). Jeśli NIE pojawia się wcale przez
     kilka tygodni — potwierdza to wniosek z planu, że wsCleanup() nie
     jest realnym źródłem blokad >10s, i Priorytet 5b można formalnie
     zamknąć jako "zbadane i wykluczone" zamiast "niezbadane".

### v155 (2026-07-20) - DIAG-TGSENDSTALL: diagnostyka Priorytetu 0/1 (residual
     risk po tgWriteGuard, v150) — NIE jest to fix, tylko pomiar, czy
     pozostałe ryzyko realnie występuje w praniu, zanim podejmie się
     decyzję o migracji tgPost() na inny stos (patrz plan_poprawek_WDT.md,
     "Realne opcje na pełny fix Priorytetu 0/1").
     KONTEKST: tgWriteGuard() (v150) łapie przypadek "gniazdo od razu
     niezapisywalne" PRZED printf()/print(), ale NIE łapie przypadku
     "select() mówi gotowe, ale peer przestaje ACK-ować W TRAKCIE samego
     send()" — to jest właśnie ten residual risk, który dotąd był tylko
     teorią, bez żadnego pomiaru w realnym logu.
     CO ZOSTAŁO DODANE: zmierzono czas trwania KAŻDEGO tgClient.printf()
     (nagłówki) i tgClient.print(body) w tgPost() (millis() przed/po).
     Próg TG_SEND_STALL_THRESHOLD_MS=2000 (2s — wyraźnie dłużej niż
     normalny send() na żywym gnieździe, wyraźnie krócej niż 10s WDT).
     Gdy przekroczony: natychmiastowy log [WARN] [TG-SEND-STALL] (site=
     headers/body, ms=, mimo że tgWriteGuard properł wcześniej), plus
     dwa nowe liczniki: g_tgSendStallCount (ile razy) i g_tgSendStallMaxMs
     (najdłuższy zaobserwowany send), raportowane też w heartbeat 💓[HEAP]
     (analogicznie do g_tlsMutexWaitTG/FB — ten sam wzorzec liczników).
     ZERO zmiany zachowania: brak nowego timeoutu, brak nowego abort —
     tylko pomiar. tgPost() działa identycznie jak w v154.
     >>> WSKAZÓWKA DLA PRZYSZŁEJ ANALIZY LOGÓW (AI lub człowiek): <<<
     Jeśli w logu pojawi się [TG-SEND-STALL] powtarzalnie (nie
     pojedynczo-sporadycznie) — to jest DOWÓD, że residual risk z
     komentarza przy tgWriteGuard() (~linia 5089 w tym pliku) realnie
     się materializuje, a nie jest tylko teoretyczny. W takim wypadku
     NASTĘPNY KROK to migracja tgPost() na HTTPClient z http.setTimeout()
     (patrz plan_poprawek_WDT.md, Priorytet 0/1, punkt "pełny fix bez
     residual risk") — ale NIE na surowy ESP_SSLClient/BearSSL bez dalszego
     researchu (odrzucone w tej samej dyskusji: nowy stos TLS = nowe,
     nieprzetestowane ryzyko na żywej akwarystyce, patrz uzasadnienie w
     rozmowie poprzedzającej v155). Jeśli [TG-SEND-STALL] NIE pojawia się
     wcale przez kilka tygodni normalnej pracy — to sygnał, że residual
     risk jest w praktyce pomijalny i migracja tgPost() może zostać
     odłożona/zamknięta jako priorytet.

### v154 (2026-07-20) - FIX-PRERESET-FLIPFLOP: Priorytet 4b (Zmiana 2) z planu
     poprawek WDT — w logu `src` w [PRE-RESET] miotał się między
     "HTTP/soft" a "WDT(brak danych)" mimo że restart był ręczny/software'owy.
     ROOT CAUSE (potwierdzony w kodzie): tgTaskFn (Core 0) woła
     PRE_RESET_UPDATE(2) co 2s jako heartbeat, niezależnie od loop() (Core 1).
     Gdy loop() robi jawny restart (src=0 soft/HTTP lub src=1 DIAG-17), między
     zapisem PRE_RESET_UPDATE(0/1) a ESP.restart() jest delay(100) na flush —
     w tym oknie tgTaskFn na drugim rdzeniu nadal się kręci i jeśli akurat
     mija 2s od ostatniego heartbeatu, nadpisuje resetSrc z powrotem na 2,
     zanim restart faktycznie nastąpi.
     RESEARCH: klasyczny cross-core race na współdzielonej zmiennej bez
     synchronizacji (ESP32 dual-core, dwa rdzenie faktycznie równoległe, nie
     time-sliced) — powszechnie opisywany wzorzec w dokumentacji
     FreeRTOS/ESP-IDF (mutex/spinlock jako właściwe rozwiązanie ogólne).
     Tu zastosowano lżejszy wariant: `g_restartPending` to write-once bool
     (ustawiany raz, nigdy nie wraca do false przed restartem), więc pełny
     mutex nie jest wymagany — jedyny możliwy "wyścig" to spóźniony o <1
     iterację heartbeat, który i tak nadpisałby resetSrc tą samą wartością.
     FIX: nowa `volatile bool g_restartPending`, ustawiana TUŻ PRZED
     PRE_RESET_UPDATE(0)/(1) w obu miejscach jawnego restartu; heartbeat w
     tgTaskFn sprawdza flagę i pomija PRE_RESET_UPDATE(2), gdy jest true.
     DO ZWERYFIKOWANIA NA URZĄDZENIU: kilka restartów ręcznych/HTTP i DIAG-17
     pod rząd — potwierdzić w logu boot, że [PRE-RESET] src już się nie
     miota (zawsze zgodny z rzeczywistą przyczyną restartu).

### v153 (2026-07-20) - FIX-CRASH-BACKOFF: backoff (4-32s) przed FB+TG reinit
     gdy crash-loop (streak>=2) + retry DNS pre-warm w FB-INIT (2 próby,
     500ms).

### v152 (2026-07-20) - FIX-FS-CAPACITY: Priorytet 3a z planu poprawek WDT —
     FS-GUARD liczył % zajętości LittleFS względem zaszytej na sztywno
     stałej 10420224 B, choć realna pojemność wolumenu jest o 65536 B
     (64 KB) mniejsza od v113 (wydzielenie osobnej partycji coredump
     ukroiło kawałek z partycji LittleFS) — dokładnie ten offset widać
     w logu 17: usedBytes() plateau na 10354688 B, 10/13 pomiarów.
     RESEARCH: znany, udokumentowany wzorzec LittleFS/littlefs-project —
     usedBytes() zamraża się na identycznej wartości tuż przed "No more
     free space" (GitHub lorol/LITTLEFS#1), a remove()/rename() mogą
     cicho zawieść gdy nie ma wolnych bloków na commit metadanych
     (GitHub littlefs-project/littlefs#533, #1011) — dotąd kod FS-GUARD
     nie sprawdzał wyników tych wywołań.
     FIX: 1) nowa globalna g_fsTotalBytes cache'owana raz z realnego
     LittleFS.totalBytes() zaraz po mount w setup(), z logiem ostrzegawczym
     jeśli różni się od starego założenia; 2) wszystkie miejsca liczące
     SMALL_FILE_LIMIT (10%) i logi %/JSON API używają teraz g_fsTotalBytes
     zamiast zaszytej stałej; 3) kroki 1-2 czyszczenia (rotacja log_b,
     usunięcie history_old.csv) sprawdzają teraz wynik remove()/rename()
     i logują błąd zamiast zakładać cichy sukces; 4) nowy marker
     [FS-GUARD-CRITICAL] gdy po całym przycinaniu dysk nadal >=97% pełny —
     od razu widać w logu czy przycinanie faktycznie nie działa.
     FS_LIMIT (8232000 B, ~79% realnej pojemności) pozostaje bez zmian —
     margines ~2.1MB nadal bezpieczny.

### v151 (2026-07-20) - REMOVE-DIAG20: Priorytet 5a z planu poprawek WDT —
     usunięcie fałszywie-alarmującej heurystyki [DIAG-20] (kolizja adresów
     1-Wire DS18B20).
     DECYZJA (nie tylko poprawka progu): diagTempSensorCollision()
     ostrzegała, gdy Płyta1 i Płyta2 zaokrąglają się do tej samej
     temperatury (różnica <0,3°C) przez >30 min w dzień — ale to DWIE
     płyty grzewcze W TEJ SAMEJ obudowie, poddane temu samemu prądowi
     otoczenia i zbliżonej mocy LED, więc identyczny odczyt jest
     fizycznie normalny, nie anomalią. RESEARCH: fabryczne 64-bit ROM ID
     DS18B20 są unikalne, kolizje adresów na tym samym poziomie 1-Wire są
     skrajnie rzadkie (por. dokumentacja DS18B20 / Maxim — kolizja bus-owa
     występuje tylko przy komendzie Skip ROM z >1 urządzeniem, nie przy
     odczycie po adresie, jak robi to ten firmware). Log 16 potwierdził to
     wprost: zrzut ROM w tym samym momencie co alarm pokazał dwa różne,
     poprawne adresy — czyli mechanizm generował fałszywe alarmy z
     założenia konstrukcyjnego (sama heurystyka "identyczny odczyt =
     kolizja" nie ma sensu przy dwóch płytach w jednej obudowie), nie da
     się tego naprawić podkręcaniem progu/histerezy.
     FIX: usunięto całą funkcję diagTempSensorCollision() (definicja +
     2x forward declaration), wywołanie w pętli głównej, oraz nagłówek
     sekcji DIAG-20. ROM-dump z [FIX-v132-DS18B20-ROMADDR] (przydatny do
     diagnostyki 1-Wire ogólnie) poszedł razem z usuniętym blokiem — nie
     był używany nigdzie indziej.

### v150 (2026-07-20) - FIX-WDT-WRITEGUARD: Priorytet 0/1 z planu poprawek WDT
     (druga warstwa, po v148 KA-window) — dodatkowy pre-check PRZED
     tgClient.printf()/print() w tgPost().
     RESEARCH (potwierdzony w GitHub arduino-esp32, patrz plan_poprawek_WDT.md):
     WiFiClient::write() (transport mbedTLS) sam robi select()+send() w
     pętli do WIFI_CLIENT_MAX_WRITE_RETRY=10x WIFI_CLIENT_SELECT_TIMEOUT_US=1s
     (issue #8303) — zaszyte na sztywno w bibliotece, ignoruje
     setTimeout()/SO_SNDTIMEO (issue #7356), i NIE da się przekonfigurować
     przez build_flags (issue #2555, stałe nie są za #ifndef) bez
     patchowania samej biblioteki. Realne ryzyko: pojedynczy printf()/print()
     może legalnie zająć do ~10s, tgPost() robi to 2x z rzędu (nagłówki+body)
     — stąd crashe WDT grupowały się właśnie wokół TG-POST.
     FIX: nowa tgWriteGuard() (~linia 4980) — select() na zapis z własnym,
     krótkim timeoutem (1500ms) TUŻ PRZED każdym printf()/print() w
     tgPost(). Gdy gniazdo nie jest zapisywalne w tym oknie: zero próby
     send, tgClient.stop() + wymuszony reconnect przy następnym wywołaniu,
     zamiast wejścia w wewnętrzną 10s pętlę biblioteki.
     UCZCIWIE: to NIE jest 100% fix (patrz komentarz przy tgWriteGuard())
     — chroni przed martwym/pół-otwartym gniazdem wykrytym PRZED wysyłką,
     nie przed peerem, który przestaje ACK-ować W TRAKCIE aktywnego
     send(). Pełny fix = migracja tgPost() na HTTPClient (jak FB),
     wciąż otwarte jako opcjonalny, większy refaktor.

### v149 (2026-07-20) - FIX-v149-TG-OFFSET-RTC: Priorytet 4 z planu poprawek WDT —
     pętla restartów przez ponowne dostarczanie starego update'u Telegrama.
     ROOT CAUSE: tgLastUpdateId był zwykłą zmienną RAM, zerowaną przy KAŻDYM
     restarcie (soft/WDT/panic). Telegram nie ma potwierdzenia odbioru starych
     update'ów — po restarcie odsyła cały niepotwierdzony backlog (w tym stary
     np. "rstyes"), firmware wykonuje restart jeszcze raz. Potwierdzone 3x w
     logach: każde legalne polecenie /restart kosztowało 4 reboot-y zamiast 1.
     FIX: nowa RTC_NOINIT_ATTR long g_tgLastUpdateId (ten sam wzorzec co
     g_preReset/g_wdtHunt od v134) — przeżywa SW/WDT/PANIC, zerowana (-1)
     TYLKO przy realnym POWER_ON/UNKNOWN. tgLastUpdateId (RAM) synchronizowany
     z g_tgLastUpdateId zaraz po bloku zerowania w setup(). Zapis do RTC RAM
     przeniesiony do miejsca przetwarzania update'u (max(tgLastUpdateId, uid)),
     PRZED jakąkolwiek akcją (np. RESTART_DO) — update jest "skwitowany" w
     RTC RAM, zanim może dojść do restartu.
     Nie ruszone: flip-flop src w [PRE-RESET] (Zmiana 2, drugorzędna,
     niekrytyczna) — zostaje jako osobny, otwarty punkt w planie.

### v148 (2026-07-20) - FIX-TG-KA-WINDOW: TCP keepalive dla tgClient miał parametry
     idle=5s/interval=5s/count=3 (razem 20s) — dwa razy dłużej niż okno WDT (10s).
     Ta sama poprawka co dla Firebase w v112 (idle=3/interval=2/count=3 → 9s,
     mieści się w oknie WDT) nigdy nie trafiła do Telegrama. Tłumaczy to, czemu
     crashe WDT grupowały się wokół tgPost()/sendTelegramDocument(), a nie FB:
     Firebase wykrywał martwe gniazdo szybciej niż mógł strzelić watchdog,
     Telegram — nie.
     FIX: applyTcpKeepalive(tgClient, 5, 5, 3) → applyTcpKeepalive(tgClient, 3, 2, 3)
     w dwóch miejscach: tgEnsureConnected() (~linia 5058) i
     sendTelegramDocument() (~linia 5478). Minimalna zmiana, bez nowej logiki —
     identyczny wzorzec, który już sprawdził się dla FB.
     Uwaga: to NIE usuwa ryzyka zawieszenia się write()/printf() wewnątrz
     send_ssl_data() (patrz Priorytet 0/1 w planie poprawek WDT) — to osobny,
     głębszy problem w tgPost(), wciąż otwarty.

### v147 (2026-07-07) - FIX-v147-PRERESET-TIMING: [PRE-RESET] BLACK BOX faktycznie
     trafia teraz do log_b.txt.
     ROOT CAUSE: mimo poprawnych danych w RTC RAM (fix v134, RTC_NOINIT_ATTR),
     blok odczytu+druku [PRE-RESET] w setup() wykonywał się PRZED LittleFS.begin()
     i PRZED startem serwera WWW — czyli przy littlefsReady==false i wsReady==false.
     logPrintln/logPrintf w tym stanie: logToFile() ma "if(!littlefsReady) return;"
     (early-return), a gałąź WiFi Terminal wymaga wsReady. Jedyny działający kanał to
     Serial (LOG_BOTH domyślnie) — więc "📦 [PRE-RESET]" i wariant "Brak danych" leciały
     WYŁĄCZNIE na USB, nigdy do trwałego logu. Stąd ta linia nie pojawiała się ANI RAZU
     w żadnym eksporcie logów, nawet po tym jak dane w RTC RAM były już poprawne.
     FIX: odczyt+druk [PRE-RESET]/[WDT-HUNT] przeniesiony z ~linii 10188 do bloku
     "if (littlefsReady) {...}" zaraz po LittleFS.begin() (za logowaniem przyczyny
     restartu, FIX-v128). Samo zerowanie g_preReset/g_wdtHunt przy POWER_ON/UNKNOWN
     zostaje w oryginalnym, wczesnym miejscu (nie zależy od littlefsReady/wsReady i
     musi zdążyć przed odczytem). g_preReset/g_wdtHunt to RTC_NOINIT_ATTR, więc dane
     przetrwały bez zmian przeniesienie punktu odczytu o ~250 linii kodu w tym samym
     setup(). Odkryte podczas audytu, czemu [PRE-RESET] nadal nie pojawiał się w logu
     mimo poprawki v134 — dwa niezależne bugi na tej samej ścieżce.

### v146 (2026-07-07) - CLEANUP-REMOVE-UNUSED-PAGES: usunięto całkowicie
   nieużywane podstrony na życzenie użytkownika (korzysta wyłącznie
   z /terminal): /control (cały panel, ~1100 linii generowania HTML),
   /wifi-settings (zbędne od v145 - WiFi wbudowane w /terminal),
   /webserial (stary alias -> /terminal), /view-log (stary alias
   -> /control), /clear-log (stara wersja GET czyszczenia logów,
   zastąpiona przez /api/log-clear POST). Poprawiono przy okazji:
   - /set-pwm przekierowywał na końcu do usuniętego /control -
     zmieniono na /terminal, żeby nie było martwego linku.
   - usunięto nieużywane już #define HTML_BUF_CAP (bufor PSRAM
     istniał wyłącznie na potrzeby /control).
   - w terminal_html.cpp: href='/control' w przycisku "Podgląd logów"
     (Szybkie akcje) zamieniony na href='#' - był tylko fallbackiem,
     JS i tak zawsze robi preventDefault() i przełącza zakładkę.
   Zero zmian w API JSON (/api/*), w logice sterowania LED/pompką/
   Telegramem ani w samym /terminal - wszystkie funkcje nadal
   dostępne z jednego miejsca.

### v145 (2026-07-07) - FEATURE-WIFI-INLINE: karta "Sieci WiFi" na /terminal
   przestała być linkiem do osobnej podstrony (/wifi-settings) i działa
   teraz DOKŁADNIE tak jak reszta ustawień - w pełni wbudowana w tę samą
   stronę, ten sam styl (row/badge/save-btn), bez otwierania nowej karty
   przeglądarki. PROBLEM: poprzednia wersja (v143/v144) miała tylko
   przycisk "Otwórz ustawienia WiFi" prowadzący do /wifi-settings -
   użytkownik wprost tego nie chciał, oczekując jednej, spójnej strony.
   ZMIANA: cały HTML formularza (lista zapisanych sieci, dodawanie,
   skanowanie z kreskami sygnału) przeniesiony do terminal_html.cpp,
   z nowymi funkcjami JS wifiLoadList()/wifiAddNet()/wifiDelNet()/
   wifiStartScan()/wifiPollScan()/wifiPickScan() korzystającymi z tych
   samych endpointów API co poprzednio (/api/wifi/list, /add, /delete,
   /scan/start, /scan/result) - zero zmian w backendzie tego pliku poza
   FW_VERSION. Endpoint /wifi-settings w tym .cpp NIE został usunięty
   (zero ryzyka regresji dla innych ew. odbiorców), po prostu przestał
   być potrzebny z poziomu /terminal.

### v144 (2026-07-07) - FEATURE-WIFI-SIGNAL-BARS: dodano wizualne "kreski"
   siły sygnału (jak w telefonie, 4 poziomy) obok wartości dBm na stronie
   /wifi-settings - zarówno w liście "Skanuj dostępne sieci", jak i przy
   aktualnie połączonej sieci w "Zapisane sieci" (jedyna zapisana sieć,
   dla której RSSI jest znane bez uruchamiania skanu - reszta zapisanych,
   ale niepołączonych sieci, nie ma znanej siły, dopóki nie pojawi się
   w wynikach skanu). Backend: wifiNetworksListJson() (~linia 4784) zwraca
   teraz dodatkowo "rssi" dla aktywnej sieci (WiFi.RSSI()). Frontend:
   nowa funkcja JS sigBars(rssi) mapuje dBm na 0-4 wypełnionych kresek
   (progi: -50/-60/-70/-80 dBm) i renderuje je w obu listach. Zero zmian
   w API /api/wifi/scan/result (rssi już tam było) ani w logice łączenia.

### v143 (2026-07-07) - FEATURE-TERMINAL-WIFI-CARD: dodano kartę "Sieci WiFi"
   (identyczną jak na /control) bezpośrednio w terminal_html.cpp, czyli
   na stronie serwowanej pod /terminal. PROBLEM: kafelek "Sieci WiFi"
   z v141 trafił tylko do /control - a /terminal to osobny plik źródłowy
   (terminal_html.cpp/.h, dołączany przez #include poza tym .cpp), będący
   w praktyce niemal pełną kopią tego samego panelu. Użytkownicy korzystający
   wyłącznie z zakładki /terminal nigdy nie widzieli tej zmiany. FIX: karta
   "Sieci WiFi" (link do /wifi-settings) dodana wprost do terminal_html.cpp,
   zaraz po karcie "Szybkie akcje" w zakładce Ustawienia - ten sam HTML/CSS,
   zero nowego JS. Zero zmian w tym pliku .cpp poza samym FW_VERSION.

### v142 (2026-07-07) - FEATURE-FW-VERSION-DISPLAY: wersja firmware widoczna
   na Dashboardzie w panelu WWW (i w /api/status jako "fwVersion").
   Nowa centralna stała #define FW_VERSION (~linia 3308, tuż po sekcji
   INCLUDES) - jedno miejsce do zmiany przy każdym kolejnym release'ie,
   zamiast pamiętać o aktualizacji w kilku miejscach. Wyświetlana w
   karcie na górze zakładki Dashboard oraz zwracana w polu "fwVersion"
   JSON-a z GET /api/status (przyda się pod przyszły live-dashboard).
   Zero zmian w logice biznesowej - czysto informacyjny dodatek.

### v141 (2026-07-07) - FIX-WIFI-UI-DISCOVERABILITY: dodano kafelek "Sieci WiFi"
   w panelu Ustawień (/control), obok "Szybkie akcje". PROBLEM: panel
   /wifi-settings z v139 istniał i działał, ale nigdzie w interfejsie nie
   było do niego linku - trzeba było znać/wpisać URL ręcznie, więc funkcja
   była praktycznie niewidoczna dla użytkownika. FIX: nowa karta w stylu
   identycznym jak pozostałe sekcje (card/card-header/toggleCard) z
   przyciskiem otwierającym /wifi-settings w nowej karcie. Zero zmian w
   JS/logice pozostałych sekcji.

### v140 (2026-07-07) - FIX-WIFI-ATOMIC-SAVE: zapis listy sieci WiFi (/wifi_list.txt)
   odporny na reset/utratę zasilania w trakcie zapisu.
   PROBLEM: saveWifiNetworks() (v139) otwierał docelowy plik w trybie "w",
   co NATYCHMIAST kasuje starą zawartość, po czym dopisywał linie. Reset
   dokładnie w tym oknie (rzadki, ale ten projekt zna to ryzyko aż nazbyt
   dobrze z historii resetów akwarium) zostawiał plik pusty/ucięty ->
   utrata całej listy zapisanych sieci przy najbliższym boocie.
   FIX: dokładnie ten sam wzorzec atomowego zapisu co HIST-COMPACT
   (linia ~9283, /history_tmp.csv): zapis do pliku tymczasowego
   (/wifi_list_tmp.txt) -> stary plik zostaje NIETKNIĘTY dopóki nowy nie
   jest w 100% zapisany i zamknięty -> dopiero wtedy atomowa podmiana
   przez rename() (stary -> /wifi_list_old.txt, tmp -> docelowy). Jeśli
   rename() zawiedzie, poprzednia działająca lista jest automatycznie
   przywracana. Dodano też sprzątanie sierot /wifi_list_tmp.txt i
   /wifi_list_old.txt w setup() (ten sam wzorzec co sierota
   history_tmp.csv), na wypadek crashu w trakcie samej podmiany.
   Zero zmian w formacie pliku ani w API WWW z v139.

### v139 (2026-07-07) - FEATURE-WIFI-MULTI: obsługa wielu sieci WiFi zamiast
   jednego hardkodowanego SSID/hasła (SECRET_SSID/SECRET_OPTIONAL_PASS).
   PROBLEM: firmware łączył się WYŁĄCZNIE z siecią zdefiniowaną na sztywno
   w kodzie (#define SECRET_SSID). Brak tej jednej sieci (awaria routera,
   przeniesienie akwarium, wymiana routera) = urządzenie całkowicie offline
   do czasu ręcznej edycji kodu i ponownego wgrania firmware.
   ZMIANA:
     (1) Nowa lista sieci WiFi trzymana w RAM (wifiNetworks, std::vector)
         i persystowana w LittleFS (/wifi_list.txt, jeden wpis JSON na
         linię — ten sam styl ręcznego parsowania co loadTelegramConfig()).
         Pierwsze uruchomienie / brak pliku → lista inicjalizowana jednym
         wpisem z dotychczasowego SECRET_SSID/SECRET_OPTIONAL_PASS (pełna
         kompatybilność wsteczna, nic się nie psuje bez konfiguracji).
     (2) wifiPickBestNetwork() skanuje eter (WiFi.scanNetworks()) i wybiera
         spośród ZAPISANYCH sieci tę o najsilniejszym sygnale (RSSI), która
         faktycznie jest w zasięgu — więc jeśli w zasięgu jest kilka znanych
         sieci (np. dom + telefon w trybie hotspot), wybierana jest najlepsza.
     (3) wifiBeginBest() zastępuje wszystkie 3 dotychczasowe wywołania
         WiFi.begin(SECRET_SSID, SECRET_OPTIONAL_PASS) (setup() pierwsza i
         druga próba, hard-reconnect w loop(), eskalacja DIAG-14) — bez
         zmiany istniejącej logiki WDT-safe retry/timeout wokół tych wywołań.
     (4) Nowe API WWW do zarządzania listą (panel /wifi-settings):
           GET  /api/wifi/list   - zapisane sieci + aktualnie połączona
           GET  /api/wifi/scan   - skanowanie eteru (SSID + siła sygnału)
           POST /api/wifi/add    - dodaj/zaktualizuj sieć {ssid,pass}
           POST /api/wifi/delete - usuń sieć {ssid} (zawsze zostaje min. 1)
         Hasła nigdy nie są zwracane w /api/wifi/list (tylko SSID + flaga
         czy hasło jest ustawione), żeby nie wyciekały przez panel WWW.
   Zero zmian w logice sensorów/LED/Firebase/Telegram — zmiana ograniczona
   do sekcji WiFi i nowych, wydzielonych endpointów API.

### v138 (2026-07-06) - FIX-WDT-SILENT-FAIL: naprawa fundamentu całego mechanizmu WDT.
   ROOT CAUSE (potwierdzone testem diagnostycznym na płytce docelowej):
   esp_task_wdt_init(&wdt_cfg) w setup() (linia ~9698) ZAWSZE zwracał
   ESP_ERR_INVALID_STATE ("TWDT already initialized") i NIC nie robił, bo
   arduino-esp32 core inicjalizuje TWDT automatycznie PRZED setup()
   (domyślnie CONFIG_ESP_TASK_WDT_TIMEOUT_S=5s). Return value tego
   wywołania nigdy nie było sprawdzane w kodzie.
   SKUTEK: od v101 ("timeout 10s → 15s") firmware w RZECZYWISTOŚCI działał
   z domyślnym ~5-sekundowym oknem TWDT, NIE z zakładanym 15s. Cała logika
   marginesów czasowych zbudowana od v101 do v137 (ITER_BUDGET_MS=10000ms,
   "TCP(7s)+TLS(4s)=11s < 15s", "esp_task_wdt_reset() co 64 linie" itd.)
   zakładała 15s okno, którego realnie nie było — to potencjalnie tłumaczy
   część wcześniejszych crashy WDT, które teoretycznie "nie powinny się
   zdarzyć" przy 15s marginesie.
   TEST POTWIERDZAJĄCY: dedykowany szkic diagnostyczny na płytce docelowej
   (ESP32-S3) potwierdził log "esp_task_wdt_init(517): TWDT already
   initialized", a następnie zmierzył realny czas do restartu po
   esp_task_wdt_reconfigure(15000ms) na busy-loopie bez wdt_reset:
   ~14.5-15.3s całkowitego czasu — zgodnie z konfigurowanym 15000ms.
   FIX: sprawdzenie zwracanej wartości esp_task_wdt_init(); jeśli
   ESP_ERR_INVALID_STATE, wywołanie esp_task_wdt_reconfigure(&wdt_cfg)
   jako fallback, które POPRAWNIE nadpisuje domyślny config na 15000ms/
   panic=true. Zero zmian logiki biznesowej poza tym jednym miejscem.

### v137 (2026-07-06) - FIX-FS-VIEW-WDT: dodatkowe, wcześniej nieodkryte miejsce z tym
   samym problemem co w v136 — endpoint GET /api/fs-view (linia ~12174, podgląd
   ostatnich N linii DOWOLNEGO pliku z LittleFS przez parametr ?file=). Stara wersja
   czytała do 65536 pojedynczych BAJTÓW przez virtual f.read() (content += (char)...)
   bez ANI JEDNEGO esp_task_wdt_reset() — gorszy wariant problemu niż /api/history,
   bo (a) brak buforowania w ogóle (sam narzut per-bajtowego wywołania), (b) dotyczy
   każdego pliku w FS, nie tylko history.csv. Handler biegnie w tasku "async_tcp"
   (ten sam kontekst i to samo uzasadnienie co FIX-API-HISTORY-WDT z v136 — task JEST
   monitorowany przez TWDT domyślnie). Fix: zamiana odczytu bajt-po-bajcie na bufor
   512B + esp_task_wdt_reset() co chunk (poprawia jednocześnie wydajność i WDT).
   Sprawdzono pozostałe endpointy plikowe (/api/log-lines, /api/log-download,
   /api/fs-list) — te już używają bezpiecznych wzorców (streaming callback biblioteki
   lub pojedyncze bulk read()), nie wymagają zmian.
### v136 (2026-07-06) - FIX-v136-WDT-AUDYT: dokończenie audytu z v135 — 2 kolejne miejsca
   z tym samym wzorcem "pętla I/O na pliku history.csv bez esp_task_wdt_reset()":
   (1) FIX-API-HISTORY-WDT: handler GET /api/history (linia ~12056) — biegnie w
       tasku "async_tcp" (AsyncTCP), który JEST monitorowany przez TWDT domyślnie
       (CONFIG_ASYNC_TCP_USE_WDT=1 z biblioteki, projekt tego nie nadpisuje —
       zweryfikowano w źródle AsyncTCP.h). Bez resetu otwarcie zakładki "Historia"
       zaraz po restarcie (histStartLine >1700) mogło crashować async_tcp, blokując
       przy okazji WSZYSTKIE połączenia sieciowe (WiFi/OTA/mDNS) do czasu abortu.
   (2) FIX-HIST-SAVE-WDT: saveHistoryPoint() (linia ~13111) — pierwszy skan pliku
       po restarcie, PRZED HIST-COMPACT. Biegnie w tgTaskFn (już zarejestrowany
       w TWDT), więc to potencjalnie WCZEŚNIEJSZY punkt crashu niż ten naprawiony
       w v135. Dodano esp_task_wdt_reset() co ~16 KB czytanego pliku.
   Oba fixy to ten sam prosty wzorzec co v135 (reset co N iteracji), zero zmian
   logiki biznesowej. Patrz audyt_zabezpieczen_wdt.md sekcja 2a/2b.
### v135 (2026-07-06) - FIX-v135-HIST-COMPACT-WDT: watchdog reset w fallbacku kompaktowania historii.
     ROOT CAUSE (potwierdzone logiem): po restarcie zaplanowane kompaktowanie
     /history.csv ([HIST-COMPACT] START startLine=1738 lineCount=2026) trafiało w
     tgTaskFn (Core 0) w gałąź fallback pomijania martwych linii — histStartByteOffset
     ([v88] PATCH-1) to zwykła zmienna RAM, zerowana przy KAŻDYM restarcie i nigdy nie
     zapisywana na LittleFS/EEPROM. Skoro kompaktowanie jest planowane właśnie po
     restarcie (histStartLine >= HISTORY_MAX_POINTS z poprzedniej sesji), szybka
     ścieżka seek() nigdy nie była dostępna w praktyce — program zawsze wchodził w
     pętlę for(...) fOld.readStringUntil('\n') czytającą histStartLine linii z
     LittleFS BEZ ANI JEDNEGO esp_task_wdt_reset(). Przy 1738 iteracjach (odczyt +
     alokacje String) czas pętli przekroczył limit WDT -> "task_wdt: tgTask + IDLE0
     nie zresetowały watchdoga" -> Abort/reboot, dokładnie jak w logu. Płytka bez
     podłączonego DS18B20 wody nie crashowała wyłącznie dlatego, że w ~2.2h loga nie
     zdążyła jeszcze nazbierać 288 martwych linii wymaganych do zaplanowania
     kompaktowania — to kwestia czasu, nie okablowania; ten sam crash wystąpiłby na
     niej po przekroczeniu progu.
     FIX: esp_task_wdt_reset() co 64 iteracje w pętli fallback (tgTaskFn ~linia 9001).
     Koszt pomijalny (jedno porównanie bitowe na iterację). Ścieżka szybka (seek() gdy
     histStartByteOffset>0) bez zmian — fix dotyczy wyłącznie gałęzi fallback.
     TODO (nie wdrożone w tej wersji): docelowo zapisywać histStartByteOffset razem z
     histStartLine na LittleFS, żeby fallback w ogóle nie był potrzebny po restarcie.

### v134 (2026-07-03) - FIX-v134-RTC-NOINIT: KRYTYCZNA poprawka fundamentu black-boxa.
     ROOT CAUSE (potwierdzone w źródle ESP-IDF cpu_start.c + community ESP32 Forum):
     g_preReset i g_wdtHunt używały RTC_DATA_ATTR, która trafia do sekcji .rtc.bss —
     startup code JĄ ZERUJE przy KAŻDYM restarcie niebędącym wybudzeniem z deep-sleep:
       "if (rst_reas[0] != DEEPSLEEP_RESET) memset(&_rtc_bss_start, 0, ...)"
     Czyli dokładnie przy WDT_TASK/PANIC/SOFTWARE — tych samych, dla których cały
     mechanizm miał działać! To wyjaśnia, czemu [PRE-RESET] od "v71" (i mój nowy
     [WDT-HUNT] z v133, zbudowany na tym samym, jak się okazało wadliwym fundamencie)
     ZAWSZE pokazywały "Brak danych" — magic był zerowany, zanim setup() go odczytał.
     FIX: RTC_DATA_ATTR → RTC_NOINIT_ATTR (sekcja .rtc_noinit, NIE zerowana przez
     startup code, potwierdzone działające dla WDT/PANIC/abort() w wielu niezależnych
     źródłach community). Cena: brak auto-inicjalizacji przy pierwszym zasilaniu —
     usunięto inicjalizator "={0}" (ignorowany przez linker dla sekcji noinit) i
     dodano jawne memset() przy bootResetReason==POWERON/UNKNOWN w setup(), PRZED
     odczytem magic. Walidacja przez pole magic (już istniejąca) dodatkowo chroni.
     Odkryte podczas audytu po serii ~50 restartów WDT_TASK/PANIC w czasie
     długotrwałej (~4h) degradacji sieci — [WDT-HUNT] miał to złapać, ale nie mógł,
     bo bazował na tym samym niedziałającym mechanizmie co oryginalny [PRE-RESET].

### v133 (2026-07-02) - FIX-v133-WDT-HUNT: rozszerza istniejący black-box (PreResetSnap,
     RTC RAM, v71) o widoczność w loop() (Core 1) — dotąd całkowicie ślepy.
     ROOT CAUSE: PRE_RESET_UPDATE(2) działa niezawodnie co 2s w tgTaskFn (Core 0),
     ale WSZYSTKIE 11 checkpointów PRE_RESET_CP(...) są wyłącznie w funkcjach Core 0
     (TG-CONN, FB-SEND/CMD/CFG, TSL-REINIT, hist...). Zero checkpointów w loop() —
     a komunikat WDT jednoznacznie wskazuje "Główna pętla" = loopTask = Core 1.
     Crash 22:51 (WDT_TASK) nie miał żadnego poprzedzającego TIMEOUT/CB — wskazuje
     na lokalną operację w loop() (OTA/WS/1-Wire), nie na sieć — poza zasięgiem CB v131.
     FIX: nowe makro PRE_RESET_CP_L(tag) (prefiks "L:") wpięte w istniejące makro
     LOOP_CP — automatyczne pokrycie ~20 checkpointów loop() bez nowych call site'ów.
     Zero logowania w normalnej pracy (czysty zapis do RTC RAM) — spełnia "bez spamu".
     FIX-v133-WDT-HUNT-STREAK: osobna struktura RTC (g_wdtHunt, przeżywa dłużej niż
     g_preReset, która czyści się po każdym odczycie) wykrywa powtarzający się crash
     w tym samym checkpoincie. Drukuje się TYLKO gdy streak>=2 i TYLKO przy
     potwierdzonym WDT/PANIC (bootResetReason) — pierwszy crash nie dodaje linii logu,
     zwykły restart (HTTP/DIAG-17/POWER_ON) nie rusza licznika.

### v132 (2026-06-30) - FIX-v132-LOGBUF-EARLY: g_logBuf + logMutex przeniesione na początek setup()
     ROOT CAUSE: logToFile() early-return gdy g_logBuf==nullptr → PRE_RESET, SYSTEM START
     i bootResetReason (TASK_WDT) cicho tracone przy każdym restarcie. Fix v128 przesunął
     switch(bootResetReason) po LittleFS.begin() (brama littlefsReady) — ale istniała druga
     brama: g_logBuf alokowany dopiero na linii ~12 535, za WiFi/NTP/sensors/webserver.
     FIX: alokacja g_logBuf[2] + logMutex przeniesiona przed pierwsze logPrintln w setup().
     FIX-v132-DS18B20-ROMADDR: dump ROM sensors1 (Płyta1) i sensors2 (Płyta2) przy starcie
     i w DIAG-20 — pozwala potwierdzić/wykluczyć kolizję adresów 1-Wire (trend 18→29→65).
     FIX-v132-DS18B20-LOOPSLOW: esp_task_wdt_reset() przed każdym requestTemperatures()
     + detekcja stall >200ms — zapobiega LOOP-SLOW 916ms bez zmiany biblioteki.

### v131 (2026-06-25) - FIX-P1-CIRCUIT-BREAKER: eliminacja WDT crashy z kumulacji bloków sieciowych.
     ROOT CAUSE (MD sekcja 4, 7.3, 7.4): suma TG-POST(5s)+FB-CMD(8s)+FB-CFG(10s)>WDT(15s).
     WARSTWA A — ITER-BUDGET: _iterStartMs + ITER_BUDGET_MS=10000ms. Każda operacja FB/TG
       sprawdza budżet przed startem; przekroczenie → skip do następnej iteracji.
       Eliminuje akumulację w jednej iteracji (Restart #1: 13s, Restart #2: 31.5s).
     WARSTWA B — CIRCUIT BREAKER: _netFailStreak licznik (NET_FAIL()/NET_SUCCESS()).
       Po >= 2 consecutive TIMEOUT/ERROR → _netCircuitOpen=true + cooldown 30s (exponential
       backoff do 120s). W stanie open: WDT-safe vTaskDelay(100ms)×N zamiast prób sieciowych.
       Po cooldown: half-open (jedna próba testowa). Sukces → pełny reset.
     FIX-WIFIGUARD-COUNTER: _wgStart/_wgIter resetowane po powrocie WiFi (bug "8516s").
     Dotyczy: tgTaskFn (ITER-BUDGET + circuit check), sendStatusToFirebase,
       checkFirebaseCommands, checkFirebaseConfig, tgPost (NET_FAIL/NET_SUCCESS).

### v130 (2026-06-23) - LOG-QUIET: redukcja spamu logu (3 zmiany):
     LOG-QUIET-WIFIGUARD: [WIFI-GUARD] loguje tylko raz przy wejściu + raz przy wyjściu
     z czasem przerwy i liczbą iteracji. Poprzednio: 60–120 linii na 2-minutową przerwę WiFi.
     LOG-QUIET-TGPOLL: [TG-POLL] START/END usunięte — [TG-POST] getUpdates w środku wystarczy.
     LOG-QUIET-TGALIVE: [TG-ALIVE] co 60s → co 5 minut.

### v129 (2026-06-23) - FIX-PSRAM-TLS-CRASHLOOP (P1, KRYTYCZNY): hook mbedtls_platform_set_calloc_free()
     przeniesiony poza blok if(WiFi.status()==WL_CONNECTED) → wywoływany zawsze, niezależnie
     od stanu WiFi. ROOT CAUSE crashloopa 20:51–20:58: gdy WiFi timeout, blok if(WiFi) był
     pomijany → pierwszy TLS handshake alokował ~44kB z DRAM → heap ~80kB → crash → pętla.
     FIX-WIFI-RETRY-2 (P2, WYSOKI): druga próba WiFi.begin() po 7.5s nieudanej 1. próby
     (arduino-esp32 #2501: po serii SW_CPU_RESET WiFi nie łączy przy 1. próbie). WDT-safe.
     FIX-SETUP-NOWIFI-LOG (P3, ŚREDNI): log [WARN] [SETUP] gdy webserialServer/ArduinoOTA
     nie wystartowały (brak WiFi po 2 próbach) — jednoznaczny sygnał diagnostyczny.
     FIX-FB-SEND-TIMEOUT (P4, NISKI): timeout FB-SEND 8s→12s — margines dla spowolnień
     Firebase (600–1426ms w logach z 20:15–20:22). WDT=15s, bezpieczne.

### v128 (2026-06-22) - FIX-RESETREASON-LOG: przyczyna restartu logowana PO LittleFS.begin()
     — teraz trafia do log_b.txt i /last_reset.txt (poprzednio littlefsReady==false).
     FIX-LOG-TIMESTAMP-MS: ms= w każdej linii logu — sort post-factum po interleave.
     FIX-DOUBLE-CB: prevTrybGlobal (global zamiast static lokalnej) eliminuje
     podwójne onTrybChange() — set-tryb/Firebase/TG sync global, DEBUG-block go widzi.
     FIX-FORMAT-DELTA: delta=%+ldB zamiast delta=-%ldB — fix delta=--80B w TLS-DRAM.

### v127 (2026-06-20) - LOOP-CP-GLOBAL: naprawa błędu v126.
     v126 używał "static" WEWNĄTRZ makra → każdy checkpoint miał WŁASNY timer.
     Efekt: checkpoint wołany co 15s (tempFaza1) raportował 15000ms zamiast
     rzeczywistego czasu wykonania. Fałszywy alarm maskował prawdziwego winowajcę.
     v127: dwie globalne zmienne _lprofT / _lprofN (wspólne dla wszystkich LOOP_CP).
     Każde LOOP_CP mierzy czas od POPRZEDNIEGO LOOP_CP w bieżącej iteracji —
     niezależnie jak rzadko dany checkpoint jest wołany.

### v126 (2026-06-20) - LOOP-CP-ALWAYS: wadliwe (static wewnątrz makra).

### v125 (2026-06-19) - LOOP-SLOW-PROFILER: automatyczny profiler loop() aktywowany gdy
     poprzednia iteracja przekroczyła 300ms (wadliwe — mierzyło następną iterację).

### v124 (2026-06-17) - HEAP-MINLOG: log [INFO] przy nowym historycznym minimum freeHeap — heap miał już alarm krytyczny/fragmentacji/early-warning i auto-restart (v41/v64-P2c), ale BEZ logu przy nowym minimum (jak loopStack/tgStack od v70-DIAG/v122)

 KOREKTA WŁASNEGO BŁĘDU Z POPRZEDNIEJ TURY:
   Wcześniej napisano użytkownikowi "heap jest tylko mierzony, bez alarmu" —
   NIEPRAWDA, sprawdzone dopiero teraz przy faktycznym czytaniu diagHeap().
   Od dawna istnieją: heapCrit (freeH<8192B → alarm 🚨 [DIAG-17], loguje się
   przy KAŻDYM wywołaniu, bez throttlingu — "crash nieuchronny"), heapFrag
   (maxAlloc<8192B przy freeH OK → alarm 🚨 [DIAG-17-FRAG] co 120s — wykrywa
   sytuację gdy suma wolnej pamięci wygląda OK, ale najwięszy ciągły blok jest
   za mały np. dla TLS), early-warning (freeH<50000B → [WARN] co 60s, próg
   przestawiony z 20kB/300s na 50kB/60s w v64-P2c) i auto-restart (v41) gdy
   zły stan trwa >15 min (zawsze) albo >5 min w nocy 03:xx z LEDami wyłączonymi
   (~linia 16238-16267 w tym pliku). "145 KB", o których wspomniano wcześniej,
   to tylko stary cel z roadmapy w komentarzu v89 — nigdy nie był progiem w
   kodzie, nie miałem racji łącząc te dwie rzeczy.

 RZECZYWISTA RÓŻNICA względem loopStack/tgStack (jedyny prawdziwy brak):
   g_heapMinValue (deklaracja ~linia 3533, aktualizacja ~linia 16198) śledzi
   historyczne minimum freeHeap identycznie jak HWM stosów — ale AKTUALIZACJA
   BYŁA CICHA. Nowe minimum było widoczne dopiero w najbliższym 💓[HEAP]
   heartbeat (do 60s później), nie w chwili wystąpienia. loopStack (v70-DIAG)
   i tgStack (v122-TGSTACK-DIAG) logują [INFO] natychmiast przy nowym minimum.

 FIX (diagHeap() ~linia 16197, tracker g_heapMinValue):
   Dodano log [INFO] [HEAP] przy każdym freeH < g_heapMinValue, analogicznie
   do wzorca loopStack/tgStack. Guard "g_heapMinValue != UINT32_MAX" pomija
   pierwszy pomiar po boocie (sentinel startowy z deklaracji — brak sensownego
   punktu odniesienia do pierwszego pomiaru) — ten sam wzorzec jak 0xFFFF dla
   _prevLoopStack/_prevTgStack w v70-DIAG/v122.

 CO ŚWIADOMIE NIE ZMIENIONO:
   Progi 8192B (krytyczny) i 50000B (early-warning), throttling 60s/120s oraz
   cała logika auto-restart — bez zmian. To wartości dostrajane od v41/v64-P2c
   przez wiele iteracji na żywym sprzęcie; zmiana ich teraz bez dowodu że są
   błędne byłaby nieuzasadnionym ryzykiem, nie naprawą.

 Nie koliduje z: v41 (heapCrit/heapFrag/auto-restart), v64-P2c (próg 50kB),
 v70-DIAG/v122-TGSTACK-DIAG (ten sam wzorzec zastosowany do stosów),
 v62-1A (pochodzenie g_heapMinValue/g_heapMinTimestamp).

***********************************************v123 (2026-06-16) - PSRAM-HEAP: #define ENABLE_PSRAM (FirebaseClient) + 9 buforów raportów Telegram (~4 KB) ze stosu tgTask → PSRAM przez psramAllocSafe() — adresuje punkt 2 z planu ("heap")

 PROBLEM:
   Stałe zużycie wewnętrznego DRAM/stosu tgTask przez (a) bufory własne
   biblioteki mobizt/FirebaseClient (JSON RTDB, AsyncResult) i (b) ~4 KB
   lokalnych snprintf-buforów w 8 funkcjach budujących raporty Telegram —
   wywoływanych wielokrotnie (przy każdym kliknięciu menu), każdorazowo
   zajmujących stos tgTask bez potrzeby, mimo dostępnych 8 MB PSRAM.

 RESEARCH (web, 2026-06-16):
   1. README mobizt/FirebaseClient, sekcja "Library Build Options":
      ENABLE_PSRAM jest udokumentowaną, prawdziwą opcją kompilacji tej
      biblioteki ("For enabling PSRAM support"), definiowaną przed
      #include FirebaseClient.h — dokładnie ten sam mechanizm jak już
      używane ENABLE_DATABASE/ENABLE_LEGACY_TOKEN.
   2. WAŻNE ROZRÓŻNIENIE ESP8266 vs ESP32: README mówi że dla ESP8266
      "this macro was defined by default" — ale dla sekcji "Memory Options
      for ESP32" ten zapis NIE występuje. Na ESP32/ESP32-S3 trzeba go
      dodać jawnie — co tu zrobiono. Wymaga PSRAM włączonego w IDE/build
      (-DBOARD_HAS_PSRAM) — już potwierdzone aktywne w tym projekcie
      (#ifdef BOARD_HAS_PSRAM, compactHistPending ~linia 8630).
   3. SKOREKTA WŁASNEGO PLANU — EXT_RAM_BSS_ATTR: pierwotny pomysł
      "static + EXT_RAM_ATTR dla dużych buforów" zweryfikowany i poprawiony:
      (a) aktualna nazwa makra w ESP-IDF to EXT_RAM_BSS_ATTR, nie EXT_RAM_ATTR
          (ta druga nazwa jest z dokumentacji ESP-IDF ≤v4.1 — od ESP32-S3-erze
          dokumentacji konsekwentnie używana jest już nazwa z "_BSS_");
      (b) działa WYŁĄCZNIE na deklaracjach bez inicjalizatora niezerowego —
          potwierdzony realny przypadek na forum ESP32: użycie na
          zainicjalizowanej zmiennej kończy się paniką/crashem;
      (c) wymaga osobnej, prawdopodobnie NIEustawionej flagi sdkconfig
          CONFIG_SPIRAM_ALLOW_BSS_SEG_EXTERNAL_MEMORY (domyślnie OFF
          w sdkconfig arduino-esp32 — potwierdzone w przykładowym dumpie
          tego pliku), a przy awarii PSRAM nie ma ścieżki fallback (adresy
          przypisane na etapie linkowania).
      WNIOSEK: zamiast wprowadzać nowy, słabiej przetestowany mechanizm
      wymagający kolejnej ręcznej zmiany sdkconfig (ten sam tryb edycji,
      który przy v89/CONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC mógł nie przetrwać
      późniejszej migracji na PlatformIO w v90-91 — wart osobnej weryfikacji
      przy okazji), użyto JUŻ ISTNIEJĄCEGO, przetestowanego od v53
      psramAllocSafe() (ps_malloc + fallback DRAM, zero nowych zależności
      od sdkconfig). Telegram jest explicite wymieniony w komentarzu
      psramAllocSafe() (~linia 9116) jako dozwolone użycie.

 FIX — 9 buforów w 8 funkcjach (wszystkie wywoływane WYŁĄCZNIE z tgTaskFn,
 Core 0, pollTelegramCommands() — potwierdzone grepem + komentarzem w kodzie
 "wywołanie bezpieczne - jesteśmy na Core 0" ~linia 8929 — brak ryzyka
 współbieżności z loop()/Core 1):
   buildTelegramTempReport()      tb[280]  → psramAllocSafe(280)
   buildTelegramEnergyReport()    tb[420]  → psramAllocSafe(420)
   buildTelegramLightReport()     tb[380]  → psramAllocSafe(380)
   buildTelegramAdaptReport()     tb[380]  → psramAllocSafe(380)
   buildTelegramSensorHistory()   tb[400]  → psramAllocSafe(400)
   buildTelegramScheduleReport()  tb[480]  → psramAllocSafe(480)
   sendTelegramInlineMenu()       headerBuf[180]+_pl[1200] → psramAllocSafe×2
   buildTelegramStatusReport()    tb[350]  → psramAllocSafe(350)
   SUMA: ~4070 B zdjęte ze stosu/DRAM tgTask, trwale (alokacja raz przy
   1. wywołaniu, wskaźnik static cache'owany na całe uptime).
   Wzorzec: `static char* tb=nullptr; if(!tb) tb=(char*)psramAllocSafe(N);
   if(!tb) return "[ERR]..."; snprintf(tb, N, ...);` — sizeof(tb) ZAMIENIONE
   na literał N (sizeof wskaźnika to 4B, nie rozmiar bufora — częsty błąd
   przy tej konwersji, zweryfikowany grepem że nie wystąpił).

 CO ŚWIADOMIE NIE ZMIENIONO (i dlaczego — 5 innych buforów >256B):
   - logPrintf() buffer[384] (~linia 7954): NAJCZĘŚCIEJ wywoływana funkcja
     w całym programie, z OBU rdzeni (loop()+tgTaskFn) współbieżnie.
     static tu = realna korupcja logów przy równoczesnym wywołaniu z 2 core.
   - /api/history GET handler _buf[512] (~linia 11466): lambda
     AsyncWebServer — biblioteka nie serializuje równoległych requestów
     do tego samego route; static = ryzyko wymieszania odpowiedzi 2 klientów.
   - compactHistPending _stackBuf[512] (~linia 8633): to jest CELOWY
     fallback "gdy ps_malloc() PSRAM zawiedzie" — przenosząc go do PSRAM
     zniszczylibyśmy sens posiadania fallbacku.
   - saveEnergyStats() buf[320]+dayBuf[120]+wkBuf[80] (~linia 12274):
     wywoływana z OBU: logDailySummary()→loop() (linia 13842) ORAZ
     tgTaskFn/compactHistPending (linia 8601) — realne ryzyko cross-core.
   - loadEnergyStats() s_buf[768], saveHistoryPoint() pierwszy-zapis
     buf[512]: wywołania jednorazowe (boot / pierwszy zapis po restarcie)
     — brak powtarzającej się presji na pamięć, znikoma korzyść.

 Nie koliduje z: v122-TGSTACK-DIAG, v105-FC (ENABLE_DATABASE/LEGACY_TOKEN),
 v89-FIX-PSRAM-TLS-A (inna warstwa — mbedTLS/TLS, nie bufory FirebaseClient/TG),
 v53-PSRAM-DIAG (pochodzenie psramAllocSafe()).

***********************************************v122 (2026-06-16) - TGSTACK-DIAG: automatyczny alarm tgStack_free<3000B + log nowego minimum (HWM) — tgTask miał monitoring w 💓[HEAP], ale (w przeciwieństwie do loopStack od v70-DIAG) bez progu alarmowego i bez śledzenia degradacji w czasie

 PROBLEM:
   tgStack_free był liczony i logowany w 💓[HEAP] (heartbeat 60s, diagHeap() ~16118)
   od dawna, ale tylko jako liczba w jednej linii logu. loopStack ma od v70-DIAG
   (1) log [INFO] przy każdym nowym historycznym minimum i (2) alarm 🚨 < 2000B.
   tgStack — mimo identycznego ryzyka (Core 0: WiFi/Firebase/Telegram/TLS, stos
   12 288B w DRAM, setup() ~12166) — nie miał żadnego z tych dwóch mechanizmów.
   Komentarz z v62-3 ("Obserwować tgStack_free w heartbeat...") zakładał RĘCZNE
   skanowanie logów — w praktyce degradacja (6408B na boot -> 6176B historyczne
   minimum, udokumentowana analizą log_combined z 11-15.06.2026) przechodziła
   niezauważona aż do analizy na żądanie, zamiast być widoczna od razu w logu.

 RESEARCH (web, 2026-06-16) — dwa fakty potwierdzone, jeden SKORYGOWANY w trakcie:
   1. POTWIERDZONE: uxTaskGetStackHighWaterMark() na porcie ESP-IDF/ESP32 zwraca
      już BAJTY (StackType_t=uint8_t), nie słowa jak w "vanilla" FreeRTOS —
      mnożenie "* sizeof(StackType_t)" już obecne w tym pliku (linie ~16086-16089)
      jest poprawne i przenośne (na tym porcie ×1), NIE jest błędem ani zbędnym
      no-opem do usunięcia. [docs.espressif.com "Minimizing RAM Usage"/freertos_idf.html;
      esp32.com/viewtopic.php?t=20043]
   2. SKORYGOWANE po głębszym sprawdzeniu: pierwsza hipoteza ("arduino-esp32
      domyślnie CHECK_STACKOVERFLOW_NONE, zero detekcji") oparta na starym wątku
      z 2017 [github.com/espressif/arduino-esp32#315] okazała się NIEAKTUALNA.
      Rzeczywisty domyślny sdkconfig frameworka (zweryfikowany w samym repo
      espressif/arduino-esp32, branch 2.0.14, tools/sdk/esp32/sdkconfig) ma
      CONFIG_FREERTOS_CHECK_STACKOVERFLOW_CANARY=y. To NIE czyni tego monitoringu
      zbędnym: CANARY wykrywa przepełnienie REAKTYWNIE — dokładnie w momencie
      gdy już nastąpiło (czysty panic "stack overflow in task X detected" zamiast
      cichej korupcji struktur FreeRTOS), ale nie daje żadnego ostrzeżenia
      WCZEŚNIEJ. Proaktywny polling HWM (ten patch) wykrywa TREND zanim dojdzie
      do przepełnienia — to dwie różne, uzupełniające się warstwy, nie duplikat.
   3. Domyślny stos loopTask w arduino-esp32 (bez SET_LOOP_TASK_STACK_SIZE — ten
      projekt go nie używa, sprawdzone grepem) to 8192B [github.com/espressif/
      arduino-esp32 .../ArduinoStackSize.ino]. Próg tgStack=3000B wybrany
      proporcjonalnie do progu loopStack=2000B: 2000/8192 = 3000/12288 = 24.4%
      — identyczny ułamek nominału, nie przypadkowa wartość (12288=1.5×8192,
      więc 2000×1.5=3000 dokładnie).

 FIX (diagHeap(), heartbeat ~16085-16110, blok analogiczny do v70-DIAG):
   if (tgTaskHandle) {
     static UBaseType_t _prevTgStack = 0xFFFF;
     ... log [INFO] przy nowym minimum (identycznie jak loopStack) ...
     if (tgStack < 3000) logPrintf("🚨 [STACK] tgStack_free=...");
   }
   Guard if(tgTaskHandle) zapobiega false-alarmem gdy task się nie utworzył
   (tgStack=0xFFFF sentinel z istniejącego kodu kilka linii wyżej w tej funkcji).

 CO ŚWIADOMIE NIEZMIENIONE:
   Rozmiar TG_STACK=12288B BEZ ZMIAN. Plan z v62-3 ("zmniejszyć do 10240 w v63")
   nie jest realizowany: zużycie wzrosło z ~4888B (pomiar v62-3) do ~6112B
   (12288-6176, historyczne minimum z logów 11-15.06.2026) — margines skurczył
   się z ~58% do ~50%. Zmniejszanie stosu teraz byłoby działaniem WBREW własnym
   danym z logów. Stos zostaje w DRAM (heap_caps_malloc MALLOC_CAP_INTERNAL,
   komentarz ~12157) — PSRAM odpada z przyczyn tam opisanych (cache SPI wyłączany
   przy operacjach flash -> stos niedostępny -> crash co ~16s).

 Nie koliduje z: v121-FIX-ISNIGHT-RESET, v70-DIAG (loopStack), v62-3 (TG_STACK=12288).

### v121 (2026-06-15) - FIX-ISNIGHT-RESET: isNightGlobal trwale "true" po pierwszej nocy -> LED=0 w przerwie południowej każdego kolejnego dnia

 ROOT CAUSE:
   isNightGlobal jest ustawiane na true w bloku 1️⃣ STAN NOCNY funkcji trybAuto()
   (gdy isNight==true), ale JEDYNY reset na false (isNightGlobal = false;) znajdował
   się w onTrybChange() - funkcji wywoływanej wyłącznie przy zmianie trybu
   MANUAL<->AUTO. W normalnej pracy w trybie AUTO (bez przełączeń trybu) ta funkcja
   nigdy nie jest wywoływana po boot'cie -> po pierwszej nocy isNightGlobal zostawało
   TRWALE true przez WSZYSTKIE kolejne dni.

 EFEKT W LOGACH (potwierdzone analizą "dzień 1" vs "dzień 2"):
   Dzień 1 (boot w trakcie przerwy poł.): "🔄 Rampa południe: cel MIN LUX = 76 PWM"
     -> "RAMPA OPADANIE ZAKOŃCZONA | PWM końcowy: 76" -> LED=76 przez całą przerwę. OK.
   Dzień 2 (po pełnej dobie, bez restartu): log "Rampa południe: cel MIN LUX" w ogóle
     się nie pojawia -> "RAMPA OPADANIE ZAKOŃCZONA | PWM końcowy: 0" -> LED=0 przez
     całą przerwę południową (12:00-18:49).

 MECHANIZM:
   1. W nocy dnia 1: isNightGlobal=true (trybAuto), a applyMinLuxMode() (real-time
      _isNocna=true) wykrywa minLuxModeActive=true -> czyści: minLuxModeActive=false,
      minLuxCurrentPWM[0..4]=0.
   2. Rano dnia 2: isNight (lokalna zmienna trybAuto) poprawnie wraca na false,
      ALE isNightGlobal NIE jest resetowane -> zostaje true (bug).
   3. Rampa poranna (rampUp=true) nie używa calculateMinLuxPWM() -> działa normalnie,
      LED dochodzi do PWM dziennego. isNightGlobal wciąż true.
   4. Rampa południowa (rampUp=false, rampStart==MIDDAY_OFF_LOCAL): target =
      calculateMinLuxPWM() -> "if (isNightGlobal) return 0;" (1. linia funkcji,
      PRZED sprawdzeniem czujnika/fallbacku) -> target=0 dla każdej iteracji ->
      rampa zjeżdża do 0 zamiast do MIN LUX.
   5. Blok aktywacji MIN LUX po rampie (rampStart==MIDDAY_OFF_LOCAL && !minLuxModeActive):
      mlPwm = calculateMinLuxPWM() = 0 (wciąż isNightGlobal=true) i fallback FIX-v51
      (mlPwm==0 && minLuxCurrentPWM[0]>0) też nie ratuje, bo minLuxCurrentPWM[0]
      zostało wyzerowane w nocy (krok 1) -> minLuxModeActive zostaje false, LED=0.

 NAPRAWA - 1 linia w trybAuto() (sekcja 1️⃣/2️⃣, ~14000):
   Bezpośrednio po blokach "if (isNight) {...; isNightGlobal = true; ...; return; }"
   dodano:
       isNightGlobal = false;
   trybAuto() jest wywoływane ~co 100ms przez cały dzień, więc isNightGlobal jest
   teraz na bieżąco zsynchronizowane z rzeczywistym stanem dzień/noc - dokładnie tak,
   jak zakłada istniejący komentarz w applyMinLuxMode() ("isNightGlobal aktualizowany
   co 100ms przez trybAuto"). onTrybChange() pozostaje bez zmian (nadal nadpisuje
   isNightGlobal=false natychmiast przy zmianie trybu - nieszkodliwa redundancja).

 EFEKTY DODATKOWE (poprawione jako pochodne tego samego źródła):
   Wszystkie miejsca z guardem "!isNightGlobal" lub "isNightGlobal && ..." używane
   do logiki/diagnostyki w ciągu dnia (np. linie ~13845 TRYBAUT rate-limit-warn,
   ~16161 diagTempSensorCollision histereza, ~7037 log TSL) były od pierwszej nocy
   permanentnie w stanie "noc" - teraz działają zgodnie z przeznaczeniem.

 Nie koliduje z: FIX-v29-5, FIX-v51-MINLUX-FALLBACK, FIX-v13, FIX-v19-ALLFINISHED-SPAM.

### v120 (2026-06-14) - LOG-FILTER-FIX: Precyzyjne filtry logów (regex per kategoria), pełny log viewer w zakładce Logi

### v119 (2026-06-14) - TG-LOG-FULL: MAX_TG_DOC 2MB → 8MB — Telegram dostaje pełne logi z FS
     ROOT CAUSE: MAX_TG_DOC=2MB był sztucznym limitem odziedziczonym po erze ps_malloc(512KB).
     Po v118 (streaming 4KB blokami, WDT reset co 4KB) nie ma żadnego technicznego powodu
     ograniczać Telegrama do mniej niż FS_LIMIT (~7.8MB). Telegram Bot API akceptuje do 50MB.
     FIX: MAX_TG_DOC 2MB → 8UL*1024UL*1024UL (8MB).
     Skutek: Telegram wysyła WSZYSTKIE logi z FS (identycznie jak /api/log-download),
     zamiast ucinać do ostatnich 2MB. Przy FS_LIMIT=79% (~7.8MB logów) plik .txt
     na Telegramie ma ten sam rozmiar co pobrany z panelu WWW.
     Zmiana: 1 linia w sendTelegramDocument() + string wersji.

### v118 (2026-06-14) - STREAM-LOG: Bezbuforowe strumieniowanie logów — bez dużego bloku PSRAM
     ROOT CAUSE: logReadCombinedTail() alokuje ps_malloc(cap+1), cap=512KB.
     Przy długiej pracy + fragmentacji PSRAM (TLS, Firebase, JSON) blok 512KB
     może nie być dostępny jako ciągły obszar → pobieranie/Telegram FAIL.
     MAX_TG_DOC=512KB był podyktowany tym limitem PSRAM, nie wymaganiami sieci.
     FIX — trzy zmiany:
       (1) sendTelegramDocument(): MAX_TG_DOC 512KB → 2MB.
           Zamiast ps_malloc(2MB) — streaming 4KB blokami wprost z log_a+log_b.
           PSRAM: ~4KB zamiast 512KB. WDT: esp_task_wdt_reset() co 4KB (bez zmian).
       (2) /api/log-download: bez limitu rozmiaru — pobiera WSZYSTKIE logi z FS.
           beginResponse() callback czyta 4KB z LittleFS seek() zamiast memcpy z bufora.
           0 B PSRAM na bufor. Content-Length = szA + szB (pełna para log_a+log_b).
       (3) logReadCombinedTail() zachowana BEZ ZMIAN — nadal używana przez
           /api/log-lines (podgląd 80KB) gdzie bufor PSRAM jest uzasadniony.
     Efekty:
       - Telegram: 4× więcej logów (2MB), brak zależności od ciągłego bloku PSRAM
       - Panel WWW download: pełne logi z FS (do ~7.8MB przy FS_LIMIT=79%)
       - FS_LIMIT: bez zmian (8232000 B) — brak potrzeby zmniejszania do 2MB
       - Nie zmienia logiki rotacji, fsSizeGuard, ani logReadCombinedTail
     Zmiana ~20 linii w dwóch miejscach:
       - sendTelegramDocument() linia ~4429 (MAX_TG_DOC + zastąpienie ps_malloc/loop)
       - /api/log-download linia ~10997 (zastąpienie logReadCombinedTail+memcpy)

### v116 (2026-06-13) - FIX-TG-PARTITION-WIPE: Telegram martwy po zmianie partycji/formacie LittleFS
     ROOT CAUSE: zmiana schematu partycji (platformio.ini board_build.partitions lub
     ręczne partition_table) powoduje formatowanie LittleFS → /telegram_config.json usunięty.
     loadTelegramConfig() sprawdza LittleFS.exists() → false → return natychmiast →
     tgBotToken="", tgChatId="", tgEnabled=false. Jedyna naprawa: ręczna re-konfiguracja
     przez panel WWW. Identyczny efekt przy: pierwszym wgraniu, format LittleFS, nowy ESP32.
     FIX: Dwie zmiany:
       (1) Nowe defines przy #define TG_CONFIG_FILE (jedna sekcja):
             #define DEFAULT_TG_BOT_TOKEN  "8709162940:AAFJJGoI79XHiBkZYtP_IEZjkXO5yEzhDpo"   ← wypełnij tokenem z @BotFather
             #define DEFAULT_TG_CHAT_ID    "429181594"   ← wypełnij chatId z @myidbot
             #define DEFAULT_TG_ENABLED    true
       (2) loadTelegramConfig() przepisany:
             - bool fileLoaded = false
             - plik istnieje + token.length()>=10 → fileLoaded=true (normalna ścieżka)
             - !fileLoaded + strlen(DEFAULT_TG_BOT_TOKEN)>=10 →
                 tgBotToken/tgChatId/tgEnabled = DEFAULT_* + saveTelegramConfig()
                 + log "[TG-CFG] 🔄 Brak pliku ... załadowano DEFAULT"
             - !fileLoaded + DEFAULT pusty →
                 log "[TG-CFG] ⚠️ Brak pliku + DEFAULT pusty — TG wyłączony"
     Zachowanie:
       - DEFAULT_TG_BOT_TOKEN="" (domyślnie): brak efektu — TG wyłączony jak wcześniej
         ale z czytelnym logiem wskazującym przyczynę i sposób naprawy
       - DEFAULT_TG_BOT_TOKEN="token...": po clean LittleFS TG działa natychmiast,
         plik jest tworzony → kolejny restart wczytuje plik normalnie (fileLoaded=true)
       - Panel WWW nadal może nadpisać token/chatId/enabled w dowolnym momencie
     Zmiana: 1 sekcja defines (3 linie) + przepisanie loadTelegramConfig() (~20 linii).

### v115 (2026-06-13) - FIX-TGENABLED-RESET: tgEnabled reset to false przez Firebase CFG bez klucza tgEnabled
     ROOT CAUSE: checkFirebaseConfig() — blok "telegram" — inicjalizuje:
       bool newEnabled = false;  // ← zawsze false gdy klucz nieobecny
     Gdy Firebase wysyła payload z tgBotToken lub tgChatId ale BEZ klucza "tgEnabled":
       enIdx < 0 → newEnabled pozostaje false, ale tgChanged = true (przez newToken/newChatId)
     W konsekwencji: if (tgChanged) { tgEnabled = newEnabled; saveTelegramConfig(); }
       → tgEnabled = false zapisane do LittleFS (/tg_config.json)
       → po restarcie: tgEnabled=false ładowane w setup() → Telegram całkowicie martwy
       → w logu: tgReady=0 przez całą sesję, tlsMutexTotalTG=0, zero TG-POLL/TG-POST
     Trigger potwierdzony: zapis nowego tgBotToken lub tgChatId przez panel webowy bez
     wyraźnego ustawienia tgEnabled → silently wyłącza Telegrama. Nie wymaga restartu
     (saveTelegramConfig() nadpisuje plik natychmiast). Bug obecny od v39 (pierwsze FB CFG).
     FIX: dodano flagę tgEnabledPresent (bool, domyślnie false).
       - newEnabled domyślnie = tgEnabled (ZACHOWAJ bieżący stan zamiast false)
       - tgEnabledPresent = true tylko gdy body.indexOf("tgEnabled") >= 0
       - tgEnabled = newEnabled TYLKO gdy tgEnabledPresent=true
       - Gdy tgEnabled nieobecny: tgChatId/tgBotToken aktualizowane, tgEnabled niezmienione
     Zmiana: 4 linie (bool newEnabled=tgEnabled; bool tgEnabledPresent=false;
             tgEnabledPresent=true; if(tgEnabledPresent) tgEnabled=newEnabled)
             + log: [FB-CFG] telegram: enabled=%d (present=%d) chatId=%s

### v114 (2026-06-12) - FIX-WDT-FBAPP-LOOP: fbApp.loop() pominięty gdy fbAppReady=false
     ROOT CAUSE: po TIMEOUT w checkFirebaseCommands/checkFirebaseConfig (8s), ustawiane jest
     fbAppReady=false + fbSSLClient.stop(). W tgTaskFn wywołanie fbApp.loop() bez sprawdzenia
     fbAppReady powoduje że biblioteka FirebaseClient widzi martwe gniazdo (stop() wywołany) i
     inicjuje reconnect TCP+TLS (~5-7s), który blokuje task na semaforze OS. Task zablokowany
     nie może sam nakarmić WDT → crash po 10s (tgTask na CPU0, IDLE0 widoczne w logu).
     Sekwencja crashu: checkFirebaseCommands() TIMEOUT → fbAppReady=false + stop() →
     fbInitialize() 51ms → kolejny TIMEOUT 8s → fbInitialize() 51ms →
     fbApp.loop() (martwe gniazdo) → TCP+TLS reconnect → block >10s → WDT.
     FIX: outer fbApp.loop() w tgTaskFn opakowany w if(fbAppReady):
       - gdy fbAppReady=true: esp_task_wdt_reset() + fbApp.loop() (jak wcześniej)
       - gdy fbAppReady=false: fbApp.loop() całkowicie pominięty — każda z 3 funkcji FB
         (sendStatus/checkCmd/checkCfg) wywołuje fbApp.loop() wewnątrz własnej polling-pętli
         z WDT resetem co 10ms, więc nic nie tracimy.
     Zmiana: 1 miejsce w tgTaskFn, 5 linii kodu (if+wdt_reset+fbApp.loop+fi+wdt_reset).
### v113 (2026-06-12) - FIX-WDT-CRASHES: 3 potwierdzone crashe wyeliminowane
     Crash C (3%):  WiFi.setSleep(false) w setup() po WiFi connect — wyłącza
       modem-sleep/ppTask/pm_tbtt_process, który zagładzał tgTask na CPU0.
       Zdekodowany backtrace: ppTask→pm_tbtt_process→ppCheckTxConnTrafficIdle.
       Ref: espressif/esp-idf#6744.
     Crash B (12%): DNS pre-warm WiFi.hostByName(FIREBASE_HOST) w fbInitialize()
       przed initializeApp() — wypełnia cache lwIP, eliminuje synchroniczny DNS
       bez TCPIP_CORE_LOCK przy wewnętrznym connect() FirebaseClient.
       Wzorzec sprawdzony dla Telegrama od v75.
     Crash A (84%): esp_task_wdt_reset() przed każdym fbDatabase.set/get/remove
       (4 miejsca) — świeże 10s okno WDT na wypadek TCP+TLS reconnect (~5–7s)
       inicjowanego przez FirebaseClient przy pierwszym wywołaniu po reinit.
     Wynik: >10 min stabilnej pracy bez WDT resetu (vs crash co ~21s w v112).
### v112 (2026-06-12) - FIX-FB-SSL-KEEPALIVE: TCP keepalive na fbSSLClient (proaktywna detekcja martwego połączenia)
     ROOT CAUSE: fbSSLClient (WiFiClientSecure) nie ustawia SO_KEEPALIVE. Przy WiFi L2 UP
     + internet L3 dead: lwIP domyślnie czeka 7200s (TCP_KEEPIDLE default) zanim uzna gniazdo
     za martwe. v111 stop() naprawia reaktywnie po 8s polling-timeout, ale wtedy jest już
     tylko 7s marginesu do WDT(15s); ponadto tcp_connect() przy reinit może wisieć kolejne N sekund.
     FIX: applyTcpKeepalive(fbSSLClient, idle=3, intvl=2, cnt=3) w body każdego polling-loop:
       idle=3s + intvl(2s)×cnt(3) = 9s całkowita detekcja → margines 6s do WDT(15s).
       Mechanizm: po 9s bez ACK lwIP wysyła RST → recv() zwraca ECONNRESET → mbedTLS
       dostaje realny błąd → fbApp.loop() wraca z błędem natychmiast zamiast blokować →
       polling-loop kończy się jako ERROR przed upływem 8s timeout → fbSSLClient.stop() (v111)
       + fbAppReady=false → reinit. KA i stop() działają razem: KA zapobiega blokadom
       WEWNĄTRZ polling-loop (przed 8s), stop() naprawia je PO timeout.
       Wywołanie idempotentne: setsockopt() nie zmienia parametrów połączenia, fd<=0 → no-op.
     FIX (4 miejsca — po 1 wywołaniu w body każdego polling-loop):
       1. sendStatusToFirebase()        while-body: + applyTcpKeepalive(fbSSLClient,3,2,3)
       2. checkFirebaseCommands() GET   while-body: + applyTcpKeepalive(fbSSLClient,3,2,3)
       3. checkFirebaseCommands() DEL   while-body: + applyTcpKeepalive(fbSSLClient,3,2,3)
       4. checkFirebaseConfig()         while-body: + applyTcpKeepalive(fbSSLClient,3,2,3)
     Zmiana: 4 linie kodu (po 1 w każdym while-body, po fbApp.loop()).

### v111 (2026-06-12) - FIX-ROOT-FB-SSL-TIMEOUT: właściwa naprawa blokady fbApp.loop() na SSL_read()
     ROOT CAUSE: fbSSLClient (WiFiClientSecure) nie ma limitu czasu na recv() w ustanowionym
     połączeniu. Przy WiFi L2 UP + internet L3 dead: fbApp.loop() → SSL_read() → recv()
     blokuje bezterminowo po powrocie z polling-loop checkFirebaseCommands/sendStatus/checkConfig.
     Znany bug arduino-esp32: setsockopt(SO_RCVTIMEO) nie zatrzymuje mbedTLS send_ssl_data().
     WiFiClientSecure NIE ma setSessionTimeout() — ta metoda należy do ESP_SSLClient (inna klasa).
     FIX (3 miejsca — reaktywne zamknięcie gniazda po każdym TIMEOUT/ERROR):
       1. sendStatusToFirebase()  TIMEOUT: + fbSSLClient.stop() po fbAppReady=false
       2. checkFirebaseCommands() TIMEOUT/ERROR: fbAppReady=false + fbSSLClient.stop() zawsze
       3. checkFirebaseConfig()   TIMEOUT/ERROR: fbAppReady=false + fbSSLClient.stop() zawsze
     Mechanizm: po stop() connected()=false → następne fbApp.loop() w tgTaskFn nie wywołuje
     SSL_read() na starym martwym gnieździe → brak blokady → esp_task_wdt_reset() działa →
     WDT-safe. Zmiana: 3×2 linie kodu w funkcjach Firebase.

### v110 (2026-06-12) - FIX-WDT-WIFI-GUARD-ELSE: esp_task_wdt_reset() brakuje w gałęzi else WIFI-GUARD
     ROOT CAUSE: v109 dodał WIFI-GUARD (else: vTaskDelay 1s gdy WiFi L2 down), ale nie dodał
     esp_task_wdt_reset() w gałęzi else. Przy WDT=15s: ~12 iteracji × 1.2s bez resetu → crash.
     Trigger: utrata WiFi trwająca >15s (np. restart routera, chwilowe zaniki WiFi AP).
     FIX: jedna linia esp_task_wdt_reset() przed vTaskDelay(1000) w bloku else WIFI-GUARD.
     Zmiana: 1 linia kodu w tgTaskFn [~linia 8398].

### v109 (2026-06-10) - FIX-WIFI-GUARD: pomiń operacje sieciowe gdy WiFi L2 down
     ROOT CAUSE: przy całkowitej utracie WiFi (WiFi.status() != WL_CONNECTED) każda
     funkcja FB ma własny early-return, ale fbApp.loop() wewnętrznie inicjuje reconnect
     → TCP connect() wisi do 5s timeout. Przy 3 operacjach FB: 3×5s = 15s bez efektu.
     FIX: const bool _wifiUpNow przed blokiem FB/TG. Jeśli WiFi down:
       - cały blok FB (fbSendPending/fbCmdPending/fbCfgPending + fbApp.loop()) pominięty
       - vTaskDelay(1000ms) zamiast ~5-7s TCP stall
       - flagi fb*Pending ZACHOWANE → retry automatycznie gdy WiFi wróci
       - bloki FS (fsGuardPending, histSavePending itp.) niezmienione — działają normalnie
     Bloki TG (bootNotify, ds3BootFail, tgPoll) mają własne WiFi.status() == WL_CONNECTED
     i nie wymagają dodatkowej ochrony.
     Zmiana: 1 const bool + restrukturyzacja bloku FB (~30 linii) w tgTaskFn [~linia 8326].

### v108 (2026-06-10) - FIX-WDT-NETWORK-STORM: WDT crash przy zdechłej sieci
     ROOT CAUSE: tgClient.connect(ip, 443, 8000) [TCP timeout=8s] + tgClient.setTimeout(4)
     [TLS handshake=4s] = 12s całkowita blokada bez WDT reset > WDT 10s → crash.
     Komentarz v103 zakładał WDT=15s (stary projekt), v107 skrócił WDT do 10s — nie
     zaktualizowano marginesów connect(). Objaw: WDT fires w tgEnsureConnected() lub
     sendTelegramDocument() gdy WiFi dead: TCP SYN bez odpowiedzi maxuje timeout.
     FIX: setTimeout 4→2s + connect timeout 8000→5000ms → TCP(5s)+TLS(2s)=7s < WDT 10s.
     Dodano esp_task_wdt_reset() PRZED connect() niezależnie od resetu po DNS.
     Dotyczy: tgEnsureConnected() [6 zmian] + sendTelegramDocument() [6 zmian].
     Margines bezpieczeństwa: 3s zapasu do WDT. Wcześniejsza detekcja błędu (5s→fail szybciej).

### v107 (2026-06-10) - FIX-USE-AFTER-FREE: StoreProhibited @ 0x00000001 w fbApp.loop()
     ROOT CAUSE: AsyncResult fbXxxResult lokalny na stosie tgTaskFn (Core 0).
     tgTaskFn wywołuje fbApp.loop() PO powrocie z sendStatusToFirebase/checkFirebaseCommands/
     checkFirebaseConfig. SlotManager::returnResult() dostarcza spóźnioną odpowiedź do
     obiektu którego destruktor już uruchomił ~String() → buffer=nullptr → zapis pod
     String::buffer[1] = 0x00000001 → StoreProhibited → panic Core 0.
     Trigger: timeout Firebase (>300ms) podczas którego fbApp.loop() przetwarza odpowiedź
     po wyjściu z polling-loop i powrocie ze scope funkcji.
     NAPRAWA: 4 globalne AsyncResult (g_fbSendResult, g_fbCmdResult, g_fbDelResult,
     g_fbCfgResult) zamiast lokalnych. Reset przez operator= przed każdą operacją.
     Zweryfikowano eksperymentalnie: test-crash (local) crashuje w ~38min,
     test-fix (global) działa 39+ minut bez crashu przy identycznych warunkach (300ms timeout).
### v106 (2026-06-10) - FIX-WDT-LOG-STREAM: WDT crash przy otwieraniu zakładki Logi
     ROOT CAUSE: ESPAsyncWebServer 3.x używa StreamString jako bufor dla request->send(char*).
     StreamString::read() wywołuje String::remove(0,1) dla każdego bajtu → memmove O(n) per bajt
     = O(n²) całkowity koszt. Dla 80KB logów: ~3 miliardy bajtów memmove → async_tcp blokuje
     CPU1 ponad 10 sekund → WDT loopTask. Potwierdzono: PR #11232 mathieucarbou (kwiecień 2025).
     Repozytorium mathieucarbou/ESPAsyncWebServer zarchiwizowane 2025-01-21.
     Aktywne repo: ESP32Async/ESPAsyncWebServer v3.10.3.
 NAPRAWA — 3 endpointy (beginResponse z callbackiem + shared_ptr):
     request->send(type, char*) / beginResponse(200, type, char*)
       → beginResponse(type, len, [sp](out, maxLen, idx)->size_t{ memcpy... })
     shared_ptr<_PB> z destruktorem free() gwarantuje zwolnienie bufora PSRAM
     nawet gdy klient rozłączy się przed końcem transferu.
     Content-Length znany → przeglądarka może pokazać postęp pobierania.
 DODAJE: #include <memory> + #include <algorithm>
 ZMIANY (5 miejsc):
   (1) #include <memory>        ~linia 2849: shared_ptr
   (2) #include <algorithm>     ~linia 2850: std::min
   (3) /api/log-lines lines=0   ~linia 10673: beginResponse+callback [v106-LOG-CHUNK]
   (4) /api/log-lines lines=N   ~linia 10699: beginResponse+callback [v106-LOG-CHUNK]
   (5) /api/log-download        ~linia 10718: beginResponse+callback [v106-LOG-CHUNK]
 PLATFORMIO — zmień lib_deps (nie zmieniono tu, zmień w platformio.ini):
     STARE: mathieucarbou/ESP Async WebServer @ ^3.6.0
     NOWE:  ESP32Async/ESPAsyncWebServer @ ^3.10.3
     Po zmianie: usuń .pio i wykonaj pełny rebuild.
***
### v105 (2026-06-09) - FIREBASE-CLIENT: migracja z ręcznego HTTPClient+WiFiClientSecure
     na mobizt/FirebaseClient (ESP_SSLClient wewnętrznie).
     ROOT CAUSE FIX: WiFiClientSecure::connected() bug (peek() zwracał true dla
     martwego socketu) — od v99 do v104 generował kolejne warstwy łatek.
     FirebaseClient z ESP_SSLClient prawidłowo wykrywa martwy socket.
     USUWA: bool fbConnect() (307 linii), fbClientReady, fbBatchActive,
            fbClientLastUse, fbLastSuccessMs, FB_IDLE_RECONNECT_MS,
            g_fbServerIP/g_fbIPTs/g_fbIPValid, #include <HTTPClient.h>
     DODAJE: void fbInitialize(), fbSSLClient, fbAsyncClient, fbApp,
             fbDatabase, fbLegacyToken, fbAppReady
     WYMAGA: lib_deps += mobizt/FirebaseClient @ ^2.x.x  w platformio.ini
***
***
***
***
***
### v104 (2026-06-09) - FIX-WDT-FB-DEAD-TCP: WDT crash gdy HTTPClient robi wewnętrzny reconnect na martwym TCP

 ROOT CAUSE — łańcuch zdarzeń prowadzący do WDT crash w tgTask:

   1. Firebase RTDB zamyka idle TCP po ~6s wysyłając FIN (mimo keepalive z v83).
      Po FIN: fbClient.connected()=false, socket martwy.

   2. v100 (FB-CONN-DIAG): recv(MSG_PEEK) wykrywa FIN i loguje "TCP dead",
      ale NIE modyfikuje fbClientReady ani needReconnect — jest pasywny by design.
      fbClientReady pozostaje true.

   3. Kolejna iteracja tgTask: FB flags ustawione przez Core 1.
      fbConnect() wywołany z fbBatchActive=false:
        — needReconnect=false (fbClientReady=true, brak idle-timeout)
        — DIAG: wykrywa FIN, loguje "TCP dead", zwraca true
      Caller: fbBatchActive = true.

   4. sendStatusToFirebase() → fbConnect():
        — fbBatchActive=true && fbClientReady=true → batch-reuse (v65)
        — loguje "batch-reuse", zwraca true NATYCHMIAST z martwym socketem

   5. esp_task_wdt_reset() (linia ~12301), potem http.sendRequest("PATCH"):
        — HTTPClient widzi !client.connected()
        — Z setReuse(true): wywołuje wewnętrznie client.connect(host, port)
        — To wywołanie 2-argumentowe BEZ TIMEOUT — identyczny bug jak w fbConnect()
          sprzed v103. Różnica: v103 naprawił NASZE connect(), ale nie HTTPClient wewnętrzny.
        — Przy chwilowej niestabilności sieci: blokada >15s → WDT crash.
      Log crash: "batch-reuse" to OSTATNI log przed WDT — sendRequest nigdy nie kończy.

   Potwierdzenie z logów (dwa identyczne crashe):
     [FB-CMD] HTTP=200 | t=5726ms
     [FB-BLOCK-DONE] total=9613ms
     [FB-CONN] DIAG: TCP dead (FIN od Firebase)
     [FB-CONN] batch-reuse (active=1 ready=1 idle=0ms)   ← OSTATNI LOG
     E (25410) task_wdt: tgTask (CPU 0)                   ← WDT po ~15s od batch-reuse

 NAPRAWA — 1 miejsce (fbConnect(), guard batch-reuse):

   Przed batch-reuse sprawdź fbClient.connected():
     — connected()=true  → socket żywy → batch-reuse jak dotychczas (zero zmiany zachowania)
     — connected()=false → socket martwy → ustaw fbClientReady=false i przejdź do
       normalnego reconnect (needReconnect = !fbClientReady = true).
       Reconnect w fbConnect() ma pełną ochronę WDT (esp_task_wdt_reset co 100ms,
       connect(ip, port, 8000) z 8s timeout z v103).

 DLACZEGO connected() tutaj nie powoduje problemu z v99:
   v99 usunął connected() z needReconnect bo Firebase co ~6s zamykał TCP → 336 reconnectów/h
   (każde wywołanie fbConnect() poza batchem triggerowało reconnect).
   Ten check jest TYLKO w guard batcha (fbBatchActive=true), który odpala się co ~5-15s
   wyłącznie gdy sieć Firebase jest aktywna. Reconnect tutaj jest uzasadniony — socket
   jest martwy i HTTPClient i tak by go reconnektował, tyle że bez WDT protection.

 KIEDY USUNĄĆ TEN FIX (komentarz [v104] przy guard):
   Jeśli Firebase przestanie zamykać TCP w oknie między iteracjami (mało prawdopodobne),
   lub gdy arduino-esp32 HTTPClient dostanie własny configurable TCP connect timeout.

 MIEJSCA ZMIANY:
   (1) fbConnect() ~linia 11887: guard batch-reuse — dodano connected() check
   (2) setup() ~linia 8696: wersja firmware v97 → v104

***********************************************v103 (2026-05-31) - FIX-TCP-CONNECT-TIMEOUT: bezpieczne connect() bez WDT crash

 PROBLEM: WiFiClientSecure::connect(ip, port) blokuje bez limitu czasu.
   Gdy internet niedostępny, TCP SYN backoff lwIP → brak powrotu przez >15s → WDT crash.
   Dotyczy: tgEnsureConnected(), sendTelegramDocument(), fbConnect() — 6 wywołań.
   Potwierdzone: GitHub espressif/arduino-esp32 issue #5398, PR #5487.
   setConnectionTimeout() nie istnieje w arduino-esp32 WiFiClientSecure.

 ROZWIĄZANIE — 3-argumentowy connect() z timeoutem w ms:
   connect(ip, port, 8000) używa non-blocking TCP connect + select() wewnętrznie.
   Przy braku odpowiedzi serwera wraca po 8s zamiast blokować nieskończenie.
   Budżet: TCP(8s) + TLS(4s) = 12s max < WDT 15s → zero crashy przy braku internetu.

 ZMIANY (6 linii):
   (1) tgEnsureConnected()     ~linia 3764: connect(g_tgServerIP, 443) → connect(…, 443, 8000)
   (2) tgEnsureConnected()     ~linia 3766: connect(TG_HOST, 443)      → connect(…, 443, 8000)
   (3) sendTelegramDocument()  ~linia 4174: connect(g_tgServerIP, 443) → connect(…, 443, 8000)
   (4) sendTelegramDocument()  ~linia 4176: connect(TG_HOST_DOC, 443)  → connect(…, 443, 8000)
   (5) fbConnect()             ~linia 11995: connect(g_fbServerIP, 443) → connect(…, 443, 8000)
   (6) fbConnect()             ~linia 11997: connect(FIREBASE_HOST, 443)→ connect(…, 443, 8000)

***********************************************v101 (2026-05-30) - FIX-WDT-CONFIRMED: 4 potwierdzone crashe naprawione po teście symulacyjnym

 KONTEKST: WDT_CrashSim_Test_v1 potwierdził wszystkie 4 hipotezy crashów.
   Scenariusz A → PANIC (assert FreeRTOS) bez WiFi + TCP connect()  [gorszy niż TASK_WDT]
   Scenariusz C → TASK_WDT po 11s (4s+3s+2s+2s) bez WDT feed między save*()
   Scenariusz D → TASK_WDT po 12s (3×4s TLS) bez cooldown/WDT feed
   esp_reset_reason() po A1 = TASK_WDT (6) [z poprzedniej sesji]
   Wszystkie wersje Z FIXEM → brak crashu (PASS).

 ZMIANY (5 miejsc):

 [FIX-1] WDT timeout 10s → 15s (setup(), linia ~8535):
   ROOT CAUSE A: fbClient.connect(IP, 443) — TCP SYN backoff lwIP może trwać >10s.
   WiFiClientSecure.setTimeout(4) dotyczy TLS READ, nie TCP connect().
   WiFi guard (już w kodzie) chroni gdy WiFi offline; 15s chroni gdy WiFi niestabilne
   i TCP SYN blokuje 6-14s. Margines: TCP(7s)+TLS(4s)=11s → 15s daje 4s zapasu.
   Potwierdzone: bez WiFi → PANIC (FreeRTOS assert), z WiFi niestabilnym → TASK_WDT.

 [FIX-2] esp_task_wdt_reset() przed Wire.end() i delay(30) w tslReinitPending (linia ~7824):
   ROOT CAUSE B: blok tslReinitPending nie resetował WDT na wejściu.
   Sekwencja: Wire.end()+delay(30)+bit-bang+Wire.begin()+delay(80) = ~140ms
   + czujnikPokojowy.begin() ~8s + czujnikNadWoda.begin() ~8s = ~16140ms > WDT.
   Jeśli poprzednia operacja tgTask zajęła 2s bez WDT reset → 2+16 = 18s → crash.
   Test: blokada 4.5s+4.5s = 9113ms przeżyta z fixem, crash bez.

 [FIX-3] esp_task_wdt_reset() przed delay(80) po Wire.begin() w tslReinitPending (linia ~7836):
   Uzupełnienie FIX-2: Wire.begin() może blokować krótko, delay(80) następuje po nim.
   Bez resetu: Wire.end()+delay(30)+Wire.begin()+delay(80) = ~140ms okno bez WDT feed.
   Małe okno samo w sobie, ale kumuluje się z poprzednimi operacjami tgTask.

 [FIX-4] esp_task_wdt_reset() między saveHistoryPoint/saveAdaptStats/saveEnergyStats (linia ~7992):
   ROOT CAUSE C: histSavePending brak WDT feed między 3 operacjami LittleFS.
   Suma: poprzednia operacja (4s) + saveHistoryPoint(3s) + saveAdaptStats(2s)
   + saveEnergyStats(2s) = 11s > WDT 10s → TASK_WDT.
   Potwierdzone testem C1 (crash po 11s). Wersja z fixem → PASS.

 [FIX-5] PRE_RESET_UPDATE(2) co 2s w głównej pętli tgTaskFn (linia ~7778):
   Diagnostyczny: po każdym crashu WDT, boot pokaże ostatni checkpoint tgTask
   (np. "FB-CONN-start", "TSL-REINIT", "histSave") → koniec debugowania w ciemno.
   src=2 = "WDT — dane z tgTask (autonomiczny zapis)".

 MIEJSCA ZMIANY:
   (1) setup()         ~linia 8535: .timeout_ms = 10000 → 15000
   (2) tgTaskFn        ~linia 7778: PRE_RESET_UPDATE(2) co 2s po esp_task_wdt_reset()
   (3) tgTaskFn        ~linia 7824: esp_task_wdt_reset() przed PRE_RESET_CP("TSL-REINIT")
   (4) tgTaskFn        ~linia 7826: esp_task_wdt_reset() między Wire.end() a delay(30)
   (5) tgTaskFn        ~linia 7836: esp_task_wdt_reset() przed delay(80)
   (6) tgTaskFn        ~linia 7992: esp_task_wdt_reset() ×4 w bloku histSavePending

***********************************************v100 (2026-05-29) - FB-CONN-DIAG: pasywna diagnostyka TCP bez reconnectów

 PROBLEM (po v99): usunięcie !connected() z needReconnect wyeliminowało 336 reconnectów/h,
   ale wraz z nimi straciliśmy widoczność cyklu życia gniazda TCP Firebase.
   Nie wiemy kiedy Firebase zamknęło połączenie (~6s idle), kiedy HTTPClient to naprawił,
   ani czy socket w ogóle żyje między żądaniami. Zero diagnostyki poza reconnectem.

 ROZWIĄZANIE — recv(MSG_PEEK|MSG_DONTWAIT) na surowym fd lwIP:
   MSG_PEEK:    bajty NIE są konsumowane — zero efektów ubocznych, dane nienaruszone.
   MSG_DONTWAIT: wraca natychmiast — zero blokowania, zero wpływu na WDT.
   fd lwIP:     pomija warstwę mbedTLS, odpytuje TCP bezpośrednio.
   wynik  >0:   dane w buforze TCP → socket żywy
   wynik  -1 + EAGAIN(11)/EWOULDBLOCK: idle, brak danych → socket żywy (normalny stan)
   wynik  0:    FIN od serwera → Firebase zamknęło połączenie po ~6s idle
   wynik  -1 + ECONNRESET/ENOTCONN: RST / siłowe zamknięcie

 IMPLEMENTACJA (1 blok, 2 statyczne zmienne):
   static bool _fbSockWasAlive, static uint32_t _fbSockDeadSince
   Blok wstrzyknięty między log "needReconn" a "if (needReconnect) {" w fbConnect().
   Aktywny TYLKO gdy fbClientReady && !needReconnect — nie koliduje z reconnect.
   Edge-triggered: log tylko przy zmianie stanu (żywy→dead, dead→żywy).
   Reset stanu przy faktycznym reconnect — poprawna krawędź po TLS handshake.

 BRAK NOWYCH #include: sys/socket.h (v82), EAGAIN/errno — już w pliku.
 ZERO NOWYCH RECONNECTÓW: blok nie modyfikuje needReconnect ani fbClientReady.

 OCZEKIWANY LOG (przykład, ~co 6s gdy Firebase zamknie idle TCP):
   [FB-CONN] DIAG: TCP dead (FIN od Firebase) — HTTPClient obsłuży @ next req | fd=5 fH=222kB
   [FB-CONN] DIAG: TCP alive po 423ms (HTTPClient reconnect OK) | fH=214kB

 MIEJSCA ZMIANY:
   (1) fbConnect() ~linia 11749: nowy blok [v100] między log-check a if(needReconnect)

***********************************************v99  (2026-05-29) - FIX-FB-RECONN: eliminacja 336 zbędnych reconnectów/h Firebase

 ROOT CAUSE — "[FB-CONN] RECONNECT" co ~7s (potwierdzony testem FB_Reconnect_Test_v1/v2):
   fbConnect() zawierał warunek needReconnect:
     !fbClientReady || (!fbClient.connected() && millis()-fbLastSuccessMs>2000ms) || idle>55s
   Firebase RTDB zamyka TCP po ~6s idle po każdej odpowiedzi (mimo Connection:"keep-alive").
   connected()=false wykrywane co 7s (interwał STATUS) → manual reconnect + pełny TLS handshake.
   Wynik: 336 reconnectów/h, każdy z ~400ms opóźnieniem.
   HTTPClient z setReuse(true) obsługuje martwe gniazdo transparentnie — nasz warunek
   wyzwalał zbędny reconnect ZANIM HTTPClient mógł zadziałać samodzielnie.
   Potwierdzone: test v1=336/h przy needReconnect z connected(), test v2=0/h po usunięciu.
   Dodatkowy bug: setsockopt(fd=0,...) → errno:9 EBADF — fd=0 to stdin, nie socket.
   Dotyczyło _fbSock>=0 w fbConnect() i fd<0 w applyTcpKeepalive() — oba przepuszczały fd=0.

 NAPRAWA (4 zmiany):
   1. fbConnect(): usunięto !connected() && >2000ms z needReconnect — HTTPClient radzi sobie sam.
   2. fbConnect(): log przyczyn RECONNECT — usunięto "%s !connected" (warunek nieaktualny).
   3. fbConnect(): _fbSock >= 0 → _fbSock > 0 (fd=0=stdin → setsockopt errno:9 EBADF).
   4. applyTcpKeepalive(): fd < 0 → fd <= 0 (ta sama naprawa fd=0).

 WYNIK: brak "[FB-CONN] RECONNECT" w logu — tylko jeden reconnect przy starcie.
        errno:9 z NetworkClient.cpp nadal widoczne — to błędy wewnątrz HTTPClient
        (framework ustawia opcje socketu przed jego otwarciem), nieusuwalne bez modyfikacji lib.

 MIEJSCA ZMIANY:
   (1) applyTcpKeepalive() ~linia 3525: fd < 0 → fd <= 0
   (2) fbConnect() ~linia 11707: needReconnect — usunięto środkowy warunek !connected()
   (3) fbConnect() ~linia 11720: logPrintf RECONNECT przyczyna — usunięto "%s !connected"
   (4) fbConnect() ~linia 11849: _fbSock >= 0 → _fbSock > 0

***********************************************v98  (2026-05-29) - FIX-LOG-CLEAR-V2: eliminacja "Has open FD" przez AsyncFileResponse

 ROOT CAUSE — "esp_littlefs: Failed to unlink path. Has open FD." (potwierdzony researched):
   v72 naprawiło kolizję z tgTask-flush (logMutex + logClearPending wzorzec).
   Jednak Serial nadal pokazywał błąd. Przyczyna:

   1. AsyncFileResponse trzyma FD przez cały transfer TCP (sekundy).
      Klasa AsyncFileResponse otwiera plik w konstruktorze i zamyka go dopiero
      w destruktorze — wywoływanym po potwierdzeniu ostatniego bajtu przez TCP stack.
      Dotyczy: request->send(LittleFS, LOG_FILE, ...) w /api/log-lines i /api/log-download.

   2. esp_littlefs VFS blokuje remove() gdy JAKIKOLWIEK FD jest otwarty.
      Źródło: esp_littlefs.c (joltwallet/esp_littlefs) — warstwa VFS ESP-IDF,
      NIE samo jądro littlefs. Littlefs core ma semantykę Unixa (unlink na otwartym pliku OK).
      Espressif celowo dodał sprawdzenie FD dla ochrony wear-leveling flash.
      Wynik: remove() zwraca EACCES (-1) z komunikatem "Has open FD" mimo że
      tgTask ma wyłączność logMutex.

   3. /api/status otwierał LOG_FILE co 2s (odpytywany przez panel WWW) żeby odczytać rozmiar.
      Każde LittleFS.open() = potencjalny otwarty FD w oknie remove().

   Diagnozy potwierdzające (web research):
   - github.com/joltwallet/esp_littlefs issues: "Has open FD" = problem w warstwie VFS, nie w LittleFS
   - github.com/ESP32Async/ESPAsyncWebServer: AsyncFileResponse destruktor = zamknięcie FD po TCP
   - esp32.com forum: usunięcie sdfile.close() = transfer działa, bo library zarządza FD lifecycle
   - me-no-dev/ESPAsyncWebServer: zarchiwizowany 2025-01; aktywny fork = ESP32Async/ESPAsyncWebServer

 NAPRAWA (10 zmian, 3 warstwy obrony):

   Warstwa 1 — blokuj HTTP readers podczas remove():
     logClearInProgress=true przed remove(), false po. HTTP handlers zwracają ""
     zamiast otwierać plik. Eliminuje race z /api/status i /api/log-lines.

   Warstwa 2 — eliminuj AsyncFileResponse dla LOG_FILE:
     /api/log-lines?lines=0 i /api/log-download: wczytaj całość do bufora PSRAM,
     zamknij FD, potem request->send(). FD zamknięty PRZED wysłaniem odpowiedzi.
     Limit: 80KB (log-lines) / 512KB (log-download) — mieści się w PSRAM 8MB.

   Warstwa 3 — cache rozmiaru logu:
     g_logFileSize aktualizowany po każdym logFlushCore0(). /api/status czyta cache
     zamiast otwierać plik co 2s. Eliminuje najczęstszy FD w złym momencie.

   Mechanizmy pomocnicze:
     vTaskDelay(150ms) po ustawieniu InProgress — czas na zamknięcie in-flight FD.
     Retry 5× co 50ms w tgTask — fallback gdyby FD nadal otwarty po 150ms.
     g_logFileSize=0 + g_lastFlush reset po remove() — blokuje natychmiastowe odtworzenie pliku.

   JS: clearLogsV99() — POST → "PENDING" → polling /api/log-clear-status co 300ms
     (max 40 prób = 12s) → toast DONE/ERROR. Zastępuje ślepe qa() które nie wiedziało
     czy operacja się udała.

 WYNIK: Serial pokazuje "[LOG-CLEAR] log.txt usuniety z panelu WWW", brak "Has open FD".
        Potwierdzone na urządzeniu (2026-05-29).

 MIEJSCA ZMIANY:
   (1)  globals ~linia 2646: 4 nowe volatile: logClearInProgress, logClearDone,
        logClearFailed, g_logFileSize
   (2)  logFlushCore0() ~linia 7080: g_logFileSize = f.size() przed f.close()
   (3)  tgTask ~linia 7806: blok logClearPending zastąpiony pełną impl. v98
        (InProgress, vTaskDelay 150ms, retry 5×, g_logFileSize=0, g_lastFlush reset)
   (4)  /api/log-clear ~linia 10247: reset Done/Failed + logClearPending + odpowiedź "PENDING"
   (5)  /api/log-clear-status ~linia 10263: nowy endpoint GET → DONE/ERROR/PENDING/IDLE
   (6)  /api/log-lines?lines=0 ~linia 10282: ps_malloc bufor + f.close() przed send()
        (eliminuje AsyncFileResponse dla ścieżki cały plik)
   (7)  /api/log-lines N-lines ~linia 10317: dodany guard logClearInProgress
   (8)  /api/log-download ~linia 10387: ps_malloc bufor + hf.close() przed send()
        (eliminuje request->send(LittleFS, path, true))
   (9)  /api/status ~linia 10681: logSz = cache g_logFileSize zamiast open/size/close co 2s
   (10) HTML ~linia 9469: onclick clearLogsV99(); JS ~linia 9587: funkcja clearLogsV99()

***********************************************v97  (2026-05-29) - OPT-LOG: redukcja logów 91% (10MB→0.9MB/24h)

 ROOT CAUSE:
   Kody diagnostyczne v81-DIAG dodane przy crashach v81-v82 nadal aktywne w v96.
   Logują każde zdarzenie niezależnie od jego istotności, generując ~10 MB/24h szumu.
   Przy KEEP_LOG=512KB (v96) log rotuje co ~1.2h zamiast co ~12h.
   Logi diagnostyczne służyły do wykrycia crashy — system v96 jest stabilny.
   Zasada: alarmy i błędy logują ZAWSZE; spokojny stan — rzadko lub wcale.

 NAPRAWA (8 zmian):
   1. [HEAP] heartbeat: 5s → 60s (alarm heapCrit w diagHeap() loguje zawsze — nie traci)
   2. [TG-ALIVE]: 5s → 60s (zawieszenie tgTask widoczne po 60s bez wpisu)
   3. [FB-CONN] check: log tylko gdy needReconn=1 (reconnect loguje zawsze osobno)
   4. [FB-BLOCK]: co każdą iterację → co 5 min; [FB-BLOCK-DONE]: tylko gdy total>500ms
   5. [FB-CONN] batch-reuse: za flagę Komentarze (nieistotny log wewnętrzny)
   6. [TG-KA] / [FB-KA]: log tylko gdy keepalive=FAIL (OK jest normą, nie informacją)
   7. [TLS-MX] FB: czekam na mutex: usunięty ("czekam" bez wait>50ms = szum)
   8. [TLS-MX] FB: mutex released: usunięty (zwolnienie mutexa nie niesie informacji)

 MIEJSCA ZMIANY:
   (1) diagHeap() ~linia 15076: 5000 → 60000
   (2) tgTaskFn ~linia 7616: 5000UL → 60000UL
   (3) fbConnect() ~linia 11508: if (needReconnect) przed logPrintf check
   (4) tgTaskFn ~linia 7885: static _fbBlockLogLast co 5min; linia 7909: >500ms
   (5) fbConnect() ~linia 11498: if (Komentarze) przed logPrintf batch-reuse
   (6) tgEnsureConnected() ~linia 3579: if (!_kaOk); fbConnect() ~linia 11658: if (!_kaOk)
   (7) fbConnect() ~linia 11549: usunięty logPrintf czekam
   (8) fbConnect() ~linia 11687: usunięty logPrintf mutex released

***********************************************v96  (2026-05-29) - OPT-TGDOC-CHUNK + FS-GUARD-TUNE

 ROOT CAUSE:
   1. sendTelegramDocument() wysyła plik w 64B chunkach → 8192 rekordów TLS dla 512KB.
      Każdy rekord TLS ma ok. 20-50B overhead: 8192 × ~35B = ~286 KB narzutu = 33% overhead.
      Dodatkowo 8192 wywołań esp_task_wdt_reset() i tgClient.write() generuje massive CPU overhead.
      Efekt: 33s na 512KB (cel: <5s).
      Źródło: ESP32 PR #11865 (espressif/arduino-esp32) — fix WiFiClientSecure partial write,
      rekomendacja chunk 4KB = jeden rekord TLS, ESP32 MTU 1460B, TLS record max 16KB.
   2. KEEP_LOG = 2MB — po przycinaniu log.txt ma 2MB, ale MAX_TG_DOC=512KB,
      więc 1.5MB jest przechowywane ale NIGDY nie wysyłane przez Telegram.
      Wasted space przyspiesza wypełnianie FS.
   3. FS_LIMIT = 85%: po wyzwoleniu fsSizeGuard ustawia flagę, tgTask wykonuje
      przycinanie. W tym czasie FS może wzrosnąć z 85% do 100% (szczególnie gdy
      tgTask zajęty CMD-RUN LOGS 33s). Log z 01:01:23 potwierdza: Zajete=100%.

 NAPRAWA:
   1. sendTelegramDocument(): 64B chunk → 4096B PSRAM bufor.
      Czyta 4KB z pliku jednym wywołaniem, zapisuje do TLS jednym wywołaniem.
      Wewnętrzna pętla retry obsługuje partial write (mbedTLS zwraca <r bajtów).
      esp_task_wdt_reset() po każdym 4KB chunk (nie co 64B).
      Oczekiwany efekt: 33s → 3-5s dla 512KB (x6-10 szybciej).
   2. KEEP_LOG: 2097152 → 524288 (2MB → 512KB, spójne z MAX_TG_DOC).
      Po przycinaniu log.txt = 512KB = max rozmiar który Telegram dostaje.
      Zero wasted storage. Czas regrowth do 75% progu x2 dłuższy.
   3. FS_LIMIT: 8857190 → 7983820 (85% → 76.6%) w fsSizeGuard() i tgTask.
      Wyzwolenie wcześniejsze daje ~86MB buforu zamiast ~30MB przed zapełnieniem.
      Utrzymano log-copy buffer 512B → 4096B PSRAM (x8 szybsze kopiowanie).

 MIEJSCA ZMIANY:
   (1) sendTelegramDocument() ~linia 3961: uint8_t buf[512] + 64B-chunk-loop
       → PSRAM bufor 4096B + single write() + partial-write retry
   (2) tgTaskFn fsGuardPending ~linia 7865: FS_LIMIT 85% → 76.6%, KEEP_LOG 2MB → 512KB
   (3) tgTaskFn fsGuardPending ~linia 7879: copy buf 512B → 4096B PSRAM
   (4) fsSizeGuard() ~linia 15566: FS_LIMIT 85% → 76.6%
   (5) setup() ~linia 8299: Wersja firmware v94 → v96
   (6) setup() PSRAM-TLS tag ~linia 10836: [PSRAM-TLS v89] → [PSRAM-TLS v96]

***********************************************v94  (2026-05-28) - FIX-DS18B20-GPIO + FIX-WDT-TGPOST

 ROOT CAUSE:
   1. DS18B20 wody ZAWSZE FAIL: GPIO39 = JTAG TCK na ESP32-S3.
      Biblioteka OneWire_direct_gpio.h traktuje piny >33 jako input-only
      (check pin <= 33 z czasów klasycznego ESP32) -> directModeOutput(39) = no-op
      -> brak reset pulse -> czujnik nigdy nie odpowiada, niezaleznie od okablowania.
   2. WDT crash w tgTask: tgClient.printf()/print(body) moze blokowac bezterminowo
      gdy socket jest martwy (TCP black hole). Brak esp_task_wdt_reset() przed
      tymi wywolaniami -> WDT po 10s -> restart -> petla crashow (log: iter=461).

 NAPRAWA:
   1. ONE_WIRE_BUS3: GPIO39 -> GPIO5 (wolny pin, pelny I/O, brak konfliktu z JTAG).
      WYMAGANA ZMIANA FIZYCZNA: przylutuj kabel DATA czujnika DS18B20 wody na GPIO5.
   2. tgPost(): esp_task_wdt_reset() przed tgClient.printf() i tgClient.print(body).
   3. diagDS18B20 retry loop: esp_task_wdt_reset() na poczatku kazdej iteracji
      i przed delay(500) -- 3 proby x ~1.25s = do 3.75s bez karmienia WDT.

 WERYFIKACJA:
   DS18B20 OK: "OK [DS-DIAG] DS18B20 wody OK." w logu przy starcie.
   WDT OK: brak WDT_TASK restartow podczas TG-POLL.

***********************************************v93  (2026-05-27) - FIX-WDT-LIBRARY: mathieucarbou/AsyncTCP jawna zależność — begin() obsługuje locking wewnętrznie

 ROOT CAUSE PRAWDZIWY (po badaniach):
   Cały problem z WDT (v90→v92) wynikał z użycia starego me-no-dev/AsyncTCP jako
   tranzytywnej zależności. Ta biblioteka nie obsługuje TCPIP core lock wewnętrznie
   i jest niekompatybilna z arduino-esp32 3.x / ESP-IDF 5.x.
   Bez jawnej deklaracji mathieucarbou/AsyncTCP w platformio.ini PlatformIO mógł
   pobierać stary fork me-no-dev/AsyncTCP jako zależność pośrednią — stąd konieczność
   ręcznego LOCK_TCPIP_CORE / tcpip_callback_with_block w kodzie.

 NAPRAWA — mathieucarbou/AsyncTCP jawna zależność + uproszczenie kodu:
   platformio.ini:
     lib_deps += mathieucarbou/AsyncTCP @ ^3.3.2
     build_flags += -D CONFIG_ASYNC_TCP_MAX_ACK_TIME=5000
                    -D CONFIG_ASYNC_TCP_RUNNING_CORE=1
                    -D CONFIG_ASYNC_TCP_STACK_SIZE=4096
   mathieucarbou/AsyncTCP >=3.3.2 obsługuje TCPIP locking wewnętrznie →
   żaden wrapper (LOCK_TCPIP_CORE / tcpip_callback_with_block) nie jest potrzebny.

 ZMIANY W KODZIE:
   1. Usunięto funkcję webserialBeginCb() [v92]
   2. We wszystkich 3 miejscach: tcpip_callback_with_block() → webserialServer.begin()
   3. Usunięto #include <lwip/tcpip.h> (nie jest już potrzebny)
   4. Zachowane esp_task_wdt_reset() przed/po begin() (defensywnie)
   5. Wersja: v92 → v93

 WERYFIKACJA W LOGU:
   Sukces: Boot bez WDT, "🌐 Terminal: http://..." w logu ✅

***********************************************v92  (2026-05-27) - FIX-WDT-TCPIP-CALLBACK: tcpip_callback_with_block() zamiast LOCK_TCPIP_CORE()

 ROOT CAUSE (WDT crash nadal w v91 — potwierdzony logami):
   esp_task_wdt_reset() przed LOCK_TCPIP_CORE() NIE pomaga, bo sam lock może blokować >10s.
   LOCK_TCPIP_CORE() to mutex trzymany przez tcpip_thread. Zaraz po WiFi.begin() tcpip_thread
   przetwarza strumień zdarzeń (DHCP, ARP, socket init) bez przerwy przez >10–15s.
   W tym czasie mutex jest niedostępny dla loopTask → loopTask blokuje wewnątrz LOCK_TCPIP_CORE()
   bez żadnego wdt_reset → WDT odpala po 10s od ostatniego resetu.
   Wzorzec crashy v90 i v91: WDT zawsze ~22.9s od bootu, zaraz po logach PSRAM-TLS.

 WŁAŚCIWA NAPRAWA — tcpip_callback_with_block():
   LOCK_TCPIP_CORE()          — loopTask CZEKA NA mutex tcpip_thread → może trwać >10s ❌
   tcpip_callback_with_block() — loopTask WYSYŁA wiadomość do kolejki tcpip_thread i czeka
                                 na potwierdzenie wykonania → nie potrzebuje mutexa.
                                 tcpip_thread wykona callback w swoim cyklu przetwarzania
                                 kolejki — zwykle <100ms, nawet gdy jest zajęty zdarzeniami ✅
   webserialServer.begin() wywołany z wnętrza tcpip_thread (przez callback) → LWIP_ASSERT
   spełniony bez blokowania loopTask → zero ryzyka WDT.

 IMPLEMENTACJA:
   1. Nowa funkcja statyczna webserialBeginCb() przed setup() — wywoływana przez tcpip_thread
   2. Wzorzec w 3 miejscach (setup + 2× loop reconnect):
        esp_task_wdt_reset();
        tcpip_callback_with_block(webserialBeginCb, nullptr, 1);
        esp_task_wdt_reset();
   3. LOCK_TCPIP_CORE() i UNLOCK_TCPIP_CORE() usunięte
   4. #include <lwip/tcpip.h> pozostaje (potrzebny dla tcpip_callback_with_block)

 MIEJSCA ZMIANY:
   (1) webserialBeginCb() — nowa funkcja statyczna przed setup()
   (2) setup() ~linia 10602: LOCK/UNLOCK zastąpione tcpip_callback_with_block()
   (3) loop() reconnect !wsReady: LOCK/UNLOCK zastąpione tcpip_callback_with_block()
   (4) loop() reconnect wsReady: LOCK/UNLOCK zastąpione tcpip_callback_with_block()
   (5) Wersja firmware: v91 → v92

 WERYFIKACJA W LOGU:
   Sukces:    Boot bez WDT, "🌐 Terminal: http://..." w logu — koniec crash loop ✅
   Porażka:   WDT nadal → problem jest gdzie indziej (nie w webserialServer.begin())

***********************************************v91  (2026-05-27) - FIX-WDT-TCPLOCK: esp_task_wdt_reset() przed/po LOCK_TCPIP_CORE ❌ BEZ EFEKTU

 STATUS: ❌ WDT crash nadal — LOCK_TCPIP_CORE() sam w sobie blokuje >10s po WiFi connect.
   esp_task_wdt_reset() przed lockiem daje świeże okno, ale lock zajmuje >10s → WDT.
   Właściwa naprawa: tcpip_callback_with_block() (v92).


 ROOT CAUSE (WDT crash w v90 — potwierdzony logami):
   LOCK_TCPIP_CORE() wywołany z loopTask (Arduino setup/loop, CPU 1) może blokować
   >10s zaraz po połączeniu WiFi, bo tcpip_thread jest wtedy bardzo zajęty
   (DHCP, ARP, socket init). Przez cały czas oczekiwania na lock nie ma żadnego
   esp_task_wdt_reset() → WDT kika po 10s.
   Wzorzec crashy: WDT zawsze w tym samym miejscu (~23s od bootu), tuż po logach
   PSRAM-TLS, bezpośrednio przed webserialServer.begin().

 NAPRAWA — esp_task_wdt_reset() przed i po LOCK_TCPIP_CORE():
   esp_task_wdt_reset() tuż przed lockiem daje świeże 10s okno WDT.
   Gdy sieć stabilna, LOCK_TCPIP_CORE() trwa <1s → mieści się w oknie.
   esp_task_wdt_reset() po begin() resetuje ponownie przed dalszą pracą setup().
   Wzorzec zastosowany w 3 miejscach: setup() + 2× loop() (reconnect WiFi).

 IMPLEMENTACJA (7 linii esp_task_wdt_reset()):
   Wzorzec we wszystkich miejscach webserialServer.begin():
     esp_task_wdt_reset();  // świeże 10s okno przed blokowaniem na TCPIP_CORE_LOCK
     LOCK_TCPIP_CORE();
     webserialServer.begin();
     UNLOCK_TCPIP_CORE();
     esp_task_wdt_reset();  // reset po begin()

 MIEJSCA ZMIANY:
   (1) setup() ~linia 10549: esp_task_wdt_reset() przed LOCK_TCPIP_CORE
   (2) setup() ~linia 10553: esp_task_wdt_reset() po begin()
   (3) loop() reconnect !wsReady ~linia 12258: esp_task_wdt_reset() przed LOCK
   (4) loop() reconnect !wsReady ~linia 12262: esp_task_wdt_reset() po begin()
   (5) loop() reconnect wsReady ~linia 12268: esp_task_wdt_reset() przed LOCK
   (6) loop() reconnect wsReady ~linia 12272: esp_task_wdt_reset() po begin()
   (7) Wersja firmware: v90 → v91

 WERYFIKACJA W LOGU:
   Sukces:    Boot bez WDT crash, "🌐 Terminal: http://..." w logu
   Częściowy: Boot OK ale WDT po ~10h → patrz Sekcja 14.6 roadmapy (tgTask)
   Porażka:   WDT nadal przy boot → LOCK trwa >10s → rozważyć tcpip_callback_with_block()

***********************************************v90  (2026-05-26) - FIX-TCP-LOCK: LOCK_TCPIP_CORE wokół webserialServer.begin() (AsyncTCP IDF 5.x)

 ROOT CAUSE (crash w ESPAsyncWebServer::begin() na ESP-IDF v5.x):
   webserialServer.begin() wywołuje tcp_new() wewnętrznie. W ESP-IDF v5.x tcp_new()
   zawiera LWIP_ASSERT_CORE_LOCKED() — asertuje gdy wywołane bez TCPIP_CORE_LOCK
   z user-task (loopTask). Identyczny problem jak tgClient.connect() z v75.

 NAPRAWA: LOCK_TCPIP_CORE() / UNLOCK_TCPIP_CORE() wokół begin().
   BEZ esp_task_wdt_reset() → powoduje WDT crash (naprawione w v91).

 MIEJSCA ZMIANY:
   (1) #include <lwip/tcpip.h> [linia ~2301]
   (2) setup(): LOCK_TCPIP_CORE() + webserialServer.begin() + UNLOCK_TCPIP_CORE()
   (3) loop() reconnect: analogicznie
   (4) Wersja firmware: v89 → v90

***********************************************v89  (2026-05-26) - FIX-PSRAM-TLS-A: CONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC → wszystkie alokacje mbedTLS przez hook PSRAM

 CEL: Zredukowanie TLS-DRAM delta z ~10 KB do ~4–6 KB przez wymuszenie, że
   WSZYSTKIE alokacje mbedTLS (w tym małe bloki < 4096 B) przechodzą przez
   nasz hook mbedtls_psram_calloc() z Fix D.

 DLACZEGO v88 (THRESHOLD=2048) nie zadziałało:
   Błędne założenie: ssl_context to jeden blok ~3 500 B.
   Rzeczywistość: mbedTLS alokuje ssl_context jako wiele osobnych calloc(),
   każdy < 2 048 B (mbedtls_ssl_session ~200–400 B, mbedtls_x509_crt ~500–800 B,
   cipher_context ~200–300 B itp.) — żaden nie przekracza progu → wszystkie w DRAM.
   Obniżanie progu < ~400 B jest ryzykowne (struktury WiFi/lwIP mogłyby trafić do PSRAM).

 FIX A — CONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC=y (oficjalny mechanizm Espressif):
   Z CONFIG_MBEDTLS_INTERNAL_MEM_ALLOC=y (domyślne): mbedTLS wywołuje wprost
     heap_caps_malloc(size, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL)
   dla DUŻYCH alokacji — hook mbedtls_platform_set_calloc_free() NIE jest wtedy
   wywoływany dla tych ścieżek (Fix D łapał tylko alokacje przechodzące przez hook).
   Z CONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC=y: mbedTLS kieruje WSZYSTKIE alokacje
   przez mbedtls_calloc() → nasz hook mbedtls_psram_calloc() → PSRAM lub DRAM.
   Fix D (alokator) zostaje bez zmian — teraz obsługuje pełny zakres alokacji.

 ZMIANY W sdkconfig (Fix A — plik SDK na maszynie):
   Lokalizacja: C:\Users\<user>\AppData\Local\Arduino15\packages\esp32\
                hardware\esp32\<wersja>\tools\sdk\esp32s3\sdkconfig
   ⚠️  Backup sdkconfig przed edycją! Aktualizacja arduino-esp32 core nadpisze zmiany.
   ⚠️  Po edycji sdkconfig wymagany pełny rebuild (Sketch → Clean → Upload).
   Zmiana:
     # CONFIG_MBEDTLS_INTERNAL_MEM_ALLOC is not set   ← wykomentuj/usuń tę linię
     CONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC=y              ← dodaj tę linię
   Jeśli w sdkconfig jest CONFIG_MBEDTLS_DEFAULT_MEM_ALLOC — usuń lub zakomentuj.

 OCZEKIWANE EFEKTY (vs v87/v88, przy Fix D aktywnym):
   TLS-DRAM delta:     ~10 KB → ~4–6 KB (małe bloki ssl_context przez hook)
   heap_min all-time:  ~139 KB → 145+ KB (cel roadmapy)
   fH podczas TLS:     ~142 KB — bez regresji
   Czas handshake:     +10–30 ms akceptowalne (małe bloki przez PSRAM)

 ZMIANY W KODZIE (.ino):
   (1) Changelog v89  [linia 3]
   (2) Wersja firmware: v87 → v89  [log startowy]
   (3) Aktualizacja logów setup() — dodano info o Fix A (sdkconfig)
   Fix D (mbedtls_psram_calloc + mbedtls_platform_set_calloc_free) — BEZ ZMIAN.
   PSRAM_TLS_THRESHOLD = 4096 — BEZ ZMIAN (prawidłowe; v88 potwierdziło).

 WERYFIKACJA W LOGU:
   Sukces:    [TLS-DRAM] delta ~4–6 KB, heap_min >145 KB
   Brak efektu: delta ~10 KB (Fix A nie zadziałał — sprawdź sdkconfig i pełny rebuild)
   Regresja:  delta ~44 KB → rollback sdkconfig do INTERNAL_MEM_ALLOC
   Minimalny czas testu: 30–60 minut, 10+ reconnectów

***********************************************v87  (2026-05-26) - FIX-PSRAM-TLS-C: CONFIG_MBEDTLS_DYNAMIC_BUFFER → mniejszy szczyt DRAM podczas handshake

 CEL: Dalsze zmniejszenie zużycia internal DRAM podczas TLS handshake.
   Fix D (v86) przeniósł duże bufory TX/RX (2×16 KB) do PSRAM → delta ~10 KB.
   Fix C (v87) dodaje dynamiczne zwalnianie buforów po zakończeniu handshake —
   certyfikat peer i dane konfiguracyjne są zwalniane zaraz po weryfikacji.
   Spodziewane dodatkowe oszczędności: −10 do −15 KB DRAM w stanie idle.

 ZMIANY W sdkconfig (Fix C — plik SDK na maszynie):
   Lokalizacja: C:\Users\<user>\AppData\Local\Arduino15\packages\esp32\
                hardware\esp32\<wersja>\tools\sdk\esp32s3\sdkconfig
   Dodane/zmienione linie:
     CONFIG_MBEDTLS_DYNAMIC_BUFFER=y
     CONFIG_MBEDTLS_DYNAMIC_FREE_PEER_CERT=y
     CONFIG_MBEDTLS_DYNAMIC_FREE_CONFIG_DATA=y
   ⚠️  Backup sdkconfig przed edycją! Aktualizacja arduino-esp32 core nadpisze zmiany.
   ⚠️  Po edycji sdkconfig wymagany pełny rebuild (Sketch → Clean → Upload).

 OCZEKIWANE EFEKTY (vs v86):
   heap_min all-time:       ~141 KB → ~150+ KB (cel)
   fH po zakończeniu TLS:   szybszy powrót do baseline (bufory zwalniane wcześniej)
   Spike DRAM podczas TLS:  ~10 KB → ~5 KB (cert peer zwalniany zaraz po weryfikacji)
   Czas handshake:          +20–40 ms akceptowalne (DYNAMIC_BUFFER ma minimalny narzut)

 ZMIANY W KODZIE (.ino):
   (1) Changelog v87  [linia 3]
   (2) Wersja firmware: v86 → v87  [log startowy]
   (3) Dodano log diagnostyczny [PSRAM-TLS v87] w setup() — informuje o Fix C
   Brak zmian w logice — Fix C działa wyłącznie przez sdkconfig, nie przez kod.

 WERYFIKACJA W LOGU:
   Sukces:    heap_min rośnie >141 KB, delta TLS-DRAM utrzymuje ~10 KB lub spada
   Regresja:  czas handshake wzrósł >500 ms lub pojawiły się crashe → rollback sdkconfig
   Minimalny czas testu: 60 minut, 10+ reconnectów

***********************************************v87  (2026-05-28) - OPT-B-HIST: offset tracking — eliminacja kopiowania pliku co 5 min

 ROOT CAUSE (histSave 874ms → potwierdzony benchmarkiem na ESP32-S3):
   saveHistoryPoint() przy pełnym buforze (288 linii) kopiowała ~16 KB
   bajt-po-bajcie przez API LittleFS. Benchmark wykazał avg=675ms,
   max=791ms. W połączeniu z FB-BATCH 2422ms → suma 3297ms co 5 minut.

 NAPRAWA — OPT-B: offset tracking:
   Zamiast fizycznie rotować plik co 5 min (kopia całości) →
   trzymamy globalny histStartLine = liczba "martwych" linii na początku.
   Każdy zapis to tylko append (~50ms). Plik rośnie do 2×MAX_POINTS
   (576 linii = ~32 KB), po czym jednorazowe kompaktowanie (buforowane,
   przez flagę compactHistPending, jak fsGuardPending).

 WYNIKI BENCHMARKU (ESP32-S3, 16MB flash, 288 linii, 12 cykli):
   ORIG:  avg=675ms  max=791ms
   OPT-B: avg= 57ms  max=170ms  (11.8× szybciej)
   Margines WDT po OPT-B: 10000 - 2422 - 57 = 7521ms (był: 6903ms)

 NOWE GLOBALE:
   int  histStartLine = 0          — nr martwych linii na początku pliku
   volatile bool compactHistPending — flaga kompaktowania (wzorzec fsGuardPending)

 MIEJSCA ZMIANY:
   (1) Globale: histStartLine + compactHistPending  [przy histLineCount]
   (2) saveHistoryPoint(): nowa logika rolling buffer bez kopiowania
   (3) tgTaskFn: blok compactHistPending po bloku histSavePending
   (4) /api/history: pomiń pierwsze histStartLine linii przy wysyłaniu
   (5) /api/history/clear: reset histStartLine = 0
   (6) FS-GUARD krok 3: reset histStartLine po przycinaniu history.csv

***********************************************v86  (2026-05-26) - FIX-PSRAM-TLS-D: własny alokator mbedTLS → bufory do PSRAM (naprawa regresji v85)

 ROOT CAUSE regresji v85 (potwierdzony logami):
   heap_caps_malloc_extmem_enable(PSRAM_TLS_THRESHOLD) z v85 NIE działa dla mbedTLS.
   mbedTLS w ESP-IDF ma własne hooki alokacji. Gdy sdkconfig ma
   CONFIG_MBEDTLS_INTERNAL_MEM_ALLOC=y, biblioteka wywołuje bezpośrednio:
     heap_caps_malloc(size, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL)
   — całkowicie omijając próg ustawiony przez heap_caps_malloc_extmem_enable().
   Efekt: log nadal pokazuje [TLS-DRAM] delta=-44628B mimo że "Fix 4 aktywny".

 NAPRAWA — mbedtls_platform_set_calloc_free():
   mbedTLS udostępnia hook platformowy mbedtls_platform_set_calloc_free()
   który nadpisuje WSZYSTKIE alokacje biblioteki, niezależnie od sdkconfig.
   Nasz alokator mbedtls_psram_calloc() kieruje alokacje >= PSRAM_TLS_THRESHOLD
   do PSRAM (MALLOC_CAP_SPIRAM), z bezpiecznym fallbackiem do DRAM.
   WiFi/DMA używa MALLOC_CAP_DMA — ignoruje ten alokator → zero ryzyka.

 OCZEKIWANE EFEKTY (vs v85 z regresją):
   [TLS-DRAM] delta:    −44 628 B → −2 000 do −5 000 B
   fH podczas TLS:      ~72 KB → ~107–112 KB
   heap_min all-time:   ~68 KB → ~105+ KB
   psramUsed spike:     +37–42 KB PSRAM na czas handshake

 IMPLEMENTACJA:
   1. #include "mbedtls/platform.h"  [przy innych includes]
   2. Dwie funkcje statyczne: mbedtls_psram_calloc() + mbedtls_psram_free()
      [globalnie, przy PSRAM_TLS_THRESHOLD]
   3. setup(): mbedtls_platform_set_calloc_free() zamiast heap_caps_malloc_extmem_enable()
   4. Wersja firmware: v85 → v86

 BEZPIECZEŃSTWO:
   Rollback: zakomentowanie 3 linii w setup() i 2 funkcji przywraca v85 behaviour.
   Monitoring: [TLS-DRAM] delta ~2 KB = sukces. delta ~44 KB = Fix D nie zadziałał.

***********************************************v85  (2026-05-26) - FIX-PSRAM-TLS: mbedTLS bufory przeniesione do PSRAM — eliminacja spike'u 45 KB

 ROOT CAUSE (potwierdzony 3 logami, 19h działania):
   Każdy TLS handshake (Firebase lub Telegram) pochłania ~45 KB z internal DRAM:
     mbedTLS TX buffer:        16 384 B  (= MBEDTLS_SSL_OUT_CONTENT_LEN)
     mbedTLS RX buffer:        16 384 B  (= MBEDTLS_SSL_IN_CONTENT_LEN)
     ssl_handshake_params:     ~9 000 B
     ssl_context + crypto:     ~3 500 B
     ---------------------------------
     Łącznie (worst case):    ~47 000 B z internal DRAM
   fH spada z ~117 KB → ~70–72 KB (normalnie) lub nawet 27 KB przy kaskadzie
   (potwierdzone logiem 06:16:31: fH=26 824 B → margines tylko 26 KB od OOM).
   Heap_min all-time: 47 612 B (at uptime=961s) → zbyt blisko zera dla komfortu.

 DIAGNOZA KASKADY 06:16 (log__2_.txt):
   FB-CMD t=941ms → heap nie zdążył wrócić przed kolejnym cyklem (13s) →
   trzy handshake'i pod rząd z malejącą bazą: 117→72→78→44→88→37 KB →
   dno 26 824 B przy trzecim handshake. Pełny recovery po ~25s.
   psramUsed skoczyło z 37 816 → 56 820 B podczas anomalii (+19 KB TG-TLS w PSRAM)
   → TG już częściowo korzystał z PSRAM, FB jeszcze nie.

 NAPRAWA — heap_caps_malloc_extmem_enable(PSRAM_TLS_THRESHOLD):
   heap_caps_malloc_extmem_enable(N) zmienia globalną politykę alokatora ESP-IDF:
   każda alokacja >= N bajtów, która normalnie poszłaby do internal DRAM,
   najpierw próbuje PSRAM (MALLOC_CAP_SPIRAM). Przy sukcesie → PSRAM.
   Tylko jeśli PSRAM pełny lub brak → wraca do DRAM (bezpieczny fallback).
   Alokacje < N bajtów zawsze w DRAM (WiFi DMA, stos, IPC, small objects).
   WiFi/lwIP używa MALLOC_CAP_DMA — ignoruje ten próg → zero ryzyka dla WiFi.

   Wartość progu: PSRAM_TLS_THRESHOLD = 4096 B.
     • mbedTLS TX/RX bufory (2 × 16 384 B) >> 4096 → trafiają do PSRAM
     • ssl_handshake_params (~9 000 B)        >> 4096 → trafia do PSRAM
     • ssl_context (~3 500 B)                  < 4096 → pozostaje w DRAM (ok)
     • Wszystkie alokacje <4 kB (WiFi, FreeRTOS, lwIP) → DRAM bez zmian

 OCZEKIWANE EFEKTY (vs v84):
   fH podczas TLS handshake:   ~72 KB → ~107–112 KB (+35–40 KB)
   fH minimum all-time:        47 612 B → ~100+ KB (szacunek)
   Kaskada 3× handshake:       dno 26 KB → dno ~95 KB (eliminacja ryzyka OOM)
   psramUsed podczas TLS:      +37–42 KB PSRAM na czas handshake (oba klienty)
   maxAlloc:                   63 476 B → stabilne 63+ KB przez całą sesję
   Szybkość handshake:         +50–150 ms (PSRAM 3× wolniejszy od DRAM → ~10–20 ms
                               per 16 KB buffer = łącznie ~30–50 ms overhead przy AES-NI)
   TLS-COOL:                   może zniknąć — heap defragmentuje się szybciej
                               gdy 45 KB alokacji idzie do PSRAM nie DRAM.

 IMPLEMENTACJA (1 linia kodu + 2 logi diagnostyczne):
   1. Stała:  static const size_t PSRAM_TLS_THRESHOLD = 4096;  [przy tlsMutex]
   2. setup() po bloku PSRAM-DIAGNOSTICS:
        heap_caps_malloc_extmem_enable(PSRAM_TLS_THRESHOLD);
        log: ✅ [PSRAM-TLS] próg=4096B — mbedTLS TX/RX bufory (2×16KB) → PSRAM
   3. fbConnect() i tgEnsureConnected(): dodano pomiar internal heap PRZED i PO connect()
        log: [TLS-DRAM] before=NkB after=NkB delta=-NkB heapSrc=FB_TLS/TG_TLS
        Weryfikuje czy bufor rzeczywiście trafił do PSRAM (delta ~2KB = sukces Fix 4).
        Jeśli delta > 20KB → PSRAM niedostępny, fallback na DRAM (log: ⚠️).

 BEZPIECZEŃSTWO:
   Rollback: komentarz jednej linii w setup() przywraca v84 behaviour.
   Monitoring: psramUsed w [HEAP] heartbeat pokaże +35–42 kB przy handshake.
   Alarm: jeśli delta TLS-DRAM > 20 KB → PSRAM nie działa → log ostrzeżenia.

 MIEJSCA ZMIANY:
   (1) static const size_t PSRAM_TLS_THRESHOLD = 4096  [nowy global przy tlsMutex]
   (2) setup(): heap_caps_malloc_extmem_enable() po PSRAM-DIAGNOSTICS
   (3) tgEnsureConnected(): pomiar DRAM przed/po connect() → log [TLS-DRAM]
   (4) fbConnect():         pomiar DRAM przed/po connect() → log [TLS-DRAM]
   (5) Wersja firmware: v84 → v85

***********************************************v84  (2026-05-25) - FIX-TLS-GAP: cooldown 3s między handshake'ami — defragmentacja heap

 DIAGNOZA (po analizie logów v83):
   tlsMutexTotalTG=0 tlsMutexTotalFB=0 we WSZYSTKICH heartbeatach v83 — mutex
   NIGDY nie był contested. TLS handshake'i są już idealnie zserializowane przez
   istniejący tlsMutex z v67. Fix 1 (TCP keepalive) nie zmienił minimum heap, bo
   Firebase RTDB zamyka przez HTTP "Connection: close", nie przez idle TCP FIN.

 ROOT CAUSE (heap minimum ~69 KB = niezmienione względem v82):
   69 KB = 117 KB − 48 KB pochłonięte przez JEDEN TLS handshake mbedTLS.
   mbedTLS alokuje TX buffer (16 KB) + RX buffer (16 KB) + SSL context + cert =
   ~40–48 KB internal DRAM na czas handshake. To granica fizyczna — niemożliwa
   do ominięcia bez (a) zmniejszenia buforów mbedTLS (wymaga rekompilacji ESP-IDF)
   lub (b) użycia ESP_SSLClient (Fix 4) który może przesunąć bufory do PSRAM.
   Jednorazowe spike'i 37↔48 KB wynikają z fragmentacji sterty — przy kolejnych
   alokacjach mbedTLS może potrzebować nieco więcej lub mniej w zależności od
   układu wolnych bloków.

 CO ROBI FIX-TLS-GAP (Fix 2 — efekt marginalny ale poprawny):
   Po zwolnieniu mutexa przez task A (FB lub TG) — task B musi odczekać minimum
   TLS_COOLDOWN_MS=3000ms zanim zacznie własny handshake. Oczekiwanie realizowane
   jest WEWNĄTRZ sekcji mutex (zaraz po acquire), z esp_task_wdt_reset() co 100ms.
   Efekt: heap ma 3s na defragmentację między handshake'ami → mbedTLS może dostać
   lepiej ułożoną stertę → potencjalnie niższy worst-case spike. Brak gwarancji
   poprawy minimum, ale eliminuje skumulowaną fragmentację przy szybkich reconnectach.

 IMPLEMENTACJA:
   Nowy global: volatile uint32_t g_lastTlsHandshakeDoneMs = 0
   Stała:       TLS_COOLDOWN_MS = 3000
   Logika cooldown wstrzyknięta PO acquire mutexa w obu miejscach:
     (1) tgEnsureConnected() — po [TLS-MX] FB: locked
     (2) fbConnect()         — po [TLS-MX] TG: locked
   Po udanym connect(): g_lastTlsHandshakeDoneMs = millis()
   Log: [TLS-COOL] waited Xms (cooldown po poprzednim handshake)

 OCZEKIWANE EFEKTY (ostrożna ocena):
   Heap minimum: potencjalnie niezmienione lub +2–5 KB (defragmentacja)
   Heap oscylacje: wygładzone (mniej spike'ów w oknie 30s heartbeatu)
   Jedyna PEWNA naprawa heap: Fix 4 (ESP_SSLClient + PSRAM TLS buffers)

 MIEJSCA ZMIANY:
   (1) volatile uint32_t g_lastTlsHandshakeDoneMs — nowy global przy tlsMutex
   (2) tgEnsureConnected(): cooldown po acquire mutexa + update po connect()
   (3) fbConnect():         cooldown po acquire mutexa + update po connect()
   (4) Wersja firmware: v83 → v84

***********************************************v83  (2026-05-25) - FIX-TCP-KA: TCP Keepalive eliminacja idle-reconnect Firebase/Telegram

 ROOT CAUSE (FB-CONN reconnect co ~13s + heap spike do 67 KB):
   Firebase RTDB serwer wysyła aktywny TCP FIN po ~15s bezczynności połączenia.
   To nie jest timeout NAT/firewall — to celowe zachowanie backendu Google.
   Każdy reconnect = pełny TLS handshake mbedTLS = spike ~40–46 KB internal DRAM
   przez ~500–2000ms. Przy 3 operacjach FB/min system był w ciągłym cyklu:
   handshake → transfer → idle 13s → server FIN → reconnect → handshake.
   Identyczny problem dotyczył tgClient (Telegram api.telegram.org).

 NAPRAWA (FIX-TCP-KA):
   TCP Keepalive na poziomie lwIP socket (poniżej mbedTLS, bez wpływu na RAM).
   Parametry: idle=5s, interval=5s, count=3 — lwIP wysyła małe ACK co 5s braku
   ruchu → serwer widzi aktywne połączenie → nie wysyła FIN → idle_time < 5s stale.
   Keepalive musi być aplikowany PO każdym connect() (nowy fd = stare opcje znikają).
   Dodano funkcję applyTcpKeepalive() wywoływaną w 3 miejscach:
     (1) tgEnsureConnected() — po bloku SO_SNDTIMEO
     (2) fbConnect()         — po bloku SO_SNDTIMEO
     (3) sendTelegramDocument() — po bloku SO_SNDTIMEO (połączenie "close",
         keepalive mniej krytyczny ale konsekwentny)
   Dodano #include <netinet/tcp.h> dla stałych TCP_KEEPIDLE/KEEPINTVL/KEEPCNT.

 OCZEKIWANE EFEKTY (vs v82):
   FB-CONN reconnect: z co ~13s → co kilka godzin (lub nigdy przy ciągłym ruchu)
   Heap minimum: z ~67 KB → ~80–100 KB (mniej równoczesnych TLS handshake'ów)
   Heap oscylacje: z 67↔117 KB → 100↔117 KB (wygładzone)
   Czas blokady TLS task: eliminacja 500–2000ms rekonnectów

 MIEJSCA ZMIANY:
   (1) #include <netinet/tcp.h> — nowy nagłówek (TCP_KEEPIDLE/KEEPINTVL/KEEPCNT)
   (2) applyTcpKeepalive() — nowa funkcja pomocnicza (20 linii, po applySocketTimeouts)
   (3) tgEnsureConnected(): wywołanie applyTcpKeepalive(tgClient) po SO_SNDTIMEO
   (4) fbConnect():         wywołanie applyTcpKeepalive(fbClient) po SO_SNDTIMEO
   (5) sendTelegramDocument(): wywołanie applyTcpKeepalive(tgClient) po SO_SNDTIMEO
   (6) Wersja firmware: v82 → v83

***********************************************v82  (2026-05-25) - FIX-WDT-MUTEX + FIX-WDT-SNDTIMEO: eliminacja WDT crash loop

 ROOT CAUSE A (FIX-WDT-MUTEX) — xSemaphoreTake z 10s blokowaniem bez WDT reset:
   Poprzedni kod: xSemaphoreTake(tlsMutex, pdMS_TO_TICKS(10000)) — jednorazowe
   blokujące wywołanie mogące wstrzymać tgTask na do 10 sekund bez żadnego
   esp_task_wdt_reset() w środku. Przy WDT timeout = 10s to wyścig: każde
   opóźnienie schedulera lub jitter sieci powodował crash.
   Dokumentacja ESP-IDF i Arduino (zbotic.in): "TWDT odpala gdy task nie wywoła
   esp_task_wdt_reset() — nawet gdy zablokowany na semaforze".
   Potwierdzony crashami: tgTask (CPU 0) wisiał >10s.

 NAPRAWA (FIX-WDT-MUTEX):
   W obu miejscach (tgEnsureConnected i fbConnect) zastąpiono jednorazowy blok
   pętlą polling co 100ms z esp_task_wdt_reset() przy każdej iteracji:
     while (millis() - start < 9000) {
       if (xSemaphoreTake(mutex, 100ms)) { locked=true; break; }
       esp_task_wdt_reset();  // feed WDT co 100ms
     }
   Wynik: WDT niemożliwy podczas czekania na mutex. Timeout 9s < WDT 10s.

 ROOT CAUSE B (FIX-WDT-SNDTIMEO) — WiFiClientSecure send_ssl_data hang:
   Bug Arduino ESP32 #7356: send_ssl_data() (wywołane przez client.write()
   wewnątrz http.sendRequest / http.GET) NIE respektuje setTimeout() przy
   black-hole TCP — gdy serwer lub sieć nie przyjmuje danych (TCP send buffer
   pełny), write() wisi w pętli nieskończenie bez WDT reset → crash po >10s.
   http.setTimeout(8000) ustawia tylko ODCZYT danych, nie ZAPIS.
   Dotyczy trzech ścieżek: fbClient w sendStatusToFirebase/checkFirebaseCommands/
   checkFirebaseConfig, tgClient w tgEnsureConnected, tgClient w sendTelegramDocument.

 NAPRAWA (FIX-WDT-SNDTIMEO):
   Po każdym udanym connect() ustawiane SO_SNDTIMEO + SO_RCVTIMEO = 8s przez
   setsockopt() na surowym socket fd (client.fd()):
     int sock = fbClient.fd();
     struct timeval tv = {8, 0};
     setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
     setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
   SO_SNDTIMEO działa na poziomie lwIP BSD socket API — wymusza powrót z
   send()/write() po 8s niezależnie od stanu sieci → WDT crash niemożliwy.
   Dodano #include <sys/socket.h> do listy nagłówków.

 MIEJSCA ZMIANY:
   (1) #include <sys/socket.h> — nowy nagłówek
   (2) tgEnsureConnected(): polling mutex + SO_SNDTIMEO/SO_RCVTIMEO po connect()
   (3) fbConnect():          polling mutex + SO_SNDTIMEO/SO_RCVTIMEO po connect()
   (4) sendTelegramDocument(): SO_SNDTIMEO/SO_RCVTIMEO po connect()

***********************************************v81-DIAG  (2026-05-24) - MAX-LOG: wersja diagnostyczna do testów na gołym ESP

 CEL: wykrycie każdego odchylenia od założeń.
 Wersja do testów — NIE WGRYWAĆ na produkcję (zbyt duży ruch logów).

 CO DODANO (vs v80):
   [TG-ALIVE]       co 5s (było 15s) — szybkie wykrycie zawieszenia tgTask
   [TG-POST]        KAŻDE wywołanie logowane (było: tylko wolne/błędy)
                    + body size wysyłany, + pełny timing DNS+connect+send+recv
   [FB-CONN]        KAŻDY reconnect z PRZYCZYNĄ (ready/conn/idle_ms/threshold)
                    + czas TCP+TLS oddzielnie + batch-hit logowany
   [FB-SEND/CMD/CFG] KAŻDE wywołanie z HTTP code + timing (było: tylko Komentarze)
   [TG-CONN]        KAŻDE połączenie TG z timing i wynikiem
   [TG-DEL]         KAŻDE deleteTelegramMessage z timing
   [TG-HIST-DEL]    KAŻDE tgDeleteAllHistory z per-msg timing
   [FB-BLOCK]       KAŻDA iteracja tgTask — flagi s/c/g + freeH
   [CMD-RUN]        KAŻDA komenda switch(cmd) z timing start→done
   [TLS-MX]         KAŻDY mutex acquire z czasem oczekiwania (było: tylko >50ms)
   [HEAP-5S]        Heartbeat pamięci co 5s (było: co 60s)
   [LOOP-DIAG]      Heartbeat pętli głównej co 10s
   [POLL]           Każdy getUpdates z ilością updateId w odpowiedzi
   [WIFI]           Zmiana statusu WiFi logowana natychmiast

***********************************************v80  (2026-05-24) - FIX-WDT-CONNECT: timeout connect() + wdt_reset po connect()

 ROOT CAUSE (WDT crash: fbConnect() zajmuje 12021ms > WDT 10s przy FAIL):
   connect() w WiFiClientSecure jest wywołaniem BLOKUJĄCYM — TCP + TLS handshake
   wykonywane synchronicznie. setTimeout(6) + setConnectionTimeout(4000) powinny
   dawać max 10s, lecz w praktyce (przy złym sygnale / busy serwerze) przekraczały
   10s (zmierzono 12021ms). Poprzedni kod miał esp_task_wdt_reset() PRZED connect(),
   ale nie PO — więc jeśli connect() trwał >10s, WDT strzelał w środku.
   Potwierdzone logiem v79: [TG-HUNT] fbConnect 12021ms ok=0.
   System nie crashnął w v79 tylko dlatego, że mutex-wait (xSemaphoreTake) był w stanie
   BLOCKED (FreeRTOS nie liczy jako "running"), co resetowało okno TWDT pośrednio.
   Jednak nie jest to gwarantowane — zależy od schedulera.

 NAPRAWA (FIX-WDT-CONNECT):
   [1] Zmniejszenie timeoutów: setTimeout(6→4) + setConnectionTimeout(4000→3000ms)
       → max blok connect() = TCP(3s) + TLS(4s) = 7s = margines 3s do WDT.
   [2] esp_task_wdt_reset() bezpośrednio PO connect() we wszystkich 3 miejscach:
       - tgEnsureConnected()
       - sendTelegramDocument()
       - fbConnect()
       Gwarantuje pełne 10s okno dla while-loop po connect(), niezależnie
       od czasu trwania samego connect().

 CLEANUP: Usunięto 40 tymczasowych logów [TG-HUNT] i [FB-TIMING] z v79.
   Root cause znaleziony — logi debugowania nie są już potrzebne.
   Pozostały produkcyjne logi: [FB-CONN], [TG-CONN], [FB-DNS], [TG-DNS].

***********************************************v75  (2026-05-24) - FIX-WDT-DNS: eliminacja assert udp_new_ip_type + WDT crash w tgTask

 ROOT CAUSE (WDT crash tgTask po ~24 min: assert failed: udp_new_ip_type):
   tgClient.connect("api.telegram.org", 443) i fbClient.connect(FIREBASE_HOST, 443)
   są w pełni blokującymi wywołaniami. Wewnętrznie wykonują:
     1. DNS lookup przez getaddrinfo() → udp_new_ip_type() BEZ TCPIP_CORE_LOCK
     2. TCP connect (do 4s przez setConnectionTimeout)
     3. TLS handshake (do 8s przez setTimeout)
   Problemy:
   (A) ASSERT: udp_new_ip_type() w ESP-IDF v5.x asertuje LWIP_ASSERT_CORE_LOCKED()
       gdy wywoływane z user-task (tgTask) bez blokady TCPIP core. Identyczny
       mechanizm jak stary bug HTTPClient naprawiony komentarzem przy linii ~13599
       ("http.begin(fbClient,url) omija ścieżkę udp_new() bez TCPIP_CORE_LOCK").
       tgClient.connect() i fbClient.connect() nie miały analogicznej poprawki.
   (B) WDT: DNS(~2s) + TCP(4s) + TLS(8s) = do 14s > WDT 10s. esp_task_wdt_reset()
       przed connect() nie pomaga — WDT strzela WEWNĄTRZ jednolitej blokady connect().
       while z WDT resetami nigdy nie jest osiągany dopóki connect() nie wróci.
   Dlaczego po ~24 min: cache DNS wygasa (TTL FIREBASE_HOST bywa 1800s = 30 min;
   po starcie pierwsze połączenia używają wpisu cache → brak UDP socket → brak assertu.
   Po wygaśnięciu cache: fresh DNS query → udp_new_ip_type() → crash.

 NAPRAWA (FIX-WDT-DNS):
   Pre-resolve hostname przez WiFi.hostByName() (wywołanie dyspatchujące do TCPIP task
   — bezpieczne z user-task, nie wymaga TCPIP_CORE_LOCK) z esp_task_wdt_reset() po.
   Następnie connect(resolvedIP, 443) — bez DNS → brak ścieżki udp_new_ip_type().
   Cache IP na 5 minut (g_tgServerIP, g_fbServerIP) — re-resolve gdy IP zmiana lub expiry.
   Fallback na hostname gdy hostByName() fail (sieć offline).
   Zmiana setTimeout(8) → setTimeout(6): DNS via hostByName osobno, connect() robi
   tylko TCP(4s) + TLS(6s) = max 10s, co przy świeżym WDT reset tuż przed gwarantuje
   że zmieści się w oknie. Przy dobrej sieci TLS zajmuje 1-3s → 100% marginesu.

 MIEJSCA ZMIANY:
   (1) Globale: g_tgServerIP/g_tgIPTs/g_tgIPValid, g_fbServerIP/g_fbIPTs/g_fbIPValid
   (2) tgEnsureConnected(): WiFi.hostByName() + setTimeout(6) + connect(IP)
   (3) sendTelegramDocument(): WiFi.hostByName() (reuse cache) + setTimeout(6) + connect(IP)
   (4) fbConnect(): WiFi.hostByName() + setTimeout(6) + connect(IP)
   (5) Wersja firmware: v74 → v75

***********************************************v74  (2026-05-24) - FIX-FB-SYNC: panel Firebase identyczny z lokalnym

 ROOT CAUSE (panel Firebase pokazuje inne wartości niż lokalny):
   sendStatusToFirebase() brakowało 5 pól obecnych w /api/status:
   autoPWM, luxPerPwm, glassTransm, rampSec, pct.
   Panel Firebase liczy skalę paska z d.autoPWM — bez niego używał
   starych danych Firebase lub fallbacku 500 lx, stąd ~88% zamiast 100%.

 NAPRAWA (FIX-FB-SYNC):
   Dodano do sendStatusToFirebase() (przed updatedAt):
   autoPWM, luxPerPwm, glassTransm, rampSec, pct.
   Po wgraniu: oba panele pokazują identyczne wartości PWM i skalę paska.

***********************************************v73  (2026-05-24) - FIX-FLASH3 + FIX-SECT5: Błysk LED po restarcie (kompletna naprawa)

 ROOT CAUSE (FLASH#3 potwierdzony logami [SSTART-INIT] z v72):
   Soft-start inicjalizował softStartTargetPwm[]=1023 (backupAuto).
   Podczas soft-startu balancer był wyłączony (anyRampNow=true, gPowerScale=1.0).
   Po zakończeniu rampy balancer odkrywał 118.7W > 90W limit → natychmiastowy
   spadek gPowerScale 1.0->0.758 = delta -247 PWM w jednej iteracji = błysk widoczny
   gołym okiem. Log [FLASH#3] SKOK HARDWARE PWM WYKRYTY! potwierdzał każdy restart.

 NAPRAWA (FIX-FLASH3):
   Nowa inline funkcja capTargetsToPowerLimit(uint16_t targets[5]):
     - Oblicza getTotalLEDPower(targets) vs LED_MAX_POWER_W
     - Jeśli przekroczono: skaluje targets[] in-place do limitu mocy
     - Zwraca zastosowaną skalę (1.0 = brak ograniczenia)
   Wywołana w 4 miejscach inicjalizacji softStartTargetPwm[]:
     1. Boot restart w oknie świecenia (trybAuto, shouldBeOn=true)
     2. Boot restart w trakcie rampy (mid-ramp dobieg 30s)
     3. Przejście MANUAL->AUTO (handleSetMode)
     4. Panel zmiana suwaka podczas rampy adaptacyjnej
   Rezultat: soft-start kończy się DOKŁADNIE na poziomie 90W → balancer
   po wyjściu z anyRampNow widzi targetScale=1.0 → zero skoku → zero błysku.
   Log [SSTART-INIT] zmieniony: ⚠️ zastąpione przez ✅ FIX-FLASH3.

 ROOT CAUSE (FIX-SECT5 odkryty z logów v73):
   Po zakończeniu soft-startu (backup=775) sekcja 5 (adapt=OFF) kopiowała
   backupAutoBrightnessComposite=1023 z powrotem do backupBrightnessComposite.
   Balancer musiał permanentnie pracować z gPowerScale=0.758 (zamiast skoczyć
   tylko raz przy init). System działał poprawnie ale stale "przez ogranicznik".

 NAPRAWA (FIX-SECT5):
   Sekcja 5 (adapt=OFF): przed skopiowaniem backupAuto → backup stosuje
   capTargetsToPowerLimit() na tymczasowej kopii, tak żeby backup zawsze
   odzwierciedlał realne możliwości zasilacza.
   Log jednorazowy: [SECT5-FIX] adapt=OFF kap zadziałał: skala=... ✅ FIX-SECT5

***********************************************v72  (2026-05-23) - FIX-LOG-CLEAR: usuwanie log.txt przez tgTask

 ROOT CAUSE (logi nie dają się wyczyścić mimo komunikatu [OK]):
   esp_littlefs (warstwa VFS ESP32) blokuje LittleFS.remove() gdy plik jest
   aktualnie otwarty przez inny task (zwraca EACCES / -1, errno=13).
   Handler HTTP /api/log-clear działał BEZ logMutex → trafiał w okno flush'a
   (tgTask: open→write→close co 5s, przy 2.74MB logu praktycznie zawsze)
   → remove() zwracał -1 → handler ignorował błąd (brak sprawdzenia wartości)
   → wysyłał [OK] mimo że plik NIE został usunięty.
   W v71 dodano zerowanie buforów PRZED remove(), co nie naprawia kolizji
   z otwartym uchwytem pliku w tgTask.

 NAPRAWA:
   Wzorzec logClearPending (identyczny jak fsGuardPending / histSavePending):
   - Handler HTTP: tylko ustawia volatile bool logClearPending = true.
   - tgTask (Core 0): obsługuje flagę po zamknięciu bieżącego uchwytu LittleFS,
     pod logMutex → gwarantuje że plik NIE jest otwarty podczas remove().
   - tgTask zeruje oba bufory PSRAM + anuluje logFlushPending[1] + remove().
   - TG_CLR_DO (Telegram): działa na Core 0 pod logMutex → bezpieczny, ale
     dodano logFlushPending[1]=false dla spójności.

***********************************************v71  (2026-05-19) - FIX: 3 bugi z analizy crashloop/FLASH/LED-skok

 [FIX-A] WDT crash loop przy braku TSL + bootNotify w tej samej iteracji:
   Po bloku tslReinitPending brakowało esp_task_wdt_reset() i continue.
   Gdy oba TSL martwe i bootNotify pending: czujnikNadWoda.begin() (~8s)
   + sendTelegramMessage() (~8s) = 16s > WDT 10s → WDT_TASK crash.
   NAPRAWA: dodano esp_task_wdt_reset() + continue na końcu bloku tslReinitPending.
   Gwarantuje że TSL reinit i bootNotify nigdy nie nakładają się w jednej iteracji.

 [FIX-B] Skok LED po restarcie (FIX-v16 startuje z backupAuto zamiast real PWM):
   backupBrightnessComposite nie był zapisywany do EEPROM. Po restarcie FIX-v16
   używał backupAuto=275 (maks harmonogramowy), stąd skok np. 27→140 w jednej chwili.
   NAPRAWA: nowy adres EEPROM 148 (uint64_t, 8 bajtów).
   Zapis przy każdej zmianie backupBrightnessComposite (w setBrightnessComposite).
   Odczyt przy starcie — FIX-v16 użyje rzeczywistej ostatniej jasności.

 [FIX-C] FLASH LED przy zapisie harmonogramu z panelu WWW (FLASH#4):
   handleSetSchedule resetował autoRampInitialized=false gdy rampScheduleActive=true.
   FIX-v16 inicjował rampę z backupAuto=275, elapsed=54/60min → target=0 → PWM=24.
   LED skakał z ~233 → 24 (błysk widoczny gołym okiem).
   NAPRAWA: przed resetem autoRampInitialized zaktualizuj backupBrightnessComposite
   do aktualnych wartości currentRampPwm, żeby FIX-v16 startował z właściwego miejsca.

 [DIAG] loopStack_free HWM — jednorazowy log gdy wartość spada do nowego minimum.
   Wyjaśnienie: HWM FreeRTOS działa w jedną stronę (historyczne minimum).
   Spadek 5244→4876B po nocnym raporcie TG-DOC to normalne zachowanie,
   nie wyciek. Potwierdzono testem loopStack_diag.ino (2026-05-20).
   Alarm 🚨 gdy loopStack_free < 2000B.

***********************************************v69b (2026-05-18) - POSTAUDIT: W2/W4/W5/W7 poprawki

 [FIX-W2] Usunięto martwy kod g_heapSrc (volatile HeapSrcTag):
   Zmienna była ustawiana w tgEnsureConnected/fbConnect od v67,
   ale po wprowadzeniu g_heapSrcAccum (v69-Z1) nigdy nie była odczytywana.
   Usunięto deklarację + 2 bloki HeapSrcTag _prevSrc w obu funkcjach TLS.
   Enum HeapSrcTag i heapSrcName() zachowane (używane przez g_heapSrcAccum).

 [FIX-W4] HEAP-DRIFT CRIT log — dodano min60=:
   Ścieżka CRIT (drift>=3h) nie miała min60= w odróżnieniu od ścieżek
   trend i OK. Dodano min60=%ukB do logPrintf CRIT.

 [FIX-W5/W6] g_tlsMutexWaitTG/FB: uint16_t → uint32_t:
   Zapis 32-bit do wyrównanego adresu jest atomowy na Xtensa LX7 (S32I).
   Eliminuje potencjalny torn read przy cross-core odczycie w heartbeat.
   Format logPrintf zaktualizowany %u → %lu.

 [FIX-W7] Etykiety liczników TLS-MUTEX w heartbeat:
   tlsMutexWaitTG/FB → tlsMutexTotalTG/FB (kumulatywne od startu).
   tlsMutexMaxWait → tlsMutexPeakWait (peak od startu).

 [FIX-W3] Próg MEM-FREE: 4096 → 1024 B w ścieżkach OOM/timeout/FB-FAIL:
   Obniżenie progu zwiększa szansę emisji logu MEM-FREE przy fragmentacji.

***********************************************v69  (2026-05-18) - DIAG: Z0-Z5 audyt rozszerzony

 [Z0] FIX-Z5 v68: INPUT → INPUT_PULLDOWN w DS-DIAG:
   pinMode(ONE_WIRE_BUS3, INPUT_PULLDOWN) — deterministyczny odczyt.
   Brak pull-up: wewnętrzny ~45kΩ do GND → zawsze LOW (nie floating).
   g_ds3PinLevel (volatile int8_t) przekazuje wynik do Telegram alertu.

 [Z1] OR-akumulator g_heapSrcAccum zamiast g_heapSrcLast (last-write-wins):
   Back-to-back TG+FB handshake'i teraz widoczne jako heapSrc=BOTH.
   Zmiana z volatile HeapSrcTag → volatile uint8_t, 0 B netto DRAM.

 [Z2] MEM-FREE instrumentacja brakujących ścieżek stop():
   2a: tgPost() OOM path: src=TG-OOM
   2b: tgPost() timeout/noHeaders: src=TG-TIMEOUT
   2c: fbConnect() FAIL path: src=FB-FAIL

 [Z3] HEAP-DRIFT: min60 + numer cyklu:
   g_heapMin60 — minimum freeH z bieżącej godziny (odporność na TLS spike'i).
   g_heapDriftCycle — numer godziny od startu.
   Log: 💧 [HEAP-DRIFT] cycle=N ... min60=XkB ...

 [Z4] DS-DIAG: reset pulse (oneWire3.reset()) + ROM scan (getAddress):
   4 scenariusze: brak pull-up / pull-up OK brak czujnika / CRC err / OK.
   g_ds3ResetOk + g_ds3AddrOk przekazywane do Telegram alertu.

 [Z5] TLS-MUTEX statystyki czasu oczekiwania:
   g_tlsMutexWaitTG/FB — licznik czekań >50ms.
   g_tlsMutexMaxWaitMs — najdłuższe oczekiwanie.
   Log: ⏳ [TLS-MUTEX] TG/FB czekał Xms na mutex.
   Heartbeat: tlsMutexTotalTG= tlsMutexTotalFB= tlsMutexPeakWait=
   (etykiety zaktualizowane w v69b FIX-W7)

 DRAM netto: +19 B (g_ds3PinLevel 1B + g_heapMin60 2B + g_heapDriftCycle 2B
             + g_ds3ResetOk 1B + g_ds3AddrOk 1B + g_tlsMutexWait* 12B)
             (10B→12B po FIX-W5/W6: 2×uint16+uint32 → 3×uint32; suma 17→19B)

***********************************************v68  (2026-05-18) - BUGFIX: Z1/Z4/Z5 poprawki audytu

 [FIX-Z1] heapSrcLast przy FAIL:
   g_heapSrcLast ustawiana zawsze (nie tylko gdy *ClientReady==true).
   Powód: mbedTLS alokuje ~47 kB również przy nieudanym handshake.
   Heartbeat nie pokazywał heapSrc= przy connect()=FAIL.

 [FIX-Z2] MEM-FREE dla sendTelegramDocument():
   Dodano pomiar freeH_before/after wokół tgClient.stop()
   w sendTelegramDocument() — ta ścieżka mogła odpowiadać za
   anomalię 158.4 kB (Connection:close po przesłaniu pliku).

 [FIX-Z4] HEAP-DRIFT rolling window — ciągły monitoring:
   Usunięto błędny reset g_heapHistFill=0 po analizie.
   Warunek analizy zmieniony na g_heapHistIdx==0 (obrót kołowy).
   Poprzedni kod tworzył "ślepą plamkę" 59 min po każdej analizie.

 [FIX-Z5] DS-DIAG — pomiar napięcia na pinie DATA:
   Dodano digitalRead(ONE_WIRE_BUS3) przed pętlą prób.
   Log: voltage=HIGH/LOW — natychmiast diagnozuje brak pull-up.
   Wymaganie spec. pominięte w v67.

***********************************************v67  (2026-05-18) - HEAP-DEBUG: heapSrc + MEM-FREE + TLS-MUTEX + HEAP-DRIFT + DS-DIAG

 [Z1] HeapSrcTag — klasyfikacja źródła dołków HEAP:
   Enum HeapSrcTag {IDLE, TG_TLS, FB_TLS, BOTH} + globalna g_heapSrc.
   Ustawiana tuż przed TLS connect (tgEnsureConnected / fbConnect),
   kasowana po zakończeniu. Heartbeat loguje: heapSrc=TG_TLS / FB_TLS / BOTH.
   Eliminuje zgadywanie z korelacji czasowej — źródło skoków znane wprost.

 [Z2] MEM-FREE log — wyjaśnienie anomalii 158.4 kB:
   Przed i po tgClient.stop() / fbClient.stop() loguje:
     🆓 [MEM-FREE] src=TG/FB freeH_before=NB freeH_after=NB delta=+NkB
   Pozwala precyzyjnie zidentyfikować który stop() zwalnia duży blok TLS.

 [Z3] TLS-MUTEX — serializacja handshake'ów TG i FB:
   Nowy SemaphoreHandle_t tlsMutex. Tworzony w setup().
   tgEnsureConnected() i fbConnect() biorą mutex przed connect(),
   oddają po ustawieniu *ClientReady. Max 1 handshake TLS naraz.
   Oczekiwany efekt: minimum freeH z ~70 kB → ~90 kB (+20 kB).
   KOSZT: 88 B DRAM (FreeRTOS semaphore).

 [Z4] HEAP-DRIFT — wykrywanie powolnego wycieku DRAM:
   g_heapHist60[60]: rolling window minut freeH (uint16_t kB).
   Co minutę (heartbeat): push nowy odczyt.
   Co godzinę: porównaj avg ostatnich 60 min z avg poprzedniej godziny.
   WARN gdy drift > 5 kB/h, CRIT gdy 3 kolejne godziny w dół.
   Log: 💧 [HEAP-DRIFT] trend=-3.2kB/h avg60=71.2kB prevAvg=74.4kB
   KOSZT: 60×2 = 120 B DRAM (BSS).

 [Z5] DS-DIAG — aktywna diagnostyka DS18B20 przy starcie:
   Po sensors3.begin() w setup(): 3 próby odczytu co 500ms.
   Log: 🔌 [DS-DIAG] pin=GPIO39 devices=N attempt=1/3 t=XX.X°C → OK/FAIL
   Jeśli FAIL: ustawia ds3BootFail=true → tgTask wysyła alert Telegram
     "⚠️ DS18B20 wody FAIL (GPIO39): sprawdź kabel i pull-up 4.7kΩ"
   KOSZT: ~200ms jednorazowy delay w setup(), 1 bool DRAM.

***********************************************v65  (2026-05-15) - FB-BATCH: jeden TLS handshake na cały blok Firebase

 ROOT CAUSE (FB-CONN: 2–4 reconnecty co ~7s):
   Firebase REST API odpowiada Connection:close → http.end() zamyka socket
   → fbClient.connected()=0 → każde kolejne wywołanie fbConnect() w tym samym
   cyklu (sendStatus/checkCmd/checkCfg) robi pełny TLS handshake (~47 kB DRAM,
   ~300–800 ms). setReuse(true) i debounce 2s (v64-P1a) nie pomagały —
   socket był zamykany przez serwer, nie przez klienta.

 NAPRAWA — fbBatchActive (max 1 TLS handshake na blok FB):
   [1] bool fbBatchActive = false — nowa flaga globalna.
   [2] W tgTaskFn: przed blokiem fbSendPending/fbCmdPending/fbCfgPending
       jeśli którakolwiek flaga jest ustawiona → fbConnect() RAZ,
       fbBatchActive=true. Po bloku: fbBatchActive=false.
   [3] fbConnect(): jeśli fbBatchActive==true AND fbClientReady==true →
       pomiń reconnect, return true (nawet gdy socket=0 — błąd HTTP obsłużymy).
   [4] W sendStatusToFirebase/checkFirebaseCommands/checkFirebaseConfig:
       po HTTP error (code<0 lub !=2xx) AND fbBatchActive → fbClientReady=false
       → następna funkcja w bloku zrobi prawdziwy reconnect (nie skip).

 EFEKT: max 1 TLS handshake / cykl FB zamiast 3–4.
   DRAM spike ~47 kB: 1× na cykl zamiast 3–4×.
   [FB-CONN] reconnect: pojawia się ≤1× na blok, nie 2–4×.

***********************************************v64  (2026-05-15) - FB-DEBOUNCE + TLS-SESSION + DIAG-HEAP + TSL-RETRY

 [P-1a] fbLastSuccessMs — debounce w fbConnect():
   Nowa zmienna globalna (unsigned long fbLastSuccessMs = 0).
   needReconnect: !fbClient.connected() wyzwala reconnect tylko gdy
   millis()-fbLastSuccessMs > 2000ms. Eliminuje spam wielokrotnych
   reconnectów w ciągu jednego cyklu wywołań FB.
   fbLastSuccessMs ustawiane po każdym HTTP 2xx w 4 funkcjach Firebase.

 [P-1b] TLS session resumption — POMINIĘTE (ESP32 core 3.x):
   WiFiClientSecure::session_t / setSession() dostępne tylko w core 2.x.
   W core 3.x (NetworkClientSecure) tego API nie ma — zmiana wycofana.

 [P-2a] last_reset.txt — zapis przyczyny restartu do LittleFS:
   W setup(), po wykryciu esp_reset_reason(), zapisuje reason + uptime
   do /last_reset.txt. Plik dostępny przez panel WWW niezależnie od stanu
   WebSerial. Nigdy nie traci się przyczyny restartu.

 [P-2b] loopStack_free w heartbeat HEAP:
   uxTaskGetStackHighWaterMark(NULL) × sizeof(StackType_t) dodane do
   linii 💓 [HEAP]. Pozwala wykryć stack overflow na Core 1 zawczasu.

 [P-2c] Próg WARN DRAM: 20 kB → 50 kB:
   diagHeap(): freeH < 50000 zamiast 20480, interwał 60s zamiast 300s.
   TLS handshake wymaga ~47 kB — ostrzeżenie musi poprzedzać alokację.

 [P-3a] TSL retry: 10 min → 30 min po wyczerpaniu 5 prób:
   tslRetryInterval: 600000 → 1800000. Każda próba blokuje tgTask ~16s
   (2 × begin() po ~8s). Rzadszy retry = mniejszy WDT margines zużyty.

***********************************************v62  (2026-05-15) - MEM-OPT: monitoring DRAM (1A/1B/1C) + html→PSRAM + tgStack 16→12 kB + wsQueue String→char[].

 ZMIANY:

 [1A] g_heapMinValue / g_heapMinTimestamp — śledzi kiedy (uptime [s]) DRAM osiągnął
      minimum. Logowane w heartbeat co 60s: min=NNNkB(at Xs).

 [1B] Delta DRAM przed/po html.reserve w /control — log [HEAP-WWW] pokazuje ile
      DRAM pochłania każde otwarcie panelu.

 [1C] psramUsed= w heartbeat — widać faktyczne zużycie PSRAM (wcześniej tylko free).

 [2] html 16 kB → PSRAM: char* htmlBuf = psramAllocSafe(HTML_BUF_CAP) + lambda ha().
     Fallback na String.reserve(HTML_BUF_CAP) gdy PSRAM niedostępny.
     Zysk: −16 kB z DRAM przy każdym żądaniu /control.
     min powinien wzrosnąć z ~34 kB do ~50+ kB.

 [3] tgTask stack: 16 384 → 12 288 B (+4 kB ciągłego DRAM).
     Watermark z logów: ~4 888 B użyte → 12 288 daje margines ~7 kB (58%).
     Obserwować tgStack_free= w heartbeat — jeśli >3 000 B przez tydzień,
     można zmniejszyć do 10 240 w v63.

 [4] _wsQueue[32]: String[] → char[32][WS_MSG_MAX_LEN=160].
     Eliminuje fragmentację heapa od dynamicznych alokacji String.
     Tablica 32×160=5 120 B w BSS (nie heap) — zamiana heap-frag na stałą BSS.

***********************************************v61  (2026-05-15) - FIX-HIST-SAVE: saveHistoryPoint() dodane do bloku histSavePending + reset flagi.

 ROOT CAUSE (Czas LED / Energia zawsze = 0 od kilku dni):
   W bloku obsługi histSavePending w tgTaskFn brakowało dwóch rzeczy:
   1. Wywołania saveHistoryPoint() — to właśnie ta funkcja akumuluje
      ledOnMinutesToday += 5, energyTodayWh += pointWh, peakPowerWToday.
   2. Resetowania flagi histSavePending = false — przez co saveAdaptStats()
      i saveEnergyStats() wywoływały się przy KAŻDEJ iteracji tgTask
      zamiast co 5 minut.

 NAPRAWA:
   if (histSavePending) {
     histSavePending = false;   // ← DODANE
     saveHistoryPoint();         // ← DODANE
     saveAdaptStats();
     saveEnergyStats();
   }

 EFEKT:
   Czas LED, energia dziś/tydzień/miesiąc, szczytowa moc — naliczają się
   poprawnie co 5 minut od momentu wgrania v61.

***********************************************v60  (2026-05-15) - FB-REUSE: http.setReuse(true) we wszystkich 4 funkcjach Firebase.

 ROOT CAUSE (v58 nie działało — FB-CONN reconnect co 1–7s zamiast co 55s):
   HTTPClient::end() wywołuje wewnętrznie disconnect() → fbClient.stop(),
   chyba że ustawiono setReuse(true) + serwer odpowiedział Connection:keep-alive.
   W v58 brak setReuse(true) → po każdym żądaniu fbClient był zamykany,
   a następna funkcja widziała conn=0 → pełny TLS re-handshake (~300–800ms).
   Globalny fbClient istniał, ale był resetowany przy każdym http.end().
   Log pokazywał: [FB-CONN] reconnect (ready=1, conn=0, idle=2743ms) —
   idle < 55s, ale conn=0 wystarczyło by wywołać reconnect.

 NAPRAWA: http.setReuse(true) przed http.begin(fbClient, ...) w 4 miejscach:
   sendStatusToFirebase()   — PATCH /status.json
   checkFirebaseCommands()  — GET /cmd.json
   checkFirebaseCommands()  — DELETE /cmd.json (hDel)
   checkFirebaseConfig()    — GET /config.json
   Firebase Realtime Database REST odpowiada Connection:keep-alive na HTTP/1.1,
   więc setReuse(true) + keep-alive serwera = brak stop() w end().

 OCZEKIWANY EFEKT:
   FB-CONN reconnect: co 1–7s → 0 (tylko po idle 55s lub przy zerwaniu)
   TLS handshake/min: 8–12 → 0–1 (zgodnie z zamierzeniem v58)
   DRAM spike przy FB call: ~0 kB (tak jak planowano w v58)

***********************************************v59  (2026-05-15) - PSRAM-FINAL: eliminacja ostatnich dwóch buforów DRAM → PSRAM.

 ZMIANA 1 – json w sendStatusToFirebase() (2800 B DRAM → PSRAM):
   String json; json.reserve(2800) wymagało ciągłego bloku 2800 B w DRAM
   przy każdym wywołaniu (co 15s). Zastąpione char* jb = psramAllocSafe(3200)
   z lambdą ja() do appenda. Fallback na String.reserve(2800) gdy brak PSRAM.
   Dodatkowo eliminuje tymczasowe Stringi hw (~220B) i ps (~128B) —
   budowane teraz bezpośrednio przez ja() do jb.

 ZMIANA 2 – resp w sendTelegramDocument() (2048 B DRAM → PSRAM):
   String resp; resp.reserve(2048) zastąpione char* respBuf = psramAllocSafe(2049).
   Sliding window (memmove po przepełnieniu) zachowany. Fallback na String.reserve(512).

 OCZEKIWANY EFEKT (obie zmiany razem):
   DRAM transient podczas FB call:   −2800 B (jb w PSRAM)
   DRAM transient podczas TG doc:    −2048 B (respBuf w PSRAM)
   DRAM free (normalnie):             +~5 kB ciągłych bloków

***********************************************v58  (2026-05-15) - FB-PERSISTENT-CLIENT: eliminacja lokalnych WiFiClientSecure w Firebase + dokończenie ZADANIA C z v55.

 ROOT CAUSE (spike DRAM ~47 kB co ~5 minut widoczny w HEAP-STAT):
   sendStatusToFirebase(), checkFirebaseCommands() i checkFirebaseConfig()
   tworzyły lokalny WiFiClientSecure na każde wywołanie (4 instancje na cykl):
     fbClientSend, fbClientCmd, fbClientDel, fbClientCfg
   Każdy wywołuje pełny TLS handshake od zera → ~47 kB DRAM transientnie:
     I/O bufory mbedTLS in+out: 2 × 16 kB = 32 kB
     ssl_handshake_params:           ~9 kB
     ssl_context + crypto state:     ~6 kB
   Firebase wywoływane co 7–30s → kilka handshake'ów/minutę na tym samym hoście.
   Heartbeat HEAP logował anomalie szczególnie co 5 min gdy trzy funkcje
   wykonywały się back-to-back po zapisie historii LittleFS.

 NAPRAWA: globalny fbClient (wzorzec identyczny z tgClient z v32):
   WiFiClientSecure fbClient — jeden kontekst SSL na cały czas działania.
   fbConnect() — reconnect tylko gdy idle >55s lub połączenie zerwane.
   Wszystkie cztery lokalne WiFiClientSecure zastąpione wywołaniem fbConnect().

 DODATKOWO: dokończenie ZADANIE C z v55 (nieukończone w v57):
   String body; body.reserve(2048) → psramAllocSafe(2048) z fallbackiem DRAM.
   Dotyczy checkFirebaseCommands() i checkFirebaseConfig().

 OCZEKIWANY EFEKT:
   Spike DRAM ~47 kB przy Firebase call → 0 kB (handshake tylko przy reconnect).
   Margines wolnej DRAM podczas FB call: ~21 kB → ~68 kB.
   Połączenia TLS Firebase: 8–12/min → 0–1/min (tylko po idle 55s).

***********************************************v57  (2026-05-15) - FIX-v57-RESET-LOOP: trzy przyczyny pętli resetów wyeliminowane.

 ROOT CAUSE A (WDT_TASK crash uptime≈241s przy braku WiFi — pętla resetów #2-#4):
   Po restarcie ESP z niedostępnym WiFi tgTask wykonywał dwie próby TLS pod rząd:
   1. tgBootNotifyPending → sendTelegramMessage() → tgConnect() → connect()=FAIL 8s
   2. natychmiastowy TG-POLL → pollTelegramCommands() → tgConnect() → connect()=FAIL 8s
   Łącznie ~16s blokady tgTask bez resetu WDT między operacjami > WDT 10s → crash.
   NAPRAWA: po wyjściu z sendTelegramMessage (bootNotify) dodano esp_task_wdt_reset()
   i guard: jeśli tgBootNotifyPending nadal aktywne (wysyłka się nie powiodła),
   TG-POLL jest pomijany w tej iteracji (continue). Eliminuje nakładanie dwóch
   bloków TLS → maksymalny czas blokady = 1 × 8s (poniżej WDT 10s).

 ROOT CAUSE B (WDT_TASK crash podczas reinicjalizacji TSL — crashe #5-#7):
   Blok tslReinitPending w tgTask miał esp_task_wdt_reset() tylko przed
   czujnikPokojowy.begin() (może blokować ~8s przy braku czujnika).
   czujnikNadWoda.begin() wywoływane bezpośrednio po nim BEZ resetu WDT.
   Dwa begin() × ~8s = ~16s > WDT 10s → crash gdy oba czujniki nie odpowiadają.
   NAPRAWA: dodano esp_task_wdt_reset() bezpośrednio przed czujnikNadWoda.begin().

 ROOT CAUSE C (crash podczas TG-POLL przy fragmenacji HEAP — crashe #1, #8, #9):
   Po połączeniu WiFi tgTask natychmiast wykonywał bootNotify TLS + TG-POLL TLS.
   Szybkie następstwo dwóch TLS handshake'ów powodowało chwilowy spike alokacji
   (maxAlloc 135→55 kB) i potencjalny mbedTLS assert przy fragmentacji HEAP.
   Ten sam guard co w FIX-A (skip TG-POLL gdy bootNotify pending) tworzy przerwę
   między operacjami TLS i daje czas na zwolnienie bufora przez GC.

***********************************************v56  (2026-05-15) - FIX-v56-WDT-RECONNECT: hard-reconnect nie blokuje loop() — prawdziwa naprawa WDT, nie maskowanie.

 ROOT CAUSE (WDT_TASK crash uptime≈241s, powtarzający się co ~4 min przy braku WiFi):
   WiFi.disconnect(true) wywołuje esp_wifi_stop() — pełne zatrzymanie stosu WiFi.
   Blokuje loop() (Core 1, monitorowany przez Task WDT) na ~5–9s.
   WiFi.begin() po zatrzymanym stosie startuje go od zera i dokłada kolejne ~2–4s.
   Łącznie >10s bez esp_task_wdt_reset() → WDT_TASK crash.
   Dodanie samego esp_task_wdt_reset() byłoby maskowaniem — watchdog wykrywa
   zablokowaną pętlę i właśnie tak działał poprawnie; omijanie go ukrywa problem.

 NAPRAWA: WiFi.disconnect(false) zamiast disconnect(true).
   false = wifioff=false → tylko esp_wifi_disconnect() (rozłącza od AP, NIE
   zatrzymuje stosu WiFi). Wraca w <1ms. WiFi.begin() z działającym stosem
   też wraca w <1ms. Blokowanie loop(): <5ms zamiast ~9s.
   Cel FIX-v39-5 zachowany: begin() resetuje stan ESP_ERR_WIFI_STATE
   (IDF ignorujący esp_wifi_connect po długim braku sieci) — stós nie jest
   restartowany, ale połączenie jest inicjowane od nowa z nowymi parametrami.
   esp_task_wdt_reset() wokół wywołań = defense-in-depth, nie główny fix.

***********************************************v55  (2026-05-14) - RESP-BUF-PSRAM: bufory odpowiedzi HTTP przeniesione z DRAM do PSRAM (CZĘŚĆ 3/3).

 ZADANIE A - tgPost() bufor odpowiedzi TLS (8 kB → PSRAM):
   RESP_CAP 4096 B (DRAM, ps_malloc+malloc fallback) → 8192 B (PSRAM, psramAllocSafe).
   Blok odczytu body zmieniony z 256-bajtowego readBytes() na char-po-char
   ze sliding window (memmove, zachowuje ostatnie 4 kB przy przepełnieniu).
   Timeout wewnętrznej pętli odczytu: 3000 ms → 9000 ms.
   Bufor PSRAM zwalniany natychmiast po String result(_respBuf).

 ZADANIE B - sendTelegramDocument() bufor strumieniowania (512 B → PSRAM):
   uint8_t buf[512] (stos) → psramAllocSafe(512) z fallbackiem alloca(512).
   Flaga bufOnStack zapobiega free() na wskaźniku stosu.
   free(buf) wywołany po zakończeniu pętli f.available().

 ZADANIE C - checkFirebaseCommands() + checkFirebaseConfig() (2×2 kB → PSRAM):
   String body; body.reserve(2048) → fbBuf = psramAllocSafe(2048).
   http.getString() kopiowane do fbBuf, oryginalna String zwalniana natychmiast.
   String body rekonstruowana z fbBuf (rozmiar = rzeczywista odpowiedź, bez
   nadmiarowej rezerwy 2048 B w DRAM). fbBuf zwalniany zaraz po rekonstrukcji.
   Efekt: eliminacja reserve(2048) = brak stałego bloku 2048 B w DRAM.

 OCZEKIWANY EFEKT (wszystkie 3 części razem):
   DRAM free:     ~149 kB → ~165-170 kB (+16-21 kB)
   maxAlloc:      ~135 kB → ~155 kB
   PSRAM używa:   ~20-25 kB (z dostępnych 8192 kB)
   Po restarcie bez WiFi: maxAlloc ~150 kB (brak dużych buforów w DRAM).

***********************************************v54  (2026-05-14) - LOG-BUF-PSRAM: bufory logów (2×8 kB) przeniesione z DRAM do PSRAM.

 ZADANIE 2 - logBuf[2] w PSRAM (CZĘŚĆ 2/3):
   String g_logBuffer[2] (DRAM) → char* g_logBuf[2] allokowane z psramAllocSafe().
   g_logBufLen[2] śledzi długość każdego bufora. logToFile() używa lambda logAppend().
   logFlushCore1() używa f.write() zamiast f.print(String).
   EFEKT: DRAM free +16 kB; PSRAM zużycie +16 kB.

***********************************************v53  (2026-05-14) - PSRAM-DIAG: psramAllocSafe() + diagnostyka PSRAM + psram= w logach HEAP i TG-ALIVE.

 ZADANIE 1 - psramAllocSafe():
   Globalny helper przed setup(). Próbuje ps_malloc(), fallback na
   heap_caps_malloc(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT).
   ZAKAZ: stosy tasków FreeRTOS, ISR, DMA, OneWire, portMUX/Semaphore.
   MOŻNA: duże bufory logów >4kB, HTTP/HTTPS, TLS, JSON Firebase/TG, WebSocket.

 ZADANIE 2 - diagnostyka PSRAM w setup():
   Po inicjalizacji LittleFS i WiFi, przed webserialServer.begin().
   psramFound() -> loguje size= (esp_psram_get_size/1024) i free= (MALLOC_CAP_SPIRAM/1024).
   Brak PSRAM -> WARN + info o DRAM fallback.

 ZADANIE 3 - rozszerzony log [HEAP]:
   Heartbeat co 60s: free= -> int= (heap_caps_get_free_size MALLOC_CAP_INTERNAL)
   + nowe pole psram= (heap_caps_get_free_size MALLOC_CAP_SPIRAM).
   min=, maxAlloc=, largestBlk=, uptime=, tgStack_free= bez zmian.

 ZADANIE 4 - rozszerzony log [TG-ALIVE]:
   Dodano psram=%ukB (heap_caps_get_free_size MALLOC_CAP_SPIRAM / 1024)
   między freeH= a maxAlloc=. Reszta pól bez zmian.

***********************************************v51  (2026-05-14) - FIX-v51-MINLUX-FALLBACK: calculateMinLuxPWM() zapisuje fallbackPwm do minLuxCurrentPWM[]; allFinished rampy poludnie uzywa fallback gdy mlPwm==0.
***********************************************v52  (2026-05-14) - FIX-v52-WDT-BOOT + FIX-v52-WWW-RECONNECT: WiFi boot loop bezpieczny dla WDT; AsyncWebServer rebind po reconnect.

 BUG #1 - WDT crash przy braku WiFi podczas boot:
   retry<20 * delay(500ms) = 10s = dokladnie granica WDT. esp_task_wdt_reset() byl
   PRZED delay() (zbedny), ale sam limit 20 prob byl graniczny.
   NAPRAWA: retry<15 (7.5s, margines 2.5s) + esp_task_wdt_reset() PRZED delay().
   Dodano uptime guard (nowMs>180000) na hard-reconnect w loop() - nie uruchamiaj
   disconnect()+begin() zaraz po bocie gdy autoReconnect jest jeszcze aktywny.

 BUG #2 - AsyncWebServer martwy po reconnect WiFi:
   server.begin() w setup() bindownal gniazdko do interfejsu bez IP.
   Po przywroceniu WiFi ESPAsyncWebServer NIE rebinnduje automatycznie.
   NAPRAWA: webserialServer.begin() wywolywany ponownie w bloku WiFi monitor
   przy kazdym odzyskaniu polaczenia (gdy _wifiLostMs != 0 -> WiFi wrocilo).
   begin() w ESPAsyncWebServer jest idempotentny - nie tworzy drugiego listenera.

 ROOT CAUSE (log "Rampa poludnie cel MIN LUX 0 PWM"):
   Czujnik TSL2561 odpada chwilowo na I2C dokladnie gdy rampa poludniowa konczy sie i
   calculateMinLuxPWM() jest wywolana po raz pierwszy (allFinished block). W tym momencie:
   czujnikPokojowyAktywny=false, minLuxModeActive=false, minLuxCurrentPWM[0]=0.
   Fallback 1 nie zachodzi bo minLuxCurrentPWM[0]==0. Fallback 2 oblicza poprawny fallbackPwm
   ale NIE zapisuje go do minLuxCurrentPWM[]. allFinished block widzi mlPwm==0 -> MIN LUX OFF.

 NAPRAWA 1 - calculateMinLuxPWM() Fallback 2:
   Po obliczeniu fallbackPwm, PRZED return, zapisz do minLuxCurrentPWM[0..4].
   Fallback 1 znajdzie historyczna wartosc i utrzyma PWM przy nastepnym wywolaniu.

 NAPRAWA 2 - allFinished blok rampy poludnie:
   Gdy calculateMinLuxPWM() zwroci 0 ale minLuxCurrentPWM[0]>0 (fallback z NAPRAWY 1),
   uzyj minLuxCurrentPWM[0] jako mlPwm -> aktywuj MIN LUX natychmiast po zakonczeniu rampy.

 Nie koliduje z: FIX-v16-SENSOR-FALLBACK, FIX-v13, FIX-v19-ALLFINISHED-SPAM.
***********************************************v50  (2026-05-14) - FIX-WDT-TCP: blokujący connect() naprawiony, DFS wyłączone, DS18B20 retry.

 ROOT CAUSE (WDT crash o 01:16 i 02:11 z logów - uptime ~22880s i ~3251s):
   Dwa niezależne problemy powodujące WDT_TASK:

 PROBLEM 1 - tgClient.connect() blokuje na poziomie TCP (przed TLS):
   tgClient.connect("api.telegram.org", 443) po stronie WiFiClientSecure to wywołanie
   synchroniczne - najpierw TCP SYN (lwIP/IDF), potem TLS handshake (mbedTLS).
   setTimeout(8) ustawia TYLKO timeout odczytu danych (mbedTLS read), NIE TCP connect.
   Jeśli serwer jest nieosiągalny lub sieć się zacina na etapie TCP SYN,
   TCP connection attempt może trwać 20-60s bez żadnego WDT reset.
   Pętla while(!connected) sprawdza tgClient.connected() DOPIERO PO powrocie z connect().
   Efekt: connect() wisi >10s bez WDT reset -> WDT_TASK crash.
   NAPRAWA: tgClient.setConnectionTimeout(4000) - ustawia TCP-level connection timeout
   (WiFiClientSecure->WiFiClient->esp_tls_conn_new_sync -> CONFIG_ESP_TLS_CONNECT_TIMEOUT).
   Dodatkowo: esp_task_wdt_reset() w pętli while za pomocą poll'ingu co 50ms,
   hard timeout 8s (< WDT 10s) zamiast 9s.

 PROBLEM 2 - DFS (Dynamic Frequency Scaling) 240->80 MHz + WiFi:
   esp_pm_configure z min_freq_mhz=80 powoduje że podczas oczekiwania na sieć
   CPU opada do 80 MHz. Przy przeciążonej WiFi stack timer ISR może nie zdążyć
   wywołać lwIP timers w WDT oknie (zwłaszcza przy TLS handshake z mbedTLS AES).
   Restart o 01:16 (TG-ALIVE widoczny o 01:16:18, restart o 01:16:47 = 29s bez logu)
   pokrywa się z sytuacją gdy WiFi reconnect + mbedTLS handshake + 80MHz = deadlock.
   NAPRAWA: min_freq_mhz = 240 (wyłączone DFS). Oszczędność energii jest marginalna
   przy stałym połączeniu WiFi i tak.

 PROBLEM 3 - DS18B20 błędy -127°C:
   19 błędów odczytu w ciągu nocy, system zachowuje stara wartość ale nie informuje.
   NAPRAWA: diagDS18B20() liczy błędy i loguje WARNING co 5 błędów (nie co odczyt).
   Bez zmian w hardware (wymaga rezystora pull-up 4.7kΩ).

 PROBLEM 4 - wersja firmware w logPrintln była "v48" zamiast aktualnej:
   Naprawiono: logPrintln aktualizuje wersję przy każdej zmianie.

***********************************************v49  (2026-05-13) - FIX-WDT-CONNECT: tgEnsureConnected() i body-read bez długich bloków + sendTelegramDocument() WDT-safe connect.

 ROOT CAUSE (WDT crash przy getUpdates timeout - log 18:17:25):
   tgClient.connect("api.telegram.org", 443) to blokujące wywołanie TLS handshake.
   Przy wolnej/zatkanej sieci może trwać >10s. Jedyny esp_task_wdt_reset() był
   wywołany PRZED connect() - jeśli connect() blokował powyżej 10s, WDT wygrywał.
   Drugi problem: pętla odczytu body w tgPost() przy tgClient.available()==0
   wywoływała vTaskDelay(5)+esp_task_wdt_reset() w pętli - to poprawne. Ale gdy
   connect() wisiał np. 12s, do pętli body nigdy nie docierało.

 NAPRAWA 1 - tgEnsureConnected(): connect() w pętli z WDT reset:
   Zamiast jednego blokującego connect(), używamy nieblokującego wzorca:
   - Wywołanie tgClient.connect() z timeout klienta 8s (zamiast 10s).
   - Pętla while(!tgClient.connected()) z esp_task_wdt_reset() co 100ms.
   - Hard timeout pętli 9s (poniżej WDT 10s) -> bezpieczny fallback FAIL.
   Wynik: WDT reset co <100ms podczas całego handshake -> crash niemożliwy.

 NAPRAWA 2 - tgPost() body-read: dodano esp_task_wdt_reset() gdy avail==0:
   Wcześniej: tylko na końcu iteracji (po vTaskDelay). Gdy avail>0 przez wiele
   bloków z rzędu (duże body), WDT nie był resetowany między readBytes().
   Dodano esp_task_wdt_reset() bezpośrednio w gałęzi avail>0 po każdym readBytes().

***********************************************v47  (2026-05-13) - FIX-WDT-WRITE: sendTelegramDocument write 64B chunk zamiast 512B.

 ROOT CAUSE (WDT crash przy wysyłaniu pliku przez TLS):
   tgClient.write(buf, 512) może zablokować >10s gdy bufor TCP pełny.
   Jeden esp_task_wdt_reset() przed/po nie wystarcza gdy pojedynczy write
   blokuje całą kwantę czasową. WDT timeout = 10s.

 NAPRAWA:
   Wewnętrzna pętla write po 64 bajty zamiast 512.
   esp_task_wdt_reset() przed każdym chunk -> maks blok = czas write 64B (~1ms).
   WDT crash niemożliwy niezależnie od prędkości TLS.

***********************************************v46  (2026-05-13) - LOG-FLUSH-CORE0: flush logów z Core 1 przeniesiony na Core 0.

 ROOT CAUSE (LOOP-SLOW 502ms):
   logToFile() wywoływana z loop() (Core 1) co 5s lub przy pełnym buforze (8KB)
   wykonywała LittleFS.open()+f.print(8KB)+f.close() na Core 1 -> 200-500ms blok.
   Potwierdzono: maxAlloc=27636B zaraz po LOOP-SLOW 502ms (heap sfragmentowany).

 NAPRAWA (wzorzec fsGuardPending/histSavePending):
   - volatile bool logFlushPending[2] - flagi per-rdzeń.
   - Core 1: shouldFlush -> logFlushPending[1]=true, natychmiastowy return.
   - tgTask Core 0: obsługuje logFlushPending[1] - zapis LittleFS poza loop().
   - Core 0: flushuje swój bufor [0] bezpośrednio (tgTask nie jest time-critical).
   Wynik: loop() nigdy nie czeka na LittleFS - zero LOOP-SLOW od logowania.

***********************************************v45  (2026-05-13) - FIX-WDT: tgPost char-by-char + sendDoc size-limit:

 ROOT CAUSE (WDT crash przy TG_MENU po uptime ~11 min):
   tgPost() czytał nagłówki HTTP przez readStringUntil('\n').
   tgClient.setTimeout(10) ustawia timeout na 10 SEKUND (nie ms).
   Gdy linia nagłówka przychodziła wolno, readStringUntil blokował
   do 10s bez wywołania esp_task_wdt_reset(). WDT timeout = 10s.
   Wynik: wyścig - przy obciążonej sieci WDT wygrywał.

 1. tgPost() - nagłówki char-by-char zamiast readStringUntil:
    Pętla czyta jeden bajt (tgClient.read()), natychmiast woła
    esp_task_wdt_reset(). Maksymalny blok = czas odczytu 1 bajtu (~1ms).
    Brak możliwości WDT crash niezależnie od prędkości sieci.

 2. sendTelegramDocument() - limit MAX_TG_DOC = 512 KB:
    Przy pliku 10 MB wysyłka przez TLS (~500 KB/s) trwa ~20s -> WDT crash.
    Naprawa: wysyłamy tylko ostatnie 512 KB pliku (f.seek(fileSize-512KB)).
    Caption informuje użytkownika: "log.txt (10240 KB, wysłano ostatnie 512 KB)".
    Dodano esp_task_wdt_reset() PRZED tgClient.write() - write może blokować
    w stercie TLS gdy bufor TCP pełny; reset przed write daje pełne 10s luzu.

 3. Aktualizacja stałej wersji z v42b -> v45 (sync z CHANGELOG).

***********************************************v44  (2026-05-13) - OPT: LOG-ALLOC + EEPROM-DEFER + FREERTOS-DELAY + TSL-MUTEX:

 1. logPrintf - jedna alokacja String zamiast dwóch:
    _wsEnqueue(String(buffer)) + logToFile(String(buffer)) -> jedna zmienna _s.
    ~377 wywołań logPrintf -> eliminuje ~377 zbędnych alokacji/zwolnień na wywołanie.

 2. logPrint/logPrintln - overloady const char*:
    ~200 wywołań logPrint("literał") cicho tworzyło tymczasowy String.
    Nowe overloady wywołują Serial.print(msg) bezpośrednio, bez String.
    Kompilator dobiera overload automatycznie - brak zmian w miejscach wywołań.

 3. EEPROM - odroczony commit przez flagę g_UNUSED_eepromCommitPending_DO_NOT_USE:
    13 funkcji save*() zastąpiło EEPROM.commit() flagą.
    commit() (~10ms Flash write) wykonywany raz w loop-slow zamiast przy każdej
    zmianie nastawy. Wzorzec spójny z istniejącym zapiszAdaptacjeDoEEPROM().
    UWAGA: loadMinLuxModeFromEEPROM() (boot) i loop-slow handler - bez zmian.

 4. tgPost - vTaskDelay zamiast delay w pętlach oczekiwania:
    delay(5) w tgTaskFn (Core 0) to busy-wait blokujący scheduler FreeRTOS.
    vTaskDelay(pdMS_TO_TICKS(5)) oddaje czas innym taskom podczas oczekiwania.

 5. tslCache - portMUX_TYPE dla spójności multi-field:
    Na ESP32 pojedynczy float/uint16 jest atomowy (aligned 32-bit),
    ale odczyt zestawu (CachePok + BrPok + IrPok) może trafić na częściowy zapis.
    portENTER/EXIT_CRITICAL owijają zapis (Core 0) i odczyt (Core 1) zestawów pól.
    Sekcja krytyczna jest ultraszybka (nanosekund) - brak wpływu na timing.

***********************************************v43  (2026-05-12) - PSRAM + LOG-RING-BUFFER + FS-GUARD-85:
        Optymalizacje pod N16R8 (8MB PSRAM, 16MB Flash).

 1. PSRAM dla dużych buforów:
    - logBuffer[2]: reserve(8192) - UWAGA: String.reserve() zawsze
      alokuje z DRAM (nie PSRAM) bez CONFIG_SPIRAM_USE_MALLOC=y.
      nie zajmuje wewnętrznego heap; flush co 5s zamiast 2s.
    - tgPost resp: NIEUKOŃCZONE - nadal String.reserve(4096) na DRAM
      (ps_malloc(8192) z CHANGELOG nie wdrożono; wymaga refaktoru).
      w wewnętrznym heap.
    - rotacja FS (buf[512]): alokacja z PSRAM przez ps_malloc.

 2. stos tgTask z PSRAM:
    xTaskCreatePinnedToCore -> ps_malloc(16384) jako stos tasku.
    Wewnętrzny heap zyskuje ~12KB ciągłego bloku.
    Użyto StaticTask_t + xTaskCreateStaticPinnedToCore.

 3. FS_LIMIT 72% -> 85% (8854190 B):
    Przy 9.9MB partycji zostawia ~1.5MB wolnego na plik tymczasowy
    rotacji (max KEEP_LOG=2MB). Rotacja uruchamia się rzadziej.
    Inne pliki (config, adapt, energy, pompa) chronione progiem
    10% całości (~1MB) - poniżej tego rozmiaru nie są przycinane.

 4. FLUSH_INTERVAL_MS: 2000 -> 5000 ms.
    Mniej write-cycles na Flash, bufor większy (PSRAM), loop czystszy.

***********************************************v42  (2026-05-01) - FIX HEAP-FRAG-ARCH: Firebase na Core 0 (flagi pending),
        http.getString() -> reserve+stream, String hw/ps reserve(),
        extractInt32 bez String needle.

 v42b (2026-05-01) - FIX-HEAP-GUARD: logFlush() guard przed LittleFS.open()
        przy sfragmentowanym HEAP (largestBlk < 2KB -> pomiń flush;
        largestBlk < 4KB -> pomiń rotację CIRCULAR LOG).
        Bezpośrednia ochrona przed: assert failed lfs_file_close lfs.c:6080
        który wcześniej crashował system przy TSL reinit + Firebase na Core 1.
        W v42 mało prawdopodobny (brak wyścigów HEAP), ale guard chroni przyszłość.

 ROOT CAUSE (nowa analiza po v41):
   Trzy niezależne źródła fragmentacji HEAP pominięte przez v41:

 1. Firebase HTTP na Core 1 (loop()) jednocześnie z TLS na Core 0 (tgTask).
    sendStatusToFirebase() co 15s + checkFirebaseCommands() co 7s +
    checkFirebaseConfig() co 30s - każde tworzy lokalny HTTPClient (~8-16KB
    alokacji) równocześnie gdy Core 0 obsługuje SSL Telegram. Obydwa rdzenie
    wchodzą na alokator HEAP jednocześnie -> dziury z obu stron.
    NAPRAWA: volatile bool fbSendPending/fbCmdPending/fbCfgPending.
    loop() tylko ustawia flagi przez timer. tgTaskFn (Core 0) wykonuje HTTP.
    Efekt: JEDEN rdzeń obsługuje całą sieć - zero równoczesnych alokacji.

 2. http.getString() bez reserve w checkFirebaseCommands() i checkFirebaseConfig().
    Firebase zwraca JSON ~1-2KB -> alokator: 256->512->1024->2048B = 3 realokacje
    i zwolnienia = 3 dziury. Dzieje się co 7s i co 30s.
    NAPRAWA: body.reserve(2048) przed http.getString().

 3. String hw i String ps bez reserve w sendStatusToFirebase().
    hourPowerWh: 24 iteracje += bez reserve -> 4 realokacje co 15s.
    pumpSlots: konkatenacje "{\"start\":...}" -> tymczasowe Stringi -> dziury.
    NAPRAWA: hw.reserve(220) i ps.reserve(128) przed pętlami.

 4. extractInt32 lambda tworzyła String needle co wywołanie (~12x per config).
    NAPRAWA: body.indexOf() bezpośrednio na const char* zamiast String.

 EFEKT: eliminacja najczęstszych źródeł fragmentacji po v41.
        Razem z v41 (cleanupClients, flushWsQueue, autoRestart) system
        powinien działać stabilnie bez ograniczania funkcjonalności.

***********************************************v39  (2026-04-21) - FIX CIRCULAR LOG: stała keepBytes zamiast fsFree-MARGIN
v39-3 (2026-04-22) - FIX fragmentacja HEAP: tgPost nagłówki bez akumulacji,
        pollTelegramCommands bez chunk-substring, TSL-WODA podwójny timestamp,
        dynamiczny alpha EMA przy skokach gain TSL2561
v39-4 (2026-04-22) - FIX TSL-WODA spam w nocy (0 lux OK gdy isNightGlobal),
        FIX fsSizeGuard przeniesiony na Core 0 (koniec LOOP-SLOW 4s przy rotacji FS)
v39-5 (2026-04-23) - FIX WiFi hard-reconnect: blok "WiFi status log co 60s" nie wywoływał
        żadnej funkcji reconnect (tylko logował). setAutoReconnect(true) nie odbudowuje
        połączenia po długim braku sieci (IDF wchodzi w stan ESP_ERR_WIFI_STATE).
        NAPRAWA: hard-reconnect (disconnect+begin) po 3 min braku WiFi w loop() +
        eskalacja w DIAG-14 po 10 min + usunięto martwy komentarz "ConnectionHandler
        sam reconnektuje" (ConnectionHandler nie istnieje od usunięcia ArduinoCloud).

 ROOT CAUSE (potwierdzony przez log.txt = 57.7 KB po przejściu z 66% -> 13%):
   logFlush() zawiera osobny mechanizm rotacji logu ("CIRCULAR LOG"),
   niezależny od fsSizeGuard(). Wyzwala się gdy fsFree < 20% dysku (>80% zajęte).
   Formuła obliczania ile zachować:
     keepBytes = fsFree - MARGIN  (MARGIN = 48 KB)
   Przy 87% zajętości:
     fsFree = ~106 KB -> keepBytes = 106-48 = 58 KB -> log.txt ucięty do ~58 KB
   Formuła jest błędna: im bardziej pełny dysk, tym mniej zachowuje log.
   Wynik: log.txt 57.7 KB (zamiast oczekiwanych ~256 KB).

   fsSizeGuard() (naprawiony w v38 do 68% progu) nie zdążył zadziałać -
   odpala co 60s, a dysk może przeskoczyć z 65% do 85% w ciągu jednej minuty
   przy intensywnym logowaniu.

 NAPRAWA - stała docelowa keepBytes = 256 KB (spójnie z fsSizeGuard):
   Jeśli fsFree > 256 KB + 48 KB (= 304 KB): keepBytes = 256 KB (stałe)
   Jeśli fsFree <= 304 KB: keepBytes = fsFree - MARGIN (fallback, brak miejsca)
   Jeśli fsFree <= MARGIN: log.txt usunięty całkowicie (krytyczne przepełnienie)
   Usunięto zbędny cap maxKeep (55% × total = 450 KB > KEEP_FIXED = 256 KB, zawsze nieaktywny).

 EFEKT: log.txt zachowuje ostatnie 256 KB po rotacji niezależnie od stopnia
   zajętości dysku (o ile jest wystarczająco miejsca na plik tymczasowy).

 v38  (2026-04-21) - FIX FS-GUARD: bezpieczne przycinanie przy pełnym dysku

 ROOT CAUSE (potwierdzony na podstawie analizy kodu + zrzutów ekranu):
   fsSizeGuard() ustawiała FS_LIMIT = 819200 B = 100% pojemności dysku.
   Gdy guard się wyzwalał, dysk był już absolutnie pełny.
   Próba otwarcia /log_tmp.txt do zapisu ("w") mogła się powieść (0 B),
   ale kolejne wywołania fDst.write() cicho failowały - brak miejsca,
   brak sprawdzania zwracanej wartości write().
   Mimo pustego/niepełnego pliku tymczasowego kod kontynuował:
     LittleFS.remove(LOG_FILE)       -> skasowanie log.txt (652 KB)
     LittleFS.rename("/log_tmp.txt", LOG_FILE) -> log.txt = plik pusty/częściowy
   Efekt: utrata całej zawartości logu zamiast zachowania ostatnich 256 KB.
   Plik log_tmp... 0 B widoczny w UI = ślad poprzedniej nieudanej próby.

 NAPRAWA 1 - obniżenie progu wyzwalania (FS_LIMIT):
   819200 -> 557056 B (68% dysku).
   Przy progu 557 KB wolne jest dokładnie 262 KB = tyle ile potrzeba
   na zapis /log_tmp.txt (KEEP_LOG = 256 KB + margines).
   Guard uruchamia się zanim dysk jest pełny - zapis pliku tymczasowego
   zawsze ma wystarczająco miejsca.

 NAPRAWA 2 - sprawdzanie błędów zapisu + rollback:
   Po każdym fDst.write() sprawdzany jest zwracany rozmiar.
   Przy błędzie (written != r): zamknięcie plików, usunięcie log_tmp.txt,
   log komunikatu błędu i RETURN - log.txt NIE jest kasowany.
   Analogiczne zabezpieczenie dla przycinania history.csv.

 EFEKT: guard nigdy nie skasuje logu przy niepowodzeniu zapisu.
   Przy normalnej pracy wyzwala się przy ~68% zamiast 100% dysku,
   mając zawsze miejsce na bezpieczną operację.

 v37  (2026-04-19) - FIX TRYBAUT ROOT: lastLogicRun=millis() nie nowMs
 v36b (2026-04-19) - FIX midnight reset: daily TG przez kolejkę nie direct call
 v36  (2026-04-19) - FIX TRYBAUT: guard zerowej rampy w applyMinLuxMode
 v35b (2026-04-19) - FIX boot notify: flag po udanym wysłaniu TG
 v35  (2026-04-19) - FIX TRYBAUT: guard zerowej rampy adaptacyjnej
 v34b (2026-04-18) - TG BOOT NOTIFY (reset reason na Telegram)
 v34  (2026-04-18) - TSL ODCZYT ASYNC NA CORE 0 (LOOP-SLOW fix)
 v33i (2026-04-18) - DS18B20 NON-BLOCKING (2-FAZOWY ODCZYT)

 ROOT CAUSE (potwierdzony w log_58 - LOOP-SLOW 726ms zbieżny z [TEMP]):
   readTemperatures() wywoływana co 15s bezpośrednio w loop() blokowała
   loop() na ~600-750ms na każde wywołanie. Dwie przyczyny:
   1. sensors.requestTemperatures() wykonuje 1-Wire reset per magistrala.
      Reset = 480µs LOW + czekanie na presence pulse (czujnik odciąga linię
      na ~240µs). Przy odłączonym/zawieszonym czujniku biblioteka OneWire
      czeka na presence który nie przychodzi -> timeout ~500µs per magistrala.
      3 magistrale × timeout = kilkaset ms BEZ delay().
   2. delay(60) + delay(60) = 120ms aktywnej blokady po requestach.
   Łącznie: requestx3 + delay120ms = 600-750ms blokady loop() co 15s.

 DLACZEGO POPRZEDNI FIX BYŁ POŁOWICZNY:
   setWaitForConversion(false) usunął blokadę podczas KONWERSJI w sensorze
   (750ms -> ręczny delay 120ms), ale nie usunął blokady 1-Wire reset
   podczas requestTemperatures() - która trwa niezależnie od WaitForConversion.

 DLACZEGO NIE PRZENOSIMY DO tgTask (nie maskujemy):
   1-Wire jest timingowo czuły - protokół wymaga precyzyjnych impulsów
   (<15µs dla słotów danych). Przeniesienie na inny rdzeń z preempcją
   FreeRTOS grozi korupcją komunikacji. tgTask obsługuje też TLS/SSL -
   mieszanie blocking 1-Wire z blocking TLS na jednym rdzeniu = deadlock.

 NAPRAWA - 2-fazowy odczyt w loop(), zero delay():
   Faza 1 (co 15s): requestTemperatures() x3 z setWaitForConversion(false).
     Request wraca natychmiast po wysłaniu rozkazu CONVERT_T (~1-2ms per bus).
     Konwersja 9-bit odbywa się SPRZĘTOWO w sensorze przez 94ms - ESP nie czeka.
     Ustawiamy tempConvRequested=true i tempConvRequestedAt=millis().
   Faza 2 (gdy minęło >=100ms od fazy 1): getTempCByIndex() x3 + walidacja.
     Odczyt gotowego wyniku - 1-Wire jest zajęty tylko przez czas transmisji
     (~1ms per sensor), bez żadnego czekania na konwersję.
   Obie fazy wykonywane w loop() - brak dodatkowych tasków, flag, mutexów.
   Brak delay() - loop() wraca natychmiast po każdej fazie.

 EFEKT: zero LOOP-SLOW z DS18B20 niezależnie od stanu czujników.
   Przy podłączonych czujnikach: Faza1 ~3ms, Faza2 ~3ms co 15s.
   Przy odłączonych: identycznie - timeout 1-Wire nie wydłuża request
   gdy WaitForConversion=false (biblioteka nie czeka na presence w tej ścieżce).

 v33h (2026-04-18) - TSL REINIT NA CORE 0

 ROOT CAUSE (potwierdzony w log_57 - Wire.setTimeOut też nie pomaga):
   Wire.setTimeOut() ustawia timeout clock-stretchingu (hardware I²C),
   ale gdy urządzenie fizycznie nie odpowiada (brak ACK po adresie),
   biblioteka Adafruit TSL2561 mimo wszystko blokuje ~8400ms.
   Źródło: begin() + getEvent() zawierają wewnętrzne opóźnienia/retries.
   Wire.setTimeout/setTimeOut nie są w stanie ich przerwać.

 NAPRAWA: volatile bool tslReinitPending - ten sam wzorzec co histSavePending.
   odczytajSwiatloZFiltrem() tylko ustawia flagę i wraca natychmiast (0ms).
   tgTaskFn (Core 0) wykonuje pełny blok reinicjalizacji: Wire.end/begin,
   clock-pulse reset, czujnik.begin(), ghost-fix getEvent().
   I²C może blokować Core 0 dowolnie długo - loop() nigdy nie czeka.

 EFEKT: zero LOOP-SLOW przy odłączonych czujnikach TSL.

 v33g (2026-04-18) - I²C TIMEOUT FIX v2 + HISTORY NA CORE 0

 ROOT CAUSE 1 (log_56 - v33e/f nie pomogły):
   Wire.setTimeout() to metoda klasy Stream - ustawia timeout odczytu
   bajtów ze strumienia. NIE wpływa na magistralę I²C.
   Właściwa funkcja ESP32: Wire.setTimeOut() (duże T w TimeOut) -
   metoda klasy TwoWire ustawiająca timeout I²C bus.
   Efekt v33e/f: TSL nadal blokował ~8400ms na każdej próbie.

 NAPRAWA 1: Wire.setTimeout(50) -> Wire.setTimeOut(50).

 ROOT CAUSE 2 (log_55/56 - v33f zredukował ale nie wyeliminował):
   saveHistoryPoint() + saveAdaptStats() + saveEnergyStats() wywoływane
   bezpośrednio z loop() (Core 1) co 5 minut. Po usunięciu skanowania
   pliku (v33f) pozostał narzut LittleFS: open+write+close = ~1.1-1.4s
   (flash wear leveling, metadata flush). LOOP-SLOW 1114-1448ms co 5 min.

 NAPRAWA 2: volatile bool histSavePending.
   loop() tylko ustawia flagę (0ms). tgTaskFn (Core 0) sprawdza flagę
   w każdej iteracji i wykonuje zapis - LittleFS blokuje Core 0,
   loop() (Core 1) nigdy nie czeka.

 EFEKT: zero LOOP-SLOW z I²C i zero LOOP-SLOW z zapisu historii.

 v33f (2026-04-18) - HISTORY SCAN FIX + CLEAR BUG FIX

 ROOT CAUSE 1 (potwierdzony w log_55):
   saveHistoryPoint() co 5 minut czytała history.csv bajt-po-bajcie
   (while f.available() { f.read() }) licząc linie. Przy 288 punktach ×
   ~160B ≈ 46KB odczytu bajt-po-bajcie na LittleFS = ~1.6s blokady loop().
   Objaw: LOOP-SLOW 1658-1672ms dokładnie co 5 minut.

 NAPRAWA 1: globalny int histLineCount = -1 jako cache liczby linii.
   Skanowanie pliku tylko raz (przy starcie lub po resecie cache).
   Kolejne wywołania inkrementują licznik bez odczytu FS -> 0ms blokady.
   Cache resetowany do 0 po wyczyszczeniu historii.
   Cache ustawiany na HISTORY_MAX_POINTS-1 po rotacji rolling buffer.
   Przy pierwszym skanowaniu użyto chunked read (buf[512]) zamiast f.read().

 ROOT CAUSE 2 (ukryty bug):
   historyClearPending ustawiany na true w /api/history/clear, ale
   nigdy resetowany do false -> po kliknięciu "Wyczyść historię"
   saveHistoryPoint() zwracała early return na zawsze (do restartu).
   Historia przestawała być zapisywana bez żadnego komunikatu.

 NAPRAWA 2: historyClearPending = false po usunięciu plików w handlerze.

 EFEKT: zero LOOP-SLOW z saveHistoryPoint(); historia działa po clear.

 v33e (2026-04-18) - I²C TIMEOUT FIX

 ROOT CAUSE (potwierdzony w log_54):
   Czujniki TSL2561 są czasem fizycznie odłączane. Przy braku ACK na I²C
   biblioteka Wire czekała domyślny timeout (~8000ms) na każdą próbę.
   5 prób reinicjalizacji × ~8400ms = ponad 40s blokady loop() po restarcie.
   Objaw: [SECTION] odczytajSwiatloZFiltrem() = 8395-8606ms -> LOOP-SLOW.
   Fix logowania (v33d) był poprawny - LOOP-SLOW z logowania zniknął.
   Nowy LOOP-SLOW pochodzi wyłącznie z I²C timeout.

 NAPRAWA (v33e) - jedna linia:
   Wire.setTimeout(50) po Wire.begin() w setup().
   Każda nieudana próba trwa <50ms zamiast ~8400ms.
   Gdy czujnik podłączony - działa normalnie (odpowiedź <1ms << 50ms).
   Gdy odłączony - 5 prób × <50ms = <250ms blokady zamiast >40s.

 EFEKT: zero LOOP-SLOW z I²C niezależnie od stanu czujników TSL.

 v33d (2026-04-18) - LOG DUAL-BUFFER FIX (zastępuje v33c)

 ROOT CAUSE (potwierdzony w log_52):
   v33c dodała mutex portMAX_DELAY -> loop() nadal blokował 1753ms gdy
   tgTask trzymał mutex podczas wolnego zapisu LittleFS (rotacja/flash erase).
   Mutex zamienił blokadę VFS na blokadę FreeRTOS, ale czas blokady był ten sam.

 NAPRAWA (v33d) - dwa osobne bufory per-rdzeń + non-blocking Core 1:
 1. Dwa niezależne logBuffer[2] (Core 0 pisze do [0], Core 1 do [1]).
    Append do bufora BEZ mutexa - brak wyścigu, bo każdy rdzeń pisze
    wyłącznie do swojego bufora.
 2. Mutex chroni tylko ZAPIS DO LittleFS (sekcja krytyczna I/O).
 3. Core 1 (loop): xSemaphoreTake(logMutex, timeout=0) - jeśli mutex
    zajęty, SKIP flush (wiadomość zostaje w buforze, zapisana przy
    następnym wywołaniu, zwykle za <100ms). loop() NIGDY nie blokuje.
 4. Core 0 (tgTask): xSemaphoreTake(logMutex, portMAX_DELAY) - czeka;
    nie jest time-critical, nie traci wpisów.

 EFEKT: zero LOOP-SLOW niezależnie od czasu operacji LittleFS.

 v33b (2026-04-17) - DBG + HEAP FIX
 FIX-v33b-HEAP-FRAG: Pięć poprawek eliminujących fragmentację HEAP w TG:
 1. tgPost(): resp.reserve(4096) po resp="" (przypisanie "" może zwolnić bufor)
 2. tgPost(): nagłówek HTTP przez printf() - zero tymczasowych alokacji Stringów
 3. sendTelegramDocument(): resp.remove(0,1000) zamiast resp=resp.substring(1000)
 4. sendTelegramMessage(): reserve() na safe i payload przed konkatenacją
 5. tgTask stack: 8192 -> 12288 B (bezpieczny margines dla TLS + builderów)
 DBG-v33b: rozbudowana diagnostyka (tymczasowa - można wyłączyć po analizie):
 - tgTaskFn: log START + live-counter co 15s (iter, stack_free, freeH, wifi)
 - tgTaskFn: log TG-POLL START/END z heap przed/po każdym getUpdates
 - tgTaskFn: log TG-CMD z numerem komendy i heap przed wykonaniem
 - tgEnsureConnected: log KAŻDEJ próby connect (nie tylko wolnych/nieudanych)
 - heartbeat: dodano largestBlk (fragmentacja) i tgStack_free (stack watermark)

 v32 (2026-04-11)
 FIX-v32-TG-PERSISTENT-CLIENT: Stałe połączenie SSL do api.telegram.org.

 ROOT CAUSE (v31 i wcześniej):
   Każda operacja TG tworzyła nowy WiFiClientSecure (~40KB SSL context).
   Przy poll co 30s + menu + raporty = setki alloc/dealloc SSL dziennie
   -> fragmentacja HEAP -> www przestaje odpowiadać po godzinach/dobach.

 ZMIANA (3 nowe elementy, wszystkie funkcje TG przepisane):
 1. Globalny WiFiClientSecure tgClient - alokacja SSL raz przy pierwszym
    wywołaniu, trzymana na zawsze. Zajmuje ~40KB RAM na stałe (przy
    starcie free heap ~120KB - zostaje ~80KB, wystarczająco).
 2. tgEnsureConnected() - sprawdza stan połączenia, reconnect gdy:
    a) pierwsze wywołanie, b) WiFi wrócił po przerwie,
    c) idle >55s (Telegram zamyka połączenie po ~60s bezczynności),
    d) serwer zerwał połączenie.
 3. tgPost(path, body) - wspólna funkcja HTTP/1.1 keep-alive dla
    wszystkich operacji TG (poll, sendMessage, delete, answer, menu).
    sendTelegramDocument używa tgClient bezpośrednio (multipart/form-data)
    i wymusza reconnect po zakończeniu (serwer wysyła Connection:close).

 KONSEKWENCJE:
 [OK] Zero fragmentacji HEAP - SSL context alokowany raz, nie co wywołanie.
 [OK] Szybsze odpowiedzi TG - brak handshake przy każdym wywołaniu.
 [OK] www działa stabilnie przez dowolnie długi czas.
 [WARN]️ ~40KB RAM zajęte na stałe (akceptowalne przy ~120KB free).
 [WARN]️ Pierwsze wywołanie TG po starcie lub po zerwaniu WiFi: ~1-2s TLS handshake.

 v31 (2026-04-11)

 ROOT CAUSE: każde wywołanie WiFiClientSecure alokuje ~30-40KB SSL context.
 tgDeleteAllHistory() kasowała do 20 wiadomości = 20 SSL alloc/dealloc per wywołanie.
 tgMenuReturnAt=5s -> co 5s: 20 SSL kontekstów -> po 15min: 600 alloc/dealloc SSL
 -> fragmentacja HEAP -> AsyncWebServer nie może alokować buforów -> www nie odpowiada.
 WDT crash gdy brak esp_task_wdt_reset() w środku pętli deletowania.

 NAPRAWA (7 zmian architektonicznych):
 1. tgDeleteAllHistory() -> kasuje TYLKO tgLastMenuMsgId (1 SSL zamiast 20).
    Ring-buffer czyszczony przez memset() bez połączeń sieciowych.
 2. pollTelegramCommands(): interwał 10s -> 30s (1 SSL/30s zamiast 1 SSL/10s).
 3. tgMenuReturnAt: 5s -> 300s (5min) - menu wraca co 5min nie co 5s.
 4. deleteTelegramMessage(): timeout 5000 -> 1500ms.
 5. answerTelegramCallback(): timeout 5000 -> 2000ms.
 6. sendTelegramMessage/InlineMenu/confirmClear/confirmRestart: timeout 8000 -> 5000ms
    + esp_task_wdt_reset() przed i po każdym http.POST().
 7. sendTelegramInlineMenu(): esp_task_wdt_reset() po tgDeleteAllHistory().

 EFEKT: max 1 WiFiClientSecure aktywny jednocześnie, max ~3 SSL/min zamiast ~120/min.
 Heap pozostaje niezdefragmentowany przez wiele godzin pracy.

 v30-3 (2026-04-11)
 FIX-v30-3-TG-MENU-RETURN: Po wybraniu opcji z menu Telegram czat nie wracał
 do menu - brakował mechanizm powrotu. Dodano zmienną tgMenuReturnAt (unsigned long).
 Po wykonaniu każdej komendy treściowej (status, logi, temp, energia, lux, adaptacja,
 historia, harmonogram, LED toggle, tryb toggle, clr_do, notif on) ustawiamy
 tgMenuReturnAt = millis() + 5000. W loop() - przed blokiem tgDeferredCmd -
 sprawdzamy timer i gdy upłynie ustawiamy TG_MENU. Efekt: menu pojawia się
 automatycznie ~5 sekund po każdej odpowiedzi bota.

 FEATURE-v30-2-TG-AUTOCLEAN: Automatyczne menu i czysty czat.
 - Ring-buffer tgMsgHistory[20] przechowuje message_id wszystkich wysłanych wiad.
 - tgDeleteAllHistory(): usuwa WSZYSTKIE poprzednie wiadomości bota przed nowym menu.
 - tgPushMsgId(): rejestruje każdą nową wiadomość w buforze.
 - pollTelegramCommands(): po każdym pollu, gdy tgLastMenuMsgId==-1 (brak menu),
   automatycznie ustawia TG_MENU -> menu pojawia się samo bez pisania /start.
 - Efekt: czat zawsze pokazuje tylko aktualne menu, stare wiadomości są usuwane.

 v30-1 (2026-04-11)
 CLEAN-v30-1-TG-NOISE: Usunięto zbędne logi TG z log.txt oraz przyciski z panelu.
 - Wszystkie logPrintf/logPrintln "📨 [TG]" usunięte z sendMessage, pollTelegram,
   sendDocument, confirmClear, confirmRestart, sendInlineMenu i callbacków.
   Pozostały tylko błędy HTTP (niezerowy kod) żeby nie utracić diagnostyki.
 - Usunięto przycisk "Wyślij TG" z zakładki Logi.
 - Usunięto przycisk "Wyślij teraz" z karty Telegram w Ustawieniach.

 v30-0 (2026-04-11)
 FEATURE-v30-0-TG-CLEAN: Automatyczne czyszczenie okna czatu Telegram.
 - deleteTelegramMessage(), parseTelegramMsgId(), tgLastMenuMsgId.
 - Nowe opcje menu: 🌊 Historia czujników, ⏰ Harmonogram, 🔕/🔔 Powiadomienia.
 - Nowe komendy: /historia, /harmonogram, /notif.
 - TgDeferCmd: TG_SENSOR_HIST, TG_SCHEDULE, TG_NOTIF_TOGGLE.

 v29-16 (2026-04-11)
 FEATURE-v29-16-TG-MENU: Rozbudowane menu Telegram z 10 przyciskami.
 - Wiersz 1: 📊 Status | 🌡️ Temperatury
 - Wiersz 2: ⚡ Energia | 🔆 Lux & Światło
 - Wiersz 3: 🧠 Adaptacja | 📋 Pobierz logi
 - Wiersz 4: 💡 LED wł/wył (live stan) | 🤖 Tryb AUTO/MAN (live)
 - Wiersz 5: 🗑️ Wyczyść logi (z potwierdzeniem) | 🔄 Restart ESP (z potwierdzeniem)
 Nowe buildTelegram*() raporty: temp, energia, lux, adaptacja.
 Potwierdzenia czyszczenia logów i restartu przez inline Yes/No.
 Togglery LED i trybu działają bezpośrednio, zapisują do EEPROM.

 v29-15 (2026-04-11)
 FIX-v29-15-TG-SSL: Menu Telegram (i inne komendy) zwracało HTTP -1
 po wpisaniu /start. Przyczyna: pollTelegramCommands() wywoływał
 sendTelegramInlineMenu() bezpośrednio, gdy WiFiClientSecure pollera
 nadal żył na stosie -> dwa jednoczesne konteksty SSL -> brak socketu.
 Naprawa: zamiast bezpośrednich wywołań send ustawiamy flagę
 tgDeferredCmd (TG_MENU/TG_LOGS/TG_STATUS). Flaga jest obsługiwana
 w loop() PO zakończeniu i destrukcji pollera - tylko jedno połączenie
 SSL aktywne w danej chwili.

 v29-14 (2026-04-10)
 FEATURE-v29-14-TELEGRAM: Integracja Telegram Bot.
 - Automatyczny dzienny raport o 00:00 (z logDailySummary).
 - Komenda /sendlogs - raport na żądanie przez Telegram.
 - Komenda /status  - bieżące wartości temp/moc/godzina.
 - API: POST /api/telegram/save  (token, chatId, enabled).
 - API: POST /api/telegram/send  (wyślij teraz z panelu).
 - API: GET  /api/telegram/status (odczyt konfiguracji).
 - Konfiguracja zapisywana w /telegram_config.json (LittleFS).
 - Przycisk "Wyslij TG" w zakładce Logi panelu WWW.
 - Karta konfiguracji Telegram w zakładce Ustawienia.

 v29-13 (2026-04-10)
 FIX-v29-13-MINLUX-LOG: Log "MOCNO uzupełniam" był wyświetlany przed sprawdzeniem
 anyChange -> spamował gdy aktualne PWM >= target (fix NOCHANGE blokował rampę ale
 log się już wyświetlił). Przeniesiono log ZA blok anyChange, tuż przed startem rampy.

 v29-12 (2026-04-10)
 FIX-v29-12-MINLUX-NOCHANGE: Po fixie FLOOR (v29-10) rampa startowała z dystansem=0
 gdy aktualne PWM >= targetPWM -> RAMPA START+KONIEC co 30s, ANOMALIA 119ms, oscylacja.
 Naprawiono: przed startem rampy sprawdź czy po zastosowaniu FLOOR jakikolwiek kanał
 faktycznie wymaga zmiany. Jeśli nie - aktualizuj minLuxCurrentPWM i return.

 v29-11 (2026-04-10)
 FIX-v29-11-WEEKHISTORY: Dodano float energyWeekHistory[5] (tydzień miesiąca 0-4).
 Przy resecie tygodnia (poniedziałek) energyWeekWh zapisywane do tablicy[wkIdx].
 Reset tablicy przy nowym miesiącu. Zapis/odczyt do energy_stats.json (w0..w4).
 Eksport przez /api/status (weekHistory[]). Wykres miesięczny T1-T4 = realne dane.
 FIX-v29-11-DAYHISTORY: Dodano float energyDayHistory[7] (Pn=0..Nd=6).
 Przy resecie dnia (północ) energyTodayWh zapisywane do tablicy[isoDay].
 Reset tablicy przy nowym tygodniu (poniedziałek). Zapis/odczyt do
 energy_stats.json (pola d0..d6). Eksport przez /api/status (dayHistory[]).
 Wykres tygodniowy używa teraz realnych danych per dzień - nie szacunków.
 FIX-v29-11-SPARK-PWM: (przeniesiony z v29-10) Sparkline oblicza W z PWM.

 v29-10 (2026-04-09)
 FIX-v29-10-MINLUX-FLOOR: applyMinLuxMode() stosowała targetPWM bezpośrednio nawet gdy
 był niższy od bieżącego PWM -> rampa w DÓŁ podczas "MOCNO uzupełniam". Naprawiono:
 channelTarget = max(targetPWM, aktualnePWM) - MIN LUX jest podłogą, nigdy sufitem.
 Zaobserwowane w logu: [12:52:59] PWM 67->59 przy lux=226 cel=1000.
 FIX-v29-10-SPARK-PWM: Sparkline 'Moc ostatnie 12h' używała fallback na ostatnią
 kolumnę CSV (lux_cel=1000 const) gdy brak power_w -> wszystkie słupki równe.
 Naprawiono: oblicz W = avg(pwm0..4)/1023 * 33W gdy brak power_w w nagłówku.
 FIX-v29-10-TRYBAUT-JITTER: Obniżono próg TRYBAUT 50->10ms. Δ=1-2ms to jitter
 millis() na ESP32/FreeRTOS gdy dwie iteracje loop() trafiają na ten sam tick 1ms.
 Prawdziwe double-calle (race condition) mają Δ<5ms i nadal będą wykryte.

 v29-9 (2026-04-09)
 FIX-v29-9-MONTH-BARS: Wykres miesięczny używa realnego weekWh dla T4 (bieżący tydzień).
 T1-T3 = szacunek z (monthWh-weekWh)/completedWks. Usunięto fikcyjne mnożniki 0.94/1.03/0.98.
 Dodano etykiety T1-T4 pod słupkami. Słupki bez danych wygaszane opacity 0.2.

 v29-8 (2026-04-09)
 FIX-v29-8-WEEK-BARS: Usunięto _loaded guard z wykresów słupkowych (tygodniowy/miesięczny/spark).
 Wykres tygodniowy zamrażał się na pierwszym renderze gdy energyWeekWh=0 (przed NTP/load).
 Dodano Math.max(0, avgD) aby ujemne avgD nie blokowało słupków poprzednich dni.

 v29-7 (2026-04-08)
 ────────────────────────────────────────────────────────

 FIX-v29-7-LOG-ROTATE-REPORT: Komunikat rotacji logował keepBytes
   (szacowany) zamiast rzeczywistego rozmiaru log.txt po rotacji.
   Po wyrównaniu do \n plik może być o kilkanaście bajtów krótszy.
   Naprawa: odczyt tmpSz z log_tmp przed rename -> komunikat podaje
   dokładny rozmiar wynikowy (identycznie jak test rotacji T1/T2/T6).

 FIX-v29-7-LOG-ROTATE-SILENT: Dwa przypadki rotacji były ciche
   (brak logu) różniące się od pliku testowego:
   1. SKIP (keepBytes >= logSize) - rotacja pomijana bez śladu w logu.
      Dodano: [LOG] SKIP rotacja: keepBytes(X) >= logSize(Y) (za Komentarze).
   2. ERR open - nie można otworzyć log.txt lub log_tmp -> cichy cleanup.
      Dodano: [LOG] BŁĄD: nie można otworzyć plików rotacji (zawsze).
   Przy okazji przepisano warunek if(keepBytes<logSize){...}else{cleanup}
   na czytelniejszy if(keepBytes>=logSize){SKIP}else{rotacja} zgodny
   z testem. Logika identyczna - tylko struktura kodu czytelniejsza.

 v29-6 (2026-04-08)
 ────────────────────────────────────────────────────────

 FIX-v29-6-LOG-ROTATE: Rotacja log.txt była fizycznie niemożliwa
   do wykonania na partycji LittleFS 896 KB.
   Przyczyna: LOG_MAX_SIZE (600 KB) + LOG_TRIM_TO (400 KB) = 1000 KB
   > 896 KB LittleFS. Podczas rotacji oba pliki (log.txt + log_tmp.txt)
   muszą istnieć jednocześnie -> brak miejsca -> fDst.write() urywał się
   w połowie bez zwracania błędu -> LittleFS.remove(log.txt) kasował
   oryginał -> rename(log_tmp -> log.txt) dawał niekompletny plik ->
   FS zapełniał się -> ESP resetował się w pętli.
   NAPRAWA: Usunięto LOG_MAX_SIZE i LOG_TRIM_TO. Rotacja w pełni
   dynamiczna: trigger = wolne < 20% FS (LittleFS.usedBytes() live),
   keepBytes = fsFree - 48 KB margines (max 55% FS). Każdy fDst.write()
   sprawdzany - przy błędzie log_tmp usuwany, log.txt nienaruszony.
   Fallback: gdy fsFree < 48 KB -> log.txt usuwany w całości.
   Działa poprawnie niezależnie od rozmiaru partycji i innych plików.

 v29-5 (2026-04-01)
 ────────────────────────────────────────────────────────

 FIX-v29-5-RAMP-MORNING: Rampa poranna nie startowała po ciągłej
   pracy przez noc (bez restartu). Przyczyna: rampScheduleFinished
   i wasRampActive były static zmiennymi deklarowanymi PO bloku
   isNight z early return. Noc robiła return przed ich deklaracją
   -> flagi nigdy nie były resetowane -> rano if(rampScheduleFinished)
   robił early-exit bez wykonania rampy -> LEDy nie świeciły.
   NAPRAWA: deklaracje przeniesione przed blok isNight; dodano
   reset obu flag wewnątrz bloku nocnego przed return.

 v29-4 (2026-03-31)
 ────────────────────────────────────────────────────────

 FIX-v29-HIST-PERSIST: /api/log-clear usuwał history.csv razem
   z log.txt -> kliknięcie "Wyczyść logi" kasowało całą historię
   wykresów. Historia ma swój własny endpoint /api/history/clear.
   Usunięto LittleFS.remove(HISTORY_FILE) z handlera log-clear.
   Rolling buffer 288 punktów (24h) działa bez zmian.

 v29-3 (2026-03-31)
 ────────────────────────────────────────────────────────

 FIX-v29-TRYBAUT-NIGHT: TRYBAUT alarmował przez całą noc (isNight=1)
   przy jitterze Δ=1ms loopa. Noc = LEDy wyłączone, żadna rampa nie
   działa -> alarm był zawsze fałszywy. Dodano !isNightGlobal do warunku
   detekcji w trybAuto().

 FIX-v29-DIAG20-NIGHT: DIAG-20 (DS18B20 KOLIZJA) fałszywy alarm w nocy
   gdy obie płyty w spoczynku mają identyczną temperaturę (~25°C).
   Dodano !isNightGlobal do warunku identical w diagTempSensorCollision().

 v29-2 (2026-03-30)
 ────────────────────────────────────────────────────────

 UI-WYKRESY: Usunięto kartę "Statystyki okresu" z zakładki Wykresy
   — dane dostępne w dashboardzie, duplikacja zbędna.
   Usunięto: card HTML, wywołanie renderStats(), guard stats-panel
   w gałęzi braku danych loadCharts().

 v29 (2026-03-30) loadEnergyStats() wywoływany przed synchronizacją NTP
   -> getLocalTimePL() zwracało false -> early return -> wszystkie liczniki
   (ledOnMinutesToday/Week/Month, energyTodayWh itd.) zostawały = 0
   po każdym restarcie. Liczniki kasowały się przy każdym wgraniu firmware.
   NAPRAWA: loadEnergyStats() przeniesiony ZA pętlę NTP w setup().
   Wywołanie teraz gwarantuje poprawny czas przed porównaniem dat.

 FIX-v29-YR: Brak pola "yr" w starym energy_stats.json (savedYr==-1)
   powodował sameDay/sameWeek/sameMon=false (rok 0 ≠ rok bieżący).
   Stary plik z innej wersji firmware był "niewidoczny". Dodano
   early return z logiem gdy savedYr<0 + komunikat diagnostyczny.

 FIX-v29-DIAG: Dodana pełna diagnostyka loadEnergyStats() w logach:
   savedDay/curDay/savedYr/curYr + sameDay/sameWeek/sameMon + wczytane wartości.
   Pozwala jednoznacznie stwierdzić dlaczego liczniki się zerują.

 FIX-v29-ADAPT-LOG: ADAPT logował identyczną wartość co 30s
   przez całe godziny przy stabilnym świetle -> dziesiątki wpisów/h.
   NAPRAWA: loguj tylko gdy adapt.PWM zmienił się o >5 LUB heartbeat
   co 5 minut (było 30s bezwarunkowo).

 FIX-v29-EEPROM-COMMIT: EEPROM.commit() w zapiszAdaptacjeDoEEPROM()
   blokował loop() synchronicznie na ~10-50ms -> dwa kolejne wywołania
   trybAuto() trafiały w Δ=1-5ms -> TRYBAUT alarm co godzinę o północy.
   Potwierdzone przez log_27: alarmy dokładnie o 01:49, 02:49, 03:49
   zbieżne z "💾 Adaptacja zapisana".
   NAPRAWA: commit odroczony przez flagę g_UNUSED_eepromCommitPending_DO_NOT_USE.
   Wykonywany w sekcji diagów - poza ścieżką trybAuto() 100ms.

 v28b (2026-03-29)
 ────────────────────────────────────────────────────────

 FIX-v28b-WEEK: ledOnMinutesWeek/energyWeekWh zerował się
   przy restarcie gdy plik był zapisany w inny dzień tygodnia.
   PRZYCZYNA: loadEnergyStats() porównywał savedWk==curIsoMon
   (klucz = yday poniedziałku tego tygodnia ISO). Przy zapisie
   w niedzielę poprzedniego tygodnia wzór dawał wk=81, a przy
   odczycie w niedzielę bieżącego tygodnia curIsoMon=82 -> niezgodność
   o 1 -> sameWeek=false -> reset tygodnia. Potwierdzone przez
   energy_stats.json: wk=81, curIsoMon=82, ledMinWk=70 -> 0.
   NAPRAWA: sameWeek = savedDay ∈ [curIsoMon .. curIsoMon+6]
   Zakres dni jest odporny na edge case niedzieli i wszelkie
   przesunięcia klucza ISO między zapisem a odczytem.

 v28 (2026-03-29)
 ────────────────────────────────────────────────────────

 FIX-v28-MINLUX-WINDOW: MIN LUX nie działał w przerwie
   między rampą południową a rampą wieczorną.
   PRZYCZYNA: inEveningWindow w trybAuto() obejmował tylko
   (eveningOnEnd..EVENING_OFF_START), a inEveningRampWindow
   (eveningOnStart..eveningOnEnd) był osobną flagą.
   Przerwa (middayOffEnd..eveningOnStart) dawała shouldBeOn=false
   ale trybAuto() nadpisywał inLightingWindowGlobal=false co 100ms
   jednocześnie z rampScheduleActive=false -> applyMinLuxMode()
   nigdy nie dostawała obu warunków spełnionych naraz.
   NAPRAWA: Scalono inEveningWindow i inEveningRampWindow:
   inEveningWindow = (nowMin >= eveningOnStart && nowMin < EVENING_OFF_START)
   Przerwa południe (middayOffEnd..eveningOnStart) -> shouldBeOn=false
   -> MIN LUX aktywny przez całą przerwę zgodnie z założeniem.

 FEATURE-v28-MINLUX-INTERVAL: Interwał sprawdzania MIN LUX
   (dotąd stały 30s const) przeniesiony do zmiennej minLuxIntervalSec
   z zakresem 10-300s, persystowany w EEPROM addr 146 (2 bajty).
   Dodano pole w panelu WWW sekcja Regulacja Adaptacyjna:
   "Interwał MIN LUX (s)" z przyciskiem Ustaw, obok pola Docelowy LUX.
   Endpoint: /api/minlux/save przyjmuje nowe pole "interval" (int).

 v27 (2026-03-28)
 ────────────────────────────────────────────────────────

 FIX-v27-TRYBAUT-DBG: Rozszerzone logowanie przy alarmie TRYBAUT.
   Przy pierwszym alarmie i każdym podsumowaniu (co 10 min) logowany
   jest snapshot flag: softStartActive, transitionActive,
   rampaAdaptacyjnaAktywna, rampScheduleActive, minLuxModeActive,
   inLightingWindowGlobal, isNightGlobal, power, tryb.
   Pozwoli zidentyfikować która ścieżka przecieka po v27.

 FIX-v27-TRACE-FLOOD: [TRYBAUT-CALL] był za flagą Komentarze=true domyślnie
   -> ~100 linii/s -> LittleFS zapełniało się w minuty -> reset.
   Naprawa: osobna flaga trybAutoTrace (domyślnie false), niezależna
   od Komentarze. Włączać ręcznie tylko na czas diagnozy.

 FIX-v27-LOG-WDT: Kopiowanie 400 KB przy rotacji log.txt blokowało
   loop() na >1s bez resetu watchdoga -> potencjalny WDT reset przy
   zapełnionym logu. Dodano esp_task_wdt_reset() w pętli kopiowania.

   (aktualizujRampeAdaptacyjna co 10ms) może zakończyć rampę
   (rampaAdaptacyjnaAktywna->false) w środku iteracji loop().
   Normalna ścieżka 100ms widzi wtedy false i woła trybAuto()
   w tej samej iteracji -> Δms=0-1 -> TRYBAUT alarm (logi 18:44, 20:24).
   Naprawa:
   1. Fast-loop adaptacyjny przeniesiony PRZED blok normalnej ścieżki.
   2. Flaga _adaptFastCalledThisLoop (lokalna, auto-reset co iterację)
      blokuje trybAuto() w normalnej ścieżce jeśli fast-loop adaptacji
      już działał — identyczny wzorzec jak _trybAutoCalledNormal (v15).

 v26 (2026-03-28)
 ────────────────────────────────────────────────────────

 FIX-v26-WODA-GUARD: Czujnik nad wodą używany TYLKO gdy
   uzywajCzujnikaNadWoda=true (ustawienie www). Poprzednio
   czujnikNadWodaAktywny=true (hardware) wystarczał by użyć
   jego wartości wszędzie - nawet gdy user go nie wybrał.
   Skutek log_23: TSL-WODA saturowany (65536 lux) ->
   calculateMinLuxPWM() zwracał 0 -> LED gasły ~1h w przerwie.
   Naprawione w: calculateMinLuxPWM, obliczAdaptacyjnaJasnosc,
   uczSieTransmisji, applyMinLuxMode (logi), diagSensorSwapped,
   odczytajSwiatloZFiltrem (blok TSL-WODA).

 FIX-v26-WODA-IR-SAT: Dodano sprawdzenie saturacji IR dla
   czujnika nad wodą (ir >= broad*0.85 -> pomiń EMA), analogicznie
   jak dla czujnika pokojowego. Log debug zmieniony na co 30s.

 FIX-v26-DS18B20-9BIT: Rozdzielczość DS18B20 obniżona z 12-bit
   (konwersja 750ms) do 9-bit (94ms, dokładność ±0.5°C).
   W log_23 readTemperatures() blokowało pętlę co 15s przez
   ~6.5s (zawieszony bus 1-Wire przy uszkodzonym czujniku wody).
   Czas oczekiwania: 3×250ms -> 2×60ms = 120ms łącznie.

 v25 (2026-03-28)
 ────────────────────────────────────────────────────────

 FIX-v25-TSL-GHOST: Czujnik TSL2561 dostarczał poprawne odczyty
   (getEvent() > 0) mimo że begin() zwracał false przez całą sesję.
   Objaw z log_19: 5 prób reinicjalizacji co 30s -> błąd I2C, ale
   odczyty SUROWE pojawiały się normalnie -> czujnikPokojowyAktywny
   pozostawało false -> adaptacja nieaktywna mimo działającego sprzętu.
   Naprawa: po nieudanym begin() weryfikacja przez getEvent().
   Jeśli light>0 -> czujnik działa, oznaczamy jako aktywny (ghost-fix).
   Konfiguracja (enableAutoRange, integrationTime) zachowana z setup().

 FIX-v25-HEAP-DIAG: Jednorazowy snapshot heap przed/po pierwszym
   wywołaniu applyMinLuxMode(). W log_19 zaobserwowano permanentny
   spadek minHeap o ~14 kB dokładnie przy tej operacji (13:22->13:23).
   Snapshot pozwoli potwierdzić lub wykluczyć to miejsce jako źródło
   wycieku przy następnej sesji.

 v24 (2026-03-25)
 ────────────────────────────────────────────────────────

 FIX-v24-ADAPT-HYST: Histereza adaptacji (log_4 analiza).
   PROBLEM: Sekcja 5 trybAuto() porównywała nowy cel vs stare
   (aktualny backup), który zmienia się w trakcie rampy.
   Po zakończeniu rampy 262->240 (stare=240), słońce zmienia
   się, nowe=231, delta=9>8 -> nowa rampa. Potem nowe=240,
   delta=9>8 -> rampa wstecz. Pętla co ~5s, spam TRYBAUT.
   NAPRAWA: Statyczna tablica _lastAdaptCommitted[5] przechowuje
   ostatni ZATWIERDZONY cel adaptacji. Porównanie idzie vs tej
   wartości, nie vs bieżącego backup. Nowa rampa odpala się
   dopiero gdy nowy cel różni się o >MIN_ZMIANA_PWM od
   poprzedniego zatwierdzonego — pełna histereza.

 v23 (2026-03-25)
 ────────────────────────────────────────────────────────

 FIX-v23-TSL-NUIT: odczytajSwiatloZFiltrem() wychodzi natychmiast
   gdy isNightGlobal=true - brak prób reinicjalizacji TSL przez noc
   (generowały logi #73..#115 co 10 min bez żadnego efektu).
   Reset tslFailCount=0 i lastTslRetry=now przy wyjściu,
   dzięki czemu świt = świeży start sprawdzenia czujnika.

 FIX-v23-TRYBAUT: MIN_ZMIANA_PWM 3->8. Przy wartości 3
   adaptacja wieczorna (harm=262, adapt=258, Δ=4) uruchamiała
   zastosujRampeAdaptacyjna() co 30s -> fast-loop woła trybAuto()
   co 10ms przez 30s -> TRYBAUT alarmował fałszywie (49 razy/10 min).
   Wartość 8 eliminuje fałszywe alarmy zachowując reaktywność MIN LUX.

 v22 (2026-03-23)
 ────────────────────────────────────────────────────────

 FIX-v20-WASRAMPACTIVE + FIX-v20-MINLUX-ZERO + FIX-v20-MINLUX-DIFF
 FIX-v21-ADAPTACJA: propozycjaLED = harmonogramLux - swiatloNaturalne
 CLEANUP-v21: usunięto obliczWspolczynnikUczenia(), minZauwazoneLux,
   maxZauwazoneLux i wszystkie ich referencje w kodzie i panelu.
 FIX-v22-BARSCALE: pasek lux ma dynamiczną skalę:
   W oknie świecenia: 100% = lux suwaka (autoPWM * luxPPwm * glassT)
   Poza oknem (MIN LUX): 100% = cel MIN LUX
   Dodano do JSON: inLightingWindow, autoPWM

 v21 (2026-03-23)
 ────────────────────────────────────────────────────────

 FIX-v20-WASRAMPACTIVE: wasRampActive był zawsze false - każda
   rampa po pierwszej w sesji wchodziła w early-exit bez działania.

 FIX-v20-MINLUX-ZERO: trybAuto() zerował backup co 100ms gdy
   minLuxModeActive=true - LED ciemny przez całą przerwę południową.

 FIX-v20-MINLUX-DIFF: applyMinLuxMode sprawdza również diffActual
   vs backupBrightnessComposite obok diff vs minLuxCurrentPWM.

 FIX-v21-ADAPTACJA: propozycjaLED = harmonogramLux - swiatloNaturalne.
   Suma słońce+LED = dokładnie tyle lux ile ustawiono suwakiem.

 CLEANUP-v21: Usunięto zbyteczny kod i panel:
   - obliczWspolczynnikUczenia() - martwa po FIX-v21-ADAPTACJA
   - adaptacja.minZauwazoneLux / maxZauwazoneLux - używane tylko
     przez usuniętą funkcję
   - EEPROM defines/put/get dla min/max
   - Panel HTML "Zakres Lux: X-Y"
   - JSON adaptMinLux / adaptMaxLux
   - inteligentyZapisAdaptacji: uproszczony (znaczacaZmiana była
     oparta na maxZauwazoneLux)

 v19 (2026-03-21)
 ─────────────────────────────────────────────────────────
 FIX-ALLFINISHED-SPAM (v19): Po zakończeniu rampy południe (allFinished=true)
   trybAuto() wywoływana co 100ms wchodziła ponownie w blok allFinished przez
   cały czas trwania okna rampowego (np. fadeMinutes=2 -> 2 minuty spamu).
   Skutki: setki logów "[FIX-v13] Rampa południe zakończona" na minutę,
   rampScheduleActive=true blokował applyMinLuxMode() -> symulacja lux i
   adaptacja nie reagowały na zmiany podczas przerwy południe.
   Przyczyna: rampInitialized[i] nie był resetowany po allFinished,
   a `progress>=1.0` powodował allFinished=true przy każdej iteracji.
   Naprawa-1: Early exit na początku if(rampActive) gdy
   autoRampInitialized=true && rampInitialized[0]=true -> ramp done,
   ustawia rampScheduleActive=false i wraca bez przetwarzania.
   Naprawa-2: Guard !minLuxModeActive na bloku FIX-v13 minlux activation
   jako dodatkowe zabezpieczenie przed ponownym uruchomieniem.

 v18 (2026-03-21)
 ─────────────────────────────────────────────────────────
 FIX-RAMP-UP-RESTART (v17): lastPwmStepMillis underflow guard
   niszczył wynik INIT rampy UP po restarcie. Guard (elapsedMs <= nowMs)
   ustawiał lastPwmStepMillis=0 gdy elapsedMs > nowMs (zawsze przez
   pierwsze minuty uptime). Blok UPDATE natychmiast nadpisywał poprawne
   currentRampPwm z INIT wartością ≈ 0 -> rampa startowała od zera
   zamiast od aktualnego punktu harmonogramu -> PWM zbyt niski przez
   pozostały czas rampy.
   Naprawa: unsigned wrap (nowMs - elapsedMs) - standardowy wzorzec millis().
   Dotyczy dwóch miejsc: ścieżka normalna (linia ~9507) i ścieżka
   currentBrightness >= target (linia ~9449).

 FIX-LIGHTING-WINDOW (v18): inLightingWindowGlobal=false podczas rampy
   harmonogramowej. Blok if(rampActive) robił return przed dotarciem do
   miejsca gdzie shouldBeOn ustawia inLightingWindowGlobal=true.
   Skutki: regulacja adaptacyjna wyłączona przez całą rampę ranną/wieczorną;
   zapis EEPROM z panelu blokowany podczas rampy; applyMinLuxMode mogła
   aktywować MIN LUX walczący z rampą wieczorną ON.
   Naprawa: inLightingWindowGlobal=true na początku bloku if(rampActive).

 FIX-WIDGET-MIDDAY-SENSOR (v18): widget rampy południe używał guard
   (czujnikPokojowyAktywny || czujnikNadWodaAktywny) przed calculateMinLuxPWM()
   co powodowało cel widgetu=0 gdy czujnik odpada (np. TSL nie znaleziony
   po restarcie). Niespójne z FIX-v16 który usunął ten guard z pętli target.
   Naprawa: usunięto guard z bloku widget - calculateMinLuxPWM() ma własny fallback.

 FIX-DEFAULT-TAB (v18): Domyślna zakładka = Ustawienia zamiast Dashboard.
   Naprawiono: active przesunięte na Dashboard w HTML i init JS.
   loadStatus() wywołuje się dopiero przy kliknięciu zakładki Ustawienia.

 FIX-TERMINAL-KEYBOARD (v18): Automatyczny focus na polu tekstowym
   terminala wywoływał klawiaturę na urządzeniach mobilnych.
   Naprawiono: focus aktywny tylko na desktopie (detekcja userAgent).

 FIX-RAMP-BADGE-GRADIENT (v18): Badżety rampy w harmonogramie -
   zielony (↑) i czerwony (↓) zastąpione gradientami nawiązującymi
   do kolorystyki LED: amber->żółty (wznoszenie), ciepły->niebieski
   (wieczór zachód), błękitny (przerwa południe). Nowa klasa rb-down-mid.

 v16 (2026-03-15)
 ─────────────────────────────────────────────────────────
 FIX-RESET-RAMP-DOWN: Natychmiastowe zgaśnięcie LED po
   restarcie w trakcie rampy wieczornej (OFF) lub południowej

 BUG: backupBrightnessComposite nie jest zapisywany do EEPROM.
      Po restarcie = 0. Blok rampActive zwraca się przed blokiem
      mid-ramp soft-start. Rampa DOWN init czytała startValue=0
      z backupBrightness -> rampDownStartValue=0 -> expectedPwm=0
      przez całe pozostałe okno rampy -> natychmiastowe zgaśnięcie.

 ANALIZA WSZYSTKICH 4 RAMP PO RESECIE:
   - Rampa UP rano    : OK - formuła target*progress nie używa backup
   - Rampa UP wieczór : OK - j.w.
   - Rampa DOWN południe : BUG - startValue=0, target=minLux,
                           diff<0 -> skok do minLux bez interpolacji
   - Rampa DOWN wieczór  : BUG - startValue=0, target=0,
                           expectedPwm=0 -> natychmiastowe zgaśnięcie

 NAPRAWA-1 (główna): W bloku init RAMPA DOWN, gdy startValue==0
      i !autoRampInitialized i backupAuto[i]>0 (=restart w trakcie
      rampy, nie celowe zerowanie): odtwórz startValue z
      backupAutoBrightnessComposite. lastPwmStepMillis[i] ustawiony
      wstecz o elapsedMin zapewnia identyczny progress w UPDATE
      -> płynna kontynuacja rampy od miejsca resetu.
      Warunek backupAuto>0 chroni przed fałszywym wejściem gdy
      użytkownik celowo ustawił jasność=0.

 NAPRAWA-2 (widget): rampWidgetPwmStart czytał backupBrightness=0
      po resecie -> dashboard pokazywał 0->295 zamiast np. 180->295.
      Naprawiono tym samym odtworzeniem z backupAuto * (1±progress).
      Ten sam warunek backupAuto>0 co w NAPRAWA-1.

 UWAGA-3 (martwy kod): Blok mid-ramp soft-start w sekcji
      !autoRampInitialized jest nieosiągalny gdy rampActive=true,
      ponieważ if(rampActive){...return;} wyprzedza go w każdym
      przypadku. Oznaczono komentarzem. Nie usuwać bez refaktoryzacji.

 NAPRAWA-4 (widget cel): rampWidgetPwmTarget dla rampy południowej
      był hardcodowany na 0 mimo że cel = minLuxPWM (np. 50).
      Dashboard pokazywał pasek dochodzący do 0 zamiast do minLux.
      Naprawiono: dla rampStart==MIDDAY_OFF_LOCAL + minLuxModeEnabled
      ustawiamy _widgetTarget = calculateMinLuxPWM().
      Nie wpływa na sterowanie LED - tylko wizualizacja widgetu.

 NAPRAWA-5 (IR saturacja): rollback luxPokojowy przy saturacji IR
      był nieskuteczny - wygladzonyLuxPokojowy był już zanieczyszczony
      złym odczytem zanim sprawdzono saturację, a przypisanie
      luxPokojowy = wygladzonyLuxPokojowy cofało do tej samej złej
      wartości. Dodatkowo detekcja działała tylko co 30s.
      Naprawa: getLuminosity() przy KAŻDYM odczycie, EMA aktualizowana
      TYLKO gdy brak saturacji -> zanieczyszczony odczyt jest pomijany
      w całości (wygladzony i luxPokojowy bez zmian).

 NAPRAWA-6 (statystyki adaptacji): statystyki redukcji PWM
      (liczbaPomiarow, sredniaRedukcja) były aktualizowane wewnątrz
      bloku if(Komentarze) -> przy wyłączonych logach dashboard
      pokazywał 0% redukcji mimo aktywnej adaptacji.
      Naprawa: statystyki aktualizowane niezależnie od flagi logów.

 NAPRAWA-7 (cloud callback midnight wrap): onBrightnessCompositeChange
      używał (EVENING_OFF_START + fadeMinutes) % 1440 do obliczenia
      outsideLightingWindow. Gdy wynik modulo = 0 (np. 23:00 + 60min),
      warunek nowMin >= 0 był zawsze prawdziwy -> live preview z apki
      nigdy nie działał dla harmonogramów kończących się po północy.
      Naprawiono identycznie jak trybAuto: osobna flaga _evWraps,
      dwie gałęzie logiki (wrap / no-wrap).

 NAPRAWA-8 (JSON reserve): /api/status buduje JSON 81 konkatenacjami
      bez reserve() -> ~10 realokacji heapu przy każdym wywołaniu (co 2s).
      Dodano json.reserve(2048) przed pierwszym +=.

 Wersja firmware w setup() zaktualizowana: v11 -> v16.

 PRZEGLĄD PEŁNY KODU (v16 - wszystkie znalezione bugi naprawione):
      Loop/timing     : OK - double-call guard, fast/slow loop
      Cloud sync      : OK - debounce 60s, RESYNC_GUARD, toggle push
      Cloud callbacks : OK po NAPRAWA-7 - midnight wrap naprawiony
      Energia LittleFS: OK - sameDay/Week/Mon reset, atomowa rotacja
      Pompka          : OK - NTP guard, debounce SSR 5s, przez północ
      Adaptacja       : OK - save=średnia, load=avg*pomiary, walidacja
      Czujniki TSL    : OK po NAPRAWA-5 - IR saturacja naprawiona
      Derating termicz: OK - snapshot z backupAuto, hystereza, filtr
      Emergency 85°C  : OK - EEPROM save, flagi reset, cloud sync
      Diagnostyka     : OK - 31 funkcji DIAG, wszystkie aktywne
      HTTP handlers   : OK - hasParam guards, constrain wszędzie
      JSON heap       : OK po NAPRAWA-8 - reserve(2048) dodane
      EEPROM mapa     : OK - brak nakładek, 146/148 bajtów zajętych

 NAPRAWA-9 (MIN LUX + zanik czujnika TSL): calculateMinLuxPWM() zwracała 0
      gdy czujnik był niedostępny (czujnikPokojowyAktywny=false) -> cel rampy
      południe = 0 -> LED gasły przez całą przerwę mimo włączonego MIN LUX.
      Potwierdzono w logach: "Rampa południe: cel MIN LUX = 0 PWM" przez
      60 minut -> brak odczytów TSL -> czujnik okresowo odpada.

      Trzy miejsca z tym samym guardem naprawione:
      1. calculateMinLuxPWM(): gdy czujnik odpada -> zwróć ostatnie znane PWM
         (minLuxCurrentPWM[0]) lub fallback = luxToPwm(minLuxDayTarget) * 80%.
      2. applyMinLuxMode() call site w loop(): usunięto guard
         (czujnikPokojowyAktywny || czujnikNadWodaAktywny) - funkcja wywołuje
         się zawsze gdy minLuxModeEnabled, wewnętrzna logika obsługuje brak czujnika.
      3. isNightExtended safety net i allFinished blok: j.w.
      4. Cel rampy południe w bloku target: j.w.

 NAPRAWA-10 (logDailySummary energia=0): energyTodayWh był zerowany
      PRZED logPrintf -> podsumowanie o 00:00 zawsze pokazywało "0.00 Wh".
      Naprawiono: log przed zerowaniem.

 NAPRAWA-11 (TRYBAUT fałszywy alarm): rampScheduleActive nie był
      w masce szybkieNormalne TRYBAUT-detektora. Podczas rampy
      harmonogramu (rano/wieczór) fast-loop woła trybAuto co 10ms -
      detektor liczył to jako błąd i logował alarm przez całą rampę.
      Naprawiono: dodano rampScheduleActive do szybkieNormalne.

 Plik: Ryby_LED_v16.ino

 v15 (2026-03-13)
 ─────────────────────────────────────────────────────────
 FIX-TRYBAUT: Eliminacja fałszywych ostrzeżeń [TRYBAUT]
 FIX-BREAK-FLASH: Eliminacja 300ms flash zasilacza przy wejściu w przerwę

 BUG-1: W tej samej iteracji loop() normalna ścieżka (100ms) wywoływała
        trybAuto(), a wewnątrz ustawiała softStartActive=true. Fast-loop
        (10ms) od razu też wywoływał trybAuto(). millis() wewnątrz każdego
        wywołania dawało Δ=0-1ms -> fałszywy alarm [TRYBAUT].
 NAPRAWA-1: Dodano flagę _trybAutoCalledNormal (lokalną dla loop()). Po
        wywołaniu trybAuto() z normalnej ścieżki flaga=true blokuje
        fast-loop w tej samej iteracji. Automatycznie reset przy kolejnym
        obiegu pętli (zmienna lokalna).

 BUG-2: Po restarcie urządzenia w trakcie rampy południe, soft-start
        (30s) obsługiwał przejście do punktu mid-ramp, omijając blok
        allFinished który synchronicznie ustawia minLuxModeActive=true
        (FIX-v13). Gdy nowMin przekroczyło middayOffEnd, trybAuto()
        widział isNightExtended=true, minLuxActive=NIE -> zerował backup
        -> zasilacz 24V wyłączał się na ~300ms -> applyMinLuxMode WŁ ponownie
        -> flash widoczny w logach: sup=OFF -> sup=ON (Δ289ms).
 NAPRAWA-2: W bloku isNightExtended dodano "siatkę bezpieczeństwa":
        gdy przerwa (nie noc) + minLuxModeEnabled + czujnik aktywny ->
        calculateMinLuxPWM() + ustaw minLuxModeActive=true + backup,
        zamiast zerować. Taka sama logika jak FIX-v13 allFinished,
        ale odpala się ZAWSZE niezależnie od ścieżki inicjalizacji.

 Plik: Ryby_LED_v16.ino

 v15 (2026-03-13)
 ─────────────────────────────────────────────────────────
 FIX-MINLUX-WINDOW: MIN LUX aktywny tylko w przerwie i nocy

 BUG: applyMinLuxMode() nie sprawdzała inLightingWindowGlobal.
      Podczas okna świecenia (rano/wieczór) MIN LUX obliczał
      wymagany PWM (np. 202) i nadpisywał nim pełne AUTO PWM
      z harmonogramu (np. 295) -> użytkownik widział 202 zamiast 295.
 NAPRAWA: Dodano guard `if (inLightingWindowGlobal) return;`
      Po wejściu w okno świecenia MIN LUX jest dezaktywowany
      i sterowanie wraca do harmonogramu (backupAuto).
      MIN LUX aktywuje się tylko w przerwie południowej
      (isNightExtended=true, inLightingWindowGlobal=false).

 Plik: Ryby_LED_v14.ino

 v13 (2026-03-12)
 ─────────────────────────────────────────────────────────
 FIX-MIDDAY-TIMING + FIX-MINLUX-RACE (podwójny bug)

 BUG-1: MIDDAY_OFF_LOCAL interpretowany jako KONIEC rampy w C++,
        ale dashboard JS pokazywał go jako START rampy -> niezgodność.
        Ustawienie 11:00 powodowało rampę 10:00-11:00 zamiast 11:00-12:00.
 NAPRAWA-1: C++ zmieniony tak, że MIDDAY_OFF_LOCAL = START rampy
        (jak dashboard). Przerwa startuje o MIDDAY_OFF_LOCAL + fadeMinutes.
        Zmienione: detekcja rampy, middayOffEnd (2x), morningWindowEnd,
        inMiddayRamp, rampStartMin, log-check.

 BUG-2: Po zakończeniu rampy południe minLuxModeActive=false przez 1
        iterację loop(). trybAuto() wywołany przed applyMinLuxMode()
        widział isNightExtended=true + minLuxActive=NIE -> gasił LED.
        Log: "🌙 Przerwa/noc - LED wyłączone (minLuxActive=NIE)"
 NAPRAWA-2: Po allFinished rampy południe synchronicznie ustawia
        minLuxModeActive=true + minLuxCurrentPWM + backup z calculateMinLuxPWM().

 Plik: Ryby_LED_v13.ino

 v12 (2026-03-12)
 ─────────────────────────────────────────────────────────
 ENERGIA Wh zamiast kWh: Zmieniono wyświetlanie energii
   z kWh (z 2 miejscami po przecinku) na Wh (1 miejsce),
   co daje bardziej czytelne wartości przy małym zużyciu.
   Dotyczy: zakładki Dziś / Tydzień / Miesiąc, opisy
   wykresów, tooltipy słupków, etykiety w wierszach.
   Koszty (zł) i kalkulacja ceny kWh - bez zmian.
 Plik: Ryby_LED_v12.ino

 v11 (2026-03-12)
 ─────────────────────────────────────────────────────────
 FIX-TRANSITION-INIT: Flash po restarcie w oknie świecenia
   OBJAW : Soft-start kończył się OK (0->295), a chwilę
           potem LEDy natychmiast gasły (FLASH#3, delta -295).
   PRZYCZYNA-A: Pętla init transition inicjalizowała tylko
           kanały pasujące do 'sections'. Pozostałe 5
           kanałów zostawało z wartością 0. Cloud pushował
           brightness=0 po reconnect (~38s) -> transition
           0->0 kończyła się natychmiast i nadpisywała
           backup=[0,...] niwecząc efekt soft-startu.
   PRZYCZYNA-B: CLOUD_RESYNC_GUARD_MS=15s za krótki -
           cloud pushuje zmienne po ~38s od reconnect.
   NAPRAWA-A: Wszystkie 5 kanałów inicjalizowane przed
           pętlą sections (current=target=backup). Transition
           pomijana gdy anyChange=false (brak zmiany PWM).
   NAPRAWA-B: CLOUD_RESYNC_GUARD_MS 15s -> 60s.
   NAPRAWA-C: Osobny timer rampStartTimeSoftStart dla
           soft-start (nie współdzielony z transition).
 Plik: Ryby_LED_v11.ino

 v10 (2026-03-12)
 ─────────────────────────────────────────────────────────
 FIX FLASH-MORNING: Błysk LEDów przy porannej rampie
   OBJAW : Zamiast płynnego wzrostu 0->296 przez 60 min,
           LEDy zapalały się natychmiast na pełną jasność
           (FLASH#4, delta +296 na wszystkich kanałach).
   PRZYCZYNA: rampDownStartValue[] jest static - wartość
           zapisana przez wieczorną rampę minLux (np. 296)
           przeżywała noc. Blok init rampy UP nie zerował
           tej zmiennej. Blok UPDATE widział startVal=296,
           target=296 -> expectedPwm=296 dla każdego
           progress -> natychmiastowy skok zamiast rampy.
   NAPRAWA: Dodano rampDownStartValue[i] = 0 w ścieżce
           normalnej inicjalizacji rampy UP (linia ~9079).
           Ścieżka minLux-wieczór (continue przed fixem)
           i rampa DOWN (blok else) są nienaruszone.
 Plik: Ryby_LED_v10.ino

 v9
 ─────────────────────────────────────────────────────────
 FIX-PANEL-BREAK  : Ochrona EEPROM przed panelem WWW
                    podczas przerwy/nocy (nie nadpisuj
                    backupAuto poza oknem świecenia).
 FIX-RECONNECT-DEBOUNCE : Wyciszono spam logu przy
                    niestabilnym WiFi/reconnect chmury.
 FIX-DS18B20-BLOCK: Naprawiono blokadę pętli głównej
                    przez sensors.requestTemperatures()
                    gdy czujniki DS18B20 odłączone.
 FIX-DS18B20-LOGSPAM: Eskalacja throttle logu błędów
                    temperatury.

