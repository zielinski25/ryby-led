// Testy hostowe Spool V2 (Etap 3). Uruchomienie: patrz run_tests.sh
#include "telemetry_spool.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "LittleFS.h"

using namespace tspool;

static uint32_t g_millis = 100000;
uint32_t millis() { return g_millis; }
static uint32_t g_rnd = 0x12345678u;
uint32_t esp_random() { g_rnd = g_rnd * 1103515245u + 12345u; return g_rnd; }

static int g_fail = 0, g_pass = 0;
#define CHECK(cond, msg) do { if (cond) { g_pass++; } else { g_fail++; printf("  FAIL: %s  [%s:%d]\n", msg, __FILE__, __LINE__); } } while (0)

static void rmrf(const std::string& dir) {
  std::string cmd = "rm -rf '" + dir + "'";
  if (system(cmd.c_str()) != 0) { /* ignore */ }
}
// Jak świeży LittleFS: root istnieje zawsze, katalog /spool tworzy moduł.
static void resetFs() {
  rmrf(LittleFS.root());
  std::string cmd = "mkdir -p '" + LittleFS.root() + "'";
  if (system(cmd.c_str()) != 0) { /* ignore */ }
}

static Sample mkSample(uint32_t seq, uint64_t nonce) {
  Sample s{};
  s.sessionNonce = nonce;
  s.sampleSeq = seq;
  s.unixTs = 1700000000u + seq * 300u;
  s.uptimeS = seq * 300u;
  s.tWaterX10 = (int16_t)(200 + (int)(seq % 50));
  s.tPlate1X10 = (int16_t)(300 + (int)(seq % 7));
  s.tPlate2X10 = NA_X10;
  s.luxRoom = (uint16_t)(100 + seq);
  s.luxWater = NA_U16;
  for (int i = 0; i < 5; ++i) s.pwm[i] = (uint16_t)((seq + i) % 101);
  s.powerWX10 = (uint16_t)(50 + seq);
  s.wifiRssi = -60;
  s.stateBits = 0x05;
  return s;
}

// Wypełnia ring N próbkami seq=1..N.
static void fillRing(Ring& r, uint32_t n, uint64_t nonce) {
  for (uint32_t i = 1; i <= n; ++i) {
    bool ok = r.push(mkSample(i, nonce));
    if (!ok) { printf("  (fill push failed at %u)\n", i); }
  }
}

static bool flipByte(const std::string& host, long offset) {
  FILE* f = fopen(host.c_str(), "r+b");
  if (!f) return false;
  fseek(f, offset, SEEK_SET);
  int c = fgetc(f);
  if (c == EOF) { fclose(f); return false; }
  fseek(f, offset, SEEK_SET);
  fputc(c ^ 0x5A, f);
  fclose(f);
  return true;
}

static bool truncateBy(const std::string& host, long cut) {
  FILE* f = fopen(host.c_str(), "rb");
  if (!f) return false;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fclose(f);
  return truncate(host.c_str(), n - cut) == 0;
}

static std::string hostPath(const char* vpath) {
  return std::string(LittleFS.root()) + vpath;
}

// Zlicza kluczy rekordów w JSON batchu (wystąpienia `":{"`).
static int countMembers(const char* json) {
  int n = 0;
  for (const char* p = json; (p = strstr(p, "\":{")) != nullptr; p += 3) n++;
  return n;
}

// ───────────────────────────────────────────────────────────────────
static void test_crc_vector() {
  printf("[crc] wektor testowy\n");
  const char* s = "123456789";
  CHECK(crc32((const uint8_t*)s, 9) == 0xCBF43926UL, "CRC32('123456789') == 0xCBF43926");
}

