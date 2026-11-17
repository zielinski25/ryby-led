# Centrala 3.57.12 — kandydat do testów

**To nie jest wydanie produkcyjne. Nie wgrywać na pracujący piec ani zdalnie pod nieobecność osoby przy instalacji.** Przygotowano zmiany kodu i testy hostowe, ale nie udało się skompilować całego firmware ani sprawdzić go na ESP32 i urządzeniu. Nie ma nowego `firmware.bin`, release ani OTA.

## Baza i pliki

- Baza: kandydat centrali 3.57.11 bez Zew 3, normalizowany SHA-256 `6e86f349a12b9c1be94ee18fc28e12991dcb07927fd4b2dc28b7f6084d8f503d`. Numer zgłoszony przez centralę nie potwierdza identyczności źródła, z którego zbudowano `.bin`.
- `centrala-3.57.11-do-3.57.12-KANDYDAT.patch` — komplet zmian względem powyższego kandydata, nie względem dowolnej innej wersji o tym samym numerze.
- `buduj-3.57.12-KANDYDAT.ps1` — Windows: sprawdza sumy bazowego pliku i patcha, tworzy kopię, aplikuje patch i buduje środowisko PlatformIO `esp32-s3-n16r8`. **Nie wgrywa i nie publikuje OTA**. Wynik na pulpicie jest oznaczony `KANDYDAT-NIE-WGRYWAC.bin`.
- `testy/` — testy C++ uruchamiane na komputerze z atrapami czujników. `wybor_interwalu.h` jest wcześniejszym prototypem, nie jest dołączany do firmware.
- Wcześniejszy patch `3.57.12-czesciowy-NIE-WGRYWAC.patch` był niepełnym szkicem; nie jest częścią tej paczki i nie należy go używać z pełnym patchem.

## Częstotliwości i sterowanie

| Źródło | Zamierzone odczyty |
|---|---|
| C.O. i powrót | 60 s, gdy oba i trociny mają poprawny świeży odczyt poniżej 30°C. Poza tym ok. 2,8 s przy włączonej pompie, 15 s przy wyłączonej. 30°C nie jest poniżej progu. |
| Trociny, panel, bojler | 60 s |
| Pokój, Zew 1 | 120 s |
| Zew 2 i wilgotność DHT | wspólnie 120 s |
| Ciśnienie BMP280 | 180 s |
| Dym | jego zadanie i cykl zasilania bez zmian |

Scheduler DS wywoływany jest co 500 ms. Każda magistrala ma konwersję przed odczytem: odczekuje co najmniej 750 ms, a po 3 s bez gotowości zgłasza błąd. Rejestruje czas rzeczywistej próby; przekroczenie harmonogramu i czas konwersji wpływają na rzeczywisty odstęp. Po skróceniu interwału nie czeka się na pierwotny długi termin. Po restarcie oraz bez poprawnego i świeżego C.O., powrotu lub trocin nie jest wybierany tryb 60 s.

**Ważne:** zlecenie konwersji dotyczy całej magistrali. Na BUS_2 konwersje bojlera i trocin mogą więc zachodzić częściej niż ich odczyt, kiedy C.O./powrót wymagają szybszej konwersji. To nie jest indywidualne wyłączanie pomiaru w DS18B20. Timer prób uruchamia się również po błędzie; ponowna próba może zatem nastąpić dopiero po przyjętym dla danego kanału odstępie.

Progi jakości przyjęto 75 s dla C.O., średniej C.O., powrotu, trocin, panelu i bojlera; 135 s dla pokoju, Zew 1 i RH; 195 s dla BMP; 180 s dla dymu bez zmian. Zew 2 ma osobny czas ważności 135 s. Błąd, odrzucenie filtra lub brak wykonania ostatniej próby daje jakość INVALID, także po wcześniejszym dobrym pomiarze. Globalna zmienna może nadal zawierać poprzednią wartość: nie należy traktować tego jako nowego fizycznego odczytu.

**Nie jest to gwarancja bezpiecznego zachowania przy awarii.** Próg jakości C.O. jest używany także przez rozpalanie, a próg trocin przez automat mieszadła; ta zmiana wpływa na te funkcje. Istniejący automat mieszadła ignoruje nieświeży odczyt, ale nie jest to równoznaczne z wymuszonym wyłączeniem już włączonego mieszadła. W istniejącym kodzie alarmy i sterowanie mogą nadal korzystać z zachowanej wartości. Opóźnienia alarmu i reakcji mogą wzrosnąć, szczególnie po błędzie lub odrzuceniu skoku przez filtr. Nie należy zakładać, że zabezpieczenia niezależne od centrali chronią także panel i przystawkę.

## Pozostałe zmiany

- Filtr DS dostaje osobną informację o powodzeniu, więc temperatura −1°C i niższe prawidłowe wartości nie są z góry traktowane jako błąd. Komunikat o błędzie nadal nie jest temperaturą.
- Próby startowe DHT mają co najmniej 2 s przerwy; pierwsza próba cykliczna uwzględnia ostatnią próbę startową. Próby po błędzie cyklicznym również podlegają interwałowi 120 s.
- Diagnostyka DHT w logu sumuje próby, błędy wilgotności i ramki z temperaturą 0°C. **0°C jest nadal dopuszczalne**. Liczniki zerują się po restarcie. Istniejąca reguła odrzucania RH ≤0,1% nie została zmieniona. Wolniejszy odczyt nie dowodzi usunięcia przyczyny błędów.
- Podbito numer do `v3.57.12`, dodano wpis zmian i komunikat informacyjny przy konfiguracji. Interwał wysyłki oraz próbkowania historii nie został zmieniony.

## Uruchomione testy

Przetestowano zastosowanie i odwrócenie patcha na kopii dokładnej bazy oraz kompilację fragmentów C++ na komputerze: rzeczywisty harmonogram DS z atrapami (progi, pompa, wspólna magistrala, błąd, timeout, restart, symulacja i zawijanie czasu); rzeczywiste funkcje jakości; fragmenty DHT/BMP i filtr temperatur; układ 80 B i serializację telemetrii. **To nie jest test całości firmware ani instalacji.**

Zainstalowano PlatformIO Core 6.2.0 i próbowano zbudować wcześniejszy kandydat 3.57.11 w znalezionym projekcie testowym, lecz pobieranie platformy ESP32 przekierowuje na host niedostępny z tego środowiska. Nie powstał build ani test sprzętowy, również nie dla 3.57.12. Znaleziony na Dysku projekt testowy ma zastępczą bibliotekę DHT, niepotwierdzoną na piecu. Nie wiadomo też, czy projekt użytkownika odpowiada znalezionemu źródłu. Skrypt Windows celowo odmówi pracy przy innym hashu bazy.

Przed ewentualnym wdrożeniem: kompilacja dokładnego projektu, test na osobnym sprzęcie bez aktywnego sterowania instalacją, test powrotu do poprzedniego firmware, sprawdzenie logów i opóźnień z pomiarami rzeczywistych czujników. Wszelkie decyzje o wgraniu na instalację wymagają obecności osoby mogącej ją nadzorować.
