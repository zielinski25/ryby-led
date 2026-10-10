// ═══════════════════════════════════════════════════════════════════════════
//  critlog.cpp — [4.4.0 CRIT-LOG] patrz critlog.h
// ═══════════════════════════════════════════════════════════════════════════
#include "critlog.h"

#include <LittleFS.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
static SemaphoreHandle_t s_mtx = nullptr;
static void critLock()   { if (s_mtx) xSemaphoreTake(s_mtx, pdMS_TO_TICKS(500)); }
static void critUnlock() { if (s_mtx) xSemaphoreGive(s_mtx); }
#else
// Host (testy): jeden wątek, bez blokad.
static void critLock()   {}
static void critUnlock() {}
#endif

namespace critlog {

static bool  s_ready = false;
static size_t s_size = 0;

void init(bool fsReady) {
#ifdef ESP_PLATFORM
  if (!s_mtx) s_mtx = xSemaphoreCreateMutex();
#endif
  s_ready = fsReady;
  s_size  = 0;
  if (!s_ready) return;
  if (LittleFS.exists(FILE_CUR)) {
    File f = LittleFS.open(FILE_CUR, "r");
    if (f) { s_size = f.size(); f.close(); }
  }
}

size_t currentSize() { return s_size; }

bool hasOld() { return s_ready && LittleFS.exists(FILE_OLD); }

// Wywoływane pod blokadą.
static void rotateLocked() {
  if (LittleFS.exists(FILE_OLD)) LittleFS.remove(FILE_OLD);
  LittleFS.rename(FILE_CUR, FILE_OLD);
  s_size = 0;
}

bool append(const char* line) {
  if (!s_ready || !line) return false;

  size_t n = strlen(line);
  if (n > MAX_LINE) n = MAX_LINE;
  const bool hasNl = (n > 0 && line[n - 1] == '\n');
  const size_t add = n + (hasNl ? 0 : 1);

  bool ok = false;
  critLock();
  if (s_size > 0 && s_size + add > MAX_BYTES) rotateLocked();

  File f = LittleFS.open(FILE_CUR, "a");
  if (f) {
    f.write(reinterpret_cast<const uint8_t*>(line), n);
    if (!hasNl) f.write(reinterpret_cast<const uint8_t*>("\n"), 1);
    s_size = f.size();
    f.close();
    ok = true;
  }
  critUnlock();
  return ok;
}

}  // namespace critlog
