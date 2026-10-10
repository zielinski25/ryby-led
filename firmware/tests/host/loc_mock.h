// ═══════════════════════════════════════════════════════════════════════════
//  loc_mock.h — atrapy ESP32 dla testu astro/poranka/RTC (4.7.0, Etap 6).
//  NVS jako mapa, serwer HTTP jako rejestr handlerów, Wire jako fałszywy DS
//  pod 0x68 z rejestrami, zegar systemowy sterowany przez g_fakeNow
//  (time()/settimeofday() są przekierowane). Nie udaje sprzętu.
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
#include "rtc_ds13xx.h"

// ── Zegar systemowy ─────────────────────────────────────────────────────────
#include <sys/time.h>
static time_t g_fakeNow = 0;  // 0 = brak NTP
static time_t fake_time(time_t* p) { if (p) *p = g_fakeNow; return g_fakeNow; }
#define time(p) fake_time(p)
static int fake_settimeofday(const struct timeval* tv) {
  g_fakeNow = (time_t)tv->tv_sec;
  return 0;
}
#define settimeofday(tv, tz) fake_settimeofday(tv)
static const int timezoneOffsetMinutes = 120;
static bool isDaylightSaving(time_t) { return true; }
static bool getLocalTimePL(struct tm* ti) {
  if (g_fakeNow < 1700000000L) return false;
  time_t t = g_fakeNow + 7200;  // CEST
  *ti = *gmtime(&t);
  return true;
}
static bool isWeekend() {
  struct tm ti;
  if (!getLocalTimePL(&ti)) return false;
  return ti.tm_wday == 0 || ti.tm_wday == 6;
}
static int32_t MORNING_ON_START_WEEKDAY = 7 * 60;
static int32_t MORNING_ON_START_WEEKEND = 8 * 60;
static int obliczZachodSlonca(int, int, int, float, float) { return 1020; }  // stub (UTC)

// ── NVS (Preferences) ───────────────────────────────────────────────────────
static std::map<std::string, float> g_nvs;   // float: lat, lon
static std::map<std::string, int> g_nvsInt;  // int: mrMode, mrOff, rtcChip
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
  int getInt(const char* key, int def) {
    auto it = g_nvsInt.find(ns_ + "/" + key);
    return it == g_nvsInt.end() ? def : it->second;
  }
  size_t putInt(const char* key, int v) {
    g_nvsInt[ns_ + "/" + key] = v;
    return sizeof(int);
  }
  bool isKey(const char* key) const { return g_nvs.count(ns_ + "/" + key) > 0; }
  void end() {}
 private:
  std::string ns_;
};

// ── Wire: fałszywy DS1307/DS3231 pod 0x68 ───────────────────────────────────
struct TwoWireMock {
  uint8_t regs[16] = {0};
  bool present = true;       // ACK na adres 0x68
  bool nackWrite = false;    // błąd zapisu (NACK po wysłaniu)
  int addr = -1;
  std::vector<uint8_t> tx;
  std::vector<uint8_t> rx;
  size_t rxPos = 0;
  uint8_t regPtr = 0;
  int writes = 0;

  void beginTransmission(uint8_t a) { addr = a; tx.clear(); }
  size_t write(uint8_t b) { tx.push_back(b); return 1; }
  size_t write(const uint8_t* buf, size_t n) {
    for (size_t i = 0; i < n; i++) tx.push_back(buf[i]);
    return n;
  }
  uint8_t endTransmission(bool sendStop = true) {
    (void)sendStop;
    if (addr != rtc13::kAddr || !present) return 2;  // NACK adresu
    if (tx.empty()) return 0;                        // sama próba (probe)
    regPtr = tx[0];
    if (tx.size() > 1) {
      if (nackWrite) return 3;                       // NACK danych
      writes++;
      for (size_t i = 1; i < tx.size(); i++) {
        if (regPtr + i - 1 < 16) regs[regPtr + i - 1] = tx[i];
      }
    }
    return 0;
  }
  uint8_t requestFrom(int a, int n) {
    rx.clear();
    rxPos = 0;
    if (a != rtc13::kAddr || !present) return 0;
    for (int i = 0; i < n; i++) {
      size_t idx = (size_t)regPtr + (size_t)i;
      rx.push_back(idx < 16 ? regs[idx] : 0);
    }
    return (uint8_t)n;
  }
  int read() { return rxPos < rx.size() ? rx[rxPos++] : -1; }
};
static TwoWireMock Wire;

// ── Serwer HTTP ─────────────────────────────────────────────────────────────
#define HTTP_GET  1
#define HTTP_POST 2

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