static void test_ring() {
  printf("[ring] SPSC, pełny, wrap, commit\n");
  Ring r;
  CHECK(r.init(), "ring init");
  CHECK(r.ready(), "ring ready");
  fillRing(r, RING_CAPACITY, 1);
  CHECK(r.size() == RING_CAPACITY, "ring full size");
  CHECK(!r.push(mkSample(999, 1)), "push on full rejected");
  CHECK(r.commitPopN(100), "commit 100");
  CHECK(r.size() == RING_CAPACITY - 100, "size after commit");
  for (uint32_t i = 0; i < 100; ++i) CHECK(r.push(mkSample(1000 + i, 1)), "push after pop (wrap)");
  Sample s{};
  CHECK(r.peekAt(0, s) && s.sampleSeq == 101, "peek[0] after commit = seq 101");
  CHECK(r.peekAt(RING_CAPACITY - 1, s) && s.sampleSeq == 1099, "peek[last] = wrapped seq 1099");
  CHECK(!r.peekAt(RING_CAPACITY, s), "peek beyond size rejected");
  CHECK(!r.commitPopN(RING_CAPACITY + 1), "commit > capacity rejected");
  CHECK(r.bytes() == sizeof(Sample) * RING_CAPACITY, "ring bytes");
}

static void test_key_and_json() {
  printf("[json] klucz idempotentny + format RTDB\n");
  Sample s{};
  s.sessionNonce = 0x0123456789ABCDEFULL;
  s.sampleSeq = 42;
  s.unixTs = 1700000000u;
  s.uptimeS = 3600u;
  s.tWaterX10 = 215;
  s.tPlate1X10 = -5;
  s.tPlate2X10 = NA_X10;
  s.luxRoom = 120;
  s.luxWater = NA_U16;
  s.pwm[0] = 0; s.pwm[1] = 10; s.pwm[2] = 50; s.pwm[3] = 100; s.pwm[4] = 25;
  s.powerWX10 = 123;
  s.wifiRssi = -60;
  s.stateBits = 5;
  char buf[320];
  size_t n = sampleToJsonMember(s, buf, sizeof(buf));
  const char* expect =
      "\"0123456789ABCDEF_42\":{\"ts\":1700000000,\"up\":3600,\"tw\":21.5,\"t1\":-0.5,"
      "\"t2\":null,\"lr\":120,\"lw\":null,\"pwm\":[0,10,50,100,25],\"pw\":12.3,"
      "\"rssi\":-60,\"st\":5}";
  CHECK(n == strlen(expect), "json member length");
  CHECK(strcmp(buf, expect) == 0, "json member exact format");
  if (strcmp(buf, expect) != 0) printf("    got: %s\n", buf);

  // mały bufor → 0, nie obcięty JSON
  char tiny[40];
  CHECK(sampleToJsonMember(s, tiny, sizeof(tiny)) == 0, "too-small buffer returns 0");

  // klucz: ten sam (nonce, seq) → ten sam klucz; inny nonce → inny klucz
  char k1[32], k2[32], k3[32];
  sampleKey(s, k1, sizeof(k1));
  Sample s2 = s; s2.sessionNonce = 0x0123456789ABCDEEULL;
  sampleKey(s2, k2, sizeof(k2));
  Sample s3 = s; s3.sampleSeq = 43;
  sampleKey(s3, k3, sizeof(k3));
  CHECK(strcmp(k1, k2) != 0, "different session -> different key");
  CHECK(strcmp(k1, k3) != 0, "different seq -> different key");
  char k4[32];
  sampleKey(s, k4, sizeof(k4));
  CHECK(strcmp(k1, k4) == 0, "same sample -> same key (idempotent)");

  CHECK(makeSessionNonce() != 0, "session nonce nonzero");
}

