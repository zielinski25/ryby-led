# Panel WWW Ryb (akwarium-firebase-panel-v13.html)

Plik HTML otwierany w przeglądarce (np. na telefonie) — steruje akwarium przez Firebase RTDB.

**Bez sekretów w pliku.** Repo jest publiczne, więc `DB_SECRET` i `CMD_TOKEN` nie są
w kodzie. Przy pierwszym otwarciu panel pyta o:

1. **Firebase Database Secret** — nowy, po rotacji (Firebase Console → Project settings →
   Service accounts → Database secrets). Stary sekret jest do odwołania.
2. **CMD_TOKEN** — ten sam, co w `firmware/src/secrets.h` na ESP.

Zapisują się w `localStorage` tej przeglądarki. Zmiana: w konsoli przeglądarki
`akwResetSecrets()`.

Uwaga: to nadal Legacy Database Secret w przeglądarce — docelowo (Etap 2, część 2)
zamieniamy to na logowanie użytkownika.
