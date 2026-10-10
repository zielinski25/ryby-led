// ═══════════════════════════════════════════════════════════════════════════
//  test_histlong.cpp — testy hostowe modułu histlong (4.5.0 HIST-LONG)
//  Kompilowany z firmware/src/histlong.cpp + atrapami stubs/ (LittleFS → katalog).
// ═══════════════════════════════════════════════════════════════════════════
#include "histlong.h"
#include <LittleFS.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static int g_pass = 0, g_fail = 0;
#define CHECK(cond, name)                                                     \
  do {                                                                        \
    if (cond) { ++g_pass; }                                                   \
    else { ++g_fail; std::printf("FAIL: %s  (linia %d)\n", name, __LINE__); } \
  } while (0)

static std::string root() {
  const char* r = std::getenv("TSPOOL_FS_ROOT");
  return r ? r : "/tmp/tspool-fakefs";
}
static void resetFs() {
  fs::remove_all(root());
  fs::create_directories(root());
}
static std::string readAll(const char* path) {
  std::ifstream in(root() + path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}
static std::vector<std::string> lines(const std::string& s) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : s) {
    if (c == '\n') { out.push_back(cur); cur.clear(); }
    else cur += c;
  }
  if (!cur.empty()) out.push_back(cur);
  return out;
}
static std::vector<std::string> split(const std::string& s, char d) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : s) {
    if (c == d) { out.push_back(cur); cur.clear(); }
    else cur += c;
  }
  out.push_back(cur);
  return out;
}

static histlong::Sample base() {
  histlong::Sample s{};
  s.tWater = 24.0f; s.tPlate1 = 30.0f; s.tPlate2 = 31.0f;
  s.luxRoom = 100.0f; s.luxWater = 200.0f;
  for (int i = 0; i < 5; i++) s.pwm[i] = 10 * (i + 1);
  s.powerW = 40.0f;
  s.autoMode = true; s.minLux = false; s.adapt = false;
  return s;
}

static const uint32_t T0 = 1800u * 1000u;   // początek kubełka nr 1000

static void testNoFsNoWrite() {
  resetFs();
  histlong::init(false);
  CHECK(!histlong::addSample(T0, base()), "fs niegotowy -> addSample=false");
  CHECK(!fs::exists(root() + histlong::FILE_CUR), "fs niegotowy -> brak pliku");
}

static void testBucketAveragesAndFlush() {
  resetFs();
  histlong::init(true);
  histlong::Sample a = base(); a.tWater = 24.0f; a.pwm[0] = 10;
  histlong::Sample b = base(); b.tWater = 25.0f; b.pwm[0] = 20;
  histlong::Sample c = base(); c.tWater = 26.0f; c.pwm[0] = 31;
  histlong::addSample(T0 + 0,   a);
  histlong::addSample(T0 + 300, b);
  histlong::addSample(T0 + 600, c);
  CHECK(!fs::exists(root() + histlong::FILE_CUR), "kubelek otwarty -> jeszcze nic nie zapisane");
  // pierwsza próbka następnego kubełka domyka poprzedni
  histlong::addSample(T0 + 1800, base());
  auto L = lines(readAll(histlong::FILE_CUR));
  CHECK(L.size() == 2, "naglowek + 1 wiersz kubelka");
  if (L.size() == 2) {
    CHECK(L[0] == histlong::HEADER, "naglowek zgodny z HEADER");
    auto F = split(L[1], ',');
    CHECK(F.size() == 16, "wiersz ma 16 pol");
    if (F.size() == 16) {
      CHECK(F[0] == std::to_string(T0), "ts = poczatek kubelka (wyrownany do 30 min)");
      CHECK(F[1] == "25.0", "t_woda = srednia 24/25/26");
      CHECK(F[6] == "20", "pwm0 = srednia 10/20/31 zaokraglona (20)");
      CHECK(F[11] == "40.0", "moc_W = srednia 40");
      CHECK(F[12] == "AUTO", "tryb = ostatnia probka");
      CHECK(F[15] == "3", "probki = 3");
    }
  }
}

static void testInvalidReadingsSkipped() {
  resetFs();
  histlong::init(true);
  histlong::Sample a = base(); a.tWater = -127.0f;      // błąd DS18B20 — pominięty
  histlong::Sample b = base(); b.tWater = 20.0f;
  histlong::Sample c = base(); c.tWater = NAN;          // NaN — pominięty
  histlong::Sample d = base(); d.tWater = 20.0f; d.luxWater = -1.0f;   // ujemne lux — pominięte
  histlong::addSample(T0, a);
  histlong::addSample(T0 + 300, b);
  histlong::addSample(T0 + 600, c);
  histlong::addSample(T0 + 900, d);
  histlong::addSample(T0 + 1800, base());
  auto F = split(lines(readAll(histlong::FILE_CUR))[1], ',');
  CHECK(F[1] == "20.0", "t_woda: tylko poprawny odczyt (20.0)");
  CHECK(F[5] == "200", "lux_woda: ujemna probka pominieta, srednia z reszty");
  CHECK(F[15] == "4", "probki liczy wszystkie 4 (nawet z bledami temp)");
}

