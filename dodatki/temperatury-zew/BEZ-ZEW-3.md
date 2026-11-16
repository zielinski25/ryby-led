# Zew 1 i Zew 2 — bez Zew 3

Aktualny kandydat zastępuje wcześniejszą funkcję trzech temperatur. Na wykresie pozostają **Zew 1 (DS18B20)** i **Zew 2 (temperatura DHT22)**; pulpit nie jest zmieniany. Nie zmieniono sterowania piecem ani algorytmu wilgotności. Usunięto dodatkowy diagnostyczny odczyt temperatury BME/BMP oraz powiązane pola statusu, telemetrii i aplikacji. Pomiar ciśnienia nadal działa jak wcześniej: jego funkcja jest identyczna z bazowym v3.57.9.

## Skąd dokąd

- `android-0.31.93-do-0.31.94-bez-zew3.patch` wymaga projektu **0.31.93 z poprzednią poprawką wykresów**, nie dowolnego projektu z tym numerem. Skrypt `buduj-wykresy-0.31.94-bez-zew3.ps1` sprawdza możliwość zastosowania poprawki, uruchamia testy i buduje debug APK. Zrób kopię projektu przed uruchomieniem. Projekt 0.31.91 lub 0.31.92 wymaga osobnego ustalenia właściwej drogi aktualizacji.
- `centrala-3.57.10-do-3.57.11-bez-zew3.patch` wymaga **dokładnie przygotowanego kandydata 3.57.10**, sprawdzanego hashem zawartości pliku po normalizacji końców linii. Skrypt `buduj-centrala-3.57.11-bez-zew3.ps1` zapisuje kopię i wyłącznie buduje; niczego nie wgrywa. Sama wersja zgłaszana przez /version nie dowodzi zgodności źródła. W razie błędu zgodności nie obchodzić sprawdzenia.
- Numer 3.57.11 jest wyższy od 3.57.10 w lokalnym porównaniu wersji. Sam numer nie tworzy release, pliku bin ani instalacji OTA. To nadal linia 3.57.x, bez poprawek osobnej linii 3.58.x.

## Stare dane

Dawne wpisy `t_zewn_3` w Firebase **nie są kasowane**. Nowa aplikacja ignoruje je i nie pokazuje. Dane Zew 2 i stare formaty cache pozostają czytelne; gdy stary rekord ma obie dodatkowe wartości, Zew 3 jest ignorowany. Pola Zew 3 nie będą wysyłane w nowych próbkach i statusach. W układzie próbki ESP zachowano zarezerwowane dwa bajty i znacznik formatu: stare kolejki nie powinny przez to zmienić rozmiaru. Nie przeprowadzono jednak testu odtwarzania flash na sprzęcie.

## Testy i ograniczenia

Sprawdzono zastosowanie i odwrócenie obu poprawek na kopiach znanych baz. Hostowy test C++ uruchamia faktyczne fragmenty kandydata ESP: ABI 80 B, odczyt Zew 2, brak danych, 0°C, symulację, ciśnienie i serializację, w tym starą próbkę i próbkę z dawnym bajtem Zew 3. Dodano testy jednostkowe Androida dla dwóch serii, zer, nieprawidłowych wartości i starych wierszy cache. **Nie uruchomiono pełnej kompilacji ESP, Android/Gradle, testów JUnit, Firebase, OTA ani testów sprzętowych** — brak odpowiednich środowisk i sprzętu. Nie wgrywać kandydata na pracujący kocioł; najpierw kompilacja i test poza instalacją oraz zapewnienie powrotu do działającej wersji.

Ta zmiana nie naprawia rozbieżności Zew 1/Zew 2 ani sporadycznych błędów DHT22.
