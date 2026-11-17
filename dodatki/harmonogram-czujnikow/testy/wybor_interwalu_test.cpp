#include "../wybor_interwalu.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
using namespace HarmonogramCzujnikow;
int main() {
  Odczyt zimny{29.9f, true}, prog{30.0f, true}, goracy{60.0f, true};
  Odczyt blad{20.0f, false}, nan{NAN, true};
  assert(interwalCoPowrot(false, zimny, zimny, zimny) == 60000);
  assert(interwalCoPowrot(true, zimny, zimny, zimny) == 60000);
  assert(interwalCoPowrot(false, prog, zimny, zimny) == 15000);
  assert(interwalCoPowrot(true, zimny, prog, zimny) == 2816);
  assert(interwalCoPowrot(true, zimny, zimny, goracy) == 2816);
  assert(interwalCoPowrot(false, blad, zimny, zimny) == 15000);
  assert(interwalCoPowrot(true, zimny, nan, zimny) == 2816);
  assert(interwalCoPowrot(false, zimny, zimny, blad) == 15000);
  assert(odczytNalezny(0, 0, false, 60000));
  assert(!odczytNalezny(59999, 0, true, 60000));
  assert(odczytNalezny(60000, 0, true, 60000));
  assert(odczytNalezny(20000, 0, true, 15000));
  assert(!odczytNalezny(0x000003e7u, 0xfffffff0u, true, 1016));
  assert(odczytNalezny(0x000003e8u, 0xfffffff0u, true, 1016));
  assert(TROCINY_MS == 60000 && PANEL_MS == 60000 && BOJLER_MS == 60000);
  assert(ZEW1_MS == 120000 && POKOJ_MS == 120000 && DHT_MS == 120000);
  assert(CISNIENIE_MS == 180000);
  puts("OK: prototyp wyboru interwalu, progi i przepelnienie licznika");
}
