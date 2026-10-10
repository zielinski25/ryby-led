# ryby-led

Sterownik oświetlenia LED akwarium (**Ryby LED**) — ESP32-S3 N16R8,
5 kanałów PWM, adaptacja wg czujników TSL2561, harmonogram, panel WWW,
Firebase + Telegram. Projekt rozwijany do poziomu „Centrali Pieca”
(repo `zielinski25/Sterownik-Pieca-C.O.`).

## Struktura repo (od Etapu 0 planu upgrade, 2026-10-09)

```
firmware/            ← projekt PlatformIO (env: esp32-s3-n16r8 / -ota)
  src/Ryby_LED_fi_S3.cpp    firmware główny (semver od v4.0.0+build.261)
  src/terminal_html.cpp/.h  panel WWW (PROGMEM)
  src/NetDiag.h             diagnostyka sieci/routera
  boards/ partitions.csv platformio.ini
  scripts/build_gate.ps1    bramka przed release (build + spójność wersji)
panel/               ← panel zdalny (do uzupełnienia — patrz Etap 4 planu)
docs/                ← research, plan upgrade, plany wdrożeniowe
CHANGELOG.md         ← pełna historia zmian (semver + archiwum vNNN)
version.txt          ← wersja semver, spójna z RYBY_FW_VERSION
```

## Dokumentacja

- [`docs/01_RESEARCH_porownanie_ryby_vs_centrala.md`](docs/01_RESEARCH_porownanie_ryby_vs_centrala.md)
  — głęboki research porównawczy Ryby LED vs Centrala Pieca (macierz luk G1–G15).
- [`docs/02_PLAN_UPGRADE_ryby_led.md`](docs/02_PLAN_UPGRADE_ryby_led.md)
  — plan upgrade'u (etapy 0–7, priorytety, DoD, zasady bezpieczeństwa zmian).
- [`docs/PLAN_WDROZENIOWY_lwip_sntp_crash.md`](docs/PLAN_WDROZENIOWY_lwip_sntp_crash.md)
  — historyczny plan naprawy crasha lwIP/SNTP.

## Wersjonowanie

Od 2026-10-09 obowiązuje **semver**; mapowanie z numeracji wewnętrznej:
`v261 → 4.0.0+build.261`. Historia wewnętrzna (v33b…v261) zachowana 1:1
w [`CHANGELOG.md`](CHANGELOG.md). Release = tag semver + assety
`firmware.bin` i `firmware.elf` (do addr2line).

## Budowanie

```
cd firmware
pio run -e esp32-s3-n16r8          # USB
pio run -e esp32-s3-n16r8-ota -t upload   # OTA (espota)
.\scripts\build_gate.ps1           # przed każdym release
```