static void testEmptyChannelLeftBlank() {
  resetFs();
  histlong::init(true);
  histlong::Sample a = base(); a.tPlate2 = -127.0f;
  histlong::addSample(T0, a);
  histlong::addSample(T0 + 1800, base());
  auto F = split(lines(readAll(histlong::FILE_CUR))[1], ',');
  CHECK(F[3] == "", "kanal bez poprawnych odczytow -> pole puste");
}

static void testFlagsFromLastSample() {
  resetFs();
  histlong::init(true);
  histlong::Sample a = base(); a.autoMode = true; a.minLux = true; a.adapt = true;
  histlong::Sample b = base(); b.autoMode = false; b.minLux = false; b.adapt = false;
  histlong::addSample(T0, a);
  histlong::addSample(T0 + 300, b);
  histlong::addSample(T0 + 1800, base());
  auto F = split(lines(readAll(histlong::FILE_CUR))[1], ',');
  CHECK(F[12] == "MAN", "tryb z ostatniej probki kubelka (MAN)");
  CHECK(F[13] == "0" && F[14] == "0", "minLux/adapt z ostatniej probki");
}

static void testRotationAndStreamOrder() {
  resetFs();
  histlong::init(true);
  // ~500 kubełków → kilka rotacji (MAX_BYTES = 40 KB, wiersz ~70 B).
  const int N = 1500;
  for (int i = 0; i < N; ++i) {
    histlong::addSample(T0 + (uint32_t)i * 1800u, base());
  }
  histlong::addSample(T0 + (uint32_t)N * 1800u, base());  // domknij ostatni
  CHECK(fs::exists(root() + histlong::FILE_OLD), "rotacja utworzyla archiwum");
  CHECK(fs::file_size(root() + histlong::FILE_CUR) <= histlong::MAX_BYTES + 200,
        "bieżący plik ~<= MAX_BYTES");
  CHECK(fs::file_size(root() + histlong::FILE_OLD) <= histlong::MAX_BYTES + 200,
        "archiwum ~<= MAX_BYTES");

  // Strumień: archiwum + bieżący, nagłówek tylko raz, ts rosnące.
  struct Ctx { std::string out; };
  Ctx ctx;
  size_t total = histlong::streamTo(&ctx, [](void* c, const uint8_t* d, size_t n) -> size_t {
    static_cast<Ctx*>(c)->out.append(reinterpret_cast<const char*>(d), n);
    return n;
  });
  CHECK(total == ctx.out.size(), "streamTo zwraca liczbe wyslanych bajtow");
  auto L = lines(ctx.out);
  size_t headers = 0;
  for (auto& l : L) if (l == histlong::HEADER) headers++;
  CHECK(headers == 1, "naglowek wystepuje dokladnie raz w strumieniu");
  CHECK(!L.empty() && L[0] == histlong::HEADER, "strumien zaczyna sie od naglowka");
  bool monotonic = true, complete = true;
  long long prev = -1;
  for (size_t i = 1; i < L.size(); ++i) {
    long long ts = std::atoll(split(L[i], ',')[0].c_str());
    if (ts <= prev) monotonic = false;
    prev = ts;
  }
  CHECK(monotonic, "ts rosnaco przez archiwum i biezacy plik");
  // Ostatni kubełek (N) musi być w strumieniu.
  long long last = (long long)(T0 + (uint32_t)(N - 1) * 1800u);
  CHECK(prev == last, "ostatni wiersz w strumieniu = ostatni domkniety kubelek");
  (void)complete;
}

static void testStreamWriteFailureStops() {
  resetFs();
  histlong::init(true);
  histlong::addSample(T0, base());
  histlong::addSample(T0 + 1800, base());
  size_t calls = 0;
  size_t total = histlong::streamTo(&calls, [](void* c, const uint8_t*, size_t) -> size_t {
    ++*static_cast<size_t*>(c);
    return 0;   // klient zerwał połączenie
  });
  CHECK(total == 0, "blad zapisu -> 0 bajtow");
  CHECK(calls == 1, "blad zapisu -> przerwanie po pierwszym wywolaniu");
}

static void testStreamEmpty() {
  resetFs();
  histlong::init(true);
  struct Ctx { size_t n = 0; } ctx;
  size_t total = histlong::streamTo(&ctx, [](void* c, const uint8_t* , size_t n) -> size_t {
    static_cast<Ctx*>(c)->n += n;
    return n;
  });
  CHECK(total == 0 && ctx.n == 0, "brak plikow -> pusty strumien");
}