static void test_write_and_scan() {
  printf("[page] zapis, commit, scan (czysta strona)\n");
  resetFs();
  Ring r; r.init();
  fillRing(r, 40, 0xAAULL);
  uint64_t first = 0, last = 0;
  char why[64] = {};
  CHECK(writePageFromRing(r, 40, 1, first, last, why, sizeof(why)), "writePageFromRing ok");
  CHECK(first == 1 && last == 40, "first/last seq returned");
  CHECK(!LittleFS.exists("/spool/p0000000001.part"), "no .part left after success");
  CHECK(LittleFS.exists("/spool/p0000000001.spl"), "final page exists");

  ScanResult sr{};
  CHECK(scanPage("/spool/p0000000001.spl", sr), "scanPage returns true");
  CHECK(sr.headerOk && sr.footerOk && sr.payloadOk, "header+footer+payload OK");
  CHECK(sr.badRecords == 0, "no bad records");
  CHECK(sr.recordCount == 40 && sr.firstSampleSeq == 1 && sr.lastSampleSeq == 40, "counts/seq in scan");
  CHECK(hostPath("/spool/p0000000001.spl").size() > 0, "path sane");

  // duplikat strony nie nadpisuje finalnej
  CHECK(!writePageFromRing(r, 40, 1, first, last, why, sizeof(why)), "same pageSeq rejected (final exists)");
  CHECK(strcmp(why, "final_exists") == 0, "reason final_exists");

  // count > size → odrzucone
  CHECK(!writePageFromRing(r, 41 + 300, 2, first, last, why, sizeof(why)), "count too big rejected");
  CHECK(!writePageFromRing(r, 0, 3, first, last, why, sizeof(why)), "count 0 rejected");
  // ring nietknięty przez zapis
  CHECK(r.size() == 40, "writePage does not pop ring (caller commits)");
}

static void test_corruption_and_negative() {
  printf("[neg] uszkodzony rekord CRC (test negatywny jak R250)\n");
  resetFs();
  Ring r; r.init();
  fillRing(r, 30, 0xBBULL);
  uint64_t f = 0, l = 0; char why[64];
  CHECK(writePageFromRing(r, 30, 7, f, l, why, sizeof(why)), "page 7 written");
  const std::string host = hostPath("/spool/p0000000007.spl");
  // bajt w rekordzie idx=7 (offset 64 + 7*52 + 5) — w obrębie próbki
  CHECK(flipByte(host, 64 + 7 * 52 + 5), "corrupt byte injected");

  const char* vpath = "/spool/p0000000007.spl";
  ScanResult sr{};
  CHECK(scanPage(vpath, sr), "scan still returns (header/footer intact)");
  CHECK(sr.headerOk && sr.footerOk, "footer still commits the page");
  CHECK(!sr.payloadOk, "payload CRC detects corruption");
  CHECK(sr.badRecords == 1, "exactly one bad record detected");

  Sample s{}; bool crcBad = false;
  CHECK(!readRecord(vpath, 7, s, crcBad) && crcBad, "readRecord(7) -> CRC bad");
  CHECK(readRecord(vpath, 8, s, crcBad) && s.sampleSeq == 9, "readRecord(8) intact");
  CHECK(!readRecord(vpath, 30, s, crcBad) && !crcBad, "readRecord beyond count -> false, not CRC");
}

static void test_truncated_page_quarantined() {
  printf("[neg] ucięta strona (brak footera) nie trafia na sieć\n");
  resetFs();
  Ring r; r.init();
  fillRing(r, 20, 0xCCULL);
  uint64_t f = 0, l = 0; char why[64];
  CHECK(writePageFromRing(r, 20, 1, f, l, why, sizeof(why)), "page 1 (will be truncated)");
  fillRing(r, 0, 0);
  CHECK(writePageFromRing(r, 20, 2, f, l, why, sizeof(why)), "page 2 (good) written");

  const std::string bad = hostPath("/spool/p0000000001.spl");
  CHECK(truncateBy(bad, 30), "truncate page 1 (simulated power cut at finalize)");
  ScanResult sr{};
  CHECK(scanPage("/spool/p0000000001.spl", sr) && sr.headerOk && !sr.footerOk, "truncated: header ok, footer NOT ok");

  ReplayCursor rc{};
  uint32_t quarantined = 0;
  CHECK(replayOpenOldest(rc, quarantined), "replayOpenOldest finds the good page");
  CHECK(quarantined == 1, "truncated page quarantined (removed)");
  CHECK(rc.pageSeq == 2 && rc.recordCount == 20, "opened page 2, 20 records");
  CHECK(!LittleFS.exists("/spool/p0000000001.spl"), "bad page deleted from FS");
}

