# Panel WWW Ryb (akwarium-firebase-panel-v14.html)

Plik HTML otwierany w przeglądarce (np. na telefonie). Steruje akwarium przez Firebase RTDB.

## Sekrety
**Bez sekretów w pliku.** Repo jest publiczne, więc `DB_SECRET` i `CMD_TOKEN` nie są
w kodzie. Przy pierwszym otwarciu panel pyta o:

1. **Firebase Database Secret**: nowy, po rotacji (Firebase Console → Project settings →
   Service accounts → Database secrets). Stary sekret jest do odwołania.
2. **CMD_TOKEN**: ten sam, co w `firmware/src/secrets.h` na ESP.

Zapisują się w `localStorage` tej przeglądarki (osobno na każdym urządzeniu).
Zmiana: w konsoli przeglądarki `akwResetSecrets()`.

## Tryby połączenia (v14)
Badge w nagłówku pokazuje, którą drogą panel rozmawia z ESP:

- **LAN ✓**: panel otwarty jako plik lokalny i ESP osiągalny pod `http://<IP>:8080`.
  Działa skan Wi-Fi, status OTA i wymuszona reinstalacja.
- **ZDALNIE (Firebase)**: sterowanie, odczyt i zwykły OTA przez komendę `update` w Firebase.
  Skan Wi-Fi i wymuszenie OTA wymagają LAN.

**Uwaga:** panel hostowany na HTTPS (np. GitHub Pages) nie połączy się z `http://<IP-ESP>`
(blokada mixed content). Do trybu LAN otwieraj plik HTML lokalnie.

## Karty dodane w v14
- **Aktualizacja firmware (OTA)**: wersja, status, postęp, „Sprawdź i zainstaluj”, „Wymuś”.
  Endpointy: `POST /api/ota-start[?force=1]`, `GET /api/ota-status`.
- **Skan sieci Wi-Fi**: `POST /api/wifi/scan/start`, poll `GET /api/wifi/scan/result`.

## Wymagania po stronie ESP
CORS + `Access-Control-Allow-Private-Network` + obsługa OPTIONS (firmware 4.3.0+).
Bez tego Chrome/Edge blokuje żądania z innego originu do adresu LAN.
