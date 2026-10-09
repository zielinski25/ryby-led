// ═══════════════════════════════════════════════════════════════════════════
// ota_github.cpp — [4.1.0 OTA-GITHUB] Etap 1 planu upgrade
// ═══════════════════════════════════════════════════════════════════════════
// IMPLEMENTACJA — patrz ota_github.h po opis architektury i uzasadnienia.
//
// Historia zmian:
// [4.1.0 OTA-GITHUB, 2026-10-09] Pierwsza wersja: port OTA-GitHub z Centrali
//   Pieca (wzorce: otaGithubApiUrl/sprawdzNajnowszaWersjeGithub/porownajWersje/
//   wykonajAktualizacjeOTA). Logika sterowania LED, adaptacji, harmonogramu,
//   MIN LUX i ramp NIETKNIETA — moduł w 100% samodzielny.
// ═══════════════════════════════════════════════════════════════════════════

#include "ota_github.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>      // [4.1.0] osobny TU — celowo NIE w Ryby_LED_fi_S3.cpp
#include <HTTPUpdate.h>      //        (tam usunięty w v105-FC; tu niezależny od Firebase)
#include <Update.h>
#include <esp_ota_ops.h>     // esp_ota_get_state_partition / mark_app_valid_cancel_rollback (IDF 5.5)
#include <esp_task_wdt.h>    // rejestracja/karmienie TWDT własnego taska
#include <ESPAsyncWebServer.h>

// ── styk z resztą firmware (symbole z Ryby_LED_fi_S3.cpp) ──────────────────
extern AsyncWebServer webserialServer;   // port 8080
extern unsigned long restartRequestedAt; // flaga restartu obsługiwana przez loop()
extern void logPrintf(const char* format, ...);
extern void logPrintln(const String &msg);

// ── konfiguracja repozytorium release ───────────────────────────────────────
#define OTA_GH_OWNER  "zielinski25"
#define OTA_GH_REPO   "ryby-led"
#define OTA_GH_ASSET  "firmware.bin"
#define OTA_GH_UA     "RybyLED-ESP32S3"   // api.github.com WYMAGA User-Agent (inaczej 403)

// ── stan modułu ─────────────────────────────────────────────────────────────
static volatile bool     s_otaRunning   = false;  // task OTA aktywny (guard single-instance)
static volatile int      s_progressPct  = -1;     // -1 = brak aktywnego pobierania
static volatile bool     s_lastOk       = false;  // wynik ostatniego podejścia
static volatile unsigned long s_lastFinishMs = 0;
static char              s_lastTag[32]  = {0};    // tag ostatnio sprawdzony/pobrany
static char              s_lastError[96] = {0};   // ostatni błąd ("" = brak)
static char              s_currentVer[48] = {0};  // FW_VERSION podstawione w setup()
static TaskHandle_t      s_otaTaskHandle = nullptr;

// ═══════════════════════════════════════════════════════════════════════════
// POMOCNICZE
// ═══════════════════════════════════════════════════════════════════════════

// [port 1:1 z Centrali: porownajWersje, FIX-OTA-DOWNGRADE] Semver
// (MAJOR.MINOR.PATCH), NIE porównanie leksykograficzne stringów.
// Akceptuje prefiks v/V i brakujące segmenty; build-metadata po '+' ignorowane
// (toInt() na "0+build" zwraca 0). Zwraca >0 gdy a nowsza, 0 gdy równe, <0 gdy starsza.
static int otaPorownajWersje(const String &a, const String &b) {
  auto segment = [](const String &s, int idx) -> long {
    String v = s;
    if (v.length() && (v[0] == 'v' || v[0] == 'V')) v = v.substring(1);
    int start = 0;
    for (int i = 0; i < idx; i++) {
      int dot = v.indexOf('.', start);
      if (dot < 0) return 0;
      start = dot + 1;
    }
    int dot = v.indexOf('.', start);
    String part = (dot < 0) ? v.substring(start) : v.substring(start, dot);
    return part.toInt();
  };
  for (int i = 0; i < 3; i++) {
    long va = segment(a, i);
    long vb = segment(b, i);
    if (va != vb) return (va > vb) ? 1 : -1;
  }
  return 0;
}

