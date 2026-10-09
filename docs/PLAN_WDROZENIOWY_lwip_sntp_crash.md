# Plan wdrożeniowy — crash `PANIC/CRASH` w lwIP (`sntp`/`udp_input` assert)

> **STATUS: ✅ ZAMKNIĘTE (2026-08-08)** — przyczyna znaleziona i naprawiona
> precyzyjniej niż w pierwotnym planie. Dotyczy JEDNEGO, rzadkiego typu
> crasha (`PANIC/CRASH`, nie `WDT_TASK`) — osobny wątek od głównej serii
> diagnostycznej WDT (`plan_poprawek_WDT.md`).

## Rozwiązanie faktycznie wdrożone

Zamiast mojej propozycji (upgrade frameworka / ręczna serializacja SNTP),
znaleziono **precyzyjniejszy, węższy fix**: `CONFIG_LWIP_TCPIP_CORE_LOCKING`
i `CONFIG_LWIP_CHECK_THREAD_SAFETY` faktycznie są włączone (domyślnie, przez
nieużywaną obsługę Matter) — potwierdzone. Ale prawdziwym wyzwalaczem był
**`CONFIG_LWIP_DHCP_GET_NTP_SRV=y`** — stos sieciowy autonomicznie
restartował klienta SNTP za każdym razem, gdy DHCP podał serwer NTP (opcja
DHCP 42), z kontekstu poza kontrolą aplikacji, kolidując z DNS/TLS w
`tgTask`. Projekt i tak jawnie podaje serwery NTP w `configTzTime()`
(pool.ntp.org, time.nist.gov), więc serwer z DHCP nigdy nie był używany —
wyłączenie `CONFIG_LWIP_DHCP_GET_NTP_SRV` usuwa wyzwalacz bez żadnej zmiany
zachowania aplikacji.

**Wdrożone przez `custom_sdkconfig` w `platformio.ini`:**
```
custom_sdkconfig =
    CONFIG_SPI_FLASH_YIELD_DURING_ERASE=y
    CONFIG_LWIP_DHCP_GET_NTP_SRV=n
```

**Przy okazji: `CONFIG_SPI_FLASH_YIELD_DURING_ERASE=y`** to dodatkowa,
bardzo wartościowa zmiana — to **oficjalnie rekomendowana przez ESP-IDF**
metoda łagodzenia INNEGO przełomowego znaleziska tego projektu (zamrożenie
obu rdzeni przy zapisie flash, patrz `audyt_pozostalych_obszarow.md` sekcja
0) — pozwala schedulerowi przełączać zadania w trakcie kasowania flash,
zamiast trzymać cały system w bezruchu przez cały czas trwania operacji.

**Zastrzeżenie sprawdzone i wykluczone:** issue esp-idf #9784 ostrzega, że
`YIELD_DURING_ERASE=y` może pogorszyć sytuację, jeśli jednocześnie włączone
jest `CONFIG_SPI_FLASH_SHARE_SPI1_BUS=y` (typowe przy współdzieleniu SPI1 z
kartą SD). Sprawdzono `sdkconfig.defaults`/`sdkconfig.esp32-s3-n16r8*` —
`SPI_FLASH_SHARE_SPI1_BUS` nigdzie nie występuje (nieustawione) — ryzykowna
kombinacja nie ma zastosowania w tym projekcie.

**Weryfikacja propagacji do faktycznie kompilowanego configu (ważna
lekcja):** pierwsza próba pokazała, że `sdkconfig.defaults` miał poprawny
fix, ale per-środowiskowe, faktycznie kompilowane pliki
(`sdkconfig.esp32-s3-n16r8`, `sdkconfig.esp32-s3-n16r8-ota`) **wciąż miały
starą wartość** `CONFIG_LWIP_DHCP_GET_NTP_SRV=y` — bo zmiana w
`custom_sdkconfig` wymaga **pełnego, czystego rebuildu**
(`.pio/build/<env>` usunięte + build od zera), żeby faktycznie się
przepisała. Po wykonaniu czystego rebuildu potwierdzono:
`sdkconfig.esp32-s3-n16r8-ota` → `# CONFIG_LWIP_DHCP_GET_NTP_SRV is not set`
✅ — fix faktycznie trafił do skompilowanego firmware.

## Aktualizacja: potwierdzenie researchem + utwardzenie kodu (2026-08-11)

Dodatkowy research (PL/EN/DE/RU) **potwierdził diagnozę i wybór fixu**, a przy
okazji ujawnił, że **Krok 2 pierwotnego planu (upgrade frameworka) by nie
zadziałał** dla tego konkretnego wariantu crasha:

