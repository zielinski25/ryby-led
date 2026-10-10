// ═══════════════════════════════════════════════════════════════════
//  telemetry_spool.cpp — implementacja Spool V2 dla Ryb (Etap 3, v4.2.0)
//  Patrz telemetry_spool.h: zasady formatu, polityki i współbieżności.
// ═══════════════════════════════════════════════════════════════════
#include "telemetry_spool.h"

#include <Arduino.h>      // millis(), ps_malloc()
#include <LittleFS.h>
#include <esp_system.h>   // esp_random()
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace tspool {

// ── helpers ─────────────────────────────────────────────────────────
static uint32_t crcUpdate(uint32_t state, const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    state ^= data[i];
    for (uint8_t b = 0; b < 8U; ++b) {
      state = (state >> 1) ^ ((state & 1U) ? 0xEDB88320UL : 0U);
    }
  }
  return state;
}

uint32_t crc32(const uint8_t* data, size_t len) {
  return ~crcUpdate(0xFFFFFFFFUL, data, len);
}

static uint32_t headerCrc(const PageHeader& h) {
  PageHeader tmp = h;
  tmp.headerCrc32 = 0U;
  return crc32(reinterpret_cast<const uint8_t*>(&tmp), sizeof(tmp));
}

static uint32_t footerCrc(const PageFooter& f) {
  PageFooter tmp = f;
  tmp.footerCrc32 = 0U;
  return crc32(reinterpret_cast<const uint8_t*>(&tmp), sizeof(tmp));
}

// Bezpieczny zapis do bufora z obcinaniem (bez przepełnienia).
struct Out {
  char*  p;
  size_t left;
  bool   ok;
};

static void app(Out& o, const char* fmt, ...) {
  if (!o.ok) return;
  if (o.left == 0U) { o.ok = false; return; }
  va_list ap;
  va_start(ap, fmt);
  const int n = vsnprintf(o.p, o.left, fmt, ap);
  va_end(ap);
  if (n < 0 || (size_t)n >= o.left) { o.ok = false; o.left = 0U; return; }
  o.p += n;
  o.left -= (size_t)n;
}

// "tag":<wartość X10> albo null; zawsze z przecinkiem na końcu.
static void appX10(Out& o, const char* tag, int16_t v) {
  if (v == NA_X10) { app(o, "\"%s\":null,", tag); return; }
  const int32_t a = (v < 0) ? -(int32_t)v : (int32_t)v;
  app(o, "\"%s\":%s%ld.%ld,", tag, (v < 0) ? "-" : "", (long)(a / 10), (long)(a % 10));
}

static void appLux(Out& o, const char* tag, uint16_t v) {
  if (v == NA_U16) { app(o, "\"%s\":null,", tag); return; }
  app(o, "\"%s\":%u,", tag, (unsigned)v);
}

static size_t outLen(const Out& o, size_t cap) { return o.ok ? (cap - o.left) : 0U; }

static bool pageNameSeq(const char* name, uint64_t& seqOut, bool& isPart) {
  // Nazwa: p<10 cyfr>.spl | p<10 cyfr>.part  (bez katalogu)
  const char* base = strrchr(name, '/');
  base = base ? base + 1 : name;
  if (base[0] != 'p' || strlen(base) < 1U + 10U + 4U) return false;
  const char* ext = base + 11;               // p + 10 cyfr = 11 znaków
  const bool spl = (strcmp(ext, ".spl") == 0);
  const bool part = (strcmp(ext, ".part") == 0);
  if (!spl && !part) return false;
  uint64_t v = 0;
  for (int i = 1; i <= 10; ++i) {
    if (base[i] < '0' || base[i] > '9') return false;
    v = v * 10U + (uint64_t)(base[i] - '0');
  }
  seqOut = v;
  isPart = part;
  return true;
}

static bool readExact(File& f, uint8_t* buf, size_t n) {
  return f.read(buf, n) == n;
}

// ── Ring ────────────────────────────────────────────────────────────
Ring::~Ring() {
  // ps_malloc → zwalniane przez free() (IDF: heap_caps_free dla dowolnej puli).
  free(m_buffer);
  m_buffer = nullptr;
}

