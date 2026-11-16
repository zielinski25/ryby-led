# Widżet telefonu a wybór Zew 1 / Zew 2 — Android 0.31.100

W wersji 0.31.99 wybór źródła zmieniał jedynie kafelek w aplikacji. Widżet Androida używał na stałe pola `t_zewn` (Zew 1), a zmiana `t_zewn_2` nie była uwzględniana w sygnaturze odświeżania.

Patch 0.31.99 → 0.31.100 (versionCode 31100) zachowuje konfigurację każdego widżetu, ale jego pozycja Zewnętrzna korzysta ze wspólnego wyboru w aplikacji. Wszystkie trzy warianty widżetu stosują ten wybór. Po zmianie źródła widżety są przerysowywane ze swojej ostatniej migawki; kolejne zmiany Zew 2 aktualizują migawkę tak jak pozostałe odczyty. Na widżecie pojawia się oznaczenie 1 lub 2. Jeżeli wybranego odczytu brak, widżet pokazuje `—`, bez podstawiania wartości drugiego czujnika. Widżet nadal wymaga napływu danych z aplikacji lub działającego monitorowania w tle; nie jest dodawane osobne okresowe odpytanie.

Symulacja Zew 1 nie oznacza odczytu Zew 2 jako symulowanego; ogólny status widżetu nadal może informować o aktywnej symulacji sterownika. Dla obu źródeł dolna granica walidacji widżetu wynosi −25°C zgodnie z poprzednim wymaganiem; górne granice to 45°C dla fizycznego Zew 1 oraz 80°C dla Zew 2. Sterowanie piecem nie jest zmieniane.

Skrypt `buduj-widget-zew-0.31.100.ps1` wymaga projektu 0.31.99 po poprzedniej poprawce. Sprawdza wersję, SHA-256 i zgodność patcha, uruchamia testy i `assembleDebug`, a po sukcesie kopiuje APK na Pulpit. Zrób kopię projektu przed użyciem; jeśli kontrola zgodności odmówi, nie omijaj jej.

Sprawdzono apply/reverse na kopii 0.31.99 oraz osobno Kotlin/JVM: wybór klucza, brak/zero, granice i kolory. Pełnego buildu Android, testów JUnit ani sprawdzenia widżetu na telefonie nie wykonano — brak Android SDK; testy Gradle uruchomi skrypt Windows.

SHA-256 patcha: `a262bd021394b7a199e470f3dd28a6554d82b8f1219c5c3914f309892b326884`.
