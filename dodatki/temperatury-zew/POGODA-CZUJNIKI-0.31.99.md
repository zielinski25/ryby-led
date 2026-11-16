# Pogoda i dwa odczyty Zewnętrzna — Android 0.31.99

Patch jest przeznaczony wyłącznie do projektu Android 0.31.98/31098 zawierającego wcześniejszą poprawkę kafelka. Zwiększa numer do 0.31.99/31099; nie zmienia firmware sterownika.

- Po otwarciu kafelka Zewnętrzna widać niezależnie Zew 1 (DS18B20) i Zew 2 (DHT22). Wybór nadal dotyczy tylko kafelka, nie sterowania piecem. Symulacja w tym oknie dotyczy wyłącznie Zew 1. Brak danych i nieaktualny status są sygnalizowane osobno dla odczytów; brak Zew 2 nie jest zastępowany Zew 1.
- Automatyczne pobieranie następuje po co najmniej 30 minutach od ostatniego **udanego** pobrania dla danej lokalizacji i liczby dni, albo gdy brak odpowiednich danych. Otwarcie aplikacji i wejście do pogody nie wymuszają wcześniejszego pobrania. W aktywnej aplikacji warunek jest też sprawdzany co 15 minut, dlatego odświeżenie może nastąpić później niż po 30 minutach. Po błędzie stare dane pozostają widoczne z ostrzeżeniem, a kolejne wywołania mogą ponawiać próbę.
- Przycisk ręcznego odświeżenia oraz zmiana lokalizacji lub liczby dni nadal mogą pobrać dane przed upływem 30 minut. Równoczesne żądania o tych samych parametrach są łączone.
- Ostatnia odpowiedź jest zapisana lokalnie i po ponownym uruchomieniu aplikacji odtwarzana, jeśli zgadza się lokalizacja i zakres. Jej wiek po restarcie wyliczany jest na podstawie zegara urządzenia; cofnięcie zegara przed czas zapisu, usunięcie/uszkodzenie cache lub brak dostępu do niego powoduje nowe pobranie. W trakcie pracy czas jest mierzony monotonicznie.

## Windows

Zrób kopię projektu. Skrypt `buduj-pogoda-czujniki-0.31.99.ps1` sprawdza wersję, sumę i zgodność patcha; uruchamia testy jednostkowe oraz `assembleDebug`, a po sukcesie kopiuje APK na Pulpit. Jeśli odmówi zastosowania, nie obchodź kontroli zgodności.

Zweryfikowano zastosowanie patcha i możliwość jego odwrócenia na kopii projektu 0.31.98. Z użyciem Kotlin/JVM uruchomiono osobny test logiki granicy 30 minut i deduplikacji oraz sprawdzono kompilację plików pogody z zastępczymi deklaracjami bibliotek. **Nie wykonano pełnego buildu Android, testów JUnit ani testów na telefonie**: w środowisku nie ma Android SDK. Testy Gradle wykona skrypt na komputerze użytkownika; nie wyklucza to problemów na urządzeniu.

SHA-256 patcha: `0eef37117b16b4dac44ac0bfa3abac2b21719cfc0ebe94818bd5d6d10078a5ce`.
