// ═══════════════════════════════════════════════════════════════════════════
//  histlong.cpp — [4.5.0 HIST-LONG] patrz histlong.h
// ═══════════════════════════════════════════════════════════════════════════
#include "histlong.h"

#include <LittleFS.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include <esp_task_wdt.h>
static void feedWdt() { esp_task_wdt_reset(); }
#else
static void feedWdt() {}
#endif

namespace histlong {

namespace {

struct Acc {
  uint32_t bucket = 0;        // ts / BUCKET_SEC
  bool     has    = false;    // czy kubełek jest otwarty
  int      n      = 0;        // liczba próbek w kubełku
  double   sumTW = 0, sumT1 = 0, sumT2 = 0;  int nTW = 0, nT1 = 0, nT2 = 0;
  double   sumLR = 0, sumLW = 0;             int nLR = 0, nLW = 0;
  double   sumPwm[5] = {0, 0, 0, 0, 0};
  double   sumPow = 0;
  bool     lastAuto = false, lastMin = false, lastAdapt = false;
};

Acc  s_acc;
bool s_ready = false;

bool validTemp(float t) { return isfinite(t) && t > -100.0f && t < 150.0f; }
bool validLux(float l)  { return isfinite(l) && l >= 0.0f && l < 200000.0f; }

void addTo(double& sum, int& cnt, double v) { sum += v; cnt++; }

// Zapis wiersza kubełka. Pod kontrolą wołającego (single writer).
bool writeBucket(uint32_t bucket) {
  if (!s_ready) return false;

  char row[200];
  char tw[12], t1[12], t2[12], lr[12], lw[12], pw[12];
  auto fmt = [](char* out, size_t cap, int cnt, double sum, const char* f) {
    if (cnt > 0) snprintf(out, cap, f, sum / cnt); else out[0] = '\0';
  };
  fmt(tw, sizeof(tw), s_acc.nTW, s_acc.sumTW, "%.1f");
  fmt(t1, sizeof(t1), s_acc.nT1, s_acc.sumT1, "%.1f");
  fmt(t2, sizeof(t2), s_acc.nT2, s_acc.sumT2, "%.1f");
  fmt(lr, sizeof(lr), s_acc.nLR, s_acc.sumLR, "%.0f");
  fmt(lw, sizeof(lw), s_acc.nLW, s_acc.sumLW, "%.0f");
  fmt(pw, sizeof(pw), 1, s_acc.sumPow / (s_acc.n > 0 ? s_acc.n : 1), "%.1f");

  int pwmAvg[5];
  for (int i = 0; i < 5; i++) {
    pwmAvg[i] = (s_acc.n > 0) ? (int)(s_acc.sumPwm[i] / s_acc.n + 0.5) : 0;
  }

  snprintf(row, sizeof(row),
           "%lu,%s,%s,%s,%s,%s,%d,%d,%d,%d,%d,%s,%s,%s,%s,%d\n",
           (unsigned long)(bucket * BUCKET_SEC),
           tw, t1, t2, lr, lw,
           pwmAvg[0], pwmAvg[1], pwmAvg[2], pwmAvg[3], pwmAvg[4],
           pw,
           s_acc.lastAuto ? "AUTO" : "MAN",
           s_acc.lastMin ? "1" : "0",
           s_acc.lastAdapt ? "1" : "0",
           s_acc.n);

  // Rotacja PRZED zapisem, gdy bieżący plik już przekroczył limit.
  size_t curSize = 0;
  if (LittleFS.exists(FILE_CUR)) {
    File f = LittleFS.open(FILE_CUR, "r");
    if (f) { curSize = f.size(); f.close(); }
  }
  if (curSize > MAX_BYTES) {
    if (LittleFS.exists(FILE_OLD)) LittleFS.remove(FILE_OLD);
    LittleFS.rename(FILE_CUR, FILE_OLD);
    curSize = 0;
  }

  File f = LittleFS.open(FILE_CUR, "a");
  if (!f) return false;
  if (curSize == 0) {
    const size_t hl = strlen(HEADER);
    f.write(reinterpret_cast<const uint8_t*>(HEADER), hl);
    f.write(reinterpret_cast<const uint8_t*>("\n"), 1);
  }
  const size_t rl = strlen(row);
  f.write(reinterpret_cast<const uint8_t*>(row), rl);
  f.close();
  return true;
}

void resetAcc() { s_acc = Acc(); }

// ── Kursor strumieniowania (jeden naraz) ────────────────────────────────────
bool     s_streaming  = false;
int      s_stage      = 0;        // 0 = archiwum, 1 = bieżący, 2 = koniec
File     s_file;
bool     s_open       = false;
bool     s_wroteAny   = false;    // czy coś już poszło (archiwum niepuste)
bool     s_inHeader   = false;    // trwa pomijanie nagłówka w buforze
uint32_t s_lastMs     = 0;

void closeFile() {
  if (s_open) { s_file.close(); s_open = false; }
}

// Przechodzi do kolejnego istniejącego pliku; ustawia s_stage=2 po ostatnim.
void openNextStage() {
  closeFile();
  while (s_stage < 2) {
    const char* path = (s_stage == 0) ? FILE_OLD : FILE_CUR;
    s_stage++;
    if (!LittleFS.exists(path)) continue;
    File f = LittleFS.open(path, "r");
    if (!f) continue;
    s_file = f;
    s_open = true;
    // Nagłówek pomijamy w bieżącym pliku, jeśli archiwum już coś wysłało.
    s_inHeader = (s_stage == 2) && s_wroteAny;
    return;
  }
}

void endStream() {
  closeFile();
  s_streaming = false;
  s_stage = 2;
}

}  // namespace


void init(bool fsReady) {
  s_ready = fsReady;
  resetAcc();
}

bool addSample(uint32_t unixTs, const Sample& s) {
  if (!s_ready) return false;
  const uint32_t bucket = unixTs / BUCKET_SEC;
  bool ok = true;

  if (s_acc.has && bucket != s_acc.bucket) {
    if (s_acc.n > 0) ok = writeBucket(s_acc.bucket);
    resetAcc();
  }
  if (!s_acc.has) {
    s_acc.has = true;
    s_acc.bucket = bucket;
  }

  s_acc.n++;
  if (validTemp(s.tWater)) addTo(s_acc.sumTW, s_acc.nTW, s.tWater);
  if (validTemp(s.tPlate1)) addTo(s_acc.sumT1, s_acc.nT1, s.tPlate1);
  if (validTemp(s.tPlate2)) addTo(s_acc.sumT2, s_acc.nT2, s.tPlate2);
  if (validLux(s.luxRoom)) addTo(s_acc.sumLR, s_acc.nLR, s.luxRoom);
  if (validLux(s.luxWater)) addTo(s_acc.sumLW, s_acc.nLW, s.luxWater);
  for (int i = 0; i < 5; i++) {
    int p = s.pwm[i];
    if (p < 0) p = 0;
    if (p > 100) p = 100;
    s_acc.sumPwm[i] += p;
  }
  if (isfinite(s.powerW) && s.powerW >= 0.0f) s_acc.sumPow += s.powerW;
  s_acc.lastAuto = s.autoMode;
  s_acc.lastMin = s.minLux;
  s_acc.lastAdapt = s.adapt;
  return ok;
}

void clear() {
  if (LittleFS.exists(FILE_CUR)) LittleFS.remove(FILE_CUR);
  if (LittleFS.exists(FILE_OLD)) LittleFS.remove(FILE_OLD);
  resetAcc();
}

bool streamBegin(uint32_t nowMs) {
  if (!s_ready) return false;
  if (s_streaming) {
    // Poprzedni klient zniknął w połowie? Wygaś po timeoutcie.
    if ((uint32_t)(nowMs - s_lastMs) < STREAM_TIMEOUT_MS) return false;
    endStream();
  }
  s_streaming = true;
  s_stage = 0;
  s_wroteAny = false;
  s_lastMs = nowMs;
  openNextStage();
  return true;
}

size_t streamRead(uint8_t* buf, size_t maxLen, uint32_t nowMs) {
  if (!s_streaming || maxLen == 0) return 0;
  s_lastMs = nowMs;
  for (;;) {
    if (!s_open) { endStream(); return 0; }
    int r = (int)s_file.read(buf, maxLen);
    if (r <= 0) {
      // koniec tego pliku → następny (albo koniec strumienia)
      openNextStage();
      if (!s_open) { endStream(); return 0; }
      continue;
    }
    feedWdt();
    size_t n = (size_t)r;
    size_t off = 0;
    if (s_inHeader) {
      while (off < n && buf[off] != '\n') off++;
      if (off >= n) continue;          // cała porcja to nagłówek — czytaj dalej
      off++;                            // pomiń '\n'
      s_inHeader = false;
    }
    if (off < n) {
      size_t out = n - off;
      if (off > 0) memmove(buf, buf + off, out);
      s_wroteAny = true;
      return out;
    }
  }
}

void streamAbort() { endStream(); }

size_t streamTo(void* ctx, WriteFn write) {
  if (!streamBegin(0)) return 0;
  uint8_t buf[512];
  size_t total = 0;
  for (;;) {
    size_t n = streamRead(buf, sizeof(buf), 0);
    if (n == 0) break;
    size_t w = write(ctx, buf, n);
    if (w == 0) { streamAbort(); break; }
    total += w;
  }
  return total;
}

size_t totalBytes() {
  size_t t = 0;
  const char* paths[2] = {FILE_CUR, FILE_OLD};
  for (const char* p : paths) {
    if (!LittleFS.exists(p)) continue;
    File f = LittleFS.open(p, "r");
    if (f) { t += f.size(); f.close(); }
  }
  return t;
}

}  // namespace histlong
