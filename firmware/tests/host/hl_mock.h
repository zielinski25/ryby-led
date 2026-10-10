// ═══════════════════════════════════════════════════════════════════════════
//  hl_mock.h — atrapy do kompilacji bloku [4.5.0 HIST-LONG] (Etap 3, G8):
//  endpoint /api/history/long oraz hak w saveHistoryPoint(). Tylko sygnatury.
// ═══════════════════════════════════════════════════════════════════════════
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "Arduino.h"      // millis()
#include "esp_system.h"
#include "LittleFS.h"

#define HTTP_GET 1
#define HTTP_POST 2

// AsyncURIMatcher::exact — sygnatura z ESPAsyncWebServer v3.10.3 (ESPAsyncWebServer.h ~l.1023).
struct AsyncURIMatcher {
  static AsyncURIMatcher exact(const char*) { return AsyncURIMatcher(); }
};

struct AsyncWebServerResponse {
  void addHeader(const char*, const char*) {}
};

struct AsyncWebServerRequest {
  void send(int, const char*, const char*) {}
  void send(AsyncWebServerResponse*) {}
  template <typename F>
  AsyncWebServerResponse* beginChunkedResponse(const char*, F&&) {
    static AsyncWebServerResponse r;
    return &r;
  }
};

struct WebSerialMock {
  template <typename M, typename F>
  void on(const M&, int, F&&) {}
};
static WebSerialMock webserialServer;

// Globalne używane przez hak w saveHistoryPoint() (typy zgodne z Ryby_LED_fi_S3.cpp).
static float tempWater = 20.0f, tempPlate1 = 25.0f, tempPlate2 = 25.0f;
static volatile float luxPokojowy = 0, luxNadWoda = 0;
static bool tryb = false, minLuxModeActive = false, regulacjaAdaptacyjnaWlaczona = false;