static void test_replay_full_and_corrupt() {
  printf("[replay] batch FIFO, ACK, pominięcie uszkodzonych, kasowanie\n");
  resetFs();
  Ring r; r.init();
  fillRing(r, 30, 0xDDULL);
  uint64_t f = 0, l = 0; char why[64];
  CHECK(writePageFromRing(r, 30, 5, f, l, why, sizeof(why)), "page 5 written");
  const std::string host = hostPath("/spool/p0000000005.spl");
  CHECK(flipByte(host, 64 + 7 * 52 + 5), "record 7 corrupted");

  ReplayCursor rc{};
  uint32_t q = 0;
  CHECK(replayOpenOldest(rc, q) && rc.recordCount == 30 && q == 0, "open page 5");
  CHECK(rc.corruptSkipped == 0, "corruption counted only during batch build");
  static char json[JSON_BUFFER_BYTES];
  int sent = 0, batches = 0, guard = 0;
  uint32_t totalTaken = 0;
  bool sawNonEmptyJson = true;
  for (;;) {
    if (++guard > 100) { CHECK(false, "replay loop runaway"); break; }
    uint32_t taken = 0;
    int rcBuild = replayBuildBatch(rc, json, sizeof(json), taken);
    if (rcBuild == 0) break;                   // strona wyczerpana
    if (!(rcBuild == 1 || rcBuild == 2)) { printf("    code=%d cursor=%u count=%u path=%s\n", rcBuild, rc.cursor, rc.recordCount, rc.path); }
    CHECK(rcBuild == 1 || rcBuild == 2, "batch code 1 or 2");
    if (rcBuild == 1) {
      batches++;
      sent += countMembers(json);
      CHECK(json[0] == '{' && json[strlen(json) - 1] == '}', "batch JSON braces");
      size_t quotes = 0; for (const char* p = json; *p; ++p) if (*p == '"') quotes++;
      CHECK(quotes % 2 == 0, "batch JSON quotes balanced");
    } else {
      sawNonEmptyJson = sawNonEmptyJson && true;
    }
    totalTaken += taken;
    replayAck(rc, taken);
  }
  (void)sawNonEmptyJson;
  CHECK(totalTaken == 30, "all 30 positions covered (incl. skipped)");
  CHECK(rc.corruptSkipped == 1, "exactly one corrupt record skipped");
  CHECK(sent == 29, "29 valid records sent");
  CHECK(batches >= 3, "batched (>=3 requests for 30 records @12)");
  CHECK(rc.cursor == rc.recordCount, "cursor reached end");

  // po ACK ostatniej partii strona jest usuwana przez caller
  CHECK(removePageSeq(5), "caller removes exhausted page");
  CHECK(!LittleFS.exists("/spool/p0000000005.spl"), "page 5 gone");
  ReplayCursor rc2{};
  CHECK(!replayOpenOldest(rc2, q), "no pages left -> false");
}

static void test_fifo_and_maxseq() {
  printf("[fifo] najstarsza strona = najmniejszy pageSeq; ciągłość numeracji\n");
  resetFs();
  Ring r; r.init();
  uint64_t f = 0, l = 0; char why[64];
  fillRing(r, 5, 0xEEULL);
  CHECK(writePageFromRing(r, 5, 30, f, l, why, sizeof(why)), "p30");
  CHECK(writePageFromRing(r, 5, 12, f, l, why, sizeof(why)), "p12");
  CHECK(writePageFromRing(r, 5, 99, f, l, why, sizeof(why)), "p99");
  char path[64]; uint64_t seq = 0; uint32_t pages = 0; size_t bytes = 0;
  CHECK(findOldestPage(path, sizeof(path), seq, pages, bytes), "findOldestPage");
  CHECK(seq == 12, "oldest = 12 (not lexicographic)");
  CHECK(pages == 3, "3 pages counted");
  CHECK(maxPageSeqOnFs() == 99, "maxPageSeqOnFs = 99");
  uint32_t fsPages = 0; size_t fsBytes = 0;
  spoolFsStats(fsPages, fsBytes);
  CHECK(fsPages == 3 && fsBytes == bytes, "fs stats agree");
  uint64_t removed = 0;
  CHECK(removeOldestPage(removed) && removed == 12, "removeOldestPage removes 12");
  CHECK(!LittleFS.exists("/spool/p0000000012.spl"), "p12 gone");
}