bool Ring::init() {
  if (m_buffer) return true;
  m_buffer = static_cast<Sample*>(ps_malloc(bytes()));
  if (!m_buffer) return false;
  __atomic_store_n(&m_head, 0U, __ATOMIC_RELAXED);
  __atomic_store_n(&m_tail, 0U, __ATOMIC_RELAXED);
  return true;
}

bool Ring::ready() const { return m_buffer != nullptr; }

bool Ring::push(const Sample& s) {
  if (!m_buffer) return false;
  const uint32_t head = __atomic_load_n(&m_head, __ATOMIC_RELAXED);
  const uint32_t tail = __atomic_load_n(&m_tail, __ATOMIC_ACQUIRE);
  if ((uint32_t)(head - tail) >= RING_CAPACITY) return false;
  m_buffer[head % RING_CAPACITY] = s;
  __atomic_store_n(&m_head, head + 1U, __ATOMIC_RELEASE);
  return true;
}

bool Ring::peekAt(uint32_t offset, Sample& out) const {
  if (!m_buffer || offset >= RING_CAPACITY) return false;
  const uint32_t tail = __atomic_load_n(&m_tail, __ATOMIC_RELAXED);
  const uint32_t head = __atomic_load_n(&m_head, __ATOMIC_ACQUIRE);
  if (offset >= (uint32_t)(head - tail)) return false;
  out = m_buffer[(tail + offset) % RING_CAPACITY];
  return true;
}

bool Ring::commitPopN(uint32_t count) {
  if (!m_buffer || count == 0U || count > RING_CAPACITY) return false;
  const uint32_t tail = __atomic_load_n(&m_tail, __ATOMIC_RELAXED);
  const uint32_t head = __atomic_load_n(&m_head, __ATOMIC_ACQUIRE);
  if ((uint32_t)(head - tail) < count) return false;
  __atomic_store_n(&m_tail, tail + count, __ATOMIC_RELEASE);
  return true;
}

uint32_t Ring::size() const {
  const uint32_t head = __atomic_load_n(&m_head, __ATOMIC_ACQUIRE);
  const uint32_t tail = __atomic_load_n(&m_tail, __ATOMIC_ACQUIRE);
  const uint32_t used = head - tail;
  return used <= RING_CAPACITY ? used : RING_CAPACITY;
}

void Ring::reset() {
  __atomic_store_n(&m_head, 0U, __ATOMIC_RELAXED);
  __atomic_store_n(&m_tail, 0U, __ATOMIC_RELAXED);
}

// ── Sesja i klucze ──────────────────────────────────────────────────
uint64_t makeSessionNonce() {
  const uint64_t hi = (uint64_t)esp_random();
  const uint64_t lo = (uint64_t)esp_random();
  uint64_t v = (hi << 32) | lo;
  if (v == 0U) v = 1U;
  return v;
}

size_t sampleKey(const Sample& s, char* out, size_t cap) {
  if (!out || cap < 28U) return 0U;
  const int n = snprintf(out, cap, "%08lX%08lX_%lu",
                         (unsigned long)((s.sessionNonce >> 32) & 0xFFFFFFFFUL),
                         (unsigned long)(s.sessionNonce & 0xFFFFFFFFUL),
                         (unsigned long)s.sampleSeq);
  if (n <= 0 || (size_t)n >= cap) return 0U;
  return (size_t)n;
}

size_t sampleToJsonMember(const Sample& s, char* out, size_t cap) {
  char key[32];
  if (sampleKey(s, key, sizeof(key)) == 0U) return 0U;
  Out o{out, cap, true};
  app(o, "\"%s\":{", key);
  app(o, "\"ts\":%lu,\"up\":%lu,", (unsigned long)s.unixTs, (unsigned long)s.uptimeS);
  appX10(o, "tw", s.tWaterX10);
  appX10(o, "t1", s.tPlate1X10);
  appX10(o, "t2", s.tPlate2X10);
  appLux(o, "lr", s.luxRoom);
  appLux(o, "lw", s.luxWater);
  app(o, "\"pwm\":[%u,%u,%u,%u,%u],", (unsigned)s.pwm[0], (unsigned)s.pwm[1],
      (unsigned)s.pwm[2], (unsigned)s.pwm[3], (unsigned)s.pwm[4]);
  app(o, "\"pw\":%u.%u,", (unsigned)(s.powerWX10 / 10U), (unsigned)(s.powerWX10 % 10U));
  app(o, "\"rssi\":%d,\"st\":%u}", (int)s.wifiRssi, (unsigned)s.stateBits);
  return outLen(o, cap);
}

