# Poprawka kompilacji kafelka Zewnętrzna — Android 0.31.98

W 0.31.97 wystąpiła kolizja sygnatur JVM między setterem właściwości `outdoorTileSource` a funkcją `setOutdoorTileSource`. Zmieniono nazwę funkcji i jej wywołania na `chooseOutdoorTileSource`; działanie wyboru Zew 1/2 pozostaje takie samo. Projekt po nieudanym buildzie 0.31.97 zawiera już zmiany i ma numer 0.31.97, dlatego patch jest właśnie od niego.

Skrypt `buduj-kafelek-0.31.98-zew.ps1` sprawdza numer wersji i zgodność patcha, następnie uruchamia testy i build oraz kopiuje APK debug na pulpit. Zrób kopię projektu przed uruchomieniem. Jeśli skrypt zgłosi błąd zgodności, nie obchodź go.

Sprawdzono apply/reverse patcha na kopii projektu 0.31.97 i statycznie zmianę nazwy oraz jej użycie. **Pełnej kompilacji Androida, testów JUnit ani testów na telefonie nie wykonano w tym środowisku**. W szczególności nie można wykluczyć innych błędów buildu; skrypt uruchamia build i testy na komputerze użytkownika.
