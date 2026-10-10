// ═══════════════════════════════════════════════════════════════════════════
//  e5_mock.h — atrapy Ryby/Arduino/AsyncWebServer do kompilacji bloku
//  [4.4.0 CRIT-LOG] (Etap 5) w izolacji: log krytyczny + endpointy + helper OTA.
//  Nie implementują zachowania — tylko typy i sygnatury, żeby łapać literówki.
// ═══════════════════════════════════════════════════════════════════════════
#pragma once
#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <string>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include "Arduino.h"
#include "esp_system.h"
#include "LittleFS.h"

// ── String (podzbiór API Arduino String używany w bloku) ────────────────────
struct String {
  std::string s;
  String() {}
  String(const char* c) : s(c ? c : "") {}
  String(const std::string& x) : s(x) {}
  String(int v) : s(std::to_string(v)) {}
  String(long v) : s(std::to_string(v)) {}
  String(unsigned long v) : s(std::to_string(v)) {}
  String(float v, int dec) { char b[32]; snprintf(b, sizeof(b), "%.*f", dec, (double)v); s = b; }
  const char* c_str() const { return s.c_str(); }
  size_t length() const { return s.size(); }
  int indexOf(const char* needle) const { size_t p = s.find(needle); return p == std::string::npos ? -1 : (int)p; }
};
inline String operator+(const String& a, const String& b) { return String(a.s + b.s); }
inline String& operator+=(String& a, const String& b) { a.s += b.s; return a; }
inline bool operator==(const String& a, const String& b) { return a.s == b.s; }

// ── Serial/log ──────────────────────────────────────────────────────────────
static inline void logPrintln(const String&) {}
static inline void logPrintln(const char*) {}
static inline void logPrintf(const char*, ...) {}

// ── czas / sieć / Telegram (sygnatury zgodne z Ryby_LED_fi_S3.cpp) ─────────
static inline void delay(uint32_t) {}
static inline bool getLocalTimePL(struct tm*) { return false; }
static inline String logTime() { return String("2026-10-10 00:00:00"); }
static inline void esp_task_wdt_reset() {}
struct EspHeap { unsigned long getFreeHeap() const { return 0; } };
static EspHeap ESP;
#define HTTP_GET 1
#define HTTP_POST 2

static bool tgEnabled = true;
static String tgBotToken = "123456:TESTTOKEN_0123456789";
static String tgChatId = "1234";
static bool littlefsReady = true;
static bool tgClientReady = false;
static bool sendTelegramMessage(const String&, bool) { return true; }
static bool tgEnsureConnected() { return true; }
static void tgDeleteAllHistory() {}
static unsigned long menuReturnInTask = 0;

// ── WiFiClientSecure (podzbiór używany przez sendTelegramCriticalDocument) ──
struct WiFiClientSecureMock {
  int fd() const { return -1; }
  void printf(const char*, ...) {}
  void print(const String&) {}
  size_t write(const uint8_t*, size_t n) { return n; }
  int read() { return -1; }
  int available() const { return 0; }
  bool connected() const { return false; }
  void stop() {}
};
static WiFiClientSecureMock tgClient;

// ── AsyncWebServer (podzbiór) ───────────────────────────────────────────────
struct AwsParam { String v; String value() const { return v; } };
struct AsyncWebServerRequest {
  bool hasParam(const char*, bool = false, bool = false) const { return false; }
  AwsParam* getParam(const char*, bool = false, bool = false) const { static AwsParam p; return &p; }
  void send(int, const char*, const char*) {}
  void send(int, const char*, const String&) {}
  void send(int, const char*) {}
  template <typename FS> void send(FS&, const char*, const char*, bool = false) {}
};
struct WebSerialMock {
  template <typename F> void on(const char*, int, F&&) {}
};
static WebSerialMock webserialServer;

// ── Helper z ota_github.cpp (sygnatura identyczna) — kompilowany osobno ────
