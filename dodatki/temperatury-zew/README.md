# Zew 1, Zew 2 i Zew 3

## Co zawierają pliki

- `android-0.31.91-do-0.31.92.patch` — zmiany dla otrzymanego projektu Android 0.31.91: Zew 1 = dotychczasowy DS18B20 (`t_zewn`), Zew 2 = temperatura DHT22 (`t_zewn_2`), Zew 3 = temperatura czujnika ciśnienia (`t_zewn_3`). Nowe kafelki są diagnostyczne, nie sterują piecem. Brak lub błędny pomiar jest pokazywany jako brak danych, a prawdziwe 0°C pozostaje prawidłowym wynikiem. Nowe kafelki dodają się także do zapisanego układu użytkownika; późniejsze ręczne ukrycie jest zapamiętane. Wersja projektu po zmianie: 0.31.92 / 31092.
- `centrala-3.57.9-PRZYKLAD.patch` — **wyłącznie propozycja do źródeł centrali v3.57.9**. Wersja faktycznie wgrana w centralę nie została potwierdzona; nie wgrywaj tej wersji do pieca. Po otrzymaniu aktualnych źródeł trzeba dostosować zmianę i osobno przetestować firmware. Paczka v3.57.9, którą dysponujemy, nie jest kompletnym projektem PlatformIO.

Sama aktualizacja aplikacji nie spowoduje pojawienia się temperatur, jeżeli centrala nie wysyła nowych pól. Przy starszym statusie Zew 2 i Zew 3 pokazują brak danych. Temperatura czujnika ciśnienia może różnić się od temperatury na dworze: zależy od miejsca montażu i nagrzania urządzenia.

## Zastosowanie poprawki Android na kopii w Windows

Potrzebny jest Git for Windows, Java 17 oraz działające środowisko Android/Gradle, używane do budowania poprzedniej wersji. Najpierw zachowaj kopię projektu. Umieść pobrany plik `android-0.31.91-do-0.31.92.patch` w folderze Pobrane. Następnie w PowerShell:

```powershell
$projekt = "$env:USERPROFILE\Downloads\co-0.31.89-build\SterownikCO-0.31.83"
$patch = "$env:USERPROFILE\Downloads\android-0.31.91-do-0.31.92.patch"
Set-Location $projekt
git apply --check --ignore-space-change "$patch"
```

Kontynuuj tylko jeśli ostatnia komenda nie zgłosi błędu. Pracuj na kopii, bo następny krok zmienia pliki:

```powershell
git apply --ignore-space-change "$patch"
if ($LASTEXITCODE -ne 0) { throw "Nie udało się zastosować poprawki." }
.\gradlew.bat :app:testDebugUnitTest :app:assembleDebug
if ($LASTEXITCODE -ne 0) { throw "Testy lub budowanie APK nie powiodły się." }
```

Po udanym wykonaniu APK debug znajdziesz w `app\build\outputs\apk\debug\app-debug.apk`. Konfiguracja projektu nadaje debugowi inny identyfikator aplikacji (`.debug`), więc może on zainstalować się obok posiadanej wersji, a nie jako jej aktualizacja. Aktualizacja istniejącej aplikacji wymaga identycznego identyfikatora i zgodnego podpisu; nie odinstalowuj dotychczasowej aplikacji w ciemno, bo możesz utracić ustawienia.

## Stan sprawdzeń

Poprawki sprawdzono pod kątem zastosowania do otrzymanych kopii źródeł; dodano testy jednostkowe Androida. Sprawdzono osobno w symulowanym środowisku C++ logikę braku danych, wartości 0°C, wygasania i przepełnienia licznika czasu. W tym środowisku nie było Java, Android SDK ani kompletnego projektu firmware, dlatego **nie skompilowano APK ani firmware i nie uruchomiono testów Androida**. Nie było też testu na urządzeniu. Przed użyciem potrzebne są kompilacja, testy i potwierdzenie aktualnej wersji oraz konfiguracji sprzętu.
