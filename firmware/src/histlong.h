// ═══════════════════════════════════════════════════════════════════════════
//  histlong.h — [4.5.0 HIST-LONG] historia długa (Etap 3, punkt 4 planu upgrade)
//
//  /history.csv (24 h, co 5 min, bez daty) zostaje bez zmian. Ten moduł dokłada
//  DŁUGĄ historię pod wykresy tygodniowe: jeden wiersz na kubełek 30 min
//  (średnie z próbek w kubełku), ze znacznikiem unix `ts` i liczbą próbek.
//
//  Pliki LittleFS:
//    /history_long.csv      — bieżący (dopisywany)
//    /history_long_old.csv  — archiwum po rotacji (nadpisywane)
//  Rotacja przy przekroczeniu MAX_BYTES (~500 wierszy ≈ 10 dni na plik).
//
//  Wiersze bez czasu (NTP niezsynchronizowany) NIE trafiają tu — wołający sprawdza.
//  Niedomknięty kubełek przy restarcie przepada (≤30 min danych).
//
//  Moduł nie zna Ryby (tylko LittleFS). Testy: firmware/tests/host/test_histlong.cpp.
// ═══════════════════════════════════════════════════════════════════════════
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace histlong {

constexpr const char* FILE_CUR   = "/history_long.csv";
constexpr const char* FILE_OLD   = "/history_long_old.csv";
constexpr size_t      MAX_BYTES  = 40 * 1024;
constexpr uint32_t    BUCKET_SEC = 30 * 60;   // 30 minut
constexpr const char* HEADER =
  "ts,t_woda,t_plyta1,t_plyta2,lux_pokoj,lux_woda,pwm0,pwm1,pwm2,pwm3,pwm4,moc_W,tryb,minLux,adapt,probki";

// Jedna próbka z saveHistoryPoint() (co 5 min).
struct Sample {
  float tWater;     // °C; poza zakresem (np. -127 = błąd DS18B20) → pomijane w średniej
  float tPlate1;
  float tPlate2;
  float luxRoom;    // lx; ujemne/NaN → pomijane
  float luxWater;
  int   pwm[5];     // %
  float powerW;     // W
  bool  autoMode;   // stan z OSTATNIEJ próbki kubełka
  bool  minLux;
  bool  adapt;
};

// Wołać raz po LittleFS.begin(). fsReady=false → addSample zwraca false, nic nie zapisuje.
void init(bool fsReady);

// Akumuluje próbkę w bieżącym kubełku (ts/BUCKET_SEC). Przy przejściu do nowego kubełka
// zapisuje wiersz poprzedniego. Zwraca false, gdy nie udało się zapisać wiersza.
bool addSample(uint32_t unixTs, const Sample& s);

// Kasuje oba pliki i bufor kubełka (/api/history/clear).
void clear();

// Strumieniowanie archiwum (jeśli jest) i bieżącego pliku, nagłówek tylko raz.
// Kursor jest jeden naraz (jedna odpowiedź HTTP). Odpowiedź chunked czyta kawałkami —
// cała historia nie trafia do RAM. Gdy klient zniknie w połowie, kursor wygasa po
// STREAM_TIMEOUT_MS i zwalnia pliki przy następnym żądaniu.
constexpr uint32_t STREAM_TIMEOUT_MS = 10000;
bool   streamBegin(uint32_t nowMs);                       // false = zajęty albo fs niegotowy
size_t streamRead(uint8_t* buf, size_t maxLen, uint32_t nowMs);  // 0 = koniec strumienia
void   streamAbort();

// Wygodna wersja dla testów/narzędzi: cały strumień przez write(ctx, data, n).
// write zwraca liczbę zapisanych bajtów (0 = błąd → przerwanie).
typedef size_t (*WriteFn)(void* ctx, const uint8_t* data, size_t n);
size_t streamTo(void* ctx, WriteFn write);

// Rozmiar bieżącego + archiwum (bajty).
size_t totalBytes();

}  // namespace histlong
