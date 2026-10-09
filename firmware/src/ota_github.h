#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// ota_github.h — [4.1.0 OTA-GITHUB] Etap 1 planu upgrade (docs/02_PLAN_UPGRADE)
// ═══════════════════════════════════════════════════════════════════════════
// OTA przez GitHub Releases — port 1:1 sprawdzonego mechanizmu z Centrali
// Pieca (main_centrala.cpp: sprawdzNajnowszaWersjeGithub / porownajWersje /
// wykonajAktualizacjeOTA), zaadaptowany do architektury Ryb:
//
//   - cały ruch sieciowy OTA żyje we WŁASNYM tasku na Core 0 (reguła projektu:
//     sieć tylko na Core 0), stos w DRAM (flash-write wyłącza cache -> stos
//     w PSRAM byłby błędem, patrz komentarz przy tgTaskFn),
//   - wyzwalacze: POST /api/ota-start (panel), komenda /update (Telegram),
//     komenda "update" (Firebase /aquarium/commands) — wszystkie prowadzą do
//     otaGithubRequest(), które tylko startuje task (nie blokuje wywołującego),
//   - anty-rollback: otaGithubConfirmPartition() wołane w setup() potwierdza
//     partycję OTA (esp_ota_mark_app_valid_cancel_rollback) — bez tego drugi
//     boot po aktualizacji cofnąłby się na starą partycję,
//   - semver: porównanie MAJOR.MINOR.PATCH, nie leksykograficzne (port
//     FIX-OTA-DOWNGRADE z Centrali); downgrade tylko z force=1.
//
// Moduł celowo SAMODZIELNY: jedyny styk z Ryby_LED_fi_S3.cpp to 1 include,
// 3 wywołania w setup() i po jednej gałęzi w parserach Telegram/Firebase/WS.
// ═══════════════════════════════════════════════════════════════════════════
#include <Arduino.h>

// setup(): potwierdź partycję po ostatnim OTA (anty-rollback). Wołać WCZEŚNIE.
void otaGithubConfirmPartition();

// setup(): podaj wersję bieżącą (FW_VERSION) do porównań semver.
void otaGithubSetCurrentVersion(const char* version);

// setup(): zarejestruj endpointy GET /api/ota-status i POST /api/ota-start
// na webserialServer (po wszystkich innych .on(), przed begin()).
void otaGithubRegisterEndpoints();

// Żądanie startu OTA (nieblokujące). force=true -> flash nawet gdy tag nie
// jest nowszy (downgrade/reinstall). Zwraca false gdy OTA już trwa albo
// nie udało się stworzyć taska (szczegóły w errOut).
bool otaGithubRequest(bool force, String &errOut);

// JSON statusu dla panelu/WS: stan taska, postęp, ostatni tag/wynik/błąd.
String otaGithubStatusJson();
