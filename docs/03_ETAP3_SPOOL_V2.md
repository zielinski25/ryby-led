# ETAP 3 — Trwała telemetria: Spool V2 + TelemetryRing + sessionNonce

Wersja: **v4.2.0+build.264** · Status: kod gotowy, testy hostowe zaliczone,
**DoD na płytce do wykonania** · Plan: `docs/02_PLAN_UPGRADE_ryby_led.md`

---

## 1. Co zostało zrobione

| Element | Gdzie | Opis |
|---|---|---|
| Próbka domenowa (48 B) | `firmware/src/telemetry_spool.h` | temp. wody/płyty ×2, lux pokój/woda, PWM ×5, moc, RSSI, flagi trybu, `unixTs`, `uptimeS`, `sessionNonce`, `sampleSeq` |
| TelemetryRing | `telemetry_spool.h/.cpp` | SPSC, 288 próbek (24 h), bufor w PSRAM (13,8 KB), head/tail acquire/release |
| Persistent Spool V2 | `telemetry_spool.cpp` | strona = HEADER 64 B + N × (48 B + CRC32) + FOOTER 64 B; **footer = commit** |
| Weryfikacja przed publikacją | `writePageFromRing()` → `scanPage()` | CRC nagłówka, stopki, payloadu i każdego rekordu — dopiero potem `rename` `.part` → `.spl` |
| sessionNonce | `makeSessionNonce()` | losowany (`esp_random`) przy każdym starcie |
| Klucze RTDB | `sampleKey()` | `<nonce 16 hex>_<sampleSeq>` — unikalne między restartami, ponowienie idempotentne |
| Producent | `saveHistoryPoint()` → `tsProduceFromHistory()` | co 5 min, Core 0, te same wartości co wiersz CSV |
| Spill | `tsSpillTick()` (tgTaskFn) | offline ≥ 60 s **albo** ring ≥ 6 próbek (30 min) |
| Replay | `fbAsyncStartTelemetryReplay()` / `fbAsyncHandleTelemetryResult()` | `PATCH /aquarium/telemetry/v1`, max 12 rekordów/żądanie, FIFO, najniższy priorytet w schedulerze FB |
| Diagnostyka | `GET /api/telemetry/status` | ring, liczniki, strony na FS, sessionNonce, wolny PSRAM |
| Testy hostowe | `firmware/tests/host/` | 200 testów logiki (ASan + UBSan) + kompilacja bloku z `Ryby_LED_fi_S3.cpp` |

**Logika sterowania nietknięta:** Ramp Arbiter, MIN LUX, PWM, Core 1 — zmiany
tylko w producencie (`saveHistoryPoint`), w `tgTaskFn` (spill) i w schedulerze FB (replay).

---

## 2. Format strony (ABI zamrożone)

```
[ HEADER 64 B ]  magic "SPL2" · formatVersion · pageSeq · firstSampleSeq · recordBytes=52 · capacity · headerCrc32
[ REKORD 52 B ]  próbka 48 B + crc32(próbka)        × recordCount
[ FOOTER 64 B ]  magic · pageSeq · first/lastSampleSeq · recordCount · payloadCrc32 · commitMarker "COMM" · footerCrc32
```

- Brak footera = strona **otwarta / przerwana** → nie jest wysyłana (kwarantanna).
- Rekord z błędnym CRC → **pomijany** przy replayu, liczony w `corruptRecords`.
- Plik `.part` po restarcie → **usuwany** (`partDiscarded`).
- Wszystkie `static_assert` pilnują rozmiarów i offsetów (48 / 64 / 64 B).

## 3. Polityka

| Parametr | Wartość | Uzasadnienie |
|---|---|---|
| Okres próbek | 5 min | ten sam co wpis historii CSV |
| Pojemność ringu | 288 próbek (24 h) | 13,8 KB PSRAM |
| Watermark spill | 6 próbek (30 min) | maksymalne okno utraty RAM przy zaniku zasilania (online) |
| Spill przy offline | po 60 s bez transportu | trwałość na wypadek zaniku zasilania w trakcie awarii sieci |
| Rekordów na stronę | do 288 | jedna strona = max doba |
| Partia replayu | 12 rekordów | ~2,3 KB JSON w PSRAM, krótkie żądania |
| Min. wolne FS | 64 KB | poniżej: kasowanie najstarszej strony (`quotaDrops`) |
| Maks. stron na FS | 240 | ~20 dni przy spill dobowym |
| Ponowienie po błędzie | 5 s | bez zalewania sieci |

## 4. Testy hostowe (wykonane)

```bash
bash firmware/tests/host/run_tests.sh
```

