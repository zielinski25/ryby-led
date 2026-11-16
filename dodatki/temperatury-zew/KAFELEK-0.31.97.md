# Wybór źródła kafelka „Zewnętrzna” — Android 0.31.97

Po dotknięciu kafelka Zewnętrzna w aplikacji, w otwartym arkuszu są przyciski **Zew 1 · DS18B20** i **Zew 2 · DHT22**. Wybrane źródło jest wyświetlane na kafelku i zapisywane w ustawieniach aplikacji; domyślnie Zew 1. Zachowany jest dotychczasowy arkusz szczegółów Zew 1 i jego symulacja. Dopisek w arkuszu wyjaśnia, że symulacja dotyczy sterownika i Zew 1, a nie DHT.

Jeśli wybrany Zew 2 nie jest dostępny, kafelek pokazuje „—” i „BRAK DANYCH”; nie przestawia się po cichu na Zew 1. Rzeczywiste 0°C pozostaje odczytem. Jeśli dane są stare, kafelek oznacza je jako nieaktualne. Symulacja Zew 1 nie oznacza jako symulowanego niezależnego odczytu Zew 2. Źródła pozostałych obliczeń, historii i sterowania piecem nie zostały zmienione. Widżet na ekranie telefonu także nie jest zmieniany. Dostępność Zew 2 wymaga odpowiednich danych z centrali.

## Budowanie

Zachowaj kopię projektu. `buduj-kafelek-0.31.97-zew.ps1` wymaga znanego projektu **0.31.96** po poprawce pogody. Skrypt sprawdza numer i zastosowanie patcha, uruchamia testy i buduje APK debug. Po sukcesie kopiuje `SterownikCO-0.31.97-kafelek-zew-debug.apk` na pulpit. Nie instaluje aplikacji. Jeżeli projekt ma inną wersję lub skrypt zgłasza błąd zgodności, nie obchodź kontroli.

Sprawdzono zastosowanie/odwrócenie patcha na kopii projektu 0.31.96 oraz statycznie trasę kliknięcia, zapis/odczyt ustawienia, brak fallbacku i odrębność symulacji. Dodano testy jednostkowe prezentacji obu źródeł, braków, zera, symulacji i nieaktualnych danych. **Pełna kompilacja Androida, JUnit i test na telefonie nie zostały uruchomione** w tym środowisku — brak Java i Android SDK. Skrypt uruchomi testy i kompilację na komputerze użytkownika.
