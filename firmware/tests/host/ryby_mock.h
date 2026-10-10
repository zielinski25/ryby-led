// Atrapy globali/API Ryby + FirebaseClient (sygnatury zweryfikowane w źródłach v2.2.13),
// wyłącznie do kompilacji bloku [4.2.0 SPOOL] w izolacji.
#pragma once
#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include "Arduino.h"
#include "esp_system.h"
#include "LittleFS.h"

#define NET_SUCCESS(x) do { (void)(x); } while (0)
#define NET_FAIL(x) do { (void)(x); } while (0)

struct String {
  String() {}
  String(const char*) {}
  const char* c_str() const { return ""; }
};

// --- FirebaseClient (zgodne z v2.2.13) ---
struct object_t {
  String buf;
  explicit object_t(const String& o) : buf(o) {}
};
struct AsyncResult {
  bool isError() const { return false; }
  bool isResult() const { return true; }
  bool isEvent() const { return false; }
  bool isDebug() const { return false; }
  bool available() const { return true; }
};
struct AsyncClientStub {};
struct RealtimeDatabaseStub {
  template <typename T>
  void update(AsyncClientStub&, const String&, const T&, AsyncResult&) {}
};
struct AppStub { bool ready() const { return true; } };

// --- Ryby globals (typy zgodne z Ryby_LED_fi_S3.cpp) ---
enum class FbAsyncOp : uint8_t { NONE, GET_COMMANDS, DELETE_COMMAND, GET_CONFIG, SET_STATUS, SET_TELEMETRY };
static FbAsyncOp g_fbAsyncOp = FbAsyncOp::NONE;
static uint32_t g_fbAsyncOpStartedMs = 0;
static uint32_t g_fbAsyncOpTimeoutMs = 0;
static bool g_fbAsyncResultHandled = false;
static uint32_t g_fbAsyncCompletedCount = 0;
static constexpr uint32_t FB_ASYNC_SEND_TIMEOUT_MS = 12000UL;
static volatile bool fbQueueInFlight = false;
static bool littlefsReady = true;
static bool fbAppReady = true;
static AppStub fbApp;
static AsyncClientStub fbAsyncClient;
static RealtimeDatabaseStub fbDatabase;
static uint32_t fbClientLastUse = 0;
static uint32_t _fbFailStreak = 0;
static float tempPlate1 = 25.0f, tempPlate2 = 25.0f, tempWater = 20.0f;
static volatile float luxPokojowy = 0, luxNadWoda = 0;
static bool tryb = false, minLuxModeActive = false, regulacjaAdaptacyjnaWlaczona = false;
static float totalPowerW = 0;
static uint32_t peakPowerWToday = 0;

#define WL_CONNECTED 3
struct WiFiStub {
  int status() const { return WL_CONNECTED; }
  int RSSI() const { return -55; }
};
static WiFiStub WiFi;
struct EspStub { uint32_t getFreePsram() const { return 0; } };
static EspStub ESP;
static inline void esp_task_wdt_reset() {}

static void fbAsyncFail(const char*) {}
static void fbAsyncResetOperation() { g_fbAsyncOp = FbAsyncOp::NONE; }
static bool fbAsyncEnsureReady() { return true; }

// Log: podpisy jak w Ryby (overloady dla const char* i formatowania)
static void logPrintln(const char*) {}
static void logPrintf(const char*, ...) {}
static void logPrintfNoFile(const char*, ...) {}

template <typename T> static T constrain(T x, T a, T b) { return x < a ? a : (x > b ? b : x); }
