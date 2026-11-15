> Nowszy kandydat z wersją liczbową: [CENTRALA-3.57.10-ZEW.md](CENTRALA-3.57.10-ZEW.md). Ten dokument opisuje poprzedni kandydat.

# Centrala ESP32 — kandydat v3.57.9-zew

Podstawa: źródło centrali v3.57.9 z paczki `SterownikCO-v3.57.9-LIMITY-SIECI.zip`; użytkownik potwierdził `/version: v3.57.9`. Poprawka dotyczy tej wersji. Nie zakładaj zgodności z innymi źródłami, nawet jeśli pokazują taki sam numer wersji.

## Zakres

- DS18B20 i `t_zewn` pozostają bez zmian (Zew 1).
- Zew 2: temperatura z normalnego odczytu DHT22, niezależna od walidacji wilgotności. Zew 3: temperatura czujnika BME/BMP przy odczycie ciśnienia. Zakresy odpowiednio -40…80°C i -40…85°C; 0°C jest prawidłowym odczytem.
- Dane są tylko diagnostyczne; nie zmieniają logiki sterowania ani algorytmu korekcji wilgotności. Brak inicjalizacji, błąd, niepoprawny zakres, symulacja wilgotności/ciśnienia i odczyt starszy niż 15 s oznaczają brak pomiaru. Odłączenie sprzętu może powodować różne błędy magistrali; walidacja nie gwarantuje wykrycia wszystkich fizycznych usterek.
- Pola `t_zewn_2` i `t_zewn_3` dodano do wspólnego statusu Firebase/LAN oraz lokalnego statusu. Minutowa telemetria Firebase zawiera te pola, tylko gdy mają poprawne wartości. Wersja Android 0.31.93 wyświetla je w wykresach. Dawne dane nie są uzupełniane.
- Struktura telemetrii nadal ma 80 B; dwie wartości wykorzystują cztery dawne bajty wyrównania. Nieużywane wysokie bity jakości zawierają sygnaturę formatu, która odróżnia nowe rekordy od starych. Kontrola statyczna rozmiaru i offsetów pozostaje w kodzie. Nowy firmware ukrywa sygnaturę przy wysyłaniu zwykłego pola `q`; starych rekordów nie uznaje za nowe. Trwały offline spool przechowuje całą próbkę, więc nowe dane mogą zostać odtworzone również po przerwie w sieci, o ile dotychczasowy mechanizm buforowania działa i nie utraci próbek.
- Lokalne archiwa CSV `/api/history` i `/api/history/long` są **bez zmian**: nie przechowują Zew 2 i Zew 3. Oś historii nowych serii opiera się na telemetrii Firebase, nie na lokalnym CSV. Sam status daje tylko aktualne punkty podczas działania aplikacji.

## Bezpieczeństwo i stan testów

To **kandydat do testów**, nie wydanie sprawdzone na pracującym piecu. Test C++ na komputerze używa rzeczywistych fragmentów zmienionego źródła, atrap czujników i zegara. Sprawdza layout 80 B, stare/nowe rekordy, kopiowanie do/z obrazu spool, serializację JSON, brak/0/ujemne/granice, wygasanie, symulację i błędy DHT/BME. Przeprowadzono też kontrolę zastosowania patcha. Nie wykonano pełnej kompilacji firmware dla ESP32 ani testów fizycznych, sieci Firebase, pamięci flash lub mechanizmów bezpieczeństwa pieca. Test hosta nie dowodzi bezpieczeństwa działania urządzenia.

Przed wgraniem: zbuduj na właściwym projekcie i przetestuj na stanowisku/egzemplarzu testowym, w tym restart, utratę sieci, spool i błędy czujników; miej możliwość powrotu do sprawdzonego firmware. Nie testuj niezweryfikowanego firmware na pracującym kotle.

## Budowanie kandydata na Windows

`buduj-centrala-3.57.9-zew.ps1` zakłada folder `%USERPROFILE%\Documents\PlatformIO\sterownik-co-esp32`, środowisko `esp32-s3-n16r8`, Git i zainstalowane PlatformIO. Sprawdza SHA-256 poprawki oraz całego znormalizowanego pliku bazowego i odmawia działania, gdy plik różni się od otrzymanej bazy v3.57.9. Wykonuje kopię `main_centrala.cpp`, nie dotyka `secrets.h`, uruchamia tylko `pio run` (bez upload), a po sukcesie kopiuje `SterownikCO-3.57.9-zew-test.bin` na pulpit Windows. Jeśli ścieżka jest inna lub kontrola się nie powiedzie, nie obchodź jej w ciemno — sprawdź właściwy projekt. Poprawiony plik pozostaje w projekcie, kopia jest w jego katalogu głównym.

Test hosta (Linux / Python 3 + g++):

```
python3 testy/test_centrala_zew.py SCIEZKA_DO_ZMIENIONEGO_main_centrala.cpp
```
