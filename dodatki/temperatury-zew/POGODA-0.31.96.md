# Odświeżanie pogody — Android 0.31.96

Aplikacja podejmuje próbę pobrania aktualnej prognozy Open-Meteo przy każdym wejściu w zakładkę Pogoda (z nawigacji i kafelka pogody) oraz po uruchomieniu lub powrocie aplikacji na pierwszy plan. Odświeżanie co 15 minut na pierwszym planie i ręczny przycisk pozostają dostępne. Równoczesne żądania o tej samej lokalizacji i zakresie korzystają z jednego trwającego pobrania; zmiana lokalizacji lub zakresu uruchamia pobranie dla nowych ustawień. Starsza odpowiedź dla zmienionej lokalizacji/zakresu nie zastępuje aktualnych danych.

Przy pobieraniu zakładka pokazuje komunikat o aktualizacji. Jeśli pobranie się nie powiedzie, pokazuje błąd, a ostatnie dane — o ile istnieją — pozostają widoczne z informacją, że są wcześniejsze. Bez internetu lub odpowiedzi serwisu nie można zagwarantować świeżych danych. Nie zmieniono sterowania ESP ani pulpitu aplikacji.

## Budowanie

Zachowaj kopię projektu. `buduj-pogoda-0.31.96.ps1` wymaga znanego projektu **0.31.95** z poprzednią poprawką widżetu. Skrypt sprawdza wersję i zgodność patcha, uruchamia testy jednostkowe oraz build Androida, a po sukcesie kopiuje `SterownikCO-0.31.96-pogoda-debug.apk` na pulpit. Nie instaluje aplikacji. W razie innej wersji lub błędu zgodności nie obchodź kontroli; prześlij komunikat.

Sprawdzono zastosowanie i odwrócenie poprawki na kopii projektu 0.31.95 oraz statycznie ścieżki nawigacji, powrotu i oznaczenia błędu. Dodano testy JUnit kolejki odświeżania, deduplikacji, błędu i zmiany lokalizacji/zakresu. **Nie uruchomiono pełnej kompilacji Androida, testów JUnit ani testów na telefonie** z powodu braku SDK i Java. Skrypt uruchamia testy i buduje na komputerze użytkownika.