// ── Ścieżki i katalog ───────────────────────────────────────────────
bool pagePaths(uint64_t pageSeq, char* finalOut, size_t finalCap,
               char* partOut, size_t partCap) {
  if (!finalOut || !partOut) return false;
  const int a = snprintf(finalOut, finalCap, "%s/p%010llu.spl", SPOOL_DIR,
                         (unsigned long long)pageSeq);
  const int b = snprintf(partOut, partCap, "%s/p%010llu.part", SPOOL_DIR,
                         (unsigned long long)pageSeq);
  return a > 0 && b > 0 && (size_t)a < finalCap && (size_t)b < partCap;
}

static bool ensureDir() {
  if (LittleFS.exists(SPOOL_DIR)) return true;
  return LittleFS.mkdir(SPOOL_DIR);
}

// ── Skan strony (pełna weryfikacja) ─────────────────────────────────
bool scanPage(const char* path, ScanResult& out) {
  out = ScanResult{};
  File f = LittleFS.open(path, "r");
  if (!f) return false;
  const size_t size = f.size();
  if (size < PAGE_META_BYTES) { f.close(); return false; }

  PageHeader h{};
  if (!readExact(f, reinterpret_cast<uint8_t*>(&h), sizeof(h))) { f.close(); return false; }
  out.headerOk = (h.magic == PAGE_MAGIC && h.formatVersion == PAGE_FORMAT_VERSION &&
                  h.headerBytes == HEADER_BYTES && h.recordBytes == RECORD_BYTES &&
                  h.headerCrc32 == headerCrc(h));
  out.pageSeq = h.pageSeq;
  out.firstSampleSeq = h.firstSampleSeq;
  if (!out.headerOk) { f.close(); return false; }

  PageFooter ft{};
  if (!f.seek(size - FOOTER_BYTES) || !readExact(f, reinterpret_cast<uint8_t*>(&ft), sizeof(ft))) {
    f.close();
    return true;   // nagłówek OK, brak footera = strona otwarta
  }
  const bool ftOk = (ft.magic == PAGE_MAGIC && ft.formatVersion == PAGE_FORMAT_VERSION &&
                     ft.footerBytes == FOOTER_BYTES && ft.commitMarker == PAGE_COMMIT_MARKER &&
                     ft.footerCrc32 == footerCrc(ft) && ft.pageSeq == h.pageSeq);
  const size_t payloadBytes = size - PAGE_META_BYTES;
  const bool geometryOk = (payloadBytes % RECORD_BYTES) == 0U;
  const uint32_t nRecs = geometryOk ? (uint32_t)(payloadBytes / RECORD_BYTES) : 0U;
  if (!ftOk || !geometryOk || nRecs != ft.recordCount) {
    f.close();
    return true;   // footer niewiarygodny → strona nie jest zatwierdzona
  }
  out.footerOk = true;
  out.recordCount = ft.recordCount;
  out.lastSampleSeq = ft.lastSampleSeq;

  if (!f.seek(HEADER_BYTES)) { f.close(); return true; }
  uint32_t state = 0xFFFFFFFFUL;
  uint8_t rec[RECORD_BYTES];
  for (uint32_t i = 0; i < nRecs; ++i) {
    if (!readExact(f, rec, RECORD_BYTES)) { out.footerOk = false; break; }
    state = crcUpdate(state, rec, RECORD_BYTES);
    uint32_t stored = 0;
    memcpy(&stored, rec + SAMPLE_BYTES, sizeof(stored));
    if (stored != crc32(rec, SAMPLE_BYTES)) {
      out.badRecords++;
    }
  }
  f.close();
  out.payloadOk = out.footerOk && (~state == ft.payloadCrc32);
  return true;
}

