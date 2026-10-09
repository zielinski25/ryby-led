# ryby-led

Sterownik oświetlenia LED akwarium (ESP32-S3 N16R8) — firmware, panel WWW
i dokumentacja rozwoju w kierunku poziomu „Centrali Pieca”.

## Dokumentacja (aktualna sesja)

- [`docs/01_RESEARCH_porownanie_ryby_vs_centrala.md`](docs/01_RESEARCH_porownanie_ryby_vs_centrala.md)
  — głęboki research porównawczy: Ryby LED vs Centrala Pieca (inwentaryzacja
  obu projektów + macierz luk / gap analysis).
- [`docs/02_PLAN_UPGRADE_ryby_led.md`](docs/02_PLAN_UPGRADE_ryby_led.md)
  — plan upgrade'u Ryb do poziomu Centrali (etapy 0–7, priorytety, szacunki,
  zasady bezpieczeństwa zmian).

## Źródła

- `ryby.zip` — archiwum z firmware v261 (`Ryby_LED_fi_S3.cpp`, panel
  `terminal_html.cpp`, `NetDiag.h`, konfiguracja PlatformIO). Docelowo zostanie
  rozpakowane do struktury `firmware/panel/docs` wg Etapu 0 planu upgrade'u.

## Wersja

`version.txt` = 107 (ostatni publiczny release); firmware w `ryby.zip` to
**v261** (2026-09-27). Rozbieżność zostanie zsynchronizowana w Etapie 0
planu (mapowanie v261 → 4.0.0 semver).
