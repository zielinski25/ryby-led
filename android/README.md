# Ryby LED — aplikacja Android (4.7.2)

Natywna wersja `panel/ryby-mobile.html` (ta sama logika, te same komendy i pola statusu).
Zakładki: Główna, Światło, Pompa, Energia, Ustawienia. Komunikacja tylko z Firebase RTDB przez HTTPS.

## Budowanie z PowerShell (Windows, najprościej)

Skrypt sam sprawdza Javę, pobiera Android SDK, akceptuje licencje, buduje APK i kopiuje go na pulpit.
Uruchom w PowerShell 5 (z folderu repo albo z rozpakowanego ZIP-a):

```powershell
powershell -ExecutionPolicy Bypass -File .\android\build_apk.ps1
```

Wynik: `ryby-led-4.7.2-debug.apk` na pulpicie. Pierwsze uruchomienie trwa kilkanaście minut (pobieranie SDK i Gradle).
Jeśli skrypt każe zainstalować Javę, zamknij PowerShell, otwórz go ponownie i uruchom skrypt jeszcze raz.
Skrypt nie był testowany na Windows w środowisku, w którym powstawał. Jeśli zgłosi błąd, wklej jego treść.

## Budowanie (Android Studio, Windows)

1. Zainstaluj Android Studio i wybierz w ustawieniach JDK 17–21 (jbr-21 jest w pakiecie).
2. `File → Open` → wskaż folder `android` z repo.
3. Poczekaj na synchronizację Gradle (pierwszy raz pobiera Gradle 8.9 i zależności).
4. `Build → Build APK(s)` albo `Run` na telefonie z włączonym debugowaniem USB.
   APK: `app/build/outputs/apk/debug/app-debug.apk`.

Z linii poleceń (PowerShell, w folderze `android`):

```powershell
.\gradlew.bat :app:assembleDebug
```

> Uwaga: ten projekt NIE został zbudowany w środowisku, w którym powstawał (brak JDK i Android SDK).
> Kontrolę przeprowadza `firmware/tests/host/check_android_app.js` (kontrakt, nie kompilacja).
> Jeśli build zgłosi błąd, wklej pierwsze linie błędu — poprawka to zwykle jedna linijka.

## Ustawienia na telefonie

Zakładka **Ustawienia**: wpisz Database Secret i CMD_TOKEN, potem **Sprawdź połączenie**.
Sekrety zostają w prywatnych ustawieniach aplikacji na tym telefonie; nie są w kodzie ani w repo.
**Wyczyść sekrety z tego telefonu** kasuje je lokalnie.

## Różnice względem wersji HTML

- Brak trybu offline ani widżetu na ekranie głównym (tylko ekran aplikacji).
- Sekrety w zwykłych (nie szyfrowanych) prywatnych SharedPreferences. Szyfrowanie możesz dodać później.
- Harmonogram tylko do odczytu (jak w wersji HTML).
