// ═══════════════════════════════════════════════════════════════════════════
//  critlog.h — [4.4.0 CRIT-LOG] trwały log zdarzeń krytycznych (Etap 5 planu)
//
//  Wzór: log_krytyczny.txt z Centrali Pieca (v3.35.0). Osobny plik na LittleFS,
//  którego NIE rusza rotacja log_a/log_b. Zdarzenia tu trafiają tylko z miejsc
//  jawnie krytycznych: start po PANIC/WDT/BROWNOUT, utrata czujników I2C/DS18B20,
//  błędy OTA. Plik przeżywa restart i rotację zwykłego logu.
//
//  Limit: MAX_BYTES na plik bieżący. Po przekroczeniu bieżący plik zmienia nazwę
//  na FILE_OLD (nadpisując poprzedni archiwalny) i zaczyna nowy. Łącznie na flash
//  najwyżej 2 * MAX_BYTES = 512 KB (SPIFFS ma ~9,7 MB).
//
//  Moduł nie zna Ryby: tylko LittleFS + opcjonalny mutex FreeRTOS. Testy hostowe:
//  firmware/tests/host/test_critlog.cpp.
// ═══════════════════════════════════════════════════════════════════════════
#pragma once
#include <stddef.h>

namespace critlog {

constexpr const char* FILE_CUR  = "/log_krytyczny.txt";
constexpr const char* FILE_OLD  = "/log_krytyczny_old.txt";
constexpr size_t      MAX_BYTES = 256 * 1024;
constexpr size_t      MAX_LINE  = 1000;   // dłuższe linie są ucinane (ochrona przed rozrostem)

// Wołać raz po LittleFS.begin(). fsReady=false → append() zwraca false, nic nie zapisuje.
void init(bool fsReady);

// Dopisuje jedną linię (dokłada '\n', jeśli brak). Rotuje plik przy przekroczeniu MAX_BYTES.
// Zwraca true, gdy linia trafiła na flash.
bool append(const char* line);

// Rozmiar bieżącego pliku (bajty), aktualizowany przy append()/init().
size_t currentSize();

// Czy istnieje archiwum (FILE_OLD).
bool hasOld();

}  // namespace critlog
