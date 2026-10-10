// ═══════════════════════════════════════════════════════════════════
//  telemetry_spool.h — Etap 3 planu upgrade (v4.2.0 / Spool V2 dla Ryb)
//
//  Port wzorca z Centrali pieca (Persistent Spool V2 + TelemetryRing +
//  sessionNonce, serie COMMIT-B/G, moduły R245–R282) — uproszczony do
//  domeny akwarium. Świadome różnice:
//    • próbka 48 B (zamiast 80 B) — kanały akwarium: 3 temperatury,
//      2 lux, 5 PWM, moc, flagi trybu;
//    • rekord = próbka + CRC32 (52 B), strona = HEADER 64 B + N × 52 B +
//      FOOTER 64 B (footer = commit; brak footera = strona otwarta/uszkodzona);
//    • klucz RTDB = sessionNonce + sampleSeq → replay jest IDEMPOTENTNY
//      (ponowne wysłanie tego samego rekordu nadpisuje ten sam węzeł);
//    • wysyłka jako operacja asynchroniczna w schedulerze FB (Core 0),
//      nie jako blokujący wait-loop.
//
//  Zasady współbieżności: producent i konsument działają WYŁĄCZNIE na
//  Core 0 (tgTaskFn → saveHistoryPoint / telemetrySpillTick). Ring jest
//  SPSC, a head/tail są publikowane acquire/release — gotowe także na
//  przyszłe przeniesienie producenta na inny rdzeń.
//
//  Ten plik nie zależy od Arduino.h — wszystkie zależności platformowe
//  (LittleFS, ps_malloc, esp_random, millis) są w telemetry_spool.cpp.
// ═══════════════════════════════════════════════════════════════════
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace tspool {

// ── Stałe polityki ───────────────────────────────────────────────────
// Próbka co 5 minut (jak wpis historii CSV) → 288 próbek = 24 h w RAM.
static constexpr uint32_t RING_CAPACITY        = 288U;
static constexpr uint32_t PAGE_MAX_RECORDS     = 288U;   // jedna strona = maks. doba
static constexpr uint32_t REPLAY_BATCH_MAX     = 12U;    // rekordów na jeden update RTDB
static constexpr uint32_t MAX_PAGES_ON_FS      = 240U;   // ~20 dni przy spill co dobę
static constexpr size_t   FS_MIN_FREE_BYTES    = 64UL * 1024UL; // poniżej → kasuj najstarszą stronę
static constexpr uint32_t SPILL_RING_WATERMARK = 6U;     // 6 × 5 min = 30 min w RAM → spill
static constexpr uint32_t SPILL_OFFLINE_MS     = 60UL * 1000UL; // offline dłużej niż to → spill
static constexpr uint32_t SPILL_COOLDOWN_MS    = 60UL * 1000UL;
static constexpr size_t   JSON_BUFFER_BYTES    = 6144U;  // bufor batcha (PSRAM)

static constexpr const char* SPOOL_DIR         = "/spool";
static constexpr uint32_t PAGE_MAGIC           = 0x53504C32UL; // "SPL2"
static constexpr uint16_t PAGE_FORMAT_VERSION  = 1U;
static constexpr uint32_t PAGE_COMMIT_MARKER   = 0x434F4D4DUL; // "COMM"
static constexpr int16_t  NA_X10               = INT16_MIN;     // brak odczytu (temperatura)
static constexpr uint16_t NA_U16               = 0xFFFFU;       // brak odczytu (lux)

// ── Próbka domenowa (ABI zamrożone: 48 B) ───────────────────────────
#pragma pack(push, 1)
struct Sample {
  uint64_t sessionNonce;  //  0  efemeryczny identyfikator uruchomienia (esp_random)
  uint32_t sampleSeq;     //  8  monotoniczny w obrębie sesji, od 1
  uint32_t unixTs;        // 12  0 gdy brak NTP
  uint32_t uptimeS;       // 16
  int16_t  tWaterX10;     // 20  °C × 10, NA_X10 = brak
  int16_t  tPlate1X10;    // 22
  int16_t  tPlate2X10;    // 24
  uint16_t luxRoom;       // 26  lx (zaokrąglone), NA_U16 = brak
  uint16_t luxWater;      // 28  lx pod wodą
  uint16_t pwm[5];        // 30  PWM kanałów w % (0..100)
  uint16_t powerWX10;     // 40  moc chwilowa W × 10
  int8_t   wifiRssi;      // 42  dBm
  uint8_t  stateBits;     // 43  bit0 AUTO, bit1 MIN LUX aktywny, bit2 adaptacja ON
  uint32_t reserved;      // 44  zarezerwowane = 0
};

struct PageHeader {       // 64 B
  uint32_t magic;
  uint16_t formatVersion;
  uint16_t headerBytes;
  uint64_t pageSeq;
  uint64_t firstSampleSeq;
  uint16_t recordBytes;
  uint16_t capacity;
  uint32_t flags;
  uint32_t firstUnixTs;
  uint32_t firstMonoMs;
  uint8_t  reserved[20];
  uint32_t headerCrc32;
};

struct PageFooter {       // 64 B — commit marker
  uint32_t magic;
  uint16_t formatVersion;
  uint16_t footerBytes;
  uint64_t pageSeq;
  uint64_t firstSampleSeq;
  uint64_t lastSampleSeq;
  uint32_t recordCount;
  uint32_t payloadCrc32;
  uint32_t commitMarker;
  uint8_t  reserved[16];
  uint32_t footerCrc32;
};
#pragma pack(pop)

static constexpr size_t SAMPLE_BYTES      = sizeof(Sample);       // 48
static constexpr size_t RECORD_BYTES      = sizeof(Sample) + 4U;  // 52
static constexpr size_t HEADER_BYTES      = 64U;
static constexpr size_t FOOTER_BYTES      = 64U;
static constexpr size_t PAGE_META_BYTES   = HEADER_BYTES + FOOTER_BYTES;

static_assert(sizeof(Sample) == 48, "tspool::Sample ABI changed: expected 48 B");
static_assert(sizeof(PageHeader) == HEADER_BYTES, "tspool::PageHeader ABI changed: expected 64 B");
static_assert(sizeof(PageFooter) == FOOTER_BYTES, "tspool::PageFooter ABI changed: expected 64 B");
static_assert(__builtin_offsetof(PageHeader, headerCrc32) == 60U, "PageHeader CRC offset changed");
static_assert(__builtin_offsetof(PageFooter, payloadCrc32) == 36U, "PageFooter payload CRC offset changed");
static_assert(__builtin_offsetof(PageFooter, commitMarker) == 40U, "PageFooter commit offset changed");
static_assert(__builtin_offsetof(PageFooter, footerCrc32) == 60U, "PageFooter CRC offset changed");

// ── CRC32 (IEEE, zgodny z Centralą) ─────────────────────────────────
uint32_t crc32(const uint8_t* data, size_t len);

// ── TelemetryRing: SPSC w PSRAM ─────────────────────────────────────
class Ring {
public:
  Ring() = default;
  ~Ring();
  Ring(const Ring&) = delete;
  Ring& operator=(const Ring&) = delete;

  bool init();                         // alokacja w PSRAM (raz)
  bool ready() const;
  bool push(const Sample& s);          // false = pełny (wtedy caller liczy drop)
  bool peekAt(uint32_t offset, Sample& out) const;
  bool commitPopN(uint32_t count);     // przesuwa tail dopiero po trwałym zapisie
  uint32_t size() const;
  uint32_t capacity() const { return RING_CAPACITY; }
  size_t bytes() const { return sizeof(Sample) * RING_CAPACITY; }
  void reset();                        // tylko do testów / po błędzie alokacji

private:
  Sample* m_buffer = nullptr;
  volatile uint32_t m_head = 0;
  volatile uint32_t m_tail = 0;
};

// ── Identyfikacja sesji ─────────────────────────────────────────────
// Losuje sessionNonce raz (esp_random). Zwraca 0 jeśli nie udało się.
uint64_t makeSessionNonce();

// ── Klucz i JSON rekordu (RTDB) ─────────────────────────────────────
// Klucz: "<nonce hex 16>_<sampleSeq>" — unikalny między restartami.
// Zwraca liczbę zapisanych znaków (bez '\0') albo 0 przy błędzie/braku miejsca.
size_t sampleKey(const Sample& s, char* out, size_t cap);
size_t sampleToJsonMember(const Sample& s, char* out, size_t cap);   // "key":{...}

// ── Strony na LittleFS ──────────────────────────────────────────────
// Ścieżki: /spool/p<pageSeq 10 cyfr>.spl (final), .part (w toku zapisu).
bool pagePaths(uint64_t pageSeq, char* finalOut, size_t finalCap,
               char* partOut, size_t partCap);

struct ScanResult {
  bool     headerOk = false;
  bool     footerOk = false;     // footer obecny, CRC OK, commit OK
  bool     payloadOk = false;    // CRC całego payloadu zgadza się
  uint32_t recordCount = 0;      // z footera
  uint32_t badRecords = 0;       // rekordy z błędnym CRC
  uint64_t pageSeq = 0;
  uint64_t firstSampleSeq = 0;
  uint64_t lastSampleSeq = 0;
};

// Pełna weryfikacja strony (header + footer + payload + CRC każdego rekordu).
bool scanPage(const char* path, ScanResult& out);

// Zapis strony z ringu: pierwsze `count` rekordów (od tail). Zapis do .part,
// weryfikacja scanPage(), dopiero potem rename → final. Nie rusza tail ringu —
// caller wywołuje ring.commitPopN(count) po sukcesie.
bool writePageFromRing(const Ring& ring, uint32_t count, uint64_t pageSeq,
                       uint64_t& firstSeqOut, uint64_t& lastSeqOut,
                       char* whyOut, size_t whyCap);

// Odczyt jednego rekordu z pozycji idx; false = brak lub CRC niezgodne.
bool readRecord(const char* path, uint32_t idx, Sample& out, bool& crcBad);

// Najstarsza finalna strona (najmniejszy pageSeq) w /spool.
bool findOldestPage(char* pathOut, size_t pathCap, uint64_t& pageSeqOut,
                    uint32_t& pagesOut, size_t& bytesOut);

// Największy pageSeq widoczny na FS (do wyznaczenia kolejnego po restarcie).
uint64_t maxPageSeqOnFs();

// Usuwa pozostałości przerwanego zapisu (*.part). Zwraca ich liczbę.
uint32_t discardPartialPages();

// Kasuje konkretną stronę (final + ewentualny .part). true = usunięto/nie było.
bool removePageSeq(uint64_t pageSeq);

// Kasuje najstarszą stronę (polityka quota / brak miejsca). Zwraca jej pageSeq.
bool removeOldestPage(uint64_t& removedSeq);

// ── Polityka spill (czysta logika, testowalna) ──────────────────────
struct SpillInputs {
  uint32_t ringSize;
  bool     transportUp;          // WiFi + Firebase gotowe
  uint32_t offlineSinceMs;       // 0 = online
  uint32_t nowMs;
  uint32_t lastSpillMs;
  bool     fsFreeOk;             // LittleFS ma >= FS_MIN_FREE_BYTES
};
// true = zrób spill teraz. `count` = ile rekordów zapisać na stronę.
bool spillDue(const SpillInputs& in, uint32_t& countOut);

// ── Replay (wysyłka FIFO, idempotentna) ─────────────────────────────
struct ReplayCursor {
  char     path[48];
  uint64_t pageSeq;
  uint32_t recordCount;
  uint32_t cursor;              // ile rekordów strony już potwierdzono
  uint32_t corruptSkipped;      // rekordy z błędnym CRC pominięte (licznik)
  bool     active;
};

// Otwiera najstarszą POPRAWNĄ stronę do replayu. Strony z uszkodzonym
// nagłówkiem/footerem są kasowane (kwarantanna) i liczone w badPagesRemoved.
// false = brak stron do wysłania.
bool replayOpenOldest(ReplayCursor& rc, uint32_t& badPagesRemoved);

// Buduje batch JSON dla kolejnych rekordów strony, zaczynając od rc.cursor.
// `taken` = liczba rekordów objętych batchem (wliczając pominięte uszkodzone).
// Zwraca:
//    1 = JSON gotowy do wysłania (ACK → replayAck(rc, taken)),
//    2 = same uszkodzone rekordy: nic do wysłania, wystarczy replayAck(rc, taken),
//    0 = strona wyczerpana (cursor == recordCount) → caller kasuje stronę,
//   -1 = błąd I/O lub bufor za mały (ponów później, bez przesuwania kursora).
int replayBuildBatch(ReplayCursor& rc, char* json, size_t jsonCap, uint32_t& taken);

// Po ACK (lub po pominięciu samych uszkodzonych): przesuwa kursor.
void replayAck(ReplayCursor& rc, uint32_t taken);

// Zlicza stan spool na FS (strony + bajty) — do /api/telemetry/status.
void spoolFsStats(uint32_t& pages, size_t& bytes);

}  // namespace tspool