- Assert "Required to lock TCPIP core functionality!" ma dwa różne warianty
  źródłowe:
  1. **Wariant "aplikacyjny"** — `configTzTime()`/`sntp_setoperatingmode()`/
     `sntp_stop()` wołane wprost z kodu użytkownika (np. dwa razy pod rząd —
     arduino-esp32 issue #10675). Ten wariant **został naprawiony** przez PR
     #10529 + #10725 ("fix(sntp): Lock / Unlock LWIP if
     CONFIG_LWIP_TCPIP_CORE_LOCKING is set", mathieucarbou), scalone do
     arduino-esp32 3.1.0.
  2. **Wariant "DHCP-wyzwalany"** (`sntp_servermode_dhcp`, sntp.c:770) — czyli
     dokładnie nasz przypadek z opcją DHCP 42. Ma **osobne** zgłoszenie:
     esphome/issues#6591 (grudzień 2024), zamknięte jako "stale" — **bez
     żadnego fixu w kodzie**. Niezależne potwierdzenie z niepowiązanego
     zgłoszenia #10252 (Ethernet+NTP): komentarz w przykładowym kodzie
     ostrzega wprost, że włączenie `esp_sntp_servermode_dhcp(1)` tworzy pętlę
     awarii ("crash loop").

**Wniosek:** PR #10529/#10725 (a więc i sam upgrade `platform-espressif32` do
nowszej wersji) łata tylko wariant 1. Nasz przypadek to wariant 2, dla którego
**nie ma scalonego fixu upstream** — `CONFIG_LWIP_DHCP_GET_NTP_SRV=n` jest więc
nie tylko "węższym, precyzyjniejszym" fixem (jak zapisano wyżej), ale
prawdopodobnie **jedynym realnie dostępnym**. Kolejny upgrade frameworka sam
w sobie NIE naprawi tej ścieżki, dopóki ktoś nie doda analogicznego
`LOCK_TCPIP_CORE()` wewnątrz callbacku DHCP w esp-idf/lwIP.

**Zmiana wprowadzona w kodzie (v228, `Ryby_LED_fi_S3.cpp`):** ponieważ fix
istnieje wyłącznie jako flaga configu (`custom_sdkconfig` w `platformio.ini`),
a taki fix można łatwo przypadkowo cofnąć (merge, aktualizacja platformy,
edycja bez pełnego czystego rebuildu) bez żadnego objawu w samym kodzie,
dodano w `setup()`, tuż przed `configTzTime()`:
- kanarek kompilacyjny `#ifdef CONFIG_LWIP_DHCP_GET_NTP_SRV` → `#warning` w
  logu builda PlatformIO, jeśli flaga kiedykolwiek wróci do `y`;
- log startowy `tag=NTP` w `aquarium_log.txt` potwierdzający aktualny stan
  flagi — obserwowalny w terenie bez ręcznego sprawdzania sdkconfig.