Wynik przy wydaniu: **200 PASS / 0 FAIL**, bez wycieków (LeakSanitizer), bez ostrzeżeń
(`-Wall -Wextra`); blok integracyjny i endpoint kompilują się (`-fsyntax-only`) przeciwko
atrapom globali Ryby i API FirebaseClient v2.2.13.

Co testy **potwierdzają**: CRC32 (wektor), ring SPSC (pełny, wrap, commit), klucze i format
JSON (dokładny ciąg), zapis + commit + weryfikacja strony, **uszkodzony bajt w rekordzie**
(wykryty, rekord pominięty, reszta wysłana), **ucięta strona** (kwarantanna, nie idzie na sieć),
replay w partiach z ACK i kasowaniem, FIFO po `pageSeq` (nie leksykograficznie), sprzątanie
`.part`, polityka spill (watermark / offline / cooldown / brak miejsca / limit 288).

Czego testy **nie potwierdzają** (wymaga płytki): ESP32 Arduino/LittleFS, realne zasilanie,
WDT, PSRAM, sam Firebase. Do tego służy sekcja 5.

## 5. DoD na płytce — procedura (w domu)

**Kolejność wgrywania:** najpierw **4.1.1** (sekrety, już zbudowana), zweryfikować
połączenie; dopiero potem **4.2.0** z tego checkoutu.

### 5.1 Pomiar PSRAM (budżet)
```powershell
Invoke-RestMethod http://IP-KONTROLERA:8080/api/telemetry/status | ConvertTo-Json -Depth 5
```
Zapisz `psramFree` **przed** i **po** pierwszym starcie v4.2.0 (różnica ≈ 20 KB:
ring 13,8 KB + bufor batcha 6 KB). W logu: `tag=TSPOOL akcja=ready ... partDiscarded=0`.

### 5.2 Spill i wysyłka (zwykła praca)
1. Po starcie: `ring.size` rośnie co 5 min; po ~30 min `fs.pages` ≥ 1, `counters.pagesCreated` ≥ 1.
2. Po chwili `counters.replayedRecords` rośnie, `fs.pages` wraca do 0.
3. Firebase Console → RTDB → `/aquarium/telemetry/v1` — widoczne klucze `XXXXXXXXXXXXXXXX_N`.

### 5.3 DoD: zanik zasilania w trakcie wysyłki (główny test)
1. Odłącz router (WAN) na ok. 2–3 min → po 60 s `fs.pages` rośnie (spill offline).
2. Podłącz router z powrotem; w trakcie replayu (`replayedRecords` jeszcze nie pełne) **wyjmij USB na 5 min**.
3. Włóż USB. Oczekiwane w logu: `akcja=ready ... partDiscarded=N` (N = 0 albo 1 — sprzątnięty `.part`).
4. Po kilku minutach: `fs.pages` = 0, `replayedRecords` = liczba zapisanych próbek, w RTDB komplet kluczy
   (**bez duplikatów** — klucze są deterministyczne, duplikat nadpisuje ten sam węzeł).

**Zaliczone, gdy:** po powrocie zasilania strony są dosyłane (`fs.pages` wraca do 0, `pagesDeleted` rośnie
o liczbę wysłanych stron), a w RTDB nie ma braków za okresy zapisane na FS. Liczniki `quotaDrops` i
`badPagesRemoved` powinny pozostać 0 w tym teście.

### 5.4 Test negatywny CRC (opcjonalny, na płytce)
Ręcznie uszkodzić stronę na FS jest trudne — pokrywa to test hostowy `corruption_and_negative`.
Na płytce wystarczy: `corruptRecords` = 0 w normalnej pracy (każdy wzrost to sygnał do zbadania).

## 6. Rollback
Jeśli 4.2.0 zachowuje się źle: wgraj ponownie **4.1.1** z USB (lub OTA z release `v4.1.1`).
Strony `/spool` na FS nie przeszkadzają wersji 4.1.1 (nie są czytane). Firebase: węzeł
`/aquarium/telemetry` można usunąć bez wpływu na sterowanie.

## 7. Znane ograniczenia (świadome)
- **Okno utraty przy zaniku zasilania (online):** do 30 min próbek z RAM. Zapisane strony są bezpieczne.
- **Historia długa** (`/api/history/long`, wykresy tygodniowe): zrobiona w 4.5.0, patrz `docs/05_ETAP3_HISTORIA_DLUGA.md`.
- **Retencja RTDB:** `/aquarium/telemetry` rośnie ok. 55 KB/dobę (288 × ~190 B) — czyszczenie starych węzłów do ustalenia.
- **Opóźnienie wysyłki:** online dane idą partiami przy spill (≈ co 30 min), nie co 5 min.
- Ring i strony są **per sesja nonce** — rekordy z różnych startów są rozróżnialne, ale nie ma jeszcze
  „ciągłości” między sesjami po stronie serwera (do zrobienia w panelu).
