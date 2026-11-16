> **Nieaktualne:** Zew 3 usunięto w nowych kandydatach. Aktualne informacje: [BEZ-ZEW-3.md](BEZ-ZEW-3.md). Poniżej historia poprzedniej wersji.

# Poprawka aplikacji: Zew 1/2/3 na wykresie

Poprawka jest przeznaczona dla projektu **0.31.92 po poprzedniej poprawce**. Przywraca definicje pulpitu dokładnie do stanu 0.31.91 (łącznie z nazwą „Zewnętrzna”); dodatkowe kafelki i ich migracja są usunięte. W wykresach istniejąca seria `zewn` ma nazwę „Zew 1” i nadal pokazuje DS18B20; obok niej są niezależnie włączane serie „Zew 2” (DHT22) i „Zew 3” (BME/BMP). Domyślnie dwie nowe serie są wyłączone, aby nie zmieniać wybranych wcześniej linii. Wyświetl je naciskając ich przyciski w zakładce Wykresy.

Nie ma zmian w sterowaniu piecem. Prawdziwe 0°C jest pomiarem; brak, null, błędny typ i wartości poza zakresem nie tworzą zera. Stary format danych pozostaje obsługiwany. Nowsza aplikacja akceptuje bieżące pola statusu `t_zewn_2`, `t_zewn_3`, a przygotowany odbiornik historii chmurowej — opcjonalne pola o tych nazwach w °C w poszczególnych rekordach telemetrii. Pola nie są jeszcze wysyłane w istniejących wersjach firmware. Stare rekordy historii pozostaną puste: nie da się odtworzyć niezapisanych pomiarów.

## Co działa, a czego brakuje

Sama poprawka aplikacji **nie tworzy historycznych odczytów centrali**. Użytkownik potwierdził wersję ESP 3.57.9; powstał kandydat `centrala-3.57.9-zew-wykresy.patch` z odczytem i zapisem do telemetrii Firebase. Szczegóły, ograniczenia i stan testów: `CENTRALA-3.57.9-ZEW.md`. Wcześniejsza przykładowa poprawka `centrala-3.57.9-PRZYKLAD.patch` dodaje tylko status bieżący — nie używaj jej. Nie ma możliwości odtworzenia niezapisanych dawniej danych.

## Budowanie na Windows

Użyj skryptu `buduj-wykresy-0.31.93.ps1`. Zakłada on dokładnie folder projektu użyty do zbudowania 0.31.92 (`Downloads\co-0.31.89-build\SterownikCO-0.31.83`), działający Git, Java oraz Android SDK. Przed uruchomieniem zachowaj kopię projektu. Skrypt weryfikuje numer wersji, SHA-256 poprawki i możliwość jej zastosowania. Dopiero po udanych testach i buildzie kopiuje wynik do `SterownikCO-0.31.93-wykresy-debug.apk` na pulpicie Windows. Ponowne uruchomienie jest możliwe, jeśli ta sama poprawka została już zastosowana.

APK debug ma identyfikator z przyrostkiem `.debug`: o tym, czy zastąpi poprzednią aplikację, decydują identyfikator i zgodny podpis. Nie usuwaj starej aplikacji w ciemno, bo możesz utracić dane.

## Sprawdzenia

Sprawdzono zastosowanie i odwrócenie poprawki na kopii 0.31.92, a `TileSpecs.kt` jest identyczny bajtowo z otrzymaną 0.31.91. Osobno skompilowano Kotlinową logikę telemetrii oraz wyodrębnioną funkcję obliczania wykresu i sprawdzono stare/nowe dane, 0°C, ujemne, błędne i symulowane próbki, etykiety i trzy niezależne szeregi. Dodano testy jednostkowe dla projektu Android. Pełnego buildu Androida ani testów JUnit nie uruchomiono w tym środowisku, bo brak Android SDK; uruchomi je skrypt na komputerze. Sprzętu nie testowano.