Przy tej okazji potwierdzono też (grep całego pliku): `configTzTime()` jest
wołane dokładnie RAZ w całym programie — wzorzec bezpieczny względem wariantu
1 (#10675), więc logika synchronizacji czasu nie wymagała żadnej zmiany.
`platformio.ini` pozostaje bez zmian — konfiguracja tam już jest poprawna
(potwierdzone wcześniej w skompilowanym `sdkconfig.esp32-s3-n16r8-ota`).

## Do obserwacji

- [ ] Obserwować kolejne logi pod kątem powtórki sygnatury `PANIC/CRASH` +
      coredump z `sntp`/`udp_input`/`assert` w backtrace, oraz nową linię
      startową `tag=NTP msg="CONFIG_LWIP_DHCP_GET_NTP_SRV=..."` (v228) — jej
      treść `WLACZONE` zamiast `wylaczone (fix aktywny)` oznacza regresję
      configu. Jeśli mimo `wylaczone (fix aktywny)` crash się powtórzy —
      **UWAGA (aktualizacja 2026-08-11):** upgrade frameworka / PR
      #10529+#10725 dotyczy innego wariantu (`configTzTime()`/
      `sntp_setoperatingmode()` wołane z kodu aplikacji) i **nie naprawia**
      wariantu DHCP-wyzwalanego (`sntp_servermode_dhcp`) — dla tego
      ostatniego nie ma znanego fixu upstream (esphome/issues#6591, "stale").
      W takim przypadku szukać dalej niż w oryginalnym Kroku 2 — np. czy coś
      w projekcie jednak jeszcze wywołuje `esp_sntp_servermode_dhcp(true)`
      albo czy DHCP serwer w sieci lokalnej zaczął podawać opcję 42 mimo
      wyłączonej flagi (co nie powinno być możliwe przy `n`, ale warte
      sprawdzenia jako pierwszy trop).
- [ ] Obserwować, czy `CONFIG_SPI_FLASH_YIELD_DURING_ERASE=y` realnie
      zmniejsza częstość/dotkliwość crashy powiązanych z zapisem flash
      (checkpoint `pompa`/`events`/`DAILY-SUMMARY`/`histFlag`/`wsFlush`
      itd. — patrz `plan_poprawek_WDT.md` i `audyt_pozostalych_obszarow.md`).

### Log kontrolny po wdrożeniu (aktualizowane na bieżąco)

**Stan na 2026-08-10:** ponad **dobę** czystej pracy od restartu OTA
(17:30:17), **zero nawrotów** sygnatury lwIP/SNTP. W tym czasie wystąpił
jeden nowy `WDT_TASK` (23:29:00, checkpoint `L:wsCleanup`) — ale to
**dobrze już zrozumiana, inna sygnatura** (`task=IDLE0`, `pc=0x4037EFCF`,
normalny backtrace WFI — ofiara zamrożenia flash, patrz `plan_poprawek_WDT.md`
i sekcja 0 `audyt_pozostalych_obszarow.md`), nie powrót problemu lwIP/SNTP.
Rosnąca, coraz bardziej wiarygodna próbka potwierdzająca skuteczność fixu
`CONFIG_LWIP_DHCP_GET_NTP_SRV=n`. Dalej obserwować — pierwotny crash
zdarzał się rzadko (raz na tygodnie), więc potrzeba więcej czasu, żeby
uznać to za ostatecznie zamknięte z pełną pewnością.

---

## Materiały źródłowe (do wglądu, jeśli problem wróci)

## 1. Co znaleźliśmy — dowód z urządzenia

Coredump z `aquarium_log__6_.txt`/`log_combined__4_.txt` (crash w trakcie
odświeżania tokena Firebase), zdekodowany `addr2line`:

```
panic_abort → esp_system_abort → __assert_func (assert.c:81)
  → udp_input (lwIP udp.c:263)
  → sntp_stop → sntp_setoperatingmode → sntp_setserver
  → dns_send → dns_alloc_pcb → dns_enqueue → dns_gethostbyname_addrtype
  → NetworkManager::hostByName()
  → NetworkClientSecure::connect()/available()
  → firebase_ns::FirebaseApp::parseToken()
  → conn_handler::connect() → AsyncClientClass::send()/process()
```

## 2. Co potwierdza research — to jest znany, nazwany bug

**GitHub `espressif/arduino-esp32` issue #10526** ("Crash with error
message: 'Required to lock TCPIP core functionality'", zgłoszone
2024-10-25) — backtrace z tego zgłoszenia jest niemal identyczny:

```
assert failed: sntp_setoperatingmode .../sntp.c:728
  (Required to lock TCPIP core functionality!)
```

**Mechanizm (potwierdzony w treści zgłoszenia i przez dokumentację
ESP-IDF):** lwIP wymaga, żeby pewne funkcje (m.in. `sntp_setoperatingmode`,
`tcp_alloc` — widziane w DRUGIM przykładzie z tego samego issue) były
wołane z **zablokowanym rdzeniem TCP/IP** (`LOCK_TCPIP_CORE`), czyli z
właściwego kontekstu taska. Wywołanie z niewłaściwego taska bez blokady →
lwIP **celowo** wykrywa to i ubija proces asercją (`assert()`), zamiast
pozwolić na cichy, trudniejszy do zdiagnozowania race condition.

**WAŻNY WARUNEK — to nie zawsze aktywne:** ten konkretny bug ujawnia się
**tylko gdy `CONFIG_LWIP_TCPIP_CORE_LOCKING` jest włączone** w konfiguracji
ESP-IDF (`sdkconfig`). Zgłaszający napisał wprost: *"following activation
of `LWIP_TCPIP_CORE_LOCKING` and `LWIP_CHECK_THREAD_SAFETY` **for
Matter**"* — czyli u niego ta opcja włączyła się przy okazji dodania
obsługi Matter (protokół smart-home). **Trzeba sprawdzić, czy w naszym
projekcie ta opcja jest w ogóle włączona** (patrz krok 3 poniżej) — jeśli
nie, ten dokładny mechanizm nie ma zastosowania i crash ma inną przyczynę
mimo podobnego wyglądu backtrace'u.

**Status naprawy w arduino-esp32:**
- PR **#10529** (merged 2024-10-29) — pierwsza łatka.
- PR **#10725** (merged 2024-12-13) — **"Completes old PR #10529"** — czyli
  pierwsza łatka nie była kompletna, druga ją domyka.
- Oba scalone do gałęzi `master` — trzeba sprawdzić, w którym wydaniu
  (release) `arduino-esp32` / `platform-espressif32` po raz pierwszy się
  pojawiły, i czy nasz projekt jest na wersji równej lub nowszej.

---

## 3. Kroki wdrożeniowe (w kolejności)

