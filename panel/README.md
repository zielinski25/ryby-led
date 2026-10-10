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

## Aplikacja mobilna (`ryby-mobile.html`, 4.7.2)
Jeden samodzielny plik HTML (jak Piec): UI, logika i style są w środku. Zdalnie przez Firebase, bez LAN.
Zakładki: Główna, Światło, Pompa, Energia, Ustawienia.

- Sekrety jak w panelu: `akw_db_secret` i `akw_cmd_token` w `localStorage` tej przeglądarki (ten sam klucz,
  więc po wpisaniu w panelu nie trzeba wpisywać drugi raz). Nic nie jest w kodzie.
- Logika (bez DOM) jest między markerami `LOGIC-BEGIN` / `LOGIC-END`. Testy: `firmware/tests/host/check_mobile_app.js`
  (krok 13 w `run_tests.sh`) czytają ją z tego pliku.
- Komendy: `POST /aquarium/commands.json` `{cmd, token, ts}`; `ts` rosnący. ESP odbiera je co ok. 14 s.
- Status: `GET /aquarium/status.json` co 5 s. ESP zapisuje status co ok. 60 s; po 150 s bez zmiany `updatedAt`
  ekran pokazuje „OPÓŹNIONE”.
- Hostowanie: dowolny HTTPS albo lokalnie. Aplikacja nie łączy się z IP ESP, więc mixed content jej nie dotyczy.
- Brak service workera (plik ma być samodzielny): bez trybu offline; po zmianie pliku odśwież stronę.
- Test E2E w headless Chromium przeciw atrapie RTDB jest poza repo (wymaga puppeteera).