// ── Zapis strony z ringu ────────────────────────────────────────────
bool writePageFromRing(const Ring& ring, uint32_t count, uint64_t pageSeq,
                       uint64_t& firstSeqOut, uint64_t& lastSeqOut,
                       char* whyOut, size_t whyCap) {
  auto why = [&](const char* w) {
    if (whyOut && whyCap) { strncpy(whyOut, w, whyCap - 1U); whyOut[whyCap - 1U] = '\0'; }
    return false;
  };
  if (count == 0U || count > PAGE_MAX_RECORDS || count > ring.size()) return why("count");
  if (!ensureDir()) return why("mkdir");

  char finalPath[48], partPath[48];
  if (!pagePaths(pageSeq, finalPath, sizeof(finalPath), partPath, sizeof(partPath))) return why("path");
  if (LittleFS.exists(partPath)) LittleFS.remove(partPath);
  if (LittleFS.exists(finalPath)) return why("final_exists");

  Sample first{}, last{};
  if (!ring.peekAt(0U, first)) return why("peek_first");

  File f = LittleFS.open(partPath, "w");
  if (!f) return why("open_part");

  auto abortWrite = [&](const char* w) {
    f.close();
    LittleFS.remove(partPath);
    return why(w);
  };

  PageHeader h{};
  h.magic = PAGE_MAGIC;
  h.formatVersion = PAGE_FORMAT_VERSION;
  h.headerBytes = (uint16_t)HEADER_BYTES;
  h.pageSeq = pageSeq;
  h.firstSampleSeq = first.sampleSeq;
  h.recordBytes = (uint16_t)RECORD_BYTES;
  h.capacity = (uint16_t)PAGE_MAX_RECORDS;
  h.firstUnixTs = first.unixTs;
  h.firstMonoMs = (uint32_t)first.uptimeS * 1000U;
  h.headerCrc32 = headerCrc(h);
  if (f.write(reinterpret_cast<const uint8_t*>(&h), sizeof(h)) != sizeof(h)) return abortWrite("write_header");

  uint32_t state = 0xFFFFFFFFUL;
  uint8_t rec[RECORD_BYTES];
  for (uint32_t i = 0; i < count; ++i) {
    Sample s{};
    if (!ring.peekAt(i, s)) return abortWrite("peek_rec");
    memcpy(rec, &s, SAMPLE_BYTES);
    const uint32_t rcrc = crc32(rec, SAMPLE_BYTES);
    memcpy(rec + SAMPLE_BYTES, &rcrc, sizeof(rcrc));
    if (f.write(rec, RECORD_BYTES) != RECORD_BYTES) return abortWrite("write_rec");
    state = crcUpdate(state, rec, RECORD_BYTES);
    last = s;
  }

  PageFooter ft{};
  ft.magic = PAGE_MAGIC;
  ft.formatVersion = PAGE_FORMAT_VERSION;
  ft.footerBytes = (uint16_t)FOOTER_BYTES;
  ft.pageSeq = pageSeq;
  ft.firstSampleSeq = first.sampleSeq;
  ft.lastSampleSeq = last.sampleSeq;
  ft.recordCount = count;
  ft.payloadCrc32 = ~state;
  ft.commitMarker = PAGE_COMMIT_MARKER;
  ft.footerCrc32 = footerCrc(ft);
  if (f.write(reinterpret_cast<const uint8_t*>(&ft), sizeof(ft)) != sizeof(ft)) return abortWrite("write_footer");
  f.flush();
  f.close();

  // Weryfikacja PRZED rename: dopiero zweryfikowana strona staje się finalna.
  ScanResult sr{};
  if (!scanPage(partPath, sr) || !sr.footerOk || !sr.payloadOk || sr.badRecords != 0U ||
      sr.recordCount != count) {
    LittleFS.remove(partPath);
    return why("verify_part");
  }
  if (!LittleFS.rename(partPath, finalPath)) {
    LittleFS.remove(partPath);
    return why("rename");
  }
  firstSeqOut = first.sampleSeq;
  lastSeqOut = last.sampleSeq;
  return true;
}

