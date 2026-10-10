# Checklista wdrożenia w domu — Etapy 1–6 (build 4.7.2+build.272)

Jedna kolejność kroków na płytce. Szczegóły testów są w dokumentach etapowych;
tu jest tylko co robić i w jakiej kolejności. Stan repo i testy hostowe:
`firmware/tests/host/run_tests.sh` (EXIT 0 przed wgraniem).

Zasada: **akwarium nie zostaje bez światła.** Przed każdym flashem zapisz
działającą konfigurację (zrzut `/api/status`, karta Telegram, lokalizacja) i miej
pod ręką poprzedni build (release `4.6.0` / tag z `CHANGELOG.md`) na wypadek rollbacku.

---

## Krok 0 — rotacja sekretów (przed flashem)

Patrz `docs/02`, PILNE, oraz `CHANGELOG.md` wpis 4.7.1.

- [ ] Firebase Console → Project settings → Service accounts → Database secrets: odwołaj stary sekret, wygeneruj nowy.
- [ ] @BotFather → `/revoke` dla bota Telegrama, nowy token.
- [ ] Wybierz nowe hasło OTA i CMD_TOKEN.
- [ ] Lokalny `firmware/src/secrets.h`: `FIREBASE_SECRET`, `CMD_TOKEN`, **`OTA_PASSWORD`** (nowy wpis w 4.7.1, bez niego kompilacja zatrzyma się na `#error`), opcjonalnie `TG_DEFAULT_BOT_TOKEN` / `TG_DEFAULT_CHAT_ID`.
- [ ] Zmienna środowiskowa dla OTA (PowerShell, w tym samym oknie przed `pio`):
      `$env:RYBY_OTA_PASSWORD = "to-samo-haslo-co-OTA_PASSWORD"`
- [ ] Nowy CMD_TOKEN wpisz w panelu (pole token).
- [ ] `python firmware/tests/host/check_secrets.py` → 0 trafień.

## Krok 1 — pierwszy flash przez USB (4.7.2)

- [ ] `pio run -e esp32-s3-n16r8` → sukces; `firmware.bin` < 3 MB (bramka `build_gate.ps1`).
- [ ] Wgraj USB, otwórz log: `lvl=INFO` przy starcie, wersja `v4.7.1+build.271`.
- [ ] Firebase łączy się z **nowym** sekretem (log bez błędów auth). Dopiero teraz odwołaj stary sekret w konsoli.
- [ ] Telegram: po czystym LittleFS bez `TG_DEFAULT_*` TG jest wyłączony — to oczekiwane; po wpisaniu w panelu działa.
- [ ] Zapisz `psramFree` (`/api/status`) jako punkt odniesienia dla Etapu 3.

## Krok 2 — OTA (Etap 1)

- [ ] `pio run -e esp32-s3-n16r8-ota --target upload` (hasło z `RYBY_OTA_PASSWORD`) → upload przechodzi, ESP restartuje.
- [ ] Bez zmiennej środowiskowej upload powinien się nie udać (błąd auth) — to test negatywny.
- [ ] Panel → karta „Aktualizacja firmware (OTA & GitHub)”: wersja zgodna z release.

## Krok 3 — trwała telemetria (Etap 3)

Procedura: `docs/03_ETAP3_SPOOL_V2.md`, sekcja 5 (PSRAM, spill, zanik zasilania, CRC).

- [ ] 5.1 PSRAM przed/po (`/api/telemetry/status`).
- [ ] 5.2 spill i wysyłka w zwykłej pracy.
- [ ] 5.3 **główny test:** wyjęcie USB na 5 min w trakcie replayu → brak braków i duplikatów w RTDB.
- [ ] 5.4 `corruptRecords` = 0 w normalnej pracy.

## Krok 4 — panel: CORS, OTA, tryb zdalny (Etap 4)

Procedura poniżej (§ „Test panelu”).

## Krok 5 — log krytyczny i Telegram (Etap 5)

Procedura: `docs/04_ETAP5_LOG_KRYTYCZNY.md`, sekcja 6.

- [ ] `/log_krytyczny` z czatu wysyła plik jako dokument.
- [ ] `/update` uruchamia OTA z czatu (po wcześniejszym sprawdzeniu release).
- [ ] Menu Telegrama: jedno kliknięcie do `/status`, `/temp`, `/lux`, `/harmonogram`, `/update`.

## Krok 6 — astronomia, poranek, RTC (Etap 6)

Procedura: `docs/06_ETAP6_ASTRO_RTC.md`, sekcja 5. Najważniejsze:

