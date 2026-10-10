// ═══════════════════════════════════════════════════════════════════════════
//  test_rtc.cpp — [4.7.0 ASTRO] sterownik RTC DS1307/DS3231 (Etap 6 pkt 2).
//  Kompiluje PRAWDZIWE rtc_ds13xx.cpp. Fałszywa magistrala z pamięcią rejestrów
//  (0x68), wstrzykiwane błędy ACK. Wartości oczekiwane z niezależnego obliczenia
//  (Python datetime), nie z kodu. ASan + UBSan.
// ═══════════════════════════════════════════════════════════════════════════
#include "rtc_ds13xx.h"

#include <cstdio>
#include <cstring>
#include <cstdint>

static int g_pass = 0, g_fail = 0;
#define CHECK(cond, name)                                                       \
  do {                                                                          \
    if (cond) { g_pass++; }                                                     \
    else { g_fail++; std::printf("FAIL: %s (linia %d)\n", name, __LINE__); }    \
  } while (0)

// Fałszywe układy: rejestry 0x00..0x0F, flaga obecności, liczniki zapisów.
struct FakeChip {
  uint8_t reg[16];
  bool present = true;     // ACK na adres 0x68
  bool nackWrite = false;  // błąd zapisu
  int writes = 0;
  int lastWriteLen = 0;
  uint8_t lastWrite[16];
  FakeChip() { std::memset(reg, 0, sizeof(reg)); }
};

static bool fakeWrite(void* ctx, uint8_t addr, const uint8_t* buf, size_t n) {
  FakeChip* c = (FakeChip*)ctx;
  if (addr != rtc13::kAddr || !c->present || c->nackWrite) return false;
  if (n == 0 || n > 16) return false;
  uint8_t reg = buf[0];
  for (size_t i = 1; i < n; i++) {
    if (reg + i - 1 < 16) c->reg[reg + i - 1] = buf[i];
  }
  c->writes++;
  c->lastWriteLen = (int)n;
  std::memcpy(c->lastWrite, buf, n);
  return true;
}

static bool fakeReadReg(void* ctx, uint8_t addr, uint8_t reg, uint8_t* buf, size_t n) {
  FakeChip* c = (FakeChip*)ctx;
  if (addr != rtc13::kAddr || !c->present) return false;
  if (reg + n > 16) return false;
  std::memcpy(buf, c->reg + reg, n);
  return true;
}

static rtc13::Bus busFor(FakeChip& c) {
  rtc13::Bus b;
  b.ctx = &c;
  b.write = fakeWrite;
  b.readReg = fakeReadReg;
  return b;
}

// Ustawia rejestry czasu w BCD (jak robi DS1307/DS3231 po wyprodukowaniu).
static void setRegs(FakeChip& c, uint8_t sec, uint8_t min, uint8_t hour, uint8_t dow,
                    uint8_t date, uint8_t mon, uint8_t yr) {
  c.reg[0] = sec; c.reg[1] = min; c.reg[2] = hour; c.reg[3] = dow;
  c.reg[4] = date; c.reg[5] = mon; c.reg[6] = yr;
}

static void testBcd() {
  CHECK(rtc13::toBcd(0) == 0x00, "BCD 0");
  CHECK(rtc13::toBcd(9) == 0x09, "BCD 9");
  CHECK(rtc13::toBcd(10) == 0x10, "BCD 10");
  CHECK(rtc13::toBcd(59) == 0x59, "BCD 59");
  CHECK(rtc13::fromBcd(0x59) == 59, "fromBcd 59");
  CHECK(rtc13::fromBcd(0x23) == 23, "fromBcd 23");
}

