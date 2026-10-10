# Proces release — Ryby LED (Etap 1 planu upgrade)

Obowiązuje od 4.0.0 (semver). Wzór: pipeline Centrali Pieca.

## 1. Przed releasem

1. Ustaw wersję w **trzech miejscach** (muszą być spójne):
   - `firmware/src/Ryby_LED_fi_S3.cpp` → `#define RYBY_FW_VERSION "vX.Y.Z+build.NNN"`
     (NNN = kolejny numer wewnętrzny, dawniej vNNN),
   - `version.txt` → `X.Y.Z`,
   - `CHANGELOG.md` → nowy wpis `## [X.Y.Z+build.NNN] TAG — data` na górze
     (szablon: Co / Dlaczego / Logika sterowania i algorytmy).
2. Semver: PATCH = fix, MINOR = nowa funkcja, MAJOR = przełom/zmiana łamiąca.
3. Uruchom bramkę (kompilacja + kontrola spójności + limit partycji 3 MB +
   artefakty):
   ```powershell
   cd firmware
   .\scripts\build_gate.ps1
   ```
   Bramka wystawia `firmware/release_artifacts/firmware.bin` i `firmware.elf`.

## 2. Tag i GitHub Release

```bash
git tag v4.1.0            # tag = wersja semver (prefiks v dozwolony; parser
git push origin v4.1.0    # semver w firmware akceptuje oba warianty)
gh release create v4.1.0 \
    firmware/release_artifacts/firmware.bin \
    firmware/release_artifacts/firmware.elf \
    --title "v4.1.0 OTA-GITHUB" --notes-file CHANGELOG-wpis.md
```

Wymagania OTA:
- asset musi nazywać się **`firmware.bin`** (adres:
  `https://github.com/zielinski25/ryby-led/releases/download/{tag}/firmware.bin`),
- release **nie może być draft/prerelease** (firmware pyta
  `releases?per_page=1` — drafty są niewidoczne bez autoryzacji),
- `firmware.elf` dołączany dla addr2line przy dekodowaniu crashy (lekcja v239).

## 3. Po release

- Urządzenia z nowym firmware zgłoszą się same (karta panelu / `/update`
  w Telegramie / komenda `update` w Firebase) — firmware porównuje semver
  i flashuje tylko wersje nowsze (downgrade wyłącznie `?force=1`).
- Po pierwszym boocie nowej wersji `setup()` potwierdza partycję OTA
  (anty-rollback) — sprawdź w logach `tag=OTA-GH`.

## 4. Cofnięcie błędnego release

1. Usuń release i tag (`gh release delete vX.Y.Z` + `git push :vX.Y.Z`),
2. wystaw poprzedni tag jako nowy release (firmware nie cofa się sam —
   wymuś na urządzeniu `POST /api/ota-start?force=1` lub espota).