static void test_partial_cleanup() {
  printf("[boot] *.part po przerwanym zapisie usuwane przy starcie\n");
  resetFs();
  Ring r; r.init();
  uint64_t f = 0, l = 0; char why[64];
  fillRing(r, 3, 0xFFULL);
  CHECK(writePageFromRing(r, 3, 1, f, l, why, sizeof(why)), "p1");
  // symulacja zaniku zasilania w trakcie zapisu: pozostaje .part
  FILE* fp = fopen(hostPath("/spool/p0000000002.part").c_str(), "wb");
  CHECK(fp != nullptr, "create fake .part");
  if (fp) { fwrite("garbage", 1, 7, fp); fclose(fp); }
  CHECK(discardPartialPages() == 1, "one .part discarded");
  CHECK(!LittleFS.exists("/spool/p0000000002.part"), ".part gone");
  CHECK(LittleFS.exists("/spool/p0000000001.spl"), "final page untouched");
  CHECK(maxPageSeqOnFs() == 1, "max seq ignores removed .part");
}

static void test_spill_policy() {
  printf("[spill] polityka: watermark / offline / cooldown / FS / cap\n");
  SpillInputs in{};
  in.nowMs = 1000000; in.lastSpillMs = 0; in.fsFreeOk = true; in.transportUp = true;
  uint32_t c = 0;
  in.ringSize = 0;  CHECK(!spillDue(in, c), "empty ring -> no spill");
  in.ringSize = 5;  CHECK(!spillDue(in, c), "5 online below watermark -> no spill");
  in.ringSize = 6;  CHECK(spillDue(in, c) && c == 6, "watermark reached -> spill 6");
  in.ringSize = 288; CHECK(spillDue(in, c) && c == PAGE_MAX_RECORDS, "cap to PAGE_MAX_RECORDS");
  in.fsFreeOk = false; CHECK(!spillDue(in, c), "no FS space -> no spill");
  in.fsFreeOk = true;
  in.lastSpillMs = in.nowMs - 1000; CHECK(!spillDue(in, c), "cooldown blocks");
  in.lastSpillMs = 0;
  in.ringSize = 1; in.transportUp = false; in.offlineSinceMs = in.nowMs - 30000;
  CHECK(!spillDue(in, c), "offline 30s < trigger -> no spill");
  in.offlineSinceMs = in.nowMs - SPILL_OFFLINE_MS;
  CHECK(spillDue(in, c) && c == 1, "offline >= trigger -> spill 1 record");
  in.ringSize = 0; CHECK(!spillDue(in, c), "offline but empty ring -> no spill");
}

static void test_non_ascii_free_paths() {
  printf("[api] ścieżki i nazwy\n");
  char f[48], p[48];
  CHECK(pagePaths(123, f, sizeof(f), p, sizeof(p)), "pagePaths ok");
  CHECK(strcmp(f, "/spool/p0000000123.spl") == 0, "final path format");
  CHECK(strcmp(p, "/spool/p0000000123.part") == 0, "part path format");
  CHECK(!pagePaths(1, f, 10, p, sizeof(p)), "pagePaths rejects small buffer");
}

int main() {
  // Czysty katalog FS dla każdego uruchomienia
  resetFs();

  test_crc_vector();
  test_ring();
  test_key_and_json();
  test_write_and_scan();
  test_corruption_and_negative();
  test_truncated_page_quarantined();
  test_replay_full_and_corrupt();
  test_fifo_and_maxseq();
  test_partial_cleanup();
  test_spill_policy();
  test_non_ascii_free_paths();

  printf("\nWYNIK: %d PASS, %d FAIL\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