// ── Odczyt rekordu ──────────────────────────────────────────────────
bool readRecord(const char* path, uint32_t idx, Sample& out, bool& crcBad) {
  crcBad = false;
  File f = LittleFS.open(path, "r");
  if (!f) return false;
  const size_t size = f.size();
  if (size < PAGE_META_BYTES) { f.close(); return false; }
  const uint32_t nRecs = (uint32_t)((size - PAGE_META_BYTES) / RECORD_BYTES);
  if (idx >= nRecs) { f.close(); return false; }
  const size_t off = HEADER_BYTES + (size_t)idx * RECORD_BYTES;
  uint8_t rec[RECORD_BYTES];
  const bool ok = f.seek(off) && readExact(f, rec, RECORD_BYTES);
  f.close();
  if (!ok) return false;
  uint32_t stored = 0;
  memcpy(&stored, rec + SAMPLE_BYTES, sizeof(stored));
  if (stored != crc32(rec, SAMPLE_BYTES)) { crcBad = true; return false; }
  memcpy(&out, rec, SAMPLE_BYTES);
  return true;
}

// ── Katalog stron ───────────────────────────────────────────────────
bool findOldestPage(char* pathOut, size_t pathCap, uint64_t& pageSeqOut,
                    uint32_t& pagesOut, size_t& bytesOut) {
  pagesOut = 0U;
  bytesOut = 0U;
  bool found = false;
  uint64_t best = 0U;
  char bestName[48] = {};
  File dir = LittleFS.open(SPOOL_DIR, "r");
  if (!dir || !dir.isDirectory()) return false;
  File e = dir.openNextFile();
  while (e) {
    uint64_t seq = 0;
    bool isPart = false;
    if (!e.isDirectory() && pageNameSeq(e.name(), seq, isPart) && !isPart) {
      pagesOut++;
      bytesOut += e.size();
      if (!found || seq < best) {
        best = seq;
        found = true;
        snprintf(bestName, sizeof(bestName), "%s/p%010llu.spl", SPOOL_DIR, (unsigned long long)seq);
      }
    }
    e.close();
    e = dir.openNextFile();
  }
  dir.close();
  if (!found || !pathOut || pathCap <= strlen(bestName)) return false;
  strncpy(pathOut, bestName, pathCap - 1U);
  pathOut[pathCap - 1U] = '\0';
  pageSeqOut = best;
  return true;
}

uint64_t maxPageSeqOnFs() {
  uint64_t best = 0U;
  File dir = LittleFS.open(SPOOL_DIR, "r");
  if (!dir || !dir.isDirectory()) return 0U;
  File e = dir.openNextFile();
  while (e) {
    uint64_t seq = 0;
    bool isPart = false;
    if (!e.isDirectory() && pageNameSeq(e.name(), seq, isPart) && seq > best) best = seq;
    e.close();
    e = dir.openNextFile();
  }
  dir.close();
  return best;
}

uint32_t discardPartialPages() {
  uint32_t removed = 0U;
  // Usuwamy jeden plik naraz i przeskanowujemy katalog od nowa — LittleFS
  // nie gwarantuje bezpiecznego kasowania w trakcie iteracji.
  for (uint32_t guard = 0U; guard < 64U; ++guard) {
    char victim[48] = {};
    File dir = LittleFS.open(SPOOL_DIR, "r");
    if (!dir || !dir.isDirectory()) return removed;
    File e = dir.openNextFile();
    while (e) {
      uint64_t seq = 0;
      bool isPart = false;
      if (!e.isDirectory() && pageNameSeq(e.name(), seq, isPart) && isPart) {
        snprintf(victim, sizeof(victim), "%s/p%010llu.part", SPOOL_DIR, (unsigned long long)seq);
      }
      e.close();
      if (victim[0]) break;
      e = dir.openNextFile();
    }
    dir.close();
    if (!victim[0]) break;
    if (LittleFS.remove(victim)) removed++;
    else break;
  }
  return removed;
}

bool removePageSeq(uint64_t pageSeq) {
  char finalPath[48], partPath[48];
  if (!pagePaths(pageSeq, finalPath, sizeof(finalPath), partPath, sizeof(partPath))) return false;
  bool ok = true;
  if (LittleFS.exists(finalPath)) ok = LittleFS.remove(finalPath);
  return ok;
}

bool removeOldestPage(uint64_t& removedSeq) {
  char path[48];
  uint64_t seq = 0;
  uint32_t pages = 0;
  size_t bytes = 0;
  if (!findOldestPage(path, sizeof(path), seq, pages, bytes)) return false;
  if (!removePageSeq(seq)) return false;
  removedSeq = seq;
  return true;
}

