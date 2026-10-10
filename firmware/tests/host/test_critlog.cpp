// ═══════════════════════════════════════════════════════════════════════════
//  test_critlog.cpp — testy hostowe modułu critlog (4.4.0 CRIT-LOG)
//  Kompilowany z firmware/src/critlog.cpp + atrapami stubs/ (LittleFS → katalog).
//  Uruchamiane przez run_tests.sh.
// ═══════════════════════════════════════════════════════════════════════════
#include "critlog.h"
#include <LittleFS.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

static int g_pass = 0, g_fail = 0;
#define CHECK(cond, name)                                                  \
  do {                                                                     \
    if (cond) { ++g_pass; }                                                \
    else { ++g_fail; std::printf("FAIL: %s  (linia %d)\n", name, __LINE__); } \
  } while (0)

static std::string root() {
  const char* r = std::getenv("TSPOOL_FS_ROOT");
  return r ? r : "/tmp/tspool-fakefs";
}

// Czyści katalog FS przed każdym testem (LittleFS stub mapuje ścieżki na root).
static void resetFs() {
  fs::remove_all(root());
  fs::create_directories(root());
}

static std::string readAll(const char* path) {
  std::ifstream in(root() + path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

static size_t countLines(const std::string& s) {
  size_t n = 0;
  for (char c : s) if (c == '\n') ++n;
  return n;
}

static void testFsNotReady() {
  resetFs();
  critlog::init(false);
  CHECK(!critlog::append("x"), "fs niegotowy -> append=false");
  CHECK(!fs::exists(root() + critlog::FILE_CUR), "fs niegotowy -> brak pliku");
  CHECK(critlog::currentSize() == 0, "fs niegotowy -> size 0");
}

static void testBasicAppend() {
  resetFs();
  critlog::init(true);
  CHECK(critlog::append("linia 1"), "append #1 ok");
  CHECK(critlog::append("linia 2\n"), "append #2 ok (z \\n)");
  CHECK(critlog::append("linia 3"), "append #3 ok");
  std::string s = readAll(critlog::FILE_CUR);
  CHECK(s == "linia 1\nlinia 2\nlinia 3\n", "zawartosc: 3 linie, kazda z \\n");
  CHECK(critlog::currentSize() == s.size(), "currentSize == rozmiar pliku");
  CHECK(!critlog::hasOld(), "bez rotacji brak archiwum");
}

static void testPersistsAcrossInit() {
  resetFs();
  critlog::init(true);
  critlog::append("przed restartem");
  size_t before = critlog::currentSize();
  // symulacja restartu: ponowna inicjalizacja odczytuje rozmiar z flash
  critlog::init(true);
  CHECK(critlog::currentSize() == before, "po init rozmiar odczytany z pliku");
  critlog::append("po restarcie");
  CHECK(countLines(readAll(critlog::FILE_CUR)) == 2, "wpisy sie kumuluja po restarcie");
}

static void testLongLineTruncated() {
  resetFs();
  critlog::init(true);
  std::string big(5000, 'A');
  CHECK(critlog::append(big.c_str()), "dluga linia: append ok");
  std::string s = readAll(critlog::FILE_CUR);
  CHECK(s.size() == critlog::MAX_LINE + 1, "dluga linia ucieta do MAX_LINE + \\n");
  CHECK(!s.empty() && s.back() == '\n', "ucieta linia konczy sie \\n");
}

static void testRotation() {
  resetFs();
  critlog::init(true);
  // linie ~1000 B -> po ~260 wpisach przekroczony MAX_BYTES (256 KB) -> rotacja
  const int N = 400;
  for (int i = 0; i < N; ++i) {
    char buf[1100];
    std::snprintf(buf, sizeof(buf), "%04d|", i);
    std::string line(buf);
    line += std::string(990, 'x');
    CHECK(critlog::append(line.c_str()), "append w petli rotacji");
  }
  CHECK(critlog::hasOld(), "rotacja utworzyla archiwum");
  CHECK(critlog::currentSize() <= critlog::MAX_BYTES, "bieżący plik <= MAX_BYTES");
  std::string cur = readAll(critlog::FILE_CUR);
  std::string old = readAll(critlog::FILE_OLD);
  CHECK(old.size() <= critlog::MAX_BYTES + critlog::MAX_LINE + 1, "archiwum <= MAX_BYTES (+1 linia)");
  // Ostatni wpis musi być w bieżącym pliku, a ciąg 0..N-1 nie może mieć dziury w środku:
  CHECK(cur.find("0399|") != std::string::npos, "ostatni wpis w bieżącym pliku");
  CHECK(old.find("0000|") != std::string::npos || cur.find("0000|") != std::string::npos,
        "najstarszy wpis zachowany (archiwum lub biezacy)");
  // Łączna liczba wpisów: archiwum + bieżący = wszystkie, które nie wypadły z dwóch plików.
  CHECK(countLines(cur) + countLines(old) <= (size_t)N, "liczba wpisow spójna");
  CHECK(countLines(cur) >= 1, "bieżący plik niepusty po rotacji");
}

static void testRotationReplacesOldArchive() {
  resetFs();
  critlog::init(true);
  // Ręcznie przygotowane stare archiwum — ma zostać nadpisane, nie dopisane.
  std::ofstream(root() + critlog::FILE_OLD) << "STARE ARCHIWUM\n";
  std::string big(900, 'y');
  for (int i = 0; i < 300; ++i) critlog::append(big.c_str());  // wymusza rotację
  std::string old = readAll(critlog::FILE_OLD);
  CHECK(old.find("STARE ARCHIWUM") == std::string::npos, "rotacja nadpisuje stare archiwum");
}

static void testFreshDeviceEmptyLog() {
  resetFs();
  critlog::init(true);
  CHECK(critlog::currentSize() == 0, "brak pliku -> size 0");
  CHECK(!critlog::hasOld(), "brak pliku -> brak archiwum");
}

int main() {
  testFsNotReady();
  testBasicAppend();
  testPersistsAcrossInit();
  testLongLineTruncated();
  testRotation();
  testRotationReplacesOldArchive();
  testFreshDeviceEmptyLog();
  std::printf("critlog: %d PASS, %d FAIL\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
