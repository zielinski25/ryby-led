// ═══════════════════════════════════════════════════════════════════════════
// secrets.example.h — SZABLON sekretów (Etap 2 planu upgrade: bezpieczeństwo)
// ═══════════════════════════════════════════════════════════════════════════
// UŻYCIE:
//   1. Skopiuj ten plik jako secrets.h do katalogu src/ (obok .cpp).
//   2. Wypełnij wartości poniżej.
//   3. secrets.h jest w .gitignore — NIGDY nie commituj go do repozytorium
//      i nie wysyłaj nikomu (zipy, screeny, wklejki do chatu).
//
// DLACZEGO OSOBNY PLIK:
//   Do v4.1.0 sekrety były zaszyte w źródle (FIREBASE_SECRET i CMD_TOKEN).
//   Wyciekły razem z publicznie udostępnionym zipem źródeł — dlatego:
//   - stary FIREBASE_SECRET należy ODWOŁAĆ w Firebase Console po wgraniu
//     firmware'u z nowym sekretem (Project Settings -> Database -> Secrets),
//   - stary CMD_TOKEN należy zmienić i wpisać nowy w panelu HTML.
// ═══════════════════════════════════════════════════════════════════════════
#pragma once

// Adres bazy Realtime Database (Firebase Console -> Project Settings ->
// General ->Your apps / sekcja Realtime Database). NIE jest tajny, ale trzymamy
// go razem z resztą dla porządku.
#define FIREBASE_HOST   "TWOJ-PROJEKT-default-rtdb.europe-west1.firebasedatabase.app"

// Tajny klucz bazy (Firebase Console -> Project Settings -> Service accounts
// -> Database secrets). UWAGA: traktuj jak hasło. W repo NIGDY.
#define FIREBASE_SECRET "WKLEJ_NOWY_DATABASE_SECRET"

// Token dostępu do komend panelu WWW (pole token w panelu HTML).
// Zmień na własny, trudny do zgadnięcia ciąg.
#define CMD_TOKEN       "ZMIEN_MNIE"
