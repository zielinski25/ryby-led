# Widżet: temperatury od −25°C — Android 0.31.95

Poprawka dotyczy wyłącznie trzech rozmiarów widżetu ekranu telefonu. Dolna granica dla wszystkich czterech wybieranych w nim czujników (piec, bojler, panel, zewnętrzna/Zew 1) wynosi teraz **−25°C włącznie** — również dla symulacji sterownika. Górne granice pozostają bez zmian. Wartość niższa niż −25°C, brak danych, null, tekst lub niepoprawna liczba daje „—”. Wartości nadal są zaokrąglane do pełnych stopni.

Nie zmienia czujników, walidacji pulpitu aplikacji ani sterowania ESP. Jeżeli centrala sama nie zwróci niskiego odczytu, ta zmiana nie spowoduje jego pojawienia się we widżecie. Wykresy Zew 1 i Zew 2 pozostają; Zew 3 jest usunięty jak w 0.31.94.

## Budowa

Zachowaj kopię projektu. Skrypt `buduj-widget-0.31.95-minus25.ps1` jest przeznaczony dla znanego projektu **0.31.94 po poprawce bez Zew 3**. Sprawdza numer i możliwość zastosowania poprawki, pobiera patch z GitHub, uruchamia `:app:testDebugUnitTest :app:assembleDebug` i po sukcesie zapisuje `SterownikCO-0.31.95-widget-minus25-debug.apk` na pulpicie. Wymaga działających Git, Java i Android SDK. Skrypt nie instaluje aplikacji. Jeśli masz inną wersję lub pojawi się błąd zgodności, nie obchodź go; prześlij komunikat.

Sprawdzono zastosowanie i odwrócenie poprawki na kopii projektu 0.31.94 oraz statycznie wartości granic i połączenie funkcji z widżetem. Dodano testy JUnit dla każdej temperatury, −25°C, niższych wartości, 0°C, błędnych danych oraz górnych granic. Nie uruchomiono pełnego buildu Androida ani testów JUnit w tym środowisku: brak SDK i Java. Skrypt uruchomi je na komputerze użytkownika.
