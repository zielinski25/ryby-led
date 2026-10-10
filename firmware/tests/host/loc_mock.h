// ═══════════════════════════════════════════════════════════════════════════
//  loc_mock.h — atrapy ESP32 dla testu lokalizacji (4.7.0 ASTRO, Etap 6).
//  Preferences (NVS) jako mapa w pamięci, AsyncWebServer jako rejestr handlerów,
//  logPrintf jako no-op. Nie udaje sprzętu — sprawdza logikę endpointów.
// ═══════════════════════════════════════════════════════════════════════════
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <time.h>
#include <math.h>
#include "astro.h"

#define HTTP_GET  1
#define HTTP_POST 2

// Stan atrapy NVS (namespace/klucz -> wartość).
static std::map<std::string, float> g_nvs;
static bool g_nvsFail = false;

class Preferences {
 public:
  bool begin(const char* ns, bool readOnly) {
    (void)readOnly;
    if (g_nvsFail) return false;
    ns_ = ns;
    return true;
  }
  float getFloat(const char* key, float def) {
    auto it = g_nvs.find(ns_ + "/" + key);
    return it == g_nvs.end() ? def : it->second;
  }
  size_t putFloat(const char* key, float v) {
    g_nvs[ns_ + "/" + key] = v;
    return sizeof(float);
  }
  bool isKey(const char* key) const { return g_nvs.count(ns_ + "/" + key) > 0; }
  void end() {}
 private:
  std::string ns_;
};

// Odpowiedź serwera: ostatni kod i treść (do asercji w teście).
struct Resp { int code = 0; std::string body; int sends = 0; };
static Resp g_resp;

struct AsyncWebServerRequest {
  void send(int code, const char* type, const char* body) {
    (void)type;
    g_resp.code = code;
    g_resp.body = body ? body : "";
    g_resp.sends++;
  }
  void send(int code, const char* type, const std::string& body) { send(code, type, body.c_str()); }
};

struct WebServerMock {
  using Handler = std::function<void(AsyncWebServerRequest*)>;
  using Body    = std::function<void(AsyncWebServerRequest*, uint8_t*, size_t, size_t, size_t)>;
  std::map<std::string, Handler> getH;
  std::map<std::string, Body>    postH;

  void on(const char* path, int method, Handler h) {
    if (method == HTTP_GET) getH[path] = h;
  }
  template <class Upload>
  void on(const char* path, int method, Handler, Upload, Body b) {
    if (method == HTTP_POST) postH[path] = b;
  }
};

static WebServerMock webserialServer;
static int sunsetMinutes = 1140;  // 19:00 — jak domyślnie w firmware

static void logPrintf(const char* format, ...) { (void)format; }
