# Widżet: temperatura zewnętrzna z miejscem po przecinku — Android 0.31.101

Patch do projektu Android **0.31.100 / 31100** zwiększa wersję do 0.31.101 / 31101. We wszystkich trzech wariantach widżetu wybrane źródło Zew 1 lub Zew 2 wyświetla jedną cyfrę po przecinku, np. `5,9°`, `0,0°`, `-10,5°`. Format jest niezależny od ustawień języka telefonu. Pozostałe wartości nadal są zaokrąglane do stopni całkowitych. Brak danych to `—`, a symulacja zachowuje znacznik `~`. Zmiana nie dotyczy sterowania ani odczytu, tylko sposobu prezentacji.

Skrypt `buduj-widget-przecinek-0.31.101.ps1` sprawdza wersję projektu, sumę i zgodność patcha, uruchamia testy jednostkowe oraz `assembleDebug`, a po sukcesie kopiuje APK debug na Pulpit. Zrób kopię projektu przed uruchomieniem; jeśli kontrola zgodności odmówi, nie omijaj jej. Jeśli projekt jest w wersji 0.31.99, najpierw zastosuj poprawkę 0.31.100.

Patch nakłada się i odwraca na odtworzonym projekcie 0.31.100; logika formatowania została uruchomiona w Kotlin/JVM (oba źródła, wartości ujemne i zero, brak danych, symulacja oraz pozostałe temperatury). **Nie wykonano pełnego buildu Android, testów JUnit ani próby na telefonie** — w środowisku brak Android SDK. Skrypt uruchomi testy i build na komputerze użytkownika.

SHA-256 patcha: `f9477ec1ab6fb8a331494c352739fd57cfbc7d36843c54f0f622180eb83d3430`.
