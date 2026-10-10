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

## Log krytyczny (v14, Etap 5)
Zakładka **Logi** → karta „Log krytyczny”: rozmiar pliku, pobieranie i archiwum. Działa tylko w LAN
(zdalnie pokazuje się komunikat, a z Telegrama jest komenda `/log_krytyczny`).

## Wykresy tygodniowe (v14, Etap 3 — historia długa, 4.5.0)
Zakładka **Wykresy** → karta „Historia tygodniowa (co 30 min)”: cztery wykresy z
`GET /api/history/long` (kubełki 30 min, ok. 3 tygodnie na ESP). Tylko w LAN; zdalnie pokazuje
się komunikat „niedostępne przez Firebase”. Przycisk „⬇ CSV” pobiera surowy plik z ESP.

## Aplikacja mobilna (`panel/mobile/`, 4.7.2)
Osobny widok na telefon, zbudowany jak panel, ale tylko **zdalnie przez Firebase** (bez LAN).
Pliki: `index.html` (UI, 5 zakładek: Główna, Światło, Pompa, Energia, Ustawienia),
`app-logic.js` (logika bez DOM, testowana), `manifest.webmanifest` + `sw.js` + `icon.svg` (instalacja jako PWA).

- Sekrety jak w panelu: `akw_db_secret` i `akw_cmd_token` w `localStorage` tej przeglądarki (ten sam klucz,
  więc po wpisaniu w panelu nie trzeba wpisywać drugi raz). Nic nie jest w kodzie.
- Komendy: `POST /aquarium/commands.json` `{cmd, token, ts}`; `ts` rosnący. ESP odbiera je co ok. 14 s.
- Status: `GET /aquarium/status.json` co 5 s. ESP zapisuje status co ok. 60 s; po 150 s bez zmiany `updatedAt`
  ekran pokazuje „OPÓŹNIONE”.
- Hostowanie: musi być HTTPS (instalacja PWA, service worker). Mixed content nie dotyczy, bo aplikacja nie
  łączy się z IP ESP. Gdzie hostować, to decyzja właściciela (np. GitHub Pages, Firebase Hosting).
- Test UI: `firmware/tests/host/check_mobile_app.js` (krok 13 w `run_tests.sh`) + test E2E w headless Chromium
  przeciw atrapie RTDB (poza repo, bo wymaga puppeteera).