void spoolFsStats(uint32_t& pages, size_t& bytes) {
  pages = 0U;
  bytes = 0U;
  File dir = LittleFS.open(SPOOL_DIR, "r");
  if (!dir || !dir.isDirectory()) return;
  File e = dir.openNextFile();
  while (e) {
    uint64_t seq = 0;
    bool isPart = false;
    if (!e.isDirectory() && pageNameSeq(e.name(), seq, isPart) && !isPart) {
      pages++;
      bytes += e.size();
    }
    e.close();
    e = dir.openNextFile();
  }
  dir.close();
}

// ── Polityka spill ──────────────────────────────────────────────────
bool spillDue(const SpillInputs& in, uint32_t& countOut) {
  countOut = 0U;
  if (in.ringSize == 0U || !in.fsFreeOk) return false;
  if ((uint32_t)(in.nowMs - in.lastSpillMs) < SPILL_COOLDOWN_MS) return false;

  const bool offlineDue = (!in.transportUp) && in.offlineSinceMs != 0U &&
                          (uint32_t)(in.nowMs - in.offlineSinceMs) >= SPILL_OFFLINE_MS;
  const bool backlogDue = in.ringSize >= SPILL_RING_WATERMARK;
  if (!offlineDue && !backlogDue) return false;

  countOut = (in.ringSize < PAGE_MAX_RECORDS) ? in.ringSize : PAGE_MAX_RECORDS;
  return countOut > 0U;
}

// ── Replay ──────────────────────────────────────────────────────────
bool replayOpenOldest(ReplayCursor& rc, uint32_t& badPagesRemoved) {
  badPagesRemoved = 0U;
  memset(&rc, 0, sizeof(rc));
  for (uint32_t guard = 0U; guard < 32U; ++guard) {
    uint64_t seq = 0;
    uint32_t pages = 0;
    size_t bytes = 0;
    char path[48] = {};
    if (!findOldestPage(path, sizeof(path), seq, pages, bytes)) return false;
    ScanResult sr{};
    if (scanPage(path, sr) && sr.headerOk && sr.footerOk && sr.pageSeq == seq) {
      strncpy(rc.path, path, sizeof(rc.path) - 1U);
      rc.pageSeq = seq;
      rc.recordCount = sr.recordCount;
      rc.cursor = 0U;
      rc.active = true;
      return true;
    }
    // Strona uszkodzona już na poziomie nagłówka/footera — kwarantanna = kasowanie.
    LittleFS.remove(path);
    badPagesRemoved++;
  }
  return false;
}

int replayBuildBatch(ReplayCursor& rc, char* json, size_t jsonCap, uint32_t& taken) {
  taken = 0U;
  if (!rc.active) return -1;
  if (rc.cursor >= rc.recordCount) return 0;
  if (!json || jsonCap < 4U) return -1;

  const uint32_t n = (REPLAY_BATCH_MAX < rc.recordCount - rc.cursor)
                         ? REPLAY_BATCH_MAX
                         : (rc.recordCount - rc.cursor);
  Out o{json, jsonCap, true};
  app(o, "{");
  bool first = true;
  uint32_t appended = 0U;
  for (uint32_t i = 0U; i < n; ++i) {
    Sample s{};
    bool crcBad = false;
    if (!readRecord(rc.path, rc.cursor + i, s, crcBad)) {
      if (crcBad) { rc.corruptSkipped++; continue; }   // uszkodzony rekord: pomiń
      return -1;                                        // błąd I/O: nie przesuwaj kursora
    }
    // Rozdzielacz przed każdym członkiem poza pierwszym.
    char member[256];
    const size_t mlen = sampleToJsonMember(s, member, sizeof(member));
    if (mlen == 0U) return -1;
    if (!first) app(o, ",");
    if (!o.ok || o.left < mlen + 2U) return -1;   // za mały bufor — nie wysyłaj połowy
    memcpy(o.p, member, mlen);
    o.p += mlen;
    o.left -= mlen;
    first = false;
    appended++;
  }
  app(o, "}");
  if (!o.ok) return -1;
  taken = n;
  if (appended == 0U) return 2;
  *o.p = '\0';
  return 1;
}

void replayAck(ReplayCursor& rc, uint32_t taken) {
  uint32_t next = rc.cursor + taken;
  if (next > rc.recordCount) next = rc.recordCount;
  rc.cursor = next;
}

}  // namespace tspool