static void testEpoch() {
  CHECK(rtc13::epochFromUtc(2000, 1, 1, 0, 0, 0) == 946684800LL, "epoch 2000-01-01");
  CHECK(rtc13::epochFromUtc(2026, 10, 10, 12, 34, 56) == 1791635696LL, "epoch 2026-10-10 12:34:56");
  CHECK(rtc13::epochFromUtc(2028, 2, 29, 23, 59, 59) == 1835481599LL, "epoch 2028-02-29 (rok przestępny)");
  CHECK(rtc13::epochFromUtc(2099, 12, 31, 23, 59, 59) == 4102444799LL, "epoch 2099-12-31");

  int y; unsigned mo, d, h, mi, s, wd;
  rtc13::utcFromEpoch(1791635696LL, y, mo, d, h, mi, s, wd);
  CHECK(y == 2026 && mo == 10 && d == 10 && h == 12 && mi == 34 && s == 56, "utc: pola daty");
  CHECK(wd == 6, "utc: 2026-10-10 to sobota (6)");
  rtc13::utcFromEpoch(1774747800LL, y, mo, d, h, mi, s, wd);
  CHECK(wd == 0 && mo == 3 && d == 29, "utc: 2026-03-29 to niedziela (0)");
  // Ujemna epoka (przed 1970) — zawijanie doby.
  rtc13::utcFromEpoch(-1, y, mo, d, h, mi, s, wd);
  CHECK(y == 1969 && mo == 12 && d == 31 && h == 23 && mi == 59 && s == 59, "utc: -1 s = 1969-12-31");
}

static void testReadDS1307() {
  FakeChip c;
  setRegs(c, 0x56, 0x34, 0x12, 0x07, 0x10, 0x10, 0x26);  // 2026-10-10 12:34:56
  int64_t t = 0;
  CHECK(rtc13::readUtc(busFor(c), t), "odczyt: poprawny zegar");
  CHECK(t == 1791635696LL, "odczyt: epoka zgodna");
}

static void testReadHaltedAndInvalid() {
  FakeChip c;
  setRegs(c, 0x80 | 0x56, 0x34, 0x12, 0x07, 0x10, 0x10, 0x26);  // CH=1
  int64_t t = 0;
  CHECK(!rtc13::readUtc(busFor(c), t), "odczyt: zatrzymany oscylator (CH) → false");

  setRegs(c, 0x00, 0x60, 0x12, 0x07, 0x10, 0x10, 0x26);  // minuty 60
  CHECK(!rtc13::readUtc(busFor(c), t), "odczyt: minuty 60 → false");

  setRegs(c, 0x00, 0x00, 0x24, 0x07, 0x10, 0x10, 0x26);  // godzina 24
  CHECK(!rtc13::readUtc(busFor(c), t), "odczyt: godzina 24 → false");

  setRegs(c, 0x00, 0x00, 0x12, 0x07, 0x10, 0x13, 0x26);  // miesiąc 13
  CHECK(!rtc13::readUtc(busFor(c), t), "odczyt: miesiąc 13 → false");

  setRegs(c, 0x00, 0x00, 0x12, 0x07, 0x00, 0x10, 0x26);  // dzień 0
  CHECK(!rtc13::readUtc(busFor(c), t), "odczyt: dzień 0 → false");
}

static void testReadTwelveHour() {
  FakeChip c;
  // 12-godzinny, PM: 0x40 (tryb) | 0x20 (PM) | 0x01 → 13:00
  setRegs(c, 0x00, 0x00, 0x40 | 0x20 | 0x01, 0x07, 0x10, 0x10, 0x26);
  int64_t t = 0;
  CHECK(rtc13::readUtc(busFor(c), t), "12 h: odczyt");
  CHECK(t == rtc13::epochFromUtc(2026, 10, 10, 13, 0, 0), "12 h PM: 01 → 13:00");

  // 12-godzinny, 12 AM: 0x40 | 0x12 → 00:00
  setRegs(c, 0x00, 0x00, 0x40 | 0x12, 0x07, 0x10, 0x10, 0x26);
  CHECK(rtc13::readUtc(busFor(c), t), "12 h: odczyt 12 AM");
  CHECK(t == rtc13::epochFromUtc(2026, 10, 10, 0, 0, 0), "12 h AM: 12 → 00:00");
}

static void testReadNoDevice() {
  FakeChip c;
  c.present = false;
  int64_t t = 0;
  CHECK(!rtc13::present(busFor(c)), "brak modułu: present() = false");
  CHECK(!rtc13::readUtc(busFor(c), t), "brak modułu: readUtc() = false");
  rtc13::Bus nullBus{nullptr, nullptr, nullptr};
  CHECK(!rtc13::present(nullBus), "null bus: present() = false");
  CHECK(!rtc13::readUtc(nullBus, t), "null bus: readUtc() = false");
}

