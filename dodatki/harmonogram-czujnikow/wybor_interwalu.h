#pragma once
// PROTOTYP DO TESTOW HOSTOWYCH. NIE JEST FIRMWARE I NIE JEST PODLACZONY DO ESP.
#include <stdint.h>
#include <math.h>

namespace HarmonogramCzujnikow {
struct Odczyt {
  float temperatura;
  bool poprawnyISwiezy; // Ocene stanu musi dostarczyc warstwa integracji.
};

// Interwaly w milisekundach; decyzja korzysta z ostatnich znanych odczytow.
static constexpr uint32_t PRACA_POMPY_MS = 2816;
static constexpr uint32_t POSTOJ_POMPY_MS = 15000;
static constexpr uint32_t ZIMNY_MS = 60000;
static constexpr uint32_t TROCINY_MS = 60000;
static constexpr uint32_t PANEL_MS = 60000;
static constexpr uint32_t BOJLER_MS = 60000;
static constexpr uint32_t ZEW1_MS = 120000;
static constexpr uint32_t POKOJ_MS = 120000;
static constexpr uint32_t DHT_MS = 120000;
static constexpr uint32_t CISNIENIE_MS = 180000;

inline bool zimnyOdczyt(const Odczyt& o) {
  return o.poprawnyISwiezy && isfinite(o.temperatura) && o.temperatura < 30.0f;
}

inline uint32_t interwalCoPowrot(bool pompaPracuje, const Odczyt& co,
                                const Odczyt& powrot, const Odczyt& trociny) {
  if (zimnyOdczyt(co) && zimnyOdczyt(powrot) && zimnyOdczyt(trociny))
    return ZIMNY_MS;
  return pompaPracuje ? PRACA_POMPY_MS : POSTOJ_POMPY_MS;
}

// Jeden termin na czujnik. Po skroceniu interwalu odczyt jest natychmiast
// nalezny, jesli poprzedni odczyt byl dawniej niz NOWY interwal.
// Faktyczny moment wymaga osobnej obslugi konwersji i zajetosci OneWire.
inline bool odczytNalezny(uint32_t teraz, uint32_t ostatniaProba,
                         bool bylaProba, uint32_t interwal) {
  return !bylaProba || uint32_t(teraz - ostatniaProba) >= interwal;
}
} // namespace HarmonogramCzujnikow