### Krok 1 — Sprawdź, czy `CONFIG_LWIP_TCPIP_CORE_LOCKING` jest włączone
- [ ] Sprawdź `platformio.ini` / `sdkconfig` / `sdkconfig.defaults` projektu
      pod kątem `CONFIG_LWIP_TCPIP_CORE_LOCKING=y` lub
      `CONFIG_LWIP_CHECK_THREAD_SAFETY=y`.
- [ ] Sprawdź, czy projekt używa czegokolwiek z ekosystemu Matter/Thread/
      Zigbee (nawet pośrednio przez zależność biblioteki) — to typowy
      sposób, w jaki ta opcja włącza się "przy okazji", bez świadomej
      decyzji.
- [ ] **Jeśli opcja jest wyłączona:** ten dokładny mechanizm z #10526 nie
      powinien mieć zastosowania — mimo podobieństwa backtrace'u, crash ma
      inne źródło. Wróć do ogólnej hipotezy "race condition przy
      współbieżnym DNS z kilku kontekstów" i szukaj dalej (np. czy jest
      jakiś INNY mechanizm blokady w używanej wersji lwIP wymuszający tę
      samą asercję niezależnie od tej opcji).
- [ ] **Jeśli opcja jest włączona:** przejdź do kroku 2.

### Krok 2 — Sprawdź wersję `arduino-esp32`/`platform-espressif32`
- [ ] Sprawdź aktualną wersję frameworka w `platformio.ini`
      (`platform = espressif32@X.X.X` lub `framework = arduino` + wersja
      core'u).
- [ ] Sprawdź, czy commit z PR #10725 (2024-12-13) jest zawarty w tej
      wersji — jeśli tak, **fix już jest w kodzie bazowym, nic więcej nie
      trzeba robić** poza upewnieniem się, że build faktycznie z niego
      korzysta (świeży `pio run -t clean` + rebuild po ewentualnej
      aktualizacji).
- [ ] Jeśli wersja jest starsza — to najprostsza, najbezpieczniejsza
      naprawa: **podbić wersję `platform-espressif32` w `platformio.ini`**
      do najnowszej stabilnej, przetestować pełny cykl (build + flash +
      obserwacja logów), potwierdzić że reszta funkcjonalności (WiFi,
      Firebase, OTA) nie regresuje.

### Krok 3 — Jeśli aktualizacja frameworka nie jest możliwa/pożądana teraz
Opcje obejścia na poziomie aplikacji (bez czekania na update frameworka):
- [ ] Rozważyć wyłączenie okresowej auto-resynchronizacji SNTP
      (`sntp_set_sync_interval()` / drugi argument `configTzTime`) i
      zamiast tego robić resync **ręcznie, z tego samego, jednego,
      kontrolowanego miejsca/taska**, co DNS dla FB/TG — tak żeby nigdy nie
      doszło do nakładania się wywołań SNTP i DNS z różnych tasków w tym
      samym momencie.
- [ ] Jeśli to nie pomoże lub nie da się łatwo wdrożyć: **zaakceptować jako
      rzadkie ryzyko szczątkowe** — to `PANIC/CRASH` (kontrolowany restart
      przez panic handler, nie zawieszenie), złapany tylko raz w
      wielotygodniowym materiale logów. Ten sam wzorzec co decyzja o
      zamrożeniu flash (sekcja 3a briefingu) — czasem najlepsza dostępna
      odpowiedź to świadome zaakceptowanie rzadkiego, dobrze zrozumianego
      ryzyka zamiast ryzykownej przebudowy architektury.

### Krok 4 — Weryfikacja po wdrożeniu
- [ ] Obserwować kolejne logi pod kątem powtórki tej samej sygnatury
      (`checkpoint=` cokolwiek + `PANIC/CRASH` + coredump z
      `sntp`/`udp_input`/`assert` w backtrace).
- [ ] Jeśli się powtórzy mimo aktualizacji frameworka — oznacza to, że
      albo #10725 nie objęło naszego dokładnego przypadku, albo
      `CONFIG_LWIP_TCPIP_CORE_LOCKING` włącza się na nowo przy każdym
      buildzie z innego powodu, niż podejrzewaliśmy — wraca się do kroku 1.

---

## 4. Podsumowanie priorytetu

To **rzadki** typ crasha (1 potwierdzone wystąpienie w tygodniach logów, na
tle znacznie częstszych `WDT_TASK`) — niższy priorytet niż główna seria
WDT z `plan_poprawek_WDT.md`, ale wart zamknięcia, bo mamy rzadką okazję:
**dokładnie nazwany, udokumentowany bug z realnym, scalonym fixem
upstream** — nie trzeba zgadywać ani pisać własnej łatki, wystarczy
zweryfikować konfigurację i ewentualnie zaktualizować framework.