static void testWriteRoundtrip() {
  const int64_t epochs[] = {
    1791635696LL,                       // 2026-10-10 12:34:56 (sobota)
    rtc13::epochFromUtc(2028, 2, 29, 23, 59, 59),
    946684800LL,                        // 2000-01-01 00:00:00
    rtc13::epochFromUtc(2099, 12, 31, 23, 59, 59),
  };
  for (int64_t e : epochs) {
    FakeChip c;
    CHECK(rtc13::writeUtc(busFor(c), rtc13::kChipDS1307, e), "zapis: DS1307");
    int64_t back = 0;
    CHECK(rtc13::readUtc(busFor(c), back) && back == e, "zapis → odczyt: ta sama epoka");
    CHECK((c.reg[0] & 0x80) == 0, "zapis: CH wyzerowany (oscylator działa)");
  }
}

static void testWriteRegisters() {
  FakeChip c;
  // 2026-10-10 12:34:56 UTC, sobota → dow 7 (w rejestrze 1..7, sobota = 7).
  CHECK(rtc13::writeUtc(busFor(c), rtc13::kChipDS1307, 1791635696LL), "zapis rejestrów");
  CHECK(c.lastWriteLen == 8 && c.lastWrite[0] == 0x00, "zapis: 8 bajtów od rejestru 0x00");
  CHECK(c.reg[0] == 0x56 && c.reg[1] == 0x34 && c.reg[2] == 0x12, "zapis: sek/min/godz w BCD");
  CHECK(c.reg[3] == 0x07, "zapis: dzień tygodnia 7 (sobota, 1 = niedziela)");
  CHECK(c.reg[4] == 0x10 && c.reg[5] == 0x10 && c.reg[6] == 0x26, "zapis: data 10.10.26");
  CHECK((c.reg[2] & 0x40) == 0, "zapis: tryb 24 h");
}

static void testWriteNiedziela() {
  FakeChip c;
  // 2026-03-29 to niedziela. Konwencja rejestru DS: 1 = poniedziałek … 7 = niedziela.
  rtc13::writeUtc(busFor(c), rtc13::kChipDS1307, 1774747800LL);
  CHECK(c.reg[3] == 0x01, "zapis: niedziela → rejestr 1");
}

static void testWriteDS3231ClearsOSF() {
  FakeChip c;
  c.reg[0x0F] = 0x80 | 0x08;  // OSF=1 (zegar był zatrzymany)
  CHECK(rtc13::writeUtc(busFor(c), rtc13::kChipDS3231, 1791635696LL), "DS3231: zapis");
  CHECK((c.reg[0x0F] & 0x80) == 0, "DS3231: OSF wyczyszczony");
  CHECK((c.reg[0x0F] & 0x08) == 0x08, "DS3231: pozostałe bity statusu nietknięte");
}

static void testWriteDS1307DoesNotTouchCtrl() {
  FakeChip c;
  c.reg[0x0F] = 0x80;
  CHECK(rtc13::writeUtc(busFor(c), rtc13::kChipDS1307, 1791635696LL), "DS1307: zapis");
  CHECK(c.reg[0x0F] == 0x80, "DS1307: rejestr 0x0F nietknięty");
}

static void testWriteFailures() {
  FakeChip c;
  c.nackWrite = true;
  CHECK(!rtc13::writeUtc(busFor(c), rtc13::kChipDS1307, 1791635696LL), "zapis NACK → false");
  c.nackWrite = false;
  CHECK(!rtc13::writeUtc(busFor(c), rtc13::kChipDS1307, rtc13::epochFromUtc(1999, 12, 31, 23, 59, 59)),
        "zapis poza 2000–2099 → false");
  CHECK(!rtc13::writeUtc(busFor(c), rtc13::kChipDS1307, rtc13::epochFromUtc(2100, 1, 1, 0, 0, 0)),
        "zapis 2100 → false");
  CHECK(c.writes == 0, "zapis NACK nie dociera do układu");
}

static void testPresent() {
  FakeChip c;
  CHECK(rtc13::present(busFor(c)), "obecność: ACK → true");
  c.present = false;
  CHECK(!rtc13::present(busFor(c)), "obecność: brak ACK → false");
}

int main() {
  testBcd();
  testEpoch();
  testReadDS1307();
  testReadHaltedAndInvalid();
  testReadTwelveHour();
  testReadNoDevice();
  testWriteRoundtrip();
  testWriteRegisters();
  testWriteNiedziela();
  testWriteDS3231ClearsOSF();
  testWriteDS1307DoesNotTouchCtrl();
  testWriteFailures();
  testPresent();
  std::printf("rtc: %d PASS, %d FAIL\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