- [ ] Lokalizacja w panelu → zachód zgodny z timeanddate ±2 min.
- [ ] Tryb poranka: stała / świt / wschód + przesunięcie; „Start poranka dziś” zgodny z kartą „Dzień”.
- [ ] **RTC tylko jeśli masz moduł.** Najpierw odczytaj nadruk (DS1307 czy DS3231). Potem procedura z `docs/06`, sekcja 5, pkt RTC 1–4.
- [ ] Brak modułu = brak zmian w działaniu (domyślnie wyłączony).

---

## Aplikacja na telefon (4.7.2) — procedura

Plik: `panel/mobile/index.html` (zdalnie przez Firebase; wymaga HTTPS do instalacji).

- [ ] Otwórz adres aplikacji na telefonie (LTE, bez WiFi domowego). Ustawienia → wpisz Database Secret i CMD_TOKEN → „Sprawdź połączenie”.
- [ ] Główna: pill ONLINE, temperatury i lux zgodne z panelem WWW.
- [ ] Włącz / Wyłącz / AUTO / MANUAL: toast „Wysłano …”, zmiana w ESP w ciągu ok. 14 s (log `tag=FB-QUEUE`).
- [ ] Światło: suwaki → „Wyślij PWM”; „Zapisz jako tryb AUTO” pyta o potwierdzenie.
- [ ] Ustawienia → „Usuń dane z tej przeglądarki” czyści sekrety (testuj na osobnej przeglądarce).
- [ ] Dodaj do ekranu głównego (PWA). Przy zmianie wersji przeładuj aplikację (service worker pobiera nową powłokę).

Hosting: wybór miejsca publikacji `panel/mobile/` to decyzja właściciela; repo nie zawiera sekretów, więc publiczny hosting jest technicznie możliwy.

## Test panelu (Etap 4) — procedura

Panel: `panel/akwarium-firebase-panel-v14.html` (LAN albo zdalnie przez Firebase).

### 4.1 CORS z innego originu (LAN)

1. Otwórz panel z innego adresu niż ESP (np. z pliku lokalnego `file://` albo innego hosta w LAN).
2. Konsola przeglądarki (F12): brak błędów CORS przy `GET /api/status`, `POST /api/location`, `POST /api/rtc`.
3. Skan sieci WiFi w panelu działa z tego originu.

### 4.2 Karty w panelu

- [ ] Karta „Aktualizacja firmware (OTA & GitHub)” pokazuje wersję i listę release.
- [ ] Karta „Log krytyczny” (Etap 5) pokazuje status i pobiera plik.
- [ ] Karta Telegram: zapis tokenu działa; `GET /api/telegram/status` zwraca tylko `tokenPrefix` (6 znaków + "...").
- [ ] Zapis AUTO z panelu (przycisk „ZAPISZ DO AUTO”, zdalnie): toast „Wysłano: autosave …”, w logu `tag=APP-CMD cmd=AUTO_SAVE_PWM`; po restarcie ESP wartości AUTO są takie jak wysłane.
- [ ] Symulacja LUX z panelu (stała i AUTO-sinusoida): w logu `tag=SIM-CFG state=APPLIED`; po restarcie `simEnabled=false` (symulacja nie jest trwała). **Pamiętaj wyłączyć po testach.**
- [ ] Zakres Min LUX w panelu: 500–8000 (jak firmware); wartość poniżej 500 nie jest wysyłana.

### 4.3 Tryb zdalny poza LAN (DoD Etapu 4)

1. Telefon na LTE (wyłącz WiFi), panel zdalnie.
2. Zmień tryb (Auto/Manual) lub wyślij „LED 100%” → w logu ESP: `tag=FB-QUEUE ... cmd=...` w ciągu kilku sekund. Wpis w Firebase pod `/aquarium/commands` znika po wykonaniu.
3. Skan sieci i OTA z tego połączenia — skan działa tylko w LAN; poza LAN panel powinien to pokazać jako zrozumiały komunikat, a nie zawieszenie.

**Zaliczone, gdy:** 4.1 bez błędów CORS, 4.2 wszystkie karty działają, 4.3 sterowanie z LTE przechodzi przez Firebase.

**Naprawione w 4.7.1:** komendy zdalne panelu v14 trafiają do kolejki `/aquarium/commands` (wcześniej `/aquarium/cmd.json`, którego firmware nie czytał). **Naprawione w 4.7.2:** zapis AUTO zdalnie (`autosave`), parser symulacji LUX (`luxSim`), zakres Min LUX. Panel v15 nie istnieje — właściciel potwierdził, że źródłem jest panel v14 (`docs/02`, Etap 4).

---

## Po wdrożeniu

- [ ] Zapisz wyniki każdego kroku (data, wersja, kluczowe liczby) w `docs/` albo `CHANGELOG.md`.
- [ ] Zaktualizuj status etapów w tabeli `docs/02` (STATUS REALIZACJI).
- [ ] Gdy któryś krok nie przechodzi: rollback wg sekcji „Rollback” w odpowiednim dokumencie etapowym.