// [port 1:1 z Centrali: otaGithubApiUrl — wariant releases?per_page=1]
// /releases/latest CICHO pomija prerelease; per_page=1 zwraca najnowszy wpis
// niezależnie od flag. Parsowanie odpowiedzi jest ręczne (indexOf tag_name),
// więc opakowanie w tablicę [...] nic nie zmienia.
static String otaGithubApiUrl() {
  return String("https://api.github.com/repos/") + OTA_GH_OWNER + "/" + OTA_GH_REPO + "/releases?per_page=1";
}

// [port 1:1 z Centrali: otaGithubDownloadUrl — permalink po tagu, nie /latest/]
static String otaGithubDownloadUrl(const String &tag) {
  return String("https://github.com/") + OTA_GH_OWNER + "/" + OTA_GH_REPO +
         "/releases/download/" + tag + "/" + OTA_GH_ASSET;
}

static void otaSetLastError(const char* msg) {
  strncpy(s_lastError, msg, sizeof(s_lastError) - 1);
  s_lastError[sizeof(s_lastError) - 1] = '\0';
}

// ── sprawdzenie najnowszego tagu BEZ pobierania binarki ────────────────────
// [port: sprawdzNajnowszaWersjeGithub] Ręczne parsowanie "tag_name":"..." —
// bez ArduinoJson, body rośnie dynamicznie (String), więc liczba assetów
// w release nie ma wpływu (lekcja SYNC-OTA/11.1 Centrali).
static bool otaSprawdzNajnowszyTag(String &tagOut, String &bladOut) {
  WiFiClientSecure client;
  client.setInsecure();     // jak w Centrali dla OTA; weryfikacja TLS = osobny etap planu
  client.setTimeout(8000);

  HTTPClient http;
  http.setUserAgent(OTA_GH_UA);
  http.setTimeout(8000);

  if (!http.begin(client, otaGithubApiUrl())) {
    bladOut = "http.begin() nieudane";
    return false;
  }

  esp_task_wdt_reset();
  int kodHttp = http.GET();
  esp_task_wdt_reset();

  if (kodHttp != 200) {
    bladOut = "GitHub API zwrocilo kod " + String(kodHttp);
    http.end();
    return false;
  }

  String body = http.getString();
  http.end();
  esp_task_wdt_reset();

  int p = body.indexOf("\"tag_name\":\"");
  if (p < 0) { bladOut = "Brak pola tag_name w odpowiedzi"; return false; }
  p += strlen("\"tag_name\":\"");
  int e = body.indexOf('"', p);
  if (e < 0) { bladOut = "Nie udalo sie sparsowac tag_name"; return false; }

  tagOut = body.substring(p, e);
  return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// TASK OTA (Core 0, stos w DRAM — patrz komentarz przy tgTaskFn: flash-write
// wyłącza cache, stos w PSRAM = assert esp_task_stack_is_sane_cache_disabled)
// ═══════════════════════════════════════════════════════════════════════════
static void otaGithubTaskFn(void* pvParams) {
  const bool force = (pvParams != nullptr);

  esp_task_wdt_add(NULL);            // ten task karmi TWDT sam (timeout z setup(): 15s)
  s_progressPct = -1;
  otaSetLastError("");
  logPrintln("lvl=INFO tag=OTA-GH msg=\"Start sprawdzenia release\" force=" + String(force ? 1 : 0));

  // ── 1. najnowszy tag ──
  String tag, blad;
  if (!otaSprawdzNajnowszyTag(tag, blad)) {
    otaSetLastError(blad.c_str());
    logPrintln("lvl=WARN tag=OTA-GH msg=\"Blad sprawdzenia release\" err=" + blad);
    s_lastOk = false; s_lastFinishMs = millis();
    s_otaRunning = false; s_progressPct = -1;
    esp_task_wdt_delete(NULL);
    vTaskDelete(NULL);
    return;
  }
  strncpy(s_lastTag, tag.c_str(), sizeof(s_lastTag) - 1);
  s_lastTag[sizeof(s_lastTag) - 1] = '\0';
  logPrintln("lvl=INFO tag=OTA-GH msg=\"Najnowszy release\" tag=" + tag + " obecna=" + String(s_currentVer));

  // ── 2. porównanie semver (downgrade tylko z force) ──
  if (!force && otaPorownajWersje(tag, String(s_currentVer)) <= 0) {
    logPrintln("lvl=INFO tag=OTA-GH msg=\"Brak nowszej wersji - koncze bez flashowania\"");
    s_lastOk = true; s_lastFinishMs = millis();
    s_otaRunning = false; s_progressPct = -1;
    esp_task_wdt_delete(NULL);
    vTaskDelete(NULL);
    return;
  }

  // ── 3. pobranie + flash (HTTPUpdate: sam robi Update.begin/write/end
  //        i obsługuje 302 github.com -> host assetów; wzór: Centrala) ──
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(15000);

  httpUpdate.rebootOnUpdate(false);  // restart robimy przez restartRequestedAt (czysty
                                     // restart w loop() z PRE_RESET_UPDATE + flush logów)
  httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  httpUpdate.onProgress([](int cur, int total) {
    esp_task_wdt_reset();            // karmienie TWDT w trakcie zapisu flash
    if (total > 0) s_progressPct = (int)((100L * cur) / total);
  });

  esp_task_wdt_reset();
  t_httpUpdate_return wynik = httpUpdate.update(client, otaGithubDownloadUrl(tag));
  esp_task_wdt_reset();

  switch (wynik) {
    case HTTP_UPDATE_OK: {
      s_lastOk = true; s_lastFinishMs = millis(); s_progressPct = 100;
      logPrintln("lvl=INFO tag=OTA-GH msg=\"Flash OK - restart za ~1s\" tag=" + tag);
      // Restart przez istniejący mechanizm (loop() wykona PRE_RESET_UPDATE(0)
      // + logForceFlush + ESP.restart) — ta sama ścieżka co /api/quick/restart.
      // Potwierdzenie nowej partycji (anty-rollback) wykona otaGithubConfirmPartition()
      // przy NASTĘPNYM starcie, w setup().
      restartRequestedAt = millis();
      s_otaRunning = false;
      esp_task_wdt_delete(NULL);
      vTaskDelete(NULL);
      return;
    }
    case HTTP_UPDATE_FAILED: {
      String b = String("Blad OTA (") + httpUpdate.getLastErrorString().c_str() + ")";
      otaSetLastError(b.c_str());
      logPrintln("lvl=ERR tag=OTA-GH msg=\"" + b + "\"");
      break;
    }
    case HTTP_UPDATE_NO_UPDATES:
      otaSetLastError("Serwer zglosil brak aktualizacji");
      logPrintln("lvl=WARN tag=OTA-GH msg=\"HTTP_UPDATE_NO_UPDATES\"");
      break;
    default:
      otaSetLastError("Nieznany wynik HTTPUpdate");
      logPrintln("lvl=WARN tag=OTA-GH msg=\"Nieznany wynik HTTPUpdate\"");
      break;
  }

  s_lastOk = false; s_lastFinishMs = millis();
  s_otaRunning = false; s_progressPct = -1;
  esp_task_wdt_delete(NULL);
  vTaskDelete(NULL);
}

// ═══════════════════════════════════════════════════════════════════════════
// API MODUŁU
// ═══════════════════════════════════════════════════════════════════════════

bool otaGithubRequest(bool force, String &errOut) {
  if (s_otaRunning) {
    errOut = "OTA juz trwa";
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    errOut = "Brak polaczenia WiFi";
    return false;
  }
  s_otaRunning = true;

  // Stos 16KB w DRAM (MALLOC_CAP_INTERNAL) — wzór tgTaskFn: stos taska MUSI
  // być w DRAM, nigdy w PSRAM (flash-write wyłącza cache). TLS + HTTPUpdate
  // potrzebują głębszego stosu niż tgTask (12KB) -> 16KB z zapasem.
  static StaticTask_t otaTaskTCB;
  static uint8_t* otaStackBuf = nullptr;
  const uint32_t OTA_STACK = 16384;
  if (!otaStackBuf) {
    otaStackBuf = (uint8_t*)heap_caps_malloc(OTA_STACK, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  }
  if (otaStackBuf) {
    s_otaTaskHandle = xTaskCreateStaticPinnedToCore(
      otaGithubTaskFn, "otaGh", OTA_STACK, (void*)(force ? 1 : 0), 1,
      otaStackBuf, &otaTaskTCB, 0
    );
  } else {
    // Fallback — standardowy xTaskCreate (DRAM), jak fallback tgTask.
    xTaskCreatePinnedToCore(otaGithubTaskFn, "otaGh", OTA_STACK, (void*)(force ? 1 : 0),
                            1, &s_otaTaskHandle, 0);
  }
  if (s_otaTaskHandle == nullptr) {
    s_otaRunning = false;
    errOut = "Nie udalo sie stworzyc taska OTA (brak pamieci)";
    logPrintln("lvl=ERR tag=OTA-GH msg=\"Task OTA nie wystartowal - brak pamieci\"");
    return false;
  }
  errOut = "";
  return true;
}

String otaGithubStatusJson() {
  String j = "{";
  j += "\"running\":"; j += s_otaRunning ? "true" : "false";
  j += ",\"progress\":"; j += String(s_progressPct);
  j += ",\"last_ok\":"; j += s_lastOk ? "true" : "false";
  j += ",\"last_finish_ms\":"; j += String((unsigned long)s_lastFinishMs);
  j += ",\"last_tag\":\""; j += s_lastTag; j += "\"";
  j += ",\"last_error\":\""; j += s_lastError; j += "\"";
  j += ",\"current\":\""; j += s_currentVer; j += "\"";
  j += "}";
  return j;
}

void otaGithubSetCurrentVersion(const char* version) {
  strncpy(s_currentVer, version ? version : "", sizeof(s_currentVer) - 1);
  s_currentVer[sizeof(s_currentVer) - 1] = '\0';
}

void otaGithubConfirmPartition() {
  // [anty-rollback, wzór Centrala potwierdzPartycjeJesliCzeka()] Po starcie
  // z partycji OTA bootloader czeka na esp_ota_mark_app_valid_cancel_rollback();
  // bez potwierdzenia NASTĘPNY restart cofnąłby się na starą partycję.
  const esp_partition_t* running = esp_ota_get_running_partition();
  if (!running) return;
  esp_ota_img_states_t state = ESP_OTA_IMG_UNDEFINED;
  if (esp_ota_get_state_partition(running, &state) != ESP_OK) return;
  if (state == ESP_OTA_IMG_PENDING_VERIFY) {
    if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
      logPrintln("lvl=INFO tag=OTA-GH msg=\"Partycja OTA potwierdzona (cancel rollback)\"");
    } else {
      logPrintln("lvl=ERR tag=OTA-GH msg=\"esp_ota_mark_app_valid_cancel_rollback nie powiodl sie\"");
    }
  }
}

void otaGithubRegisterEndpoints() {
  // GET /api/ota-status — stan OTA (panel WWW / WS / diagnostyka).
  webserialServer.on("/api/ota-status", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", otaGithubStatusJson());
  });

  // POST /api/ota-start[?force=1] — start aktualizacji (task na Core 0).
  webserialServer.on("/api/ota-start", HTTP_POST, [](AsyncWebServerRequest *request) {
    bool force = request->hasParam("force", true) &&
                 request->getParam("force", true)->value() == "1";
    String err;
    bool ok = otaGithubRequest(force, err);
    if (ok) {
      request->send(202, "application/json",
                    String("{\"started\":true,\"force\":") + (force ? "true" : "false") + "}");
    } else {
      request->send(409, "application/json",
                    String("{\"started\":false,\"error\":\"") + err + "\"}");
    }
  });
}
