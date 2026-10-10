// ═══════════════════════════════════════════════════════════════════════════
//  rtc_ds13xx.h — [4.7.0 ASTRO] Etap 6 pkt 2: zegar odporny na brak NTP.
//
//  Sterownik DS1307 i DS3231 (I²C, adres 0x68). Oba układy mają ten sam układ
//  rejestrów czasu 0x00–0x06 (BCD), więc jeden kod obsługuje oba. Bez RTClib,
//  bez dodatkowej zależności. Magistrala podawana jako wskaźniki funkcji
//  (w firmware: Wire; w testach: fałszywa magistrala).
//
//  Czas w RTC przechowujemy jako UTC. Odczyt zwraca epokę UTC.
//  Brak modułu, zatrzymany oscylator (bit CH) albo zły zapis → false.
//  Ryby używa tego tylko gdy użytkownik włączy RTC w panelu (domyślnie wyłączone).
//  Testy: firmware/tests/host/test_rtc.cpp.
// ═══════════════════════════════════════════════════════════════════════════
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace rtc13 {

constexpr uint8_t kAddr = 0x68;

enum Chip : int { kChipNone = 0, kChipDS1307 = 1307, kChipDS3231 = 3231 };

// Magistrala I²C jako wskaźniki funkcji (kontekst przekazywany jako void*).
struct Bus {
  void* ctx;
  // Zapis n bajtów do urządzenia addr. true = ACK i sukces.
  bool (*write)(void* ctx, uint8_t addr, const uint8_t* buf, size_t n);
  // Ustawia wskaźnik rejestru reg, potem czyta n bajtów. true = sukces.
  bool (*readReg)(void* ctx, uint8_t addr, uint8_t reg, uint8_t* buf, size_t n);
};

// Czy pod adresem 0x68 odpowiada urządzenie (ACK na odczyt rejestru 0x00).
bool present(const Bus& bus);

// Odczyt czasu UTC (epoka sekund). false: brak modułu, CH=1 (zegar zatrzymany),
// niepoprawne pola BCD albo zakres dat spoza 2000–2099.
bool readUtc(const Bus& bus, int64_t& outEpoch);

// Zapis czasu UTC. Zeruje bit CH (start oscylatora). Dla DS3231 dodatkowo
// czyści flagę OSF (rejestr 0x0F, bit 7). chip = kChipDS1307 albo kChipDS3231.
bool writeUtc(const Bus& bus, int chip, int64_t epoch);

// Pomocnicze (eksportowane do testów).
uint8_t toBcd(unsigned v);
int fromBcd(uint8_t b);
int64_t epochFromUtc(int y, unsigned mo, unsigned d, unsigned h, unsigned mi, unsigned s);
// Rozbija epokę UTC na pola; wday: 0 = niedziela.
void utcFromEpoch(int64_t t, int& y, unsigned& mo, unsigned& d,
                  unsigned& h, unsigned& mi, unsigned& s, unsigned& wday);

}  // namespace rtc13