static void testClear() {
  resetFs();
  histlong::init(true);
  for (int i = 0; i < 1500; ++i) histlong::addSample(T0 + (uint32_t)i * 1800u, base());
  CHECK(histlong::totalBytes() > 0, "przed clear sa dane");
  histlong::clear();
  CHECK(!fs::exists(root() + histlong::FILE_CUR), "clear usuwa bieżący");
  CHECK(!fs::exists(root() + histlong::FILE_OLD), "clear usuwa archiwum");
  CHECK(histlong::totalBytes() == 0, "po clear 0 bajtow");
  // Bufor kubełka zresetowany: nowa próbka nie domyka starego kubełka.
  histlong::addSample(T0 + 90000, base());
  histlong::addSample(T0 + 90000 + 1800, base());
  auto L = lines(readAll(histlong::FILE_CUR));
  CHECK(L.size() == 2, "po clear tylko naglowek + jeden nowy wiersz");
}

static void testRestartResumesAppend() {
  resetFs();
  histlong::init(true);
  histlong::addSample(T0, base());
  histlong::addSample(T0 + 1800, base());      // zapisuje kubełek T0
  histlong::init(true);                        // "restart": nowy stan RAM, pliki zostają
  histlong::addSample(T0 + 3600, base());
  histlong::addSample(T0 + 5400, base());      // zapisuje kubełek T0+1800
  auto L = lines(readAll(histlong::FILE_CUR));
  size_t headers = 0;
  for (auto& l : L) if (l == histlong::HEADER) headers++;
  CHECK(headers == 1, "po restarcie nagłówek nie jest dublowany");
  CHECK(L.size() == 3, "po restarcie: naglowek + 2 wiersze");
}

static void testCursorSmallChunksAndTimeout() {
  resetFs();
  histlong::init(true);
  for (int i = 0; i < 1500; ++i) histlong::addSample(T0 + (uint32_t)i * 1800u, base());
  histlong::addSample(T0 + 1500u * 1800u, base());
  // Odczyt małymi kawałkami (7 B) — nagłówek łamie się między odczytami.
  std::string out;
  CHECK(histlong::streamBegin(1000), "kursor: start");
  CHECK(!histlong::streamBegin(1001), "kursor: drugi klient w trakcie -> odmowa");
  uint8_t buf[7];
  for (int guard = 0; guard < 1000000; ++guard) {
    size_t n = histlong::streamRead(buf, sizeof(buf), 1002);
    if (n == 0) break;
    out.append(reinterpret_cast<const char*>(buf), n);
  }
  auto L = lines(out);
  size_t headers = 0;
  for (auto& l : L) if (l == histlong::HEADER) headers++;
  CHECK(headers == 1, "kursor 7B: naglowek raz");
  CHECK(!L.empty() && L[0] == histlong::HEADER, "kursor 7B: zaczyna sie od naglowka");
  std::printf("  info: wiersze=%zu bajty=%zu\n", L.size(), out.size());
  // Retencja: ponad tydzień (336 wierszy) i nie więcej niż dwa pliki po MAX_BYTES.
  CHECK(L.size() > 336, "kursor 7B: wiecej niz tydzien wierszy (archiwum + biezacy)");
  CHECK(out.size() <= 2 * histlong::MAX_BYTES + 2048, "kursor 7B: retencja <= 2 x MAX_BYTES");
  // Po zakończeniu nowy klient może wystartować od razu.
  CHECK(histlong::streamBegin(1003), "kursor: po koncu nowy start OK");
  histlong::streamAbort();

  // Klient zniknął w połowie: po timeoutcie kursor wygasa i można zacząć od nowa.
  CHECK(histlong::streamBegin(5000), "kursor: start 2");
  histlong::streamRead(buf, sizeof(buf), 5001);
  CHECK(!histlong::streamBegin(5000 + histlong::STREAM_TIMEOUT_MS - 1), "kursor: przed timeoutem zajety");
  CHECK(histlong::streamBegin(5001 + histlong::STREAM_TIMEOUT_MS), "kursor: po timeoutcie wygasa");
  histlong::streamAbort();
}

static void testCursorNoFsNoStart() {
  resetFs();
  histlong::init(false);
  CHECK(!histlong::streamBegin(1), "kursor: fs niegotowy -> brak startu");
}

int main() {
  testNoFsNoWrite();
  testBucketAveragesAndFlush();
  testInvalidReadingsSkipped();
  testEmptyChannelLeftBlank();
  testFlagsFromLastSample();
  testRotationAndStreamOrder();
  testStreamWriteFailureStops();
  testStreamEmpty();
  testClear();
  testRestartResumesAppend();
  testCursorSmallChunksAndTimeout();
  testCursorNoFsNoStart();
  std::printf("histlong: %d PASS, %d FAIL\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
