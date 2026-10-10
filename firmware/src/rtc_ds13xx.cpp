// ═══════════════════════════════════════════════════════════════════════════
//  rtc_ds13xx.cpp — [4.7.0 ASTRO] patrz rtc_ds13xx.h.
//  Kalendarz: algorytm dni od 1970 (Howard Hinnant, domena publiczna).
// ═══════════════════════════════════════════════════════════════════════════
#include "rtc_ds13xx.h"

namespace rtc13 {

namespace {

constexpr uint8_t kRegSec  = 0x00;  // bit 7 = CH (zatrzymanie oscylatora)
constexpr uint8_t kRegCtrl = 0x0F;  // DS3231 status: bit 7 = OSF
constexpr uint8_t kCH  = 0x80;
constexpr uint8_t kOSF = 0x80;

// Dni od 1970-01-01 do podanej daty (proleptyczny kalendarz gregoriański).
int64_t daysFromCivil(int y, unsigned m, unsigned d) {
  y -= m <= 2;
  const int64_t era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (int64_t)doe - 719468;
}

void civilFromDays(int64_t z, int& y, unsigned& m, unsigned& d) {
  z += 719468;
  const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
  const unsigned doe = (unsigned)(z - era * 146097);
  const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  const int64_t yy = (int64_t)yoe + era * 400;
  const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const unsigned mp = (5 * doy + 2) / 153;
  d = doy - (153 * mp + 2) / 5 + 1;
  m = mp < 10 ? mp + 3 : mp - 9;
  y = (int)(yy + (m <= 2));
}

}  // namespace

uint8_t toBcd(unsigned v) {
  return (uint8_t)(((v / 10) << 4) | (v % 10));
}

int fromBcd(uint8_t b) {
  return (b >> 4) * 10 + (b & 0x0F);
}

int64_t epochFromUtc(int y, unsigned mo, unsigned d, unsigned h, unsigned mi, unsigned s) {
  return daysFromCivil(y, mo, d) * 86400 + (int64_t)h * 3600 + (int64_t)mi * 60 + s;
}

void utcFromEpoch(int64_t t, int& y, unsigned& mo, unsigned& d,
                  unsigned& h, unsigned& mi, unsigned& s, unsigned& wday) {
  int64_t days = t / 86400;
  int64_t rem = t % 86400;
  if (rem < 0) { rem += 86400; days -= 1; }
  civilFromDays(days, y, mo, d);
  h = (unsigned)(rem / 3600);
  mi = (unsigned)((rem % 3600) / 60);
  s = (unsigned)(rem % 60);
  int64_t w = (days + 4) % 7;  // 1970-01-01 był czwartkiem (4)
  if (w < 0) w += 7;
  wday = (unsigned)w;
}

bool present(const Bus& bus) {
  if (!bus.readReg) return false;
  uint8_t b = 0;
  return bus.readReg(bus.ctx, kAddr, kRegSec, &b, 1);
}

bool readUtc(const Bus& bus, int64_t& outEpoch) {
  if (!bus.readReg) return false;
  uint8_t r[7] = {0};
  if (!bus.readReg(bus.ctx, kAddr, kRegSec, r, sizeof(r))) return false;
  if (r[0] & kCH) return false;  // zegar zatrzymany (nowy układ / brak baterii)

  unsigned sec = (unsigned)fromBcd(r[0] & 0x7F);
  unsigned min = (unsigned)fromBcd(r[1] & 0x7F);
  unsigned hour;
  if (r[2] & 0x40) {  // tryb 12-godzinny
    hour = (unsigned)fromBcd(r[2] & 0x1F);
    if (hour == 12) hour = 0;
    if (r[2] & 0x20) hour += 12;  // PM
  } else {
    hour = (unsigned)fromBcd(r[2] & 0x3F);
  }
  unsigned date = (unsigned)fromBcd(r[4] & 0x3F);
  unsigned mon  = (unsigned)fromBcd(r[5] & 0x1F);
  int year = 2000 + fromBcd(r[6]);

  if (sec > 59 || min > 59 || hour > 23) return false;
  if (mon < 1 || mon > 12 || date < 1 || date > 31) return false;
  outEpoch = epochFromUtc(year, mon, date, hour, min, sec);
  return true;
}

bool writeUtc(const Bus& bus, int chip, int64_t epoch) {
  if (!bus.write || !bus.readReg) return false;
  int y; unsigned mo, d, h, mi, s, wday;
  utcFromEpoch(epoch, y, mo, d, h, mi, s, wday);
  if (y < 2000 || y > 2099) return false;

  // Rejestry 0x00..0x06: sec (CH=0), min, hour (24 h), dow (1..7), date, month, year.
  uint8_t buf[8];
  buf[0] = kRegSec;
  buf[1] = toBcd(s);
  buf[2] = toBcd(mi);
  buf[3] = toBcd(h);
  buf[4] = toBcd(wday + 1);  // rejestr dnia: 1 = niedziela … 7 = sobota (odczyt go nie używa)
  buf[5] = toBcd(d);
  buf[6] = toBcd(mo);
  buf[7] = toBcd((unsigned)(y - 2000));
  if (!bus.write(bus.ctx, kAddr, buf, sizeof(buf))) return false;

  if (chip == kChipDS3231) {
    uint8_t ctrl = 0;
    if (!bus.readReg(bus.ctx, kAddr, kRegCtrl, &ctrl, 1)) return false;
    if (ctrl & kOSF) {
      uint8_t out[2] = {kRegCtrl, (uint8_t)(ctrl & ~kOSF)};
      if (!bus.write(bus.ctx, kAddr, out, sizeof(out))) return false;
    }
  }
  return true;
}

}  // namespace rtc13
