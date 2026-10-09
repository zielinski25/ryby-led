/*************************************************************
 *  CHANGELOG
 *
 * [v4.1.0 OTA-GITHUB, 2026-10-09] Etap 1 planu upgrade: OTA przez GitHub Releases.
 *     Co: nowy, samodzielny modul src/ota_github.h/.cpp (port 1:1 sprawdzonego
 *     mechanizmu z Centrali Pieca): GET /api/ota-status, POST /api/ota-start,
 *     komenda /update w Telegramie, komendy update/ota w Firebase
 *     (/aquarium/commands) i konsoli WS (ota / ota status). Task OTA na Core 0,
 *     stos 16KB w DRAM, karmienie TWDT, anty-rollback (potwierdzenie partycji
 *     w setup), porownanie semver (downgrade tylko z force=1), restart przez
 *     istniejacy mechanizm restartRequestedAt.
 *     Dlaczego: dotad jedyne OTA = espota z haslem z laptopa (brak zdalnych
 *     aktualizacji); parytet z Centrala Pieca (karta OTA + /update).
 *     Logika sterowania LED, adaptacji, harmonogramu, MIN LUX, ramp i Ramp
 *     Arbiter: NIETKNIETA. Firebase transport, Telegram poll i logi: NIETKNIETE.
 *
 * [v4.0.0 REPO-RESTRUCTURE, 2026-10-09] Semver baseline (v261 = 4.0.0+build.261).
 *     Naglowek skrocony do ostatnich 10 wersji — pelna historia (do v33b)
 *     przeniesiona 1:1 do CHANGELOG.md w katalogu glownym repo.
 *     Bez zmian logiki sterowania — zmiana wylacznie organizacyjna + wersja.
 *
 * v261 (2026-09-27) - CONFIG QUEUE BACKPRESSURE.
 *      Gdy fbConfigApplyQueue jest pełna, snapshot nie powoduje już
 *      kolejnych GET /aquarium/config. Core 0 przechowuje immutable
 *      snapshot i ponawia enqueue z backoffem, zachowując config_refresh
 *      pending do chwili przejęcia snapshotu przez Core 1.
 * v259 (2026-09-27) - FIREBASE LEGACY PATH REMOVAL.
 *      Trzy stare, blokujące implementacje sendStatusToFirebase(),
 *      checkFirebaseCommands() i checkFirebaseConfig() zostały zastąpione
 *      lekkimi wrapperami ustawiającymi pending flagi. Aktywny transport
 *      Firebase pozostaje wyłącznie w native async schedulerze na Core 0.
 * v258 (2026-09-27) - CONFIG DURABLE ACK / TWO-PHASE APPLY.
 *      v257 potwierdzał zastosowanie runtime zanim Core 0 zakończył trwały zapis.
 *      W v258 cały immutable FirebaseConfigSnapshot trafia do osobnej
 *      fbConfigPersistQueue. Core 0 wykonuje komplet EEPROM.commit() + zapis
 *      LittleFS, retry przy błędzie, i dopiero wtedy wysyła PERSIST_ACK.
 *      Core 1 aplikuje runtime dopiero po PERSIST_ACK; dopiero po kolejnym
 *      FirebaseConfigAck Core 0 zapisuje cfgTs i uzbraja DELETE config_refresh.
 *      Dzięki temu restart/awaria storage nie może potwierdzić konfiguracji
 *      Firebase jako wykonanej przed trwałą persystencją.
 * v257 (2026-09-27) - CONFIG RUNTIME/STORAGE SPLIT.
 *      Firebase config_refresh jest parsowany na Core 0 do immutable snapshotu,
 *      snapshot trafia przez queue do Core 1, a potwierdzenie zastosowania wraca
 *      przez ACK. Dopiero po ACK Core 0 zapisuje cfgTs i usuwa config_refresh.
 *      Runtime EEPROM/LittleFS nie wykonuje już bezpośrednich commitów: wartości
 *      są kolejkowane do storageQueue, a zapis/commit realizuje wyłącznie Core 0.
 *      Dodano retry nieudanego EEPROM.commit() i jawne logowanie przepełnienia
 *      kolejek. onPowerChange()/onTrybChange() korzystają wyłącznie z RAM.
 * v252 (2026-09-26) - RAMP-ARBITER: jeden właściciel zasobu PWM.
 *      Dodano Ramp Arbiter z wykrywaniem właściciela, blokadą startu automatycznych
 *      ramp oraz kontrolowanym wywłaszczeniem dla poleceń użytkownika. Arbiter
 *      rozwiązuje kolizję soft-start/adaptacja/harmonogram/transition i loguje
 *      BLOCK/PREEMPT/COLLISION_RESOLVED. Zachowane zostały istniejące mechanizmy
 *      ramp, ale nie mogą już równolegle prowadzić backupBrightnessComposite.
 *      W tej wersji nie zmieniono hardware/PWM API ani protokołu Firebase.
 * v246 (2026-09-26) - FIREBASE-TURBO-QUEUE-ONLY: tylko /aquarium/commands.
 *      Polling komend/kolejki pozostaje co 1 s, ale status Firebase jest
 *      wysyłany co 10 s zamiast co 1 s. Podczas Turbo pomijany jest legacy
 *      fallback /aquarium/cmd, ponieważ panel v15 używa /aquarium/commands.
 *      Dzięki temu jedna iteracja nie wykonuje już dwóch GET-ów komendy
 *      przy 1-sekundowym Turbo. Celem jest ograniczenie blokad Firebase,
 *      timeoutów i wtórnych rozłączeń terminala LAN.
 * v244 (2026-09-26) - FIX-FB-STATE-CALLBACK-RACE: Firebase Core 0 no longer
 *      executes onPowerChange()/onTrybChange() directly. It only changes the
 *      requested state; the existing Core 1 debug/change detector in loop()
 *      applies the callback exactly once. This removes the observed race where
 *      Core 1 saw the new `tryb/power` before Core 0 synchronized prevTrybGlobal,
 *      causing duplicate WYWOLANIE/ZAPIS/EEPROM/COUNTER/PRZEJSCIE side effects.
 *      Firebase path no longer performs duplicate EEPROM/updateLEDs work for
 *      power changes either.
 * v243 (2026-09-26) - FIREBASE-COMMAND-QUEUE: robust first-key queue + config_refresh + 64-bit ts.
 * v240 (2026-09-26) - CRASH-ARCHIVE: domkniecie lancucha crash -> Telegram.
 *      Dotychczas po restarcie z PANIC/WDT kod tylko czytal summary coredumpu
 *      i kasowal zrodlo przez esp_core_dump_image_erase(), przez co surowy plik
 *      nie nadawal sie do pozniejszego wyslania. Dodano archiwizacje raw obrazu
 *      do LittleFS (/coredumps), trwaly seq w NVS, retencje 20 plikow oraz
 *      automatyczna kolejke wysylki do Telegrama. Wysylka dziala po starcie
 *      taska Telegram i retryuje po awarii sieci. Coredump jest kasowany ze
 *      zrodla dopiero po poprawnym zweryfikowaniu pliku archiwum.
 *
 *      Dodatkowo poprawiono Firebase Turbo: okno 60 s nadal jest odswiezane
 *      tylko przez prawidlowe, nowe komendy po walidacji tokenu i identyfikatora
 *      polecenia; sam uszkodzony payload nie aktywuje juz kosztownego turbo.
 *      Turbo pozostaje 1s komendy / 10s status, a po minucie ciszy wraca do wartosci normalnych.
 *
 * v239 (2026-09-05) - FIX-SNTP-FB-RACE: crash zdekodowany przez addr2line
 *      (użytkownik przesłał prawdziwy firmware.elf, potwierdzony jako ten
 *      sam v238 co crashował) na realnym teście "odłączony ruter":
 *      setup() -> fbInitialize() [17052] -> WiFi.hostByName() -> lwIP DNS ->
 *      assert() w udp_new_ip_type (udp.c:1278). W tym samym momencie SNTP
 *      (uruchomione configTzTime(), ~188 linii wcześniej) samo w tle próbuje
 *      kolejnego serwera NTP (sntp_try_next_server->sntp_dns_found) - dwa
 *      niezależne wywołania DNS w nie-reentrantnym module lwIP zderzają się.
 *      Nasila się przy złej sieci (ruter odłączony) - własna pętla czekania
 *      na NTP w setup() (10x1s) i tak się kończy, ale NIE zatrzymuje klienta
 *      SNTP, który dalej ponawia w tle, częściej niż przy dobrej sieci.
 *      TWARDE OGRANICZENIE (już z v228): configTzTime()/sntp_stop() wołane
 *      >1x w programie to osobna, znana przyczyna TEGO SAMEGO assertu
 *      (arduino-esp32 #10675) - więc nie wolno "zatrzymać SNTP na chwilę".
 *      Fix: 3s przerwa osadzająca (WDT-safe, po 100ms) między końcem sekcji
 *      NTP a pierwszym fbInitialize() - zmniejsza (NIE gwarantuje ze 100%
 *      pewnością, patrz Ryby_LED_S3_plan_fix_sntp_race.md) prawdopodobieństwo
 *      zazębienia w czasie. Prawdziwy pewny fix wymagałby łatki
 *      lwIP/arduino-esp32 (poza zasięgiem tego projektu).
 *      Backtrace WDT_TASK z tego samego logu (checkpoint=L:wifiCheck+sensors,
 *      uptime=12s) prawdopodobnie ta sama rasa, tylko bez trafienia w assert -
 *      niepotwierdzone z pewnością, ale spójne z hipotezą.
 *
 *
 * === STARSZE WERSJE (do v238 wlacznie): patrz CHANGELOG.md (katalog glowny) ===
 *
 *************************************************************/

/*************************************************************
 *  INCLUDES
 *************************************************************/
#include <Arduino.h>  // [AUTO] added for .cpp compilation
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <atomic>  // [v235] FIX-LOG-SEQ-ATOMIC: std::atomic dla g_logSeq (cross-core increment)

// ═══════════════════════════════════════════════════════════
// [v141] WERSJA FIRMWARE - jedno miejsce do zmiany przy każdym release'ie.
// Wyświetlana na Dashboardzie (panel WWW) oraz w /api/status, żeby zawsze
// było widać, jaka wersja jest faktycznie wgrana na płytce.
// ═══════════════════════════════════════════════════════════
#define RYBY_FW_VERSION "v4.1.0+build.262"
#define FW_VERSION RYBY_FW_VERSION

// ═══════════════════════════════════════════════════════════
// Forward declarations (auto-generated for .cpp compilation)
// ═══════════════════════════════════════════════════════════
class AsyncWebSocketClient;  // forward-declare before use in handleWsCmd
static void* mbedtls_psram_calloc(size_t n, size_t size);
static void mbedtls_psram_free(void* ptr);
float getPowerForChannel(int ch, uint16_t pwm10bit);
float getTotalLEDPower(const uint16_t pwm[5]);
static inline float capTargetsToPowerLimit(uint16_t targets[5]);
String logTime();
void loadTelegramConfig();
void saveTelegramConfig();
void loadWifiNetworks();
bool saveWifiNetworks();
bool wifiAddNetwork(const String& ssid, const String& pass);
bool wifiDeleteNetwork(const String& ssid);
int  wifiPickBestNetwork();
void wifiBeginBest(bool allowScan = true);
String wifiNetworksListJson();
bool tgEnsureConnected();
String tgPost(const String& path, const String& body);
void deleteTelegramMessage(long msgId);
long parseTelegramMsgId(const String& body);
void tgPushMsgId(long id);
void tgDeleteAllHistory();
bool sendTelegramMessage(const String& text, bool trackMsgId);
bool sendTelegramDocument();
String buildTelegramTempReport();
String buildTelegramEnergyReport();
String buildTelegramLightReport();
String buildTelegramAdaptReport();
String buildTelegramSensorHistory();
String buildTelegramScheduleReport();
bool sendTelegramConfirmClear();
bool sendTelegramConfirmRestart();
bool sendTelegramInlineMenu();
void answerTelegramCallback(const String& callbackId);
String buildTelegramStatusReport();
void pollTelegramCommands();
void logDailySummary();
void saveAutoBrightnessToEEPROM();
void loadAutoBrightnessFromEEPROM();
void loadBackupBrightnessFromEEPROM();
void loadFadeFromEEPROM();
void saveFadeToEEPROM();
void saveEmaFilterToEEPROM();
void loadEmaFilterFromEEPROM();
void saveSensIntToEEPROM();
void loadSensIntFromEEPROM();
void saveRampSecToEEPROM();
void loadRampSecFromEEPROM();
void saveScheduleToEEPROM();
void savePumpScheduleToEEPROM();
void loadPumpScheduleFromEEPROM();
void loadScheduleFromEEPROM();
void saveMinLuxModeToEEPROM();
void saveKwhPrice();
void loadKwhPrice();
void loadMinLuxModeFromEEPROM();
uint16_t calculateMinLuxPWM();
void applyMinLuxMode();
uint16_t getBrightnessForSection(uint64_t composite, int section);
void setBrightnessForSection(uint64_t &composite, int section, uint16_t value);
bool isDaylightSaving(time_t now);
bool getLocalTimePL(struct tm *ti);
uint16_t mapSliderTo10bit(int sliderValue);
void updateLEDs();
void updatePump();
bool isWeekend();
int getLocalMinutes();
void onSectionsChange();
void onBrightnessCompositeChange();
void onTrybChange();
float szacujTransmisje(float luxPokojowy);
float pwmToLux(uint16_t pwm);
uint16_t luxToPwm(float lux);
void odczytajSwiatloZFiltrem();
void aktualizujUczenie();
void uczSieTransmisji();
uint16_t obliczAdaptacyjnaJasnosc(uint16_t harmonogramPWM);
bool zastosujRampeAdaptacyjna(uint16_t nowiePWM[5], const char* caller);
void aktualizujRampeAdaptacyjna();
void zapiszAdaptacjeDoEEPROM();
void odczytajAdaptacjeZEEPROM();
void inteligentyZapisAdaptacji();
void logToFile(const String &message);
static void _wsEnqueue(const String &msg);
void flushWsQueue();
void logPrint(const String &msg);
void logFlushCore1();
void logForceFlush();
void logPrintln(const String &msg);
void logPrint(const char* msg);
void logPrintln(const char* msg);
void logPrintf(const char* format, ...);
void logPrintfNoFile(const char* format, ...);  // [v230]
String getLogModeString();
void saveLogModeToEEPROM();
void loadLogModeFromEEPROM();
void savePowerTrybToEEPROM();
void saveBackupBrightnessToEEPROM();
void bumpChangeCounter();
void loadChangeCounterFromEEPROM();
void loadPowerTrybFromEEPROM();
void saveAdaptiveSettingsToEEPROM();
void loadAdaptiveSettingsFromEEPROM();
void handleWsCmd(const String& rawCmd, AsyncWebSocketClient* client);
void tgTaskFn(void* /*pvParams*/);
void* psramAllocSafe(size_t size);
void setup();
// [v222] FIX-OTA-NEVER-STARTED: patrz komentarz przy definicji (~przed setup()).
void startOtaIfNeeded();
void saveEnergyStats();
void applyEnergyRollover();  // [FIX race-audit 2026-08-13] single-writer dla akumulatorów energii, wołane z tgTaskFn (Core 0)
static float parseJsonFloat(const char* s, const char* key);  // [v88] PATCH-2: const char* zamiast String& — zero heap alloc
static int parseJsonInt(const char* s, const char* key);      // [v88] PATCH-2: const char* zamiast String& — zero heap alloc
void loadEnergyStats();
void saveAdaptStats();
void loadAdaptStats();
void saveHistoryPoint();
// [v105-FC] bool fbConnect() USUNIĘTY — zastąpiony przez void fbInitialize()
void fbInitialize();
void sendStatusToFirebase();
void checkFirebaseCommands();
void checkFirebaseConfig();
static bool wykonajKomendeFirebase(const String& cmd);
static bool fbParseU64Field(const String& body, int valueIndex, uint64_t& out);
void loop();
void trybAuto(const char* caller);
void readTemperatures();
float calculateLinearDerating(float currentTemp);
void multiZoneLinearDerating();
void emergencyThermalShutdown();
int obliczZachodSlonca(int rok, int miesiac, int dzien, float szerokosc, float dlugosc);
uint16_t gammaCorrect(uint16_t value10bit, float gamma);
void trybManual();
void wlaczKamera();
void wylaczKamera();
void onPowerChange();
void runDiagnostics();
void diagTslFrozenCheck();
void diagManualTransitionStuck();
void diagHeap();
void diagAutoBackupZero();
void diagSunsetRange();
void diagMinLuxOscillation();
void diagNtpTimeout();
void diagPwmRange();
void diagCameraTimeout();
void diagScheduleSanity();
void diagHealthReport();
void diagDeratingDuration();
void diagWaterTemp();
void diagTransmissionSanity();
void diagSensorSwapped();
void diagLearningStuck();
void fsSizeGuard();

// [v240] Crash archive + automatyczna wysyłka coredumpów do Telegrama.
// Definicje są umieszczone przed pierwszym użyciem, ponieważ preprocessor
// widzi makra tylko od miejsca ich definicji w dół pliku.
#define COREDUMP_DIR                 "/coredumps"
#define COREDUMP_MAX_FILES           20U
#define COREDUMP_PATH_MAX_LEN        96U
#define COREDUMP_NVS_NAMESPACE       "coredump"
#define COREDUMP_NVS_KEY_SEQ         "seq"
#define COREDUMP_NVS_KEY_TEXT_SEQ    "text_seq"
#define COREDUMP_NVS_KEY_FILE_SEQ    "file_seq"
#define COREDUMP_NVS_KEY_BOOT        "boot"
#define COREDUMP_COPY_CHUNK          1024U

void archiwizujCoredumpJesliCrash(esp_reset_reason_t powodResetu);
void wyslijOczekujaceCoredumpyTelegram();
static uint32_t coredumpNvsGet(const char* key, uint32_t def = 0);
static bool coredumpNvsPut(const char* key, uint32_t value);
static uint32_t coredumpAlokujBootId();
static bool coredumpParsujNazwe(const String& pelnaNazwa, uint32_t& seq, uint32_t& boot);
static uint32_t coredumpNajwyzszeSeqZFS();
static uint32_t coredumpAlokujSeq();
static uint16_t coredumpPoliczPliki();
static bool coredumpZnajdzNajstarszy(bool tylkoNiewyslany, String& path, uint32_t& seq, uint32_t& boot, size_t& size);
static void coredumpPrzytnijArchiwum();
static String coredumpCrashMessage(uint32_t boot, uint32_t seq, size_t size);
static bool sendTelegramCoredumpPlik(const String& path, uint32_t boot, uint32_t seq, size_t fileSize);
// ═══════════════════════════════════════════════════════════

// [v105-FC] #include <HTTPClient.h> USUNIĘTY — zastąpiony przez FirebaseClient.
// FirebaseClient używa wewnętrznie ESP_SSLClient (nie WiFiClientSecure) —
// prawidłowy connected() check eliminuje root cause WDT crashy z v99–v104.
// WYMAGANE w platformio.ini:  lib_deps += mobizt/FirebaseClient @ ^2.x.x
// [v105-FC-FIX] FirebaseClient v2.x wymaga jawnych define przed include —
// bez nich RealtimeDatabase i LegacyToken są ukryte za #ifdef i nie kompilują się.
#define ENABLE_DATABASE       // eksponuje klasę RealtimeDatabase
#define ENABLE_LEGACY_TOKEN   // eksponuje klasę LegacyToken (Database Secret auth)
// [v123-PSRAM] ENABLE_PSRAM — udokumentowana opcja biblioteki (sekcja "Memory Options
// for ESP32" / "Library Build Options" w README mobizt/FirebaseClient). Dla ESP32
// (w odróżnieniu od ESP8266) NIE jest włączona domyślnie — wymaga jawnego #define
// przed #include FirebaseClient.h. BOARD_HAS_PSRAM jest już potwierdzone aktywne
// w tym projekcie (patrz #ifdef BOARD_HAS_PSRAM w compactHistPending, ~linia 8630),
// więc warunek wstępny biblioteki jest spełniony. Patrz CHANGELOG v123.
#define ENABLE_PSRAM
#include <FirebaseClient.h>
// ─── FIREBASE-v1 ─────────────────────────────────────────────────────────────
#define FIREBASE_HOST   "akwarium-367be-default-rtdb.europe-west1.firebasedatabase.app"
#define FIREBASE_SECRET "RhKVp49qYKKpnBl4qdu49x3uNKoIpR3kDMQUmVxt"   // wklej z Firebase Console -> Project Settings -> Service accounts -> Database secrets
#define CMD_TOKEN       "Akwarium2026!"           // ten sam token wpisz w panelu HTML
// ─────────────────────────────────────────────────────────────────────────────
#include <WiFiClientSecure.h>
// [v33] arduino_secrets.h usunięty - Arduino IoT Cloud usunięty
// [v33] thingProperties.h usunięty - Arduino IoT Cloud usunięty

/*************************************************************
 *  WIFI CREDENTIALS (zastępują arduino_secrets.h)
 *  [WARN]️  ZMIEŃ na swoje dane WiFi przed wgraniem!
 *************************************************************/
#define SECRET_SSID          "Pytka"
#define SECRET_OPTIONAL_PASS "42172535"

/*************************************************************
 *  [v139] FEATURE-WIFI-MULTI: lista wielu sieci WiFi
 *  Zamiast łączenia WYŁĄCZNIE z SECRET_SSID, urządzenie trzyma listę
 *  znanych sieci w LittleFS i przy każdym połączeniu wybiera tę o
 *  najsilniejszym sygnale spośród faktycznie widocznych w skanie.
 *  Format pliku /wifi_list.txt: jeden JSON-obiekt na linię, np.
 *    {"ssid":"Pytka","pass":"42172535"}
 *    {"ssid":"TelefonHotspot","pass":"12345678"}
 *  (ten sam ręczny styl parsowania co loadTelegramConfig(), bez
 *  dodatkowej zależności od ArduinoJson dla parsowania).
 *************************************************************/
#define WIFI_LIST_FILE     "/wifi_list.txt"
#define WIFI_LIST_TMP_FILE "/wifi_list_tmp.txt"   // [v140] plik roboczy przy zapisie atomowym
#define WIFI_LIST_OLD_FILE "/wifi_list_old.txt"    // [v140] backup podczas atomowej podmiany
#define WIFI_MAX_NETWORKS 8   // rozsądny limit - LittleFS + RAM

struct WifiNet {
  String ssid;
  String pass;
};

std::vector<WifiNet> wifiNetworks;   // lista sieci wczytana do RAM przy starcie

/*************************************************************
 *  ZMIENNE CLOUD (zastępują thingProperties.h)
 *  Były generowane automatycznie przez Arduino IoT Cloud.
 *  Teraz zwykłe zmienne globalne - logika onChange() wywołana
 *  ręcznie z loop() gdy wartość się zmieni.
 *************************************************************/
bool     power              = false;   // zasilanie LED
bool     tryb               = false;   // false=MANUAL, true=AUTO
int      sections            = 31;     // maska wybranych sekcji (31 = wszystkie 5)
int      brightnessComposite = 0;      // wartość suwaka jasności (slider); w Cloud był int (0-100)
// [v33] float temperatura usunięty - był używany wyłącznie przez Arduino IoT Cloud (READ only)

// Forward declarations funkcji onChange (definicje poniżej w kodzie)
void onPowerChange();
void onTrybChange();
void onSectionsChange();
void onBrightnessCompositeChange();
#include <EEPROM.h>
#include <vector>               // [v139] FEATURE-WIFI-MULTI: lista sieci WiFi (wifiNetworks)
#include <WiFi.h>
#include <ArduinoOTA.h>         // [v98-OTA] espota support (ArduinoOTA.begin/handle)
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <esp_task_wdt.h>
#include <esp_pm.h>
#include "esp_core_dump.h"      // [v171] FEATURE-COREDUMP-SUMMARY: esp_core_dump_get_summary() itp.
#include <esp_flash.h>        // [v240] RAW-COREDUMP: API flash dla diagnostyki
#include <esp_partition.h>   // [v240] RAW-COREDUMP: bezpieczny odczyt przez API partycji
#include <Preferences.h>     // [v240] RAW-COREDUMP: trwały licznik seq + potwierdzenie wysyłki
                                // Nagłówek sam robi #include "sdkconfig.h" i osłania deklaracje
                                // esp_core_dump_summary_t/get_summary/image_check/image_erase
                                // za #if CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH — jeśli w tym
                                // konkretnym buildzie ta flaga jest =0, kod niżej i tak
                                // się nie skompiluje w środku własnego #if (patrz FEATURE blok).
// #include <esp_psram.h>          // ESP.getPsramSize() / psramFound()
#include "mbedtls/platform.h"  // [v86] FIX-PSRAM-TLS-D: mbedtls_platform_set_calloc_free()
#include <sys/socket.h>         // [v82] FIX-WDT-SNDTIMEO: SO_SNDTIMEO dla WiFiClientSecure
#include <netinet/tcp.h>        // [v83] FIX-TCP-KA: TCP_KEEPIDLE / TCP_KEEPINTVL / TCP_KEEPCNT
#include <sys/select.h>         // [v150] FIX-WDT-WRITEGUARD: fd_set/select() dla tgWriteGuard()
// [v93] #include <lwip/tcpip.h> USUNIĘTY — nie potrzebny po przejściu na mathieucarbou/AsyncTCP (obsługuje locking wewnętrznie)

// Forward declaration – definicja w dalszej części pliku (po setup/loop)
void* psramAllocSafe(size_t size);
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
// ===== WEBSERIAL - Logi przez WiFi =====
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <memory>     // [v106] shared_ptr dla chunked response (log endpoints)
#include <algorithm>  // [v106] std::min w callbackach beginResponse
#include "NetDiag.h"  // [NETDIAG] test routera/łącza - patrz komentarz w pliku


/*************************************************************
 *  EEPROM
 *************************************************************/
#define EEPROM_ADDR_BACKUP_AUTO_BRIGHTNESS 0
#define EEPROM_SIZE 148  // [OK] ZMIENIONE: +2 bajty SENS_INT (142-143) +2 bajty RAMP_SEC (144-145)
#define EEPROM_ADDR_SENS_INT  142  // 2 bajty uint16_t - interwał odczytu czujnika [s]
#define EEPROM_ADDR_RAMP_SEC  144  // 2 bajty uint16_t - czas rampy PWM [s]
#define EEPROM_ADDR_MINLUX_INTERVAL 146  // 2 bajty uint16_t - interwał sprawdzania MIN LUX [s]
#define EEPROM_ADDR_BACKUP_BRIGHTNESS 148 // 8 bajtów uint64_t - backupBrightnessComposite (FIX-v70-B)
#define EEPROM_ADDR_CHANGE_COUNTER 134  // 4 bajty uint32_t - licznik wersji nastaw
#define EEPROM_ADDR_EMA_FILTER     138  // 4 bajty float - współczynnik filtra EMA czujnika światła
#define EEPROM_ADDR_FADE_MIN 8   // po uint64_t (8 bajtów)
#define EEPROM_ADRES_ADAPT_SREDNIA 44
#define EEPROM_ADRES_ADAPT_PROBKI 48
#define EEPROM_ADRES_ADAPT_TRANSMISJA 52
#define EEPROM_ADRES_ADAPT_POPRAWNE 56
#define EEPROM_ADRES_OSTATNI_ZAPIS 57  // [WARN]️ ZAREZERWOWANE - 8 bajtów (57-64), niezaimplementowane. NIE używać dla innych danych.
#define EEPROM_ADDR_POWER  65  // 1 bajt
#define EEPROM_ADDR_TRYB   66  // 1 bajt

// [OK] NOWE - Harmonogram (dodane wcześniej)
#define EEPROM_ADDR_MORNING_WEEKDAY 12
#define EEPROM_ADDR_MORNING_WEEKEND 16
#define EEPROM_ADDR_MIDDAY_OFF 20
#define EEPROM_ADDR_EVENING_BEFORE 24
#define EEPROM_ADDR_EVENING_OFF 28

// [OK] NOWE - Terminal WiFi & Czujniki
#define EEPROM_ADDR_LOG_MODE 70           // Tryb logowania (1 bajt)
#define EEPROM_ADDR_ADAPTIVE_MODE 71      // Tryb czujników 0/1/2 (1 bajt)
#define EEPROM_ADDR_ADAPTIVE_ENABLED 72   // Regulacja ON/OFF (1 bajt)
#define EEPROM_ADDR_LEARNING_ENABLED 73   // Uczenie się ON/OFF (1 bajt)

/*************************************************************
 *  CZAS / STREFA CZASOWA / NTP
 *************************************************************/
// Polska: lato UTC+2 (120), zima UTC+1 (60)
// U Ciebie DST obsługiwany logicznie w kodzie
const int timezoneOffsetMinutes = 120;

// Synchronizacja NTP obsługiwana przez recalcInterval w pętli głównej
static unsigned long lastNtpSync = 0;

/*************************************************************
 *  WEBSERIAL - SERWER I KONFIGURACJA
 *************************************************************/
AsyncWebServer webserialServer(8080);
AsyncWebSocket wsTerminal("/ws");
bool wsReady = false;

// ═══ SYSTEM WYBORU LOGÓW ═══
enum LogDestination {
  LOG_SERIAL_ONLY = 0,   // Tylko USB Serial Monitor
  LOG_WIFI_ONLY = 1,     // Tylko WiFi Terminal
  LOG_BOTH = 2,          // Serial + WiFi Terminal
  LOG_OFF = 3            // Wyłączone (oszczędność CPU)
};

LogDestination logMode = LOG_BOTH;  // Domyślnie oba

// [v230] FIX-LOG-SEQ: globalny licznik linii zapisanych na flash. Pozwala wykryć
// utracone linie (np. bufor RAM nie zdążył się flushnąć przed restartem/utratą
// zasilania) — skok w seq= między kolejnymi liniami = tyle linii przepadło.
// Inkrementowany centralnie w logToFile(), nie w logPrintf() — bo to zapis na
// flash jest tym, co realnie może się urwać w połowie; Serial/WebSocket i tak
// są ulotne z definicji.
std::atomic<uint32_t> g_logSeq{0};
// [v235] FIX-LOG-SEQ-ATOMIC: było volatile uint32_t — g_logSeq++ nie jest atomowe
// między rdzeniami (read-modify-write, 3 kroki), a logToFile() jest wołane z obu
// rdzeni (każdy ma własny bufor PSRAM). Potwierdzone w praktyce (aquarium_log__10_.txt,
// pierwszy log na v231+): drobne nie-monotoniczne cofnięcia w seq= (np. 181->175) —
// artefakt gubionych/duplikowanych inkrementów, nie prawdziwa utrata linii (te dają
// duże skoki, nie ±kilka). std::atomic<uint32_t> na Xtensa kompiluje się do pętli
// CAS na sprzętowej instrukcji S32C1I — poprawnie atomowe, zero zmiany kosztu
// praktycznego (pojedyncza operacja przy każdej linii loga, i tak już tam jesteśmy).
// ═══════════════════════════════════════════════════
// LITTLEFS LOGGING
// ═══════════════════════════════════════════════════
// [v102] DUAL-FILE LOG: log_a.txt (archiwum) + log_b.txt (aktywny zapis)
#define LOG_FILE_B "/log_b.txt"  // aktywny — tu idą nowe logi
#define LOG_FILE_A "/log_a.txt"  // archiwum — starsze logi
#define LOG_B_MAX  262144UL      // 256 KB — po przekroczeniu fsSizeGuard rotuje
#define HISTORY_FILE "/history.csv"
#define HISTORY_MAX_POINTS 288   // 24h przy zapisie co 5 minut
#define LED_MAX_POWER_W    90    // Maksymalna moc zasilacza [W] - limit dla power-balancera
// [OK] CIRCULAR LOG - dynamiczny, bez stałych rozmiarów.
// Trigger: wolne miejsce < 20% LittleFS (niezależnie od rozmiarów innych plików).
// keepBytes obliczane live z fsFree - rotacja zawsze bezpieczna nawet przy dużym history.csv.

bool littlefsReady = false;
// [v152] FIX-FS-CAPACITY: prawdziwy rozmiar wolumenu LittleFS, cache'owany raz po mount.
// Fallback 10420224 to stara (błędna od v113) wartość - używana tylko zanim LittleFS.begin() zwróci realny rozmiar.
size_t g_fsTotalBytes = 10420224UL;
volatile bool historyClearPending = false;  // blokada zapisu podczas czyszczenia historii - [FIX race-audit 2026-08-13] dodano volatile: flaga ustawiana/czyszczona w handlerze /api/history/clear (3-ci kontekst wykonania, AsyncWebServer), czytana w saveHistoryPoint() na Core 0
int  histLineCount       = -1;     // [OK] FIX-v33f: cache liczby linii - unika skanowania pliku co 5 min
int  histStartLine       =  0;     // [v87] OPT-B: nr "martwych" linii na początku history.csv (offset rolling buffer)
uint32_t histStartByteOffset = 0;  // [v88] PATCH-1: byte offset do pierwszej żywej linii w CSV (0 = nie ustalony)
volatile bool compactHistPending = false;  // [v87] OPT-B: flaga kompaktowania (wzorzec fsGuardPending) — wykonuje tgTask
volatile bool histSavePending  = false;  // [OK] FIX-v33g: sygnał do tgTask (Core 0) - save historii poza loop()
volatile bool fbSendPending    = false;  // [v42] FIX-ARCH: Firebase status   -> Core 0 zamiast Core 1
volatile bool fbCmdPending     = false;  // [v42] FIX-ARCH: Firebase commands -> Core 0 zamiast Core 1
volatile bool fbCfgPending     = false;  // [v42] FIX-ARCH: Firebase config   -> Core 0 zamiast Core 1
volatile bool tslReinitPending = false;  // [OK] FIX-v33h: sygnał do tgTask (Core 0) - reinit I²C poza loop()
volatile bool fsGuardPending   = false;  // [OK] FIX-v39-4: sygnał do tgTask (Core 0) - rotacja FS poza loop()
volatile bool energyRolloverPending = false;  // [FIX race-audit 2026-08-13] sygnał do tgTask (Core 0) - dzienny/tygodniowy/miesięczny
                                               // reset akumulatorów energii (energyTodayWh i cała rodzina) poza loop().
                                               // logDailySummary() (Core 1) tylko wykrywa zmianę dnia i ustawia flagę;
                                               // samą mutację robi wyłącznie applyEnergyRollover() na Core 0 - ten sam
                                               // rdzeń co saveHistoryPoint() (histSavePending), single-writer, bez wyścigu.
volatile size_t g_fsGuardTriggerUsed = 0;  // [v223] DIAG-FSGUARD-DRIFT: LittleFS.usedBytes() zapamiętane w momencie
                                            // WYZWOLENIA (fsSizeGuard, Core 1) - do porównania z odczytem w tgTaskFn
                                            // (Core 0) przy faktycznym przetwarzaniu, żeby złapać dryf między tymi
                                            // dwoma momentami (patrz RYBY_LED_FS_GUARD_investigation_v223.md)
volatile bool logFlushPending[2] = {false, false};  // [v46] FIX-LOG-FLUSH: flush buforów logu na Core 0
volatile bool logClearPending   = false;             // [v72] FIX-LOG-CLEAR: usuwanie log.txt przez tgTask (Core 0) - unika EACCES gdy plik otwarty
volatile bool logClearInProgress = false;            // [v98] zablokuj HTTP readers podczas remove()
volatile bool logClearDone       = false;            // [v98] sygnał sukcesu dla JS polling
volatile bool logClearFailed     = false;            // [v98] sygnał błędu dla JS polling
volatile size_t g_logFileSize    = 0;                // [v102] cache rozmiaru log_b.txt (aktualizowany po każdym flush)
volatile size_t g_logFileSizeA   = 0;                // [v102] cache rozmiaru log_a.txt (aktualizowany po rotacji/clear)
unsigned long tslLastRetry     = 0;      // [OK] FIX-v33h: globalny (dostęp z tgTaskFn i odczytajSwiatloZFiltrem)
uint8_t       tslFailCount     = 0;      // [OK] FIX-v33h: globalny (jw.)
// ── FIX-v34-TSL-ASYNC: odczyt I²C zlecony do Core 0, loop() używa cache ──
volatile bool     tslReadPending   = false;  // sygnał: czas na odczyt czujników
volatile float    tslCachePok      = -1.0f;  // -1 = brak danych jeszcze
volatile float    tslCacheWoda     = -1.0f;
volatile uint16_t tslCacheBrPok    = 0;
volatile uint16_t tslCacheIrPok    = 0;
volatile uint16_t tslCacheBrWoda   = 0;
volatile uint16_t tslCacheIrWoda   = 0;
// [OK] OPT-v44: mutex dla spójności cache TSL między Core 0 (zapis) a Core 1 (odczyt).
// Na ESP32 pojedynczy float (4B, aligned) jest atomowy, ale odczyt kilku pól naraz
// (Pok + BrPok + IrPok) może trafić na częściowo zaktualizowany zestaw.
portMUX_TYPE tslCacheMux = portMUX_INITIALIZER_UNLOCKED;
// ── FIX-v34b-BOOT-NOTIFY: powiadomienie TG o resecie ──
// ══════════════════════════════════════════════════════
//  [v71] PRE-RESET BLACK BOX — przeżywa restart w RTC RAM
//  Nie kasowane przez soft-reset, zerowane przez power-on.
// ══════════════════════════════════════════════════════
struct PreResetSnap {
  uint32_t magic;          // 0xB00BBABE — walidacja
  uint32_t uptime_s;       // millis()/1000 przed resetem
  uint32_t freeHeap;       // heap_caps_get_free_size
  uint32_t maxAlloc;       // largest_free_block
  uint32_t psramUsed;      // heapPsramUsed
  uint8_t  wifiState;      // WiFi.status()
  uint8_t  fbReady;        // fbAppReady [v105-FC]
  uint8_t  tgReady;        // tgClientReady
  uint8_t  resetSrc;       // 0=soft/HTTP, 1=DIAG17, 2=WDT(nie zdazy), 3=PANIC, 4=OTA [v169]
  char     checkpoint[40]; // ostatni checkpoint
};
// [FIX-v134-RTC-NOINIT] KRYTYCZNA POPRAWKA: RTC_DATA_ATTR → RTC_NOINIT_ATTR.
// ROOT CAUSE (potwierdzone w źródle ESP-IDF cpu_start.c): RTC_DATA_ATTR trafia do
// sekcji .rtc.bss, którą startup code ZERUJE przy KAŻDYM restarcie niebędącym
// wybudzeniem z deep-sleep — czyli WŁAŚNIE przy WDT_TASK/PANIC/SOFTWARE, dla
// których ten mechanizm miał działać! To wyjaśnia, czemu [PRE-RESET] od "v71"
// ZAWSZE pokazywał "Brak danych": magic był zerowany, zanim setup() go odczytał.
// RTC_NOINIT_ATTR trafia do .rtc_noinit — NIE jest zerowana przez startup code
// (stąd nazwa), przeżywa WDT/PANIC/SW reset. Cena: brak automatycznej inicjalizacji
// przy PIERWSZYM zasilaniu (wartość niezdefiniowana) — dlatego USUNIĘTO inicjalizator
// "= {0}" (dla .rtc_noinit jest on i tak ignorowany przez linker) i DODANO jawne
// zerowanie przy bootResetReason==POWERON/UNKNOWN w setup() (patrz PRE-RESET blok).
// Walidacja przez pole "magic" i tak już chroniła przed odczytem śmieciowych danych.
RTC_NOINIT_ATTR PreResetSnap g_preReset;

// [FIX-v154-PRERESET-FLIPFLOP] Priorytet 4b z planu poprawek WDT.
// ROOT CAUSE: PRE_RESET_UPDATE(2) leci co 2s z tgTaskFn (Core 0) jako "heartbeat"
// niezależnie od tego, co robi loop() (Core 1). Gdy loop() wywołuje jawny restart
// (soft/HTTP src=0 lub DIAG-17 src=1), między zapisem PRE_RESET_UPDATE(0/1) a
// faktycznym ESP.restart() jest delay(100) — w tym oknie tgTaskFn na drugim rdzeniu
// nadal się kręci i jeśli akurat mija 2s od ostatniego heartbeatu, nadpisuje
// g_preReset.resetSrc z powrotem na 2, zanim restart się wykona. Stąd log
// pokazywał src miotający się między HTTP/soft a WDT mimo restartu software'owego.
// FIX: prosta flaga blokująca — ustawiana TUŻ PRZED jawnym PRE_RESET_UPDATE(0/1),
// heartbeat w tgTaskFn sprawdza ją i pomija się, gdy restart jest już w toku.
// Nie wymaga mutexu/spinlocka: to write-once bool (nigdy nie wraca do false przed
// restartem), a jedyny "wyścig" możliwy do zdarzenia to spóźniony o <1 iterację
// heartbeat, który i tak nadpisałby tylko diagnostyczne pola tym samym src.
volatile bool g_restartPending = false;
#define PRE_RESET_MAGIC 0xB00BBABEU

// [FIX-v133-WDT-HUNT] Licznik powtarzających się WDT/PANIC w tym samym checkpoincie.
// Osobna od g_preReset — ta jest czyszczona po każdym odczycie (magic=0), więc nie
// nadaje się do śledzenia trendu przez wiele restartów. g_wdtHunt żyje niezależnie.
// Aktualizowana WYŁĄCZNIE przy potwierdzonym WDT/PANIC (bootResetReason) — normalny
// restart (HTTP/DIAG-17/POWER_ON) nie rusza streak, więc celowy reboot między dwoma
// crashami nie zamaskuje trendu. Drukuje się w logu TYLKO raz, TYLKO gdy streak>=2 —
// pierwszy crash w danym miejscu nie generuje dodatkowej linii (bez spamu).
struct WdtHuntState {
  uint32_t magic;
  char     lastCp[40];
  uint16_t streak;
};
// [FIX-v134-RTC-NOINIT] j.w. — RTC_DATA_ATTR byłby zerowany właśnie przy WDT/PANIC,
// czyniąc streak-detection bezużytecznym. RTC_NOINIT_ATTR + jawne zero przy POWER_ON.
RTC_NOINIT_ATTR WdtHuntState g_wdtHunt;
#define WDT_HUNT_MAGIC 0xACA7ACA7U

// [FIX-v149-TG-OFFSET-RTC] Priorytet 4 z planu poprawek: tgLastUpdateId był zwykłą
// zmienną RAM, zerowaną przy KAŻDYM restarcie (soft/WDT/panic) — Telegram nie ma
// potwierdzenia odbioru starych update'ów, więc po restarcie odsyła cały niepotwierdzony
// backlog (w tym stary np. "rstyes"), co wywołuje kolejny restart w pętli (potwierdzone
// 3x w logach: 4 restarty zamiast jednego). Ten sam wzorzec co g_preReset/g_wdtHunt:
// RTC_NOINIT_ATTR przeżywa SW/WDT/PANIC, zerowana jawnie TYLKO przy POWER_ON/UNKNOWN
// (patrz blok zerowania w setup()), bo przy pierwszym zasilaniu wartość jest niezdefiniowana.
RTC_NOINIT_ATTR long g_tgLastUpdateId;

// strcpy do RTC RAM - zero alokacji, bezpieczne z dowolnego miejsca
#define PRE_RESET_CP(tag) \
  do { strncpy(g_preReset.checkpoint, (tag), sizeof(g_preReset.checkpoint)-1); \
       g_preReset.checkpoint[sizeof(g_preReset.checkpoint)-1] = '\0'; } while(0)

// [FIX-v133-WDT-HUNT] Wariant PRE_RESET_CP dla checkpointów z loop() (Core 1).
// ROOT CAUSE: wszystkie 11 dotychczasowych PRE_RESET_CP(...) są w funkcjach Core 0
// (tgTask/FB/TG) — zero widoczności w loop() (Core 1), a komunikat WDT jednoznacznie
// wskazuje "Główna pętla była zablokowana" = loopTask = Core 1. Stąd [PRE-RESET]
// nigdy nie złapał prawdziwego winowajcy przy ostatnim crashu (WDT_TASK, 22:51).
// Prefiks "L:" odróżnia checkpointy loop() od checkpointów tgTask w raporcie boot.
// Zero logowania — czysty zapis do RTC RAM, spięty z istniejącym makrem LOOP_CP
// (~20 wywołań na iterację loop()) więc pokrycie jest gęste bez nowych call site'ów.
#define PRE_RESET_CP_L(tag) \
  do { \
    g_preReset.checkpoint[0] = 'L'; g_preReset.checkpoint[1] = ':'; \
    strncpy(g_preReset.checkpoint + 2, (tag), sizeof(g_preReset.checkpoint) - 3); \
    g_preReset.checkpoint[sizeof(g_preReset.checkpoint) - 1] = '\0'; \
  } while(0)

#define PRE_RESET_UPDATE(src) \
  do { \
    g_preReset.magic     = PRE_RESET_MAGIC; \
    g_preReset.uptime_s  = millis() / 1000UL; \
    g_preReset.freeHeap  = heap_caps_get_free_size(MALLOC_CAP_8BIT); \
    g_preReset.maxAlloc  = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT); \
    g_preReset.psramUsed = heap_caps_get_total_size(MALLOC_CAP_SPIRAM) \
                         - heap_caps_get_free_size(MALLOC_CAP_SPIRAM); \
    g_preReset.wifiState = (uint8_t)WiFi.status(); \
    g_preReset.fbReady   = (uint8_t)fbAppReady; /* [v105-FC] fbClientReady→fbAppReady */ \
    g_preReset.tgReady   = (uint8_t)tgClientReady; \
    g_preReset.resetSrc  = (src); \
  } while(0)

esp_reset_reason_t bootResetReason  = ESP_RST_UNKNOWN;  // zapisana w setup()
volatile bool      tgBootNotifyPending = true;          // wyślij przy pierwszym połączeniu TG

// ─────────────────────────────────────────────────────────────
// TELEGRAM - konfiguracja i stan
// ─────────────────────────────────────────────────────────────
#define TG_CONFIG_FILE "/telegram_config.json"

// [v116] FIX-TG-PARTITION-WIPE: Hardkodowane wartości domyślne Telegrama.
// Gdy LittleFS jest czysty (zmiana partycji, formatowanie, nowy układ flash),
// loadTelegramConfig() użyje tych wartości i od razu zapisze plik — TG działa
// bez żadnej konfiguracji przez panel WWW.
// Wypełnij TOKEN i CHAT_ID swoimi danymi (token z @BotFather, chatId z @myidbot):
#define DEFAULT_TG_BOT_TOKEN  "8709162940:AAFJJGoI79XHiBkZYtP_IEZjkXO5yEzhDpo"   // ← wklej token np. "1234567890:AABBccDDeeff..."
#define DEFAULT_TG_CHAT_ID    "429181594"   // ← wklej chat_id np. "-1001234567890"
#define DEFAULT_TG_ENABLED    true // ← false = TG domyślnie wyłączony po czystym LittleFS

String  tgBotToken   = "";   // token bota - ustawiany przez panel WWW lub DEFAULT_TG_BOT_TOKEN
String  tgChatId     = "";   // chat_id odbiorcy - ustawiany przez panel WWW lub DEFAULT_TG_CHAT_ID
bool    tgEnabled    = false;
long    tgLastUpdateId = -1;
unsigned long tgLastPollMs = 0;

// ── v32: stałe połączenie SSL do api.telegram.org ──
// Jeden obiekt TLS trzymany przez cały czas działania programu.
// Eliminuje fragmentację HEAP spowodowaną setkowym alloc/dealloc SSL.
WiFiClientSecure tgClient;                           // globalny - alokacja SSL raz na zawsze
bool             tgClientReady = false;
unsigned long    tgClientLastUse = 0;
// [v75] FIX-WDT-DNS: cache adresów IP aby connect() nie robił DNS (udp_new_ip_type assert)
IPAddress        g_tgServerIP;
unsigned long    g_tgIPTs    = 0;     // czas ostatniego resolve
bool             g_tgIPValid = false; // czy IP jest poprawne
const unsigned long TG_IDLE_RECONNECT_MS = 55000UL; // Telegram zamyka idle po ~60s -> my po 55s

// ── [v105-FC] FirebaseClient globals ─────────────────────────────────────────
// ZASTĄPIŁY (usunięte zmienne):
//   WiFiClientSecure fbClient          — własny SSL client, błędny connected()
//   bool fbClientReady / fbBatchActive  — ręczne zarządzanie stanem TCP
//   unsigned long fbClientLastUse / fbLastSuccessMs
//   const unsigned long FB_IDLE_RECONNECT_MS
//   IPAddress g_fbServerIP / g_fbIPTs / g_fbIPValid
//
// ROOT CAUSE FIX (v99–v104): WiFiClientSecure::connected() zwracał true dla
// martwego socketu (bug peek() w ESP32 core). FirebaseClient używa ESP_SSLClient
// z prawidłowym connected() check → eliminuje całą klasę WDT crashy.
//
// bool fbConnect() (307 linii, wersje v65→v104) → zastąpiona przez
// void fbInitialize() (~50 linii), wywoływana raz w setup() po WiFi.connected().
// ─────────────────────────────────────────────────────────────────────────────
WiFiClientSecure    fbSSLClient;                                       // [v105-FC] SSL transport
// [v105-FC-FIX] DefaultNetwork usunięty w FirebaseClient v2.x — sieć wykrywana automatycznie.
AsyncClientClass    fbAsyncClient(fbSSLClient);                        // [v105-FC] async HTTP client (v2: tylko ssl_client)
FirebaseApp         fbApp;                                             // [v105-FC] Firebase application
RealtimeDatabase    fbDatabase;                                        // [v105-FC] RTDB interface
LegacyToken         fbLegacyToken(FIREBASE_SECRET);                   // [v105-FC] Database Secret auth
bool                fbAppReady = false;                                // [v105-FC] true po initializeApp()
// [v226] FIX-FB-IDLE-RECONNECT: `fbClientLastUse`/`FB_IDLE_RECONNECT_MS`
// usunięto w v105 na założeniu, że FirebaseClient+ESP_SSLClient "prawidłowo
// wykrywa martwy socket" — eliminując całą klasę problemów, w tym potrzebę
// wiekowego reconnectu. To założenie okazało się częściowo błędne:
// log_combined__21_.txt (2026-07-2x) pokazał `[FB-BLOCK-DONE] total=9624ms`
// — sygnaturę niemal identyczną z pierwotnym bugiem z v104 (`total=9613ms`,
// martwe gniazdo Firebase podczas reuse). Autor biblioteki FirebaseClient
// (mobizt) sam potwierdza w github.com/mobizt/FirebaseClient/discussions/99:
// "known bug in ESP32 WiFiClient... when remote server closes the connection
// while ssl_client.connected() still returned true... you have to close the
// connection using ...stop() at least when it was connected in more than
// two minutes" — dokładnie ten sam bug (peek()), tylko przeniesiony do
// ESP_SSLClient, bo ta klasa dziedziczy po tym samym WiFiClient. Ta sama
// ochrona istnieje po stronie TG od v32 (`tgClientLastUse`/
// `TG_IDLE_RECONNECT_MS`, ~linia 6968) i nigdy nie została usunięta stamtąd —
// tylko FB stracił ją w v105. Przywrócone tutaj, z progiem dobranym
// konserwatywnie poniżej granicy "2 minuty" z rekomendacji mobizt (podobnie
// jak TG działa poniżej granicy zamknięcia idle przez serwer, nie na niej).
unsigned long       fbClientLastUse = 0;
const unsigned long FB_IDLE_RECONNECT_MS = 100000UL; // mobizt: reconnect przed 120s

// ─────────────────────────────────────────────────────────────────────────────
// [FIX-v131-CIRCUIT-BREAKER] Ochrona przed kumulacją blokad sieciowych → WDT.
// Każda funkcja FB/TG z osobna jest WDT-safe (reset co 10ms w polling-loop),
// ale suma kolejnych bloków może przekroczyć 15s okno WDT.
// Mechanizm: licznik streak + cooldown + budżet iteracji.
// ─────────────────────────────────────────────────────────────────────────────
volatile uint8_t  _fbFailStreak       = 0;    // ile kolejnych TIMEOUT/ERROR z rzędu — tylko Firebase
volatile uint8_t  _tgFailStreak       = 0;    // ile kolejnych TIMEOUT/ERROR z rzędu — tylko Telegram
// [v230] FIX-SHARED-STREAK: rozdzielone z jednego _netFailStreak — patrz niżej w makrach.
volatile bool     _netCircuitOpen     = false; // true = circuit open, skip FB+TG
volatile uint32_t _netCooldownUntilMs = 0;    // millis() kiedy wolno próbować ponownie
volatile uint32_t _netBackoffMs       = 30000UL; // aktualny backoff (30s → 60s → 120s, max 120s)
// Stałe circuit breakera
static const uint8_t  NET_FAIL_THRESHOLD  = 2;      // ile streak trigguje circuit open
static const uint32_t NET_BACKOFF_INIT_MS = 30000UL; // pierwszy cooldown: 30s
static const uint32_t NET_BACKOFF_MAX_MS  = 120000UL;// maksymalny cooldown: 120s

// [FIX-v131] Inkrementuj streak i otwórz circuit jeśli próg przekroczony.
// Wywołuj na końcu KAŻDEGO TIMEOUT/ERROR path w FB i TG.
// [v230] FIX-SHARED-STREAK: streak przekazywany jako parametr (_fbFailStreak/_tgFailStreak)
// zamiast jednego wspólnego licznika — sukces jednej usługi już nie kasuje streaku drugiej.
// _netCircuitOpen/_netCooldownUntilMs/_netBackoffMs zostają WSPÓLNE (celowo) — to ochrona
// budżetu iteracji/WDT, ryzyko globalne niezależne od tego, która usługa akurat zawiodła.
#define NET_FAIL(streak) do { \
  streak = (uint8_t)(streak + 1); \
  if (streak >= NET_FAIL_THRESHOLD && !_netCircuitOpen) { \
    _netCircuitOpen     = true; \
    _netCooldownUntilMs = millis() + _netBackoffMs; \
    logPrintf("lvl=WARN tag=CB msg=\"Circuit OPEN\" streak=%d backoff=%lus cooldown_until=%lus\n", \
              (int)streak, \
              (unsigned long)(_netBackoffMs / 1000UL), \
              (unsigned long)(_netCooldownUntilMs / 1000UL)); \
    _netBackoffMs = min(_netBackoffMs * 2, NET_BACKOFF_MAX_MS); \
  } \
} while(0)

// [FIX-v131] Zeruj streak po sukcesie operacji sieciowej.
// Wywołuj po każdym OK w FB i TG.
#define NET_SUCCESS(streak) do { \
  if (streak > 0 || _netCircuitOpen) { \
    logPrintf("lvl=INFO tag=CB msg=\"Circuit RESET\" streak_byl=%d\n", (int)streak); \
  } \
  streak          = 0; \
  _netCircuitOpen = false; \
  _netBackoffMs   = NET_BACKOFF_INIT_MS; \
} while(0)
// [v106-FIX-USE-AFTER-FREE] AsyncResult MUSI być globalny — nie może być lokalny na stosie.
// tgTaskFn wywołuje fbApp.loop() PO powrocie z funkcji; SlotManager::returnResult()
// zapisuje wynik do obiektu, który lokalny destruktor już usunął → String::buffer=nullptr
// → StoreProhibited @ 0x00000001 (EXCVADDR). Globalny obiekt żyje przez cały czas programu.
AsyncResult         g_fbSendResult;   // status
AsyncResult         g_fbCmdResult;    // legacy /aquarium/cmd (disabled)
AsyncResult         g_fbQueueResult;  // command queue
AsyncResult         g_fbDelResult;    // command delete
AsyncResult         g_fbCfgResult;    // config

// [v253] NATIVE ASYNC FIREBASE STATE MACHINE
// Jeden request Firebase naraz. fbApp.loop() jest pompowane z tgTask, ale
// nigdy nie czekamy tutaj w while na zakończenie TLS/HTTP.
enum class FbAsyncOp : uint8_t { NONE, GET_COMMANDS, DELETE_COMMAND, GET_CONFIG, SET_STATUS };
static FbAsyncOp g_fbAsyncOp = FbAsyncOp::NONE;
static uint32_t g_fbAsyncOpStartedMs = 0;
static uint32_t g_fbAsyncOpTimeoutMs = 0;
static String g_fbAsyncActiveKey = "";
static uint64_t g_fbAsyncActiveTs = 0;
static String g_fbAsyncConfigBody = "";
static String g_fbAsyncStatusJson = "";
static bool g_fbAsyncResultHandled = false;
static uint32_t g_fbAsyncRetryAtMs = 0;
static uint32_t g_fbAsyncTimeoutCount = 0;
static uint32_t g_fbAsyncCompletedCount = 0;
static bool g_fbConfigRefreshPending = false;
static uint64_t g_fbConfigRefreshTs = 0;
static String g_fbConfigRefreshKey = "";
String g_fbConfigRefreshDeletePendingKey = "";
static constexpr uint32_t FB_ASYNC_CMD_TIMEOUT_MS = 9000UL;
static constexpr uint32_t FB_ASYNC_DEL_TIMEOUT_MS = 6000UL;
static constexpr uint32_t FB_ASYNC_CFG_TIMEOUT_MS = 9000UL;
static constexpr uint32_t FB_ASYNC_SEND_TIMEOUT_MS = 12000UL;

static inline const char* fbAsyncOpName(FbAsyncOp op) {
  switch (op) {
    case FbAsyncOp::GET_COMMANDS: return "CMD_GET";
    case FbAsyncOp::DELETE_COMMAND: return "CMD_DEL";
    case FbAsyncOp::GET_CONFIG: return "CFG_GET";
    case FbAsyncOp::SET_STATUS: return "STATUS_SET";
    default: return "NONE";
  }
}
// ─────────────────────────────────────────────────────────────────────────────

long    tgLastMenuMsgId = -1;   // message_id aktualnego menu w czacie
// ── v30-2: lista ostatnich message_id bota do masowego usunięcia ──
#define TG_HISTORY_SIZE 20
long    tgMsgHistory[TG_HISTORY_SIZE] = {0};  // ring-buffer message_id wysłanych wiadomości
int     tgMsgHistoryIdx = 0;

// ── Telegram - odroczone komendy (FIX-v29-15-TG-SSL, FEATURE-v29-16) ──
enum TgDeferCmd : uint8_t {
  TG_NONE = 0,
  TG_MENU,          // pokaż główne menu
  TG_LOGS,          // wyślij plik log.txt
  TG_STATUS,        // raport statusu
  TG_TEMP,          // raport temperatur
  TG_ENERGY,        // raport energii
  TG_LIGHT,         // raport lux / światła
  TG_ADAPT,         // raport adaptacji
  TG_LED_TOGGLE,    // przełącz zasilanie LED
  TG_TRYB_TOGGLE,   // przełącz tryb AUTO↔MANUAL
  TG_CLR_CONFIRM,   // prośba o potwierdzenie czyszczenia logów
  TG_CLR_DO,        // faktyczne czyszczenie logów
  TG_RESTART_CONFIRM, // prośba o potwierdzenie restartu
  TG_RESTART_DO,    // faktyczny restart
  // ── v30-0 ──
  TG_SENSOR_HIST,   // historia czujników (ostatnie wartości)
  TG_SCHEDULE,      // raport harmonogramu świateł
  TG_NOTIF_TOGGLE,  // włącz/wyłącz powiadomienia Telegram
  // ── v34 ──
  TG_POLL,          // wewnętrzny: wykonaj getUpdates (timer w tasku)
  // ── v227 (test WDT-DAILYSUM, tymczasowe) ──
  TG_TEST_DAILY_SUMMARY, // wymuś jednorazowe logDailySummary() bez czekania na północ
  // ── 4.1.0 OTA-GITHUB (Etap 1 planu upgrade) ──
  TG_OTA_UPDATE    // sprawdź release na GitHub i zaktualizuj firmware (komenda /update)
};
volatile TgDeferCmd tgDeferredCmd = TG_NONE;
unsigned long tgMenuReturnAt = 0;   // gdy >0: powróć do menu po upływie czasu

// [v227] TEST-ONLY (plan flash-freeze, Partia 4): flaga ustawiana komendą
// /testdaily, sprawdzana w logDailySummary() na Core 1 - pozwala wymusić
// jednorazowe wykonanie funkcji (razem z nowymi esp_task_wdt_reset()) bez
// czekania na naturalną zmianę dnia o północy. Do usunięcia po zakończeniu
// testów Partii 4 - patrz RYBY_PLAN_FLASH_FREEZE.md.
volatile bool g_forceDailySummaryTest = false;

// ── v34: FreeRTOS task - Telegram na Core 0, loop() na Core 1 ──
// Wszystkie operacje sieciowe TG (connect, POST, read) wykonują się w tgTask,
// nigdy nie blokując loop(). loop() tylko wrzuca TgDeferCmd do kolejki.
QueueHandle_t tgCmdQueue   = nullptr;  // kolejka komend loop() -> tgTask
TaskHandle_t  tgTaskHandle = nullptr;  // handle tasku (do diagnostyki)

// [v250-ARCH] Jedyny kanał wejściowy dla ZEWNĘTRZNYCH zmian sterujących.
// Callbacki sieciowe (Firebase/HTTP/Telegram/WebSocket) tylko enqueue.
// Konsumentem i wykonawcą jest loop() na Core 1.
enum class AppCommandType : uint8_t { POWER_SET, TRYB_SET, POWER_TOGGLE, TRYB_TOGGLE, PUMP_SET, LED_TEST, LED_OFF, RESTART, PWM_SET };
struct AppCommand {
  AppCommandType type = AppCommandType::POWER_SET;
  bool value = false;
  uint16_t pwm[5] = {0,0,0,0,0};
  char source[12] = {0};
  bool firebaseAck = false;
  uint64_t firebaseTs = 0;
  char firebaseKey[64] = {0};
};
QueueHandle_t appCmdQueue = nullptr;

struct FirebaseAck {
  bool success = false;
  uint64_t ts = 0;
  char key[64] = {0};
};
QueueHandle_t fbAckQueue = nullptr;
volatile bool fbQueueInFlight = false;
static bool enqueueAppCommand(AppCommandType type, bool value, const uint16_t* pwm, const char* source,
                               uint64_t firebaseTs = 0, const char* firebaseKey = nullptr) {
  if (!appCmdQueue) return false;
  AppCommand c; c.type=type; c.value=value;
  if (pwm) memcpy(c.pwm, pwm, sizeof(c.pwm));
  if (source) { strncpy(c.source, source, sizeof(c.source)-1); c.source[sizeof(c.source)-1]='\0'; }
  if (firebaseKey && firebaseKey[0]) {
    c.firebaseAck = true; c.firebaseTs = firebaseTs;
    strncpy(c.firebaseKey, firebaseKey, sizeof(c.firebaseKey)-1);
    c.firebaseKey[sizeof(c.firebaseKey)-1] = '\0';
  }
  if (xQueueSend(appCmdQueue, &c, 0) != pdPASS) {
    logPrintf("lvl=WARN tag=APP-QUEUE msg=\"FULL\" source=%s type=%u\n", c.source, (unsigned)c.type);
    return false;
  }
  if (c.firebaseAck) fbQueueInFlight = true;
  logPrintf("lvl=INFO tag=APP-QUEUE msg=\"ENQUEUE\" source=%s type=%u fbAck=%d\n", c.source, (unsigned)c.type, c.firebaseAck ? 1 : 0);
  return true;
}
static void processAppCommands();

// [OK] FIX-v33d-LOG-MUTEX: mutex serializuje zapis do LittleFS z Core 0 i Core 1.
// logToFile() używa dwóch niezależnych buforów (per-rdzeń) -> brak wyścigu na String.
// Core 1 (loop) używa timeout=0 -> nigdy nie blokuje -> zero LOOP-SLOW.
// Core 0 (tgTask) używa portMAX_DELAY -> czeka, nie traci wpisów (nie jest time-critical).
SemaphoreHandle_t logMutex = nullptr;
// [v46] LOG-FLUSH-CORE0: bufory logów przeniesione z static-wewnątrz-funkcji na globalne
// żeby logFlushCore1() (wywoływana z tgTask) mogła zapisać bufor Core 1 do LittleFS.
// [v54] LOG-BUF-PSRAM: bufory logów przeniesione z DRAM (String) do PSRAM (char*).
// Alokacja w setup() przez psramAllocSafe(). DRAM free: +16 kB.
#define LOG_BUF_SIZE  8192
// [v146] USUNIETO: #define HTML_BUF_CAP (używane wyłącznie przez /control)
char*         g_logBuf[2]    = {nullptr, nullptr};
size_t        g_logBufLen[2] = {0, 0};
unsigned long g_lastFlush[2] = {0, 0};

// [v62-1A] Śledzenie chwilowego minimum DRAM z dokładnym znacznikiem czasu.
// Aktualizowane w diagHeap() co 30s. Logowane w heartbeat co 60s jako min=NB(at Xs).
static uint32_t g_heapMinValue     = UINT32_MAX;  // najniższe zmierzone DRAM free
static uint32_t g_heapMinTimestamp = 0;           // uptime [s] gdy minimum zostało pobite

// ── [v67-Z1] HeapSrcTag — klasyfikacja źródła dołków HEAP ─────────────────────
// Ustawiana w tgEnsureConnected() / fbConnect() przed TLS connect().
// Kasowana po ustawieniu *ClientReady. Logowana w heartbeat co 60s jako heapSrc=.
enum HeapSrcTag : uint8_t {
  HEAP_SRC_IDLE   = 0,  // brak aktywnego TLS handshake
  HEAP_SRC_TG_TLS = 1,  // TLS handshake Telegram
  HEAP_SRC_FB_TLS = 2,  // TLS handshake Firebase
  HEAP_SRC_BOTH   = 3   // oba jednocześnie (nie powinno się zdarzyć po Z3)
};
// [v69b-FIX-W2] volatile HeapSrcTag g_heapSrc usunięta — dead code od v69-Z1 (zastąpiona przez g_heapSrcAccum)
// [v69-Z1] Akumulator: OR wszystkich źródeł od ostatniego heartbeat.
// HEAP_SRC_TG_TLS=1, FB_TLS=2 → OR=3 gdy oba w tym samym cyklu.
// Nigdy nie gubi informacji przy back-to-back handshake'ach.
volatile uint8_t g_heapSrcAccum = 0;  // bit0=TG, bit1=FB. 1 B DRAM.
static const char* heapSrcName(HeapSrcTag t) {
  switch(t) {
    case HEAP_SRC_TG_TLS: return "TG_TLS";
    case HEAP_SRC_FB_TLS: return "FB_TLS";
    case HEAP_SRC_BOTH:   return "BOTH";
    default:              return "IDLE";
  }
}

// ── [v67-Z3] TLS-MUTEX — max 1 handshake TLS naraz ───────────────────────────
// Tworzony w setup(). tgEnsureConnected() i fbConnect() biorą przed connect(),
// oddają po ustawieniu *ClientReady. Eliminuje jednoczesne TG+FB TLS i ich sumę ~90kB.
SemaphoreHandle_t tlsMutex = nullptr;  // [v67-Z3] KOSZT: 88 B DRAM
// [v69-Z5] Statystyki TLS-MUTEX — czas oczekiwania na mutex. KOSZT: 10 B DRAM (BSS).
volatile uint32_t g_tlsMutexWaitTG    = 0;   // uint32: atomowy zapis na Xtensa LX7 (wyrównany) [FIX-W5]
volatile uint32_t g_tlsMutexWaitFB    = 0;   // uint32: atomowy zapis na Xtensa LX7 (wyrównany) [FIX-W5]
volatile uint32_t g_tlsMutexMaxWaitMs = 0;   // najdłuższe oczekiwanie [ms]
// [v84] FIX-TLS-GAP: timestamp zakończenia ostatniego TLS handshake (obu tasks).
// Używany do wymuszenia minimalnego odstępu między handshake'ami — defragmentacja sterty.
// Dostęp z dwóch tasków: volatile + 32-bit aligned = atomowy na Xtensa LX7.
volatile uint32_t g_lastTlsHandshakeDoneMs = 0;  // [v84] KOSZT: 4 B DRAM (BSS)

// [v155] DIAG-TGSENDSTALL: licznik + max czas dla printf()/print() w tgPost(),
// które trwały > TG_SEND_STALL_THRESHOLD_MS MIMO że tgWriteGuard() (v150)
// wcześniej przepuścił gniazdo jako zapisywalne — czyli realny odczyt
// residual risk opisanego w komentarzu przy tgWriteGuard() (~linia 5089).
// Ten sam wzorzec licznika co g_tlsMutexWaitTG/g_tlsMutexMaxWaitMs.
volatile uint32_t g_tgSendStallCount = 0;   // ile razy próg przekroczony (headers+body łącznie)
volatile uint32_t g_tgSendStallMaxMs = 0;   // najdłuższy zaobserwowany pojedynczy send()
static const unsigned long TG_SEND_STALL_THRESHOLD_MS = 2000;  // 2s: wyraźnie > normalny send(), wyraźnie < WDT 10s

// [v156] DIAG-WSCLEANUP: licznik + max czas dla wsTerminal.cleanupClients()
// w loop() — Priorytet 5b z planu poprawek WDT ("[v125] podejrzany #1 — co
// 30s, potencjalne 200-500ms"). Nigdy wcześniej nie zmierzone na żywym
// urządzeniu, tylko podejrzewane. Ten sam wzorzec licznika co
// g_tgSendStallCount/g_tgSendStallMaxMs (v155).
volatile uint32_t g_wsCleanupStallCount = 0;   // ile razy próg przekroczony
volatile uint32_t g_wsCleanupStallMaxMs = 0;   // najdłuższy zaobserwowany cleanupClients()
static const unsigned long WS_CLEANUP_STALL_THRESHOLD_MS = 200;  // dolna granica z podejrzenia [v125] "200-500ms"

// [v157] DIAG-FLASHSTALL: licznik + max czas dla zapisu LittleFS (LittleFS.open+
// write+close) wywoływanego na Core 0 wewnątrz logToFile()/logFlushCore1() —
// zbadanie hipotezy [L:pompa] z planu poprawek WDT (Priorytet 0/1, wątek
// "zbadać osobno"): ESP-IDF dokumentuje, że operacje flash na SPI1 wymagają
// wyłączenia cache instrukcji na OBU rdzeniach jednocześnie (nie tylko na
// rdzeniu wykonującym zapis) — więc długi zapis LittleFS na Core 0 (tgTask)
// może zablokować Core 1 (loop(), w tym updatePump()) mimo że sam
// updatePump() nie zawiera żadnego blokującego kodu. Ten sam wzorzec licznika
// co g_tgSendStallCount (v155) / g_wsCleanupStallCount (v156).
volatile uint32_t g_flashStallCount = 0;   // ile razy próg przekroczony (oba miejsca łącznie)
volatile uint32_t g_flashStallMaxMs = 0;   // najdłuższy zaobserwowany pojedynczy zapis LittleFS
static const unsigned long FLASH_STALL_THRESHOLD_MS = 150;  // > udokumentowane ~80ms/4KB (micropython #3782), margines na wolniejsze karty

// [v158] DIAG-EVENTSSTALL: log_combined__12_.txt (03:08:04, WDT_TASK) miał
// ostatni checkpoint [L:events] — sekcja tuż przed [L:adaptacja] w loop(),
// obejmująca warunkowe wywołanie onSectionsChange(). Przegląd kodu
// (saveFadeToEEPROM/saveScheduleToEEPROM) nie znalazł oczywistej blokady —
// ten licznik mierzy realny czas trwania onSectionsChange() na żywym
// urządzeniu, żeby to potwierdzić lub wykluczyć. Ten sam wzorzec co
// g_flashStallCount (v157) / g_tgSendStallCount (v155). To NIE jest fix —
// zachowanie onSectionsChange() bez zmian.
volatile uint32_t g_eventsStallCount = 0;   // ile razy próg przekroczony
volatile uint32_t g_eventsStallMaxMs = 0;   // najdłuższy zaobserwowany czas onSectionsChange()
static const unsigned long EVENTS_STALL_THRESHOLD_MS = 150;  // spójne z FLASH_STALL_THRESHOLD_MS

// [v160] DIAG-FSGUARDSTALL: patrz komentarz przy `if (fsGuardPending)` w
// tgTask — mierzy cały blok fsGuardPending (teraz wraz z sendTelegramDocument()
// z v159 FIX-LOG-SEND-TRIGGER + rotacją/przycinaniem FS), nieobjęty dotąd
// żadną instrumentacją. Ten sam wzorzec co pozostałe liczniki.
volatile uint32_t g_fsGuardStallCount = 0;
volatile uint32_t g_fsGuardStallMaxMs = 0;

// [v162] DIAG-DNSSTALL: WiFi.hostByName() (tgEnsureConnected, sendTelegramDocument,
// fbInitialize) jest jedynym pozostałym w pełni blokującym wywołaniem na ścieżce
// half-open, dotąd nigdy realnie niezmierzonym — tylko zabezpieczonym resetem WDT
// przed i po. 5 kolejnych crashy WDT (log 12/15/17/18) miało 5 różnych checkpointów
// w loop() (Core 1), a żaden licznik Core-1 (events/flash/fsGuard) nic nie złapał —
// patrz plan_poprawek_WDT.md. To najsilniejszy pozostały kandydat.
volatile uint32_t g_dnsStallCount = 0;
volatile uint32_t g_dnsStallMaxMs = 0;
static const unsigned long DNS_STALL_THRESHOLD_MS = 2000;  // powyżej dolnej granicy udokumentowanej "normy" ~1-4s
static const uint32_t TLS_COOLDOWN_MS = 3000;    // [v84] 3s cooldown między handshake'ami
static const size_t PSRAM_TLS_THRESHOLD = 1;  // [v85/v86] FIX-PSRAM-TLS: próg alokacji extmem

// ── [v86] FIX-PSRAM-TLS-D: własny alokator mbedTLS → bufory TX/RX do PSRAM ──────
// heap_caps_malloc_extmem_enable() (v85) nie działa dla mbedTLS ponieważ biblioteka
// wywołuje heap_caps_malloc(MALLOC_CAP_INTERNAL) bezpośrednio, omijając próg extmem.
// Rozwiązanie: nadpisujemy hooki platformowe mbedtls_platform_set_calloc_free().
// Alokacje >= PSRAM_TLS_THRESHOLD i psramFound() → PSRAM (MALLOC_CAP_SPIRAM).
// Fallback: internal DRAM gdy PSRAM niedostępny lub pełny (bezpieczne).
// WiFi/DMA używa MALLOC_CAP_DMA — ignoruje ten alokator → zero ryzyka dla WiFi.
// ─────────────────────────────────────────────────────────────────────────────────
static void* mbedtls_psram_calloc(size_t n, size_t size) {
    size_t total = n * size;
    void* ptr = nullptr;
    if (total >= PSRAM_TLS_THRESHOLD && psramFound()) {
        ptr = heap_caps_malloc(total, MALLOC_CAP_SPIRAM);
    }
    if (!ptr) {
        // fallback: internal DRAM (bezpieczny, wymagany dla ssl_context < 4096 B)
        ptr = heap_caps_malloc(total, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    }
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

static void mbedtls_psram_free(void* ptr) {
    free(ptr);
}
// ─────────────────────────────────────────────────────────────────────────────────

// ── [v67-Z4] HEAP-DRIFT — rolling window 60 minut ────────────────────────────
// Co minutę (heartbeat) push freeH [kB] do kołowego bufora.
// Co godzinę: avg60 vs prevAvg60 → detekcja powolnego wycieku.
static uint16_t g_heapHist60[60]   = {0};  // KOSZT: 120 B DRAM (BSS)
static uint8_t  g_heapHistIdx      = 0;
static uint8_t  g_heapHistFill     = 0;    // ile slotów wypełnionych (maks 60)
static float    g_heapPrevAvg      = 0.0f; // avg poprzedniej godziny [kB]
static uint8_t  g_heapDriftCount   = 0;    // ile godzin z rzędu spada
static uint16_t g_heapMin60        = 65535; // [v69-Z3a] minimum freeH bieżącej godziny [kB]. 2 B DRAM.
static uint16_t g_heapDriftCycle   = 0;    // [v69-Z3b] numer godziny od startu. 2 B DRAM.

// ── [v67-Z5] DS-DIAG — flaga błędu czujnika wody przy starcie ─────────────────
volatile bool   ds3BootFail    = false;  // ustawiana w setup() → alert Telegram w tgTask
volatile int8_t g_ds3PinLevel  = -1;     // [v69-Z0] -1=niezmierzone, 0=LOW, 1=HIGH. 1 B DRAM.
volatile bool   g_ds3ResetOk   = false;  // [v69-Z4] wynik reset pulse 1-Wire. 1 B DRAM.
volatile bool   g_ds3AddrOk    = false;  // [v69-Z4] czy ROM odczytany poprawnie. 1 B DRAM.


/*************************************************************
 *  CZUJNIKI TEMPERATURY (DS18B20)
 *************************************************************/
#define ONE_WIRE_BUS1 14   // płyta LED 1
#define ONE_WIRE_BUS2 17   // płyta LED 2 (S3: GPIO 27 nie wyprowadzony na tej płytce)
#define ONE_WIRE_BUS3 5    // [v94-FIX-DS18B20] temperatura wody — zmieniono z GPIO39 (JTAG TCK,
                           // biblioteka OneWire_direct_gpio.h traktuje piny >33 jako input-only na ESP32-S3)
                           // GPIO5 = pełny I/O, brak konfliktu z JTAG/PSRAM.

OneWire oneWire1(ONE_WIRE_BUS1);
OneWire oneWire2(ONE_WIRE_BUS2);
OneWire oneWire3(ONE_WIRE_BUS3);

/*************************************************************
 *  CZUJNIK OBECNOŚCI WODY (analogowy, wykrywanie zalania)
 *  GPIO6 = ADC1_CH5, wolny pin, pełny I/O, ADC1 działa poprawnie
 *  przy aktywnym WiFi (w przeciwieństwie do ADC2).
 *************************************************************/
#define WATER_LEAK_PIN 6
// Progi 0-4095 (12-bit ADC). Czysty pomiar: sucho ~0, w wodzie ~2100-2200.
// Histereza (DETECT > CLEAR) zapobiega migotaniu powiadomień przy odczycie
// oscylującym/wysychającym w strefie przejściowej.
#define WATER_LEAK_THRESHOLD_DETECT 500   // powyzej tego = woda wykryta (zmniejszone o polowe dla czulosci)
#define WATER_LEAK_THRESHOLD_CLEAR  200   // ponizej tego = sucho

volatile bool waterLeakDetectedNotifyPending = false;
volatile bool waterLeakClearedNotifyPending  = false;

DallasTemperature sensors1(&oneWire1);
DallasTemperature sensors2(&oneWire2);
DallasTemperature sensors3(&oneWire3);

// Aktualne temperatury
// [OK] FIX F: Inicjalizuj temperaturami bliskimi normalnej pracy (nie 0.0).
// Przy 0.0 derating i emergency thermal są niewidome przez pierwsze ~15s (do pierwszego odczytu DS18B20).
// 25°C to bezpieczna wartość - poniżej TEMP_START_DERATING (45°C), derating nie aktywuje się fałszywie,
// ale też nie ukrywa realnie gorącej płyty przy szybkim restarcie (gdzie temp może być >45°C od razu).
float tempPlate1 = 25.0;
float tempPlate2 = 25.0;
float tempWater  = 20.0;
// [OK] FIX-v33i: 2-fazowy odczyt DS18B20 - zero delay() w loop()
bool          tempConvRequested   = false;  // true gdy Faza1 (request) już wysłana
unsigned long tempConvRequestedAt = 0;      // millis() momentu wysłania requestu
unsigned long tempLastRead        = 0;      // millis() ostatniego pełnego cyklu (Faza2)
/*************************************************************
 *  LED - PINY / PWM / KOLORY
 *************************************************************/
// Kolejność sekcji:
// 0: białe | 1: fs | 2: fs białe | 3: niebieskie | 4: czerwone
const int pinyLED[] = {16, 19, 4, 10, 13};  // S3: 25 nie istnieje -> 10
const int pwmChannels[] = {0, 1, 2, 3, 4};

const char* kolor[] = {
  "białe", "fs", "fs białe", "niebieskie", "czerwone"
};

// Gamma per kanał
float gammaTable[] = {1, 1, 1, 1, 1};

/*************************************************************
 *  TABELE MOCY - pomiary rzeczywiste (CSV pwm_test_all.csv)
 *  Indeks kanału: 0=Biale, 1=FS, 2=FS_biale, 3=Niebieskie, 4=Czerwone
 *  21 kroków PWM: 0, 50, 100, ... 1000  (krok co 50)
 *************************************************************/
static const float powerTable[5][21] = {
  // 0: Białe
  { 0.00f, 1.70f, 2.80f, 3.80f, 4.40f, 5.30f, 6.80f, 7.50f, 8.50f, 9.50f,
   10.60f,11.50f,12.00f,13.30f,14.30f,15.50f,16.50f,17.00f,18.30f,19.10f,20.00f },
  // 1: FS
  { 0.00f, 1.50f, 3.00f, 3.50f, 4.20f, 5.30f, 6.60f, 7.40f, 8.20f, 9.60f,
   10.50f,11.90f,12.50f,13.70f,14.60f,15.70f,16.70f,17.70f,18.40f,19.40f,20.30f },
  // 2: FS_biale (FS + Biale łącznie)
  { 0.00f, 3.60f, 7.00f, 9.70f,13.10f,16.50f,19.50f,22.60f,25.60f,29.00f,
   33.30f,36.60f,39.70f,42.70f,45.50f,48.50f,51.90f,54.60f,57.70f,61.00f,63.80f },
  // 3: Niebieskie
  { 0.00f, 1.20f, 1.50f, 2.00f, 2.70f, 2.70f, 3.30f, 3.80f, 3.90f, 4.40f,
    4.80f, 5.10f, 5.50f, 5.80f, 6.10f, 6.70f, 7.00f, 7.30f, 7.90f, 8.20f, 8.50f },
  // 4: Czerwone
  { 0.00f, 1.20f, 1.50f, 1.80f, 2.10f, 2.40f, 2.70f, 3.00f, 3.20f, 3.30f,
    3.60f, 3.90f, 4.10f, 4.50f, 4.80f, 4.80f, 5.10f, 5.40f, 5.80f, 5.80f, 6.10f }
};

// Interpolacja liniowa: zwraca moc [W] dla danego kanału i wartości PWM 10-bit (0..1023)
float getPowerForChannel(int ch, uint16_t pwm10bit) {
  if (ch < 0 || ch > 4) return 0.0f;
  float pwmNorm = (float)pwm10bit * (1000.0f / 1023.0f);
  if (pwmNorm <= 0.0f) return 0.0f;
  if (pwmNorm >= 1000.0f) return powerTable[ch][20];
  int idx = (int)(pwmNorm / 50.0f);
  if (idx >= 20) idx = 19;
  float frac = (pwmNorm - idx * 50.0f) / 50.0f;
  return powerTable[ch][idx] + frac * (powerTable[ch][idx + 1] - powerTable[ch][idx]);
}

// Suma mocy wszystkich 5 kanałów dla podanych wartości PWM 10-bit
float getTotalLEDPower(const uint16_t pwm[5]) {
  float total = 0.0f;
  for (int i = 0; i < 5; i++) total += getPowerForChannel(i, pwm[i]);
  return total;
}

// ── FIX-FLASH3: Kapowanie celów soft-startu do limitu mocy zasilacza ──
// Wywołuj PO ustawieniu softStartTargetPwm[] przed aktywacją rampy.
// Zapobiega FLASH#3: bez tej korekty balancer wchodzi po zakończeniu
// [v88] PATCH-4: helper heap — jedno przejście listy zamiast dwóch (BM-15: 36µs → 30µs co 5s)
static inline void getHeapStats(uint32_t* pFree, uint32_t* pLargest) {
  multi_heap_info_t _hinfo;
  heap_caps_get_info(&_hinfo, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (pFree)    *pFree    = _hinfo.total_free_bytes;
  if (pLargest) *pLargest = _hinfo.largest_free_block;
}

// soft-startu ze skokiem (1.0 -> 0.758 jednym krokiem = widoczny błysk).
// Zwraca zastosowaną skalę (1.0 = brak ograniczenia, <1.0 = ograniczono).
static inline float capTargetsToPowerLimit(uint16_t targets[5]) {
  float pwr = getTotalLEDPower(targets);
  if (pwr <= (float)LED_MAX_POWER_W) return 1.0f;           // mieści się w limicie
  float scale = (float)LED_MAX_POWER_W / pwr;
  for (int i = 0; i < 5; i++)
    targets[i] = (uint16_t)((float)targets[i] * scale);
  return scale;
}

// ── Globalny współczynnik skalowania mocy (Power Balancer) ──
// Opada NATYCHMIAST gdy moc > limit (ochrona zasilacza)
// Wraca POWOLI gdy moc < limit (zapobiega migotaniu)
float gPowerScale     = 1.0f;   // 1.0 = pełna moc, <1.0 = ograniczona
float gPowerScaleLastLogged = 1.0f;  // do filtrowania logów

/*************************************************************
 *  WYJŚCIA STERUJĄCE
 *************************************************************/
const int pumpPin      = 18;  // S3: GPIO 26 nie wyprowadzony na tej płytce
const int cameraPin    = 41;  // S3: GPIO 33-37 = wewnętrzny PSRAM OPI
const int ledSupplyPin = 15;  // S3: GPIO 32 nie wyprowadzony na tej płytce

// [OK] FIX BUG-T: Globalne flagi statusu deratingu - eksponowane dla trybManual()
// aby nie nadpisywał kanałów zarządzanych przez aktywny derating termiczny.
bool derating1ActiveGlobal = false;  // Zone1: sekcja 2 (FS białe, tempPlate1)
bool derating2ActiveGlobal = false;  // Zone2: sekcje 0,1,3,4 (tempPlate2)
// [OK] FIX MINLUX-WINDOW: Flaga aktywnego okna świecenia (rano/wieczór)
// Ustawiana przez trybAuto() - blokuje applyMinLuxMode() gdy LED już świeci z harmonogramu
bool inLightingWindowGlobal = false;
// Flaga nocy - blokuje logowanie TSL gdy ciemno i nic nie działa
bool isNightGlobal = false;
// [FIX-v128-DOUBLE-CB] Global zamiast static lokalnej w DEBUG-block —
// pozwala handlerom (set-tryb, Firebase, TG) zsynchronizować stan i uniknąć
// podwójnego onTrybChange() (raz w handlerze, raz przez DEBUG-block loop()).
bool prevPowerGlobal = true;  // [v250-ARCH]
bool prevTrybGlobal = true;  // init=true jak oryginalna static (EEPROM zwykle AUTO)

// [OK] FIX LOGIC-E: Timer auto-wyłączenia kamery po 30 minutach
unsigned long cameraOnTime = 0;
bool minLuxModeEnabled = false; // Włącz/wyłącz funkcję
float minLuxDayTarget = 2000.0; // Docelowy LUX do utrzymania
// [OK] Transmisja szyby akwarium - czujnik TSL zamontowany PRZED szybą (8mm szkło ≈ 85%).
// Zmień tę wartość jeśli masz inną szybę lub inną geometrię montażu czujnika.
// Przykłady: 6mm=0.88, 8mm=0.85, 10mm=0.82, 12mm=0.80
const float GLASS_TRANSMITTANCE = 0.85f;
bool minLuxModeActive = false; // Stan - czy aktualnie uzupełnia światło
bool currentPumpState = false;  // Stan pompki (globalny do JSON)
unsigned long restartRequestedAt = 0;
// [OK] FIX BUG-H / ULT-C: Manual override pompki - blokuje updatePump() przez 30 minut
unsigned long pumpManualOverrideUntil = 0;
uint16_t minLuxCurrentPWM[5] = {0,0,0,0,0}; // Aktualne PWM w trybie MIN LUX
unsigned long lastMinLuxUpdate = 0; // Timestamp ostatniego update
uint16_t minLuxIntervalSec = 30;                             // [OK] WWW-edytowalny: interwał sprawdzania MIN LUX [s]
unsigned long minLuxUpdateInterval = 30000UL;                // przeliczony na ms (aktualizowany przy zmianie)

// EEPROM adresy
#define EEPROM_ADDR_MINLUX_ENABLED 74 // 1 bajt - ON/OFF
#define EEPROM_ADDR_MINLUX_TARGET 75 // 4 bajty - float

// Harmonogram pompki - dynamiczna tablica (max 6 przedziałów)
// Layout: 1 bajt count + 6*2*4 bajty (int start, int end) = 49 bajtów od addr 79
#define EEPROM_ADDR_PUMP_COUNT  79   // 1 bajt
#define EEPROM_ADDR_PUMP_SLOTS  80   // 6 * 2 * sizeof(int) = 48 bajtów
#define EEPROM_ADDR_KWH_PRICE  130  // 4 bajty - float, cena PLN/kWh
#define PUMP_MAX_SLOTS 6

struct PumpSlot { int start; int end; };

// [v258] CONFIG RUNTIME/STORAGE SPLIT + DURABLE PERSIST ACK
// Immutable snapshot parsowany na Core 0. Core 1 jedynie aplikuje runtime;
// persystencja wraca do Core 0 przez storageQueue.
static constexpr uint8_t FB_CFG_KEY_LEN = 64;
static constexpr uint8_t TG_TOKEN_SNAPSHOT_LEN = 64;
static constexpr uint8_t TG_CHAT_SNAPSHOT_LEN  = 40;
enum FirebaseConfigField : uint16_t {
  CFG_MWD=1u<<0, CFG_MWE=1u<<1, CFG_MIDOFF=1u<<2, CFG_EVENB=1u<<3, CFG_EVENOFF=1u<<4, CFG_FADE=1u<<5,
  CFG_SENSOR=1u<<6, CFG_ADAPT=1u<<7, CFG_LEARN=1u<<8,
  CFG_ML_ENABLED=1u<<9, CFG_ML_TARGET=1u<<10, CFG_ML_INTERVAL=1u<<11,
  CFG_KWH=1u<<12, CFG_RAMP=1u<<13, CFG_EMA=1u<<14, CFG_SENSINT=1u<<15
};

struct FirebaseConfigSnapshot {
  uint64_t cfgTs = 0;
  uint16_t fieldMask = 0;
  char firebaseKey[FB_CFG_KEY_LEN] = {0}; // niepuste tylko dla config_refresh
  char configType[16] = {0};

  bool hasSchedule = false;
  int32_t morningWD = 0;
  int32_t morningWE = 0;
  int32_t middayOff = 0;
  int32_t eveningBefore = 0;
  int32_t eveningOff = 0;
  int32_t fadeMinutesValue = 0;

  bool hasAdapt = false;
  uint8_t sensorMode = 0;
  bool adaptEnabled = false;
  bool learningEnabled = false;

  bool hasMinLux = false;
  bool minLuxEnabledValue = false;
  float minLuxTargetValue = 0.0f;
  uint16_t minLuxIntervalValue = 30;

  bool hasParams = false;
  float kwhPriceValue = 0.0f;
  uint16_t rampSecValue = 0;
  float emaFilterValue = 0.0f;
  uint16_t sensIntValue = 0;

  bool hasPump = false;
  uint8_t pumpCount = 0;
  PumpSlot pumpSlotsValue[PUMP_MAX_SLOTS] = {};

  bool hasTelegram = false;
  bool telegramEnabledPresent = false;
  bool telegramEnabledValue = false;
  char telegramToken[TG_TOKEN_SNAPSHOT_LEN] = {0};
  char telegramChatId[TG_CHAT_SNAPSHOT_LEN] = {0};
};

QueueHandle_t fbConfigApplyQueue = nullptr; // Core 0 -> Core 1
QueueHandle_t fbConfigPersistQueue = nullptr; // Core 1 -> Core 0, immutable snapshot for durable storage

struct FirebaseConfigAck {
  bool success = false;
  uint64_t cfgTs = 0;
  char firebaseKey[FB_CFG_KEY_LEN] = {0};
};
QueueHandle_t fbConfigAckQueue = nullptr; // Core 1 -> Core 0

struct FirebaseConfigPersistAck {
  bool success = false;
  uint64_t cfgTs = 0;
};
QueueHandle_t fbConfigPersistAckQueue = nullptr; // Core 0 -> Core 1

// [v257] STORAGE QUEUE — jedyny runtime owner EEPROM/LittleFS na Core 0.
static constexpr uint8_t EEPROM_MAX_PARTS = 13;
static constexpr uint8_t EEPROM_PART_BYTES = 16;
enum class StorageRequestType : uint8_t { EEPROM_BATCH, TELEGRAM_CONFIG };
struct EepromWritePart {
  uint16_t addr = 0;
  uint8_t size = 0;
  uint8_t data[EEPROM_PART_BYTES] = {0};
};
struct StorageRequest {
  StorageRequestType type = StorageRequestType::EEPROM_BATCH;
  uint8_t partCount = 0;
  EepromWritePart parts[EEPROM_MAX_PARTS];
  char tag[24] = {0};
  char tgToken[TG_TOKEN_SNAPSHOT_LEN] = {0};
  char tgChatId[TG_CHAT_SNAPSHOT_LEN] = {0};
  bool tgEnabled = false;
};
QueueHandle_t storageQueue = nullptr;
volatile bool g_eepromCommitRetryPending = false;
volatile bool g_storageLastEnqueueOk = true;

// [v258] durable config transaction state
static bool g_fbConfigPersistPending = false;        // Core 1 waits for Core 0 storage result
static FirebaseConfigSnapshot g_fbConfigPersistRetrySnapshot{}; // Core 0 retry buffer
static bool g_fbConfigPersistRetryPending = false;
static uint32_t g_fbConfigPersistRetryAtMs = 0;
static FirebaseConfigPersistAck g_fbConfigPersistAckRetry{};
static bool g_fbConfigPersistAckRetryPending = false;
static FirebaseConfigAck g_fbConfigFinalAckPending{};
static bool g_fbConfigFinalAckPendingValid = false;
static char g_fbConfigPendingFirebaseKey[FB_CFG_KEY_LEN] = {0};
static FirebaseConfigSnapshot g_fbConfigApplyRetrySnapshot{}; // Core 0 retry if Core1 apply queue is full
static bool g_fbConfigApplyRetryPending = false;
static uint32_t g_fbConfigApplyRetryAtMs = 0;
static uint64_t g_fbConfigQueuedTs = 0; // single config snapshot dedup high-water mark

 pumpSlots[PUMP_MAX_SLOTS] = {
  { 8*60, 11*60 },   // 08:00-11:00
  {15*60, 18*60 },   // 15:00-18:00
  {    0,     0 },
  {    0,     0 },
  {    0,     0 },
  {    0,     0 }
};
int pumpSlotCount = 2;

/*************************************************************
 *  TRYB AUTO / RAMPA
 *************************************************************/
bool autoRampInitialized = false;

// Soft-start po resecie - KROKOWY (jak rampa)
const int SOFTSTARTSECONDS = 30;
bool softStartActive = false;
uint16_t softStartPwm[5] = {0, 0, 0, 0, 0};
uint16_t softStartTargetPwm[5] = {0, 0, 0, 0, 0};  // CEL soft-startu (normalnie = backupAuto, mid-ramp = punkt rampy)
unsigned long softStartStepMillis[5] = {0, 0, 0, 0, 0};
unsigned long softStartIntervalMs[5] = {0, 0, 0, 0, 0};

// [OK] RAMPA PRZEJŚCIOWA przy zmianie jasności LED (10s)
const int TRANSITIONSECONDS = 3;
bool transitionActive = false;
uint16_t transitionCurrentPwm[5] = {0, 0, 0, 0, 0};
uint16_t transitionTargetPwm[5] = {0, 0, 0, 0, 0};
unsigned long transitionStepMillis[5] = {0, 0, 0, 0, 0};
unsigned long transitionIntervalMs[5] = {0, 0, 0, 0, 0};

/*************************************************************
// LICZNIK WERSJI NASTAW (lokalny, bez chmury)
 *************************************************************/
// [v33] cloudInitialSyncDone usunięty - Arduino IoT Cloud usunięty
// [v33] lastCloudReconnectMs / CLOUD_RESYNC_GUARD_MS usunięte
// [v33] cloudCounterReceived usunięty
uint32_t localChangeCounter = 0;   // wersja zapisana w EEPROM (lokalny licznik nastaw)
// [v33] int changeCounter usunięty - była to zmienna CloudVariable

/*************************************************************
 *  TRYB MANUAL / RAMPA
 *************************************************************/
// RAMPA MANUALNA - SOFT START 30s przy włączeniu
const int MANUALSOFTSTARTSECONDS = 30;
bool manualSoftStartActive = false;
uint16_t manualSoftStartPwm[5] = {0, 0, 0, 0, 0};
unsigned long manualSoftStartStepMillis[5] = {0, 0, 0, 0, 0};

// RAMPA MANUALNA - TRANSITION 10s przy zmianie jasności
const int MANUALTRANSITIONSECONDS = 3;
bool manualTransitionActive = false;
uint16_t manualTransitionCurrentPwm[5] = {0, 0, 0, 0, 0};
uint16_t manualTransitionTargetPwm[5] = {0, 0, 0, 0, 0};
unsigned long manualTransitionStepMillis[5] = {0, 0, 0, 0, 0};
unsigned long manualTransitionIntervalMs[5] = {0, 0, 0, 0, 0};

// ===== NOWY MECHANIZM RAMPY CO 1 PWM =====
static unsigned long lastPwmStepMillis[5] = {0, 0, 0, 0, 0};  // [OK] per kanał
static uint16_t currentRampPwm[5] = {0, 0, 0, 0, 0};
static bool rampInitialized[5] = {false, false, false, false, false};
// [v236] FIX-RAMPA-SUWAK-1: wartość suwaka przechwycona w /set-pwm, ZANIM cokolwiek
// w trybAuto() zdąży nadpisać backupBrightnessComposite między iteracjami — patrz
// Ryby_LED_S3_plan_fix_suwak_rampa.md. -1 = brak oczekującej wartości dla kanału.
static int16_t pendingManualRampStart[5] = {-1, -1, -1, -1, -1};
// [v237] FEATURE-MINLUX-MANUAL-LOCK: gdy ustawione, applyMinLuxMode() nie
// przelicza PWM od nowa — Twoja ręczna wartość z suwaka trzyma się aż do
// końca aktualnej przerwy (koniec = wejście w okno świecenia, rampa
// wieczorna/poranna). Ustawiane tylko gdy suwak poruszony PODCZAS aktywnego
// MIN LUX (rampaMinLuxPriorytet), nie przy innych rampach adaptacyjnych.
volatile bool minLuxManualOverrideUntilBreakEnd = false;
static unsigned long stepIntervalMs[5] = {0, 0, 0, 0, 0};
static uint16_t currentRampTarget[5] = {0, 0, 0, 0, 0};  // [OK] DODAJ TUTAJ!
static uint16_t rampDownStartValue[5] = {0, 0, 0, 0, 0};  // [OK] FIX #2: Wartość startowa rampy DOWN zapisana przy inicjalizacji

// [OK] FIX A: Globalny flag aktywnej rampy harmonogramu - widoczny z loop() dla applyMinLuxMode guard
bool rampScheduleActive = false;

// ── v252 RAMP ARBITER ───────────────────────────────────────────────────────
// Jeden właściciel zasobu PWM. Flagi legacy pozostają jako stan kompatybilności,
// ale nowe rampy nie mogą wystartować, gdy inna aktywna rampa już prowadzi PWM.
// Cel: wyeliminować kolizje soft-start + adaptacja + harmonogram + transition,
// które wcześniej były wykrywane dopiero przez DIAG-1.
enum RampOwner : uint8_t {
  RAMP_NONE = 0,
  RAMP_AUTO_SCHEDULE,
  RAMP_AUTO_ADAPTIVE,
  RAMP_AUTO_SOFTSTART,
  RAMP_AUTO_TRANSITION,
  RAMP_MANUAL_SOFTSTART,
  RAMP_MANUAL_TRANSITION
};

static RampOwner gRampOwner = RAMP_NONE;
static unsigned long gRampOwnerSinceMs = 0;
static uint32_t gRampArbiterBlocked = 0;
static uint32_t gRampArbiterPreempted = 0;

static const char* rampOwnerName(RampOwner o) {
  switch (o) {
    case RAMP_AUTO_SCHEDULE:      return "AUTO_SCHEDULE";
    case RAMP_AUTO_ADAPTIVE:      return "AUTO_ADAPTIVE";
    case RAMP_AUTO_SOFTSTART:     return "AUTO_SOFTSTART";
    case RAMP_AUTO_TRANSITION:    return "AUTO_TRANSITION";
    case RAMP_MANUAL_SOFTSTART:   return "MANUAL_SOFTSTART";
    case RAMP_MANUAL_TRANSITION:  return "MANUAL_TRANSITION";
    default:                      return "NONE";
  }
}

static bool rampArbiterAnyActive() {
  return softStartActive || transitionActive || manualSoftStartActive ||
         manualTransitionActive || rampaAdaptacyjnaAktywna || rampScheduleActive;
}

static RampOwner rampArbiterDetectOwner() {
  // Priorytet: użytkownik > auto soft-start > harmonogram > adaptacja.
  if (manualTransitionActive) return RAMP_MANUAL_TRANSITION;
  if (manualSoftStartActive) return RAMP_MANUAL_SOFTSTART;
  if (transitionActive) return RAMP_AUTO_TRANSITION;
  if (softStartActive) return RAMP_AUTO_SOFTSTART;
  if (rampScheduleActive) return RAMP_AUTO_SCHEDULE;
  if (rampaAdaptacyjnaAktywna) return RAMP_AUTO_ADAPTIVE;
  return RAMP_NONE;
}

static void rampArbiterCancelAutomation(RampOwner keep) {
  if (keep != RAMP_AUTO_SCHEDULE) {
    rampScheduleActive = false;
  }
  if (keep != RAMP_AUTO_ADAPTIVE) {
    rampaAdaptacyjnaAktywna = false;
    rampaMinLuxPriorytet = false;
  }
  if (keep != RAMP_AUTO_SOFTSTART) {
    softStartActive = false;
  }
  if (keep != RAMP_AUTO_TRANSITION) {
    transitionActive = false;
  }
  if (keep != RAMP_MANUAL_SOFTSTART) {
    manualSoftStartActive = false;
  }
  if (keep != RAMP_MANUAL_TRANSITION) {
    manualTransitionActive = false;
  }
}

// Start rampy. force=true tylko dla bezpośredniego polecenia użytkownika / zmiany trybu:
// taki rozkaz może przerwać automatyczną rampę. Automaty same sobie nie mogą się wywłaszczać.
static bool rampArbiterTryStart(RampOwner wanted, bool force = false) {
  RampOwner current = rampArbiterDetectOwner();
  if (current == wanted) {
    gRampOwner = wanted;
    if (gRampOwnerSinceMs == 0) gRampOwnerSinceMs = millis();
    return true;
  }
  if (current != RAMP_NONE) {
    if (!force) {
      ++gRampArbiterBlocked;
      if (Komentarze) {
        logPrintf("lvl=WARN tag=RAMP-ARB msg=\"START_BLOCK\" wanted=%s owner=%s blocked=%lu\n",
                  rampOwnerName(wanted), rampOwnerName(current),
                  (unsigned long)gRampArbiterBlocked);
      }
      return false;
    }
    ++gRampArbiterPreempted;
    rampArbiterCancelAutomation(wanted);
    if (Komentarze) {
      logPrintf("lvl=WARN tag=RAMP-ARB msg=\"PREEMPT\" old=%s new=%s count=%lu\n",
                rampOwnerName(current), rampOwnerName(wanted),
                (unsigned long)gRampArbiterPreempted);
    }
  }
  gRampOwner = wanted;
  gRampOwnerSinceMs = millis();
  if (Komentarze) logPrintf("lvl=INFO tag=RAMP-ARB event=acquire owner=%s\n", rampOwnerName(wanted));
  return true;
}

static void rampArbiterRelease(RampOwner owner) {
  if (gRampOwner == owner) {
    gRampOwner = RAMP_NONE;
    gRampOwnerSinceMs = 0;
  }
}

static void rampArbiterReconcile() {
  RampOwner detected = rampArbiterDetectOwner();
  if (detected == RAMP_NONE) {
    gRampOwner = RAMP_NONE;
    gRampOwnerSinceMs = 0;
    return;
  }
  RampOwner previous = gRampOwner;
  if (previous != detected) {
    gRampOwner = detected;
    gRampOwnerSinceMs = millis();
  }

  int active = (int)softStartActive + (int)transitionActive +
               (int)manualSoftStartActive + (int)manualTransitionActive +
               (int)rampaAdaptacyjnaAktywna + (int)rampScheduleActive;
  if (active > 1) {
    // Zachowaj najwyższy priorytet i natychmiast zatrzymaj pozostałe źródła.
    rampArbiterCancelAutomation(detected);
    if (Komentarze) {
      logPrintf("lvl=ERR tag=RAMP-ARB msg=\"COLLISION_RESOLVED\" owner=%s active=%d\n",
                rampOwnerName(detected), active);
    }
  }
}

// ── Ramp widget tracking (dla /api/status JSON) ──
unsigned long rampWidgetStartMs   = 0;
uint16_t      rampWidgetPwmStart  = 0;
uint16_t      rampWidgetPwmTarget = 0;
bool          rampWidgetUp        = true;
bool          rampWidgetWasActive = false;
int32_t       rampWidgetRampStartMin = -1;  // minut harmonogramu początku aktywnej rampy
int32_t       lastNowMin = -1;              // bieżąca minuta dnia (aktualizowana przez trybAuto)


/*************************************************************
 *  STANY SYSTEMU
 *************************************************************/
int  lastSections   = 0;
/*************************************************************
 *  PODGLĄD JASNOŚCI (SLIDER)
 *************************************************************/
uint16_t previewBrightness10bit = 0;
bool brightnessPreviewActive = false;
/*************************************************************
 *  HARMONOGRAM (JEDNO ŹRÓDŁO PRAWDY)
 *************************************************************/
// [OK] FIX #4: int32_t zamiast int - gwarantuje 4 bajty na każdej platformie (zgodność z EEPROM.put/get)
int32_t MORNING_ON_START_WEEKDAY = 2 * 60 + 0;
int32_t MORNING_ON_START_WEEKEND = 8 * 60;
int32_t MIDDAY_OFF_LOCAL         = 2 * 60 + 35;
int32_t EVENING_ON_BEFORE_SUNSET_MIN = 0;
int32_t EVENING_OFF_START        = 21 * 60;


// Rampa - sterowana z aplikacji IoT
int32_t fadeMinutes = 30;


/*************************************************************
 *  ZACHÓD SŁOŃCA
 *************************************************************/
// minuty od północy
int sunsetMinutes = 1140; // 19:00 domyślnie
const double latitude  = 52.1345;
const double longitude = 20.1418;

/*************************************************************
 *  TERMICZNY DERATING
 *************************************************************/
const float TEMP_FULL_POWER      = 40.0;
const float TEMP_START_DERATING  = 45.0;
const float TEMP_ZERO_POWER      = 55.0;
const float HYSTERESIS           = 3.0;
const float FILTER_ALPHA         = 0.8;

static unsigned long lastTempCheck = 0;
const unsigned long tempInterval = 1500;

/*************************************************************
 *  BACKUP JASNOŚCI
 *************************************************************/
// 5 kanałów × 10 bit -> pakowane w uint64_t
uint64_t backupBrightnessComposite       = 0;
uint64_t backupManualBrightnessComposite = 0;
uint64_t backupAutoBrightnessComposite   = 0;

/*************************************************************
 *  DEBUG
 *************************************************************/
bool Komentarze = true;
bool trybAutoTrace = false;  // [OK] FIX-v27: osobna flaga dla trace trybAuto() - NIE łączyć z Komentarze (100 linii/s -> przepełnienie LittleFS)
// ─── FIREBASE-v1: zmienne czasowe ────────────────────────────────────────────
unsigned long lastFirebaseSend  = 0;
unsigned long lastFirebaseCmd   = 0;
uint64_t lastFirebaseCmdTs = 0;
String lastFirebaseCmdKey = "";
uint64_t lastFirebaseCfgTs = 0;
static constexpr const char* FB_STATE_NVS_NS = "fb_state";
static constexpr const char* FB_STATE_KEY_CMD = "cmd_ts";
static constexpr const char* FB_STATE_KEY_CMD_KEY = "cmd_key";
static constexpr const char* FB_STATE_KEY_CFG = "cfg_ts";

static void fbLoadPersistentState() {
  Preferences prefs;
  if (!prefs.begin(FB_STATE_NVS_NS, true)) return;
  lastFirebaseCmdTs = prefs.getULong64(FB_STATE_KEY_CMD, 0);
  lastFirebaseCmdKey = prefs.getString(FB_STATE_KEY_CMD_KEY, "");
  lastFirebaseCfgTs = prefs.getULong64(FB_STATE_KEY_CFG, 0);
  prefs.end();
}

static void fbSaveCmdTs(uint64_t ts) {
  Preferences prefs;
  if (!prefs.begin(FB_STATE_NVS_NS, false)) return;
  (void)prefs.putULong64(FB_STATE_KEY_CMD, ts);
  prefs.end();
}
static void fbSaveCmdKey(const String& key) {
  Preferences prefs;
  if (!prefs.begin(FB_STATE_NVS_NS, false)) return;
  (void)prefs.putString(FB_STATE_KEY_CMD_KEY, key);
  prefs.end();
}

static void fbSaveCfgTs(uint64_t ts) {
  Preferences prefs;
  if (!prefs.begin(FB_STATE_NVS_NS, false)) return;
  (void)prefs.putULong64(FB_STATE_KEY_CFG, ts);
  prefs.end();
}
const unsigned long FIREBASE_SEND_INTERVAL = 60000;   // status co 60s  [v66: benchmark strategia 6 — 4.0 TLS/min, blok=6.3%]
const unsigned long FIREBASE_CMD_INTERVAL  = 14000;   // komendy co 14s [v66: benchmark strategia 6]

// [v240] FIREBASE TURBO: po poprawnej, nowej komendzie aktywuj chwilowo
// częstszy polling. Zapis od Core 0, odczyt z Core 1 — używamy operacji atomowych.
// Normalnie: 60s status / 14s kolejka. Turbo: 5s status / 1s kolejka; scheduler wykonuje tylko jeden request Firebase naraz. Każda nowa poprawna komenda
// przedłuża okno o kolejne 60s. Błędny payload/token nie może uruchomić turbo.
volatile uint32_t fbTurboUntilMs = 0;
static constexpr uint32_t FB_TURBO_WINDOW_MS = 60000UL;
static constexpr uint32_t FB_TURBO_SEND_INTERVAL_MS = 5000UL;
static constexpr uint32_t FB_TURBO_CMD_INTERVAL_MS  = 1000UL;

static bool fbTurboAktywne() {
  const uint32_t until = __atomic_load_n(&fbTurboUntilMs, __ATOMIC_ACQUIRE);
  if (until == 0) return false;
  const uint32_t now = millis();
  if ((int32_t)(now - until) < 0) return true;
  __atomic_store_n(&fbTurboUntilMs, 0U, __ATOMIC_RELEASE);
  return false;
}

static void fbTurboAktywuj(const char* cmd) {
  const uint32_t until = millis() + FB_TURBO_WINDOW_MS;
  __atomic_store_n(&fbTurboUntilMs, until, __ATOMIC_RELEASE);
  if (Komentarze) {
    logPrintfNoFile("lvl=INFO tag=FB-TURBO event=activate cmd=%s windowMs=%lu\n",
                   cmd ? cmd : "?", (unsigned long)FB_TURBO_WINDOW_MS);
  }
}
// ─── FIREBASE-v3: konfiguracja z panelu ──────────────────────────────────────
unsigned long lastFirebaseConfig   = 0;
const unsigned long FIREBASE_CFG_INTERVAL = 120000; // sprawdzaj co 120s [v66: benchmark strategia 6]
// ─────────────────────────────────────────────────────────────────────────────

/*************************************************************
 * CZUJNIKI ŚWIATŁA TSL2561
*************************************************************/
Adafruit_TSL2561_Unified czujnikPokojowy = 
  Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
  
Adafruit_TSL2561_Unified czujnikNadWoda = 
  Adafruit_TSL2561_Unified(TSL2561_ADDR_LOW, 12346);

// [FIX-BUG3] volatile: luxPokojowy/luxNadWoda pisane z Core 1 (loop/odczytajSwiatloZFiltrem),
// czytane z Core 0 (tgTaskFn/sendStatusToFirebase). Brak volatile = C++ data race (UB).
// Na Xtensa LX7 float 4B wyrównany jest atomowy (L32I/S32I), ale volatile gwarantuje
// że kompilator nie zakeszuje wartości w rejestrze między wywołaniami cross-core.
volatile float luxPokojowy = 0;
volatile float luxNadWoda = 0;
float wygladzonyLuxPokojowy = 0;
unsigned long ostatniOdczytSwiatla = 0;
uint16_t sensIntSec = 30;                                    // [OK] WWW-edytowalny: interwał odczytu TSL2561 [s]
unsigned long intervalOdczytu = (unsigned long)sensIntSec * 1000UL;  // przeliczony na ms
float emaFilterAlpha = 0.25f;  // [OK] Współczynnik filtra EMA (0.05=bardzo wolny ... 0.5=szybki). Domyślnie 0.25.

/*************************************************************
 * REGULACJA ADAPTACYJNA - STEROWANIE
*************************************************************/
bool regulacjaAdaptacyjnaWlaczona = false;
bool uzywajCzujnikaPokojowego = false;
bool uzywajCzujnikaNadWoda = false;
bool uczenieSieWlaczone = false;

bool czujnikPokojowyAktywny = false;
bool czujnikNadWodaAktywny = false;

// ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
// [SIM] SYMULACJA CZUJNIKA ŚWIATŁA
// Pozwala testować logikę MIN LUX i adaptacji bez fizycznego
// czujnika TSL2561. Sterowanie przez terminal WebSocket:
//   sim lux on          - włącz symulację (stała wartość)
//   sim lux off         - wyłącz, wróć do hardware
//   sim lux auto        - włącz tryb automatyczny (sinusoida)
//   sim lux [wartość]   - ustaw stałą wartość lux (0-50000)
//   sim lux status      - pokaż aktualny stan
// ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
// [SIM] Czy symulacja jest aktywna (zastępuje hardware)
bool simLuxEnabled = false;
// [SIM] Stała wartość lux używana gdy simLuxAuto=false
float simLuxValue = 300.0f;
// [SIM] Tryb automatyczny: sinusoida między simLuxMin a simLuxMax
bool simLuxAuto = false;
// [SIM] Zakres sinusoidy w trybie auto
float simLuxAutoMin = 0.0f;
float simLuxAutoMax = 1500.0f;
// [SIM] Okres pełnego cyklu sinusoidy w ms (domyślnie 10 minut)
unsigned long simLuxAutoPeriodMs = 600000UL;
// [SIM] Wygładzona wartość wysyłana do reszty systemu
float simLuxSmoothed = 0.0f;

/*************************************************************
 * PARAMETRY BIOLOGICZNE
*************************************************************/
float minLuxDlaRoslin = 2000;
float maxBezpiecznyLux = 12000;

const float maxLimitLED = 1.2;

/*************************************************************
 * UCZENIE SIĘ
*************************************************************/
struct DaneAdaptacyjne {
  float sredniaLuxDzien = 500;
  uint32_t liczbaProbek = 0;
  float nauczonaTransmisja = 0.30;
} adaptacja;

const uint32_t minProbek = 200;
const uint32_t optymalneProbki = 1000;
const int godzinaStartUczenia = 8;
const int godzinaKoniecUczenia = 18;

/*************************************************************
 * RAMPA ADAPTACYJNA (30s)
*************************************************************/
uint16_t rampSec = 30;                          // [OK] WWW-edytowalny: czas rampy PWM [s]
#define ADAPTIVE_RAMP_SECONDS rampSec           // makro - reszta kodu używa tej nazwy bez zmian
const uint16_t MIN_ZMIANA_PWM = 8;  // [OK] FIX-v23: podwyższono z 3 -> 8; wartość 3 powodowała uruchamianie rampy adaptacyjnej co 30s przy stałym PWM (harm=262 adapt=258, Δ=4>3) -> TRYBAUT fałszywy spam wieczorami
bool rampaAdaptacyjnaAktywna = false;
uint16_t rampaAdaptacyjnaCel[5] = {0,0,0,0,0};
uint16_t rampaAdaptacyjnaAktualne[5] = {0,0,0,0,0};
unsigned long rampaAdaptacyjnaKrok[5] = {0,0,0,0,0};
unsigned long rampaAdaptacyjnaInterval[5] = {0,0,0,0,0};

// Flaga priorytetu - true gdy rampę uruchomił applyMinLuxMode().
// Chroni przed nadpisaniem tablic rampy przez zastosujRampeAdaptacyjna() z trybAuto().
bool rampaMinLuxPriorytet = false;

/*************************************************************
 * STATYSTYKI
*************************************************************/
struct StatystykiDzienne {
  uint32_t korektyMinimum = 0;
  uint32_t korektyMaksimum = 0;
  float zaoszczedzonaEnergia = 0;
  float sredniaRedukcja = 0;
  uint32_t liczbaPomiarow = 0;
} statystyki;

// ── Tracking energii, czasu LED, lux-godzin, MIN LUX ──
float     energyTodayWh      = 0.0f;   // Wh dziś (reset o północy)
float     energyWeekWh       = 0.0f;   // Wh ten tydzień (reset w poniedziałek)
float     energyMonthWh      = 0.0f;   // Wh ten miesiąc (reset 1. dnia miesiąca)
// [OK] FIX-v29-11-DAYHISTORY: Tablica realnych wartości per dzień tygodnia (Pn=0..Nd=6)
// Wypełniana przy resecie dnia (o północy), przechowuje energię zamkniętych dni.
// Bieżący dzień (isoToday) = energyTodayWh (live). Pozostałe = z tablicy.
float     energyDayHistory[7] = {0,0,0,0,0,0,0};
// [OK] FIX-v29-11-WEEKHISTORY: Tablica realnych wartości per tydzień w miesiącu (max 5 tygodni).
// Indeks = numer tygodnia miesiąca (0-bazowany). Bieżący tydzień = energyWeekWh (live).
float     energyWeekHistory[5] = {0,0,0,0,0};
uint32_t  ledOnMinutesToday  = 0;      // minuty pracy LED dziś
uint32_t  ledOnMinutesWeek   = 0;      // minuty pracy LED ten tydzień
uint32_t  ledOnMinutesMonth  = 0;      // minuty pracy LED ten miesiąc
float     luxHoursTodayWater = 0.0f;   // lux-godziny (woda) dziś
uint32_t  minLuxActivToday   = 0;      // aktywacje MIN LUX dziś
float     peakPowerWToday    = 0.0f;   // szczytowa moc dziś [W]
float     powerHourWh[24]   = {0.0f}; // energia Wh per godzina dziś (do wykresu)
uint32_t  minLuxActivWeek    = 0;      // aktywacje MIN LUX w tygodniu
float     kwhPrice           = 0.80f;  // PLN/kWh (z EEPROM)
static bool _minLuxWasActive = false;  // do detekcji krawędzi aktywacji

unsigned long ostatniZapisAdaptacji = 0;
const unsigned long intervalZapisuAdaptacji = 3600000; // 1h

// ── DEBUG: globalne zmienne śledzenia rampy adaptacyjnej ──
const char* _ostatniWywolujacyRampe = "?";
unsigned long _ostatniStartRampy = 0;

// Forward declarations - funkcje pomocnicze (zdefiniowane po miejscach użycia)
uint16_t getBrightnessForSection(uint64_t composite, int section);
bool     getLocalTimePL(struct tm *ti);
bool     isWeekend();
int      getLocalMinutes();
float    szacujTransmisje(float luxPokojowy);
uint16_t luxToPwm(float lux);
void     bumpChangeCounter();

/*************************************************************
 * HELPER - aktualny czas PL jako string "HH:MM:SS"
 * Używaj wszędzie zamiast ręcznego snprintf
*************************************************************/
String logTime() {
  struct tm ti;
  if (!getLocalTimePL(&ti)) return String("[??:??:??]");
  char buf[12];
  snprintf(buf, sizeof(buf), "[%02d:%02d:%02d]", ti.tm_hour, ti.tm_min, ti.tm_sec);
  return String(buf);
}

/*************************************************************
 * PODSUMOWANIE DNIA - wywoływane raz na dobę o północy
*************************************************************/
// lastDailySummary usunięty - logDailySummary() używa teraz prawdziwego zegara (BUG-O fix)

// Forward declarations
void saveEnergyStats();
void loadEnergyStats();
void saveAdaptStats();
void loadAdaptStats();
void saveHistoryPoint();  // [OK] FIX-v33g: forward decl (tgTaskFn wywołuje przed definicją)
void applyEnergyRollover();  // [FIX race-audit 2026-08-13] forward decl (tgTaskFn wywołuje przed definicją)
void logPrint(const String &msg);
void logPrintln(const String &msg);
void logPrint(const char* msg); // [OK] OPT-v44: bez alokacji String dla literałów
void logPrintln(const char* msg); // [OK] OPT-v44: bez alokacji String dla literałów
void logPrintf(const char* format, ...);
void logPrintfNoFile(const char* format, ...);  // [v230]
void logForceFlush();
void logFlushCore1();
void savePowerTrybToEEPROM();

// Forward declarations - moduł diagnostyczny
void runDiagnostics();
void diagTslFrozenCheck();
void diagManualTransitionStuck();
void diagHeap();
// [v42] forward declarations - funkcje Firebase zdefiniowane po tgTaskFn
void sendStatusToFirebase();
void checkFirebaseCommands();
void checkFirebaseConfig();
void diagAutoBackupZero();
void diagSunsetRange();
void diagMinLuxOscillation();
void diagNtpTimeout();
void diagPwmRange();
void diagCameraTimeout();
void diagScheduleSanity();
void diagHealthReport();
void diagDeratingDuration();
void diagWaterTemp();
void diagTransmissionSanity();
void diagSensorSwapped();
void diagLearningStuck();
void fsSizeGuard();

// ─────────────────────────────────────────────────────────────
// TELEGRAM - wczytaj konfigurację z LittleFS
// ─────────────────────────────────────────────────────────────
void loadTelegramConfig() {
  // [v116] FIX-TG-PARTITION-WIPE: śledź czy dane z pliku były kompletne
  bool fileLoaded = false;

  if (littlefsReady && LittleFS.exists(TG_CONFIG_FILE)) {
    File f = LittleFS.open(TG_CONFIG_FILE, "r");
    if (f) {
      String json = f.readString();
      f.close();
      auto extractStr = [&](const char* key) -> String {
        String k = String("\"") + key + "\":\"";
        int p = json.indexOf(k);
        if (p < 0) return "";
        p += k.length();
        int e = json.indexOf("\"", p);
        return (e < 0) ? "" : json.substring(p, e);
      };
      tgBotToken = extractStr("token");
      tgChatId   = extractStr("chatId");
      String enStr = extractStr("enabled");
      tgEnabled  = (enStr == "1");
      // Plik istnieje i token wczytany poprawnie
      if (tgBotToken.length() >= 10) fileLoaded = true;
    }
  }

  // [v116] FIX-TG-PARTITION-WIPE: gdy brak pliku LUB token pusty (np. po zmianie partycji /
  //   formatowaniu LittleFS) — załaduj hardkodowane wartości domyślne i zapisz plik.
  //   Kolejna zmiana partycji ponownie uruchomi ten mechanizm.
  //   Warunek: DEFAULT_TG_BOT_TOKEN musi być wypełniony (strlen >= 10).
  if (!fileLoaded) {
    constexpr size_t defTokenLen = sizeof(DEFAULT_TG_BOT_TOKEN) - 1;
    if (defTokenLen >= 10) {
      tgBotToken = DEFAULT_TG_BOT_TOKEN;
      tgChatId   = DEFAULT_TG_CHAT_ID;
      tgEnabled  = DEFAULT_TG_ENABLED;
      saveTelegramConfig();  // utwórz/odtwórz plik z domyślnymi wartościami
      logPrintln("lvl=INFO tag=TG-CFG msg=\"Brak pliku konfiguracji TG, zaladowano DEFAULT i zapisano do LittleFS\"");
    } else {
      logPrintln("lvl=WARN tag=TG-CFG msg=\"Brak pliku TG i pusty DEFAULT_TG_BOT_TOKEN, TG pozostaje wylaczony\"");
      logPrintln("lvl=INFO tag=TG-CFG msg=\"Skonfiguruj przez panel WWW lub wypelnij DEFAULT_TG_* w kodzie zrodlowym\"");
    }
  }
}

// ─────────────────────────────────────────────────────────────
// TELEGRAM - zapisz konfigurację do LittleFS
// ─────────────────────────────────────────────────────────────
void saveTelegramConfig() {
  if (!littlefsReady) return;
  if (!storageQueue) {
    // setup() fallback: przed startem worker'a nie ma jeszcze ścieżki runtime.
    esp_task_wdt_reset();
    File f = LittleFS.open(TG_CONFIG_FILE, "w");
    if (!f) return;
    f.printf("{\"token\":\"%s\",\"chatId\":\"%s\",\"enabled\":\"%s\"}", tgBotToken.c_str(), tgChatId.c_str(), tgEnabled ? "1" : "0");
    f.close();
    esp_task_wdt_reset();
    return;
  }
  StorageRequest req; req.type = StorageRequestType::TELEGRAM_CONFIG;
  strncpy(req.tgToken, tgBotToken.c_str(), sizeof(req.tgToken)-1);
  strncpy(req.tgChatId, tgChatId.c_str(), sizeof(req.tgChatId)-1);
  req.tgEnabled = tgEnabled;
  strncpy(req.tag, "telegram_config", sizeof(req.tag)-1);
  if (xQueueSend(storageQueue, &req, 0) != pdPASS) {
    logPrintf("lvl=ERR tag=STORAGE state=TELEGRAM_QUEUE_FULL\n");
    return;
  }
  logPrintfNoFile("lvl=INFO tag=STORAGE state=TELEGRAM_QUEUED\n");
}

// ─────────────────────────────────────────────────────────────
// [v139] WIFI-MULTI - wczytaj listę sieci z LittleFS
// Jeśli plik nie istnieje (pierwsze uruchomienie / świeży LittleFS),
// inicjalizuje listę jednym wpisem = dotychczasowy SECRET_SSID/PASS,
// żeby nic nie przestało działać bez ręcznej konfiguracji.
// ─────────────────────────────────────────────────────────────
void loadWifiNetworks() {
  wifiNetworks.clear();
  if (littlefsReady && LittleFS.exists(WIFI_LIST_FILE)) {
    File f = LittleFS.open(WIFI_LIST_FILE, "r");
    if (f) {
      while (f.available() && wifiNetworks.size() < WIFI_MAX_NETWORKS) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;
        auto extractStr = [&](const char* key) -> String {
          String k = String("\"") + key + "\":\"";
          int p = line.indexOf(k);
          if (p < 0) return "";
          p += k.length();
          int e = line.indexOf("\"", p);
          return (e < 0) ? "" : line.substring(p, e);
        };
        String s = extractStr("ssid");
        String p = extractStr("pass");
        if (s.length() > 0) wifiNetworks.push_back({s, p});
      }
      f.close();
    }
  }

  if (wifiNetworks.empty()) {
    // Brak pliku / pusta lista -> kompatybilność wsteczna z SECRET_SSID
    wifiNetworks.push_back({String(SECRET_SSID), String(SECRET_OPTIONAL_PASS)});
    saveWifiNetworks();
    logPrintln("lvl=INFO tag=WIFI-CFG msg=\"Brak /wifi_list.txt, utworzono z domyslna siecia SECRET_SSID\"");
  } else {
    logPrintf("lvl=INFO tag=WIFI-CFG msg=\"Wczytano skonfigurowane sieci WiFi z LittleFS\" liczba=%d\n",
              (int)wifiNetworks.size());
  }
}

// ─────────────────────────────────────────────────────────────
// [v139] WIFI-MULTI - zapisz listę sieci do LittleFS (1 JSON/linia)
// [v140] FIX-WIFI-ATOMIC-SAVE: zapis atomowy przez plik tymczasowy + rename,
//   ten sam wzorzec co HIST-COMPACT (linia ~9283). Bez tego, reset/utrata
//   zasilania DOKŁADNIE w trakcie f.printf() (tryb "w" od razu kasuje starą
//   zawartość) zostawiał plik pusty/uciety -> utrata całej listy sieci.
//   Teraz: stary plik zostaje NIETKNIĘTY, dopóki nowy nie jest w 100%
//   zapisany i zamknięty - dopiero wtedy następuje atomowa podmiana.
// ─────────────────────────────────────────────────────────────
bool saveWifiNetworks() {
  if (!littlefsReady) return false;

  // [v227] FIX-WDT-DAILYSUM (Partia 3): zapis atomowy = do 2 rename()/remove()
  // + pętla zapisu, zero esp_task_wdt_reset() w całej funkcji - domykamy,
  // wzorzec jak w bloku histSavePending (~linia 12848).
  esp_task_wdt_reset();   // [v227] reset przed otwarciem pliku tymczasowego

  File fTmp = LittleFS.open(WIFI_LIST_TMP_FILE, "w");
  if (!fTmp) return false;

  bool writeOk = true;
  for (auto &n : wifiNetworks) {
    size_t written = fTmp.printf("{\"ssid\":\"%s\",\"pass\":\"%s\"}\n",
                                  n.ssid.c_str(), n.pass.c_str());
    if (written == 0 && n.ssid.length() > 0) { writeOk = false; break; }  // np. brak miejsca na FS
  }
  fTmp.close();

  if (!writeOk) {
    LittleFS.remove(WIFI_LIST_TMP_FILE);
    logPrintln("lvl=ERR tag=WIFI-CFG msg=\"Blad zapisu pliku tymczasowego, stara lista zachowana\"");
    return false;
  }

  esp_task_wdt_reset();   // [v227] reset przed operacjami rename/remove (podmiana atomowa)

  // Atomowa podmiana: stary plik -> backup, tmp -> docelowy.
  // Jeśli cokolwiek pójdzie źle w trakcie, oryginał wraca na miejsce.
  bool hadOld = LittleFS.exists(WIFI_LIST_FILE);
  if (hadOld) LittleFS.rename(WIFI_LIST_FILE, WIFI_LIST_OLD_FILE);

  if (LittleFS.rename(WIFI_LIST_TMP_FILE, WIFI_LIST_FILE)) {
    if (hadOld) LittleFS.remove(WIFI_LIST_OLD_FILE);
    esp_task_wdt_reset();   // [v227] reset po całości bloku
    return true;
  } else {
    // Rename się nie powiódł (bardzo rzadkie) - przywróć poprzednią, działającą listę.
    if (hadOld) LittleFS.rename(WIFI_LIST_OLD_FILE, WIFI_LIST_FILE);
    LittleFS.remove(WIFI_LIST_TMP_FILE);
    logPrintln("lvl=ERR tag=WIFI-CFG msg=\"Blad podmiany pliku, przywrocono poprzednia liste sieci\"");
    return false;
  }
}

// Dodaj nową sieć lub zaktualizuj hasło, jeśli SSID już istnieje na liście.
bool wifiAddNetwork(const String& ssid, const String& pass) {
  if (ssid.length() == 0) return false;
  for (auto &n : wifiNetworks) {
    if (n.ssid == ssid) { n.pass = pass; return saveWifiNetworks(); }
  }
  if (wifiNetworks.size() >= WIFI_MAX_NETWORKS) return false;  // limit osiągnięty
  wifiNetworks.push_back({ssid, pass});
  return saveWifiNetworks();
}

// Usuń sieć po SSID. Zawsze zostawia min. 1 wpis na liście (żeby urządzenie
// nigdy nie zostało bez żadnej skonfigurowanej sieci).
bool wifiDeleteNetwork(const String& ssid) {
  if (wifiNetworks.size() <= 1) return false;
  for (size_t i = 0; i < wifiNetworks.size(); i++) {
    if (wifiNetworks[i].ssid == ssid) {
      wifiNetworks.erase(wifiNetworks.begin() + i);
      return saveWifiNetworks();
    }
  }
  return false;
}

// Skanuje eter i wybiera, spośród ZAPISANYCH sieci, tę widoczną o
// najsilniejszym RSSI. Zwraca indeks w wifiNetworks, albo -1 jeśli żadna
// zapisana sieć nie została znaleziona w skanie (np. ukryty SSID / słaby
// zasięg) - wtedy wołający powinien spróbować po prostu pierwszej z listy.
// UWAGA WDT: WiFi.scanNetworks() jest wywołaniem BLOKUJĄCYM (zwykle 1-4s).
// Używać TYLKO tam, gdzie taki jednorazowy blok jest akceptowalny (setup()).
// NIE wywoływać z hot-pathów w loop() (hard-reconnect / DIAG-14) - tam
// obowiązuje zasada <5ms ustalona w FIX-v56-WDT-RECONNECT.
int wifiPickBestNetwork() {
  if (wifiNetworks.empty()) return -1;
  if (wifiNetworks.size() == 1) return 0;  // jedna sieć -> nie ma co skanować

  esp_task_wdt_reset();           // margines przed blokującym skanem
  int n = WiFi.scanNetworks();
  esp_task_wdt_reset();           // margines po blokującym skanie
  if (n <= 0) { WiFi.scanDelete(); return -1; }

  int bestIdx = -1;
  int32_t bestRssi = -1000;
  for (int i = 0; i < n; i++) {
    String foundSsid = WiFi.SSID(i);
    for (size_t j = 0; j < wifiNetworks.size(); j++) {
      if (wifiNetworks[j].ssid == foundSsid && WiFi.RSSI(i) > bestRssi) {
        bestRssi = WiFi.RSSI(i);
        bestIdx = (int)j;
      }
    }
  }
  WiFi.scanDelete();
  return bestIdx;
}

// [v139] Zastępuje dotychczasowe WiFi.begin(SECRET_SSID, SECRET_OPTIONAL_PASS).
// Wybiera zapisaną sieć i wywołuje na niej WiFi.begin(). WiFi.begin() samo w
// sobie jest ASYNCHRONICZNE (nie blokuje) - identycznie jak poprzednio.
//
// allowScan=true  (domyślnie, użyj w setup()): jednorazowy skan eteru i wybór
//   najsilniejszej widocznej, znanej sieci. Blokujące ~1-4s, akceptowalne
//   przy starcie urządzenia.
// allowScan=false (użyj w hot-pathach loop(): hard-reconnect, DIAG-14):
//   BEZ skanowania (zero ryzyka WDT) - jeśli jest >1 znana sieć, przełącza
//   się cyklicznie (round-robin) na kolejną z listy przy każdym wywołaniu,
//   żeby z czasem wypróbować wszystkie skonfigurowane sieci bez blokowania
//   loop() wielosekundowym skanem.
void wifiBeginBest(bool allowScan) {
  if (wifiNetworks.empty()) {
    WiFi.begin(SECRET_SSID, SECRET_OPTIONAL_PASS);  // absolutny fallback
    return;
  }

  int idx = 0;
  if (wifiNetworks.size() > 1) {
    if (allowScan) {
      int picked = wifiPickBestNetwork();
      idx = (picked >= 0) ? picked : 0;
    } else {
      static size_t _rrIdx = 0;   // stan round-robin dla hot-pathów bez skanu
      _rrIdx = (_rrIdx + 1) % wifiNetworks.size();
      idx = (int)_rrIdx;
    }
  }

  logPrintf("lvl=INFO tag=WiFi msg=\"Proba polaczenia\" ssid=%s idx=%d/%d\n",
            wifiNetworks[idx].ssid.c_str(), idx + 1, (int)wifiNetworks.size());
  WiFi.begin(wifiNetworks[idx].ssid.c_str(), wifiNetworks[idx].pass.c_str());
}

// Buduje JSON z listą zapisanych sieci (BEZ haseł) + info o aktualnym
// połączeniu, do użytku w GET /api/wifi/list.
String wifiNetworksListJson() {
  String cur = (WiFi.status() == WL_CONNECTED) ? WiFi.SSID() : String("");
  String json = "{\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false");
  json += ",\"currentSsid\":\"" + cur + "\",\"networks\":[";
  for (size_t i = 0; i < wifiNetworks.size(); i++) {
    if (i > 0) json += ",";
    json += "{\"ssid\":\"" + wifiNetworks[i].ssid + "\"";
    json += ",\"hasPass\":" + String(wifiNetworks[i].pass.length() > 0 ? "true" : "false");
    bool isActive = (wifiNetworks[i].ssid == cur);
    json += ",\"active\":" + String(isActive ? "true" : "false");
    // [v144] RSSI aktualnego połączenia - jedyna zapisana sieć, dla której
    // urządzenie NA PEWNO zna siłę sygnału w danej chwili (bez skanowania).
    if (isActive && WiFi.status() == WL_CONNECTED) {
      json += ",\"rssi\":" + String(WiFi.RSSI());
    }
    json += "}";
  }
  json += "]}";
  return json;
}

// ─────────────────────────────────────────────────────────────
// TELEGRAM - utrzymaj stałe połączenie SSL  v32
// Zwraca true jeśli połączenie jest gotowe do użycia.
// ─────────────────────────────────────────────────────────────
// [v83] FIX-TCP-KA: TCP Keepalive — zapobiega idle TCP FIN od serwera.
//
// Firebase RTDB i Telegram api.telegram.org wysyłają aktywny TCP FIN
// po ~15s bezczynności → reconnect → TLS handshake → spike 40–46 KB DRAM.
// TCP Keepalive wysyła małe ACK przez stos lwIP co <idle_sec> sekund braku
// ruchu. Serwer widzi aktywne połączenie → nie wysyła FIN.
//
// Działa na poziomie lwIP BSD socket (poniżej mbedTLS) — zero wpływu na RAM.
// MUSI być wywołane PO każdym connect() — nowy socket fd = stare opcje znikają.
//
// Parametry domyślne:
//   idle_sec=5   → pierwszy keepalive probe po 5s ciszy
//   interval=5   → kolejne proby co 5s
//   count=3      → po 3 nieodpowiedzianych → połączenie martwe (system reconnect)
// ─────────────────────────────────────────────────────────────
static bool applyTcpKeepalive(WiFiClientSecure &client,
                               int idle_sec    = 5,
                               int interval_sec = 5,
                               int count       = 3)
{
  int fd = client.fd();
  if (fd <= 0) return false;  // [v99-FIX-FD] fd=0=stdin, fd<0=brak gniazda → errno:9

  int opt = 1;
  if (setsockopt(fd, SOL_SOCKET,  SO_KEEPALIVE,   &opt,          sizeof(opt))          < 0) return false;
  if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE,   &idle_sec,     sizeof(idle_sec))     < 0) return false;
  if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL,  &interval_sec, sizeof(interval_sec)) < 0) return false;
  if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT,    &count,        sizeof(count))        < 0) return false;

  return true;
}

// ─────────────────────────────────────────────────────────────
// [v150] FIX-WDT-WRITEGUARD: bounded select() writability check
// PRZED tgClient.printf()/print() w tgPost().
//
// KONTEKST (Priorytet 0/1, research potwierdzony w GitHub arduino-esp32):
// WiFiClient::write() (używane wewnątrz mbedTLS jako BIO send callback)
// samo w sobie robi select()+send(MSG_DONTWAIT) w pętli do
// WIFI_CLIENT_MAX_WRITE_RETRY=10 razy po WIFI_CLIENT_SELECT_TIMEOUT_US=1s
// (issue #8303, #2555) — to jest ZASZYTE na sztywno w bibliotece, NIE
// honoruje client.setTimeout()/SO_SNDTIMEO, i NIE da się tego zmienić bez
// patchowania samej biblioteki (build_flags nie pomagają — stałe nie są
// za #ifndef). Efekt: pojedyncze printf()/print() może legalnie zająć do
// ~10s — dokładnie na granicy 10s WDT, x2 w tgPost() (nagłówki + body).
//
// Ten guard NIE naprawia samej biblioteki (nie da się z zewnątrz), ale
// odcina najczęstszy trigger: gdy gniazdo jest już martwe/pół-otwarte
// (peer przestał ACK-ować, ale FIN/RST jeszcze nie doszedł), select()
// na zapis zwykle zwraca "niegotowe" od razu — łapiemy to PRZED wejściem
// w wewnętrzną pętlę biblioteki, z naszym własnym, krótkim timeoutem
// (domyślnie 1500ms), zamiast czekać do 10s x2. Jeśli guard nie przejdzie,
// traktujemy socket jako martwy: stop() + wymuszony reconnect przy
// następnym wywołaniu, zero próby printf()/print() na gnieździe, które i
// tak najpewniej by zawisło.
//
// Residual risk (uczciwie, patrz plan_poprawek_WDT.md): guard chroni przed
// przypadkiem "socket od razu niezapisywalny". NIE eliminuje w 100% ryzyka
// zawisu w trakcie AKTYWNEGO wysyłania (select() mówi "gotowe", ale peer
// przestaje ACK-ować dopiero w trakcie send()) — to wciąż otwarty problem,
// pełny fix wymaga migracji na HTTPClient/async (patrz Priorytet 0/1 pkt 3).
// ─────────────────────────────────────────────────────────────
static bool tgWriteGuard(WiFiClientSecure &client, int timeoutMs = 1500)
{
  int fd = client.fd();
  if (fd <= 0) return false;  // [v99-FIX-FD] wzorzec jak w applyTcpKeepalive()

  fd_set wset;
  FD_ZERO(&wset);
  FD_SET(fd, &wset);
  struct timeval tv;
  tv.tv_sec  = timeoutMs / 1000;
  tv.tv_usec = (timeoutMs % 1000) * 1000;

  int rc = select(fd + 1, NULL, &wset, NULL, &tv);
  esp_task_wdt_reset();  // select() sam może czekać do timeoutMs — nakarm WDT zaraz po powrocie
  if (rc <= 0) return false;             // 0=timeout, <0=błąd select()
  return FD_ISSET(fd, &wset);
}

// Reconnect tylko gdy: pierwsze wywołanie / WiFi wrócił /
//   Telegram zamknął idle / połączenie zerwane.
// ─────────────────────────────────────────────────────────────
bool tgEnsureConnected() {
  PRE_RESET_CP("TG-CONN-start");  // [v71]
  if (WiFi.status() != WL_CONNECTED)         { tgClientReady = false; return false; }
  if (tgBotToken.length() < 10)              { return false; }

  bool needReconnect = !tgClientReady
                    || !tgClient.connected()
                    || (millis() - tgClientLastUse > TG_IDLE_RECONNECT_MS);

  if (needReconnect) {
    // ── DBG-CONN: loguj KAŻDĄ próbę - poprzednio tylko wolne/nieudane ──
    logPrintf("lvl=INFO tag=TG-CONN msg=\"proba connect\" ready=%d connected=%d idleMs=%lu freeH=%luB\n",
              (int)tgClientReady, (int)tgClient.connected(),
              (unsigned long)(millis() - tgClientLastUse),
              (unsigned long)ESP.getFreeHeap());

    // [v67-Z2] MEM-FREE: zmierz DRAM przed stop() — zidentyfikuj czy stop() zwalnia duży blok
    uint32_t _freeBeforeStop = ESP.getFreeHeap();
    tgClient.stop();
    uint32_t _freeAfterStop = ESP.getFreeHeap();
    if (_freeAfterStop > _freeBeforeStop + 4096) {
      logPrintf("lvl=INFO tag=MEM-FREE src=TG freeH_before=%luB freeH_after=%luB delta=+%ldB\n",
                (unsigned long)_freeBeforeStop, (unsigned long)_freeAfterStop,
                (long)(_freeAfterStop - _freeBeforeStop));
    }

    tgClientReady = false;
    tgClient.setInsecure();
    tgClient.setTimeout(2);           // [v108] FIX-WDT-NETWORK-STORM: TLS 4→2s. TCP(5s)+TLS(2s)=7s < WDT 10s
    // [v50] FIX-WDT-TCP: setConnectionTimeout ustawia TCP-level connect timeout.
    // setConnectionTimeout() — niedostępna w arduino-esp32. Używamy connect(ip, port, timeout_ms) [v103]

    // [v69-Z5] TLS-MUTEX: weź mutex przed handshake + pomiar czasu oczekiwania
    // [v82] FIX-WDT-MUTEX: polling co 100ms zamiast jednorazowego bloku 10s.
    // Poprzedni kod: xSemaphoreTake(10000ms) blokował tgTask bez WDT reset przez
    // maksymalnie 10s → WDT strzela dokładnie przy granicy. Polling co 100ms
    // daje esp_task_wdt_reset() przy każdej iteracji → WDT niemożliwy podczas czekania.
    bool _tlsLocked = false;
    if (tlsMutex) {
      unsigned long _mutexWaitStart = millis();
      while (millis() - _mutexWaitStart < 9000UL) {
        if (xSemaphoreTake(tlsMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
          _tlsLocked = true;
          break;
        }
        esp_task_wdt_reset();  // [v82] FIX-WDT-MUTEX: feed WDT co 100ms podczas czekania
      }
      if (_tlsLocked) {
        uint32_t _waited = (uint32_t)(millis() - _mutexWaitStart);
        if (_waited > 50) {
          g_tlsMutexWaitTG = g_tlsMutexWaitTG + 1;  // [v98-FIX] volatile++ deprecated w C++20
          if (_waited > g_tlsMutexMaxWaitMs) g_tlsMutexMaxWaitMs = _waited;
          logPrintf("lvl=WARN tag=TLS-MUTEX msg=\"TG czekal na mutex\" ms=%lu waitCntTG=%lu maxWaitMs=%lu\n",
                    (unsigned long)_waited, (unsigned long)g_tlsMutexWaitTG,
                    (unsigned long)g_tlsMutexMaxWaitMs);
        }
      } else {
        logPrintf("lvl=WARN tag=TLS-MUTEX msg=\"TG: timeout 9s polling, kontynuuje bez mutex\"\n");
      }
    }

    // [v84] FIX-TLS-GAP: cooldown po poprzednim TLS handshake (FB lub TG).
    // Daje heapowi czas na defragmentację przed kolejną alokacją mbedTLS (~48 KB).
    // Realizowane wewnątrz mutexa — mutex = sekcja krytyczna handshake'u.
    {
      uint32_t _sinceLastTls = millis() - g_lastTlsHandshakeDoneMs;
      if (g_lastTlsHandshakeDoneMs > 0 && _sinceLastTls < TLS_COOLDOWN_MS) {
        uint32_t _coolWait = TLS_COOLDOWN_MS - _sinceLastTls;
        logPrintf("lvl=INFO tag=TLS-COOL msg=\"czekam na cooldown po poprzednim handshake\" ms=%lu\n",
                  (unsigned long)_coolWait);
        uint32_t _coolStart = millis();
        while (millis() - _coolStart < _coolWait) {
          esp_task_wdt_reset();
          vTaskDelay(pdMS_TO_TICKS(100));
        }
      }
    }

    // [v67-Z1] HeapSrcTag: oznacz źródło spike'u HEAP — g_heapSrc usunięta [FIX-W2], używamy tylko g_heapSrcAccum

    // [v75] FIX-WDT-DNS: pre-resolve hostname przez WiFi.hostByName() (TCPIP-task-safe, nie
    // wywołuje udp_new_ip_type() z user-task). connect(IP) pomija DNS → brak assert IDF v5.x.
    // Cache 5 min — re-resolve gdy wygaśnie lub brak valid IP.
    const char* TG_HOST = "api.telegram.org";
    const unsigned long TG_DNS_CACHE_MS = 300000UL;  // 5 minut
    esp_task_wdt_reset();  // świeże okno WDT przed DNS (może trwać do 4s)
    if (!g_tgIPValid || (millis() - g_tgIPTs > TG_DNS_CACHE_MS)) {
      IPAddress _resolved;
      // [v162] DIAG-DNSSTALL: WiFi.hostByName() jest zabezpieczone resetem
      // WDT PRZED i PO wywołaniem, ale NIE MA żadnej ochrony W TRAKCIE — to
      // jedno wywołanie blokujące, tak samo jak wcześniej tgClient.printf()
      // (v94, ostatecznie zmierzone w v155 jako TG-SEND-STALL, zawsze
      // czyste). Komentarz dev-a zakłada "może trwać do 4s" jako normę, ale
      // to NIGDY nie było realnie zmierzone. 5 kolejnych crashy WDT (log
      // combined 12/15/17/18) miało 5 różnych checkpointów w loop() (Core 1)
      // przy identycznym momencie (cooldown=0s) i ŻADEN z liczników
      // Core-1 (events/flash/fsGuard) nic nie złapał — patrz plan WDT.
      // hostByName() w tgEnsureConnected() (Core 0, tgTask) to jedyne
      // pozostałe, dotąd niezmierzone, w pełni bezterminowe pojedyncze
      // wywołanie na ścieżce half-open. Próg 2000ms (powyżej udokumentowanej
      // "normy" ~1-4s deweloperskiego komentarza — łapiemy odchylenia
      // powyżej dolnej granicy tego zakresu, nie czekamy na katastrofę).
      unsigned long _dnsT0 = millis();
      bool _tgDnsOk = WiFi.hostByName(TG_HOST, _resolved);
      // [v164] FIX-DNS-ZERO-IP: patrz komentarz w fbInitialize (ten sam bug,
      // symetrycznie dla TG).
      if (_tgDnsOk && _resolved == IPAddress(0, 0, 0, 0)) {
        _tgDnsOk = false;
        logPrintf("lvl=WARN tag=DNS-ZERO-IP site=tgEnsureConnected msg=\"hostByName() zwrocil 0.0.0.0, traktuje jako blad\"\n");
      }
      unsigned long _dnsMs = millis() - _dnsT0;
      if (_dnsMs >= DNS_STALL_THRESHOLD_MS) {
        g_dnsStallCount = g_dnsStallCount + 1;
        if (_dnsMs > g_dnsStallMaxMs) g_dnsStallMaxMs = _dnsMs;
        logPrintf("lvl=WARN tag=DNS-STALL site=tgEnsureConnected ms=%lu ok=%d cnt=%lu maxMs=%lu\n",
                  _dnsMs, (int)_tgDnsOk,
                  (unsigned long)g_dnsStallCount, (unsigned long)g_dnsStallMaxMs);
      }
      if (_tgDnsOk) {
        g_tgServerIP = _resolved;
        g_tgIPValid  = true;
        g_tgIPTs     = millis();
        logPrintf("lvl=INFO tag=TG-DNS msg=\"Resolved\" host=%s ip=%s\n", TG_HOST, g_tgServerIP.toString().c_str());
      } else {
        g_tgIPValid = false;
        logPrintf("lvl=WARN tag=TG-DNS msg=\"hostByName() fail, fallback hostname\" host=%s\n", TG_HOST);
      }
    }
    esp_task_wdt_reset();  // reset po DNS, świeże 10s okno dla TCP+TLS

    // [v85] FIX-PSRAM-TLS: zmierz internal DRAM przed i po connect() — weryfikacja Fix 4.
    // Jeśli PSRAM-TLS działa: delta ~2–5 KB (tylko ssl_context w DRAM). Jeśli nie: ~45 KB.
    uint32_t _tlsDramBefore = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    unsigned long _tgConnStart = millis();
    // [v108] FIX-WDT-NETWORK-STORM: v103 zakładał WDT=15s, v107 ma WDT=10s.
    // TCP(8s)+TLS(4s)=12s > WDT 10s → crash przy martwej sieci.
    // Fix: TCP 8000→5000ms + TLS setTimeout 4→2s = 7s max < WDT 10s (3s margines).
    // Dodatkowy esp_task_wdt_reset() PRZED connect() — świeże 10s okno niezależnie
    // od długości kodu od resetu po DNS.
    esp_task_wdt_reset();  // [v108] świeże 10s okno przed TCP+TLS (max 7s łącznie)
    // [v227] FIX-TLS-HANDSHAKE-TIMEOUT: komentarz wyżej (v108) zakładał, że parametr
    // connect(ip,port,timeoutMs) ogranicza CAŁY handshake (TCP+TLS) do 5000ms — ale to
    // nieprawdziwe założenie (research: arduino-esp32 issues #481, #2896, #6077 — TCP-level
    // timeout NIE ogranicza fazy mbedtls_ssl_handshake(), która może wisieć bezterminowo).
    // log_combined__2_.txt (15:28:15-15:28:35) potwierdził to empirycznie: 14s ciszy
    // między [TLS-DRAM] a spodziewanym [connect wynik], które nigdy się nie pojawiło,
    // mimo że connect() miał timeout=5000ms. WiFiClientSecure ma DEDYKOWANĄ,
    // OSOBNĄ metodę na tę fazę: setHandshakeTimeout(ms) (issue #6077 potwierdza jej
    // istnienie i że handshake_timeout resetuje się do 0 po każdym stop() — trzeba
    // wywoływać przed KAŻDĄ próbą connect(), nie raz przy starcie).
    tgClient.setHandshakeTimeout(4000);  // [v227] margines: TCP(5s, nie zawsze pełne) + TLS(4s) w oknie WDT(10s)
    if (g_tgIPValid) {
      tgClient.connect(g_tgServerIP, 443, 5000);  // [v108] TCP timeout 8000→5000ms
    } else {
      tgClient.connect(TG_HOST, 443, 5000);        // [v108] TCP timeout 8000→5000ms
    }
    esp_task_wdt_reset();  // [v108] reset po connect() — TCP(5s)+TLS(2s)=7s < WDT 10s ✓
    while (!tgClient.connected() && (millis() - _tgConnStart < 5000UL)) {  // [v108] 8000→5000ms spójne z connect()
      esp_task_wdt_reset();
      vTaskDelay(pdMS_TO_TICKS(50));  // [v50] 50ms zamiast 100ms - szybszy poll stanu TCP
    }
    tgClientReady = tgClient.connected();
    unsigned long _tgConnMs = millis() - _tgConnStart;
    esp_task_wdt_reset();

    // [v82] FIX-WDT-SNDTIMEO: ustaw SO_SNDTIMEO na gnieździe TCP po udanym connect.
    // Bug Arduino ESP32 #7356: send_ssl_data() (wywołane przez tgClient.write()) nie
    // respektuje setTimeout() przy black-hole TCP (sieć nie przyjmuje danych, TCP
    // send buffer pełny). Wisi nieskończenie bez WDT reset → WDT crash po >10s.
    // SO_SNDTIMEO działa na poziomie BSD socket → wymusza powrót z write() po 8s.
    // Symetryczny fix w fbConnect().
    if (tgClientReady) {
      int _tgSock = tgClient.fd();  // underlying TCP socket fd
      if (_tgSock >= 0) {
        struct timeval _tv;
        _tv.tv_sec  = 8;  // 8s — mieści się w oknie WDT 10s
        _tv.tv_usec = 0;
        setsockopt(_tgSock, SOL_SOCKET, SO_SNDTIMEO, &_tv, sizeof(_tv));
        setsockopt(_tgSock, SOL_SOCKET, SO_RCVTIMEO, &_tv, sizeof(_tv));
      }
    }

    // [v83] FIX-TCP-KA: TCP Keepalive — zapobiega idle FIN od Telegram serwera.
    // [v147] FIX-TG-KA-WINDOW: idle=3s, interval=2s, count=3 (razem 9s < WDT 10s).
    // Wcześniej idle=5/interval=5 dawało 20s — dwa razy dłużej niż okno WDT,
    // ten sam błąd co dla Firebase przed v112 (patrz fbConnect).
    // Musi być po connect() (nowy fd).
    if (tgClientReady) {
      bool _kaOk = applyTcpKeepalive(tgClient, 2, 1, 2); // [v167] KA-TIGHTEN-TG: idle=2s+intvl(1)×cnt(2)=4s (bylo 9s) - symetria z fix v163 dla FB
      if (!_kaOk) logPrintf("lvl=WARN tag=TG-KA msg=\"keepalive=FAIL\" fd=%d\n", tgClient.fd()); // [v97-LOG] tylko FAIL
      g_lastTlsHandshakeDoneMs = millis();  // [v84] FIX-TLS-GAP: znacznik dla cooldown
    }

    // [v85] FIX-PSRAM-TLS: weryfikacja — ile internal DRAM pochłonął handshake TG.
    // Oczekiwane po Fix 4: delta ~2–5 KB (ssl_context w DRAM, TX/RX bufory w PSRAM).
    // Jeśli delta > 20 KB → PSRAM-TLS nie działa (brak PSRAM lub próg za wysoki).
    {
      uint32_t _tlsDramAfter = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
      int32_t  _tlsDramDelta = (int32_t)_tlsDramBefore - (int32_t)_tlsDramAfter;
      const char* _psramOk   = (_tlsDramDelta < 20000) ? "PSRAM" : "DRAM";
      logPrintf("lvl=INFO tag=TLS-DRAM src=TG beforeKB=%lu afterKB=%lu delta=%+ldB heapSrc=TG_TLS state=%s\n", // [FIX-v128-FORMAT-DELTA] -%ldB%+ldB (delta=before-after: ujemna=DRAM zwolniony/PSRAM ok)
                (unsigned long)(_tlsDramBefore / 1024),
                (unsigned long)(_tlsDramAfter / 1024),
                (long)_tlsDramDelta,
                _psramOk);
      if (_tlsDramDelta > 20000) {
        logPrintf("lvl=WARN tag=PSRAM-TLS src=TG msg=\"Fix 4 nieaktywny, mbedTLS wciaz w DRAM, sprawdz PSRAM\" deltaB=%ld\n",
                  (long)_tlsDramDelta);
      }
    }

    // [v81-DIAG] zawsze loguj z pełnym timing i flagami
    logPrintf("lvl=INFO tag=TG-CONN msg=\"connect wynik\" ok=%s ms=%lu via=%s freeH=%luB slow=%s\n",
              tgClientReady ? "true" : "false",
              _tgConnMs,
              g_tgIPValid ? g_tgServerIP.toString().c_str() : TG_HOST,
              (unsigned long)ESP.getFreeHeap(),
              _tgConnMs > 5000 ? "bardzo" : (_tgConnMs > 2000 ? "tak" : "nie"));

    // [v68-FIX-Z1] Zapamiętaj źródło do heartbeat zawsze (nie tylko SUCCESS).
    // mbedTLS alokuje ~47 kB przy każdej próbie handshake — FAIL też jest spikiem.
    g_heapSrcAccum |= (uint8_t)HEAP_SRC_TG_TLS;  // [v69-Z1] OR — nie nadpisuje FB jeśli już ustawione
    // [v69b-FIX-W2] g_heapSrc = ... usunięte (dead code)

    // [v67-Z3] Zwolnij mutex TLS po zakończeniu handshake
    if (_tlsLocked && tlsMutex) xSemaphoreGive(tlsMutex);

    if (!tgClientReady) {
      // ************************************************FIX-v57-RESET-LOOP (ROOT CAUSE A)
      // Po connect()=FAIL tgTask natychmiast wracał do pętli i wchodził w kolejny blok
      // TLS (TG-POLL lub bootNotify), kumulując ~16s blokady > WDT 10s.
      // esp_task_wdt_reset() tutaj zeruje timer WDT po nieudanym connect (8s),
      // dając pewność że następna operacja startuje z pełnym oknem 10s.
      esp_task_wdt_reset();  // FIX-v57: reset WDT po nieudanym connect (8s blokady)
      return false;
    }
  }
  tgClientLastUse = millis();
  return true;
}

// ─────────────────────────────────────────────────────────────
// TELEGRAM - wykonaj żądanie HTTP POST przez stały tgClient  v32
// Buduje i wysyła surowe HTTP/1.1, zwraca odpowiedź jako String.
// ─────────────────────────────────────────────────────────────
String tgPost(const String& path, const String& body) {
  if (!tgEnsureConnected()) return "";

  unsigned long _tgPostStart = millis();

  // Wyślij request HTTP/1.1 keep-alive
  // [OK] FIX-v33b: printf zamiast konkatenacji Stringów - zero alokacji tymczasowych
  // [v94-FIX-WDT] Reset przed blokującymi operacjami send: martwy socket (TCP black hole)
  // może blokować printf/print bezterminowo. Reset daje pełne 10s okno WDT.
  // [v150-FIX-WDT-WRITEGUARD] Dodatkowo: select()-owy pre-check przed KAŻDYM
  // printf()/print() — odcina najczęstszy trigger (martwe/pół-otwarte gniazdo)
  // zanim wejdziemy w wewnętrzną 10s pętlę biblioteki (patrz komentarz przy
  // tgWriteGuard()). Nie zastępuje esp_task_wdt_reset(), tylko go uzupełnia.
  esp_task_wdt_reset();
  if (!tgWriteGuard(tgClient)) {
    logPrintf("lvl=WARN tag=TG-POST site=naglowki msg=\"WRITEGUARD: gniazdo niezapisywalne, pomijam send, wymuszam reconnect\"\n");
    tgClient.stop();
    tgClientReady = false;
    return String();
  }
  // [v155] DIAG-TGSENDSTALL: zmierz realny czas printf() mimo że writeguard przeszedł
  {
    unsigned long _sendT0 = millis();
    tgClient.printf(
      "POST %s HTTP/1.1\r\n"
      "Host: api.telegram.org\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: %u\r\n"
      "Connection: keep-alive\r\n"
      "\r\n",
      path.c_str(), (unsigned)body.length());
    uint32_t _sendMs = (uint32_t)(millis() - _sendT0);
    if (_sendMs > TG_SEND_STALL_THRESHOLD_MS) {
      g_tgSendStallCount = g_tgSendStallCount + 1;
      if (_sendMs > g_tgSendStallMaxMs) g_tgSendStallMaxMs = _sendMs;
      logPrintf("lvl=WARN tag=TG-SEND-STALL site=headers msg=\"writeguard OK, ale send trwal dlugo\" ms=%lu cnt=%lu maxMs=%lu\n",
                (unsigned long)_sendMs, (unsigned long)g_tgSendStallCount, (unsigned long)g_tgSendStallMaxMs);
    }
  }
  esp_task_wdt_reset();
  if (!tgWriteGuard(tgClient)) {
    logPrintf("lvl=WARN tag=TG-POST site=body msg=\"WRITEGUARD: gniazdo niezapisywalne, pomijam send, wymuszam reconnect\"\n");
    tgClient.stop();
    tgClientReady = false;
    return String();
  }
  // [v155] DIAG-TGSENDSTALL: zmierz realny czas print(body) mimo że writeguard przeszedł
  {
    unsigned long _sendT0 = millis();
    tgClient.print(body);
    uint32_t _sendMs = (uint32_t)(millis() - _sendT0);
    if (_sendMs > TG_SEND_STALL_THRESHOLD_MS) {
      g_tgSendStallCount = g_tgSendStallCount + 1;
      if (_sendMs > g_tgSendStallMaxMs) g_tgSendStallMaxMs = _sendMs;
      logPrintf("lvl=WARN tag=TG-SEND-STALL site=body msg=\"writeguard OK, ale send trwal dlugo\" ms=%lu cnt=%lu maxMs=%lu\n",
                (unsigned long)_sendMs, (unsigned long)g_tgSendStallCount, (unsigned long)g_tgSendStallMaxMs);
    }
  }

  // Odczytaj odpowiedź (max 5s, max 4KB)
  unsigned long t0 = millis();
  // [OK] FIX-v39-3: Eliminacja fragmentacji HEAP w tgPost().
  //
  // POPRZEDNI KOD (v33):
  //   resp += c  (char po char dla nagłówków) -> bufor rośnie ~100-300B, potem resp=""
  //   -> alokator zwalnia blok, ale zostawia dziurę w stercie.
  //   Po ~2h pracy (uptime ~6856s) dziury skumulowane -> maxAlloc spada z 55284B
  //   do 36852B -> SSL handshake (wymaga ~40KB ciągłego bloku) może się nie udać
  //   -> crash lub wieczny reconnect.
  //
  // NAPRAWA: nagłówki HTTP czytamy przez readStringUntil('\n') - linia po linii,
  //   bez akumulacji do jednego Stringa. Body zbieramy jedną pre-alokacją 4096B
  //   (bez drugiego reserve po clear).
  // [OK] FIX-v45-WDT: readStringUntil('\n') blokuje do client.timeout (10s = WDT limit).
  // Zastąpiono pętlą char-po-char: esp_task_wdt_reset() po każdym bajcie -> max blok ~1ms.
  bool headersDone = false;
  String _hLine;
  _hLine.reserve(128);
  while (millis() - t0 < 5000UL) {
    if (!tgClient.available()) { vTaskDelay(pdMS_TO_TICKS(5)); esp_task_wdt_reset(); continue; }
    char c = (char)tgClient.read();
    esp_task_wdt_reset();  // po każdym bajcie - brak blokowania dłuższego niż czas odczytu 1B
    if (c == '\n') {
      // Pusta linia (CRLF lub sam LF) = koniec nagłówków HTTP
      if (_hLine.length() == 0 || (_hLine.length() == 1 && _hLine[0] == '\r')) {
        headersDone = true;
        break;
      }
      _hLine = "";  // następna linia nagłówka
    } else {
      if (_hLine.length() < 256) _hLine += c;  // ogranicz rozmiar linii (ochrona)
    }
  }
  // [v55] RESP-BUF-PSRAM: bufor odpowiedzi 8 kB przeniesiony do PSRAM.
  // Sliding window: gdy bufor pełny, zachowaj ostatnie 4 kB (memmove).
  // Timeout pętli: 9 s (poniżej WDT 10 s, z marginesem na nagłówki).
  const size_t RESP_BUF_CAP = 8192;
  char* _respBuf = (char*)psramAllocSafe(RESP_BUF_CAP);
  if (!_respBuf) {
    // OOM — nie możemy zebrać odpowiedzi; wymuś reconnect i wróć pusty
    logPrintf("lvl=WARN tag=TG-POST msg=\"OOM: brak PSRAM na bufor odpowiedzi\"\n");
    { uint32_t _b = ESP.getFreeHeap();                                    // [v69-Z2a]
      tgClient.stop();
      uint32_t _a = ESP.getFreeHeap();
      if (_a > _b + 1024) logPrintf("lvl=WARN tag=MEM-FREE src=TG-OOM freeH_before=%luB freeH_after=%luB delta=+%ldB\n",
        (unsigned long)_b, (unsigned long)_a, (long)(_a - _b)); }
    tgClientReady = false;
    return String();
  }
  _respBuf[0] = '\0';
  size_t _respLen = 0;
  if (headersDone) {
    unsigned long t1 = millis();
    while (millis() - t1 < 9000UL) {
      if (!tgClient.available()) {
        if (_respLen > 0) break;  // odpowiedź skompletowana
        vTaskDelay(pdMS_TO_TICKS(5));
        esp_task_wdt_reset();
        continue;
      }
      char c = (char)tgClient.read();
      esp_task_wdt_reset();  // reset po każdym bajcie — brak blokowania >~1 ms
      if (_respLen < RESP_BUF_CAP - 1) {
        _respBuf[_respLen++] = c;
        _respBuf[_respLen]   = '\0';
      } else {
        // Przesuń okno — zachowaj ostatnie 4 kB
        size_t half = RESP_BUF_CAP / 2;
        memmove(_respBuf, _respBuf + half, half + 1);
        _respLen = half;
        _respBuf[_respLen++] = c;
        _respBuf[_respLen]   = '\0';
      }
    }
  }
  String resp(_respBuf);  // jednorazowa alokacja String w DRAM
  free(_respBuf);          // zwolnij PSRAM natychmiast
  tgClientLastUse = millis();

  // DIAG: wykryj martwe gniazdo i timeout
  unsigned long _tgPostMs = millis() - _tgPostStart;
  bool _timedOut  = (_tgPostMs >= 4900);
  bool _noHeaders = !headersDone;
  // [v81-DIAG] loguj KAŻDE wywołanie (nie tylko wolne/błędy)
  {
    String _shortPath = path.substring(path.lastIndexOf('/') + 1);
    if (_shortPath.length() > 20) _shortPath = _shortPath.substring(0, 20);
    logPrintf("lvl=INFO tag=TG-POST path=%s ms=%lu hdr=%s reqB=%u respB=%d freeH=%luB timeout=%s\n",
              _shortPath.c_str(),
              _tgPostMs,
              headersDone ? "true" : "false",
              (unsigned)body.length(),
              (int)resp.length(),
              (unsigned long)ESP.getFreeHeap(),
              _timedOut ? "true" : "false");
    if (_timedOut || _noHeaders) {
      // Martwe gniazdo - wymuś reconnect przy następnym wywołaniu
      { uint32_t _b = ESP.getFreeHeap();                                  // [v69-Z2b]
        tgClient.stop();
        uint32_t _a = ESP.getFreeHeap();
        if (_a > _b + 1024) logPrintf("lvl=WARN tag=MEM-FREE src=TG-TIMEOUT freeH_before=%luB freeH_after=%luB delta=+%ldB\n",
          (unsigned long)_b, (unsigned long)_a, (long)(_a - _b)); }
      tgClientReady = false;
      logPrintf("lvl=WARN tag=TG-POST msg=\"wymuszono disconnect\" tgClientReady=false\n");
      NET_FAIL(_tgFailStreak);  // [FIX-v131][v230] inkrementuj streak — TG timeout też liczy
    }
  }

  if (!_timedOut && headersDone && resp.length() > 0) {
    NET_SUCCESS(_tgFailStreak);  // [FIX-v131][v230] — TG odpowiedział poprawnie
  }
  return resp;
}

// ─────────────────────────────────────────────────────────────
// TELEGRAM - usuń wiadomość (czyszczenie okna czatu) v30-0
// ─────────────────────────────────────────────────────────────
void deleteTelegramMessage(long msgId) {
  if (msgId <= 0 || tgBotToken.length() < 10 || tgChatId.length() < 3) return;
  unsigned long _delT = millis();  // [v81-DIAG]
  String payload = "{\"chat_id\":\"" + tgChatId + "\",\"message_id\":" + String(msgId) + "}";
  String _resp = tgPost("/bot" + tgBotToken + "/deleteMessage", payload);  // v32: stały tgClient
  logPrintf("lvl=INFO tag=TG-DEL id=%ld ms=%lu ok=%d freeH=%luB\n", // [v81-DIAG]
            msgId, millis()-_delT, (int)(_resp.indexOf("\"ok\":true")>=0),
            (unsigned long)ESP.getFreeHeap());
}

// ─────────────────────────────────────────────────────────────
// TELEGRAM - parsuj message_id z odpowiedzi JSON API  v30-0
// ─────────────────────────────────────────────────────────────
long parseTelegramMsgId(const String& body) {
  int pos = body.indexOf("\"message_id\":");
  if (pos < 0) return -1;
  pos += 13;
  while (pos < (int)body.length() && body[pos] == ' ') pos++;
  long id = body.substring(pos, pos + 15).toInt();
  return (id > 0) ? id : -1;
}

// ─────────────────────────────────────────────────────────────
// TELEGRAM - zapisz message_id do ring-buffera  v30-2
// ─────────────────────────────────────────────────────────────
void tgPushMsgId(long id) {
  if (id <= 0) return;
  tgMsgHistory[tgMsgHistoryIdx % TG_HISTORY_SIZE] = id;
  tgMsgHistoryIdx++;
  tgLastMenuMsgId = id;
}

// ─────────────────────────────────────────────────────────────
// TELEGRAM - usuń TYLKO poprzednią wiadomość menu  v31-FIX
// (1 połączenie SSL zamiast 20 - eliminuje fragmentację HEAP)
// Ring-buffer historii zachowany dla kompatybilności ale
// kasujemy wyłącznie tgLastMenuMsgId.
// ─────────────────────────────────────────────────────────────
// [v78 FIX-TG-HISTORY] Kasuje wszystkie wiadomości z ring-buffera (nie tylko menu).
// Używa max 2 połączeń SSL (ostatnia wiadom. + menu) bo tylko te są w ring-bufferze.
// ─────────────────────────────────────────────────────────────
void tgDeleteAllHistory() {
  // Kasuj wszystkie unikalne message_id z ring-buffera
  unsigned long _histT = millis();  // [v81-DIAG]
  int _histN = 0;
  logPrintf("lvl=INFO tag=TG-HIST-DEL faza=start freeH=%luB\n", (unsigned long)ESP.getFreeHeap()); // [v81-DIAG]
  for (int i = 0; i < TG_HISTORY_SIZE; i++) {
    long mid = tgMsgHistory[i];
    if (mid > 0) {
      esp_task_wdt_reset();
      logPrintf("lvl=INFO tag=TG-HIST-DEL faza=iter idx=%d total=%d id=%ld freeH=%luB\n", // [v81-DIAG]
                i, TG_HISTORY_SIZE, mid, (unsigned long)ESP.getFreeHeap());
      deleteTelegramMessage(mid);
      tgMsgHistory[i] = 0;
      _histN++;
    }
  }
  tgLastMenuMsgId = -1;
  tgMsgHistoryIdx = 0;
  logPrintf("lvl=INFO tag=TG-HIST-DEL faza=done liczba=%d ms=%lu freeH=%luB\n", // [v81-DIAG]
            _histN, millis()-_histT, (unsigned long)ESP.getFreeHeap());
}

// ─────────────────────────────────────────────────────────────
// TELEGRAM - wyślij wiadomość tekstową (v30-0: zapisuje message_id)
// ─────────────────────────────────────────────────────────────
bool sendTelegramMessage(const String& text, bool trackMsgId = true) {
  PRE_RESET_CP("TG-MSG");  // [v71]
  if (!tgEnabled || tgBotToken.length() < 10 || tgChatId.length() < 3) return false;
  String safe = text;
  safe.reserve(text.length() + 64);  // [OK] FIX-v33b: margines na escape znaki
  safe.replace("\\", "\\\\");
  safe.replace("\"", "\\\"");
  safe.replace("\n", "\\n");
  String payload;
  payload.reserve(safe.length() + 64);  // [OK] FIX-v33b: chat_id + parse_mode overhead ~50B
  payload = "{\"chat_id\":\"" + tgChatId + "\",\"text\":\"" + safe + "\",\"parse_mode\":\"HTML\"}";
  String resp = tgPost("/bot" + tgBotToken + "/sendMessage", payload);  // v32: stały tgClient
  bool ok = (resp.indexOf("\"ok\":true") >= 0);
  if (ok && trackMsgId) {
    long newId = parseTelegramMsgId(resp);
    tgPushMsgId(newId);
  }
  if (!ok) logPrintln("lvl=ERR tag=TG msg=\"sendMessage blad\"");
  return ok;
}

// ─────────────────────────────────────────────────────────────
// TELEGRAM - wyślij połączone logi [log_a.txt + log_b.txt] jako jeden dokument
// [v117] DUAL-FILE COMBINED: identycznie jak panel WWW — logReadCombinedTail()
//        łączy log_a (starsze, rano) + log_b (nowsze) chronologicznie w jeden bufor PSRAM.
// Używa multipart/form-data.
// ─────────────────────────────────────────────────────────────

static bool sendTelegramCoredumpPlik(const String& path, uint32_t boot, uint32_t seq, size_t fileSize) {
  if (!tgEnabled || tgBotToken.length() < 10 || tgChatId.length() < 3) return false;
  if (WiFi.status() != WL_CONNECTED || !littlefsReady) return false;
  File src = LittleFS.open(path, "r");
  if (!src) return false;
  fileSize = src.size();
  if (fileSize == 0 || fileSize > 50UL * 1024UL * 1024UL) {
    src.close();
    logPrintf("lvl=ERR tag=CRASH-TG zdarzenie=plik_niepoprawny rozmiar=%uB plik=%s\n",
              (unsigned)fileSize, path.c_str());
    return false;
  }

  const String boundary = "ESP32CDBnd8f";
  String caption = "RYBY coredump seq=" + String((unsigned long)seq) +
                   " boot=" + String((unsigned long)boot) +
                   " size=" + String((unsigned long)fileSize) + "B";
  String head =
    "--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" + tgChatId + "\r\n"
    "--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"caption\"\r\n\r\n" + caption + "\r\n"
    "--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"document\"; filename=\"coredump_seq" +
    String((unsigned long)seq) + "_boot" + String((unsigned long)boot) + ".bin\"\r\n"
    "Content-Type: application/octet-stream\r\n\r\n";
  String tail = "\r\n--" + boundary + "--\r\n";
  size_t totalLen = head.length() + fileSize + tail.length();

  // Coredump jest wysylany tylko z tgTaskFn, wiec dzielimy to samo gniazdo\n  // z reszta Telegrama sekwencyjnie. Uzywamy tych samych limitow WDT/TCP co log upload.\n  tgClient.stop();
  tgClientReady = false;
  tgClient.setInsecure();
  tgClient.setTimeout(2);
  tgClient.setHandshakeTimeout(4000);

  const char* host = "api.telegram.org";
  const unsigned long DNS_CACHE_MS = 300000UL;
  if (!g_tgIPValid || (millis() - g_tgIPTs > DNS_CACHE_MS)) {
    IPAddress resolved;
    esp_task_wdt_reset();
    bool dnsOk = WiFi.hostByName(host, resolved);
    esp_task_wdt_reset();
    if (dnsOk && resolved != IPAddress(0,0,0,0)) {
      g_tgServerIP = resolved;
      g_tgIPValid = true;
      g_tgIPTs = millis();
    } else {
      g_tgIPValid = false;
      src.close();
      return false;
    }
  }

  esp_task_wdt_reset();
  unsigned long connT = millis();
  bool connected = g_tgIPValid ? tgClient.connect(g_tgServerIP, 443, 5000)
                               : tgClient.connect(host, 443, 5000);
  esp_task_wdt_reset();
  if (!connected) {
    src.close();
    return false;
  }
  while (!tgClient.connected() && millis() - connT < 5000UL) {
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(50));
  }
  if (!tgClient.connected()) {
    src.close();
    tgClient.stop();
    return false;
  }
  tgClientReady = true;
  tgClientLastUse = millis();

  int fd = tgClient.fd();
  if (fd >= 0) {
    struct timeval tv;
    tv.tv_sec = 8; tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  }
  (void)applyTcpKeepalive(tgClient, 2, 1, 2);

  esp_task_wdt_reset();
  tgClient.printf(
    "POST /bot%s/sendDocument HTTP/1.1\r\n"
    "Host: api.telegram.org\r\n"
    "Content-Type: multipart/form-data; boundary=%s\r\n"
    "Content-Length: %u\r\n"
    "Connection: close\r\n\r\n",
    tgBotToken.c_str(), boundary.c_str(), (unsigned)totalLen);
  tgClient.print(head);

  uint8_t* buf = (uint8_t*)ps_malloc(4096);
  if (!buf) buf = (uint8_t*)malloc(4096);
  if (!buf) {
    src.close(); tgClient.stop(); tgClientReady = false;
    return false;
  }

  bool streamOk = true;
  while (src.available()) {
    size_t n = src.read(buf, 4096);
    if (n == 0) { streamOk = false; break; }
    size_t off = 0;
    while (off < n) {
      esp_task_wdt_reset();
      size_t w = tgClient.write(buf + off, n - off);
      if (w == 0) { streamOk = false; break; }
      off += w;
    }
    if (!streamOk) break;
  }
  free(buf);
  src.close();
  if (!streamOk) {
    tgClient.stop(); tgClientReady = false;
    return false;
  }
  tgClient.print(tail);

  bool ok = false;
  unsigned long respStart = millis();
  String response;
  response.reserve(768);
  while (tgClient.connected() && millis() - respStart < 12000UL) {
    while (tgClient.available()) {
      char c = tgClient.read();
      if (response.length() < 2000) response += c;
    }
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  ok = response.indexOf("\"ok\":true") >= 0;
  tgClient.stop();
  tgClientReady = false;
  return ok;
}

void wyslijOczekujaceCoredumpyTelegram() {
  static unsigned long lastAttemptMs = 0;
  if (!tgEnabled || WiFi.status() != WL_CONNECTED || !littlefsReady) return;
  if (millis() - lastAttemptMs < 60000UL) return;
  lastAttemptMs = millis();

  String path;
  uint32_t seq = 0, boot = 0;
  size_t fileSize = 0;
  if (!coredumpZnajdzNajstarszy(true, path, seq, boot, fileSize)) return;

  uint32_t textSeq = coredumpNvsGet(COREDUMP_NVS_KEY_TEXT_SEQ, 0);
  uint32_t fileSeq = coredumpNvsGet(COREDUMP_NVS_KEY_FILE_SEQ, 0);

  if (textSeq < seq) {
    String msg = coredumpCrashMessage(boot, seq, fileSize);
    if (!sendTelegramMessage(msg)) {
      logPrintf("lvl=WARN tag=CRASH-TG zdarzenie=tekst_retry seq=%lu\n", (unsigned long)seq);
      return;
    }
    if (!coredumpNvsPut(COREDUMP_NVS_KEY_TEXT_SEQ, seq)) {
      logPrintf("lvl=WARN tag=CRASH-TG zdarzenie=tekst_ack_nieudany seq=%lu\n", (unsigned long)seq);
      return;
    }
    logPrintf("lvl=INFO tag=CRASH-TG zdarzenie=tekst_wyslany seq=%lu\n", (unsigned long)seq);
  }

  if (fileSeq < seq) {
    if (!sendTelegramCoredumpPlik(path, boot, seq, fileSize)) {
      logPrintf("lvl=WARN tag=CRASH-TG zdarzenie=plik_retry seq=%lu plik=%s\n",
                (unsigned long)seq, path.c_str());
      return;
    }
    if (!coredumpNvsPut(COREDUMP_NVS_KEY_FILE_SEQ, seq)) {
      logPrintf("lvl=WARN tag=CRASH-TG zdarzenie=plik_ack_nieudany seq=%lu\n", (unsigned long)seq);
      return;
    }
    logPrintf("lvl=INFO tag=CRASH-TG zdarzenie=plik_wyslany seq=%lu rozmiar=%uB\n",
              (unsigned long)seq, (unsigned)fileSize);
  }

  if (textSeq <= seq && fileSeq <= seq) {
    logPrintf("lvl=INFO tag=CRASH-TG zdarzenie=raport_zakonczony seq=%lu\n", (unsigned long)seq);
  }
}

bool sendTelegramDocument() {
  if (!tgEnabled || tgBotToken.length() < 10 || tgChatId.length() < 3) return false;
  if (WiFi.status() != WL_CONNECTED) return false;
  // [v117] DUAL-FILE COMBINED: logReadCombinedTail łączy log_a+log_b jak panel WWW.
  if (!littlefsReady) {
    sendTelegramMessage("📋 Brak pliku logów na urządzeniu."); return false;
  }
  // [v119] TG-LOG-FULL: MAX_TG_DOC 2MB→8MB — pokrywa pełny FS_LIMIT (~7.8MB logów).
  // Telegram Bot API akceptuje do 50MB. Streaming 4KB + WDT reset co blok → bezpieczne.
  const size_t MAX_TG_DOC = 8UL * 1024UL * 1024UL;  // 8 MB (v118: 2MB, v117: 512KB)

  // [v118] Pobierz rozmiary z cache (aktualizowany po każdym flush i rotacji)
  size_t szA = g_logFileSizeA, szB = g_logFileSize;
  // Weryfikacja cache gdy 0 (np. tuż po restarcie)
  if (szA == 0 && LittleFS.exists(LOG_FILE_A)) {
    File _f = LittleFS.open(LOG_FILE_A, "r"); if (_f) { szA = _f.size(); _f.close(); }
  }
  if (szB == 0 && LittleFS.exists(LOG_FILE_B)) {
    File _f = LittleFS.open(LOG_FILE_B, "r"); if (_f) { szB = _f.size(); _f.close(); }
  }
  size_t totalLogSize = szA + szB;
  if (totalLogSize == 0) {
    sendTelegramMessage("📋 Brak pliku logów na urządzeniu."); return false;
  }
  size_t sendSize    = (totalLogSize > MAX_TG_DOC) ? MAX_TG_DOC : totalLogSize;
  bool wasTruncated  = (totalLogSize > MAX_TG_DOC);
  size_t combinedLen = sendSize;  // alias dla caption poniżej

  // Buduj nagłówek i stopkę multipart
  const String boundary = "ESP32TGBnd7x";
  // [v117] Caption pokazuje łączny rozmiar log_a+log_b
  String caption = "📋 log_a+log_b (";
  caption += String(combinedLen / 1024);
  caption += " KB";
  if (wasTruncated) {
    caption += ", ostatnie ";
    caption += String(MAX_TG_DOC / 1024);
    caption += " KB z ";
    caption += String(totalLogSize / 1024);
    caption += " KB total";
  }
  caption += ") - ";
  caption += logTime();
  String partHead =
    "--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" +
    tgChatId + "\r\n"
    "--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"caption\"\r\n\r\n" +
    caption + "\r\n"
    "--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"document\"; filename=\"aquarium_log.txt\"\r\n"
    "Content-Type: text/plain\r\n\r\n";
  String partTail = "\r\n--" + boundary + "--\r\n";
  size_t totalLen = partHead.length() + sendSize + partTail.length();

  // v32: wymuś świeże połączenie - sendDocument używa Connection:close
  // więc serwer zamknie gniazdo po odpowiedzi. Oznaczamy klienta jako
  // nieaktualny żeby tgEnsureConnected() zreconnectował przy następnym użyciu.
  // [v68-FIX-Z2] MEM-FREE: mierz DRAM przed/po stop() w sendTelegramDocument.
  // Ta ścieżka (Connection:close po multipart) jest kandydatem na anomalię 158.4 kB.
  {
    uint32_t _freeDocBefore = ESP.getFreeHeap();
    tgClient.stop();
    uint32_t _freeDocAfter = ESP.getFreeHeap();
    if (_freeDocAfter > _freeDocBefore + 4096) {
      logPrintf("lvl=INFO tag=MEM-FREE src=TG-DOC freeH_before=%luB freeH_after=%luB delta=+%ldB\n",
                (unsigned long)_freeDocBefore, (unsigned long)_freeDocAfter,
                (long)(_freeDocAfter - _freeDocBefore));
    }
  }
  tgClientReady = false;
  tgClient.setInsecure();
  tgClient.setTimeout(2);            // [v108] FIX-WDT-NETWORK-STORM: TLS 4→2s. TCP(5s)+TLS(2s)=7s < WDT 10s
  // setConnectionTimeout() — niedostępna w arduino-esp32. Używamy connect(ip, port, timeout_ms) [v103]
  // [v75] FIX-WDT-DNS: reuse cache IP z tgEnsureConnected (g_tgServerIP), re-resolve gdy potrzeba
  esp_task_wdt_reset();
  const char* TG_HOST_DOC = "api.telegram.org";
  const unsigned long TG_DNS_CACHE_MS_DOC = 300000UL;
  if (!g_tgIPValid || (millis() - g_tgIPTs > TG_DNS_CACHE_MS_DOC)) {
    IPAddress _resolved;
    // [v162] DIAG-DNSSTALL: ten sam pomiar co w tgEnsureConnected (patrz tam
    // komentarz) — sendTelegramDocument() to nowa (v159) ścieżka wywoływana
    // wewnątrz fsGuardPending, dokładnie w warunkach, gdy sieć może być zła.
    unsigned long _dnsT0doc = millis();
    bool _tgDnsOkDoc = WiFi.hostByName(TG_HOST_DOC, _resolved);
    // [v164] FIX-DNS-ZERO-IP: patrz komentarz w fbInitialize (ten sam bug).
    if (_tgDnsOkDoc && _resolved == IPAddress(0, 0, 0, 0)) {
      _tgDnsOkDoc = false;
      logPrintf("lvl=WARN tag=DNS-ZERO-IP site=sendTelegramDocument msg=\"hostByName() zwrocil 0.0.0.0, traktuje jako blad\"\n");
    }
    unsigned long _dnsMsDoc = millis() - _dnsT0doc;
    if (_dnsMsDoc >= DNS_STALL_THRESHOLD_MS) {
      g_dnsStallCount = g_dnsStallCount + 1;
      if (_dnsMsDoc > g_dnsStallMaxMs) g_dnsStallMaxMs = _dnsMsDoc;
      logPrintf("lvl=WARN tag=DNS-STALL site=sendTelegramDocument ms=%lu ok=%d cnt=%lu maxMs=%lu\n",
                _dnsMsDoc, (int)_tgDnsOkDoc,
                (unsigned long)g_dnsStallCount, (unsigned long)g_dnsStallMaxMs);
    }
    if (_tgDnsOkDoc) {
      g_tgServerIP = _resolved;
      g_tgIPValid  = true;
      g_tgIPTs     = millis();
    } else {
      g_tgIPValid = false;
    }
  }
  esp_task_wdt_reset();  // reset po DNS, świeże 10s okno dla TCP+TLS
  // [v108] FIX-WDT-NETWORK-STORM: identyczny fix jak tgEnsureConnected.
  // TCP 8000→5000ms + TLS setTimeout 4→2s = 7s max < WDT 10s.
  unsigned long _docConnStart = millis();
  esp_task_wdt_reset();  // [v108] świeże 10s okno przed TCP+TLS (max 7s łącznie)
  // [v227] FIX-TLS-HANDSHAKE-TIMEOUT: patrz komentarz w tgEnsureConnected (~linia 8309).
  tgClient.setHandshakeTimeout(4000);
  if (g_tgIPValid) {
    tgClient.connect(g_tgServerIP, 443, 5000);  // [v108] TCP timeout 8000→5000ms
  } else {
    tgClient.connect(TG_HOST_DOC, 443, 5000);   // [v108] TCP timeout 8000→5000ms
  }
  esp_task_wdt_reset();  // [v108] reset po connect() — TCP(5s)+TLS(2s)=7s < WDT 10s ✓
  while (!tgClient.connected() && (millis() - _docConnStart < 5000UL)) {  // [v108] 8000→5000ms
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(50));  // [v50] 50ms zamiast 100ms
  }
  if (!tgClient.connected()) {
    // [v118] brak combinedBuf — streaming bezpośrednio z pliku, nic do zwolnienia
    return false;
  }
  tgClientReady = true;
  tgClientLastUse = millis();
  esp_task_wdt_reset();

  // [v82] FIX-WDT-SNDTIMEO: ustaw SO_SNDTIMEO — ten sam bug #7356 dotyczy
  // sendTelegramDocument(), która strumieniuje do 512KB pliku przez tgClient.write().
  // Bez SO_SNDTIMEO: przy black-hole TCP write() wisi bez WDT reset → crash.
  {
    int _docSock = tgClient.fd();
    if (_docSock >= 0) {
      struct timeval _tv;
      _tv.tv_sec  = 8;
      _tv.tv_usec = 0;
      setsockopt(_docSock, SOL_SOCKET, SO_SNDTIMEO, &_tv, sizeof(_tv));
      setsockopt(_docSock, SOL_SOCKET, SO_RCVTIMEO, &_tv, sizeof(_tv));
    }
  }

  // [v83] FIX-TCP-KA: TCP Keepalive — spójny z tgEnsureConnected/fbConnect.
  // sendTelegramDocument używa "Connection: close" więc reconnect tu rzadki,
  // ale keepalive chroni przy dużych plikach (wielokrotne write() przez >5s).
  // [v147] FIX-TG-KA-WINDOW: idle=3s, interval=2s, count=3 — spójne z tgEnsureConnected.
  {
    bool _kaOk = applyTcpKeepalive(tgClient, 2, 1, 2); // [v167] KA-TIGHTEN-TG: idle=2s+intvl(1)×cnt(2)=4s (bylo 9s) - symetria z fix v163 dla FB
    logPrintf("lvl=INFO tag=DOC-KA keepalive=%s fd=%d\n", _kaOk ? "OK" : "FAIL", tgClient.fd());
  }

  // [v169] DIAG-TGDOC-THROUGHPUT: zmierz realny czas i przepustowość streamingu
  // sendSize bajtów przez TLS. Kontekst: [FLASH-STALL] site=fsGuard w logu z
  // urządzenia pokazał całe fsGuardPending (w tym ten wysyłkę) trwające ~29s dla
  // ~4.1 MB, blokując cały tgTaskFn na ten czas (stąd towarzyszący log
  // "ITER-BUDGET: skip TG-POLL" — bot nie odpowiada na komendy przez te ~29s).
  // Badania (ESP32 forum, mbedTLS software crypto) pokazują typową przepustowość
  // TLS-upload rzędu kilkuset kbps do ~1 Mbps na ESP32 — a więc kilkadziesiąt
  // sekund na kilka MB to w dużej mierze WŁAŚCIWOŚĆ sprzętu/mbedTLS, nie błąd w
  // tym kodzie. Ten pomiar ma to policzyć wprost (KB/s), żeby przy kolejnym
  // auto-sendzie było widać czy to "normalna" prędkość, czy realna degradacja
  // sieci (np. słaby sygnał WiFi, przeciążony router) dokładająca się do problemu.
  unsigned long _docSendT0 = millis();

  // Wyślij nagłówek HTTP
  tgClient.printf(
    "POST /bot%s/sendDocument HTTP/1.1\r\n"
    "Host: api.telegram.org\r\n"
    "Content-Type: multipart/form-data; boundary=%s\r\n"
    "Content-Length: %u\r\n"
    "Connection: close\r\n\r\n",
    tgBotToken.c_str(), boundary.c_str(), (unsigned)totalLen);

  tgClient.print(partHead);

  // [v118] STREAM-LOG: Strumieniuj log_a + log_b bezpośrednio z LittleFS (4KB bloki).
  // Brak ps_malloc(sendSize) — bufor 4KB z PSRAM (lub DRAM jako fallback).
  // skip = ile pominąć od początku [log_a, log_b] żeby zmieścić się w MAX_TG_DOC.
  {
    const size_t DOC_CHUNK = 4096;
    uint8_t* chBuf = (uint8_t*)ps_malloc(DOC_CHUNK);
    if (!chBuf) chBuf = (uint8_t*)malloc(DOC_CHUNK);
    if (!chBuf) {
      logPrintf("lvl=ERR tag=TG-DOC msg=\"Brak pamieci na bufor 4KB, abort\"\n");
      tgClient.stop(); tgClientReady = false;
      return false;
    }

    // Oblicz punkt startu w parze [log_a|log_b] (ogon = ostatnie sendSize bajtów)
    size_t skip  = (totalLogSize > MAX_TG_DOC) ? (totalLogSize - MAX_TG_DOC) : 0;
    size_t skipA = (skip < szA) ? skip : szA;
    size_t skipB = (skip > szA) ? (skip - szA) : 0;
    size_t leftA = szA - skipA;   // bajty do wysłania z log_a
    size_t leftB = szB - skipB;   // bajty do wysłania z log_b

    // ── log_a (archiwum) ──────────────────────────────────────
    if (leftA > 0 && LittleFS.exists(LOG_FILE_A)) {
      File fa = LittleFS.open(LOG_FILE_A, "r");
      if (fa) {
        fa.seek(skipA);
        size_t sent = 0;
        while (sent < leftA) {
          size_t toRead = (leftA - sent > DOC_CHUNK) ? DOC_CHUNK : (leftA - sent);
          size_t n = fa.read(chBuf, toRead);
          if (n == 0) break;
          size_t written = 0;
          while (written < n) {
            size_t w = tgClient.write(chBuf + written, n - written);
            if (w == 0) break;
            written += w;
          }
          if (written == 0) break;
          sent += written;
          esp_task_wdt_reset();  // reset co 4KB — WDT safe
        }
        fa.close();
      }
    }

    // ── log_b (aktywny) ───────────────────────────────────────
    if (leftB > 0 && LittleFS.exists(LOG_FILE_B)) {
      File fb = LittleFS.open(LOG_FILE_B, "r");
      if (fb) {
        fb.seek(skipB);
        size_t sent = 0;
        while (sent < leftB) {
          size_t toRead = (leftB - sent > DOC_CHUNK) ? DOC_CHUNK : (leftB - sent);
          size_t n = fb.read(chBuf, toRead);
          if (n == 0) break;
          size_t written = 0;
          while (written < n) {
            size_t w = tgClient.write(chBuf + written, n - written);
            if (w == 0) break;
            written += w;
          }
          if (written == 0) break;
          sent += written;
          esp_task_wdt_reset();
        }
        fb.close();
      }
    }

    free(chBuf);
  }
  // [v169] DIAG-TGDOC-THROUGHPUT: koniec pomiaru samego streamingu (bez oczekiwania
  // na odpowiedź poniżej — to liczone osobno, żeby odróżnić "wolna sieć/TLS" od
  // "Telegram wolno odpowiada"). sendSize to bajty do wysłania (patrz wyżej).
  {
    unsigned long _docSendMs = millis() - _docSendT0;
    float _kBps = (_docSendMs > 0) ? ((float)sendSize / 1024.0f) / ((float)_docSendMs / 1000.0f) : 0.0f;
    logPrintf("lvl=INFO tag=DOC-SPEED sent=%uB ms=%lu kBps=%.1f\n",
              (unsigned)sendSize, _docSendMs, _kBps);
  }
  tgClient.print(partTail);

  // Odczytaj odpowiedź
  // [v59] Bufor odpowiedzi w PSRAM – eliminuje reserve(2048) z DRAM
  bool ok = false;
  unsigned long t0 = millis();
  const size_t TDOC_RESP_CAP = 2048;
  char* respBuf = (char*)psramAllocSafe(TDOC_RESP_CAP + 1);
  size_t respLen = 0;
  String respFb;  // fallback gdy brak PSRAM
  if (respBuf) {
    respBuf[0] = '\0';
  } else {
    respFb.reserve(512);  // mniejszy fallback gdy PSRAM niedostępny
  }

  while (tgClient.connected() && millis() - t0 < 12000UL) {
    while (tgClient.available()) {
      char c = tgClient.read();
      if (respBuf) {
        if (respLen >= TDOC_RESP_CAP - 1) {
          size_t half = TDOC_RESP_CAP / 2;          // sliding window – memmove jak w tgPost
          memmove(respBuf, respBuf + half, respLen - half + 1);
          respLen -= half;
        }
        respBuf[respLen++] = c;
        respBuf[respLen] = '\0';
      } else {
        if (respFb.length() >= 500) respFb.remove(0, 250);
        respFb += c;
      }
    }
    delay(10);
    esp_task_wdt_reset();
  }
  // Serwer zamknął gniazdo (Connection:close) -> wymuszamy reconnect przy następnym użyciu
  tgClient.stop();
  tgClientReady = false;

  if (respBuf) {
    ok = (strstr(respBuf, "\"ok\":true") != nullptr);
    free(respBuf);
  } else {
    ok = (respFb.indexOf("\"ok\":true") >= 0);
  }
  return ok;
}

// ─────────────────────────────────────────────────────────────
// TELEGRAM - raport temperatur 🌡️
// ─────────────────────────────────────────────────────────────
String buildTelegramTempReport() {
  struct tm ti;
  // [v123-PSRAM] tb[280] (stos) → psramAllocSafe (PSRAM, alokacja raz przy 1. wywołaniu,
  // potem trwały wskaźnik na całe uptime). Bezpieczne: funkcja wywoływana wyłącznie
  // z tgTaskFn (Core 0, pollTelegramCommands()) — brak ryzyka współbieżności. Patrz CHANGELOG v123.
  static char* tb = nullptr;
  const size_t TB_CAP = 280;
  if (!tb) tb = (char*)psramAllocSafe(TB_CAP);
  if (!tb) return "[ERR] Brak pamięci na raport temperatur";
  String timeStr = "??:??";
  if (getLocalTimePL(&ti)) {
    char buf[9]; snprintf(buf, 9, "%02d:%02d:%02d", ti.tm_hour, ti.tm_min, ti.tm_sec);
    timeStr = String(buf);
  }
  // status termiczny
  const char* zone1 = derating1ActiveGlobal ? " [WARN]️ DERATING" : " [OK] OK";
  const char* zone2 = derating2ActiveGlobal ? " [WARN]️ DERATING" : " [OK] OK";
  snprintf(tb, TB_CAP,
    "🌡️ <b>Temperatury - %s</b>\n\n"
    "🔥 Płyta LED 1: <b>%.1f°C</b>%s\n"
    "🔥 Płyta LED 2: <b>%.1f°C</b>%s\n"
    "💧 Woda:        <b>%.1f°C</b>\n\n"
    "[WARN]️ Próg deratingu: %.0f°C\n"
    "🚨 Wyłączenie awaryjne: %.0f°C",
    timeStr.c_str(),
    tempPlate1, zone1,
    tempPlate2, zone2,
    tempWater,
    TEMP_START_DERATING,
    85.0f);
  return String(tb);
}

// ─────────────────────────────────────────────────────────────
// TELEGRAM - raport energii ⚡
// ─────────────────────────────────────────────────────────────
String buildTelegramEnergyReport() {
  uint16_t p[5];
  for (int i = 0; i < 5; i++) p[i] = (uint16_t)getBrightnessForSection(backupBrightnessComposite, i);
  float totW = getTotalLEDPower(p);
  int ledH = (int)(ledOnMinutesToday / 60);
  int ledM = (int)(ledOnMinutesToday % 60);
  float costToday = energyTodayWh / 1000.0f * kwhPrice;
  float costWeek  = energyWeekWh  / 1000.0f * kwhPrice;
  float costMonth = energyMonthWh / 1000.0f * kwhPrice;
  // [v123-PSRAM] tb[420] (stos) → psramAllocSafe. Wyłącznie z tgTaskFn. Patrz CHANGELOG v123.
  static char* tb = nullptr;
  const size_t TB_CAP = 420;
  if (!tb) tb = (char*)psramAllocSafe(TB_CAP);
  if (!tb) return "[ERR] Brak pamięci na raport energii";
  snprintf(tb, TB_CAP,
    "⚡ <b>Energia LED</b>\n\n"
    "🔌 Moc teraz: <b>%.1f W</b>\n\n"
    "📅 Dziś:     <b>%.1f Wh</b>  (%.2f zł)\n"
    "📅 Tydzień:  <b>%.1f Wh</b>  (%.2f zł)\n"
    "📅 Miesiąc:  <b>%.1f Wh</b>  (%.2f zł)\n\n"
    "⏱ Czas LED dziś: %dh %02dm\n"
    "🎯 Aktywacje MIN LUX dziś: %u\n"
    "⚡ Szczytowa moc dziś: %.1f W\n"
    "💰 Cena kWh: %.2f zł",
    totW,
    energyTodayWh, costToday,
    energyWeekWh,  costWeek,
    energyMonthWh, costMonth,
    ledH, ledM,
    minLuxActivToday,
    peakPowerWToday,
    kwhPrice);
  return String(tb);
}

// ─────────────────────────────────────────────────────────────
// TELEGRAM - raport lux i światła 🔆
// ─────────────────────────────────────────────────────────────
String buildTelegramLightReport() {
  struct tm ti;
  // [v123-PSRAM] tb[380] (stos) → psramAllocSafe. Wyłącznie z tgTaskFn. Patrz CHANGELOG v123.
  static char* tb = nullptr;
  const size_t TB_CAP = 380;
  if (!tb) tb = (char*)psramAllocSafe(TB_CAP);
  if (!tb) return "[ERR] Brak pamięci na raport światła";
  String timeStr = "??:??";
  if (getLocalTimePL(&ti)) {
    char buf[9]; snprintf(buf, 9, "%02d:%02d", ti.tm_hour, ti.tm_min);
    timeStr = String(buf);
  }
  uint16_t p[5];
  for (int i = 0; i < 5; i++) p[i] = (uint16_t)getBrightnessForSection(backupBrightnessComposite, i);
  int avgPwm = 0;
  for (int i = 0; i < 5; i++) avgPwm += p[i];
  avgPwm /= 5;
  const char* nightStr = isNightGlobal ? "🌙 NOC" : (inLightingWindowGlobal ? "☀️ ŚWIECI" : "🌿 PRZERWA");
  const char* minLuxStr = minLuxModeActive ? "aktywny 🟢" : (minLuxModeEnabled ? "gotowy 🟡" : "wyłączony ⚫");
  snprintf(tb, TB_CAP,
    "🔆 <b>Lux & Światło - %s</b>\n\n"
    "☀️ Lux pokój: <b>%.0f lx</b>  %s\n"
    "💧 Lux woda: <b>%.0f lx</b>  %s\n\n"
    "💡 Śr. PWM:  <b>%d / 1023</b> (%.0f%%)\n"
    "🔲 Kanały: B=%d FS=%d FSB=%d N=%d C=%d\n\n"
    "🕐 Faza: <b>%s</b>\n"
    "🌿 MIN LUX: %s\n"
    "🎯 Cel MIN LUX: %.0f lx\n"
    "🌅 Zachód: %02d:%02d",
    timeStr.c_str(),
    luxPokojowy, czujnikPokojowyAktywny ? "[OK]" : "[ERR]",
    luxNadWoda,  czujnikNadWodaAktywny  ? "[OK]" : "[ERR]",
    avgPwm, avgPwm / 1023.0f * 100.0f,
    p[0], p[1], p[2], p[3], p[4],
    nightStr,
    minLuxStr,
    minLuxDayTarget,
    sunsetMinutes / 60, sunsetMinutes % 60);
  return String(tb);
}

// ─────────────────────────────────────────────────────────────
// TELEGRAM - raport adaptacji 🧠
// ─────────────────────────────────────────────────────────────
String buildTelegramAdaptReport() {
  float avgReduction = (statystyki.liczbaPomiarow > 0)
    ? (statystyki.sredniaRedukcja / statystyki.liczbaPomiarow) : 0.0f;
  // [v123-PSRAM] tb[380] (stos) → psramAllocSafe. Wyłącznie z tgTaskFn. Patrz CHANGELOG v123.
  static char* tb = nullptr;
  const size_t TB_CAP = 380;
  if (!tb) tb = (char*)psramAllocSafe(TB_CAP);
  if (!tb) return "[ERR] Brak pamięci na raport adaptacji";
  const char* modeStr = "OFF";
  if (uzywajCzujnikaPokojowego && uzywajCzujnikaNadWoda) modeStr = "Pokój + Woda";
  else if (uzywajCzujnikaPokojowego) modeStr = "Pokojowy";
  snprintf(tb, TB_CAP,
    "🧠 <b>Adaptacja</b>\n\n"
    "🔧 Tryb czujników: <b>%s</b>\n"
    "📊 Regulacja adaptacyjna: %s\n"
    "📖 Uczenie się: %s\n\n"
    "📚 Próbek uczenia: <b>%u</b>\n"
    "🪟 Transmisja szyby: <b>%.1f%%</b>\n"
    "📉 Śr. redukcja LED: <b>%.1f%%</b>\n"
    "🔼 Korekty MIN: %u  🔽 Korekty MAX: %u\n\n"
    "🌡️ Śr. lux dzienny: %.0f lx\n"
    "🔆 Lux·h przy roślinach dziś: %.0f lx·h",
    modeStr,
    regulacjaAdaptacyjnaWlaczona ? "[OK] WŁ" : "⚫ WYŁ",
    uczenieSieWlaczone ? "[OK] WŁ" : "⚫ WYŁ",
    adaptacja.liczbaProbek,
    adaptacja.nauczonaTransmisja * 100.0f,
    avgReduction,
    statystyki.korektyMinimum,
    statystyki.korektyMaksimum,
    adaptacja.sredniaLuxDzien,
    luxHoursTodayWater);
  return String(tb);
}

// ─────────────────────────────────────────────────────────────
// TELEGRAM - wyślij potwierdzenie czyszczenia logów 🗑️
// ─────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────
// TELEGRAM - raport historii czujników  v30-0
// ─────────────────────────────────────────────────────────────
String buildTelegramSensorHistory() {
  struct tm ti;
  if (!getLocalTimePL(&ti)) return "❓ Brak czasu NTP";
  // [v123-PSRAM] tb[400] (stos) → psramAllocSafe. Wyłącznie z tgTaskFn. Patrz CHANGELOG v123.
  static char* tb = nullptr;
  const size_t TB_CAP = 400;
  if (!tb) tb = (char*)psramAllocSafe(TB_CAP);
  if (!tb) return "[ERR] Brak pamięci na raport historii";
  snprintf(tb, TB_CAP,
    "🌊 <b>Historia czujników</b>  %02d:%02d\n\n"
    "🌡️ <b>Temperatura:</b>\n"
    "  Woda: %.1f°C\n"
    "  Płyta 1: %.1f°C  |  Płyta 2: %.1f°C\n\n"
    "🔆 <b>Oświetlenie:</b>\n"
    "  Pokój: %.0f lux\n"
    "  Nad wodą: %.0f lux\n\n"
    "📊 <b>LED:</b>\n"
    "  Stan: %s  |  Tryb: %s\n"
    "  Pomiarów adaptacji: %u",
    ti.tm_hour, ti.tm_min,
    tempWater, tempPlate1, tempPlate2,
    luxPokojowy, luxNadWoda,
    power ? "WŁ 💡" : "WYŁ ⚫",
    tryb  ? "AUTO 🤖" : "MANUAL 🔧",
    statystyki.liczbaPomiarow);
  return String(tb);
}

// ─────────────────────────────────────────────────────────────
// TELEGRAM - raport harmonogramu świateł  v30-0
// ─────────────────────────────────────────────────────────────
String buildTelegramScheduleReport() {
  struct tm ti;
  if (!getLocalTimePL(&ti)) return "❓ Brak czasu NTP";
  // [v123-PSRAM] tb[480] (stos) → psramAllocSafe. Wyłącznie z tgTaskFn. Patrz CHANGELOG v123.
  static char* tb = nullptr;
  const size_t TB_CAP = 480;
  if (!tb) tb = (char*)psramAllocSafe(TB_CAP);
  if (!tb) return "[ERR] Brak pamięci na raport harmonogramu";
  bool isWeekend = (ti.tm_wday == 0 || ti.tm_wday == 6);
  int32_t morningStart = isWeekend ? MORNING_ON_START_WEEKEND : MORNING_ON_START_WEEKDAY;
  snprintf(tb, TB_CAP,
    "⏰ <b>Harmonogram świateł</b>  %02d:%02d\n\n"
    "🌅 Start poranny (%s): <b>%02d:%02d</b>\n"
    "☀️ Przerwa południowa od: <b>%02d:%02d</b>\n"
    "🌆 Wygaszanie od: <b>%02d:%02d</b>\n"
    "💫 Czas rampy: <b>%ld min</b>\n\n"
    "⚡ Stan: Noc=%s | Okno=%s | Rampa=%s",
    ti.tm_hour, ti.tm_min,
    isWeekend ? "weekend" : "dzień roboczy",
    (int)(morningStart / 60), (int)(morningStart % 60),
    (int)(MIDDAY_OFF_LOCAL  / 60), (int)(MIDDAY_OFF_LOCAL  % 60),
    (int)(EVENING_OFF_START / 60), (int)(EVENING_OFF_START % 60),
    (long)fadeMinutes,
    isNightGlobal          ? "TAK 🌙" : "NIE ☀️",
    inLightingWindowGlobal ? "TAK 💡" : "NIE",
    rampScheduleActive     ? "TAK ▶️" : "NIE ⏸");
  return String(tb);
}

bool sendTelegramConfirmClear() {
  if (!tgEnabled || tgBotToken.length() < 10 || tgChatId.length() < 3) return false;
  size_t logSz = 0;
  // [v102] DUAL-FILE: pokaż łączny rozmiar obu plików — cache bez otwierania pliku
  logSz = g_logFileSizeA + g_logFileSize;
  char sizeStr[20];
  if (logSz > 1048576) snprintf(sizeStr, 20, "%.1f MB", logSz / 1048576.0f);
  else if (logSz > 1024) snprintf(sizeStr, 20, "%.0f KB", logSz / 1024.0f);
  else snprintf(sizeStr, 20, "%u B", (unsigned)logSz);
  String payload =
    "{\"chat_id\":\"" + tgChatId + "\","
    "\"text\":\"🗑️ <b>Wyczyścić logi?</b>\\n\\nRozmiar pliku: " + String(sizeStr) + "\\nOperacja jest nieodwracalna.\","
    "\"parse_mode\":\"HTML\","
    "\"reply_markup\":{\"inline_keyboard\":[[{"
    "\"text\":\"[OK] TAK - wyczyść\",\"callback_data\":\"clryes\""
    "},{\"text\":\"[ERR] NIE - anuluj\",\"callback_data\":\"clrno\""
    "}]]}}";
  String resp = tgPost("/bot" + tgBotToken + "/sendMessage", payload);  // v32
  return (resp.indexOf("\"ok\":true") >= 0);
}

// ─────────────────────────────────────────────────────────────
// TELEGRAM - wyślij potwierdzenie restartu 🔄
// ─────────────────────────────────────────────────────────────
bool sendTelegramConfirmRestart() {
  if (!tgEnabled || tgBotToken.length() < 10 || tgChatId.length() < 3) return false;
  String payload =
    "{\"chat_id\":\"" + tgChatId + "\","
    "\"text\":\"🔄 <b>Restartować ESP32?</b>\\n\\nUrządzenie będzie niedostępne przez ~10 sekund.\","
    "\"parse_mode\":\"HTML\","
    "\"reply_markup\":{\"inline_keyboard\":[[{"
    "\"text\":\"[OK] TAK - restartuj\",\"callback_data\":\"rstyes\""
    "},{\"text\":\"[ERR] NIE - anuluj\",\"callback_data\":\"rstno\""
    "}]]}}";
  String resp = tgPost("/bot" + tgBotToken + "/sendMessage", payload);  // v32
  return (resp.indexOf("\"ok\":true") >= 0);
}

// ─────────────────────────────────────────────────────────────
// TELEGRAM - wyślij menu inline  v30-0: auto-clean + 7 wierszy
// ─────────────────────────────────────────────────────────────
bool sendTelegramInlineMenu() {
  if (!tgEnabled || tgBotToken.length() < 10 || tgChatId.length() < 3) return false;

  tgDeleteAllHistory();  // kasuje tylko poprzednią wiadomość menu (1 call)
  esp_task_wdt_reset();

  // [v123-PSRAM] headerBuf[180] + _pl[1200] (stos, razem 1380B) → psramAllocSafe.
  // Wyłącznie z tgTaskFn. Patrz CHANGELOG v123.
  static char* headerBuf = nullptr;
  const size_t HEADER_CAP = 180;
  if (!headerBuf) headerBuf = (char*)psramAllocSafe(HEADER_CAP);
  if (!headerBuf) { logPrintln("lvl=ERR tag=TG msg=\"inlineMenu blad: brak pamieci (headerBuf)\""); return false; }

  struct tm ti;
  if (getLocalTimePL(&ti)) {
    uint16_t p[5];
    for (int i = 0; i < 5; i++) p[i] = (uint16_t)getBrightnessForSection(backupBrightnessComposite, i);
    float totW = getTotalLEDPower(p);
    snprintf(headerBuf, HEADER_CAP,
      "🐟 <b>Ryby LED</b>  |  %02d:%02d\n"
      "💡 %s  🤖 %s  ⚡ %.1fW\n"
      "Wybierz akcję:",
      ti.tm_hour, ti.tm_min,
      power ? "LED: WŁ" : "LED: WYŁ",
      tryb  ? "AUTO"    : "MANUAL",
      totW);
  } else {
    snprintf(headerBuf, HEADER_CAP, "🐟 <b>Ryby LED</b>\nWybierz akcję:");
  }

  String ledBtn   = power     ? "💡 LED: WŁ  ->wyłącz" : "💡 LED: WYŁ ->włącz";
  String trybBtn  = tryb      ? "🤖 Tryb: AUTO ->MAN"  : "🤖 Tryb: MAN ->AUTO";
  String notifBtn = tgEnabled ? "🔕 Wyłącz powiad."   : "🔔 Włącz powiad.";
  ledBtn.replace("\"", "\\\""); trybBtn.replace("\"", "\\\""); notifBtn.replace("\"", "\\\"");
  String safeHeader = String(headerBuf);
  safeHeader.replace("\"", "\\\""); safeHeader.replace("\n", "\\n");

  // FIX-v44: snprintf zamiast ~15 konkatenacji String + - brak tymczasowych obiektów
  static char* _pl = nullptr;
  const size_t PL_CAP = 1200;
  if (!_pl) _pl = (char*)psramAllocSafe(PL_CAP);
  if (!_pl) { logPrintln("lvl=ERR tag=TG msg=\"inlineMenu blad: brak pamieci (_pl)\""); return false; }
  snprintf(_pl, PL_CAP,
    "{\"chat_id\":\"%s\","
    "\"text\":\"%s\","
    "\"parse_mode\":\"HTML\","
    "\"reply_markup\":{\"inline_keyboard\":["
    "[{\"text\":\"📊 Status\",\"callback_data\":\"status\"}"
    ",{\"text\":\"🌡️ Temperatury\",\"callback_data\":\"temp\"}],"
    "[{\"text\":\"⚡ Energia\",\"callback_data\":\"energy\"}"
    ",{\"text\":\"🔆 Lux & Światło\",\"callback_data\":\"light\"}],"
    "[{\"text\":\"🧠 Adaptacja\",\"callback_data\":\"adapt\"}"
    ",{\"text\":\"📋 Pobierz logi\",\"callback_data\":\"logs\"}],"
    "[{\"text\":\"%s\",\"callback_data\":\"ledtog\"}"
    ",{\"text\":\"%s\",\"callback_data\":\"trybtog\"}],"
    "[{\"text\":\"🌊 Historia czujników\",\"callback_data\":\"sensorhist\"}"
    ",{\"text\":\"⏰ Harmonogram\",\"callback_data\":\"schedule\"}],"
    "[{\"text\":\"%s\",\"callback_data\":\"notiftog\"}"
    ",{\"text\":\"🗑️ Wyczyść logi\",\"callback_data\":\"clrlogs\"}],"
    "[{\"text\":\"🔄 Restart ESP\",\"callback_data\":\"restart\"}]"
    "]}}",
    tgChatId.c_str(), safeHeader.c_str(),
    ledBtn.c_str(), trybBtn.c_str(), notifBtn.c_str());
  String payload(_pl);

  String resp = tgPost("/bot" + tgBotToken + "/sendMessage", payload);  // v32: stały tgClient
  bool ok = (resp.indexOf("\"ok\":true") >= 0);
  if (ok) {
    long newId = parseTelegramMsgId(resp);
    tgPushMsgId(newId);
  } else {
    logPrintln("lvl=ERR tag=TG msg=\"inlineMenu blad\"");
  }
  return ok;
}

// ─────────────────────────────────────────────────────────────
// TELEGRAM - odpowiedz na callback_query (wymagane przez API)
// ─────────────────────────────────────────────────────────────
void answerTelegramCallback(const String& callbackId) {
  if (tgBotToken.length() < 10) return;
  String payload = "{\"callback_query_id\":\"" + callbackId + "\"}";
  tgPost("/bot" + tgBotToken + "/answerCallbackQuery", payload);  // v32: stały tgClient
}

// ─────────────────────────────────────────────────────────────
// TELEGRAM - buduj raport statusu (używany przez przycisk 📊)
// ─────────────────────────────────────────────────────────────
String buildTelegramStatusReport() {
  struct tm ti;
  if (!getLocalTimePL(&ti)) return "❓ Brak czasu NTP";
  uint16_t p[5];
  for (int i = 0; i < 5; i++)
    p[i] = (uint16_t)getBrightnessForSection(backupBrightnessComposite, i);
  float avgReduction = (statystyki.liczbaPomiarow > 0)
    ? (statystyki.sredniaRedukcja / statystyki.liczbaPomiarow) : 0.0f;
  // [v123-PSRAM] tb[350] (stos) → psramAllocSafe. Wyłącznie z tgTaskFn. Patrz CHANGELOG v123.
  static char* tb = nullptr;
  const size_t TB_CAP = 350;
  if (!tb) tb = (char*)psramAllocSafe(TB_CAP);
  if (!tb) return "[ERR] Brak pamięci na raport statusu";
  snprintf(tb, TB_CAP,
    "🐟 <b>Status Ryby LED</b>\n"
    "🕐 %02d:%02d:%02d\n\n"
    "⚡ Moc: %.1fW\n"
    "💡 Energia dziś: %.1f Wh | Tydzień: %.1f Wh\n\n"
    "🌡️ Woda: %.1f°C | Płyty: %.1f/%.1f°C\n"
    "🔆 Lux: %.0f (pokój) / %.0f (woda)\n\n"
    "🧠 Pomiarów: %u | Redukcja: %.1f%%\n"
    "  Transmisja szyby: %.1f%%",
    ti.tm_hour, ti.tm_min, ti.tm_sec,
    getTotalLEDPower(p),
    energyTodayWh, energyWeekWh,
    tempWater, tempPlate1, tempPlate2,
    luxPokojowy, luxNadWoda,
    statystyki.liczbaPomiarow, avgReduction,
    adaptacja.nauczonaTransmisja * 100.0f);
  return String(tb);
}

// ─────────────────────────────────────────────────────────────
// TELEGRAM - polling komend i callback_query
// Obsługuje: /start, /sendlogs, /status + przyciski inline
// Wywołuj co ~30s z loop()
// ─────────────────────────────────────────────────────────────
void pollTelegramCommands() {
  if (!tgEnabled)                    { return; }
  if (tgBotToken.length() < 10)      { return; }
  if (WiFi.status() != WL_CONNECTED) { return; }
  unsigned long nowMs = millis();
  if (nowMs - tgLastPollMs < 30000UL) return;   // co 30s
  tgLastPollMs = nowMs;

  String reqBody = "{\"timeout\":0,\"allowed_updates\":[\"message\",\"callback_query\"]";
  if (tgLastUpdateId >= 0) reqBody += ",\"offset\":" + String(tgLastUpdateId + 1);
  reqBody += "}";

  String body = tgPost("/bot" + tgBotToken + "/getUpdates", reqBody);  // v32: stały tgClient
  if (body.indexOf("\"ok\":true") < 0) return;

  int pos = 0;
  while (true) {
    // Znajdź kolejne update_id
    int uidPos = body.indexOf("\"update_id\":", pos);
    if (uidPos < 0) break;
    uidPos += 12;
    long uid = body.substring(uidPos, uidPos + 12).toInt();
    tgLastUpdateId = max(tgLastUpdateId, uid);
    g_tgLastUpdateId = tgLastUpdateId;  // [FIX-v149-TG-OFFSET-RTC] zapis do RTC RAM ZANIM
                                         // wykonamy jakąkolwiek akcję (np. RESTART_DO niżej) —
                                         // update musi być "skwitowany" przed restartem, inaczej
                                         // Telegram odeśle go ponownie po reboocie

    // Znajdź koniec tego obiektu update (szukamy następnego update_id jako granicy)
    // [OK] FIX-v39-3: zamiast body.substring() -> chunk (nowy String przy każdym update)
    // parsujemy bezpośrednio na body używając zakresu [chunkStart, chunkEnd].
    // Eliminuje alokację ~4KB Stringa przy każdym update -> brak dziur w stercie.
    // [FIX-BUG2] Usunięto duplikat: pierwotna deklaracja 'int nextUid' była natychmiast
    // nadpisywana identycznym indexOf — artifact po refaktoringu v39-3. Zostało jedno wywołanie.
    int nextUid = body.indexOf("\"update_id\":", uidPos);
    int chunkStart = uidPos;
    int chunkEnd   = (nextUid > 0) ? nextUid : (int)body.length();

    // ── callback_query: kliknięcie przycisku inline ──
    int cbqPos = body.indexOf("\"callback_query\"", chunkStart);
    if (cbqPos >= 0 && cbqPos < chunkEnd) {
      // Szukaj "id":" ZA kluczem callback_query
      String cbId = "";
      int idPos = body.indexOf("\"id\":\"", cbqPos);
      if (idPos >= 0 && idPos < chunkEnd) {
        idPos += 6;
        int idEnd = body.indexOf("\"", idPos);
        if (idEnd > idPos) cbId = body.substring(idPos, idEnd);
      }
      // "data":"logs" itp.
      int dataPos = body.indexOf("\"data\":\"", cbqPos);
      if (dataPos >= 0 && dataPos < chunkEnd) {
        dataPos += 8;
        int dataEnd = body.indexOf("\"", dataPos);
        String cbData = body.substring(dataPos, dataEnd);
        logPrintf("lvl=INFO tag=POLL cbData=%s uid=%ld freeH=%luB\n", // [v81-DIAG]
                  cbData.c_str(), uid, (unsigned long)ESP.getFreeHeap());
              answerTelegramCallback(cbId);
        if      (cbData == "logs")       { tgDeferredCmd = TG_LOGS; }
        else if (cbData == "status")     { tgDeferredCmd = TG_STATUS; }
        else if (cbData == "temp")       { tgDeferredCmd = TG_TEMP; }
        else if (cbData == "energy")     { tgDeferredCmd = TG_ENERGY; }
        else if (cbData == "light")      { tgDeferredCmd = TG_LIGHT; }
        else if (cbData == "adapt")      { tgDeferredCmd = TG_ADAPT; }
        else if (cbData == "ledtog")     { tgDeferredCmd = TG_LED_TOGGLE; }
        else if (cbData == "trybtog")    { tgDeferredCmd = TG_TRYB_TOGGLE; }
        else if (cbData == "clrlogs")    { tgDeferredCmd = TG_CLR_CONFIRM; }
        else if (cbData == "clryes")     { tgDeferredCmd = TG_CLR_DO; }
        else if (cbData == "clrno")      { sendTelegramMessage("[ERR] Anulowano czyszczenie logów.", false); }
        else if (cbData == "restart")    { tgDeferredCmd = TG_RESTART_CONFIRM; }
        else if (cbData == "rstyes")     { tgDeferredCmd = TG_RESTART_DO; }
        else if (cbData == "rstno")      { sendTelegramMessage("[ERR] Anulowano restart.", false); }
        else if (cbData == "sensorhist") { tgDeferredCmd = TG_SENSOR_HIST; }
        else if (cbData == "schedule")   { tgDeferredCmd = TG_SCHEDULE; }
        else if (cbData == "notiftog")   { tgDeferredCmd = TG_NOTIF_TOGGLE; }
        else                             { tgDeferredCmd = TG_MENU; }
      }
    }
    // ── message: wiadomość tekstowa ──
    else {
      int textPos = body.indexOf("\"text\":\"", chunkStart);
      if (textPos >= 0 && textPos < chunkEnd) {
        textPos += 8;
        int textEnd = body.indexOf("\"", textPos);
        String cmd = body.substring(textPos, textEnd);
        if (cmd.startsWith("/start") || cmd.startsWith("/help")) {
          tgDeferredCmd = TG_MENU;
        } else if (cmd.startsWith("/sendlogs") || cmd.startsWith("/logi")) {
          tgDeferredCmd = TG_LOGS;
        } else if (cmd.startsWith("/status")) {
          tgDeferredCmd = TG_STATUS;
        } else if (cmd.startsWith("/temp")) {
          tgDeferredCmd = TG_TEMP;
        } else if (cmd.startsWith("/energia") || cmd.startsWith("/energy")) {
          tgDeferredCmd = TG_ENERGY;
        } else if (cmd.startsWith("/lux") || cmd.startsWith("/swiatlo")) {
          tgDeferredCmd = TG_LIGHT;
        } else if (cmd.startsWith("/adapt")) {
          tgDeferredCmd = TG_ADAPT;
        } else if (cmd.startsWith("/historia") || cmd.startsWith("/czujniki")) {
          tgDeferredCmd = TG_SENSOR_HIST;
        } else if (cmd.startsWith("/harmonogram") || cmd.startsWith("/schedule")) {
          tgDeferredCmd = TG_SCHEDULE;
        } else if (cmd.startsWith("/notif") || cmd.startsWith("/powiadomienia")) {
          tgDeferredCmd = TG_NOTIF_TOGGLE;
        } else if (cmd.startsWith("/testdaily")) {
          // [v227] TEST-ONLY (plan flash-freeze, Partia 4) - do usunięcia po testach
          tgDeferredCmd = TG_TEST_DAILY_SUMMARY;
        } else if (cmd.startsWith("/update")) {
          // [4.1.0 OTA-GITHUB] Etap 1 planu upgrade — OTA przez GitHub Releases
          tgDeferredCmd = TG_OTA_UPDATE;
        } else {
          tgDeferredCmd = TG_MENU;  // nieznana komenda -> pokaż menu
        }
      }
    }

    pos = uidPos + 10;
  }
  // [OK] FIX-v39-3: explicite zwolnij body natychmiast po parsowaniu.
  // String() bez argumentów to pusty string bez alokacji - destruktor
  // poprzedniego bufora zwalnia pamięć natychmiast, zanim tgTask
  // wykona kolejne operacje sieciowe.
  body = String();

  // ── v30-2: jeśli brak aktywnego menu w czacie -> zaplanuj wysłanie ──
  // (wywoła się po każdym pollu gdy czat jest pusty lub menu zniknęło)
  if (tgLastMenuMsgId < 0 && tgDeferredCmd == TG_NONE) {
    tgDeferredCmd = TG_MENU;
  }
}

void logDailySummary() {
  PRE_RESET_CP("DAILY-SUMMARY");  // [v71]
  // [OK] FIX BUG-O: Używaj prawdziwego zegara - podsumowanie o godzinie 00:00 (nie 24h po restarcie)
  static int lastSummaryDay = -1;
  struct tm ti;
  if (!getLocalTimePL(&ti)) return;  // NTP nie zsynchronizowany - pomiń

  // [FIX-v165-DAYROLL] Poprzednio wymagane było trafienie DOKŁADNIE w minutę
  // 00:00 (tm_hour==0 && tm_min==0). Jeśli loop() zawiesił się w tym jednym
  // 60-sekundowym oknie (reconnect WiFi, zapis LittleFS, WDT, itp.), całe
  // rolowanie dnia było pomijane -> energyTodayWh nigdy się nie zerowało ->
  // energyWeekWh/energyMonthWh nie odklejały się od energyTodayWh (tydzień
  // i miesiąc pokazywały to samo co dziś, mimo upływu wielu dni).
  // Teraz wykrywamy wprost zmianę tm_yday - odpali się najpóźniej przy
  // pierwszej iteracji loop() po zmianie dnia, niezależnie od godziny.
  // [v227] TEST-ONLY (plan flash-freeze, Partia 4) - do usunięcia po testach.
  // Komenda /testdaily ustawia tę flagę; tu ją konsumujemy i obchodzimy
  // normalny warunek "czy zmienił się dzień", żeby wymusić jednorazowe
  // wykonanie funkcji (razem z esp_task_wdt_reset() z Partii 3) bez
  // czekania na naturalną północ. Reszta logiki funkcji - bez zmian.
  bool forcedTest = g_forceDailySummaryTest;
  if (forcedTest) {
    g_forceDailySummaryTest = false;
    logPrintf("lvl=INFO tag=SUMMARY msg=\"Wymuszony test (v227) - pomijam warunek zmiany dnia\"\n");
  }

  if (lastSummaryDay < 0) {
    lastSummaryDay = ti.tm_yday;  // pierwsza inicjalizacja po starcie - nie licz jako zmianę dnia
    if (!forcedTest) return;
  }
  if (ti.tm_yday == lastSummaryDay && !forcedTest) return;  // już wykonano dla bieżącego dnia
  lastSummaryDay = ti.tm_yday;

  // Oblicz zaoszczędzoną energię (uproszczone: każda korekta MIN = pełna praca bez niej)
  float avgReduction = (statystyki.liczbaPomiarow > 0)
    ? (statystyki.sredniaRedukcja / statystyki.liczbaPomiarow)
    : 0.0f;

  uint16_t curPwm[5];
  for (int i = 0; i < 5; i++) curPwm[i] = (uint16_t)getBrightnessForSection(backupBrightnessComposite, i);
  float totW = getTotalLEDPower(curPwm);

  logPrintf("lvl=INFO tag=SUMMARY date=%04d-%02d-%02d time=%s korMin=%u korMax=%u pomiarow=%u srRedukcjaPct=%.1f luxPokoj=%.0f luxWoda=%.0f tempP1=%.1f tempP2=%.1f tempWoda=%.1f transmisjaPct=%.1f probek=%u mocW=%.1f mocLimitW=%d mocB=%.1f mocFS=%.1f mocFSB=%.1f mocN=%.1f mocC=%.1f\n",
    ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday, logTime().c_str(),
    statystyki.korektyMinimum, statystyki.korektyMaksimum, statystyki.liczbaPomiarow,
    avgReduction, luxPokojowy, luxNadWoda, tempPlate1, tempPlate2, tempWater,
    adaptacja.nauczonaTransmisja * 100.0f, adaptacja.liczbaProbek,
    totW, LED_MAX_POWER_W,
    getPowerForChannel(0, curPwm[0]), getPowerForChannel(1, curPwm[1]),
    getPowerForChannel(2, curPwm[2]), getPowerForChannel(3, curPwm[3]),
    getPowerForChannel(4, curPwm[4]));


  // Reset liczników dziennych
  // [v227] FIX-WDT-DAILYSUM (Partia 3, plan flash-freeze): te same dwie funkcje
  // (saveAdaptStats/saveEnergyStats) są już osłonięte resetem WDT w bloku
  // histSavePending w tgTask (~linia 12848) - tu, wywoływane raz dziennie
  // z loop() na Core 1, były bez ŻADNEGO esp_task_wdt_reset(). Wzorzec
  // przeniesiony 1:1 z tamtego bloku.
  esp_task_wdt_reset();          // [v227] reset przed saveAdaptStats
  saveAdaptStats();              // Zapisz statystyki adaptacji przed resetem
  statystyki.korektyMinimum  = 0;
  statystyki.korektyMaksimum = 0;
  statystyki.liczbaPomiarow  = 0;
  statystyki.sredniaRedukcja = 0;

  // ── [FIX race-audit 2026-08-13] Reset dziennych/tyg./mies. liczników energii ──
  // Wcześniej ten blok czytał/zerował energyTodayWh i całą rodzinę TUTAJ, na Core 1
  // (loop()), podczas gdy saveHistoryPoint() na Core 0 (tgTask) robi na tych samych
  // zmiennych "+=" co ~5 min - niezsynchronizowany wyścig między-rdzeniowy (patrz
  // Ryby_LED_S3_analiza_WDT_race_conditions.md, sekcja 5), realne ryzyko cichej
  // utraty przyrostu na granicy północy.
  // Naprawa: logDailySummary() (Core 1) TYLKO sygnalizuje rollover; całą mutację
  // (zapis do historii + saveEnergyStats() + zerowanie) robi wyłącznie
  // applyEnergyRollover() na Core 0 - ten sam rdzeń co saveHistoryPoint(), więc
  // "+=" i "=0" nigdy się nie przeplatają (single-writer, bez mutexa/snapshotu).
  // Statystyki adaptacji (statystyki.*) wyżej ZOSTAJĄ tutaj bez zmian - ich drugi
  // "pisarz" (obliczAdaptacyjnaJasnosc()) też jest na Core 1, więc przenoszenie
  // ich na Core 0 otworzyłoby nowy wyścig zamiast zamknąć istniejący.
  energyRolloverPending = true;

  // [v159] FIX-LOG-SEND-TRIGGER: usunięto automatyczne wysyłanie log_a+log_b
  // na Telegram o północy. Wysyłka jest teraz wyzwalana przez fsSizeGuard()/
  // fsGuardPending, gdy LittleFS realnie zbliża się do pełna (patrz tgTaskFn,
  // blok fsGuardPending, ~linia 10149) — nie przez porę dnia. Ręczna komenda
  // /sendlogs (TG_LOGS) działa bez zmian.
}
/*************************************************************
 *  EEPROM - runtime -> Core 0 storage queue
 *************************************************************/
static bool eepromBuildPart(EepromWritePart& part, uint16_t addr, const void* data, size_t size) {
  if (size == 0 || size > EEPROM_PART_BYTES) { g_storageLastEnqueueOk = false; return false; }
  part.addr = addr;
  part.size = (uint8_t)size;
  memcpy(part.data, data, size);
  return true;
}

static bool eepromWriteBatchSyncSetup(const EepromWritePart* parts, uint8_t count, const char* tag) {
  if (!parts || count == 0 || count > EEPROM_MAX_PARTS) return false;
  for (uint8_t i = 0; i < count; ++i) {
    if (parts[i].size == 0 || parts[i].size > EEPROM_PART_BYTES) return false;
    for (uint8_t j = 0; j < parts[i].size; ++j) EEPROM.write(parts[i].addr + j, parts[i].data[j]);
  }
  esp_task_wdt_reset();
  bool ok = EEPROM.commit();
  esp_task_wdt_reset();
  if (!ok) logPrintf("lvl=ERR tag=EEPROM-STORAGE state=SETUP_COMMIT_FAIL tag=%s\n", tag ? tag : "?");
  return ok;
}

static bool enqueueEepromBatch(const EepromWritePart* parts, uint8_t count, const char* tag) {
  if (!parts || count == 0 || count > EEPROM_MAX_PARTS) { g_storageLastEnqueueOk = false; return false; }
  // setup() jest jedynym miejscem przed startem tgTask. Runtime nigdy nie ma prawa
  // wpaść do tej gałęzi; zapis bezpośredni jest jednorazową inicjalizacją defaults.
  if (!storageQueue) {
    bool ok = eepromWriteBatchSyncSetup(parts, count, tag);
    if (!ok) g_storageLastEnqueueOk = false;
    return ok;
  }

  StorageRequest req;
  req.type = StorageRequestType::EEPROM_BATCH;
  req.partCount = count;
  if (tag) { strncpy(req.tag, tag, sizeof(req.tag)-1); req.tag[sizeof(req.tag)-1] = '\0'; }
  memcpy(req.parts, parts, sizeof(EepromWritePart) * count);
  if (xQueueSend(storageQueue, &req, 0) != pdPASS) {
    g_storageLastEnqueueOk = false;
    logPrintf("lvl=ERR tag=EEPROM-STORAGE state=QUEUE_FULL tag=%s count=%u\n", req.tag, (unsigned)count);
    return false;
  }
  logPrintfNoFile("lvl=INFO tag=EEPROM-STORAGE state=QUEUED tag=%s count=%u\n", req.tag, (unsigned)count);
  return true;
}

static bool enqueueEepromSingle(uint16_t addr, const void* data, size_t size, const char* tag) {
  EepromWritePart part;
  if (!eepromBuildPart(part, addr, data, size)) return false;
  return enqueueEepromBatch(&part, 1, tag);
}

void saveAutoBrightnessToEEPROM() {
  EepromWritePart p;
  if (!eepromBuildPart(p, EEPROM_ADDR_BACKUP_AUTO_BRIGHTNESS, &backupAutoBrightnessComposite, sizeof(backupAutoBrightnessComposite))) return;
  (void)enqueueEepromBatch(&p, 1, "backup_auto_brightness");
}

void loadAutoBrightnessFromEEPROM() { EEPROM.get(EEPROM_ADDR_BACKUP_AUTO_BRIGHTNESS, backupAutoBrightnessComposite); }

void saveBackupBrightnessToEEPROM() {
  EepromWritePart p;
  if (!eepromBuildPart(p, EEPROM_ADDR_BACKUP_BRIGHTNESS, &backupBrightnessComposite, sizeof(backupBrightnessComposite))) return;
  (void)enqueueEepromBatch(&p, 1, "backup_brightness");
}

void loadBackupBrightnessFromEEPROM() {
  uint64_t saved = 0;
  EEPROM.get(EEPROM_ADDR_BACKUP_BRIGHTNESS, saved);
  if (saved != 0xFFFFFFFFFFFFFFFFULL) backupBrightnessComposite = saved;
}

void loadFadeFromEEPROM() {
  EEPROM.get(EEPROM_ADDR_FADE_MIN, fadeMinutes);
  if (fadeMinutes < 1 || fadeMinutes > 180) {
    fadeMinutes = 30;
  }
}


void saveFadeToEEPROM() {
  (void)enqueueEepromSingle(EEPROM_ADDR_FADE_MIN, &fadeMinutes, sizeof(fadeMinutes), "fade");
}

void saveEmaFilterToEEPROM() {
  (void)enqueueEepromSingle(EEPROM_ADDR_EMA_FILTER, &emaFilterAlpha, sizeof(emaFilterAlpha), "ema");
}

void loadEmaFilterFromEEPROM() {
  float saved = 0.0f;
  EEPROM.get(EEPROM_ADDR_EMA_FILTER, saved);
  if (isnan(saved) || saved < 0.05f || saved > 0.5f) {
    emaFilterAlpha = 0.25f;  // domyślna wartość przy pierwszym uruchomieniu
    saveEmaFilterToEEPROM();
  } else {
    emaFilterAlpha = saved;
  }
  if (Komentarze) logPrintf("lvl=INFO tag=EEPROM msg=\"Filtr czujnika EMA\" alpha=%.2f\n", emaFilterAlpha);
}

// ══════════════════════════════════════════════════════
// EEPROM - SENS_INT (interwał odczytu czujnika)
// ══════════════════════════════════════════════════════
void saveSensIntToEEPROM() {
  (void)enqueueEepromSingle(EEPROM_ADDR_SENS_INT, &sensIntSec, sizeof(sensIntSec), "sens_int");
}
void loadSensIntFromEEPROM() {
  uint16_t saved = 0;
  EEPROM.get(EEPROM_ADDR_SENS_INT, saved);
  if (saved < 5 || saved > 120) {
    sensIntSec = 30;
    saveSensIntToEEPROM();
  } else {
    sensIntSec = saved;
  }
  intervalOdczytu = (unsigned long)sensIntSec * 1000UL;
  logPrintf("lvl=INFO tag=EEPROM msg=\"Interwal odczytu czujnika\" sek=%us\n", sensIntSec);
}

// ══════════════════════════════════════════════════════
// EEPROM - RAMP_SEC (czas rampy PWM)
// ══════════════════════════════════════════════════════
void saveRampSecToEEPROM() {
  (void)enqueueEepromSingle(EEPROM_ADDR_RAMP_SEC, &rampSec, sizeof(rampSec), "ramp_sec");
}
void loadRampSecFromEEPROM() {
  uint16_t saved = 0;
  EEPROM.get(EEPROM_ADDR_RAMP_SEC, saved);
  if (saved < 5 || saved > 120) {
    rampSec = 30;
    saveRampSecToEEPROM();
  } else {
    rampSec = saved;
  }
  logPrintf("lvl=INFO tag=EEPROM msg=\"Czas rampy PWM\" sek=%us\n", rampSec);
}



// ================= EEPROM - HARMONOGRAM =================
// Adresy zdefiniowane globalnie na górze pliku (EEPROM_ADDR_MORNING_WEEKDAY itd.)

void saveScheduleToEEPROM() {
  EepromWritePart p[5]; uint8_t n = 0;
  if (eepromBuildPart(p[n++], EEPROM_ADDR_MORNING_WEEKDAY, &MORNING_ON_START_WEEKDAY, sizeof(MORNING_ON_START_WEEKDAY)) &&
      eepromBuildPart(p[n++], EEPROM_ADDR_MORNING_WEEKEND, &MORNING_ON_START_WEEKEND, sizeof(MORNING_ON_START_WEEKEND)) &&
      eepromBuildPart(p[n++], EEPROM_ADDR_MIDDAY_OFF, &MIDDAY_OFF_LOCAL, sizeof(MIDDAY_OFF_LOCAL)) &&
      eepromBuildPart(p[n++], EEPROM_ADDR_EVENING_BEFORE, &EVENING_ON_BEFORE_SUNSET_MIN, sizeof(EVENING_ON_BEFORE_SUNSET_MIN)) &&
      eepromBuildPart(p[n++], EEPROM_ADDR_EVENING_OFF, &EVENING_OFF_START, sizeof(EVENING_OFF_START))) {
    (void)enqueueEepromBatch(p, n, "schedule");
  }
}

void savePumpScheduleToEEPROM() {
  EepromWritePart p[EEPROM_MAX_PARTS]; uint8_t n = 0;
  uint8_t cnt = (uint8_t)constrain(pumpSlotCount, 0, PUMP_MAX_SLOTS);
  if (!eepromBuildPart(p[n++], EEPROM_ADDR_PUMP_COUNT, &cnt, sizeof(cnt))) return;
  for (int i = 0; i < PUMP_MAX_SLOTS && n + 1 < EEPROM_MAX_PARTS; ++i) {
    int addr = EEPROM_ADDR_PUMP_SLOTS + i * 2 * sizeof(int);
    if (!eepromBuildPart(p[n++], addr, &pumpSlots[i].start, sizeof(pumpSlots[i].start))) return;
    if (!eepromBuildPart(p[n++], addr + sizeof(int), &pumpSlots[i].end, sizeof(pumpSlots[i].end))) return;
  }
  (void)enqueueEepromBatch(p, n, "pump_schedule");
}

void loadPumpScheduleFromEEPROM() {
  uint8_t cnt = 0;
  EEPROM.get(EEPROM_ADDR_PUMP_COUNT, cnt);
  if (cnt == 0 || cnt > PUMP_MAX_SLOTS) {
    // Pierwszy start lub uszkodzone dane - zostają domyślne
    if (Komentarze) logPrintln("lvl=INFO tag=POMPA msg=\"Brak danych EEPROM, domyslny harmonogram\"");
    return;
  }
  pumpSlotCount = (int)cnt;
  for (int i = 0; i < PUMP_MAX_SLOTS; i++) {
    int addr = EEPROM_ADDR_PUMP_SLOTS + i * 2 * sizeof(int);
    EEPROM.get(addr,               pumpSlots[i].start);
    EEPROM.get(addr + sizeof(int), pumpSlots[i].end);
    pumpSlots[i].start = constrain(pumpSlots[i].start, 0, 1439);
    pumpSlots[i].end   = constrain(pumpSlots[i].end,   0, 1439);
  }
  if (Komentarze) {
    logPrintf("lvl=INFO tag=POMPA msg=\"Zaladowano przedzialy\" liczba=%d\n", pumpSlotCount);
    for (int i = 0; i < pumpSlotCount; i++) {
      logPrintf("lvl=INFO tag=POMPA-SLOT slot=%d start=%02d:%02d koniec=%02d:%02d\n", i+1,
        pumpSlots[i].start/60, pumpSlots[i].start%60,
        pumpSlots[i].end/60, pumpSlots[i].end%60);
    }
  }
}

void loadScheduleFromEEPROM() {
  EEPROM.get(EEPROM_ADDR_MORNING_WEEKDAY, MORNING_ON_START_WEEKDAY);
  EEPROM.get(EEPROM_ADDR_MORNING_WEEKEND, MORNING_ON_START_WEEKEND);
  EEPROM.get(EEPROM_ADDR_MIDDAY_OFF,      MIDDAY_OFF_LOCAL);
  EEPROM.get(EEPROM_ADDR_EVENING_BEFORE,  EVENING_ON_BEFORE_SUNSET_MIN);
  EEPROM.get(EEPROM_ADDR_EVENING_OFF,     EVENING_OFF_START);

  // [OK] FIX C: Wykrywanie pierwszego uruchomienia na świeżym ESP32 (EEPROM = 0xFF).
  // Na nowym chipie int32_t z 0xFF = -1, constrain(-1,0,1439)=0 -> wszystkie czasy = 0:00
  // -> isNight=true przez 23h30 -> LED nigdy się nie zapalają bez żadnego komunikatu.
  // Jeśli EVENING_OFF_START wychodzi na 0 (podejrzanie), a MORNING też 0 -> świeży chip.
  // Przywróć bezpieczne wartości domyślne i zaloguj ostrzeżenie.
  bool freshChip = (MORNING_ON_START_WEEKDAY <= 0 && MORNING_ON_START_WEEKEND <= 0
                    && EVENING_OFF_START <= 0 && MIDDAY_OFF_LOCAL <= 0);
  if (freshChip) {
    MORNING_ON_START_WEEKDAY     = 7 * 60;        // 07:00
    MORNING_ON_START_WEEKEND     = 8 * 60;        // 08:00
    MIDDAY_OFF_LOCAL             = 13 * 60;       // 13:00 (przerwa)
    EVENING_ON_BEFORE_SUNSET_MIN = -60;           // 1h przed zachodem
    EVENING_OFF_START            = 22 * 60;       // 22:00
    saveScheduleToEEPROM();
    logPrintln("lvl=WARN tag=EEPROM msg=\"Swiezy chip, zaladowano domyslny harmonogram dnia. Skonfiguruj przez panel WWW.\"");
    return;
  }

  // zabezpieczenie zakresów
  MORNING_ON_START_WEEKDAY     = constrain(MORNING_ON_START_WEEKDAY,     0, 1439);
  MORNING_ON_START_WEEKEND     = constrain(MORNING_ON_START_WEEKEND,     0, 1439);
  MIDDAY_OFF_LOCAL             = constrain(MIDDAY_OFF_LOCAL,             0, 1439);
  EVENING_ON_BEFORE_SUNSET_MIN = constrain(EVENING_ON_BEFORE_SUNSET_MIN, -180, 180);
  EVENING_OFF_START            = constrain(EVENING_OFF_START,            0, 1439);

  // [OK] FIX E: Ostrzeżenie gdy konfiguracja logicznie niespójna (MIDDAY_OFF > eveningOnStart)
  // eveningOnStart jest obliczany dynamicznie, więc tu sprawdzamy tylko oczywiste przypadki
  if (MIDDAY_OFF_LOCAL >= EVENING_OFF_START && MIDDAY_OFF_LOCAL > 0) {
    logPrintf("lvl=WARN tag=HARMONOGRAM msg=\"MIDDAY_OFF_LOCAL >= EVENING_OFF_START, przerwa poludniowa naklada sie na wieczor\" midday=%02d:%02d wieczor=%02d:%02d\n",
              MIDDAY_OFF_LOCAL/60, MIDDAY_OFF_LOCAL%60,
              EVENING_OFF_START/60, EVENING_OFF_START%60);
  }
  if (MORNING_ON_START_WEEKDAY >= EVENING_OFF_START) {
    logPrintf("lvl=WARN tag=HARMONOGRAM msg=\"MORNING_ON_START_WEEKDAY >= EVENING_OFF_START, rano po wieczorze\" rano=%02d:%02d wieczor=%02d:%02d\n",
              MORNING_ON_START_WEEKDAY/60, MORNING_ON_START_WEEKDAY%60,
              EVENING_OFF_START/60, EVENING_OFF_START%60);
  }

if (Komentarze) {
  logPrintf("lvl=INFO tag=HARMONOGRAM msg=\"odczytano z EEPROM\" ranoRobocze=%02d:%02d ranoWeekend=%02d:%02d poludnieWyl=%02d:%02d wieczorPrzed=%02d:%02d wieczorWyl=%02d:%02d\n",
    MORNING_ON_START_WEEKDAY/60, MORNING_ON_START_WEEKDAY%60,
    MORNING_ON_START_WEEKEND/60, MORNING_ON_START_WEEKEND%60,
    MIDDAY_OFF_LOCAL/60, MIDDAY_OFF_LOCAL%60,
    EVENING_ON_BEFORE_SUNSET_MIN/60, EVENING_ON_BEFORE_SUNSET_MIN%60,
    EVENING_OFF_START/60, EVENING_OFF_START%60);
}



}


/*************************************************************
* EEPROM - MIN LUX MODE
*************************************************************/
void saveMinLuxModeToEEPROM() {
  uint8_t enabled = (uint8_t)minLuxModeEnabled;
  EepromWritePart p[3];
  if (eepromBuildPart(p[0], EEPROM_ADDR_MINLUX_ENABLED, &enabled, sizeof(enabled)) &&
      eepromBuildPart(p[1], EEPROM_ADDR_MINLUX_TARGET, &minLuxDayTarget, sizeof(minLuxDayTarget)) &&
      eepromBuildPart(p[2], EEPROM_ADDR_MINLUX_INTERVAL, &minLuxIntervalSec, sizeof(minLuxIntervalSec))) {
    (void)enqueueEepromBatch(p, 3, "minlux");
  }
  if (Komentarze) {
    logPrintf("lvl=INFO tag=EEPROM msg=\"Min LUX zapisano\" stan=%s target=%.0f interval=%ds\n",
              minLuxModeEnabled ? "ON" : "OFF", minLuxDayTarget, (int)minLuxIntervalSec);
  }
}

void saveKwhPrice() {
  (void)enqueueEepromSingle(EEPROM_ADDR_KWH_PRICE, &kwhPrice, sizeof(kwhPrice), "kwh_price");
}
void loadKwhPrice() {
  float v = 0.80f;
  EEPROM.get(EEPROM_ADDR_KWH_PRICE, v);
  if (isnan(v) || isinf(v) || v < 0.01f || v > 99.0f) v = 0.80f;
  kwhPrice = v;
}

void loadMinLuxModeFromEEPROM() {
  uint8_t enabled = 0;
  EEPROM.get(EEPROM_ADDR_MINLUX_ENABLED, enabled);
  EEPROM.get(EEPROM_ADDR_MINLUX_TARGET, minLuxDayTarget);
  minLuxModeEnabled = (enabled == 1);

  // [OK] FIX: NaN/Inf/zakres - auto-naprawa i zapis
  if (isnan(minLuxDayTarget) || isinf(minLuxDayTarget) ||
      minLuxDayTarget < 500 || minLuxDayTarget > 8000) {
    minLuxDayTarget = 2000.0;
    (void)enqueueEepromSingle(EEPROM_ADDR_MINLUX_TARGET, &minLuxDayTarget, sizeof(minLuxDayTarget), "minlux_target_repair");
  }

  // [OK] FEATURE-v28: interwał MIN LUX z EEPROM
  uint16_t savedInterval = 0;
  EEPROM.get(EEPROM_ADDR_MINLUX_INTERVAL, savedInterval);
  if (savedInterval < 10 || savedInterval > 300) savedInterval = 30;
  minLuxIntervalSec = savedInterval;
  minLuxUpdateInterval = (unsigned long)minLuxIntervalSec * 1000UL;

  if (Komentarze) {
    logPrintf("lvl=INFO tag=EEPROM msg=\"Min LUX\" stan=%s target=%.0f interval=%ds\n",
              minLuxModeEnabled ? "ON" : "OFF", minLuxDayTarget, (int)minLuxIntervalSec);
  }
}

/*************************************************************
* FUNKCJA OBLICZANIA MIN LUX PWM
*************************************************************/
uint16_t calculateMinLuxPWM() {
  if (!minLuxModeEnabled) return 0;

  // Noc - nie doświetlaj (czujnik i tak mierzy 0 lux, pętla nieskończona)
  if (isNightGlobal) return 0;

  // Sprawdź czy czujnik działa
  if (!czujnikPokojowyAktywny && !czujnikNadWodaAktywny) {
    // [OK] FIX-v16-SENSOR-FALLBACK: Gdy oba czujniki niedostępne (np. chwilowy zanik I2C),
    // NIE zwracaj 0 - to wyłącza LED na cały czas braku czujnika.
    // Zamiast tego: jeśli MIN LUX był wcześniej aktywny, utrzymaj ostatnie znane PWM.
    // Jeśli nigdy nie był aktywny, użyj fallback = luxToPwm(minLuxDayTarget) jako
    // bezpieczne maksimum (zakładamy brak światła naturalnego = potrzeba 100% MIN LUX).
    if (minLuxModeActive && minLuxCurrentPWM[0] > 0) {
      if (Komentarze) {
        static unsigned long _lastSensorFallbackLog = 0;
        if (millis() - _lastSensorFallbackLog > 1800000UL) {  // [v71] throttle: 2min -> 30min
          logPrintf("lvl=WARN tag=MIN-LUX msg=\"Czujnik niedostepny, utrzymuje ostatnie PWM (fallback)\" pwm=%d\n",
                    minLuxCurrentPWM[0]);
          _lastSensorFallbackLog = millis();
        }
      }
      return minLuxCurrentPWM[0];  // utrzymaj ostatnie znane PWM
    }
    // Jeszcze nie był aktywny i czujnik odpada - użyj bezpiecznego fallback
    uint16_t fallbackPwm = luxToPwm(minLuxDayTarget);
    uint16_t maxAllowed = getBrightnessForSection(backupAutoBrightnessComposite, 0);
    if (maxAllowed == 0) maxAllowed = 300;
    fallbackPwm = constrain(fallbackPwm, 0, (uint16_t)(maxAllowed * 0.8f));
    if (Komentarze) {
      static unsigned long _lastFallbackLog = 0;
      if (millis() - _lastFallbackLog > 1800000UL) {  // [v71] throttle: 2min -> 30min
        logPrintf("lvl=WARN tag=MIN-LUX msg=\"Czujnik niedostepny (brak historii), fallback\" pwm=%d\n",
                  fallbackPwm);
        _lastFallbackLog = millis();
      }
    }
    // [OK] FIX-v51-MINLUX-FALLBACK: Zapisz fallbackPwm do minLuxCurrentPWM[] przed return.
    // BEZ tego: przy następnym wywołaniu (czujnik nadal niedostępny) minLuxCurrentPWM[0]==0
    // -> warunek Fallback 1 (minLuxModeActive && minLuxCurrentPWM[0] > 0) nie zachodzi
    // -> obliczamy fallback od nowa (OK), ale gdy minLuxModeActive=true i czujnik wraca
    // z luxPokojowy=0, Fallback 1 też zwraca 0 -> LED gaszą się.
    // Z tym: Fallback 1 ma historyczną wartość i utrzyma PWM przez cały czas braku czujnika.
    if (fallbackPwm > 0) {
      for (int j = 0; j < 5; j++) minLuxCurrentPWM[j] = fallbackPwm;
    }
    return fallbackPwm;
  }
  
  // Oblicz aktualne światło nad roślinami
  // [OK] FIX-v26: używaj czujnika nad wodą TYLKO gdy user go wybrał (uzywajCzujnikaNadWoda)
  float currentLux = 0;
  if (czujnikNadWodaAktywny && uzywajCzujnikaNadWoda) {
    // TRYB 2 - czujnik nad wodą (bezpośredni pomiar)
    currentLux = luxNadWoda;
  } else if (czujnikPokojowyAktywny) {
    // TRYB 1 - czujnik pokojowy + transmisja
    currentLux = luxPokojowy * szacujTransmisje(luxPokojowy);
  }
  
  // Sprawdź czy naturalne światło wystarczy
  if (currentLux >= minLuxDayTarget) {
    return 0; // Wystarczająco jasno - tylko słońce
  }

  // [WARN]️ USUNIĘTY błędny blok "FIX MINLUX-OVERRIDE" (totalLux + currentLEDLux).
  // Tamten kod liczył LUX z LED, który MIN LUX SAMA właśnie ustawiła, jako "już wystarczy"
  // i zwracał 0 -> wyłączał te same LED -> pętla co 60s: ramp ON -> ramp OFF -> ramp ON...
  // Prawidłowo: decyzja czy MIN LUX jest potrzebna bazuje TYLKO na świetle NATURALNYM.

  // Oblicz ile LUX z LED potrzebujemy (brakuje do celu, bez uwzględniania własnych LED)
  float requiredLEDLux = minLuxDayTarget - currentLux;

  // Bezpieczeństwo - max 80% mocy normalnego harmonogramu
  // UWAGA: używamy backupAutoBrightnessComposite (nastawy AUTO), NIE backupBrightnessComposite
  // (które zawiera chwilowy PWM rampy MIN LUX i zaniżałoby maxAllowedPWM co cykl)
  uint16_t maxAllowedPWM = getBrightnessForSection(backupAutoBrightnessComposite, 0);
  if (maxAllowedPWM == 0) maxAllowedPWM = 300; // Fallback (~30%) gdy harmonogram = 0 (przerwa)
  maxAllowedPWM = (uint16_t)(maxAllowedPWM * 0.8f);

  // Konwersja LUX -> PWM
  uint16_t requiredPWM = luxToPwm(requiredLEDLux);
  requiredPWM = constrain(requiredPWM, 0, maxAllowedPWM);

  // [OK] FIX-v19: Poprzedni warunek "requiredPWM <= minLuxCurrentPWM[0]" powodował
  // że gdy słońce rosło (requiredPWM malał), funkcja zwracała stare wyższe PWM
  // zamiast nowego niższego -> LED nie dimmowały wraz ze wzrostem nasłonecznienia.
  // Jedynym wyjątkiem jest filtr histerezowy w applyMinLuxMode który obsługuje
  // płynność zmian - tutaj zawsze zwracamy obliczone requiredPWM.

  return requiredPWM;
}

/*************************************************************
* ZASTOSOWANIE MIN LUX MODE (wywołanie co 30s)
*************************************************************/
void applyMinLuxMode() {
  if (!minLuxModeEnabled) {
    minLuxModeActive = false;
    return;
  }

  // Noc - wyczyść stan i wyjdź (blokada pętli minLux-w-nocy)
  // [OK] PRIORYTET: blokuj MIN LUX gdy czas > godzina_gaszenia + fadeMinutes
  // isNightGlobal może być opóźniony (aktualizowany co 100ms przez trybAuto)
  // więc sprawdzamy też bezpośrednio po czasie
  {
    int _nowMin = getLocalMinutes();
    int _evOffEnd = EVENING_OFF_START + fadeMinutes;
    bool _afterEvening;
    if (_evOffEnd >= 1440) {
      _evOffEnd -= 1440;
      _afterEvening = (_nowMin >= EVENING_OFF_START) || (_nowMin <= _evOffEnd);
    } else {
      _afterEvening = (_nowMin >= _evOffEnd);
    }
    int _morningStart = isWeekend() ? MORNING_ON_START_WEEKEND : MORNING_ON_START_WEEKDAY;
    bool _beforeMorning = (_nowMin < _morningStart);
    bool _isNocna = _afterEvening && _beforeMorning;

    if (isNightGlobal || _isNocna) {
      if (minLuxModeActive) {
        minLuxModeActive = false;
        rampaAdaptacyjnaAktywna = false;
        rampaMinLuxPriorytet    = false;
        for (int i = 0; i < 5; i++) minLuxCurrentPWM[i] = 0;
      }
      return;
    }
  }

  // [OK] FIX: Nie uruchamiaj minLux podczas soft-startu - zaburza sekwencję startu
  if (softStartActive || manualSoftStartActive) {
    return;
  }

  // [v237] FEATURE-MINLUX-MANUAL-LOCK: user ręcznie ustawił wartość suwakiem
  // podczas przerwy — trzymaj ją, nie przeliczaj co 30s. Blokada znika sama
  // przy końcu przerwy (patrz trybAuto(), detekcja przejścia inLightingWindowGlobal
  // false->true). minLuxModeActive ZOSTAJE true - MIN LUX koncepcyjnie nadal
  // "rządzi" przerwą, tylko chwilowo nie nadpisuje PWM.
  if (minLuxManualOverrideUntilBreakEnd) {
    return;
  }

  // [OK] FIX-1: Nie nadpisuj PWM gdy rampa harmonogramu jest aktywna -> zapobiega błyskowi do 100%
  if (rampScheduleActive) {
    return;
  }

  // [OK] FIX-WINDOW: MIN LUX działa TYLKO w przerwie południowej i nocy.
  // W oknie świecenia (rano/wieczór) LED już świeci pełnym AUTO PWM z harmonogramu -
  // MIN LUX nie powinien go ograniczać do obliczonego minimum.
  if (inLightingWindowGlobal) {
    if (minLuxModeActive) {
      minLuxModeActive = false;
      rampaAdaptacyjnaAktywna = false;
      rampaMinLuxPriorytet    = false;
      for (int i = 0; i < 5; i++) minLuxCurrentPWM[i] = 0;
      logPrintf("lvl=INFO tag=MIN-LUX msg=\"Wejscie w okno swiecenia, wylaczam MIN LUX\"\n");
    }
    return;
  }

  unsigned long now = millis();
  if (now - lastMinLuxUpdate < minLuxUpdateInterval) {
    return;
  }
  lastMinLuxUpdate = now;
  
  uint16_t targetPWM = calculateMinLuxPWM();
  
  static unsigned long lastMinLuxDebug = 0;
  bool shouldDebug = (now - lastMinLuxDebug > 120000);
  
  if (targetPWM == 0) {
    if (minLuxModeActive) {
      // [OK] FIX-RAMP-OFF: Wyłączenie MIN LUX przez rampę w dół, nie skok.
      // Poprzednio: natychmiast zerował backup -> DIAG-3 "SKOK BACKUP ch0: 70->0"
      // Teraz: uruchamia rampę adaptacyjną 70->0 przez ADAPTIVE_RAMP_SECONDS.
      float _t = (adaptacja.nauczonaTransmisja > 0.01f) ? adaptacja.nauczonaTransmisja : GLASS_TRANSMITTANCE;
      float currentLux = (czujnikNadWodaAktywny && uzywajCzujnikaNadWoda) ? luxNadWoda : (luxPokojowy * _t);  // [OK] FIX-v26
      if (Komentarze) {
        logPrintf("lvl=INFO tag=MIN-LUX msg=\"lux wystarczajace, rampa w dol do 0\" lux=%.0f cel=%.0f\n",
                  currentLux, minLuxDayTarget);
      }
      minLuxModeActive = false;
      for (int i = 0; i < 5; i++) minLuxCurrentPWM[i] = 0;
      // Uruchom rampę w dół do 0 (ten sam mechanizm co włączanie)
      // [OK] FIX-v36: sprawdź czy jest co gasić
      {
        bool anyOn = false;
        for (int i = 0; i < 5; i++) {
          if (getBrightnessForSection(backupBrightnessComposite, i) > 0) { anyOn = true; break; }
        }
        if (!anyOn) return;  // już ciemno - nie uruchamiaj zerowej rampy
      }
      _ostatniWywolujacyRampe = "applyMinLuxMode-OFF";
      _ostatniStartRampy = now;
      if (!rampArbiterTryStart(RAMP_AUTO_ADAPTIVE, false)) return;
      rampaAdaptacyjnaAktywna = true;
      rampaMinLuxPriorytet = true;
      gPowerScale = 1.0f; gPowerScaleLastLogged = 1.0f;
      if (Komentarze) {
        logPrintf("lvl=INFO tag=RAMPA akcja=stop wywolal=applyMinLuxMode-OFF od=%d do=0 plan=%ds\n",
                  getBrightnessForSection(backupBrightnessComposite, 0),
                  ADAPTIVE_RAMP_SECONDS);
      }
      for (int i = 0; i < 5; i++) {
        uint16_t aktualnePWM = getBrightnessForSection(backupBrightnessComposite, i);
        rampaAdaptacyjnaAktualne[i] = aktualnePWM;
        rampaAdaptacyjnaCel[i] = 0;
      }
      return;
    }

    // MIN LUX już był nieaktywny - tylko debug log
    if (shouldDebug && Komentarze) {
      float _t = (adaptacja.nauczonaTransmisja > 0.01f) ? adaptacja.nauczonaTransmisja : GLASS_TRANSMITTANCE;
      float currentLux = czujnikNadWodaAktywny ? luxNadWoda : (luxPokojowy * _t);
      logPrintf("lvl=INFO tag=MIN-LUX msg=\"wystarczajaco\" lux=%.0f cel=%.0f\n",
                currentLux, minLuxDayTarget);
      lastMinLuxDebug = now;
    }
    return;
  }
  
  minLuxModeActive = true;

  int diff = abs((int)targetPWM - (int)minLuxCurrentPWM[0]);
  int diffActual = abs((int)targetPWM - (int)getBrightnessForSection(backupBrightnessComposite, 0));
  // [OK] FIX-v20-MINLUX-DIFF: oba muszą być małe żeby pominąć aktualizację
  if (diff < MIN_ZMIANA_PWM && diffActual < MIN_ZMIANA_PWM) {
    return;
  }

  // [OK] FIX-v29-12: Sprawdź czy po zastosowaniu FLOOR (max(target,aktualne)) jest faktycznie co robić.
  // Jeśli aktualne PWM >= target dla wszystkich kanałów -> rampa dystans=0 -> nie startuj.
  {
    bool anyChange = false;
    for (int i = 0; i < 5; i++) {
      uint16_t ak = getBrightnessForSection(backupBrightnessComposite, i);
      uint16_t ct = (targetPWM > ak) ? targetPWM : ak;
      if (i >= 3) ct = constrain(ct, 0, 100);
      if (ct != ak) { anyChange = true; break; }
    }
    if (!anyChange) {
      // Bieżące PWM jest już >= target - nic nie rób, odśwież tylko minLuxCurrentPWM
      for (int i = 0; i < 5; i++) minLuxCurrentPWM[i] = getBrightnessForSection(backupBrightnessComposite, i);
      return;
    }
  }
  // [OK] FIX-v29-12: Log tylko gdy rampa faktycznie ruszy (anyChange=true)
  if (shouldDebug && Komentarze) {
    float currentLux = (czujnikNadWodaAktywny && uzywajCzujnikaNadWoda) ? luxNadWoda :
                       (luxPokojowy * szacujTransmisje(luxPokojowy));
    const char* intensywnosc = (currentLux < 500) ? "MOCNO" : "LAGODNIE";
    logPrintf("lvl=INFO tag=MIN-LUX msg=\"uzupelniam\" tryb=%s lux=%.0f cel=%.0f pwm=%d->%d czujnik=%s\n",
              intensywnosc,
              currentLux, minLuxDayTarget,
              minLuxCurrentPWM[0], targetPWM,
              (czujnikNadWodaAktywny && uzywajCzujnikaNadWoda) ? "nad_woda" : "pokojowy+transmisja");
    lastMinLuxDebug = now;
  }
  // Doświetlenie - BEZPIECZNA rampa (bez /0)
  // Wszystkie 5 kanałów - niebieskie i czerwone ograniczone hard-limitem
  // (czujnik TSL2561 ich nie widzi -> bez limitu mogłoby przepalać w nieskończoność)
  // Hard-limit ch3/ch4: max 50% wartości AUTO (bezpieczny pułap)
  // [OK] FIX-v36-MINLUX-TRYBAUT: sprawdź czy jest realna różnica przed startem rampy
  // applyMinLuxMode() jest wywoływana co ~30s; gdy backup == target dla wszystkich kanałów
  // rampaAdaptacyjnaAktywna=true -> aktualizujRampe widzi wszystkieGotowe=true natychmiast
  // -> rampaAdaptacyjnaAktywna=false w tej samej iteracji -> 3ms -> trybAuto -> TRYBAUT alarm
  {
    bool anyDiff = false;
    for (int i = 0; i < 5; i++) {
      uint16_t aktualne = getBrightnessForSection(backupBrightnessComposite, i);
      uint16_t cel = (targetPWM > aktualne) ? targetPWM : aktualne;
      if (i >= 3) cel = constrain(cel, 0, 100);
      if (cel != aktualne) { anyDiff = true; break; }
    }
    if (!anyDiff) {
      minLuxCurrentPWM[0] = targetPWM;  // zaktualizuj cache bez rampy
      return;  // [OK] backup już na celu - nie uruchamiaj zerowej rampy
    }
  }
  if (!rampArbiterTryStart(RAMP_AUTO_ADAPTIVE, false)) return;
  _ostatniWywolujacyRampe = "applyMinLuxMode";
  _ostatniStartRampy = now;
  rampaAdaptacyjnaAktywna = true;
  rampaMinLuxPriorytet = true;
  gPowerScale = 1.0f; gPowerScaleLastLogged = 1.0f;  // balancer nie walczy z rampą
  if (Komentarze) {
    logPrintf("lvl=INFO tag=RAMPA akcja=start wywolal=applyMinLuxMode od=%d do=%d plan=%ds\n",
              getBrightnessForSection(backupBrightnessComposite, 0),
              targetPWM,
              ADAPTIVE_RAMP_SECONDS);
  }
  for (int i = 0; i < 5; i++) {
    uint16_t aktualnePWM = getBrightnessForSection(backupBrightnessComposite, i);
    rampaAdaptacyjnaAktualne[i] = aktualnePWM;

    // [OK] FIX-v29-10-MINLUX-FLOOR: MIN LUX jest podłogą - nigdy nie obniża PWM poniżej bieżącego.
    // Poprzednio targetPWM < aktualnePWM powodował rampę w DÓŁ podczas "uzupełniania".
    uint16_t channelTarget = (targetPWM > aktualnePWM) ? targetPWM : aktualnePWM;

    // Kanały 3=niebieskie, 4=czerwone: czujnik TSL2561 ich nie widzi
    // -> hard-limit 100 PWM (~10%) żeby nie rosły bez końca w pętli MIN LUX
    if (i >= 3) {
      channelTarget = constrain(channelTarget, 0, 100);
    }

    rampaAdaptacyjnaCel[i] = channelTarget;
    rampaAdaptacyjnaKrok[i] = now;

    int diffPWM = abs((int)channelTarget - (int)aktualnePWM);
    if (diffPWM > 0) {  // OCHRONA PRZED /0!
      rampaAdaptacyjnaInterval[i] = (ADAPTIVE_RAMP_SECONDS * 1000UL) / diffPWM;
    } else {
      rampaAdaptacyjnaInterval[i] = 1000;
      continue;
    }

    minLuxCurrentPWM[i] = channelTarget;
  }
}



// --------------------------------------------------------------------------
// ═══════════════════════════════════════════════════════
// Deklaracje funkcji
// ═══════════════════════════════════════════════════════
void trybManual();
void trybAuto(const char* caller = "?");
void updatePump();
void readTemperatures();

void multiZoneLinearDerating();
void emergencyThermalShutdown();

void wlaczKamera();
void wylaczKamera();
int obliczZachodSlonca(int rok, int miesiac, int dzien, float szerokosc, float dlugosc);
uint16_t gammaCorrect(uint16_t value10bit, float gamma);

// ==========================================================================
// Funkcje pomocnicze: odczyt i zapis bajtów ze skompresowanej wartości jasności
// Używamy typu uint64_t, aby pomieścić 5 bajtów (40 bitów)
uint16_t getBrightnessForSection(uint64_t composite, int section) {
  return (composite >> (section * 10)) & 0x3FF;   // 10-bit (0-1023)
}


void setBrightnessForSection(uint64_t &composite, int section, uint16_t value) {
  composite &= ~((uint64_t)0x3FF << (section * 10));        // czyścimy 10 bitów
  composite |= ((uint64_t)(value & 0x3FF) << (section * 10)); // zapisujemy 10 bitów
}





// Funkcja sprawdzająca, czy obecnie obowiązuje czas letni (CEST) w Polsce
bool isDaylightSaving(time_t now) {
  struct tm *t = gmtime(&now);
  if (!t) return false;

  int month = t->tm_mon + 1;   // 1-12
  int day   = t->tm_mday;
  int wday  = t->tm_wday;      // 0=Nd

  // Marzec - ostatnia niedziela (zmiana o UTC 01:00: zima->lato)
  if (month == 3) {
    if (day - wday < 25) return false;   // przed ostatnią niedzielą
    if (wday != 0) return true;          // po ostatniej niedzieli
    // Ostatnia niedziela: DST zaczyna się od UTC 01:00
    return (t->tm_hour >= 1);            // [OK] FIX BUG-J: przed 01:00 UTC jeszcze zima
  }

  // Kwiecień-wrzesień -> zawsze DST
  if (month > 3 && month < 10) return true;

  // Październik - do ostatniej niedzieli (zmiana o UTC 01:00: lato->zima)
  if (month == 10) {
    if (day - wday < 25) return true;    // przed ostatnią niedzielą
    if (wday != 0) return false;         // po ostatniej niedzieli
    // Ostatnia niedziela: DST kończy się o UTC 01:00
    return (t->tm_hour < 1);             // [OK] FIX BUG-J: przed 01:00 UTC jeszcze lato
  }

  return false;
}

// Zwraca czas lokalny (Polska: UTC+1 zima / UTC+2 lato)
// configTzTime ustawione na UTC0, więc getLocalTime zwraca UTC - NIE używaj go do logów!
// Używaj getLocalTimePL() wszędzie tam gdzie potrzebujesz poprawnej godziny lokalnej.
bool getLocalTimePL(struct tm *ti) {
  time_t nowT = time(nullptr);
  // [OK] FIX #4: Próg 1700000000 = rok 2023. Poprzedni próg 100000 (= 27h od 1970-01-01)
  // był za niski - ESP mógł uznać nieskalibrowany RTC za "zsynchronizowany" i uruchomić
  // pompę/LED o złej godzinie. Teraz wymagamy realnej epoki współczesnej.
  if (nowT < 1700000000L) return false;  // NTP niesynchronizowany
  bool dst = isDaylightSaving(nowT);
  int offsetSec = (dst ? timezoneOffsetMinutes : (timezoneOffsetMinutes - 60)) * 60;
  time_t localT = nowT + offsetSec;
  struct tm *t = gmtime(&localT);
  if (!t) return false;
  *ti = *t;
  return true;
}




// ==========================================================================
// Aktualizacja LEDów - ustawia diody według backupBrightnessComposite oraz steruje zasilaczami
// ==========================================================================


uint16_t mapSliderTo10bit(int sliderValue) {
  return (uint16_t)constrain(sliderValue, 0, 1023);
}

void updateLEDs() {

  // ═══════════════════════════════════════════════════════════
  // ZMIENNE STATYCZNE - TRACKING ZMIAN
  // ═══════════════════════════════════════════════════════════
  static uint64_t lastBackupBrightness = 0;
  static bool lastPowerDebug = false;
  static bool lastLedStatus = false;
  static bool firstRun = true;

  // [OK] TRACKING RAMPY - LOG START/FINISH
  static bool softStartLogged = false;
  static bool transitionLogged = false;
  static bool manualSoftStartLogged = false;
  static bool manualTransitionLogged = false;
  static bool autoRampUpLogged = false;
  static bool autoRampDownLogged = false;
  static unsigned long rampStartTime = 0;
  static unsigned long rampStartTimeSoftStart = 0;  // [OK] FIX-C: osobny timer dla soft-start (nie współdzielony z transition)
  static uint16_t rampStartValue = 0;
  static uint16_t rampTargetValue = 0;

  // Sprawdź czy są zmiany
  bool hasChanged = (backupBrightnessComposite != lastBackupBrightness) ||
                    (power != lastPowerDebug) ||
                    firstRun;

  // ═══════════════════════════════════════════════════════════
  // HARD POWER OFF
  // ═══════════════════════════════════════════════════════════
  if (!power) {
    for (int i = 0; i < 5; i++) {
      ledcWrite(pinyLED[i], 0);
    }

    digitalWrite(ledSupplyPin, HIGH);  // aktywny LOW: HIGH = wyłączony

    if (hasChanged && Komentarze) {
      logPrintln("lvl=INFO tag=POWER-OFF msg=\"wszystkie LED wylaczone natychmiast\"");
    }

    lastBackupBrightness = backupBrightnessComposite;
    lastPowerDebug = power;
    lastLedStatus = false;
    firstRun = false;
    
    // Reset wszystkich flag
    softStartLogged = false;
    transitionLogged = false;
    manualSoftStartLogged = false;
    manualTransitionLogged = false;
    autoRampUpLogged = false;
    autoRampDownLogged = false;
    rampStartTime = 0;

    // [OK] POPRAWKA: Resetuj też rampy funkcjonalne - po włączeniu LED nie wznowią przestarzałej rampy
    softStartActive       = false;
    transitionActive      = false;
    manualSoftStartActive = false;
    manualTransitionActive= false;
    rampaAdaptacyjnaAktywna = false;
    rampaMinLuxPriorytet    = false;
    minLuxModeActive        = false;
    
    return;
  }

  // ═══════════════════════════════════════════════════════════
  // GŁÓWNA PĘTLA PWM
  // ═══════════════════════════════════════════════════════════
  bool anyPWMActive = false;
  uint16_t rawValues[5];
  uint16_t finalValues[5];

  // Zbierz surowe wartości i zastosuj korekcję gamma
  for (int i = 0; i < 5; i++) {
    uint16_t currentBrightness = getBrightnessForSection(backupBrightnessComposite, i);
    rawValues[i]   = currentBrightness;
    finalValues[i] = gammaCorrect(currentBrightness, gammaTable[i]);
  }

  // ══════════════════════════════════════════════════════════
  // POWER BALANCER - limit zasilacza LED_MAX_POWER_W (90 W)
  //
  // KLUCZOWA ZASADA: balancer jest WYŁĄCZONY podczas aktywnych ramp.
  // Powód: rampa (soft-start, adaptacyjna, harmonogram, manual) samodzielnie
  // kontroluje backup PWM i sprowadzi moc do właściwego poziomu. Jeśli
  // balancer działa równocześnie z rampą, oba systemy walczą o hardware PWM
  // -> hardware oscyluje ±1 LSB z częstotliwością ~4Hz = widoczne migotanie.
  //
  // Gdy rampa JEST aktywna: resetuj gPowerScale=1.0 i pomiń skalowanie.
  // Gdy rampa NIE JEST aktywna i moc > limit:
  //   - opada NATYCHMIAST (ochrona zasilacza)
  //   - wraca POWOLI gdy moc spada (brak migotania)
  // ══════════════════════════════════════════════════════════
  {
    // Czy jakakolwiek rampa jest teraz aktywna?
    bool anyRampNow = softStartActive        || transitionActive
                   || manualSoftStartActive  || manualTransitionActive
                   || rampaAdaptacyjnaAktywna || rampScheduleActive;

    if (anyRampNow) {
      // Rampa przejęła kontrolę - balancer śpi, skala = pełna moc
      if (gPowerScale < 1.0f) {
        gPowerScale = 1.0f;
        gPowerScaleLastLogged = 1.0f;
      }
      // NIE modyfikuj finalValues - rampa odpowiada za właściwy poziom PWM
    } else {
      // Tryb stabilny - balancer aktywny
      float totalPower = getTotalLEDPower(finalValues);
      float targetScale = (totalPower > (float)LED_MAX_POWER_W)
                          ? ((float)LED_MAX_POWER_W / totalPower)
                          : 1.0f;

      // Natychmiastowy spadek (ochrona PSU), wolny powrót (brak migotania)
      const float SCALE_RISE_STEP = 0.005f;  // ~400ms powrotu z 0.8->1.0
      if (targetScale < gPowerScale) {
        gPowerScale = targetScale;           // szybki spadek
      } else {
        gPowerScale += SCALE_RISE_STEP;
        if (gPowerScale > 1.0f) gPowerScale = 1.0f;
      }

      // Aplikuj skalowanie tylko gdy rzeczywiście poniżej 1.0
      if (gPowerScale < (1.0f - 0.001f)) {
        for (int i = 0; i < 5; i++) {
          finalValues[i] = (uint16_t)((float)finalValues[i] * gPowerScale);
        }
      }

      // LOG - tylko gdy zmiana ≥1% (nie co 10ms)
      if (Komentarze && fabsf(gPowerScale - gPowerScaleLastLogged) >= 0.01f) {
        float chPwr[5];
        float limitedPower = 0.0f;
        for (int i = 0; i < 5; i++) {
          chPwr[i] = getPowerForChannel(i, finalValues[i]);
          limitedPower += chPwr[i];
        }
        logPrintf("lvl=INFO tag=PWR-LIMIT skala=%.3f zadanaW=%.1f limitW=%.1f maxW=%d mocB=%.1f mocFS=%.1f mocFSB=%.1f mocN=%.1f mocC=%.1f\n",
          gPowerScale, totalPower, limitedPower, LED_MAX_POWER_W,
          chPwr[0], chPwr[1], chPwr[2], chPwr[3], chPwr[4]);
        gPowerScaleLastLogged = gPowerScale;
      }
    }
  }

  // Wyślij finalne wartości PWM do kanałów
  for (int i = 0; i < 5; i++) {
    ledcWrite(pinyLED[i], finalValues[i]);
    if (finalValues[i] > 0) anyPWMActive = true;
  }

  // ═══════════════════════════════════════════════════════════════════
  // 🔍 FLASH DETECTOR - śledzi HARDWARE PWM (finalValues), ZAWSZE aktywny
  // Cel: złapać skoki 0->MAX bez względu na stan ramp/soft-start
  // ═══════════════════════════════════════════════════════════════════
  {
    static uint16_t _hwPrev[5]    = {0,0,0,0,0};
    static bool     _hwPrevInit   = false;
    static bool     _lastPowerHW  = false;
    static bool     _lastSupplyHW = false;
    static unsigned long _flashCount = 0;
    static unsigned long _callCount  = 0;
    _callCount++;

    bool flashDetected = false;
    int  flashDelta[5] = {0,0,0,0,0};

    if (_hwPrevInit) {
      for (int i = 0; i < 5; i++) {
        int d = (int)finalValues[i] - (int)_hwPrev[i];
        flashDelta[i] = d;
        // Próg 200 PWM = wyraźny błysk widzialny gołym okiem
        if (abs(d) >= 200) flashDetected = true;
      }
    }

    bool supplyNow = (power && anyPWMActive);

    if (flashDetected || (_lastPowerHW != power) || (_lastSupplyHW != supplyNow)) {
      _flashCount++;
      // Zbuduj string flag wszystkich ramp
      char flagBuf[80];
      snprintf(flagBuf, sizeof(flagBuf),
        "pwr=%s sup=%s ss=%s tr=%s mss=%s mtr=%s rAdpt=%s rSch=%s scale=%.3f",
        power?"ON":"OFF",
        supplyNow?"ON":"OFF",
        softStartActive?"T":"F",
        transitionActive?"T":"F",
        manualSoftStartActive?"T":"F",
        manualTransitionActive?"T":"F",
        rampaAdaptacyjnaAktywna?"T":"F",
        rampScheduleActive?"T":"F",
        gPowerScale);

      if (flashDetected) {
        logPrintf("lvl=WARN tag=FLASH msg=\"skok hardware PWM wykryty\" nr=%lu call=%lu\n", _flashCount, _callCount);
      } else {
        logPrintf("lvl=INFO tag=STATE-CHG msg=\"zmiana power/supply\" call=%lu\n", _callCount);
      }
      logPrintf("lvl=INFO tag=FLAGS-STATE %s\n", flagBuf);
      logPrintf("lvl=INFO tag=BACKUP-PWM c0=%u c1=%u c2=%u c3=%u c4=%u composite=0x%016llX\n",
        (uint16_t)getBrightnessForSection(backupBrightnessComposite, 0),
        (uint16_t)getBrightnessForSection(backupBrightnessComposite, 1),
        (uint16_t)getBrightnessForSection(backupBrightnessComposite, 2),
        (uint16_t)getBrightnessForSection(backupBrightnessComposite, 3),
        (uint16_t)getBrightnessForSection(backupBrightnessComposite, 4),
        (unsigned long long)backupBrightnessComposite);
      logPrintf("lvl=INFO tag=RAW-PWM msg=\"po gamma\" c0=%u c1=%u c2=%u c3=%u c4=%u\n",
        rawValues[0], rawValues[1], rawValues[2], rawValues[3], rawValues[4]);
      logPrintf("lvl=INFO tag=HW-PWM msg=\"po scale, do ledcWrite\" c0=%u c1=%u c2=%u c3=%u c4=%u\n",
        finalValues[0], finalValues[1], finalValues[2], finalValues[3], finalValues[4]);
      logPrintf("lvl=INFO tag=HW-PWM-PREV c0=%u c1=%u c2=%u c3=%u c4=%u\n",
        _hwPrev[0], _hwPrev[1], _hwPrev[2], _hwPrev[3], _hwPrev[4]);
      if (flashDetected) {
        logPrintf("lvl=WARN tag=FLASH-DELTA c0=%+d c1=%+d c2=%+d c3=%+d c4=%+d\n",
          flashDelta[0], flashDelta[1], flashDelta[2], flashDelta[3], flashDelta[4]);
      }
      logPrintf("lvl=INFO tag=MANUAL-PWM c0=%u c1=%u c2=%u c3=%u c4=%u\n",
        (uint16_t)getBrightnessForSection(backupManualBrightnessComposite, 0),
        (uint16_t)getBrightnessForSection(backupManualBrightnessComposite, 1),
        (uint16_t)getBrightnessForSection(backupManualBrightnessComposite, 2),
        (uint16_t)getBrightnessForSection(backupManualBrightnessComposite, 3),
        (uint16_t)getBrightnessForSection(backupManualBrightnessComposite, 4));
      logPrintf("lvl=INFO tag=AUTO-PWM c0=%u c1=%u c2=%u c3=%u c4=%u\n",
        (uint16_t)getBrightnessForSection(backupAutoBrightnessComposite, 0),
        (uint16_t)getBrightnessForSection(backupAutoBrightnessComposite, 1),
        (uint16_t)getBrightnessForSection(backupAutoBrightnessComposite, 2),
        (uint16_t)getBrightnessForSection(backupAutoBrightnessComposite, 3),
        (uint16_t)getBrightnessForSection(backupAutoBrightnessComposite, 4));
      logPrintf("lvl=INFO tag=FLASH-TEMP led1_c=%.1f led2_c=%.1f woda_c=%.1f tryb=%s ms=%lu\n",
        tempPlate1, tempPlate2, tempWater, tryb?"AUTO":"MAN", millis());
    }

    // Aktualizuj poprzednie wartości
    for (int i = 0; i < 5; i++) _hwPrev[i] = finalValues[i];
    _hwPrevInit   = true;
    _lastPowerHW  = power;
    _lastSupplyHW = supplyNow;
  }

  // ═══════════════════════════════════════════════════════════
  // LOGI - TYLKO PRZY ZMIANACH LUB START/FINISH RAMP
  // ═══════════════════════════════════════════════════════════
if (Komentarze) {
  // ====== SOFT-START (po resecie) ======
  if (softStartActive) {
    uint16_t target = getBrightnessForSection(backupAutoBrightnessComposite, 0);
    if (!softStartLogged) {
      softStartLogged = true;
      rampStartTimeSoftStart = millis();  // [OK] FIX-C: osobny timer
      logPrintf("lvl=INFO tag=SOFT-START faza=START od=0 do=%d plan=%ds temp1=%.0f temp2=%.0f woda=%.0f\n", 
                target, SOFTSTARTSECONDS, tempPlate1, tempPlate2, tempWater);
    }
  } else if (softStartLogged) {
    unsigned long duration = (millis() - rampStartTimeSoftStart) / 1000;  // [OK] FIX-C
    logPrintf("lvl=INFO tag=SOFT-START msg=\"Zakonczony\" od=0 do=%d czas=%lus\n", softStartPwm[0], duration);
    softStartLogged = false;
    rampStartTimeSoftStart = 0;
  }

  // ====== TRANSITION 10s LOG ======
  if (transitionActive) {
    uint16_t target = transitionTargetPwm[0];
    if (!transitionLogged) {
      transitionLogged = true;
      rampStartTime = millis();
      rampStartValue = transitionCurrentPwm[0];
      logPrintf("lvl=INFO tag=PRZEJSCIE tryb=AUTO faza=START od=%d do=%d plan=%ds temp1=%.0f temp2=%.0f woda=%.0f\n", 
                rampStartValue, target, TRANSITIONSECONDS, tempPlate1, tempPlate2, tempWater);
    }
  } else if (transitionLogged) {
    unsigned long duration = (millis() - rampStartTime) / 1000;
    logPrintf("lvl=INFO tag=PRZEJSCIE tryb=AUTO msg=\"Zakonczone\" od=%d do=%d czas=%lus\n", 
              rampStartValue, rawValues[0], duration);
    transitionLogged = false;
    rampStartTime = 0;
  }

  // ====== MANUAL TRANSITION 10s LOG ======
  if (!tryb && manualTransitionActive) {
    uint16_t target = manualTransitionTargetPwm[0];
    if (!manualTransitionLogged) {
      manualTransitionLogged = true;
      rampStartTime = millis();
      rampStartValue = manualTransitionCurrentPwm[0];
      logPrintf("lvl=INFO tag=PRZEJSCIE tryb=MANUAL faza=START od=%d do=%d plan=%ds temp1=%.0f temp2=%.0f woda=%.0f\n", 
                rampStartValue, target, MANUALTRANSITIONSECONDS, tempPlate1, tempPlate2, tempWater);
    }
  } else if (manualTransitionLogged) {
    unsigned long duration = (millis() - rampStartTime) / 1000;
    logPrintf("lvl=INFO tag=PRZEJSCIE tryb=MANUAL msg=\"Zakonczone\" od=%d do=%d czas=%lus\n", 
              rampStartValue, rawValues[0], duration);
    manualTransitionLogged = false;
    rampStartTime = 0;
  }

  // ═══════════════════════════════════════════════════════════
  // NORMALNA PRACA
  // ═══════════════════════════════════════════════════════════
  // ═══════════════════════════════════════════════════════════
  // NORMALNA PRACA - log tylko przy znaczącej zmianie lub co 60s
  // ═══════════════════════════════════════════════════════════
  if (!softStartActive && !transitionActive && !manualSoftStartActive && !manualTransitionActive) {
    static uint16_t lastLoggedPWM = 9999;
    static unsigned long lastLedLog = 0;
    unsigned long nowMs = millis();
    uint16_t currentPWM = rawValues[0];
    bool znaczacaZmiana = abs((int)currentPWM - (int)lastLoggedPWM) >= 10;
    bool mineloCzas = (nowMs - lastLedLog >= 60000);

    if (znaczacaZmiana || (mineloCzas && hasChanged)) {
      logPrintf("lvl=INFO tag=LED-TEMP tryb=STALE pwm=%d led1_c=%.0f led2_c=%.0f woda_c=%.0f\n",
        currentPWM, tempPlate1, tempPlate2, tempWater);

      const char* names[5] = {"biale", "fs", "fs_biale", "niebieski", "czerwone"};
      for (int i = 0; i < 5; i++) {
        float pct = (finalValues[i] / 1023.0f) * 100.0f;
        bool deratingActive = (rawValues[i] != finalValues[i]);
        const char* hot = deratingActive ? " derating=tak" : "";
        logPrintf("lvl=INFO tag=LED-CH kanal=%d nazwa=%s pwm=%d pct=%.0f%% gpio=%d%s\n", i, names[i], finalValues[i], pct, pinyLED[i], hot);
      }
      lastLoggedPWM = currentPWM;
      lastLedLog = nowMs;
    }
  }
}
  // ═══════════════════════════════════════════════════════════
  // STATUS LED I ZASILACZ - TYLKO przy zmianie
  // ═══════════════════════════════════════════════════════════
  bool currentLedStatus = (power && anyPWMActive);
  
  if ((currentLedStatus != lastLedStatus || firstRun) && Komentarze) {
    logPrintf("lvl=INFO tag=LED-STATUS stan=%s zasilacz=%s\n",
              currentLedStatus ? "WL" : "WYL",
              currentLedStatus ? "WLACZONY" : "WYLACZONY");
  }

  digitalWrite(ledSupplyPin, currentLedStatus ? LOW : HIGH);  // aktywny LOW: LOW = włączony

  // ═══════════════════════════════════════════════════════════
  // AKTUALIZACJA ZMIENNYCH TRACKING
  // ═══════════════════════════════════════════════════════════
  lastBackupBrightness = backupBrightnessComposite;
  lastPowerDebug = power;
  lastLedStatus = currentLedStatus;
  firstRun = false;
}




// ==========================================================================
// Funkcja updatePump() - steruje włączaniem/wyłączaniem pompki

void updatePump() {
  // [OK] FIX BUG-H: Honouruj manual override przez 30 minut
  if (millis() < pumpManualOverrideUntil) return;

  // LOKALNY CZAS (UTC + offset + DST)
  int currentMinutes = getLocalMinutes();  // 0-1439

  // [OK] POPRAWKA: Tylko przy zmianie stanu - nie pisz digitalWrite tysiące razy na sekundę
  static bool lastPumpState = false;
  // [OK] FIX-7: Debounce 5s - chroni SSR przed szybkimi przełączeniami (skraca żywotność przekaźnika)
  static unsigned long lastPumpChangeMs = 0;

  // Jeśli czas nie jest zsynchronizowany z NTP - pompa wyłączona (bezpieczny fallback)
  // [OK] POPRAWKA: Guard na brak NTP - getLocalMinutes() zwraca 0 gdy brak czasu
  time_t now = time(nullptr);
  // [OK] FIX #4: Ujednolicony próg NTP - 1700000000 (rok 2023), spójny z getLocalTimePL().
  // Poprzedni próg 1000000 (= 11 dni od 1970) był zbyt niski - pompa mogła się włączyć
  // przy złym czasie RTC zaraz po restarcie bez WiFi.
  if (now < 1700000000L) {
    // Czas nie zsynchronizowany (epoch < ~1970+11dni) - nie ryzykuj
    if (lastPumpState) {
      digitalWrite(pumpPin, HIGH);  // aktywny LOW: HIGH = wyłączona
      lastPumpState = false;
      if (Komentarze) logPrintln("lvl=WARN tag=POMPA msg=\"Brak NTP, pompa wylaczona\"");
    }
    return;
  }

  bool shouldBeOn = false;
  for (int i = 0; i < pumpSlotCount; i++) {
    int start = pumpSlots[i].start;
    int end   = pumpSlots[i].end;
    // [OK] FIX LOGIC-B: Obsługa harmonogramów przez północ (np. 23:00-01:00)
    if (start < end) {
      if (currentMinutes >= start && currentMinutes < end) { shouldBeOn = true; break; }
    } else if (start > end) {
      if (currentMinutes >= start || currentMinutes < end) { shouldBeOn = true; break; }
    }
    // start == end -> ignoruj (błędny slot)
  }

  if (shouldBeOn != lastPumpState) {
    // [OK] FIX-7: Debounce - min. 5s między zmianami stanu SSR
    if (millis() - lastPumpChangeMs < 5000UL) {
      return;  // za wcześnie - poczekaj
    }
    digitalWrite(pumpPin, shouldBeOn ? LOW : HIGH);  // aktywny LOW: LOW = włączona
    lastPumpState = shouldBeOn;
    lastPumpChangeMs = millis();
    currentPumpState = shouldBeOn;
    if (Komentarze) logPrintf("lvl=INFO tag=POMPA stan=%s czas=%02d:%02d\n",
                              shouldBeOn ? "WL" : "WYL",
                              currentMinutes / 60, currentMinutes % 60);
  }
}


// ==========================================================================
// Tryb automatyczny - harmonogram symulujący wschód i zachód słońca
// ==========================================================================

// Sprawdzenie, czy dzisiaj to weekend (0 = niedziela, 6 = sobota)
bool isWeekend() {
  time_t now = time(nullptr);
  struct tm *utc = gmtime(&now);
  if (!utc) return false;

  bool dst = isDaylightSaving(now);
  int offset = dst ? timezoneOffsetMinutes : (timezoneOffsetMinutes - 60);

  int localMinutes = utc->tm_hour * 60 + utc->tm_min + offset;

  // [OK] FIX #7: Korekcja dnia tygodnia na podstawie localMinutes, NIE offsetu.
  // Poprzedni warunek (offset >= 1440) nigdy nie był prawdziwy dla Polski (offset=60 lub 120).
  int wday = utc->tm_wday;
  if (localMinutes >= 1440) {
    wday = (wday + 1) % 7;   // lokalny czas przekroczył północ -> następny dzień
    localMinutes -= 1440;
  } else if (localMinutes < 0) {
    wday = (wday + 6) % 7;   // lokalny czas przed UTC północą -> poprzedni dzień
    localMinutes += 1440;
  }

  return (wday == 0 || wday == 6);
}



// Oblicza wartość autoBrightness  wg harmonogramu:
// - Poranek: fade in przez 20 minut, start: 7:00 (dni robocze) lub 8:00 (weekendy)
// - Południe: fade out od 12:00 do 12:10
// - Wieczór: fade in 10 minut, start: 2 godziny przed zachodem (sunsetMinutes)
//           fade out od 21:30 do 21:45


int getLocalMinutes() {
  time_t now = time(nullptr);

  struct tm *utc = gmtime(&now);   // ZAWSZE UTC
  if (!utc) return 0;

  bool dst = isDaylightSaving(now);   // [OK] PRZEKAZUJ time_t
  int offset = dst ? timezoneOffsetMinutes : (timezoneOffsetMinutes - 60);

  int minutes = utc->tm_hour * 60 + utc->tm_min + offset;

  if (minutes < 0) minutes += 1440;
  if (minutes >= 1440) minutes -= 1440;

  return minutes;
}


// ==========================================================================
// CALLBACKI dla Arduino IoT Cloud
// ==========================================================================

// Callback dla suwaka jasności - ręczna zmiana tylko dla wybranych sekcji
// Jeśli w "sections" wybierzesz 31, to oznacza "wszystkimi"
// Jeśli wybierzesz 32, to oznacza "zapisz ustawienia ze wszystkich diod" (kopiujemy manualne ustawienia do backupu auto)


// Callback przy zmianie wyboru diody (sekcji).
// Jeśli "wszystkimi" (31) - przyjmujemy wartość z diody 1; w przeciwnym razie ustawiamy suwak na wartość zapamiętaną dla wybranej diody.
void onSectionsChange() {
  // [v33] Arduino IoT Cloud usunięty - guard boot phase usunięty

  // [OK] KLUCZOWE - ignoruj fałszywe zmiany
  static int lastRealSections = -1;
  
  if (sections == lastRealSections) {
    // To tylko synchronizacja, nie rzeczywista zmiana - ignoruj
    return;
  }
  
  // Zapisz prawdziwą zmianę
  lastRealSections = sections;
  
  int prevSections = lastSections;

  if (Komentarze) {
    logPrintf("lvl=INFO tag=SECTIONS sections=%d\n", sections);
  }

  // [v33] Bloki sections==64/128/256 (temperatura Cloud) i sections==250 (kamera Cloud) usunięte -
  // były używane wyłącznie przez Arduino IoT Cloud do synchronizacji zmiennej CloudVariable.


// =====================================================
// === ZAPIS DO AUTO (sections == 32)
// =====================================================
if (sections == 32) {
  if (Komentarze) {
    logPrintln("lvl=INFO tag=WYBOR msg=\"Zapis do AUTO\" sections=32");
  }
  
  // [OK] ZAPISZ harmonogram BEZ resetu soft-start
  saveFadeToEEPROM();
  saveScheduleToEEPROM();

  // [v33] Guard cloudInitialSyncDone usunięty - cloud usunięty
  uint16_t val = previewBrightness10bit;
  
  if (!brightnessPreviewActive) {
    if (Komentarze) {
      logPrintln("lvl=INFO tag=AUTO msg=\"Zmiana czasu, harmonogram zapisany (bez LED)\"");
    }
    return;  // [OK] EXIT BEZ ŻADNEJ RAMPY!
  }
  
  // [OK] 10s RAMPA TYLKO dla LED preview
  if (tryb && power) {
    if (!rampArbiterTryStart(RAMP_AUTO_TRANSITION, true)) return;
    transitionActive = true;
    unsigned long nowMs = millis();
    unsigned long totalTransitionMs = TRANSITIONSECONDS * 1000UL;

    
    for (int i = 0; i < 5; i++) {
      if (lastSections == 31 || (lastSections & (1 << i))) {
        uint16_t currentVal = getBrightnessForSection(backupBrightnessComposite, i);
        transitionCurrentPwm[i] = currentVal;
        transitionTargetPwm[i] = val;
        transitionStepMillis[i] = nowMs;
        
        int diff = abs((int)val - (int)currentVal);
        if (diff > 0) {
          transitionIntervalMs[i] = totalTransitionMs / diff;
        } else {
          transitionIntervalMs[i] = 1000;
        }
        
        setBrightnessForSection(backupAutoBrightnessComposite, i, val);
      }
    }
    
    if (Komentarze) {
      logPrintf("lvl=INFO tag=PRZEJSCIE faza=START od=%d do=%d plan=10s\n", 
                    transitionCurrentPwm[0], transitionTargetPwm[0]);
    }
  } else {
    for (int i = 0; i < 5; i++) {
      if (lastSections == 31 || (lastSections & (1 << i))) {
        setBrightnessForSection(backupAutoBrightnessComposite, i, val);
      }
    }
  }
  
  saveAutoBrightnessToEEPROM();
  brightnessPreviewActive = false;
  
  if (Komentarze) {
    logPrintf("lvl=INFO tag=AUTO msg=\"Zapisano LED do EEPROM\" pwm=%d\n", val);
  }
  return;  // [OK] DODAJ return!
}




  // =====================================================
  // === LEDY - PODGLĄD (BEZ ZMIAN PWM)
  // =====================================================
  brightnessPreviewActive = false;

  if (sections == 31) {
    previewBrightness10bit = getBrightnessForSection(backupBrightnessComposite, 0);
    brightnessComposite = previewBrightness10bit;
    if (Komentarze) {
      logPrintf("lvl=INFO tag=WYBOR kanal=WSZYSTKIE jasnosc=%d jasnosc_proc=%.0f\n", 
                    previewBrightness10bit,
                    previewBrightness10bit / 1023.0f * 100.0f);
    }
    return;
  }

  // Nazwy kanałów LED
  const char* ledNames[5] = {"BIAŁE", "FS", "FS BIAŁE", "NIEBIESKIE", "CZERWONE"};

  for (int i = 0; i < 5; i++) {
    if (sections & (1 << i)) {
      previewBrightness10bit = getBrightnessForSection(backupBrightnessComposite, i);
      brightnessComposite = previewBrightness10bit;
      
      if (Komentarze) {
        logPrintf("lvl=INFO tag=WYBOR kanal=%s jasnosc=%d jasnosc_proc=%.0f\n", 
                      ledNames[i],
                      previewBrightness10bit,
                      previewBrightness10bit / 1023.0f * 100.0f);
      }
      return;
    }
  }
}


void onBrightnessCompositeChange() {
  // [v33] Arduino IoT Cloud usunięty

  // Harmonogram i rampa: obsługiwane wyłącznie przez panel WWW

    // =====================================================
  // ===== JASNOŚĆ LED - LIVE PREVIEW ====================
  // =====================================================

  // ⛔ jeżeli zasilanie OFF -> absolutnie NIC nie rób
  if (!power) {
    return;
  }

  uint16_t liveValue = mapSliderTo10bit(brightnessComposite);

  // 🔒 JEŚLI TRYB AUTO I POZA OKNEM ŚWIECENIA
  // -> NIE DOTYKAJ backupBrightnessComposite (brak migania)
  if (tryb) {
    int nowMin = getLocalMinutes();

    int morningStart =
      isWeekend() ? MORNING_ON_START_WEEKEND : MORNING_ON_START_WEEKDAY;

    // [OK] FIX-v16-WRAP: Użyj tej samej logiki co trybAuto (eveningOffWraps).
    // Poprzedni kod: nowMin >= ((EVENING_OFF_START + fadeMinutes) % 1440)
    // Gdy EVENING_OFF_START+fadeMinutes >= 1440 (np. 23:00 + 60min = 00:00),
    // modulo daje 0 -> warunek ZAWSZE prawdziwy -> live preview nigdy nie działał
    // przy rampie wieczornej przekraczającej północ.
    int _evOffEnd = EVENING_OFF_START + fadeMinutes;
    bool _evWraps = (_evOffEnd >= 1440);
    if (_evWraps) _evOffEnd -= 1440;

    bool outsideLightingWindow;
    if (_evWraps) {
      // Rampa kończy się rano (np. 00:30) -> "poza oknem" = od końca rampy do rana
      outsideLightingWindow = (nowMin >= _evOffEnd) && (nowMin < morningStart);
    } else {
      outsideLightingWindow = (nowMin < morningStart) || (nowMin >= _evOffEnd);
    }

    if (outsideLightingWindow) {
      // tylko zapamiętaj do zapisu AUTO
      previewBrightness10bit = liveValue;
      brightnessPreviewActive = true;
      return;
    }
  }

  // =====================================================
  // ===== LIVE PREVIEW Z RAMPĄ ==========================
  // =====================================================
  previewBrightness10bit = liveValue;
  brightnessPreviewActive = true;

  if (tryb && power) {
    // 🎯 TRYB AUTO + zasilanie ON -> RAMPA 10s

    // [OK] FIX-TRANSITION-INIT: Najpierw zainicjalizuj WSZYSTKIE kanały aktualną
    // wartością z backup (current==target), żeby kanały poza 'sections' nie
    // miały śmieciowych wartości. Bez tego transition mogła zaraz się "skończyć"
    // z target=0 i wyzerować backup tych kanałów (flash po soft-starcie).
    unsigned long nowMs = millis();
    unsigned long totalTransitionMs = TRANSITIONSECONDS * 1000UL;
    for (int i = 0; i < 5; i++) {
      uint16_t currentVal = getBrightnessForSection(backupBrightnessComposite, i);
      transitionCurrentPwm[i] = currentVal;
      transitionTargetPwm[i]  = currentVal;  // domyślnie: bez zmiany
      transitionStepMillis[i] = nowMs;
      transitionIntervalMs[i] = 1000;
    }

    // Ustaw cel tylko dla kanałów należących do 'sections'
    bool anyChange = false;
    for (int i = 0; i < 5; i++) {
      if (sections == 31 || (sections & (1 << i))) {
        uint16_t currentVal = getBrightnessForSection(backupBrightnessComposite, i);
        transitionCurrentPwm[i] = currentVal;
        transitionTargetPwm[i]  = liveValue;
        transitionStepMillis[i] = nowMs;

        int diff = abs(int(liveValue) - int(currentVal));
        if (diff > 0) {
          transitionIntervalMs[i] = totalTransitionMs / diff;
          anyChange = true;
        } else {
          transitionIntervalMs[i] = 1000;
        }
      }
    }

    // Jeśli żaden kanał nie zmienia wartości - nie uruchamiaj transition
    if (!anyChange) {
      if (Komentarze) logPrintf("lvl=INFO tag=PRZEJSCIE tryb=PODGLAD msg=\"Pominieto, brak zmiany PWM\" livevalue=%d\n", liveValue);
      return;
    }

    if (!rampArbiterTryStart(RAMP_AUTO_TRANSITION, true)) return;
    transitionActive = true;

    if (Komentarze) {
      logPrintf("lvl=INFO tag=PRZEJSCIE tryb=PODGLAD faza=START od=%d do=%d plan=10s\n", 
                    transitionCurrentPwm[0], transitionTargetPwm[0]);
    }
    
  } else if (!tryb && power) {
    // 🎯 TRYB MANUAL + zasilanie ON -> RAMPA 10s (MANUALNA)
    if (!rampArbiterTryStart(RAMP_MANUAL_TRANSITION, true)) return;
    manualTransitionActive = true;
    unsigned long nowMs = millis();
    unsigned long totalTransitionMs = (unsigned long)MANUALTRANSITIONSECONDS * 1000UL;

    for (int i = 0; i < 5; i++) {
      if (sections == 31 || (sections & (1 << i))) {
        uint16_t currentVal = getBrightnessForSection(backupBrightnessComposite, i);
        
        manualTransitionCurrentPwm[i] = currentVal;
        manualTransitionTargetPwm[i] = liveValue;
        manualTransitionStepMillis[i] = nowMs;

        // Oblicz interwał dla +1 PWM na krok
        int diff = abs(int(liveValue) - int(currentVal));
        if (diff > 0) {
          manualTransitionIntervalMs[i] = totalTransitionMs / diff;
        } else {
          manualTransitionIntervalMs[i] = 1000; // Backup
        }
      }
    }
    
    if (Komentarze) {
      logPrintf("lvl=INFO tag=PRZEJSCIE tryb=PODGLAD-MANUAL faza=START od=%d do=%d plan=10s\n", 
                    manualTransitionCurrentPwm[0], manualTransitionTargetPwm[0]);
    }
    
  } else {
    // ⚡ power OFF -> natychmiastowa zmiana (BEZ RAMPY)
    if (sections == 31) {   // WSZYSTKIE
      for (int i = 0; i < 5; i++) {
        setBrightnessForSection(backupBrightnessComposite, i, liveValue); 
      }
    } else {
      for (int i = 0; i < 5; i++) {
        if (sections & (1 << i)) {
          setBrightnessForSection(backupBrightnessComposite, i, liveValue); 
        }
      }
    }
    bumpChangeCounter();  // oznacz realną zmianę jasności
    updateLEDs();
  }
}



void onTrybChange() {
  // [v257] callback runtime korzysta wyłącznie z RAM; storage owner działa na Core 0.
  logPrintf("lvl=INFO tag=WYWOLANIE zdarzenie=tryb tryb=%s czas=%lus\n",
    tryb ? "AUTO" : "MANUAL", millis() / 1000);

  // [v33] Guard cloudInitialSyncDone usunięty - cloud usunięty

  logPrintf("lvl=INFO tag=ZAPIS cel=EEPROM tryb=%s\n", tryb ? "AUTO" : "MANUAL");
  savePowerTrybToEEPROM();
  bumpChangeCounter();  // ← oznacz że to realna zmiana (nie echo Cloud po restarcie)

  // *** PRZECHWYCENIE aktualnego stanu PWM ***
  if (tryb) {  // MANUAL -> AUTO: zapisz aktualne jako MANUAL
    backupManualBrightnessComposite = backupBrightnessComposite;
    if (Komentarze) logPrintf("lvl=INFO tag=MANUAL-BACKUP val=%d\n", getBrightnessForSection(backupManualBrightnessComposite, 0));
  } else {     // AUTO -> MANUAL: opcjonalnie zapisz aktualne jako AUTO
    // backupAutoBrightnessComposite = backupBrightnessComposite;
  }

  // ZATRZYMAJ wszystkie inne rampy
  manualSoftStartActive = false;
  manualTransitionActive = false;
  softStartActive = false;
  transitionActive = false;
  brightnessPreviewActive = false;

  if (tryb) {  // TRYB AUTO
    // ... (cała logika okna świecenia bez zmian)
    int nowMin = getLocalMinutes();
    time_t now;
    time(&now);
    struct tm *utc = gmtime(&now);
    if (!utc) return;
    bool dst = isDaylightSaving(now);
    int offset = dst ? timezoneOffsetMinutes : timezoneOffsetMinutes - 60;
    int localMinutes = utc->tm_hour * 60 + utc->tm_min + offset;
    int wday = utc->tm_wday;
    // [OK] FIX #7: Korekcja dnia tygodnia na podstawie localMinutes
    if (localMinutes >= 1440) {
      wday = (wday + 1) % 7;
      localMinutes -= 1440;
    } else if (localMinutes < 0) {
      wday = (wday + 6) % 7;
      localMinutes += 1440;
    }
    bool isWeekendLocal = (wday == 0 || wday == 6);
    
    int morningStart = isWeekendLocal ? MORNING_ON_START_WEEKEND : MORNING_ON_START_WEEKDAY;
    int eveningOnStart = sunsetMinutes + EVENING_ON_BEFORE_SUNSET_MIN;
    if (eveningOnStart < 0) eveningOnStart += 1440;
    if (eveningOnStart > 1440) eveningOnStart -= 1440;
    
    bool inMorningWindow = (nowMin >= morningStart) && (nowMin < MIDDAY_OFF_LOCAL);  // [OK] FIX-v13: rano do startu rampy
    // [OK] BUG#2 FIX: obsługa overflow przez północ
    int _evOffEndTryb = EVENING_OFF_START + fadeMinutes;
    bool inEveningWindow;
    if (_evOffEndTryb >= 1440) {
      _evOffEndTryb -= 1440;
      inEveningWindow = (nowMin >= eveningOnStart) || (nowMin <= _evOffEndTryb);
    } else {
      inEveningWindow = (nowMin >= eveningOnStart) && (nowMin <= _evOffEndTryb);
    }
    bool shouldBeOn = inMorningWindow || inEveningWindow;
    // [OK] FIX MINLUX-WINDOW: aktualizuj globalną flagę okna świecenia
    bool _bylWOknie = inLightingWindowGlobal;
    inLightingWindowGlobal = shouldBeOn;
    // [v237] FEATURE-MINLUX-MANUAL-LOCK: koniec przerwy (wejście w okno świecenia)
    // zwalnia blokadę ręcznego suwaka — normalne świecenie i tak przejmuje PWM,
    // a przy kolejnej przerwie MIN LUX ma zacząć liczyć od nowa, bez starej blokady.
    if (!_bylWOknie && inLightingWindowGlobal) {
      minLuxManualOverrideUntilBreakEnd = false;
      if (Komentarze) logPrintln("lvl=INFO tag=MIN-LUX msg=\"Koniec przerwy - blokada reczna zwolniona\"");
    }
    isNightGlobal = false;

    if (power) {  // *** ZAWSZE rampa 10s do AUTO ***
      // [OK] PRIORYTET: MANUAL->AUTO = soft-start 30s (nie transition 3s)
      if (!rampArbiterTryStart(RAMP_AUTO_SOFTSTART, true)) return;
      softStartActive = true;
      gPowerScale = 1.0f; gPowerScaleLastLogged = 1.0f;
      unsigned long nowMs = millis();
      unsigned long totalSoftStartMs = SOFTSTARTSECONDS * 1000UL;

      for (int i = 0; i < 5; i++) {
        uint16_t currentVal = getBrightnessForSection(backupBrightnessComposite, i);
        uint16_t targetVal = shouldBeOn ? getBrightnessForSection(backupAutoBrightnessComposite, i) : 0;
        softStartPwm[i]        = currentVal;   // startuj od aktualnej wartości, nie od 0
        softStartTargetPwm[i]  = targetVal;
        softStartStepMillis[i] = nowMs;
      }

      // [FIX-FLASH3] Kap target do limitu mocy zasilacza
      if (shouldBeOn) {
        float prevPwr = getTotalLEDPower(softStartTargetPwm);
        float appliedScale = capTargetsToPowerLimit(softStartTargetPwm);
        if (Komentarze && appliedScale < 0.999f)
          logPrintf("lvl=WARN tag=SSTART-INIT kontekst=MAN-AUTO moc=%.1f limit=%d skala=%.3f\n",
            prevPwr, LED_MAX_POWER_W, appliedScale);
      }

      // Przelicz interwały kroku po ewentualnym kapowaniu
      for (int i = 0; i < 5; i++) {
        int diff = abs((int)softStartTargetPwm[i] - (int)softStartPwm[i]);
        softStartIntervalMs[i] = (diff > 0) ? (totalSoftStartMs / diff) : 1000;
      }
      if (Komentarze) logPrintf("lvl=INFO tag=PRZEJSCIE tryb=MANUAL-AUTO faza=START od=%d do=%d plan=30s\n",
        getBrightnessForSection(backupBrightnessComposite, 0),
        softStartTargetPwm[0]);
    }

    // Sync suwaka...
    if (sections == 31) brightnessComposite = getBrightnessForSection(backupBrightnessComposite, 0);
    else for (int i = 0; i < 5; i++) if (sections == (1 << i)) { brightnessComposite = getBrightnessForSection(backupBrightnessComposite, i); break; }
    if (Komentarze) logPrintf("lvl=INFO tag=PRZEJSCIE tryb=AUTO msg=\"Rampa aktywna\"\n");

  } else {  // TRYB MANUAL - *** ZAWSZE rampa 10s ***
    if (power) {
      if (!rampArbiterTryStart(RAMP_MANUAL_TRANSITION, true)) return;
      manualTransitionActive = true;
      unsigned long nowMs = millis();
      unsigned long totalTransitionMs = MANUALTRANSITIONSECONDS * 1000UL;
      
      for (int i = 0; i < 5; i++) {
        uint16_t currentVal = getBrightnessForSection(backupBrightnessComposite, i);
        uint16_t targetVal = getBrightnessForSection(backupManualBrightnessComposite, i);
        manualTransitionCurrentPwm[i] = currentVal;
        manualTransitionTargetPwm[i] = targetVal;
        manualTransitionStepMillis[i] = nowMs;
        int diff = abs((int)targetVal - (int)currentVal);
        if (diff > 0) {
          manualTransitionIntervalMs[i] = totalTransitionMs / diff;
        } else {
          manualTransitionIntervalMs[i] = 1000;
        }
      }
      if (Komentarze) logPrintf("lvl=INFO tag=PRZEJSCIE tryb=AUTO-MANUAL faza=START od=%d do=%d plan=10s\n", 
        manualTransitionCurrentPwm[0], manualTransitionTargetPwm[0]);
    }

    if (Komentarze) logPrintf("lvl=INFO tag=PRZEJSCIE tryb=MANUAL msg=\"Rampa aktywna\"\n");
  }
}






/*************************************************************
 * TRANSMISJA - szacunek dynamiczny
*************************************************************/
float szacujTransmisje(float luxPokojowy) {
  // [OK] FIX-TRANSM: Czujnik TSL siedzi PRZED szybą akwarium (8mm szkło = ~85% transmisji).
  // Poprzednia tabela (2-45%) była dla czujnika w odległym pokoju - całkowicie błędna
  // przy czujniku zamontowanym bezpośrednio przy szybie.
  // Fizyka: szkło 8mm traci ~15% -> przepuszcza 85%.
  // Wartość konfigurowalna przez stałą GLASS_TRANSMITTANCE.
  (void)luxPokojowy; // nieużywany - transmisja nie zależy od poziomu lux przy tej geometrii
  return GLASS_TRANSMITTANCE;
}



/*************************************************************
 * KONWERSJA PWM ↔ LUX
*************************************************************/
// Kalibracja: PWM 300 = 3907 lux (z Twoich pomiarów)
const float LUX_PER_PWM = 3907.0 / 300.0; // ~13.02 lux/PWM

float pwmToLux(uint16_t pwm) {
  return (float)pwm * LUX_PER_PWM;
}

uint16_t luxToPwm(float lux) {
  if (lux <= 0) return 0;
  uint16_t pwm = (uint16_t)(lux / LUX_PER_PWM);
  return constrain(pwm, 0, 1023);
}




/*************************************************************
 * ODCZYT CZUJNIKÓW Z FILTREM
*************************************************************/
void odczytajSwiatloZFiltrem() {
  unsigned long teraz = millis();
  
  // KLUCZOWE: Sprawdź czy minął czas
  if (teraz - ostatniOdczytSwiatla < intervalOdczytu) {
    return; // Za wcześnie
  }
  
  ostatniOdczytSwiatla = teraz; // RESET timera

  // [v71] SENSOR-OFF: gdy użytkownik wybrał OFF w WWW - pomiń całe I2C.
  // Czujniki nie są odpytywane, nie generują błędów, nie wyzwalają reinit.
  // Przełączenie OFF→Pokój/Pokój+Woda powoduje automatyczny reinit bez restartu.
  if (!uzywajCzujnikaPokojowego && !uzywajCzujnikaNadWoda) {
    czujnikPokojowyAktywny = false;
    czujnikNadWodaAktywny  = false;
    tslReadPending         = false;  // nie zlecaj odczytu Core 0
    return;
  }

  // ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  // [SIM] Gdy symulacja aktywna - nadpisz wartości lux i wróć.
  // Cały kod hardware poniżej jest pomijany. Czujnik pokojowy
  // jest traktowany jako „aktywny" żeby MIN LUX i adaptacja
  // reagowały normalnie. Czujnik nad wodą wyłączony (brak fizycznie).
  // ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  if (simLuxEnabled) {
    // [SIM] Oblicz aktualną wartość docelową (stała lub sinusoida)
    float simTarget;
    if (simLuxAuto) {
      // [SIM] Sinusoida: pełny cykl co simLuxAutoPeriodMs
      float phase = (float)(teraz % simLuxAutoPeriodMs) / (float)simLuxAutoPeriodMs;
      // sin() zwraca -1..1, mapujemy na 0..1 (wschodząco-zachodząco)
      float sinus = (sinf(phase * 2.0f * PI) + 1.0f) * 0.5f;
      simTarget = simLuxAutoMin + sinus * (simLuxAutoMax - simLuxAutoMin);
    } else {
      // [SIM] Stała wartość ustawiona przez terminal
      simTarget = simLuxValue;
    }

    // [SIM] Filtr EMA - wygładzamy symulowany sygnał tak samo jak hardware,
    // żeby MIN LUX i adaptacja nie reagowały na skokowe zmiany z terminala.
    const float SIM_FILTR = emaFilterAlpha;  // [OK] używa globalnego współczynnika EMA
    if (simLuxSmoothed == 0.0f) {
      simLuxSmoothed = simTarget;
    } else {
      simLuxSmoothed = simLuxSmoothed * (1.0f - SIM_FILTR) + simTarget * SIM_FILTR;
    }

    // [SIM] Udostępnij wartość reszcie systemu
    luxPokojowy           = simLuxSmoothed;
    luxNadWoda            = 0;
    czujnikPokojowyAktywny = true;   // [SIM] system widzi „sprawny czujnik"
    czujnikNadWodaAktywny  = false;  // [SIM] woda nie podłączona

    // [SIM] Log co 30s (taki sam rytm jak hardware)
    static unsigned long simLastLog = 0;
    if (teraz - simLastLog >= 30000UL && !isNightGlobal) {
      simLastLog = teraz;
      logPrintf("lvl=INFO tag=SIM-LUX tryb=%s cel=%.0f wygladzone=%.0f czujnik=symulowany\n",
                simLuxAuto ? "AUTO-SIN" : "STALY",
                simTarget, simLuxSmoothed);
    }
    return; // [SIM] Pomiń cały kod hardware poniżej
  }
  // [SIM] ─── Koniec bloku symulacji - dalej normalny hardware ───
  // ═══════════════════════════════════════════════════════════
  // [OK] FIX-v34-TSL-ASYNC: zlec odczyt do Core 0, użyj cache z poprzedniego cyklu
  tslReadPending = true;

  if (czujnikPokojowyAktywny && tslCachePok >= 0.0f) {
    portENTER_CRITICAL(&tslCacheMux);  // [OK] OPT-v44: spójny odczyt zestawu pól
    float suroweLux  = tslCachePok;
    uint16_t broad   = tslCacheBrPok;
    uint16_t ir      = tslCacheIrPok;
    portEXIT_CRITICAL(&tslCacheMux);

    if (suroweLux > 0.0f) {
      // [OK] FIX-v16-IR-SAT: Sprawdź saturację IR PRZED aktualizacją EMA.
      // Poprzedni kod: wygladzony był już zanieczyszczony złym odczytem zanim sprawdzono saturację,
      // a "rollback" luxPokojowy = wygladzonyLuxPokojowy przypisywał tę samą złą wartość.
      // Dodatkowo detekcja działała tylko co 30s - przez 29.9s brak ochrony.
      // Naprawa: getLuminosity() przy każdym odczycie, EMA aktualizowana TYLKO gdy brak saturacji.
      bool irSaturated = (broad > 0 && ir >= (uint16_t)(broad * 0.85f));

      if (!irSaturated) {
        // FILTR EMA - wartość konfigurowana przez użytkownika (domyślnie 0.25)
        // 0.05=bardzo wolny (odporny na chmury), 0.5=szybki (reaguje natychmiast)
        // [OK] FIX-v39-3: dynamiczny alpha przy dużych skokach lux (zmiana gain TSL2561).
        // Przy skoku >200 lux (przełączenie gain 1x->16x lub chmura) filtr z alpha=0.25
        // potrzebuje ~15 min żeby dojść do nowej wartości -> błędne decyzje adaptacyjne.
        // Przy dużym skoku używamy wyższego alpha (szybsza reakcja), potem wraca do ustawień.
        float _delta      = fabsf(suroweLux - wygladzonyLuxPokojowy);
        float SZYBKI_FILTR = (_delta > 200.0f)
                              ? min(emaFilterAlpha * 2.0f, 0.6f)  // maks 0.6 przy skokach
                              : emaFilterAlpha;                    // normalny tryb
        if (wygladzonyLuxPokojowy == 0) {
          wygladzonyLuxPokojowy = suroweLux;
        } else {
          wygladzonyLuxPokojowy = wygladzonyLuxPokojowy * (1.0f - SZYBKI_FILTR) + suroweLux * SZYBKI_FILTR;
        }
        luxPokojowy = wygladzonyLuxPokojowy;
      }
      // Gdy irSaturated: wygladzony i luxPokojowy bez zmian - poprzednia wartość zachowana

      static unsigned long lastDebug = 0;
      if (millis() - lastDebug > 30000 && !isNightGlobal) {
        if (irSaturated) {
          logPrintf("lvl=WARN tag=TSL msg=\"SATURACJA IR, odczyt odrzucony\" surowe=%.0f wygladzone=%.0f broad=%u ir=%u\n",
            suroweLux, luxPokojowy, broad, ir);
        } else {
          logPrintf("lvl=INFO tag=TSL surowe=%.0f wygladzone=%.0f broad=%u ir=%u\n",
            suroweLux, luxPokojowy, broad, ir);
        }
        lastDebug = millis();
      }
    } else {
      // Czujnik zwrócił 0 - problem! (ale nie loguj w nocy - to normalne)
      if (!isNightGlobal) {
        static unsigned long lastError = 0;
        if (millis() - lastError > 300000) {  // [OK] FIX SPAM: max raz na 5 minut, nie co 10s
          logPrintln("lvl=WARN tag=TSL msg=\"Czujnik zwrocil 0 lux\"");
          lastError = millis();
        }
      }
    }
  }
  
  // ═══════════════════════════════════════════════════════════
  //  CZUJNIK NAD WODĄ (tylko gdy użytkownik go wybrał na www)
  // ═══════════════════════════════════════════════════════════
  // [OK] FIX-v26-WODA-GUARD: Czytaj czujnik nad wodą TYLKO gdy uzywajCzujnikaNadWoda=true.
  // Poprzednio: czujnikNadWodaAktywny=true (hardware obecny) wystarczało, by używać
  // jego wartości w calculateMinLuxPWM() i obliczAdaptacyjnaJasnosc() - nawet gdy
  // użytkownik go nie wybrał. Skutek z log_23: TSL-WODA saturowany (65536 lux) ->
  // calculateMinLuxPWM() zwracał 0 -> LED gasły przez ~1h w przerwie południowej.
  // [OK] FIX-v34-TSL-ASYNC: czujnik nad wodą - dane z cache Core 0
  if (czujnikNadWodaAktywny && uzywajCzujnikaNadWoda && tslCacheWoda >= 0.0f) {
    portENTER_CRITICAL(&tslCacheMux);  // [OK] OPT-v44: spójny odczyt zestawu pól
    float suroweLuxWoda = tslCacheWoda;
    uint16_t broadW     = tslCacheBrWoda;
    uint16_t irW        = tslCacheIrWoda;
    portEXIT_CRITICAL(&tslCacheMux);

    if (suroweLuxWoda > 0.0f) {
      // [OK] FIX-v26-WODA-IR-SAT: Sprawdź saturację IR przed aktualizacją EMA.
      // Czujnik nad wodą jest blisko LED i może nasycać kanał IR (broad=ir=65535).
      // Przy saturacji odczyt jest bezużyteczny - pomijamy EMA (jak dla czujnika pokojowego).
      bool irSaturatedW = (broadW > 0 && irW >= (uint16_t)(broadW * 0.85f));

      if (!irSaturatedW) {
        // [OK] FIX-v39-3: dynamiczny alpha dla czujnika nad wodą - spójne z czujnikiem pokojowym.
        static float wygladzonyLuxNadWoda = 0;
        if (wygladzonyLuxNadWoda == 0) {
          wygladzonyLuxNadWoda = suroweLuxWoda;
        } else {
          float _deltaW     = fabsf(suroweLuxWoda - wygladzonyLuxNadWoda);
          float FILTR_WODA  = (_deltaW > 100.0f) ? 0.5f : 0.3f;  // szybciej przy skokach
          wygladzonyLuxNadWoda = wygladzonyLuxNadWoda * (1.0f - FILTR_WODA) + suroweLuxWoda * FILTR_WODA;
        }
        luxNadWoda = wygladzonyLuxNadWoda;
      }
      // Przy saturacji: luxNadWoda bez zmian - poprzednia wartość zachowana

      static unsigned long lastDebugWoda = 0;
      if (millis() - lastDebugWoda > 30000 && !isNightGlobal) {
        lastDebugWoda = millis();
        if (irSaturatedW) {
          // [OK] FIX-v39-3: usunięto %s + logTime() - logToFile() samo dodaje timestamp.
          // Poprzednio: logPrintf("%s ...", logTime()...) -> podwójny [HH:MM:SS] w pliku (379 wpisów).
          logPrintf("lvl=WARN tag=TSL-WODA msg=\"SATURACJA IR, odrzucony\" surowe=%.0f wygladzone=%.0f broad=%u ir=%u\n",
            suroweLuxWoda, luxNadWoda, broadW, irW);
        } else {
          logPrintf("lvl=INFO tag=TSL-WODA surowe=%.0f wygladzone=%.0f broad=%u ir=%u\n",
            suroweLuxWoda, luxNadWoda, broadW, irW);
        }
      }
    } else {
      // [OK] FIX-v39-4: blokuj log błędu w nocy - 0 lux jest normalne przy zgaszonej lampie.
      // Poprzednio: warunek !isNightGlobal był tylko na debug-log (suroweLuxWoda>0),
      // ale ścieżka błędu (suroweLuxWoda==0) nie miała tej ochrony -> spam co 5 min całą noc.
      if (!isNightGlobal) {
        static unsigned long lastErrorWoda = 0;
        if (millis() - lastErrorWoda > 300000) {
          logPrintln("lvl=WARN tag=TSL-WODA msg=\"Czujnik zwrocil 0 lux\"");
          lastErrorWoda = millis();
        }
      }
    }
  }

  // ═══════════════════════════════════════════════════════════════
  //  AUTO-REINICJALIZACJA TSL2561
  //  Pierwsze 5 prób co 30s, potem co 10 min - log tylko przy zmianie stanu
  // ═══════════════════════════════════════════════════════════════
  // [OK] FIX-v33h: tslLastRetry i tslFailCount przeniesione do globalnych (dostęp z tgTaskFn)
  // Po 5 nieudanych próbach wydłuż interwał do 10 minut
  unsigned long tslRetryInterval = (tslFailCount >= 5) ? 1800000UL : 30000UL;  // [v64-P3a] 10→30 min po 5 próbach
  // [OK] FIX-v23-TSL-NUIT: W nocy TSL zbędny - LED wyłączone, brak słońca.
  // Reinicjalizacja co 10 min przez całą noc generuje zbędne logi.
  // Reset licznika przy wyjściu z nocy -> świeży start o świcie.
  if (isNightGlobal) {
    tslLastRetry = millis();
    tslFailCount = 0;
    return;
  }
  if ((!czujnikPokojowyAktywny || !czujnikNadWodaAktywny)
      && millis() - tslLastRetry > tslRetryInterval) {
    tslLastRetry = millis();
    // [OK] FIX-v33h: reinit I²C (Wire.end/begin + czujnik.begin) przeniesiony do tgTask (Core 0)
    // begin() przy braku urządzenia blokuje ~8400ms (I²C hardware timeout) -> LOOP-SLOW
    // Flaga wybudza tgTask który wykona reinit nie blokując loop()
    if (!tslReinitPending) {
      if (tslFailCount < 5 || tslFailCount % 6 == 0) {
        logPrintf("lvl=INFO tag=TSL msg=\"Proba reinicjalizacji\" numer=%d\n", tslFailCount + 1);
      }
      tslReinitPending = true;
    }
  }
}



/*************************************************************
 * UCZENIE SIĘ
*************************************************************/
void aktualizujUczenie() {
  struct tm infoczas;
  if (!getLocalTimePL(&infoczas)) return;
  
  int godzina = infoczas.tm_hour;
  if (godzina < godzinaStartUczenia || godzina >= godzinaKoniecUczenia) {
    return;
  }
  
  if (!czujnikPokojowyAktywny || luxPokojowy <= 0) return;
  
  adaptacja.sredniaLuxDzien = adaptacja.sredniaLuxDzien * 0.99 + luxPokojowy * 0.01;
  adaptacja.liczbaProbek++;
  
  if (Komentarze && adaptacja.liczbaProbek % 100 == 0) {
    logPrintf("lvl=INFO tag=UCZENIE probek=%u\n", adaptacja.liczbaProbek);
  }
}

/*************************************************************
 * UCZ SIĘ TRANSMISJI (TRYB 2)
*************************************************************/
void uczSieTransmisji() {
  if (!czujnikPokojowyAktywny || !czujnikNadWodaAktywny || !uzywajCzujnikaNadWoda) return;  // [OK] FIX-v26
  
  uint16_t sredniePWM = 0;
  for (int i = 0; i < 5; i++) {
    sredniePWM += getBrightnessForSection(backupBrightnessComposite, i);
  }
  sredniePWM /= 5;
  
  if (sredniePWM < 50 && luxPokojowy > 200) {
    float zmierzonaTransmisja = luxNadWoda / luxPokojowy;
    
    if (zmierzonaTransmisja > 0.05 && zmierzonaTransmisja < 0.50) {
      adaptacja.nauczonaTransmisja = 
        adaptacja.nauczonaTransmisja * 0.98 + zmierzonaTransmisja * 0.02;
      
      if (Komentarze) {
        logPrintf("lvl=INFO tag=TRANSMISJA-UCZENIE zmierzona=%.1f uczona=%.1f\n",
                      zmierzonaTransmisja * 100,
                      adaptacja.nauczonaTransmisja * 100);
      }
    }
  }
}

/*************************************************************
 * GŁÓWNA FUNKCJA - OBLICZ ADAPTACYJNĄ JASNOŚĆ
*************************************************************/
uint16_t obliczAdaptacyjnaJasnosc(uint16_t harmonogramPWM) {
  if (!regulacjaAdaptacyjnaWlaczona) {
    return harmonogramPWM;
  }
  
  // [OK] POPRAWKA: Ochrona przed dzieleniem przez zero (noc/przerwa południowa)
  if (harmonogramPWM == 0) {
    return 0;
  }
  
  float harmonogramLux = pwmToLux(harmonogramPWM);
  
  // KROK 1: Światło naturalne
  float swiatloNaturalne = 0;
  
  // [OK] FIX-v26: TRYB 2 tylko gdy user wybrał czujnik nad wodą (uzywajCzujnikaNadWoda)
  if (czujnikNadWodaAktywny && uzywajCzujnikaNadWoda && czujnikPokojowyAktywny) {
    // TRYB 2 - oba czujniki, nauczona transmisja
    swiatloNaturalne = luxPokojowy * adaptacja.nauczonaTransmisja;
  } else if (czujnikPokojowyAktywny) {
    // TRYB 1 - tylko pokojowy + stała transmisja szyby
    swiatloNaturalne = luxPokojowy * szacujTransmisje(luxPokojowy);
  } else {
    // TRYB 0 - brak czujnika
    // [DIAG-v72] log dlaczego adaptacja zwraca harm bez zmian
    if (Komentarze) {
      static unsigned long _lastAdaptTryb0 = 0;
      if (millis() - _lastAdaptTryb0 >= 60000UL) {
        _lastAdaptTryb0 = millis();
        logPrintf("lvl=INFO tag=ADAPT-RESULT tryb=0-brak-czujnika harm=%d result=%d passthrough=tak czPok=%s czWoda=%s\n",
          harmonogramPWM, harmonogramPWM,
          czujnikPokojowyAktywny ? "T" : "F",
          czujnikNadWodaAktywny ? "T" : "F");
      }
    }
    return harmonogramPWM;
  }
  
  // KROK 2: Propozycja
  // [OK] FIX-v21: suma słońce+LED = dokładnie tyle lux ile ustawiono suwakiem
  float propozycjaLED = harmonogramLux - swiatloNaturalne;
  
  // KROK 3: Walidacja
  float sumaLux = swiatloNaturalne + propozycjaLED;
  
  if (sumaLux < minLuxDlaRoslin) {
    // [OK] Nie nadpisuj gdy applyMinLuxMode() właśnie steruje rampą
    if (!rampaMinLuxPriorytet) {
      propozycjaLED = minLuxDlaRoslin - swiatloNaturalne;
      statystyki.korektyMinimum++;
      if (Komentarze) {
        static unsigned long _lastKorMin = 0;
        if (millis() - _lastKorMin > 60000) {
          logPrintf("lvl=WARN tag=ADAPT-KOREKTA akcja=min nat=%.0f min=%.0f dobijam=%.0f pwm=%d\n",
                    swiatloNaturalne, minLuxDlaRoslin,
                    propozycjaLED, luxToPwm(propozycjaLED));
          _lastKorMin = millis();
        }
      }
    }
  }
  
  if (sumaLux > maxBezpiecznyLux) {
    propozycjaLED = maxBezpiecznyLux - swiatloNaturalne;
    statystyki.korektyMaksimum++;
    if (Komentarze) {
      static unsigned long _lastKorMax = 0;
      if (millis() - _lastKorMax > 60000) {
        logPrintf("lvl=WARN tag=ADAPT-KOREKTA akcja=max nat=%.0f max=%.0f redukcja_do=%.0f pwm=%d\n",
                  swiatloNaturalne, maxBezpiecznyLux,
                  propozycjaLED, luxToPwm(propozycjaLED));
        _lastKorMax = millis();
      }
    }
  }
  
  if (propozycjaLED < 0) {
    propozycjaLED = 0;
  }
  
  float maxDozwoloneLED = harmonogramLux * maxLimitLED;
  if (propozycjaLED > maxDozwoloneLED) {
    propozycjaLED = maxDozwoloneLED;
  }
  
  uint16_t finalnePWM = luxToPwm(propozycjaLED);

  // [OK] PRIORYTET: Kompromis adaptacja/MIN LUX
  // Adaptacja może redukować LED, ale nigdy poniżej wartości MIN LUX
  if (minLuxModeEnabled && minLuxCurrentPWM[0] > 0 && finalnePWM < minLuxCurrentPWM[0]) {
    if (Komentarze) {
      static unsigned long _lastMinLuxClamp = 0;
      if (millis() - _lastMinLuxClamp > 60000) {
        logPrintf("lvl=WARN tag=ADAPT-CLAMP wynik=%d min_lux_pwm=%d\n",
                  finalnePWM, minLuxCurrentPWM[0]);
        _lastMinLuxClamp = millis();
      }
    }
    finalnePWM = minLuxCurrentPWM[0];
  }

  // [OK] FIX-v16-STATS: Statystyki redukcji zbierane niezależnie od flagi Komentarze.
  // Poprzednio: liczbaPomiarow++ i sredniaRedukcja++ były wewnątrz if(Komentarze) ->
  // przy wyłączonych logach dashboard pokazywał 0% redukcji mimo aktywnej adaptacji.
  {
    static unsigned long _lastStatUpdate = 0;
    unsigned long _nowMs2 = millis();
    if (_nowMs2 - _lastStatUpdate > 30000) {
      _lastStatUpdate = _nowMs2;
      float hPWM2 = (float)harmonogramPWM;
      float redukcja2 = (hPWM2 > 0.0f) ? ((1.0f - (float)finalnePWM / hPWM2) * 100.0f) : 0.0f;
      statystyki.liczbaPomiarow++;
      statystyki.sredniaRedukcja += redukcja2;
    }
  }

  if (Komentarze) {
    static unsigned long _lastAdaptLog = 0;
    static uint16_t _lastLoggedPWM = 9999;
    unsigned long _nowMs = millis();
    bool _pwmChanged = abs((int)finalnePWM - (int)_lastLoggedPWM) > 5;
    bool _heartbeat  = (_nowMs - _lastAdaptLog > 300000);  // [OK] FIX-v29: heartbeat co 5 min (było 30s -> spam)
    // [OK] FIX-v29-ADAPT-LOG: Loguj tylko gdy PWM się zmienił lub heartbeat 5 min.
    // Poprzednio: co 30s bezwarunkowo -> dziesiątki identycznych wpisów/h przy stabilnym świetle.
    if (_pwmChanged || _heartbeat) {
      float hPWM = (float)harmonogramPWM;
      float redukcja = (hPWM > 0.0f) ? ((1.0f - (float)finalnePWM / hPWM) * 100.0f) : 0.0f;
      logPrintf("lvl=INFO tag=ADAPT nat=%.0f led=%.0f suma=%.0f harm_pwm=%d adapt_pwm=%d redukcja=%.0f transmisja=%.0f\n",
                swiatloNaturalne, propozycjaLED,
                propozycjaLED + swiatloNaturalne,
                harmonogramPWM, finalnePWM, redukcja,
                (czujnikNadWodaAktywny && czujnikPokojowyAktywny)
                  ? adaptacja.nauczonaTransmisja * 100.0f
                  : szacujTransmisje(luxPokojowy) * 100.0f);
      _lastAdaptLog = _nowMs;
      _lastLoggedPWM = finalnePWM;
    }
  }
  
  return finalnePWM;
}

/*************************************************************
 * RAMPA ADAPTACYJNA - START
*************************************************************/
// ═══════════════════════════════════════════════════════════════════
// DEBUG: śledzi skąd rampa jest wywoływana (przekaż stałą tekstową)
// ═══════════════════════════════════════════════════════════════════

bool zastosujRampeAdaptacyjna(uint16_t nowiePWM[5], const char* caller = "?") {
  // OCHRONA przed kolizją: nie nadpisuj rampy uruchomionej przez applyMinLuxMode()
  if (rampaMinLuxPriorytet) {
    if (Komentarze) logPrintf("lvl=WARN tag=RAMPA msg=\"Zablokowana, priorytet MIN LUX\" wywolal=%s\n", caller);
    return false;
  }

  const bool explicitUserRamp = (caller && (strcmp(caller, "firebase") == 0 || strcmp(caller, "http") == 0 || strcmp(caller, "telegram") == 0 || strcmp(caller, "websocket") == 0));

  // ── DEBUG: wykryj ponowne wywołanie gdy rampa już aktywna ──
  if (rampaAdaptacyjnaAktywna && Komentarze) {
    logPrintf("lvl=WARN tag=RAMP-DBG msg=\"wywolana gdy rampa AKTYWNA\" ms=%lu caller=%s poprzedni=%s\n",
              millis(), caller, _ostatniWywolujacyRampe);
  }

  _ostatniWywolujacyRampe = caller;
  _ostatniStartRampy = millis();

  // [OK] FIX-v35: Sprawdź czy jest cokolwiek do zrobienia przed ustawieniem flagi
  {
    bool anyWork = false;
    for (int i = 0; i < 5; i++) {
      uint16_t aktualne = getBrightnessForSection(backupBrightnessComposite, i);
      if (aktualne != nowiePWM[i]) { anyWork = true; break; }
    }
    if (!anyWork) {
      if (Komentarze) logPrintf("lvl=WARN tag=RAMP-DBG msg=\"pominieta, brak roznicy\" caller=%s\n", caller);
      return true;  // poprawna komenda, tylko brak zmiany
    }
  }

  if (!rampArbiterTryStart(RAMP_AUTO_ADAPTIVE, explicitUserRamp)) return false;
  rampaAdaptacyjnaAktywna = true;
  gPowerScale = 1.0f; gPowerScaleLastLogged = 1.0f;  // balancer nie walczy z rampą
  unsigned long teraz = millis();
  unsigned long calyczas = ADAPTIVE_RAMP_SECONDS * 1000UL;
  
  if (Komentarze) {
    logPrintf("lvl=INFO tag=RAMPA akcja=start wywolal=%s od=%d do=%d plan=%ds\n",
              caller,
              getBrightnessForSection(backupBrightnessComposite, 0),
              nowiePWM[0],
              ADAPTIVE_RAMP_SECONDS);
  }

  for (int i = 0; i < 5; i++) {
    uint16_t aktualne = getBrightnessForSection(backupBrightnessComposite, i);
    rampaAdaptacyjnaAktualne[i] = aktualne;
    rampaAdaptacyjnaCel[i] = nowiePWM[i];
    rampaAdaptacyjnaKrok[i] = teraz;
    
    int roznica = abs((int)nowiePWM[i] - (int)aktualne);
    if (roznica > 0) {  // OCHRONA!
      rampaAdaptacyjnaInterval[i] = calyczas / roznica;
      // ── DEBUG: log parametrów każdego kanału ──
      if (Komentarze) {
        logPrintf("lvl=INFO tag=RAMP-CH kanal=%d aktualne=%d cel=%d roznica=%d interval_ms=%lu\n",
                  i, aktualne, nowiePWM[i], roznica, rampaAdaptacyjnaInterval[i]);
      }
    } else {
      rampaAdaptacyjnaInterval[i] = 1000;  // Domyślnie 1s
      if (Komentarze) {
        logPrintf("lvl=INFO tag=RAMP-CH kanal=%d aktualne=%d cel=%d roznica=0 msg=\"pomijam kanal\"\n", i, aktualne, aktualne);
      }
    }
  }
  return true;
}




/*************************************************************
 * RAMPA ADAPTACYJNA - UPDATE
*************************************************************/
void aktualizujRampeAdaptacyjna() {
  if (!rampaAdaptacyjnaAktywna) return;
  
  unsigned long teraz = millis();
  bool wszystkieGotowe = true;
  
  for (int i = 0; i < 5; i++) {
    if (rampaAdaptacyjnaAktualne[i] == rampaAdaptacyjnaCel[i]) continue;
    wszystkieGotowe = false;
    
    if (teraz - rampaAdaptacyjnaKrok[i] >= rampaAdaptacyjnaInterval[i]) {
      bool idziemyDol = (rampaAdaptacyjnaAktualne[i] > rampaAdaptacyjnaCel[i]);
      if (idziemyDol) {
        rampaAdaptacyjnaAktualne[i]--;
      } else {
        rampaAdaptacyjnaAktualne[i]++;
      }
      
      // BEZPIECZNIK: nie przekrocz celu (overshoot)
      // [OK] FIX #3: Poprzedni warunek abs(_akt - _cel) > 5 strzelał ZAWSZE gdy rampa
      // dopiero startowała z wartości dalekiej od celu (np. 294->71: abs(293-71)=222 > 5).
      // Powodowało to natychmiastowy SNAP zamiast płynnej 30s rampy.
      // Poprawka: sprawdzaj wyłącznie OVERSHOOT (przeskoczenie celu), nie odległość od celu.
      int _cel = (int)rampaAdaptacyjnaCel[i];
      int _akt = (int)rampaAdaptacyjnaAktualne[i];
      bool overshoot = idziemyDol ? (_akt < _cel) : (_akt > _cel);
      if (overshoot) {
        // ── DEBUG: bezpiecznik strzelił ──
        if (Komentarze) {
          logPrintf("lvl=WARN tag=BEZPIECZNIK kanal=%d wartosc=%d cel=%d akcja=snap msOdStartu=%lu\n",
                    i, rampaAdaptacyjnaAktualne[i], rampaAdaptacyjnaCel[i],
                    teraz - _ostatniStartRampy);
        }
        rampaAdaptacyjnaAktualne[i] = rampaAdaptacyjnaCel[i];
      }
      
      setBrightnessForSection(backupBrightnessComposite, i, rampaAdaptacyjnaAktualne[i]);
      rampaAdaptacyjnaKrok[i] = teraz;
    }
  }
  
  if (wszystkieGotowe) {
    unsigned long czas_trwania = teraz - _ostatniStartRampy;

    // [OK] FIX MINLUX-BACKUPAUTO: NIE synchronizuj backupAuto gdy rampę wywołał applyMinLuxMode()!
    // MIN LUX to TYMCZASOWE doświetlanie - nie może nadpisać nastaw harmonogramu (np. 294 PWM).
    // Poprzednio: rampa MIN LUX do 72 PWM -> backupAuto=294 stawało się 72
    //   -> następny cykl: maxAllowedPWM=72×0.8=57 -> rampa do 57 -> autoB=57 -> ...
    //   -> logarytmicznie malejące PWM każdego cyklu aż do ~0. EEPROM też corrupted!
    bool _bylaRampaMinLux = rampaMinLuxPriorytet;  // zapamiętaj PRZED resetem!
    rampaAdaptacyjnaAktywna = false;
    rampaMinLuxPriorytet = false;
    rampArbiterRelease(RAMP_AUTO_ADAPTIVE);

    // [OK] FIX-v19c: NIE synchronizuj backupAuto z wynikiem rampy adaptacyjnej.
    // Poprzednio: po zakończeniu rampy (np. 312->234) backupAuto stawało się 234.
    // Następna iteracja adaptacji: harm=234 -> adapt=221 -> backupAuto=221 -> harm=221...
    // -> spirala logarytmiczna w dół co 5s aż do 0 (identyczny bug jak MIN LUX, opisany wyżej).
    // backupAutoBrightnessComposite = nastawienie użytkownika (read-only dla adaptacji).
    // Po restarcie soft-start celuje w pełny harmonogram (312) i adaptacja redukuje od nowa.
    // Guard MIN LUX (bylaRampaMinLux) pozostawiony dla czytelności kodu - oba przypadki = false.
    if (false) {
      // martwy kod - zachowany strukturalnie
      for (int i = 0; i < 5; i++) {
        uint16_t finalPwm = getBrightnessForSection(backupBrightnessComposite, i);
        setBrightnessForSection(backupAutoBrightnessComposite, i, finalPwm);
      }
    }

    if (Komentarze) {
      logPrintf("lvl=INFO tag=RAMPA-KONIEC wywolal=%s pwm=%d czas=%lus\n",
                _ostatniWywolujacyRampe,
                getBrightnessForSection(backupBrightnessComposite, 0),
                czas_trwania / 1000);
      if (czas_trwania < 500) {
        logPrintf("lvl=WARN tag=ANOMALIA msg=\"Rampa zakonczona natychmiast\" ms=%lu\n",
                  czas_trwania);
      }
    }
  }
}


/*************************************************************
 * EEPROM - ZAPIS/ODCZYT ADAPTACJI
*************************************************************/
void zapiszAdaptacjeDoEEPROM() {
  uint8_t flaga = 123;
  EepromWritePart p[4];
  if (eepromBuildPart(p[0], EEPROM_ADRES_ADAPT_SREDNIA, &adaptacja.sredniaLuxDzien, sizeof(adaptacja.sredniaLuxDzien)) &&
      eepromBuildPart(p[1], EEPROM_ADRES_ADAPT_PROBKI, &adaptacja.liczbaProbek, sizeof(adaptacja.liczbaProbek)) &&
      eepromBuildPart(p[2], EEPROM_ADRES_ADAPT_TRANSMISJA, &adaptacja.nauczonaTransmisja, sizeof(adaptacja.nauczonaTransmisja)) &&
      eepromBuildPart(p[3], EEPROM_ADRES_ADAPT_POPRAWNE, &flaga, sizeof(flaga))) {
    (void)enqueueEepromBatch(p, 4, "adaptacja");
  }
  if (Komentarze) logPrintln("lvl=INFO tag=EEPROM msg=\"Adaptacja zapisana\"");
}

void odczytajAdaptacjeZEEPROM() {
  uint8_t flaga = 0;
  EEPROM.get(EEPROM_ADRES_ADAPT_POPRAWNE, flaga);
  
  if (flaga != 123) {
    if (Komentarze) {
      logPrintln("lvl=INFO tag=EEPROM msg=\"Brak danych adaptacji, pierwszy start\"");
    }
    return;
  }
  
  EEPROM.get(EEPROM_ADRES_ADAPT_SREDNIA, adaptacja.sredniaLuxDzien);
  EEPROM.get(EEPROM_ADRES_ADAPT_PROBKI, adaptacja.liczbaProbek);
  EEPROM.get(EEPROM_ADRES_ADAPT_TRANSMISJA, adaptacja.nauczonaTransmisja);
  
  // [OK] POPRAWKA: Walidacja - uszkodzone dane mogą powodować crash
  bool dataOK = true;
  if (isnan(adaptacja.nauczonaTransmisja) || adaptacja.nauczonaTransmisja <= 0 || adaptacja.nauczonaTransmisja > 1.0f) dataOK = false;
  if (isnan(adaptacja.sredniaLuxDzien)   || adaptacja.sredniaLuxDzien < 0) dataOK = false;
  
  if (!dataOK) {
    logPrintln("lvl=WARN tag=EEPROM msg=\"Dane adaptacji uszkodzone, reset do domyslnych\"");
    adaptacja.sredniaLuxDzien   = 500;
    adaptacja.liczbaProbek      = 0;
    adaptacja.nauczonaTransmisja = 0.30;
    return;
  }
  
  if (Komentarze) {
    logPrintf("lvl=INFO tag=EEPROM msg=\"Adaptacja odczytana\" probek=%u\n", adaptacja.liczbaProbek);
  }
}

void inteligentyZapisAdaptacji() {
  static uint32_t ostatniZapisaneProbki = 0;
  unsigned long teraz = millis();
  
  bool czasNaZapis = (teraz - ostatniZapisAdaptacji > intervalZapisuAdaptacji);
  bool duzoProbek = (adaptacja.liczbaProbek - ostatniZapisaneProbki > 100);
  
  if (czasNaZapis || duzoProbek) {
    zapiszAdaptacjeDoEEPROM();
    ostatniZapisAdaptacji = teraz;
    ostatniZapisaneProbki = adaptacja.liczbaProbek;
  }
}

/************************************************************
 * ZAPIS LOGÓW DO LITTLEFS
 ************************************************************/
void logToFile(const String &message) {
  if (!littlefsReady) return;

  // [OK] FIX-v33d-LOG-MUTEX: Dwa niezależne bufory per-rdzeń.
  //
  // PROBLEM (v33b i wcześniej): static logBuffer był WSPÓLNY dla obu rdzeni.
  // Dwa rdzenie jednocześnie modyfikowały ten sam String (operator+=) bez
  // synchronizacji -> korupcja heap + VFS mutex blokował jeden rdzeń do 1700ms.
  //
  // PROBLEM (v33c): mutex portMAX_DELAY -> Core 1 czekał aż Core 0 skończy zapis
  // LittleFS (rotacja nawet >1.7s) -> LOOP-SLOW nadal obecny.
  //
  // ROZWIĄZANIE (v33d):
  //   1. Każdy rdzeń ma WŁASNY bufor (logBuffer[0] dla Core 0, [1] dla Core 1).
  //      Append do bufora bez mutexa - jeden rdzeń pisze tylko do swojego bufora.
  //   2. Mutex chroni wyłącznie ZAPIS DO LittleFS (sekcja krytyczna I/O).
  //   3. Core 1 (loop): timeout=0 - jeśli mutex zajęty, SKIP flush (wiadomość
  //      zostaje w buforze, zostanie zapisana przy następnym wywołaniu).
  //      -> loop() NIGDY nie blokuje na logowaniu -> zero LOOP-SLOW.
  //   4. Core 0 (tgTask): portMAX_DELAY - czeka na mutex; tgTask nie jest
  //      time-critical, nie traci wpisów.

  const int core = (int)xPortGetCoreID();  // 0 lub 1 (bezpieczne z obu rdzeni)
  const int idx  = (core == 0) ? 0 : 1;

  // ── Bufory i timestampy per-rdzeń (dostęp tylko z jednego rdzenia - brak wyścigu) ──
  // [v54]: g_logBuf/g_logBufLen alokowane w PSRAM (setup), g_lastFlush w DRAM
  char**        logBuf    = g_logBuf;
  size_t*       logBufLen = g_logBufLen;
  unsigned long (&lastFlush)[2] = g_lastFlush;

  // Jeśli bufor jeszcze nie przydzielony (np. bardzo wczesne logowanie przed setup())
  // wróć bez zapisu – nie crash.
  if (!logBuf[idx]) return;

  // [v54] Helper: bezpieczny append C-stringa do bufora PSRAM
  auto logAppend = [&](const char* s) {
    size_t rem = LOG_BUF_SIZE - logBufLen[idx] - 1;
    if (rem == 0) return;
    size_t n = strnlen(s, rem);
    memcpy(logBuf[idx] + logBufLen[idx], s, n);
    logBufLen[idx] += n;
    logBuf[idx][logBufLen[idx]] = '\0';
  };

  const unsigned long FLUSH_INTERVAL_MS = 5000;  // v43: 2000->5000ms, mniej write-cycles
  const size_t        FLUSH_THRESHOLD   = 7000;  // v54: próg flush (bufor 8192 B)

  // Dodaj timestamp + wiadomość do PER-RDZENIOWEGO bufora (bez mutexa - bezpieczne)
  // [FIX-v128-LOG-TIMESTAMP-MS] Dopisano ms= — dwa niezależne bufory per-rdzeń
  // z asynchronicznym flush powodują że kolejność linii w pliku NIE odpowiada
  // kolejności zdarzeń. ms= daje monotoniczny znacznik do sortowania post-factum.
  unsigned long now = millis();  // jeden millis() dla timestamp I shouldFlush
  struct tm logTm;
  char timeBuf[40];
  uint32_t _thisSeq = g_logSeq.fetch_add(1, std::memory_order_relaxed) + 1;  // [v235] atomowy inkrement, zwraca wartość TEJ linii
  if (getLocalTimePL(&logTm)) {
    snprintf(timeBuf, sizeof(timeBuf), "[%02d:%02d:%02d ms=%lu] seq=%lu ",
             logTm.tm_hour, logTm.tm_min, logTm.tm_sec, now, (unsigned long)_thisSeq);
  } else {
    snprintf(timeBuf, sizeof(timeBuf), "[??:??:?? ms=%lu] seq=%lu ", now, (unsigned long)_thisSeq);
  }
  logAppend(timeBuf);
  logAppend(message.c_str());
  if (!message.endsWith("\n")) logAppend("\n");

  bool shouldFlush = (now - lastFlush[idx] >= FLUSH_INTERVAL_MS) ||
                     (logBufLen[idx] >= FLUSH_THRESHOLD);
  if (!shouldFlush) return;

  // ── [v46] LOG-FLUSH-CORE0: Core 1 nie pisze do LittleFS bezpośrednio ────
  // Zapis 8KB blokował loop() przez 200-500ms -> LOOP-SLOW.
  // Core 1: ustaw flagę i wróć natychmiast. tgTask (Core 0) wykona zapis.
  // Core 0: flushuje bezpośrednio (tgTask nie jest time-critical).
  if (idx == 1) {
    lastFlush[idx] = now;  // reset timera żeby nie triggerować co wywołanie
    logFlushPending[1] = true;
    return;
  }

  // ── Sekcja krytyczna: zapis do LittleFS (Core 0 tylko) ──────────────────
  if (logMutex) {
    if (xSemaphoreTake(logMutex, portMAX_DELAY) != pdTRUE) {
      return;
    }
  }

  lastFlush[idx] = now;

  // ── CIRCULAR LOG USUNIĘTY (FIX-v43) ────────────────────────────────────────
  // ROOT CAUSE: blok CIRCULAR LOG otwierał fCheck+fSrc+fDst w jednej sekcji krytycznej.
  // Przy lfs_file_opencfg() z alokacją wewnątrz, częściowe wejście do mlist + błąd
  // alloc -> destruktor File woła lfs_file_close na pliku nie będącym w mlist -> assert.
  // NAPRAWA: usunięto CIRCULAR LOG z logToFile. Przycinanie LOG_FILE obsługuje
  // wyłącznie fsSizeGuard/fsGuardPending (Core 0, bezpieczne, co 60s).
  // fsSizeGuard już trymuje LOG_FILE do 256KB gdy FS > 68% pełny - redundancja usunięta.

  // Dopisz bufor na koniec pliku
  // Guard heap: LittleFS.open() przy sfragmentowanym HEAP może wywołać assert lfs_file_close.
  // Próg 8192B = bezpieczny margines (lfs wewnętrznie alokuje cache 512B + metadane,
  // a z fragmentacją blok ciągły może być mniejszy niż sumaryczne free).
  if (heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) < 8192) {
    Serial.printf("lvl=WARN tag=LOG-GUARD site=logToFile msg=\"heap fragm, zapis LittleFS pominiety, bufor zrzucony\" largestBlk=%uB\n",
                  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    logBuf[idx][0] = '\0'; logBufLen[idx] = 0;
    if (logMutex) xSemaphoreGive(logMutex);
    return;
  }
  // [v157] DIAG-FLASHSTALL: zmierz czas open+write+close — ta sekcja biegnie
  // na Core 0 (tgTask) i wg dokumentacji ESP-IDF (spi_flash_disable_cache)
  // może stalować Core 1 (loop()/updatePump()) na czas trwania operacji.
  unsigned long _flashT0 = millis();
  File f = LittleFS.open(LOG_FILE_B, "a");  // [v102] log_b.txt
  if (!f) {
    logBuf[idx][0] = '\0'; logBufLen[idx] = 0;
    if (logMutex) xSemaphoreGive(logMutex);
    return;
  }
  if (logBufLen[idx] > 0) {
    f.write((uint8_t*)logBuf[idx], logBufLen[idx]);
  }
  g_logFileSize = f.size();  // [v102] cache rozmiaru log_b przed zamknięciem
  f.close();
  unsigned long _flashMs = millis() - _flashT0;
  if (_flashMs >= FLASH_STALL_THRESHOLD_MS) {
    g_flashStallCount = g_flashStallCount + 1;
    if (_flashMs > g_flashStallMaxMs) g_flashStallMaxMs = _flashMs;
    Serial.printf("lvl=WARN tag=FLASH-STALL site=logToFile ms=%lu bytes=%u cnt=%lu maxMs=%lu\n",
                  _flashMs, (unsigned)logBufLen[idx],
                  (unsigned long)g_flashStallCount, (unsigned long)g_flashStallMaxMs);
    // Uwaga: celowo Serial.printf zamiast logPrintf/logToFile — jesteśmy
    // wewnątrz logToFile() samego, rekurencyjne wywołanie zapisu do LittleFS
    // tuż po zmierzonym stallu tylko pogłębiłoby badany problem.
  }
  logBuf[idx][0] = '\0'; logBufLen[idx] = 0;

  esp_task_wdt_reset();
  if (logMutex) xSemaphoreGive(logMutex);
}



/*************************************************************
 *  WEBSERIAL - FUNKCJE LOGOWANIA Z WYBOREM
 *************************************************************/

// [OK] FIX #5: Kolejka buforująca dla WebSocket - thread safety.
// wsTerminal.textAll() może być wywołane z głównego loop() podczas gdy AsyncTCP
// przetwarza dane na osobnym zadaniu FreeRTOS -> potencjalny crash/korupcja heapu.
// Rozwiązanie: logPrint* zapisuje wiadomości do kolejki FIFO (chronionej mutexem),
// a flushWsQueue() wywoływana z loop() bezpiecznie wysyła je do WebSocket.
//
// [v62-4] _wsQueue: String[] → char[WS_QUEUE_SIZE][WS_MSG_MAX_LEN]
// String alokuje dynamicznie na heap przy każdym enqueue/dequeue → fragmentacja.
// Stała tablica char ląduje w segmencie BSS (nie heap) → zero alokacji dynamicznych.
// 32 × 160 = 5 120 B w BSS zamiast dynamicznych mikro-alokacji String.
// Wiadomości dłuższe niż WS_MSG_MAX_LEN-1 są przycinane (strncpy z NUL terminator).
#define WS_QUEUE_SIZE  32
#define WS_MSG_MAX_LEN 200   // max długość jednego wpisu WS; było 160 — za małe dla linii firmware version (165 zn.)
static char    _wsQueue[WS_QUEUE_SIZE][WS_MSG_MAX_LEN];
static uint8_t _wsQueueHead = 0;
static uint8_t _wsQueueTail = 0;
static portMUX_TYPE _wsQueueMux = portMUX_INITIALIZER_UNLOCKED;

static void _wsEnqueue(const String &msg) {
  portENTER_CRITICAL(&_wsQueueMux);
  uint8_t nextTail = (_wsQueueTail + 1) % WS_QUEUE_SIZE;
  if (nextTail != _wsQueueHead) {  // nie nadpisuj gdy pełna
    strncpy(_wsQueue[_wsQueueTail], msg.c_str(), WS_MSG_MAX_LEN - 1);
    _wsQueue[_wsQueueTail][WS_MSG_MAX_LEN - 1] = '\0';
    _wsQueueTail = nextTail;
  }
  portEXIT_CRITICAL(&_wsQueueMux);
}

// Wywołaj raz w loop() - wysyła kolejkę do WS z bezpiecznego kontekstu
void flushWsQueue() {
  if (!wsReady) return;
  // [v255] WS-BURST-GUARD: nie opróżniaj całego ringa w jednej iteracji loop().
  // Przy spiętrzeniu logów 32x textAll() naraz może przeciążyć AsyncTCP klienta LAN.
  // Maks. 8 wiadomości/iterację daje transportowi i Firebase miejsce na pracę.
  uint8_t flushed = 0;
  while (flushed < 8) {
    portENTER_CRITICAL(&_wsQueueMux);
    if (_wsQueueHead == _wsQueueTail) { portEXIT_CRITICAL(&_wsQueueMux); break; }
    // Kopiuj lokalnie przed zwolnieniem mux aby minimalizować czas w sekcji krytycznej
    char msgBuf[WS_MSG_MAX_LEN];
    strncpy(msgBuf, _wsQueue[_wsQueueHead], WS_MSG_MAX_LEN - 1);
    msgBuf[WS_MSG_MAX_LEN - 1] = '\0';
    _wsQueueHead = (_wsQueueHead + 1) % WS_QUEUE_SIZE;
    portEXIT_CRITICAL(&_wsQueueMux);
    wsTerminal.textAll(msgBuf);  // textAll(const char*) — bez dodatkowego String
    ++flushed;
  }
}

void logPrint(const String &msg) {
  if (logMode == LOG_OFF) return;
  if (logMode == LOG_SERIAL_ONLY || logMode == LOG_BOTH) {
    Serial.print(msg);
  }
  if ((logMode == LOG_WIFI_ONLY || logMode == LOG_BOTH) && wsReady) {
    _wsEnqueue(msg);
  }
  // [OK] FIX BUG-N: Zapisuj do pliku (logPrint był jedyną funkcją bez logToFile())
  if (littlefsReady) {
    logToFile(msg);
  }
}

// [v46] LOG-FLUSH-CORE0: faktyczny zapis bufora Core 1 do LittleFS.
// Wywoływana z tgTask (Core 0) gdy logFlushPending[1]=true.
// MUSI być wywoływana pod mutexem logMutex (tgTask go bierze przed wywołaniem).
void logFlushCore1() {
  PRE_RESET_CP("LOG-FLUSH");  // [v71]
  if (!littlefsReady) return;
  if (!g_logBuf[1] || g_logBufLen[1] == 0) return;
  if (heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) < 8192) {
    Serial.printf("lvl=WARN tag=LOG-GUARD site=logFlushCore1 msg=\"heap fragm, bufor zrzucony\" largestBlk=%uB\n",
                  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    g_logBuf[1][0] = '\0'; g_logBufLen[1] = 0;
    return;
  }
  // [v157] DIAG-FLASHSTALL: ten zapis też biegnie na Core 0 (tgTask flushuje
  // bufor Core 1) — ten sam potencjalny mechanizm stallu jak w logToFile().
  unsigned long _flashT0 = millis();
  File f = LittleFS.open(LOG_FILE_B, "a");  // [v102]
  if (!f) { g_logBuf[1][0] = '\0'; g_logBufLen[1] = 0; return; }
  f.write((uint8_t*)g_logBuf[1], g_logBufLen[1]);
  f.close();
  unsigned long _flashMs = millis() - _flashT0;
  if (_flashMs >= FLASH_STALL_THRESHOLD_MS) {
    g_flashStallCount = g_flashStallCount + 1;
    if (_flashMs > g_flashStallMaxMs) g_flashStallMaxMs = _flashMs;
    Serial.printf("lvl=WARN tag=FLASH-STALL site=logFlushCore1 ms=%lu bytes=%u cnt=%lu maxMs=%lu\n",
                  _flashMs, (unsigned)g_logBufLen[1],
                  (unsigned long)g_flashStallCount, (unsigned long)g_flashStallMaxMs);
  }
  g_logBuf[1][0] = '\0'; g_logBufLen[1] = 0;
  esp_task_wdt_reset();
}

// [v71] NAPRAWIONY logForceFlush() — faktycznie flushuje oba bufory PSRAM do LittleFS.
// Poprzednia implementacja (v43) pisala tylko marker, tracac g_logBuf[0/1] (do 16 KB).
// WYWOLUJ: przed kazdym ESP.restart(). Nie sprawdza mutexa — system i tak idzie w dol.
void logForceFlush() {
  if (!littlefsReady) return;
  File f = LittleFS.open(LOG_FILE_B, "a");  // [v102]
  if (!f) return;
  // Flushuj bufor Core 0 (tgTask)
  if (g_logBuf[0] && g_logBufLen[0] > 0) {
    f.write((const uint8_t*)g_logBuf[0], g_logBufLen[0]);
    g_logBuf[0][0] = '\0';
    g_logBufLen[0] = 0;
  }
  // Flushuj bufor Core 1 (loop)
  if (g_logBuf[1] && g_logBufLen[1] > 0) {
    f.write((const uint8_t*)g_logBuf[1], g_logBufLen[1]);
    g_logBuf[1][0] = '\0';
    g_logBufLen[1] = 0;
  }
  f.print("lvl=INFO tag=FORCE-FLUSH msg=\"koniec buforow przed restartem\"\n");
  f.flush();  // jawne flush przed close — gwarantuje zapis do flash
  f.close();
}

void logPrintln(const String &msg) {
  if (logMode == LOG_OFF) return;
  if (logMode == LOG_SERIAL_ONLY || logMode == LOG_BOTH) {
    Serial.println(msg);
  }
  if ((logMode == LOG_WIFI_ONLY || logMode == LOG_BOTH) && wsReady) {
    _wsEnqueue(msg + "\n");
  }
  if (littlefsReady) {
    logToFile(msg);
  }
}

// [OK] OPT-v44: overloady const char* - zero alokacji String dla wywołań z literałami
// ~200 wywołań logPrint/logPrintln("stały tekst") w kodzie tworzyło tymczasowy String.
// Overload wybierany przez kompilator automatycznie (najlepsze dopasowanie).
void logPrint(const char* msg) {
  if (logMode == LOG_OFF) return;
  if (logMode == LOG_SERIAL_ONLY || logMode == LOG_BOTH) Serial.print(msg);
  if ((logMode == LOG_WIFI_ONLY || logMode == LOG_BOTH) && wsReady) _wsEnqueue(msg);
  if (littlefsReady) logToFile(msg);
}
void logPrintln(const char* msg) {
  if (logMode == LOG_OFF) return;
  if (logMode == LOG_SERIAL_ONLY || logMode == LOG_BOTH) Serial.println(msg);
  if ((logMode == LOG_WIFI_ONLY || logMode == LOG_BOTH) && wsReady) {
    String _s(msg); _s += '\n'; _wsEnqueue(_s);
  }
  if (littlefsReady) logToFile(msg);
}


void logPrintf(const char* format, ...) {
  if (logMode == LOG_OFF) return;
  
  // [OK] POPRAWKA: 384 zamiast 256 - emoji UTF-8 zajmują 3-4 bajty każde
  char buffer[384];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  
  if (logMode == LOG_SERIAL_ONLY || logMode == LOG_BOTH) {
    Serial.print(buffer);
  }
  
  // [OK] OPT-v44: jedna alokacja String zamiast dwóch - buffer użyty dwukrotnie
  if ((logMode == LOG_WIFI_ONLY || logMode == LOG_BOTH || littlefsReady) && wsReady) {
    String _s(buffer);
    if ((logMode == LOG_WIFI_ONLY || logMode == LOG_BOTH) && wsReady) _wsEnqueue(_s);
    if (littlefsReady) logToFile(_s);
  } else if (littlefsReady) {
    logToFile(String(buffer));
  }
}

// [v230] FIX-LOG-VOLUME: identyczna jak logPrintf(), ale BEZ logToFile() —
// dla rutynowych potwierdzeń sukcesu, które i tak nigdy nie są potrzebne do
// diagnozy (błąd tej samej operacji i tak leci osobną, nietkniętą ścieżką
// logPrintf() na lvl=ERR/WARN). Serial + WebSocket bez zmian — podgląd na
// żywo widzi wszystko; oszczędność dotyczy wyłącznie zapisu na flash, czyli
// tego, co realnie napędza objętość pliku i częstotliwość FS-GUARD.
void logPrintfNoFile(const char* format, ...) {
  if (logMode == LOG_OFF) return;
  char buffer[384];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  if (logMode == LOG_SERIAL_ONLY || logMode == LOG_BOTH) {
    Serial.print(buffer);
  }
  if ((logMode == LOG_WIFI_ONLY || logMode == LOG_BOTH) && wsReady) {
    _wsEnqueue(String(buffer));
  }
}


// Pomocnicza funkcja - sprawdź aktualny tryb
String getLogModeString() {
  switch (logMode) {
    case LOG_SERIAL_ONLY: return "Serial USB";
    case LOG_WIFI_ONLY:   return "WiFi Terminal";
    case LOG_BOTH:        return "Serial + WiFi";
    case LOG_OFF:         return "Wylaczone";
    default:              return "Nieznany";
  }
}

/*************************************************************
 *  EEPROM - TRYB LOGOWANIA (1 bajt)
 *************************************************************/
// EEPROM_ADDR_LOG_MODE zdefiniowany w sekcji EEPROM na początku pliku (adres 70)

void saveLogModeToEEPROM() {
  uint8_t mode = (uint8_t)logMode;
  (void)enqueueEepromSingle(EEPROM_ADDR_LOG_MODE, &mode, sizeof(mode), "log_mode");
  logPrintln("lvl=INFO tag=EEPROM msg=\"Tryb logowania zapisany\"");
}

void loadLogModeFromEEPROM() {
  uint8_t savedMode = 0;
  EEPROM.get(EEPROM_ADDR_LOG_MODE, savedMode);

  if (savedMode <= (uint8_t)LOG_OFF) {  // walidacja 0..3
    logMode = (LogDestination)savedMode;
    logPrintf("lvl=INFO tag=EEPROM msg=\"Tryb logowania\" tryb=%s\n", getLogModeString().c_str());
  } else {
    logMode = LOG_BOTH;
    logPrintln("lvl=INFO tag=EEPROM msg=\"Tryb logowania: domyslny (BOTH)\"");
  }
}

/*************************************************************
 *  EEPROM - POWER & TRYB (po 1 bajcie)
 *************************************************************/
void savePowerTrybToEEPROM() {
  uint8_t pwr = (uint8_t)power, mode = (uint8_t)tryb;
  EepromWritePart p[2];
  if (eepromBuildPart(p[0], EEPROM_ADDR_POWER, &pwr, sizeof(pwr)) &&
      eepromBuildPart(p[1], EEPROM_ADDR_TRYB, &mode, sizeof(mode))) {
    (void)enqueueEepromBatch(p, 2, "power_tryb");
  }
  if (Komentarze) logPrintln("lvl=INFO tag=EEPROM msg=\"Power & Tryb zapisane\"");
}

// ─────────────────────────────────────────────────────────────────
// bumpChangeCounter() - wywołaj po każdej realnej zmianie nastaw
// (power, tryb, jasnosc, harmonogram). Inkrementuje lokalny licznik
// i zapisuje do EEPROM (lokalny numer wersji nastaw).
// [v33] changeCounter (chmura) usunięty.
// ─────────────────────────────────────────────────────────────────
void bumpChangeCounter() {
  localChangeCounter++;
  (void)enqueueEepromSingle(EEPROM_ADDR_CHANGE_COUNTER, &localChangeCounter, sizeof(localChangeCounter), "change_counter");
  if (Komentarze) logPrintf("lvl=INFO tag=COUNTER msg=\"Licznik nastaw\" wartosc=%u\n", localChangeCounter);
}

void loadChangeCounterFromEEPROM() {
  uint32_t saved = 0;
  EEPROM.get(EEPROM_ADDR_CHANGE_COUNTER, saved);
  if (saved == 0xFFFFFFFF) saved = 0;
  localChangeCounter = saved;
  if (Komentarze) logPrintf("lvl=INFO tag=COUNTER msg=\"Wczytano licznik z EEPROM\" wartosc=%u\n", localChangeCounter);
}

void loadPowerTrybFromEEPROM() {
  uint8_t savedPower = 0;
  uint8_t savedTryb  = 0;

  EEPROM.get(EEPROM_ADDR_POWER, savedPower);
  EEPROM.get(EEPROM_ADDR_TRYB,  savedTryb);

  bool newPower = (savedPower <= 1) ? (bool)savedPower : power;
  bool newTryb  = (savedTryb  <= 1) ? (bool)savedTryb  : tryb;

  // [OK] FIX SPAM: loguj tylko gdy wartość się zmienia, nie przy każdym wywołaniu
  bool changed = (newPower != power || newTryb != tryb);

  if (savedPower <= 1) power = newPower;
  if (savedTryb  <= 1) tryb  = newTryb;

  if (Komentarze && changed) {
    logPrintf("lvl=INFO tag=EEPROM power=%s tryb=%s\n",
              power ? "ON" : "OFF",
              tryb ? "AUTO" : "MANUAL");
  }
}

/*************************************************************
 *  EEPROM - PARAMETRY CZUJNIKÓW ŚWIATŁA
 *************************************************************/
void saveAdaptiveSettingsToEEPROM() {
  uint8_t mode = 0;
  if (uzywajCzujnikaPokojowego && uzywajCzujnikaNadWoda) mode = 2;
  else if (uzywajCzujnikaPokojowego) mode = 1;
  uint8_t enabled = (uint8_t)regulacjaAdaptacyjnaWlaczona;
  uint8_t learning = (uint8_t)uczenieSieWlaczone;
  EepromWritePart p[3];
  if (eepromBuildPart(p[0], EEPROM_ADDR_ADAPTIVE_MODE, &mode, sizeof(mode)) &&
      eepromBuildPart(p[1], EEPROM_ADDR_ADAPTIVE_ENABLED, &enabled, sizeof(enabled)) &&
      eepromBuildPart(p[2], EEPROM_ADDR_LEARNING_ENABLED, &learning, sizeof(learning))) {
    (void)enqueueEepromBatch(p, 3, "adaptive");
  }
  logPrintln("lvl=INFO tag=EEPROM msg=\"Parametry czujnikow zapisane\"");
}

void loadAdaptiveSettingsFromEEPROM() {
  uint8_t mode     = 0;
  uint8_t enabled  = 0;
  uint8_t learning = 0;

  EEPROM.get(EEPROM_ADDR_ADAPTIVE_MODE, mode);
  EEPROM.get(EEPROM_ADDR_ADAPTIVE_ENABLED, enabled);
  EEPROM.get(EEPROM_ADDR_LEARNING_ENABLED, learning);

  // Ochrona przed śmieciami z EEPROM (żeby nie wyjść poza tablicę / logikę)
  if (mode > 2) mode = 0;

  // Tryb czujników
  if (mode == 0) {
    uzywajCzujnikaPokojowego = false;
    uzywajCzujnikaNadWoda = false;
    regulacjaAdaptacyjnaWlaczona = false;
  } else if (mode == 1) {
    uzywajCzujnikaPokojowego = true;
    uzywajCzujnikaNadWoda = false;
    regulacjaAdaptacyjnaWlaczona = (enabled == 1);
  } else { // mode == 2
    uzywajCzujnikaPokojowego = true;
    uzywajCzujnikaNadWoda = true;
    regulacjaAdaptacyjnaWlaczona = (enabled == 1);
  }

  uczenieSieWlaczone = (learning == 1);

  if (Komentarze) {
    const char* modeStr[] = {"OFF", "Pokojowy", "Pokojowy+Woda"};
    logPrintf("lvl=INFO tag=EEPROM msg=\"Czujniki\" tryb=%s aktywne=%s uczenie=%s\n",
              modeStr[mode],
              regulacjaAdaptacyjnaWlaczona ? "TAK" : "NIE",
              uczenieSieWlaczone ? "TAK" : "NIE");
  }
}






// ==========================================================================
// setup() oraz loop()
// ==========================================================================
// ═══════════════════════════════════════════════════════════
//  WŁASNY TERMINAL WiFi - WebSocket /ws
// ═══════════════════════════════════════════════════════════
// Obsługa komend terminalowych (WebSocket)

void handleWsCmd(const String& rawCmd, AsyncWebSocketClient* client) {
  String cmd = rawCmd;
  cmd.trim();
  cmd.toLowerCase();
  auto reply = [&](const String& msg) {
    client->text(msg + "\n");
    Serial.println(msg);
  };
  if (cmd == "serial" || cmd == "usb") {
    logMode = LOG_SERIAL_ONLY; reply("Logi: Serial USB");
  } else if (cmd == "wifi" || cmd == "web") {
    logMode = LOG_WIFI_ONLY;  reply("Logi: WiFi Terminal");
  } else if (cmd == "both" || cmd == "all") {
    logMode = LOG_BOTH;       reply("Logi: Serial + WiFi");
  } else if (cmd == "off" || cmd == "disable") {
    logMode = LOG_OFF;        reply("Logi: Wyłączone");
  } else if (cmd == "save") {
    saveLogModeToEEPROM(); saveAdaptiveSettingsToEEPROM();
    reply("Zapisano do EEPROM");
  } else if (cmd == "logon") {
    littlefsReady = true;  reply("LittleFS WŁ");
  } else if (cmd == "logoff") {
    littlefsReady = false; reply("LittleFS WYŁ");
  } else if (cmd == "logsize") {
    // [v102] sumuj oba pliki
    size_t szA = 0, szB = 0;
    if (LittleFS.exists(LOG_FILE_A)) {
      File fa = LittleFS.open(LOG_FILE_A, "r");
      if (fa) { szA = fa.size(); fa.close(); }
    }
    if (LittleFS.exists(LOG_FILE_B)) {
      File fb = LittleFS.open(LOG_FILE_B, "r");
      if (fb) { szB = fb.size(); fb.close(); }
    }
    if (szA + szB > 0) reply("log_a=" + String(szA) + " B, log_b=" + String(szB) + " B, total=" + String(szA+szB) + " B");
    else reply("Brak pliku logów");
  } else if (cmd == "minlux") {
    reply(String("MIN LUX: ") + (minLuxModeEnabled ? "WŁ" : "WYŁ") + ", " + String(minLuxDayTarget, 0) + " lux");
  } else if (cmd == "minlux on") {
    minLuxModeEnabled = true; saveMinLuxModeToEEPROM(); reply("MIN LUX WŁ");
  } else if (cmd == "minlux off") {
    minLuxModeEnabled = false; minLuxModeActive = false; saveMinLuxModeToEEPROM(); reply("MIN LUX WYŁ");
  } else if (cmd.startsWith("minlux target ")) {
    int val = cmd.substring(14).toInt();
    if (val >= 500 && val <= 8000) { minLuxDayTarget = val; saveMinLuxModeToEEPROM(); reply("Target: " + String(val) + " lux"); }
    else reply("Zakres: 500-8000");
  } else if (cmd == "adaptive") {
    reply("Adaptacja: " + String(regulacjaAdaptacyjnaWlaczona ? "WŁ" : "WYŁ"));
    reply("Uczenie: "   + String(uczenieSieWlaczone ? "WŁ" : "WYŁ"));
    reply("Czujnik pokojowy: " + String(czujnikPokojowyAktywny ? "OK" : "WYŁ"));
    reply("Czujnik woda: "     + String(czujnikNadWodaAktywny  ? "OK" : "WYŁ"));
    reply("Lux: " + String(luxPokojowy,0) + " (pokój), " + String(luxNadWoda,0) + " (woda)");
    reply("Próbek: " + String(adaptacja.liczbaProbek) + ", Transmisja: " + String(adaptacja.nauczonaTransmisja*100,1) + "%");
  } else if (cmd == "status") {
    uint16_t pwm0 = getBrightnessForSection(backupBrightnessComposite, 0);
    reply("\nSTATUS:");
    reply("LED1: " + String(tempPlate1,1) + "°C");
    reply("LED2: " + String(tempPlate2,1) + "°C");
    reply("Woda: " + String(tempWater,1)  + "°C");
    reply("PWM0: " + String(pwm0) + " (" + String(pwm0*100/1023) + "%)");
    reply("Zasilanie: " + String(power ? "WŁ" : "WYŁ"));
    reply("Tryb: "     + String(tryb  ? "AUTO" : "MANUAL"));
    reply("Lux: "      + String(luxPokojowy,0) + " (pokój), " + String(luxNadWoda,0) + " (woda)");
    reply("");
  } else if (cmd == "reset-adapt") {
    adaptacja.sredniaLuxDzien    = 500;
    adaptacja.liczbaProbek       = 0;
    adaptacja.nauczonaTransmisja = 0.30;
    uint8_t zero = 0;
    (void)enqueueEepromSingle(EEPROM_ADRES_ADAPT_POPRAWNE, &zero, sizeof(zero), "adapt_reset");
    // [OK] Resetuj też statystyki korekt i kasuj plik LittleFS
    statystyki.korektyMinimum       = 0;
    statystyki.korektyMaksimum      = 0;
    statystyki.liczbaPomiarow       = 0;
    statystyki.sredniaRedukcja      = 0.0f;
    statystyki.zaoszczedzonaEnergia = 0.0f;
    if (littlefsReady) {
      LittleFS.remove("/adapt_stats.json");
      logPrintln("lvl=INFO tag=RESET-ADAPT msg=\"usunieto adapt_stats.json\"");
    }
    reply("[OK] Dane adaptacji + statystyki zresetowane! Restartuję...");
    // BUG#6 FIX: delay()+restart w AsyncWS callback blokuje AsyncTCP task
    restartRequestedAt = millis();
  } else if (cmd == "restart") {
    reply("🔄 Restartuję...");
    // BUG#6 FIX: delay()+restart w AsyncWS callback blokuje AsyncTCP task
    restartRequestedAt = millis();
  } else if (cmd == "ota" || cmd == "update" || cmd == "ota start") {
    // [4.1.0 OTA-GITHUB] Etap 1 planu upgrade — start OTA z konsoli terminala.
    // otaGithubRequest() jest nieblokujące (task OTA na Core 0), bezpieczne w
    // callbacku WS (żadnego delay() — patrz BUG#6 powyżej).
    String _otaErr;
    if (otaGithubRequest(false, _otaErr)) {
      reply("⬇️ OTA: sprawdzam release na GitHub... status: komenda 'ota status'");
    } else {
      reply("[ERR] OTA: " + _otaErr);
    }
  } else if (cmd == "ota status") {
    reply("OTA: " + otaGithubStatusJson());
  } else if (cmd == "ip") {
    reply("IP: " + WiFi.localIP().toString());
  } else if (cmd == "sunset") {
    reply("Zachód: " + String(sunsetMinutes/60) + ":" + String(sunsetMinutes%60 < 10 ? "0" : "") + String(sunsetMinutes%60));
  } else if (cmd == "time") {
    struct tm ti;
    if (getLocalTimePL(&ti))
      reply("Czas PL: " + String(ti.tm_hour) + ":" + (ti.tm_min<10?"0":"") + String(ti.tm_min) + ":" + (ti.tm_sec<10?"0":"") + String(ti.tm_sec));
    else reply("Brak synchronizacji NTP");
  } else if (cmd == "ping") {
    client->text("pong\n");  // cichy keepalive
  // ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  // [SIM] Komendy symulacji czujnika światła
  // ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  } else if (cmd == "sim lux on") {
    // [SIM] Włącz symulację stałą wartością
    simLuxEnabled = true;
    simLuxAuto    = false;
    simLuxSmoothed = simLuxValue; // [SIM] Reset wygładzenia - unikamy startowego tłumienia
    czujnikPokojowyAktywny = true;
    reply("🎭 [SIM] Symulacja WŁ | stała=" + String(simLuxValue,0) + " lux");
  } else if (cmd == "sim lux off") {
    // [SIM] Wyłącz symulację - hardware przejmuje kontrolę
    simLuxEnabled   = false;
    simLuxAuto      = false;
    simLuxSmoothed  = 0.0f;
    // [SIM] Status czujników zostanie zaktualizowany przez
    // normalny cykl odczytajSwiatloZFiltrem() przy następnym wywołaniu
    reply("🎭 [SIM] Symulacja WYŁ - hardware aktywny");
  } else if (cmd == "sim lux auto") {
    // [SIM] Włącz tryb sinusoidalny: cykl 0-1500 lux co 10 minut
    simLuxEnabled = true;
    simLuxAuto    = true;
    simLuxSmoothed = 0.0f; // [SIM] Reset wygładzenia na start cyklu
    czujnikPokojowyAktywny = true;
    reply("🎭 [SIM] Symulacja AUTO-SIN WŁ | " +
          String(simLuxAutoMin,0) + "-" + String(simLuxAutoMax,0) +
          " lux | okres=" + String(simLuxAutoPeriodMs/1000) + "s");
  } else if (cmd.startsWith("sim lux ")) {
    // [SIM] Ustaw stałą wartość i od razu aktywuj symulację
    float val = cmd.substring(8).toFloat();
    if (val >= 0 && val <= 50000) {
      simLuxValue    = val;
      simLuxEnabled  = true;
      simLuxAuto     = false;
      simLuxSmoothed = val; // [SIM] Reset wygładzenia - natychmiastowy efekt
      czujnikPokojowyAktywny = true;
      reply("🎭 [SIM] Symulacja WŁ | lux=" + String(val,0));
    } else {
      reply("[ERR] [SIM] Zakres: 0-50000 lux");
    }
  } else if (cmd == "sim lux status") {
    // [SIM] Raport stanu symulacji
    if (simLuxEnabled) {
      reply("🎭 [SIM] AKTYWNA | tryb=" + String(simLuxAuto ? "AUTO-SIN" : "STAŁY") +
            " | ustawiona=" + String(simLuxValue,0) +
            " | wygładzona=" + String(simLuxSmoothed,0) +
            " | luxPokojowy=" + String(luxPokojowy,0));
      if (simLuxAuto) {
        reply("        sin: " + String(simLuxAutoMin,0) + "-" + String(simLuxAutoMax,0) +
              " lux, okres=" + String(simLuxAutoPeriodMs/1000) + "s");
      }
    } else {
      reply("🎭 [SIM] WYŁĄCZONA - czujnik hardware | lux=" + String(luxPokojowy,0));
    }
  } else if (cmd.startsWith("sim lux min ")) {
    // [SIM] Ustaw dolny zakres sinusoidy
    float val = cmd.substring(12).toFloat();
    if (val >= 0 && val < simLuxAutoMax) { simLuxAutoMin = val; reply("🎭 [SIM] Min=" + String(val,0)); }
    else reply("[ERR] [SIM] min musi być < max (" + String(simLuxAutoMax,0) + ")");
  } else if (cmd.startsWith("sim lux max ")) {
    // [SIM] Ustaw górny zakres sinusoidy
    float val = cmd.substring(12).toFloat();
    if (val > simLuxAutoMin && val <= 50000) { simLuxAutoMax = val; reply("🎭 [SIM] Max=" + String(val,0)); }
    else reply("[ERR] [SIM] max musi być > min (" + String(simLuxAutoMin,0) + ")");
  } else if (cmd.startsWith("sim lux period ")) {
    // [SIM] Ustaw okres sinusoidy w sekundach (min 10s, max 24h)
    int sek = cmd.substring(15).toInt();
    if (sek >= 10 && sek <= 86400) {
      simLuxAutoPeriodMs = (unsigned long)sek * 1000UL;
      reply("🎭 [SIM] Okres=" + String(sek) + "s");
    } else reply("[ERR] [SIM] Zakres: 10-86400 sekund");
  } else if (cmd == "help") {
    reply("Użyj przycisków w panelu graficznym - http://" + WiFi.localIP().toString() + ":8080/terminal");
    // [SIM] Wylistuj komendy symulacji których nie ma w panelu graficznym
    reply("--- Symulacja czujnika światła ---");
    reply("  sim lux on          - symulacja stała WŁ");
    reply("  sim lux off         - powrót do hardware");
    reply("  sim lux auto        - sinusoida 0-1500 lux / 10min");
    reply("  sim lux [0-50000]   - ustaw stałą wartość lux");
    reply("  sim lux status      - raport stanu");
    reply("  sim lux min [val]   - dolna granica sinusoidy");
    reply("  sim lux max [val]   - górna granica sinusoidy");
    reply("  sim lux period [s]  - okres sinusoidy w sekundach");
  } else {
    reply("Nieznana komenda: " + rawCmd);
  }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("lvl=INFO tag=WS akcja=connect klient=%u ip=%s\n", client->id(), client->remoteIP().toString().c_str());
    client->text("── Połączono z ESP32 Terminal ──\n");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("lvl=INFO tag=WS akcja=disconnect klient=%u\n", client->id());
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      String cmd;
      cmd.reserve(len);
      for (size_t i = 0; i < len; i++) cmd += (char)data[i];
      handleWsCmd(cmd, client);
    }
  }
}


#include "terminal_html.h"
#include "ota_github.h"   // [4.1.0 OTA-GITHUB] Etap 1 planu upgrade — OTA przez GitHub Releases


// [OK] FIX BUG-V: Globalna flaga opóźnionego restartu (dostępna z loop() i handlera HTTP)

// ══════════════════════════════════════════════════════════
//  v34: TELEGRAM TASK - Core 0
//  Wszystkie operacje sieciowe TG (connect/POST/read) działają
//  tu, nigdy nie blokując loop() na Core 1.
//  loop() tylko wrzuca TgDeferCmd do tgCmdQueue (non-blocking).
//  Task ma własny timer 30s dla pollTelegramCommands().
// ══════════════════════════════════════════════════════════
// [v105-FC] bool fbConnect() USUNIĘTY (v66→v104) — zastąpiony przez void fbInitialize()
void fbInitialize(); // [v105-FC] forward declaration — definicja niżej
static void processFirebaseAcks() {
  if (!fbAckQueue) return;
  // ACK jest tylko przygotowaniem do asynchronicznego DELETE. Nie wykonujemy
  // tutaj sieciowego wait/while — jeżeli Firebase jest zajęty, ACK zostaje w kolejce.
  if (g_fbAsyncOp != FbAsyncOp::NONE) return;

  FirebaseAck ack;
  if (xQueueReceive(fbAckQueue, &ack, 0) != pdPASS) return;

  if (!ack.success) {
    fbQueueInFlight = false;
    logPrintf("lvl=WARN tag=FB-ACK msg=\"FAILED\" key=%s\n", ack.key);
    return;
  }

  if (ack.ts > 0) {
    // Zapisujemy high-water mark PRZED DELETE. Dzięki temu po restarcie po udanym
    // wykonaniu komendy rekord pozostający w Firebase zostanie potraktowany jako stale.
    lastFirebaseCmdTs = ack.ts;
    lastFirebaseCmdKey = ack.key;
    fbSaveCmdTs(ack.ts);
    fbSaveCmdKey(ack.key);
  }

  g_fbAsyncActiveKey = String(ack.key);
  g_fbAsyncActiveTs = ack.ts;
  g_fbDelResult = AsyncResult{};
  esp_task_wdt_reset();
  String path = String("/aquarium/commands/") + g_fbAsyncActiveKey;
  fbDatabase.remove(fbAsyncClient, path, g_fbDelResult);
  g_fbAsyncOp = FbAsyncOp::DELETE_COMMAND;
  g_fbAsyncOpStartedMs = millis();
  g_fbAsyncOpTimeoutMs = FB_ASYNC_DEL_TIMEOUT_MS;
  g_fbAsyncResultHandled = false;
  logPrintf("lvl=INFO tag=FB-ACK state=DELETE_START key=%s\n", g_fbAsyncActiveKey.c_str());
}




static void fbAsyncResetOperation() {
  g_fbAsyncOp = FbAsyncOp::NONE;
  g_fbAsyncOpStartedMs = 0;
  g_fbAsyncOpTimeoutMs = 0;
  g_fbAsyncResultHandled = false;
}

static bool fbAsyncEnsureReady() {
  if (WiFi.status() != WL_CONNECTED) return false;
  if (fbAppReady && fbApp.ready()) return true;
  fbInitialize();
  return fbAppReady && fbApp.ready();
}

static void fbAsyncFail(const char* reason) {
  const FbAsyncOp op = g_fbAsyncOp;
  const uint32_t elapsed = g_fbAsyncOpStartedMs ? (millis() - g_fbAsyncOpStartedMs) : 0;
  logPrintf("lvl=ERR tag=FB-ASYNC op=%s reason=%s ms=%lu\n",
            fbAsyncOpName(op), reason ? reason : "error", (unsigned long)elapsed);
  fbAsyncTimeoutCount++;
  fbAppReady = false;
  fbSSLClient.stop();
  NET_FAIL(_fbFailStreak);
  g_fbAsyncRetryAtMs = millis() + 1000UL;
  g_fbAsyncActiveKey = "";
  g_fbAsyncActiveTs = 0;
  fbAsyncResetOperation();
  g_fbSendResult = AsyncResult{};
  g_fbQueueResult = AsyncResult{};
  g_fbDelResult = AsyncResult{};
  g_fbCfgResult = AsyncResult{};
}

static bool fbAsyncResultHasFinal(AsyncResult &r, bool payloadExpected) {
  if (r.isError()) return true;
  // FirebaseClient wysyła również event/debug informacje w AsyncResult. Nie traktuj
  // ich jako finalnego wyniku operacji.
  if (r.isEvent() || r.isDebug()) return false;
  if (!r.isResult()) return false;
  if (payloadExpected && !r.available()) return false;
  return true;
}

static void fbAsyncStartCommandGet() {
  if (g_fbAsyncOp != FbAsyncOp::NONE) return;
  if (fbQueueInFlight) return;
  if (millis() < g_fbAsyncRetryAtMs) return;
  if (!fbAsyncEnsureReady()) return;

  g_fbQueueResult = AsyncResult{};
  DatabaseOptions q;
  q.filter.limitToFirst(1);
  // Odczyt jest prawdziwie asynchroniczny — nie ma tu wait-loop.
  fbDatabase.get(fbAsyncClient, "/aquarium/commands", q, g_fbQueueResult);
  g_fbAsyncOp = FbAsyncOp::GET_COMMANDS;
  g_fbAsyncOpStartedMs = millis();
  g_fbAsyncOpTimeoutMs = FB_ASYNC_CMD_TIMEOUT_MS;
  g_fbAsyncResultHandled = false;
  logPrintfNoFile("lvl=INFO tag=FB-ASYNC op=CMD_GET state=START\n");
}

static void fbAsyncStartConfigGet() {
  if (g_fbAsyncOp != FbAsyncOp::NONE) return;
  if (millis() < g_fbAsyncRetryAtMs) return;
  if (!fbAsyncEnsureReady()) return;

  g_fbCfgResult = AsyncResult{};
  fbDatabase.get(fbAsyncClient, "/aquarium/config", g_fbCfgResult);
  g_fbAsyncOp = FbAsyncOp::GET_CONFIG;
  g_fbAsyncOpStartedMs = millis();
  g_fbAsyncOpTimeoutMs = FB_ASYNC_CFG_TIMEOUT_MS;
  g_fbAsyncResultHandled = false;
  logPrintfNoFile("lvl=INFO tag=FB-ASYNC op=CFG_GET state=START\n");
}

static void fbAsyncStartStatusSet() {
  if (g_fbAsyncOp != FbAsyncOp::NONE) return;
  if (millis() < g_fbAsyncRetryAtMs) return;
  if (!fbAsyncEnsureReady()) return;

  // Status budujemy dopiero tuż przed wysłaniem, ale zapisujemy do globalnego Stringa,
  // żeby lifetime danych trwał przez całą operację async.
  uint16_t pArr[5] = {
    getBrightnessForSection(backupBrightnessComposite, 0),
    getBrightnessForSection(backupBrightnessComposite, 1),
    getBrightnessForSection(backupBrightnessComposite, 2),
    getBrightnessForSection(backupBrightnessComposite, 3),
    getBrightnessForSection(backupBrightnessComposite, 4)
  };
  float chW[5]; float totW = 0;
  for (int i=0;i<5;i++) { chW[i]=getPowerForChannel(i,pArr[i]); totW += chW[i]; }
  int sensorMode = (uzywajCzujnikaPokojowego && uzywajCzujnikaNadWoda) ? 2 : (uzywajCzujnikaPokojowego ? 1 : 0);
  int ledH=(int)(ledOnMinutesToday/60), ledM=(int)(ledOnMinutesToday%60);
  (void)ledH; (void)ledM;

  g_fbAsyncStatusJson.reserve(7000);
  g_fbAsyncStatusJson = "{";
  auto ja = [&](const String& x){ g_fbAsyncStatusJson += x; };
  ja("\"mode\":\"" + String(tryb?"AUTO":"MANUAL") + "\",");
  ja("\"power\":" + String(power?"true":"false") + ",");
  ja("\"pumpOn\":" + String(currentPumpState?"true":"false") + ",");
  ja("\"uptime\":" + String(millis()/1000) + ",");
  ja("\"wifi_rssi\":" + String(WiFi.RSSI()) + ",");
  ja("\"localIp\":\"" + WiFi.localIP().toString() + "\",");
  ja("\"temps\":[" + String(tempPlate1,1) + "," + String(tempPlate2,1) + "," + String(tempWater,1) + "],");
  ja("\"lux\":" + String(luxNadWoda,1) + ",\"luxRoom\":" + String(luxPokojowy,1) + ",\"luxNadWoda\":" + String(luxNadWoda,1) + ",");
  ja("\"pwm\":["+String(pArr[0])+","+String(pArr[1])+","+String(pArr[2])+","+String(pArr[3])+","+String(pArr[4]) + "],");
  ja("\"powerW\":"+String(totW,1)+",\"powerNowW\":"+String(totW,1)+",\"powerNowCh\":["+String(chW[0],1)+","+String(chW[1],1)+","+String(chW[2],1)+","+String(chW[3],1)+","+String(chW[4],1)+"],");
  ja("\"powerLimitW\":"+String(LED_MAX_POWER_W)+",\"sensorMode\":"+String(sensorMode)+",");
  ja("\"adaptEnabled\":"+String(regulacjaAdaptacyjnaWlaczona?"true":"false")+",\"adaptTransmisja\":"+String((int)(adaptacja.nauczonaTransmisja*100))+",");
  ja("\"minLuxEnabled\":"+String(minLuxModeEnabled?"true":"false")+",\"minLuxTarget\":"+String((int)minLuxDayTarget)+",\"minLuxActive\":"+String(minLuxModeActive?"true":"false")+",");
  ja("\"inLightingWindow\":"+String(inLightingWindowGlobal?"true":"false")+",\"isNight\":"+String(isNightGlobal?"true":"false")+",\"rampActive\":"+String(rampScheduleActive?"true":"false")+",");
  ja("\"fadeMinutes\":"+String(fadeMinutes)+",");
  ja("\"schedule\":{\"morningWD\":"+String(MORNING_ON_START_WEEKDAY)+",\"morningWE\":"+String(MORNING_ON_START_WEEKEND)+",\"middayOff\":"+String(MIDDAY_OFF_LOCAL)+",\"eveningBefore\":"+String(EVENING_ON_BEFORE_SUNSET_MIN)+",\"eveningOff\":"+String(EVENING_OFF_START)+",\"sunsetMin\":"+String(sunsetMinutes)+"},");
  ja("\"energyTodayWh\":"+String(energyTodayWh,1)+",\"energyWeekWh\":"+String(energyWeekWh,1)+",\"energyMonthWh\":"+String(energyMonthWh,1)+",");
  ja("\"ledOnMinutesToday\":"+String(ledOnMinutesToday)+",\"ledOnMinutesWeek\":"+String(ledOnMinutesWeek)+",\"ledOnMinutesMonth\":"+String(ledOnMinutesMonth)+",");
  ja("\"luxHoursTodayWater\":"+String(luxHoursTodayWater,0)+",\"minLuxActivToday\":"+String(minLuxActivToday)+",\"peakPowerWToday\":"+String(peakPowerWToday,1)+",\"kwhPrice\":"+String(kwhPrice,2)+",");
  ja("\"dayHistory\":["+String(energyDayHistory[0],1)+","+String(energyDayHistory[1],1)+","+String(energyDayHistory[2],1)+","+String(energyDayHistory[3],1)+","+String(energyDayHistory[4],1)+","+String(energyDayHistory[5],1)+","+String(energyDayHistory[6],1)+"],");
  ja("\"hourPowerWh\":[");
  for(int i=0;i<24;i++){ if(i) ja(","); ja(String(powerHourWh[i],2)); }
  ja("],\"weekHistory\":["+String(energyWeekHistory[0],1)+","+String(energyWeekHistory[1],1)+","+String(energyWeekHistory[2],1)+","+String(energyWeekHistory[3],1)+","+String(energyWeekHistory[4],1)+"],");
  ja("\"minLuxActivWeek\":"+String(minLuxActivWeek)+",");
  ja("\"pumpSlots\":[");
  for(int i=0;i<pumpSlotCount && i<PUMP_MAX_SLOTS;i++){ if(i) ja(","); ja(String("{\"start\":")+String(pumpSlots[i].start)+",\"end\":"+String(pumpSlots[i].end)+"}"); }
  ja("],\"pumpSlotCount\":"+String(pumpSlotCount)+",");
  ja("\"autoPWM\":"+String((uint16_t)getBrightnessForSection(backupAutoBrightnessComposite,0))+",\"luxPerPwm\":"+String(LUX_PER_PWM,2)+",\"glassTransm\":"+String(GLASS_TRANSMITTANCE,2)+",\"rampSec\":"+String(rampSec)+",");
  ja("\"pct\":"+String((int)((getBrightnessForSection(backupBrightnessComposite,0)/1023.0f)*100))+",\"updatedAt\":"+String(millis())+"}");

  g_fbSendResult = AsyncResult{};
  fbDatabase.set<object_t>(fbAsyncClient, "/aquarium/status", object_t(g_fbAsyncStatusJson), g_fbSendResult);
  g_fbAsyncOp = FbAsyncOp::SET_STATUS;
  g_fbAsyncOpStartedMs = millis();
  g_fbAsyncOpTimeoutMs = FB_ASYNC_SEND_TIMEOUT_MS;
  g_fbAsyncResultHandled = false;
  logPrintfNoFile("lvl=INFO tag=FB-ASYNC op=STATUS_SET state=START size=%uB\n", (unsigned)g_fbAsyncStatusJson.length());
}

static void fbAsyncStartDelete(const String& key) {
  if (g_fbAsyncOp != FbAsyncOp::NONE || key.length()==0) return;
  if (!fbAsyncEnsureReady()) return;
  g_fbAsyncActiveKey = key;
  g_fbDelResult = AsyncResult{};
  String path = String("/aquarium/commands/") + key;
  fbDatabase.remove(fbAsyncClient, path, g_fbDelResult);
  g_fbAsyncOp = FbAsyncOp::DELETE_COMMAND;
  g_fbAsyncOpStartedMs = millis();
  g_fbAsyncOpTimeoutMs = FB_ASYNC_DEL_TIMEOUT_MS;
  g_fbAsyncResultHandled = false;
}

static void fbAsyncHandleCommandResult() {
  AsyncResult &r = g_fbQueueResult;
  if (r.isError()) { fbAsyncFail("CMD_ERROR"); return; }
  if (!r.available()) return;
  String body = String(r.c_str());
  fbClientLastUse = millis();
  NET_SUCCESS(_fbFailStreak);
  logPrintfNoFile("lvl=INFO tag=FB-ASYNC op=CMD_GET state=DONE ms=%lu\n", (unsigned long)(millis()-g_fbAsyncOpStartedMs));

  if (body == "null" || body.length() < 5) { g_fbAsyncCompletedCount++; fbAsyncResetOperation(); return; }
  DynamicJsonDocument doc(6144);
  DeserializationError de = deserializeJson(doc, body);
  if (de) { logPrintf("lvl=WARN tag=FB-QUEUE msg=\"JSON parse error\" err=%s\n", de.c_str()); fbAsyncResetOperation(); return; }
  JsonObject root = doc.as<JsonObject>();
  auto it = root.begin();
  if (it == root.end()) { fbAsyncResetOperation(); return; }
  String key = String(it->key().c_str());
  JsonObject item = it->value().as<JsonObject>();
  if (item.isNull()) { fbAsyncStartDelete(key); return; }
  const char* token = item["token"] | "";
  const char* cmd = item["cmd"] | "";
  uint64_t ts = item["ts"] | 0ULL;
  if (ts==0) ts = item["createdAt"] | 0ULL;

  if (strcmp(token, CMD_TOKEN) != 0 || ts==0 || !cmd || !*cmd) {
    logPrintf("lvl=WARN tag=FB-QUEUE msg=\"ODRZUCONO\" key=%s reason=invalid\n", key.c_str());
    fbAsyncStartDelete(key);
    return;
  }
  if (ts < lastFirebaseCmdTs || (ts == lastFirebaseCmdTs && lastFirebaseCmdKey.length() && key <= lastFirebaseCmdKey)) {
    logPrintf("lvl=INFO tag=FB-QUEUE msg=\"STALE\" key=%s\n", key.c_str());
    fbAsyncStartDelete(key);
    return;
  }

  if (strcmp(cmd, "config_refresh") == 0) {
    fbTurboAktywuj(cmd);
    g_fbConfigRefreshTs = ts;
    g_fbConfigRefreshKey = key;
    g_fbConfigRefreshPending = true;
    logPrintfNoFile("lvl=INFO tag=FB-QUEUE cmd=config_refresh key=%s state=START_CFG\n", key.c_str());
    fbAsyncResetOperation();
    return;
  }

  bool ok = wykonajKomendeFirebase(String(cmd), ts, key.c_str());
  if (!ok) {
    // Nic nie kasujemy: kolejna próba pobierze ten sam rekord.
    g_fbAsyncRetryAtMs = millis() + 1000UL;
    logPrintf("lvl=WARN tag=FB-QUEUE msg=\"komenda odrzucona\" cmd=%s key=%s\n", cmd, key.c_str());
    fbAsyncResetOperation();
    return;
  }
  if (!fbQueueInFlight) {
    fbAsyncStartDelete(key);
  } else {
    // Core 1 ma wykonać komendę i dopiero wtedy wrzuci ACK do fbAckQueue.
    logPrintfNoFile("lvl=INFO tag=FB-QUEUE ts=%llu cmd=%s key=%s state=WAIT_APP_ACK\n",
                    (unsigned long long)ts, cmd, key.c_str());
    fbAsyncResetOperation();
  }
}

static void fbAsyncHandleConfigResult() {
  AsyncResult &r = g_fbCfgResult;
  if (r.isError()) { fbAsyncFail("CFG_ERROR"); return; }
  if (!r.available()) return;
  g_fbAsyncConfigBody = String(r.c_str());
  fbClientLastUse = millis();
  NET_SUCCESS(_fbFailStreak);
  logPrintfNoFile("lvl=INFO tag=FB-ASYNC op=CFG_GET state=DONE ms=%lu\n", (unsigned long)(millis()-g_fbAsyncOpStartedMs));
  String body = g_fbAsyncConfigBody;
  // [v257] Config runtime/persistence split: Core 0 parsuje immutable snapshot,
  // Core 1 aplikuje runtime + zgłasza ACK. DELETE/configTs dopiero po tym ACK.
  fbAsyncResetOperation();
  if (body.length() > 4) {
    FirebaseConfigSnapshot snap;
    if (fbParseConfigSnapshot(body, snap)) {
      if (g_fbConfigRefreshPending && g_fbConfigRefreshKey.length() > 0) {
        strncpy(snap.firebaseKey, g_fbConfigRefreshKey.c_str(), sizeof(snap.firebaseKey)-1);
      }
      if (snap.cfgTs > lastFirebaseCfgTs) {
        if (snap.cfgTs != g_fbConfigQueuedTs) {
          if (enqueueFirebaseConfigSnapshot(snap)) {
            g_fbConfigQueuedTs = snap.cfgTs;
            logPrintfNoFile("lvl=INFO tag=FB-CFG state=QUEUED_CORE1 ts=%llu key=%s\n", (unsigned long long)snap.cfgTs, snap.firebaseKey);
            // config_refresh zostało przejęte przez snapshot; teraz czekamy na ACK Core 1.
            if (g_fbConfigRefreshPending) {
              g_fbConfigRefreshPending = false;
              g_fbConfigRefreshKey = "";
              g_fbConfigRefreshTs = 0;
            }
          } else {
            g_fbConfigApplyRetrySnapshot = snap;
            g_fbConfigApplyRetryPending = true;
            g_fbConfigApplyRetryAtMs = millis() + 250UL;
            logPrintf("lvl=WARN tag=FB-CFG state=CORE1_QUEUE_BACKPRESSURE ts=%llu retry=250ms key=%s\n",
                      (unsigned long long)snap.cfgTs, snap.firebaseKey);
          }
        }
      }
    }
  }
}

static void fbAsyncHandleDeleteResult() {
  AsyncResult &r = g_fbDelResult;
  if (r.isError()) { fbAsyncFail("CMD_DELETE_ERROR"); fbQueueInFlight=false; return; }
  if (!r.isResult() || r.isEvent() || r.isDebug()) return;
  fbClientLastUse = millis();
  NET_SUCCESS(_fbFailStreak);
  logPrintfNoFile("lvl=INFO tag=FB-ASYNC op=CMD_DEL state=DONE key=%s ms=%lu\n",
                  g_fbAsyncActiveKey.c_str(), (unsigned long)(millis()-g_fbAsyncOpStartedMs));
  g_fbAsyncCompletedCount++;
  if (g_fbConfigRefreshDeletePendingKey == g_fbAsyncActiveKey) g_fbConfigRefreshDeletePendingKey = "";
  fbQueueInFlight = false;
  g_fbAsyncActiveKey = "";
  g_fbAsyncActiveTs = 0;
  fbAsyncResetOperation();
}

static void fbAsyncHandleStatusResult() {
  AsyncResult &r = g_fbSendResult;
  if (r.isError()) { fbAsyncFail("STATUS_ERROR"); return; }
  if (!r.isResult() || r.isEvent() || r.isDebug()) return;
  fbClientLastUse = millis();
  NET_SUCCESS(_fbFailStreak);
  logPrintfNoFile("lvl=INFO tag=FB-ASYNC op=STATUS_SET state=DONE ms=%lu size=%uB\n",
                  (unsigned long)(millis()-g_fbAsyncOpStartedMs), (unsigned)g_fbAsyncStatusJson.length());
  g_fbAsyncCompletedCount++;
  fbAsyncResetOperation();
}

static void fbAsyncProcessResult() {
  if (g_fbAsyncOp == FbAsyncOp::NONE) return;
  if (g_fbAsyncOpTimeoutMs && millis() - g_fbAsyncOpStartedMs > g_fbAsyncOpTimeoutMs) {
    fbAsyncFail("TIMEOUT");
    return;
  }
  switch (g_fbAsyncOp) {
    case FbAsyncOp::GET_COMMANDS: fbAsyncHandleCommandResult(); break;
    case FbAsyncOp::GET_CONFIG: fbAsyncHandleConfigResult(); break;
    case FbAsyncOp::DELETE_COMMAND: fbAsyncHandleDeleteResult(); break;
    case FbAsyncOp::SET_STATUS: fbAsyncHandleStatusResult(); break;
    default: break;
  }
}

static void processFirebaseConfigAcks() {
  if (!fbConfigAckQueue) return;
  FirebaseConfigAck ack;
  if (xQueueReceive(fbConfigAckQueue, &ack, 0) != pdPASS) return;
  if (!ack.success) return;
  if (ack.cfgTs > lastFirebaseCfgTs) {
    lastFirebaseCfgTs = ack.cfgTs;
    fbSaveCfgTs(ack.cfgTs);
  }
  logPrintfNoFile("lvl=INFO tag=FB-CFG state=ACK_CORE0 ts=%llu key=%s\n", (unsigned long long)ack.cfgTs, ack.firebaseKey);
  if (ack.firebaseKey[0]) {
    g_fbConfigRefreshDeletePendingKey = String(ack.firebaseKey);
  }
}

static bool fbConfigPersistWriteEepromPart(uint16_t addr, const void* data, size_t size) {
  if (size == 0 || size > 16) return false;
  const uint8_t* b = static_cast<const uint8_t*>(data);
  for (size_t i=0; i<size; ++i) EEPROM.write(addr + (uint16_t)i, b[i]);
  return true;
}

static bool persistFirebaseConfigSnapshotCore0(const FirebaseConfigSnapshot& c) {
  bool eepromDirty = false;
  bool ok = true;
  auto ew = [&](uint16_t addr, const void* data, size_t size)->bool {
    if (!fbConfigPersistWriteEepromPart(addr, data, size)) return false;
    eepromDirty = true;
    return true;
  };

  if (c.hasSchedule) {
    ok &= ew(EEPROM_ADDR_MORNING_WEEKDAY, &c.morningWD, sizeof(c.morningWD));
    ok &= ew(EEPROM_ADDR_MORNING_WEEKEND, &c.morningWE, sizeof(c.morningWE));
    ok &= ew(EEPROM_ADDR_MIDDAY_OFF, &c.middayOff, sizeof(c.middayOff));
    ok &= ew(EEPROM_ADDR_EVENING_BEFORE, &c.eveningBefore, sizeof(c.eveningBefore));
    ok &= ew(EEPROM_ADDR_EVENING_OFF, &c.eveningOff, sizeof(c.eveningOff));
    if (c.fieldMask & CFG_FADE) ok &= ew(EEPROM_ADDR_FADE_MIN, &c.fadeMinutesValue, sizeof(c.fadeMinutesValue));
  }
  if (c.hasAdapt) {
    uint8_t mode = c.sensorMode, enabled = c.adaptEnabled ? 1 : 0, learning = c.learningEnabled ? 1 : 0;
    ok &= ew(EEPROM_ADDR_ADAPTIVE_MODE, &mode, sizeof(mode));
    ok &= ew(EEPROM_ADDR_ADAPTIVE_ENABLED, &enabled, sizeof(enabled));
    ok &= ew(EEPROM_ADDR_LEARNING_ENABLED, &learning, sizeof(learning));
  }
  if (c.hasMinLux) {
    uint8_t enabled = c.minLuxEnabledValue ? 1 : 0;
    ok &= ew(EEPROM_ADDR_MINLUX_ENABLED, &enabled, sizeof(enabled));
    ok &= ew(EEPROM_ADDR_MINLUX_TARGET, &c.minLuxTargetValue, sizeof(c.minLuxTargetValue));
    ok &= ew(EEPROM_ADDR_MINLUX_INTERVAL, &c.minLuxIntervalValue, sizeof(c.minLuxIntervalValue));
  }
  if (c.hasParams) {
    if (c.fieldMask & CFG_KWH) ok &= ew(EEPROM_ADDR_KWH_PRICE, &c.kwhPriceValue, sizeof(c.kwhPriceValue));
    if (c.fieldMask & CFG_RAMP) ok &= ew(EEPROM_ADDR_RAMP_SEC, &c.rampSecValue, sizeof(c.rampSecValue));
    if (c.fieldMask & CFG_EMA) ok &= ew(EEPROM_ADDR_EMA_FILTER, &c.emaFilterValue, sizeof(c.emaFilterValue));
    if (c.fieldMask & CFG_SENSINT) ok &= ew(EEPROM_ADDR_SENS_INT, &c.sensIntValue, sizeof(c.sensIntValue));
  }
  if (c.hasPump) {
    uint8_t cnt = (uint8_t)constrain(c.pumpCount, 0, PUMP_MAX_SLOTS);
    ok &= ew(EEPROM_ADDR_PUMP_COUNT, &cnt, sizeof(cnt));
    for (int i=0; i<PUMP_MAX_SLOTS; ++i) {
      int addr = EEPROM_ADDR_PUMP_SLOTS + i * 2 * sizeof(int);
      ok &= ew((uint16_t)addr, &c.pumpSlotsValue[i].start, sizeof(int));
      ok &= ew((uint16_t)(addr + sizeof(int)), &c.pumpSlotsValue[i].end, sizeof(int));
    }
  }

  if (!ok) return false;

  if (eepromDirty) {
    esp_task_wdt_reset();
    bool committed = EEPROM.commit();
    esp_task_wdt_reset();
    if (!committed) {
      logPrintf("lvl=ERR tag=FB-CFG state=PERSIST_EEPROM_COMMIT_FAIL ts=%llu retry=scheduled\n", (unsigned long long)c.cfgTs);
      return false;
    }
  }

  if (c.hasTelegram) {
    if (!littlefsReady) {
      logPrintf("lvl=ERR tag=FB-CFG state=PERSIST_TELEGRAM_FS_NOT_READY ts=%llu\n", (unsigned long long)c.cfgTs);
      return false;
    }
    esp_task_wdt_reset();
    File f = LittleFS.open(TG_CONFIG_FILE, "w");
    if (!f) {
      logPrintf("lvl=ERR tag=FB-CFG state=PERSIST_TELEGRAM_OPEN_FAIL ts=%llu\n", (unsigned long long)c.cfgTs);
      esp_task_wdt_reset();
      return false;
    }
    const bool tgEn = c.telegramEnabledPresent ? c.telegramEnabledValue : tgEnabled;
    const char* tgToken = c.telegramToken[0] ? c.telegramToken : tgBotToken.c_str();
    const char* tgChat = c.telegramChatId[0] ? c.telegramChatId : tgChatId.c_str();
    f.printf("{\"token\":\"%s\",\"chatId\":\"%s\",\"enabled\":\"%s\"}",
             tgToken, tgChat, tgEn ? "1" : "0");
    f.close();
    esp_task_wdt_reset();
  }
  return true;
}

static void flushFirebaseConfigPersistAck() {
  if (!g_fbConfigPersistAckRetryPending || !fbConfigPersistAckQueue) return;
  if (xQueueSend(fbConfigPersistAckQueue, &g_fbConfigPersistAckRetry, 0) == pdPASS) {
    g_fbConfigPersistAckRetryPending = false;
    logPrintfNoFile("lvl=INFO tag=FB-CFG state=PERSIST_ACK_RETRY ts=%llu ok=%d\n",
                    (unsigned long long)g_fbConfigPersistAckRetry.cfgTs,
                    g_fbConfigPersistAckRetry.success ? 1 : 0);
  }
}

static void processFirebaseConfigPersistStorage() {
  if (!fbConfigPersistQueue || !fbConfigPersistAckQueue) return;
  flushFirebaseConfigPersistAck();
  if (g_fbConfigPersistAckRetryPending) return;

  if (g_fbConfigPersistRetryPending) {
    if ((int32_t)(millis() - g_fbConfigPersistRetryAtMs) < 0) return;
    if (persistFirebaseConfigSnapshotCore0(g_fbConfigPersistRetrySnapshot)) {
      g_fbConfigPersistRetryPending = false;
      g_fbConfigPersistAckRetry.success = true;
      g_fbConfigPersistAckRetry.cfgTs = g_fbConfigPersistRetrySnapshot.cfgTs;
      flushFirebaseConfigPersistAck();
      if (g_fbConfigPersistAckRetryPending) return;
      logPrintfNoFile("lvl=INFO tag=FB-CFG state=PERSIST_RETRY_OK ts=%llu\n", (unsigned long long)g_fbConfigPersistRetrySnapshot.cfgTs);
    } else {
      g_fbConfigPersistRetryAtMs = millis() + 1000UL;
    }
    return;
  }

  FirebaseConfigSnapshot c;
  if (xQueueReceive(fbConfigPersistQueue, &c, 0) != pdPASS) return;
  if (persistFirebaseConfigSnapshotCore0(c)) {
    g_fbConfigPersistAckRetry.success = true;
    g_fbConfigPersistAckRetry.cfgTs = c.cfgTs;
    flushFirebaseConfigPersistAck();
    if (g_fbConfigPersistAckRetryPending) {
      logPrintf("lvl=ERR tag=FB-CFG state=PERSIST_ACK_QUEUE_FULL ts=%llu retry=scheduled\n", (unsigned long long)c.cfgTs);
    } else {
      logPrintfNoFile("lvl=INFO tag=FB-CFG state=PERSIST_OK ts=%llu\n", (unsigned long long)c.cfgTs);
    }
  } else {
    g_fbConfigPersistRetrySnapshot = c;
    g_fbConfigPersistRetryPending = true;
    g_fbConfigPersistRetryAtMs = millis() + 1000UL;
    logPrintf("lvl=ERR tag=FB-CFG state=PERSIST_FAIL ts=%llu retry=scheduled\n", (unsigned long long)c.cfgTs);
  }
}

static void processStorageQueue() {
  if (!storageQueue) return;

  // Retry commit bez ponownego kolejkowania danych — EEPROM emulation zachowuje
  // bufor RAM po nieudanym commit(). Retry jest wykonywany WYŁĄCZNIE na Core 0.
  if (g_eepromCommitRetryPending) {
    esp_task_wdt_reset();
    bool ok = EEPROM.commit();
    esp_task_wdt_reset();
    if (ok) {
      g_eepromCommitRetryPending = false;
      logPrintfNoFile("lvl=INFO tag=EEPROM-STORAGE state=COMMIT_RETRY_OK\n");
    } else {
      logPrintf("lvl=ERR tag=EEPROM-STORAGE state=COMMIT_RETRY_FAIL\n");
      return;
    }
  }

  bool eepromDirty = false;
  StorageRequest req;
  uint8_t processed = 0;
  while (processed < 4 && xQueueReceive(storageQueue, &req, 0) == pdPASS) {
    ++processed;
    if (req.type == StorageRequestType::EEPROM_BATCH) {
      for (uint8_t i=0; i<req.partCount && i<EEPROM_MAX_PARTS; ++i) {
        const EepromWritePart& p = req.parts[i];
        if (p.size == 0 || p.size > EEPROM_PART_BYTES) continue;
        for (uint8_t j=0;j<p.size;++j) EEPROM.write(p.addr+j, p.data[j]);
      }
      eepromDirty = true;
      logPrintfNoFile("lvl=INFO tag=EEPROM-STORAGE state=WRITE_RAM tag=%s count=%u\n", req.tag, (unsigned)req.partCount);
    } else if (req.type == StorageRequestType::TELEGRAM_CONFIG) {
      if (!littlefsReady) {
        logPrintf("lvl=ERR tag=STORAGE state=TELEGRAM_FS_NOT_READY\n");
        continue;
      }
      esp_task_wdt_reset();
      File f = LittleFS.open(TG_CONFIG_FILE, "w");
      if (!f) {
        logPrintf("lvl=ERR tag=STORAGE state=TELEGRAM_OPEN_FAIL\n");
      } else {
        f.printf("{\"token\":\"%s\",\"chatId\":\"%s\",\"enabled\":\"%s\"}",
                 req.tgToken, req.tgChatId, req.tgEnabled ? "1" : "0");
        f.close();
        logPrintfNoFile("lvl=INFO tag=STORAGE state=TELEGRAM_WRITE_OK\n");
      }
      esp_task_wdt_reset();
    }
  }

  if (eepromDirty) {
    esp_task_wdt_reset();
    bool ok = EEPROM.commit();
    esp_task_wdt_reset();
    if (!ok) {
      g_eepromCommitRetryPending = true;
      logPrintf("lvl=ERR tag=EEPROM-STORAGE state=COMMIT_FAIL retry=scheduled\n");
    } else {
      logPrintfNoFile("lvl=INFO tag=EEPROM-STORAGE state=COMMIT_OK\n");
    }
  }
}

static void processFirebaseConfigApplyQueueRetry() {
  if (!g_fbConfigApplyRetryPending) return;
  if ((int32_t)(millis() - g_fbConfigApplyRetryAtMs) < 0) return;
  if (!enqueueFirebaseConfigSnapshot(g_fbConfigApplyRetrySnapshot)) {
    g_fbConfigApplyRetryAtMs = millis() + 250UL;
    return;
  }
  g_fbConfigApplyRetryPending = false;
  g_fbConfigQueuedTs = g_fbConfigApplyRetrySnapshot.cfgTs;
  logPrintfNoFile("lvl=INFO tag=FB-CFG state=QUEUED_CORE1_RETRY ts=%llu key=%s\n",
                  (unsigned long long)g_fbConfigApplyRetrySnapshot.cfgTs,
                  g_fbConfigApplyRetrySnapshot.firebaseKey);
  if (g_fbConfigRefreshPending &&
      g_fbConfigRefreshKey.length() > 0 &&
      strncmp(g_fbConfigRefreshKey.c_str(), g_fbConfigApplyRetrySnapshot.firebaseKey, FB_CFG_KEY_LEN) == 0) {
    g_fbConfigRefreshPending = false;
    g_fbConfigRefreshKey = "";
    g_fbConfigRefreshTs = 0;
  }
}

static void fbAsyncSchedulerStep() {
  // Jeden request in-flight. Kolejność: ACK/delete -> config_refresh ->
  // command -> config -> status. Po 5 command GET wymuszamy nie-command,
  // aby status/config nie mogły zostać zagłodzone podczas Turbo.
  static uint8_t cmdOpsSinceNonCmd = 0;

  processFirebaseAcks();
  if (g_fbAsyncOp != FbAsyncOp::NONE) return;
  processFirebaseConfigAcks();
  if (g_fbAsyncOp != FbAsyncOp::NONE) return;
  if (g_fbConfigRefreshDeletePendingKey.length() > 0) {
    fbAsyncStartDelete(g_fbConfigRefreshDeletePendingKey);
    return;
  }

  if (g_fbConfigApplyRetryPending) {
    processFirebaseConfigApplyQueueRetry();
    if (g_fbConfigApplyRetryPending) return;
  }

  if (g_fbConfigRefreshPending && !fbQueueInFlight) {
    fbAsyncStartConfigGet();
    if (g_fbAsyncOp == FbAsyncOp::GET_CONFIG) {
      cmdOpsSinceNonCmd = 0;
    }
    return;
  }
  if (fbQueueInFlight) return;

  const bool allowNonCmd = (cmdOpsSinceNonCmd >= 5);
  if (fbCfgPending && allowNonCmd) {
    fbCfgPending = false;
    cmdOpsSinceNonCmd = 0;
    fbAsyncStartConfigGet();
    return;
  }
  if (fbSendPending && allowNonCmd) {
    fbSendPending = false;
    cmdOpsSinceNonCmd = 0;
    fbAsyncStartStatusSet();
    return;
  }
  if (fbCmdPending) {
    fbCmdPending = false;
    fbAsyncStartCommandGet();
    if (g_fbAsyncOp == FbAsyncOp::GET_COMMANDS && cmdOpsSinceNonCmd < 250) ++cmdOpsSinceNonCmd;
    return;
  }
  if (fbCfgPending) {
    fbCfgPending = false;
    cmdOpsSinceNonCmd = 0;
    fbAsyncStartConfigGet();
    return;
  }
  if (fbSendPending) {
    fbSendPending = false;
    cmdOpsSinceNonCmd = 0;
    fbAsyncStartStatusSet();
    return;
  }
}

void tgTaskFn(void* /*pvParams*/) {
  // Zarejestruj ten task w Task Watchdog Timer.
  // Bez tego każde esp_task_wdt_reset() wewnątrz tasku zwraca "task not found".
  esp_task_wdt_add(NULL);

  const TickType_t xWait = pdMS_TO_TICKS(200);  // sprawdzaj kolejkę co 200ms
  unsigned long lastPollInTask   = 0;
  unsigned long menuReturnInTask = 0;
  // ── DBG-TASK: diagnostyka żywotności tasku ──
  unsigned long _dbgLastAlive    = 0;   // log "alive" co 15s
  unsigned long _dbgLastStack    = 0;   // log stack watermark co 60s
  uint32_t      _dbgIterations   = 0;   // licznik iteracji pętli
  logPrintf("lvl=INFO tag=TG-TASK msg=\"start\" stack=%uB freeH=%luB core=%d\n",
            12288, (unsigned long)ESP.getFreeHeap(), xPortGetCoreID());

  for (;;) {
    TgDeferCmd cmd = TG_NONE;
    _dbgIterations++;
    unsigned long _cmdT = 0;  // [v131] hoisted: goto nie może przeskakiwać inicjalizacji (C++ §6.7)

    // Reset watchdoga na początku każdej iteracji pętli (co ~200ms).
    // Bez tego WDT odpala gdy task jest bezczynny (brak tokena TG, oczekiwanie na kolejkę).
    esp_task_wdt_reset();

    processFirebaseAcks();

    // [FIX-v131-ITER-BUDGET] Timestamp budżetu tej iteracji.
    // Używany do sprawdzenia "czy zostało czasu na kolejną operację sieciową".
    // Budżet: 10000ms (margines 5s do WDT=15s).
    const uint32_t _iterStartMs   = millis();
    const uint32_t ITER_BUDGET_MS = 10000UL;
    // Makro: sprawdź czy mamy jeszcze budżet; jeśli nie → skip operacji
    #define ITER_HAS_BUDGET() ((millis() - _iterStartMs) < ITER_BUDGET_MS)

    // [NETDIAG] Test routera/łącza - wykonywany TYLKO tutaj, na Core 0.
    // Celowo WCZEŚNIE w iteracji i POZA sekcją _netCircuitOpen (circuit
    // breaker FB/TG niżej) - to niezależny mechanizm, nie mieszam jego
    // fail-streaków ze statystykami testu routera. Każdy blok zagatowany
    // przez ITER_HAS_BUDGET(), tak jak fbSendPending/fbCmdPending niżej.
    // Koszt: maks. NETDIAG_TCP_TIMEOUT_MS (800ms) raz na 60s (faza gęsta)
    // lub raz na 5 min (faza lekka) - działa naprzemiennie z odbiorem
    // komend Telegram (xQueueReceive niżej), więc w najgorszym razie
    // opóźnia jeden odczyt kolejki o te 800ms, nie więcej.
    if (netDiagPending && ITER_HAS_BUDGET()) {
      netDiagPending = false;
      netDiagRunPendingTest();
      esp_task_wdt_reset();
    }
    if (netDiagSummaryPending && ITER_HAS_BUDGET()) {
      netDiagSummaryPending = false;
      netDiagWritePendingSummary();
    }

    processFirebaseConfigPersistStorage();
    processStorageQueue();

    // [v101] FIX-5: PRE_RESET_UPDATE co 2s — po crashu WDT boot pokaże ostatni checkpoint
    // tgTask (np. "FB-CONN-start", "TSL-REINIT") zamiast poprzedniej sesji. src=2 = tgTask.
    // [FIX-v154-PRERESET-FLIPFLOP] pominięte, gdy loop() już zainicjował jawny restart
    // (g_restartPending) — inaczej ten heartbeat potrafi nadpisać resetSrc=0/1 z powrotem
    // na 2 w oknie między zapisem a ESP.restart().
    {
      static unsigned long _lastPRU = 0;
      if (millis() - _lastPRU >= 2000UL && !g_restartPending) {
        _lastPRU = millis();
        PRE_RESET_UPDATE(2);
      }
    }

    // ── DBG-TASK: log "alive" co 15s - potwierdza że pętla się kręci ──
    {
      unsigned long _now = millis();
      if (_now - _dbgLastAlive >= 300000UL) {  // [v97-LOG] co 60s→ [v130-LOG] co 5min (60s było za głośne)
        _dbgLastAlive = _now;
        UBaseType_t stackFree = uxTaskGetStackHighWaterMark(nullptr);  // bajty wolne na stosie
        logPrintf("lvl=INFO tag=TG-ALIVE iter=%lu stack_free=%uB freeH=%luB psram=%ukB maxAlloc=%luB wifi=%d tgReady=%d fbReady=%d\n",
                  (unsigned long)_dbgIterations,
                  (unsigned)stackFree * sizeof(StackType_t),
                  (unsigned long)ESP.getFreeHeap(),
                  (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024),
                  (unsigned long)ESP.getMaxAllocHeap(),
                  (int)WiFi.status(),
                  (int)tgClientReady,
                  (int)fbAppReady);  // [v105-FC] fbAppReady (bylo fbClientReady)
      }
      // [v81-DIAG] Wykrywanie zmian statusu WiFi
      { static int _lastWifi = -1;
        int _wNow = (int)WiFi.status();
        if (_wNow != _lastWifi) {
          logPrintf("lvl=INFO tag=WIFI-STATUS od=%d do=%d ip=%s freeH=%luB\n",
                    _lastWifi, _wNow, WiFi.localIP().toString().c_str(),
                    (unsigned long)ESP.getFreeHeap());
          _lastWifi = _wNow;
        }
      }
    }

    // Odbierz komendę z kolejki (nieblokująco z timeout 200ms)
    if (tgCmdQueue) {
      xQueueReceive(tgCmdQueue, &cmd, xWait);
    } else {
      vTaskDelay(xWait);
    }

    // ── [OK] FIX-v33h: Reinicjalizacja TSL I²C na Core 0 - nie blokuje loop() ──
    if (tslReinitPending) {
      tslReinitPending = false;
      // [v71] SENSOR-OFF: gdy tryb OFF - skasuj flagę bez dotykania I2C.
      // Flaga mogła zostać ustawiona przed zmianą trybu - ignorujemy ją.
      if (!uzywajCzujnikaPokojowego && !uzywajCzujnikaNadWoda) {
        esp_task_wdt_reset();
        continue;
      }
      // [v101] FIX-2: esp_task_wdt_reset() na wejście bloku — brakujący od początku.
      // Poprzednia operacja tgTask mogła zająć kilka sekund bez WDT feed.
      // Wire.end()+delay(30)+bit-bang+Wire.begin()+delay(80)+begin()×2 = do ~16s > WDT.
      // Potwierdzono testem B1: 9113ms blokady I2C crashuje bez resetu, przeżywa z nim.
      esp_task_wdt_reset();      // [v101] FIX-2: reset WDT na wejście tslReinitPending
      PRE_RESET_CP("TSL-REINIT");  // [v71]
      // Szybki reset magistrali I2C
      Wire.end();
      esp_task_wdt_reset();      // [v101] FIX-2: reset przed delay(30)
      delay(30);
      pinMode(20, OUTPUT);  // S3: GPIO 22 nie istnieje -> 20
      for (int i = 0; i < 9; i++) {
        digitalWrite(20, HIGH); delayMicroseconds(10);
        digitalWrite(20, LOW);  delayMicroseconds(10);
      }
      pinMode(20, INPUT);
      delay(30);
      Wire.begin(21, 20);  // SDA=21, SCL=20 (S3: GPIO 22 nie istnieje)
      Wire.setTimeOut(50);
      esp_task_wdt_reset();  // [v101] FIX-3: reset przed delay(80) (Wire.begin() może chwilę blokować)
      delay(80);

      bool anyFailed = false;
      if (!czujnikPokojowyAktywny) {
        esp_task_wdt_reset();  // S3: begin() może blokować ~8s gdy czujnik niepodłączony
  if (czujnikPokojowy.begin()) {
          czujnikPokojowy.enableAutoRange(true);
          czujnikPokojowy.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);
          czujnikPokojowyAktywny = true;
          tslFailCount = 0;
          logPrintln("lvl=INFO tag=TSL czujnik=pokojowy msg=\"Reinicjalizacja OK\"");
        } else {
          sensors_event_t _testEvt;
          czujnikPokojowy.getEvent(&_testEvt);
          if (_testEvt.light > 0.0f && _testEvt.light < 65535.0f) {
            czujnikPokojowyAktywny = true;
            tslFailCount = 0;
            logPrintf("lvl=INFO tag=TSL czujnik=pokojowy msg=\"begin()=false ale getEvent() aktywny (ghost-fix)\" lux=%.0f\n", _testEvt.light);
          } else {
            logPrintf("lvl=ERR tag=TSL czujnik=pokojowy msg=\"begin()=false, sensor NIEAKTYWNY\" lux=%.0f\n", _testEvt.light);
            anyFailed = true;
          }
        }
      }
      if (!czujnikNadWodaAktywny) {
        // ************************************************FIX-v57-RESET-LOOP (ROOT CAUSE B)
        // Brakowało esp_task_wdt_reset() przed begin() drugiego czujnika.
        // czujnikPokojowy.begin() może zablokować ~8s (przy braku odpowiedzi I2C).
        // Bez resetu WDT tutaj: dwa begin() × ~8s = ~16s > WDT 10s → crash.
        esp_task_wdt_reset();  // FIX-v57: reset WDT przed begin() czujnika nad wodą
        if (czujnikNadWoda.begin()) {
          czujnikNadWoda.enableAutoRange(true);
          czujnikNadWoda.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);
          czujnikNadWodaAktywny = true;
          tslFailCount = 0;
          logPrintln("lvl=INFO tag=TSL czujnik=woda msg=\"Reinicjalizacja OK\"");
        } else {
          sensors_event_t _testEvtW;
          czujnikNadWoda.getEvent(&_testEvtW);
          if (_testEvtW.light > 0.0f && _testEvtW.light < 65535.0f) {
            czujnikNadWodaAktywny = true;
            tslFailCount = 0;
            logPrintf("lvl=INFO tag=TSL czujnik=woda msg=\"begin()=false ale getEvent() aktywny (ghost-fix)\" lux=%.0f\n", _testEvtW.light);
          } else {
            logPrintf("lvl=ERR tag=TSL czujnik=woda msg=\"begin()=false, sensor NIEAKTYWNY\" lux=%.0f\n", _testEvtW.light);
            anyFailed = true;
          }
        }
      }
      if (anyFailed) {
        tslFailCount++;
        if (tslFailCount == 5) {
          logPrintln("lvl=ERR tag=TSL msg=\"Czujniki niedostepne po 5 probach, retry co 10 min. Sprawdz okablowanie I2C\"");
        }
      }
      // ************************************************FIX-v70-A (WDT crash loop)
      // czujnikNadWoda.begin() mogło blokować ~8s. Bez resetu WDT tutaj i bez continue,
      // ta sama iteracja mogła wpaść w bootNotify→sendTelegramMessage() (~8s):
      // 8s + 8s = 16s > WDT 10s → WDT_TASK crash. Identyczny wzorzec jak FIX-v57 ROOT CAUSE A.
      esp_task_wdt_reset();  // FIX-v70-A: reset po potencjalnie ~8s blokadzie begin()
      continue;              // FIX-v70-A: nie nakładaj TSL reinit + bootNotify w jednej iteracji
    }

    // ── [OK] FIX-v34-TSL-ASYNC: Odczyt TSL I²C na Core 0 - nie blokuje loop() ──
    if (tslReadPending) {
      tslReadPending = false;

      if (czujnikPokojowyAktywny) {
        sensors_event_t _ev;
        czujnikPokojowy.getEvent(&_ev);
        uint16_t b = 0, ir = 0;
        czujnikPokojowy.getLuminosity(&b, &ir);
        portENTER_CRITICAL(&tslCacheMux);  // [OK] OPT-v44: atomowy zapis zestawu pól
        tslCachePok   = _ev.light;
        tslCacheBrPok = b;
        tslCacheIrPok = ir;
        portEXIT_CRITICAL(&tslCacheMux);
      }

      if (czujnikNadWodaAktywny && uzywajCzujnikaNadWoda) {
        sensors_event_t _evW;
        czujnikNadWoda.getEvent(&_evW);
        uint16_t bW = 0, irW = 0;
        czujnikNadWoda.getLuminosity(&bW, &irW);
        portENTER_CRITICAL(&tslCacheMux);  // [OK] OPT-v44: atomowy zapis zestawu pól
        tslCacheWoda   = _evW.light;
        tslCacheBrWoda = bW;
        tslCacheIrWoda = irW;
        portEXIT_CRITICAL(&tslCacheMux);
      }
    }

    // ── [v46] LOG-FLUSH-CORE0: flush bufora logu Core 1 na Core 0 ──
    // Core 1 (loop) ustawia logFlushPending[1] zamiast pisać do LittleFS.
    // Tu (Core 0, tgTask) wykonujemy faktyczny zapis - brak blokowania loop().
    if (logFlushPending[1]) {
      logFlushPending[1] = false;
      if (logMutex && xSemaphoreTake(logMutex, portMAX_DELAY) == pdTRUE) {
        logFlushCore1();
        xSemaphoreGive(logMutex);
      }
    }

    // ── [v102] FIX-LOG-CLEAR: usuwanie log_a+log_b przez tgTask (Core 0) ──
    // DUAL-FILE: log_a i log_b są małe (≤256KB) — zamknięcie FD przez HTTP szybsze.
    // Brak remove() na pliku serwowanym przez AsyncFileResponse (problem z jednym log.txt).
    if (logClearPending) {
      // [v225] DIAG-FSGUARD-STALE-PENDING: jesli w tym momencie guard byl
      // jeszcze "w locie" (ustawiony, ale nie obsluzony przez blok nizej),
      // to po tym czyszczeniu ten blok zaloguje "Pending guard nieaktualny" -
      // ta linia pozwala to jednoznacznie skorelowac w logu.
      if (fsGuardPending) {
        logPrintf("lvl=WARN tag=FS-GUARD msg=\"Reczne czyszczenie logow (WWW) z pending guardem w locie\"\n");
      }
      logClearPending   = false;
      logClearDone      = false;
      logClearFailed    = false;
      logClearInProgress = true;

      vTaskDelay(pdMS_TO_TICKS(150));  // czas na zamknięcie in-flight FD

      if (logMutex && xSemaphoreTake(logMutex, portMAX_DELAY) == pdTRUE) {
        if (g_logBuf[0]) { g_logBuf[0][0] = '\0'; g_logBufLen[0] = 0; }
        if (g_logBuf[1]) { g_logBuf[1][0] = '\0'; g_logBufLen[1] = 0; }
        logFlushPending[1] = false;

        // [v102] DUAL-FILE: usuń oba pliki — żadnego retry potrzebnego
        bool okB = !littlefsReady || !LittleFS.exists(LOG_FILE_B) || LittleFS.remove(LOG_FILE_B);
        bool okA = !littlefsReady || !LittleFS.exists(LOG_FILE_A) || LittleFS.remove(LOG_FILE_A);
        bool ok = okA && okB;

        if (ok) {
          g_logFileSize  = 0;  // [v102]
          g_logFileSizeA = 0;  // [v102]
          g_lastFlush[0] = g_lastFlush[1] = millis();
          logClearDone = true;
          Serial.printf("lvl=INFO tag=LOG-CLEAR akcja=ok msg=\"log_a+log_b usuniete z panelu WWW\"\n");
        } else {
          logClearFailed = true;
          Serial.printf("lvl=ERR tag=LOG-CLEAR akcja=fail msg=\"blad usuwania log_a/log_b\"\n");
        }
        xSemaphoreGive(logMutex);
      }
      logClearInProgress = false;
    }

    // ── [OK] FIX-v33g: Zapis historii na Core 0 - nie blokuje loop() ──
    // [OK] FIX-v61: dodano saveHistoryPoint() + reset flagi histSavePending.
    // [v101] FIX-4: esp_task_wdt_reset() przed każdą operacją FS.
    //   ROOT CAUSE C (potwierdzone testem C1): poprzednia operacja tgTask (np. FB-CONN 4s)
    //   + saveHistoryPoint(3s) + saveAdaptStats(2s) + saveEnergyStats(2s) = 11s > WDT 10s.
    //   LittleFS przy pełnym flash lub po długim logu może blokować dłużej niż typowe.
    if (histSavePending) {
      histSavePending = false;   // [v61] FIX: resetuj flagę (było: brak resetu)
      esp_task_wdt_reset();      // [v101] FIX-4: reset przed saveHistoryPoint (długi zapis CSV)
      saveHistoryPoint();         // [v61] FIX: akumulacja ledOnMinutes / energy
      esp_task_wdt_reset();      // [v101] FIX-4: reset między operacjami FS
      saveAdaptStats();
      esp_task_wdt_reset();      // [v101] FIX-4: reset między operacjami FS
      saveEnergyStats();
      esp_task_wdt_reset();      // [v101] FIX-4: reset po całości bloku
    }

    // ── [FIX race-audit 2026-08-13] Single-writer dla akumulatorów energii ──
    // logDailySummary() (Core 1/loop()) tylko wykrywa zmianę dnia i ustawia flagę;
    // cała mutacja energyTodayWh i całej rodziny dzieje się TUTAJ, na tym samym
    // rdzeniu co saveHistoryPoint() wyżej ("+=" i "=0" nigdy się nie przeplatają) -
    // patrz Ryby_LED_S3_analiza_WDT_race_conditions.md sekcja 5/10 i komentarz
    // przy energyRolloverPending.
    if (energyRolloverPending) {
      energyRolloverPending = false;
      esp_task_wdt_reset();
      applyEnergyRollover();
      esp_task_wdt_reset();
    }

    // ── [v87] OPT-B: Kompaktowanie history.csv — usuwa martwe linie z początku ──
    // Wywoływane raz na ~24h (gdy histStartLine >= HISTORY_MAX_POINTS).
    // Poza tym zapis to tylko append ~50ms — bezpieczne dla WDT.
    if (compactHistPending) {
      compactHistPending = false;
      PRE_RESET_CP("tgTask-compactHist");
      unsigned long _ct = millis();
      logPrintf("lvl=INFO tag=HIST-COMPACT faza=START startline=%d linecount=%d fH=%uB\n",
                histStartLine, histLineCount, (unsigned)ESP.getFreeHeap());
      bool compOk = false;
      File fOld = LittleFS.open(HISTORY_FILE, "r");
      File fTmp = LittleFS.open("/history_tmp.csv", "w");
      if (fOld && fTmp) {
        // Przepisz nagłówek
        String hdr = fOld.readStringUntil('\n');
        fTmp.print(hdr); fTmp.print('\n');
        // Pomiń martwe linie (histStartLine sztuk) — [v88] PATCH-1: O(1) seek zamiast O(N×lineLen)
        if (histStartByteOffset > 0) {
          fOld.seek(histStartByteOffset, SeekSet);  // O(1) zamiast pętli readStringUntil
        } else {
          // fallback: przy pierwszym uruchomieniu lub gdy offset nie był jeszcze śledzony
          // [v135] FIX-v135-HIST-COMPACT-WDT: esp_task_wdt_reset() co 64 linie.
          // ROOT CAUSE: histStartLine potrafi być >1700 (rośnie z lineCount od poprzedniej
          // sesji) — pętla bez ANI JEDNEGO resetu WDT crashowała tgTask tuż po restarcie
          // (potwierdzone logiem: [HIST-COMPACT] START startLine=1738 → task_wdt Abort).
          for (int _i = 0; _i < histStartLine; _i++) {
            fOld.readStringUntil('\n');
            if ((_i & 0x3F) == 0) esp_task_wdt_reset();  // co 64 linie
          }
          histStartByteOffset = fOld.position();
        }
        // Skopiuj żywe linie — [v88] PATCH-3: bufor PSRAM 16 KB (było: stackBuf 512 B)
#ifdef BOARD_HAS_PSRAM
        uint8_t* _copyBuf = (uint8_t*)ps_malloc(16384);
        size_t   _copyBufSz = (_copyBuf != nullptr) ? 16384 : 512;
        uint8_t  _stackBuf[512];
        if (!_copyBuf) _copyBuf = _stackBuf;
#else
        uint8_t  _stackBuf[512];
        uint8_t* _copyBuf   = _stackBuf;
        size_t   _copyBufSz = 512;
#endif
        int _r;
        while (fOld.available()) {
          _r = fOld.read(_copyBuf, _copyBufSz);
          if (_r > 0) fTmp.write(_copyBuf, _r);
          esp_task_wdt_reset();
        }
#ifdef BOARD_HAS_PSRAM
        if (_copyBuf != _stackBuf) free(_copyBuf);
#endif
        fOld.close(); fTmp.close();
        // Atomowa podmiana
        // [v88] BM-16 DISCLAIMER: rename() może triggerować metadata compaction LittleFS (~300-400ms).
        // To normalne zachowanie — chroni przed korupcją przy power-loss. Nie optymalizować.
        LittleFS.rename(HISTORY_FILE, "/history_old.csv");
        if (LittleFS.rename("/history_tmp.csv", HISTORY_FILE)) {
          LittleFS.remove("/history_old.csv");
          histLineCount  -= histStartLine;
          histStartLine   = 0;
          histStartByteOffset = 0;  // [v88] PATCH-1: po compaction plik jest czysty, offset = 0
          compOk = true;
          logPrintf("lvl=INFO tag=HIST-COMPACT faza=DONE czas=%lums linecount=%d fH=%uB\n",
                    millis()-_ct, histLineCount, (unsigned)ESP.getFreeHeap());
        } else {
          LittleFS.rename("/history_old.csv", HISTORY_FILE);
          LittleFS.remove("/history_tmp.csv");
          logPrintf("lvl=ERR tag=HIST-COMPACT msg=\"Blad rename, przywrocono oryginal\"\n");
        }
      } else {
        if (fOld) fOld.close();
        if (fTmp) fTmp.close();
        logPrintf("lvl=ERR tag=HIST-COMPACT msg=\"Blad otwarcia pliku\"\n");
      }
      (void)compOk;
      PRE_RESET_CP("tgTask-compactHist-done");
    }

    // ── [v109] WIFI-GUARD: pomiń operacje sieciowe gdy WiFi L2 niedostępny ──
    // Cel: przy WiFi.status() != WL_CONNECTED każda fcja FB ma własny early-return,
    // ale fbApp.loop() może inicjować reconnect → TCP connect() wisi 5s timeout × N operacji.
    // Zamiast ~5-15s stall: 1s delay, flagi fb*Pending zachowane (retry gdy WiFi wróci).
    // Bloki FS (fsGuardPending, histSavePending itp.) po tym bloku działają normalnie.
    const bool _wifiUpNow = (WiFi.status() == WL_CONNECTED);

    // ── [v42] FIX-ARCH: Firebase HTTP na Core 0 - nie blokuje loop(), brak wyścigów HEAP ──
    // Trzy funkcje Firebase wywoływane sekwencyjnie po historii - jeden rdzeń, cała sieć.
    // [v65] FB-BATCH: jeśli którakolwiek flaga ustawiona → jeden fbConnect() na cały blok.
    //   fbBatchActive=true → fbConnect() wewnątrz funkcji pomija reconnect (return true od razu)
    //   gdy fbClientReady==true. Po bloku: fbBatchActive=false.
    // [v97-LOG] FB-BLOCK timer (deklaracja poza if — używana przy FB-BLOCK-DONE)
    unsigned long _fbBlockT = 0;
    if (_wifiUpNow) {
      // [v253] NATIVE ASYNC: jeden request in-flight, bez wait-loop.
      if (_netCircuitOpen && millis() < _netCooldownUntilMs) {
        goto _tg_task_iter_end;
      }
      if (_netCircuitOpen && millis() >= _netCooldownUntilMs) {
        logPrintf("lvl=INFO tag=CB msg=\"Half-open, probuje FB async\"\n");
        _netCircuitOpen = false;
      }

      if (fbAppReady) {
        esp_task_wdt_reset();
        fbApp.loop();
        esp_task_wdt_reset();
      }
      fbAsyncProcessResult();

      if (ITER_HAS_BUDGET()) {
        fbAsyncSchedulerStep();
      } else if (fbCmdPending || fbCfgPending || fbSendPending || fbQueueInFlight) {
        logPrintf("lvl=WARN tag=CB msg=\"ITER-BUDGET, odkladam ASYNC FB\" czas=%lums\n",
                  (unsigned long)(millis() - _iterStartMs));
      }
      esp_task_wdt_reset();
    } else {
      // WiFi down: nic nie startujemy; pending flags zostają.
      // fbApp.loop() nie jest wołane przy braku L2.
      static bool _wgLogged = false;
      if (!_wgLogged) {
        _wgLogged = true;
        logPrintf("lvl=WARN tag=WIFI-GUARD akcja=start s=%d c=%d g=%d\n",
                  (int)fbSendPending, (int)fbCmdPending, (int)fbCfgPending);
      }
      vTaskDelay(pdMS_TO_TICKS(100));
      if (WiFi.status() == WL_CONNECTED) {
        _wgLogged = false;
        logPrintf("lvl=INFO tag=WIFI-GUARD akcja=koniec\n");
      }
    }


    // ── [OK] FIX-v39-4: Rotacja FS na Core 0 - nie blokuje loop() ──
    // fsSizeGuard() z loop() ustawiała tę flagę zamiast robić kopiowanie 500KB
    // bezpośrednio na Core 1 (co blokowało loop() przez 4+ sekund).
    if (fsGuardPending) {
      fsGuardPending = false;
      // [v160] DIAG-FSGUARDSTALL: cały ten blok — teraz łącznie z nowym (v159
      // FIX-LOG-SEND-TRIGGER) sendTelegramDocument() — mierzony jako całość.
      // sendTelegramDocument() to wywołanie sieciowe (wysyłka pliku logów przez
      // TLS) uruchamiane DOKŁADNIE w tych samych warunkach degradacji sieci, co
      // dotychczasowe crashe WDT (fsGuardPending odpala się gdy FS się zapełnia,
      // co koreluje z długimi sesjami/dużym ruchem w logach, w tym seriami
      // TIMEOUT). To nowy, silniejszy kandydat niż same remove()/rename() —
      // wysyłka pliku przez sieć pod złymi warunkami może zająć dużo więcej niż
      // 150ms. Komentarz w kompaktowaniu history.csv (v88, ~linia 9998) i
      // komentarz przy rotacji log_b->log_a (linia ~10210) też się nie zgadzają
      // co do kosztu samego rename() (300-400ms vs "~2ms") - nikt tego nie
      // zmierzył. Ten jeden pomiar obejmuje oba podejrzenia naraz.
      unsigned long _fsGuardT0 = millis();
      // [v102] FS_LIMIT 76.6% → 79% — brak log_tmp.txt zwalnia 512KB buforu tymczasowego.
      // Bufor 1.8MB nadal pokrywa: CMD-RUN LOGS (33s), tgTask, wear-leveling.
      // [v152] FIX-FS-CAPACITY: FS_LIMIT to nadal bezpieczny stały margines (~79% realnej
      // pojemności ~10.35MB - ~2.1MB zapasu), ale SMALL_FILE_LIMIT i logi % używają teraz
      // g_fsTotalBytes (realny rozmiar wolumenu) zamiast starej, zaszytej wartości 10420224 B.
      const size_t FS_LIMIT = 8232000UL;  // ~79% z realnej pojemności (było: 76.6% = 7983820)
      size_t used = LittleFS.usedBytes();

      // [v224] DIAG-FSGUARD-DRIFT: po FIX-FSGUARD-RACE, g_fsGuardTriggerUsed = log_a+log_b
      // (cached, z Core 1), a used = LittleFS.usedBytes() (autoryzatywny, z Core 0).
      // _diff > 0: na FS jest więcej niż same logi (historia, inne pliki, narzut LittleFS).
      // _diff < 0: niemożliwe w normalnych warunkach (logi > całość FS).
      // Duże _diff (>1MB): podejrzany narzut LittleFS lub duże pliki poza logami — sprawdź DIAG-FSBREAKDOWN.
      {
        long _diff = (long)used - (long)g_fsGuardTriggerUsed;
        logPrintf("lvl=INFO tag=DIAG-FSGUARD-DRIFT log_bytes_trigger=%uB used_tgtask=%uB diff_logi_vs_used=%ldB msg=\"%s\"\n",
                  (unsigned)g_fsGuardTriggerUsed, (unsigned)used, _diff,
                  (_diff > 1048576L) ? "DUZE_PLIKI_POZA_LOGAMI" : "ok");
      }

      // [v168] DIAG-FSBREAKDOWN: jednorazowy zrzut rozbicia zajętości FS w chwili
      // wyzwolenia FS-GUARD — PRZED sendTelegramDocument()/rotacją/kasowaniem,
      // żeby złapać realny stan, a nie już posprzątany.
      // [v170] FIX-DIAG-FSBREAKDOWN-FULLSCAN: v168 sumowała TYLKO twardą listę
      // znanych plików (log_a/log_b/history*/energy_stats/adapt_stats/wifi_list/
      // telegram_config) — każdy plik SPOZA tej listy, ale realnie istniejący w
      // kodzie (/history_tmp.csv, /wifi_list_tmp.txt, /wifi_list_old.txt,
      // /last_reset.txt) wpadał w "różnica_vs_used", myląco sugerując czysty
      // narzut LittleFS zamiast policzalnych, konkretnych plików - a to dokładnie
      // ten sam scenariusz (FS bliski limitu), w którym te pliki-sieroty realnie
      // powstają. FIX: pełny przelot katalogu root (ten sam wzorzec co
      // /api/fs-list, ~linia 12646) sumuje rozmiar KAŻDEGO pliku faktycznie
      // obecnego na dysku — zero zgadywania z góry, złapie też cokolwiek, o czym
      // nikt teraz nie pamięta.
      {
        size_t _sumAll = 0;
        uint16_t _fileCount = 0;
        String _biggestName;
        size_t _biggestSz = 0;
        File _root = LittleFS.open("/");
        if (_root && _root.isDirectory()) {
          File _entry = _root.openNextFile();
          while (_entry) {
            if (!_entry.isDirectory()) {
              size_t _sz = _entry.size();
              String _nm = String(_entry.name());
              if (!_nm.startsWith("/")) _nm = "/" + _nm;
              _sumAll += _sz;
              _fileCount++;
              if (_sz > _biggestSz) { _biggestSz = _sz; _biggestName = _nm; }
              // Loguj indywidualnie tylko pliki >=4KB - drobne configi (rzędu
              // setek B) nie są interesujące przy szukaniu "widma" rzędu MB.
              if (_sz >= 4096) {
                logPrintf("lvl=INFO tag=DIAG-FSBREAKDOWN plik=%s rozmiar=%u\n", _nm.c_str(), (unsigned)_sz);
              }
            }
            _entry.close();
            _entry = _root.openNextFile();
          }
          _root.close();
        }
        logPrintf("lvl=INFO tag=DIAG-FSBREAKDOWN used=%uB total=%uB plikow=%u suma_plikow=%uB roznica_vs_used=%ldB msg=\"rzeczywisty narzut LittleFS\"\n",
                  (unsigned)used, (unsigned)g_fsTotalBytes, (unsigned)_fileCount,
                  (unsigned)_sumAll, (long)used - (long)_sumAll);
        if (_biggestSz > 0) {
          logPrintf("lvl=INFO tag=DIAG-FSBREAKDOWN najwiekszy_plik=%s rozmiar=%uB\n", _biggestName.c_str(), (unsigned)_biggestSz);
        }
      }

      // [v159] FIX-LOG-SEND-TRIGGER: zanim przytniemy logi "na ślepo", spróbuj
      // najpierw wysłać pełny log_a+log_b na Telegram. Jeśli się uda -> skasuj
      // oba pliki całkowicie (więcej odzyskanego miejsca niż zwykła rotacja).
      // Jeśli się nie uda (WiFi padło, tgEnabled=false, Telegram nie odpowiada) ->
      // _autoSendOk zostaje false i leci stary system rotacji/nadpisywania
      // (Krok 1-3 niżej) bez żadnych zmian, dokładnie jak wcześniej.
      bool _autoSendOk = false;
      // [v225] FIX-FSGUARD-STALE-PENDING: fsGuardPending mogla zostac ustawiona
      // gdy g_logFileSize+g_logFileSizeA > LOG_GUARD_LIMIT, ale zanim ten blok
      // zdazyl sie wykonac, reczne czyszczenie logow (panel WWW /api/log-clear,
      // ~linia 12689, lub Telegram TG_CLR_DO, ~linia 13412) moglo juz wyzerowac
      // g_logFileSize/g_logFileSizeA - ZADNA z tych dwoch sciezek nie resetuje
      // fsGuardPending (potwierdzone czytaniem kodu). Bez tego sprawdzenia
      // ponizszy blok i tak probowalby wyslac na Telegram i skasowac juz
      // nieistniejace/puste pliki logow, logujac mylacy duzy ujemny
      // diff_logi_vs_used. Kroki 1-3 nizej juz same re-sprawdzaja swieze
      // used>FS_LIMIT, wiec nie potrzebuja analogicznej poprawki.
      const size_t LOG_GUARD_LIMIT = 7UL * 1024UL * 1024UL;  // identyczne z fsSizeGuard()
      size_t _logBytesNow = (size_t)g_logFileSize + (size_t)g_logFileSizeA;
      if (_logBytesNow <= LOG_GUARD_LIMIT) {
        logPrintf("lvl=INFO tag=FS-GUARD msg=\"Pending guard nieaktualny - logi juz wyczyszczone recznie w miedzyczasie\" log_bytes_teraz=%uB prog=%uB\n",
                  (unsigned)_logBytesNow, (unsigned)LOG_GUARD_LIMIT);
      } else if (tgEnabled) {
        esp_task_wdt_reset();
        _autoSendOk = sendTelegramDocument();
        if (_autoSendOk) {
          if (logMutex && xSemaphoreTake(logMutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
            if (g_logBuf[0]) { g_logBuf[0][0] = '\0'; g_logBufLen[0] = 0; }
            if (g_logBuf[1]) { g_logBuf[1][0] = '\0'; g_logBufLen[1] = 0; }
            logFlushPending[1] = false;
            // [v223] FIX-FSGUARD-LOG: zmierz rozmiar log_a+log_b PRZED kasowaniem,
            // żeby "odzyskane" było realnym rozmiarem skasowanych plików, a nie
            // mylącym usedBytes() sprzed całej operacji (stary bug: logował ~used
            // całego FS jako "odzyskane", stąd wrażenie "wysłało przy 4MB").
            size_t _szLogA = 0, _szLogB = 0;
            { File _f = LittleFS.open(LOG_FILE_A, "r"); if (_f) { _szLogA = _f.size(); _f.close(); } }
            { File _f = LittleFS.open(LOG_FILE_B, "r"); if (_f) { _szLogB = _f.size(); _f.close(); } }
            bool okB = !LittleFS.exists(LOG_FILE_B) || LittleFS.remove(LOG_FILE_B);
            bool okA = !LittleFS.exists(LOG_FILE_A) || LittleFS.remove(LOG_FILE_A);
            if (okA && okB) {
              g_logFileSize  = 0;
              g_logFileSizeA = 0;
              g_lastFlush[0] = g_lastFlush[1] = millis();
              size_t _usedAfter = LittleFS.usedBytes();
              // [v223] log zawiera: used w chwili wyzwolenia (Core 1), rozmiary plików
              // logów, realnie odzyskane (used_before - used_after), used po kasowaniu.
              logPrintf("lvl=INFO tag=FS-GUARD msg=\"Logi wyslane na Telegram, log_a+log_b usuniete\""
                        " used_trigger=%uB log_a=%uB log_b=%uB logi_razem=%uB odzyskane=%ldB used_po=%uB\n",
                        (unsigned)g_fsGuardTriggerUsed,
                        (unsigned)_szLogA, (unsigned)_szLogB,
                        (unsigned)(_szLogA + _szLogB),
                        (long)used - (long)_usedAfter,
                        (unsigned)_usedAfter);
            } else {
              logPrintf("lvl=ERR tag=FS-GUARD msg=\"Wyslano na Telegram, ale usuwanie log_a/log_b NIEUDANE\" fallback=rotacja\n");
              _autoSendOk = false;  // fallback: leci stary system przycinania niżej
            }
            xSemaphoreGive(logMutex);
          } else {
            logPrintf("lvl=ERR tag=FS-GUARD msg=\"Brak dostepu do logMutex po wyslaniu\" fallback=rotacja\n");
            _autoSendOk = false;
          }
        } else {
          logPrintf("lvl=WARN tag=FS-GUARD msg=\"Wysylka logow na Telegram nieudana\" powod=brak_neta_lub_TG akcja=rotacja_bez_zmian\n");
        }
      }

      // [v102] DUAL-FILE: Krok 1 — rotacja log_b → log_a (zero kopiowania)
      // remove(log_a) i rename(log_b, log_a) to operacje metadanych (~2ms, 0 B flash I/O).
      // Brak log_tmp.txt: nie potrzeba 512KB buforu wolnego miejsca na FS,
      // brak ryzyka Has open FD (nie ma remove() na serwowanym pliku).
      // [v152] FIX-FS-GUARD-SILENT-FAIL: remove()/rename() mogą cicho zawieść gdy LittleFS
      // nie ma wolnych bloków na commit metadanych (znany limit littlefs przy ENOSPC) —
      // teraz sprawdzamy wynik i logujemy błąd zamiast zakładać sukces.
      // [v159] Ten cały blok Kroki 1-3 to teraz TYLKO fallback — wykonuje się
      // wyłącznie gdy wysyłka na Telegram (wyżej) się nie powiodła.
      if (!_autoSendOk && used > FS_LIMIT && LittleFS.exists(LOG_FILE_B)) {
        size_t szB = 0;
        { File _f = LittleFS.open(LOG_FILE_B, "r"); if (_f) { szB = _f.size(); _f.close(); } }
        if (szB > 0) {  // rotuj tylko gdy log_b ma dane
          bool rmOk  = LittleFS.exists(LOG_FILE_A) ? LittleFS.remove(LOG_FILE_A) : true;  // usuń stare archiwum
          bool renOk = LittleFS.rename(LOG_FILE_B, LOG_FILE_A);                            // log_b staje się archiwum
          if (rmOk && renOk) {
            g_logFileSizeA = szB;                             // [v102] zaktualizuj cache
            g_logFileSize  = 0;                               // log_b nie istnieje jeszcze
            logPrintf("lvl=INFO tag=FS-GUARD msg=\"Rotacja logu: log_b -> log_a, log_b reset\" log_b=%uB\n",
                      (unsigned)szB);
          } else {
            logPrintf("lvl=ERR tag=FS-GUARD msg=\"Rotacja logu NIEUDANA, prawdopodobnie brak wolnych blokow na commit\" remove=%d rename=%d\n",
                      (int)rmOk, (int)renOk);
          }
        }
      }

      // Krok 2: Usuń history_old.csv jeśli nadal za dużo (tylko jeśli > 10% dysku)
      used = LittleFS.usedBytes();
      const size_t SMALL_FILE_LIMIT = g_fsTotalBytes / 10;  // 10% realnej pojemności - pliki mniejsze są nienaruszalne
      if (used > FS_LIMIT && LittleFS.exists("/history_old.csv")) {
        File fTmp = LittleFS.open("/history_old.csv", "r");
        size_t szTmp = fTmp ? fTmp.size() : 0; if (fTmp) fTmp.close();
        if (szTmp > SMALL_FILE_LIMIT) {
          bool rmOk = LittleFS.remove("/history_old.csv");
          if (rmOk) {
            logPrintf("lvl=INFO tag=FS-GUARD msg=\"Usunieto /history_old.csv\" rozmiar=%uB\n", (unsigned)szTmp);
          } else {
            logPrintf("lvl=ERR tag=FS-GUARD msg=\"Usuwanie /history_old.csv NIEUDANE, prawdopodobnie brak wolnych blokow na commit\" rozmiar=%uB\n", (unsigned)szTmp);
          }
        } else {
          logPrintf("lvl=INFO tag=FS-GUARD msg=\"history_old.csv < 10%% dysku, pominieto\" rozmiar=%uB\n", (unsigned)szTmp);
        }
      }

      // Krok 3: Przytnij history.csv (zostaw ostatnie 512 KB) - tylko jeśli > 10% dysku
      used = LittleFS.usedBytes();
      if (used > FS_LIMIT && LittleFS.exists(HISTORY_FILE)) {
        File hSrc = LittleFS.open(HISTORY_FILE, "r");
        if (hSrc) {
          size_t hSz = hSrc.size();
          const size_t KEEP_HIST = 524288UL;  // 512 KB
          if (hSz <= SMALL_FILE_LIMIT) {
            hSrc.close();
            logPrintf("lvl=INFO tag=FS-GUARD msg=\"history.csv < 10%% dysku, pominieto\" rozmiar=%uB\n", (unsigned)hSz);
          } else if (hSz > KEEP_HIST) {
            hSrc.seek(hSz - KEEP_HIST);
            while (hSrc.available() && hSrc.peek() != '\n') hSrc.read();
            if (hSrc.available()) hSrc.read();
            File hDst = LittleFS.open("/history_tmp.csv", "w");
            if (hDst) {
              bool writeOk = true;
              // [v96] bufor 4096B PSRAM zamiast 512B → x8 szybsze kopiowanie history.csv
              uint8_t* buf = (uint8_t*)ps_malloc(4096);
              if (!buf) buf = (uint8_t*)malloc(4096);
              if (buf) {
                while (hSrc.available()) {
                  size_t r = hSrc.read(buf, 4096);
                  if (r > 0 && hDst.write(buf, r) != r) { writeOk = false; break; }
                  esp_task_wdt_reset();
                }
                free(buf);
              } else { writeOk = false; }
              hDst.close(); hSrc.close();
              if (writeOk) {
                LittleFS.rename(HISTORY_FILE, "/history_old.csv");
                if (LittleFS.rename("/history_tmp.csv", HISTORY_FILE)) {
                  LittleFS.remove("/history_old.csv");
                  logPrintf("lvl=INFO tag=FS-GUARD msg=\"history.csv przyciety\" przed=%uB po=~%uB\n", (unsigned)hSz, (unsigned)KEEP_HIST);
                  // [v87] OPT-B: po przycinaniu przez FS-GUARD wymuś reskan przy następnym zapisie
                  histStartLine      = 0;
                  histLineCount      = -1;   // -1 = wymuś reskan pliku
                  compactHistPending = false;
                } else {
                  LittleFS.rename("/history_old.csv", HISTORY_FILE);
                  LittleFS.remove("/history_tmp.csv");
                }
              } else {
                LittleFS.remove("/history_tmp.csv");
                logPrintf("lvl=ERR tag=FS-GUARD msg=\"Blad zapisu history_tmp.csv\"\n");
              }
            } else { hSrc.close(); }  // hSz <= KEEP_HIST
          } else { hSrc.close(); }    // hSz <= SMALL_FILE_LIMIT (ten else-if nie istnieje, close already done above)
        }
      }

      used = LittleFS.usedBytes();
      float usedPct = (float)used * 100.0f / (float)g_fsTotalBytes;
      logPrintf("lvl=INFO tag=FS-GUARD msg=\"Po przycinaniu\" uzyte=%uB limit=%uB procent=%.1f%%\n",
                (unsigned)used, (unsigned)g_fsTotalBytes, usedPct);
      // [v152] FIX-FS-GUARD-PLATEAU: jeśli mimo wszystkich 3 kroków dysk nadal ≥97% pełny,
      // czyszczenie realnie nie zwolniło miejsca (znany symptom: usedBytes() plateau tuż
      // przed ENOSPC) — osobny, łatwo grepowalny marker do diagnostyki następnym razem.
      if (usedPct >= 97.0f) {
        logPrintf("lvl=ERR tag=FS-GUARD-CRITICAL msg=\"Dysk nadal pelny PO przycinaniu, czyszczenie nieskuteczne\" procent=%.1f%% mozliwa_przyczyna=brak_wolnych_blokow_littlefs\n",
                  usedPct);
      }
      // [v160] DIAG-FSGUARDSTALL: koniec pomiaru całego bloku (patrz komentarz
      // przy fsGuardPending=false wyżej) — obejmuje sendTelegramDocument() +
      // Kroki 1-3.
      unsigned long _fsGuardMs = millis() - _fsGuardT0;
      if (_fsGuardMs >= FLASH_STALL_THRESHOLD_MS) {
        g_fsGuardStallCount = g_fsGuardStallCount + 1;
        if (_fsGuardMs > g_fsGuardStallMaxMs) g_fsGuardStallMaxMs = _fsGuardMs;
        logPrintf("lvl=WARN tag=FLASH-STALL site=fsGuard ms=%lu cnt=%lu maxMs=%lu\n",
                  _fsGuardMs, (unsigned long)g_fsGuardStallCount, (unsigned long)g_fsGuardStallMaxMs);
      }
    }

    // ── [OK] FIX-v39-2: Powiadomienie TG o resecie - przy pierwszym połączeniu ──
    // POPRAWKA 1: emoji zastąpione czystymi ASCII-tagami (zepsute bajty UTF-8
    //             powodowały że Telegram odrzucał JSON -> sendTelegramMessage
    //             zawsze zwracał false -> pętla co 200ms przez cały czas pracy).
    // POPRAWKA 2: cooldown 30 s między próbami - nawet jeśli TG chwilowo
    //             niedostępne, nie generuje to flooding-u setkami requestów/min.
    {
      static unsigned long _bootNotifyLastTry = 0;
      if (tgBootNotifyPending && tgEnabled &&
          tgBotToken.length() >= 10 && tgChatId.length() >= 3 &&
          WiFi.status() == WL_CONNECTED &&
          (millis() - _bootNotifyLastTry >= 30000UL)) {
        _bootNotifyLastTry = millis();
        const char* rstStr   = "NIEZNANY";
        const char* rstLabel = "[?]";   // ASCII-bezpieczny prefix zamiast emoji
        switch (bootResetReason) {
          case ESP_RST_POWERON:   rstStr = "POWER_ON";      rstLabel = "[PWR]";   break;
          case ESP_RST_EXT:       rstStr = "EXT_PIN";       rstLabel = "[EXT]";   break;
          case ESP_RST_SW:        rstStr = "SOFTWARE";      rstLabel = "[SW]";    break;
          case ESP_RST_PANIC:     rstStr = "PANIC/CRASH";   rstLabel = "[CRASH]"; break;
          case ESP_RST_INT_WDT:   rstStr = "WDT_INTERRUPT"; rstLabel = "[WDT]";   break;
          case ESP_RST_TASK_WDT:  rstStr = "WDT_TASK";      rstLabel = "[WDT]";   break;
          case ESP_RST_WDT:       rstStr = "WDT_OTHER";     rstLabel = "[WDT]";   break;
          case ESP_RST_BROWNOUT:  rstStr = "BROWNOUT";      rstLabel = "[BRNOUT]";break;
          default: break;
        }
        String bootMsg = String(rstLabel) + " *RESTART SYSTEMU*\n";
        bootMsg += "Przyczyna: `" + String(rstStr) + "`\n";
        bootMsg += "Uptime: " + String(millis() / 1000) + "s\n";
        bootMsg += "Heap: " + String(ESP.getFreeHeap() / 1024) + " kB";
        // Dla nienormalnych resetów dodaj ostrzezenie (ASCII only)
        if (bootResetReason == ESP_RST_PANIC)
          bootMsg += "\n[!] CRASH - sprawdz logi!";
        else if (bootResetReason == ESP_RST_TASK_WDT ||
                 bootResetReason == ESP_RST_INT_WDT ||
                 bootResetReason == ESP_RST_WDT)
          bootMsg += "\n[!] Watchdog - petla byla zablokowana!";
        else if (bootResetReason == ESP_RST_BROWNOUT)
          bootMsg += "\n[!] Brownout - sprawdz zasilanie!";
        if (sendTelegramMessage(bootMsg)) {
          tgBootNotifyPending = false;
        }
        // ************************************************FIX-v57-RESET-LOOP (ROOT CAUSE A+C)
        // Niezależnie od wyniku wysyłki: zresetuj WDT po potencjalnie długiej operacji TLS
        // i pomiń TG-POLL w tej iteracji. Zapobiega nakładaniu dwóch bloków TLS (~16s)
        // które przekraczają WDT (10s). Jeśli bootNotify się nie powiodło, zostanie
        // ponowiony po cooldown 30s — TG-POLL nie jest pilniejszy niż boot notify.
        esp_task_wdt_reset();  // FIX-v57: reset WDT po sendMessage (potencjalnie 8s)
        continue;              // FIX-v57: pomiń TG-POLL, daj WDT/HEAP czas na reset
      }
    }

    // ── [v69-Z0/Z4] DS-DIAG alert Telegram — wysłany raz po starcie gdy czujnik wody FAIL ──
    if (ds3BootFail && tgEnabled && tgBotToken.length() >= 10 &&
        WiFi.status() == WL_CONNECTED && !tgBootNotifyPending) {
      String ds3Msg = "⚠️ [DS-DIAG] DS18B20 wody FAIL (GPIO" + String(ONE_WIRE_BUS3) + "):\n";
      ds3Msg += "Czujnik nie odpowiedział przy starcie (3 próby).\n";
      // [v69-Z0] Stan pinu DATA
      ds3Msg += "Stan pinu DATA (GPIO" + String(ONE_WIRE_BUS3) + "): ";
      ds3Msg += (g_ds3PinLevel == 1) ? "HIGH — pull-up OK, problem z czujnikiem\n"
              : (g_ds3PinLevel == 0) ? "LOW — BRAK PULL-UP lub zwarcie DATA do GND!\n"
              :                        "niezmierzony\n";
      // [v69-Z4] Reset pulse i ROM
      ds3Msg += "Reset pulse: " + String(g_ds3ResetOk ? "OK (urządzenie odpowiedziało)" : "BRAK (brak urządzenia lub zwarcie)") + "\n";
      ds3Msg += "ROM: " + String(g_ds3AddrOk ? "odczytany" : "BRAK (CRC error lub brak urządzenia)") + "\n";
      ds3Msg += "Sprawdź:\n";
      ds3Msg += "1. Kabel DATA na GPIO" + String(ONE_WIRE_BUS3) + "\n";
      ds3Msg += "2. Rezystor pull-up 4.7kΩ między DATA a VCC\n";
      ds3Msg += "3. Zasilanie VCC (3.3V lub 5V pasożytnicze)\n";
      ds3Msg += "Temperatura wody: NIEZNANA (używana ostatnia wartość: " +
                String(tempWater, 1) + "°C)";
      if (sendTelegramMessage(ds3Msg)) {
        ds3BootFail = false;  // wyślij tylko raz
        logPrintf("lvl=INFO tag=DS-DIAG msg=\"Alert Telegram wyslany\"\n");
      }
      esp_task_wdt_reset();
      continue;
    }

    // ── CZUJNIK OBECNOŚCI WODY — powiadomienia Telegram (wykrycie / zanik) ──
    if (waterLeakDetectedNotifyPending && tgEnabled && tgBotToken.length() >= 10 &&
        WiFi.status() == WL_CONNECTED) {
      if (sendTelegramMessage("🚨 <b>WYKRYTO WODĘ</b> (GPIO" + String(WATER_LEAK_PIN) + ")! Sprawdź akwarium.")) {
        waterLeakDetectedNotifyPending = false;
        logPrintf("lvl=INFO tag=WATER-LEAK msg=\"Alert Telegram wyslany\"\n");
      }
      esp_task_wdt_reset();
      continue;
    }
    if (waterLeakClearedNotifyPending && tgEnabled && tgBotToken.length() >= 10 &&
        WiFi.status() == WL_CONNECTED) {
      if (sendTelegramMessage("✅ Czujnik wody: sucho — detekcja ustąpiła (GPIO" + String(WATER_LEAK_PIN) + ").")) {
        waterLeakClearedNotifyPending = false;
        logPrintf("lvl=INFO tag=WATER-LEAK msg=\"Alert clear Telegram wyslany\"\n");
      }
      esp_task_wdt_reset();
      continue;
    }

    if (tgEnabled && tgBotToken.length() >= 10 && WiFi.status() == WL_CONNECTED) {
      unsigned long nowMs = millis();
      if (nowMs - lastPollInTask >= 30000UL) {
        lastPollInTask = nowMs;
        // [FIX-v131] TG poll tylko jeśli mamy budżet i circuit zamknięty
        if (!_netCircuitOpen && ITER_HAS_BUDGET()) {
          // [v240] Najpierw spróbuj domknąć oczekujące crash-reporty; potem zwykły polling TG.
          // Wysyłka jest throttlowana wewnątrz wyslijOczekujaceCoredumpyTelegram().
          wyslijOczekujaceCoredumpyTelegram();
          esp_task_wdt_reset();
          pollTelegramCommands();   // wywołanie bezpieczne - jesteśmy na Core 0
          esp_task_wdt_reset();
        } else if (!ITER_HAS_BUDGET()) {
          logPrintf("lvl=WARN tag=CB msg=\"ITER-BUDGET, skip TG-POLL\" czas=%lums\n",
                    (unsigned long)(millis() - _iterStartMs));
        }
        // [v130-LOG] LOG-QUIET-TGPOLL: START/END usunięte — [TG-POST] getUpdates w środku wystarczy.
        // pollTelegramCommands() może ustawić tgDeferredCmd; obsłuż go od razu
        if (tgDeferredCmd != TG_NONE) {
          cmd = tgDeferredCmd;
          tgDeferredCmd = TG_NONE;
        }
      }
      // ── Timer powrotu do menu ──
      if (menuReturnInTask > 0 && nowMs >= menuReturnInTask && cmd == TG_NONE) {
        menuReturnInTask = 0;
        cmd = TG_MENU;
      }
    }

    if (cmd == TG_NONE) continue;

    // ── DBG-CMD: loguj każdą komendę z heap przed i po ──
    logPrintf("lvl=INFO tag=TG-CMD cmd=%d freeH=%luB maxAlloc=%luB\n",
              (int)cmd, (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getMaxAllocHeap());
    _cmdT = millis();  // [v81-DIAG] timing komendy

    // ── Wykonaj komendę ──
    switch (cmd) {
      case TG_MENU:
        logPrintf("lvl=INFO tag=CMD-RUN cmd=MENU faza=start fH=%lu\n", (unsigned long)ESP.getFreeHeap());
        sendTelegramInlineMenu();
        logPrintf("lvl=INFO tag=CMD-RUN cmd=MENU faza=done czas=%lums fH=%lu\n", millis()-_cmdT, (unsigned long)ESP.getFreeHeap());
        break;

      case TG_LOGS:
        logPrintf("lvl=INFO tag=CMD-RUN cmd=LOGS faza=start fH=%lu\n", (unsigned long)ESP.getFreeHeap());
        tgDeleteAllHistory();
        sendTelegramDocument();
        menuReturnInTask = millis() + 30000UL;
        logPrintf("lvl=INFO tag=CMD-RUN cmd=LOGS faza=done czas=%lums fH=%lu\n", millis()-_cmdT, (unsigned long)ESP.getFreeHeap());
        break;

      case TG_STATUS:
        logPrintf("lvl=INFO tag=CMD-RUN cmd=STATUS faza=start fH=%lu\n", (unsigned long)ESP.getFreeHeap());
        tgDeleteAllHistory();
        sendTelegramMessage(buildTelegramStatusReport());
        menuReturnInTask = millis() + 30000UL;
        logPrintf("lvl=INFO tag=CMD-RUN cmd=STATUS faza=done czas=%lums fH=%lu\n", millis()-_cmdT, (unsigned long)ESP.getFreeHeap());
        break;

      case TG_TEMP:
        logPrintf("lvl=INFO tag=CMD-RUN cmd=TEMP faza=start fH=%lu\n", (unsigned long)ESP.getFreeHeap());
        tgDeleteAllHistory();
        sendTelegramMessage(buildTelegramTempReport());
        menuReturnInTask = millis() + 30000UL;
        logPrintf("lvl=INFO tag=CMD-RUN cmd=TEMP faza=done czas=%lums fH=%lu\n", millis()-_cmdT, (unsigned long)ESP.getFreeHeap());
        break;

      case TG_ENERGY:
        logPrintf("lvl=INFO tag=CMD-RUN cmd=ENERGY faza=start fH=%lu\n", (unsigned long)ESP.getFreeHeap());
        tgDeleteAllHistory();
        sendTelegramMessage(buildTelegramEnergyReport());
        menuReturnInTask = millis() + 30000UL;
        logPrintf("lvl=INFO tag=CMD-RUN cmd=ENERGY faza=done czas=%lums fH=%lu\n", millis()-_cmdT, (unsigned long)ESP.getFreeHeap());
        break;

      case TG_LIGHT:
        logPrintf("lvl=INFO tag=CMD-RUN cmd=LIGHT faza=start fH=%lu\n", (unsigned long)ESP.getFreeHeap());
        tgDeleteAllHistory();
        sendTelegramMessage(buildTelegramLightReport());
        menuReturnInTask = millis() + 30000UL;
        logPrintf("lvl=INFO tag=CMD-RUN cmd=LIGHT faza=done czas=%lums fH=%lu\n", millis()-_cmdT, (unsigned long)ESP.getFreeHeap());
        break;

      case TG_ADAPT:
        logPrintf("lvl=INFO tag=CMD-RUN cmd=ADAPT faza=start fH=%lu\n", (unsigned long)ESP.getFreeHeap());
        tgDeleteAllHistory();
        sendTelegramMessage(buildTelegramAdaptReport());
        menuReturnInTask = millis() + 30000UL;
        logPrintf("lvl=INFO tag=CMD-RUN cmd=ADAPT faza=done czas=%lums fH=%lu\n", millis()-_cmdT, (unsigned long)ESP.getFreeHeap());
        break;

      case TG_SENSOR_HIST:
        logPrintf("lvl=INFO tag=CMD-RUN cmd=SENSOR_HIST faza=start fH=%lu\n", (unsigned long)ESP.getFreeHeap());
        tgDeleteAllHistory();
        sendTelegramMessage(buildTelegramSensorHistory());
        menuReturnInTask = millis() + 30000UL;
        logPrintf("lvl=INFO tag=CMD-RUN cmd=SENSOR_HIST faza=done czas=%lums fH=%lu\n", millis()-_cmdT, (unsigned long)ESP.getFreeHeap());
        break;

      case TG_SCHEDULE:
        logPrintf("lvl=INFO tag=CMD-RUN cmd=SCHEDULE faza=start fH=%lu\n", (unsigned long)ESP.getFreeHeap());
        tgDeleteAllHistory();
        sendTelegramMessage(buildTelegramScheduleReport());
        menuReturnInTask = millis() + 30000UL;
        logPrintf("lvl=INFO tag=CMD-RUN cmd=SCHEDULE faza=done czas=%lums fH=%lu\n", millis()-_cmdT, (unsigned long)ESP.getFreeHeap());
        break;

      case TG_LED_TOGGLE: {
        bool ok = enqueueAppCommand(AppCommandType::POWER_TOGGLE, false, nullptr, "telegram");
        sendTelegramMessage(ok ? "💡 Przełączam zasilanie LED…" : "[ERR] Kolejka sterowania pełna.");
        menuReturnInTask = millis() + 30000UL;
        break;
      }

      case TG_TRYB_TOGGLE: {
        bool ok = enqueueAppCommand(AppCommandType::TRYB_TOGGLE, false, nullptr, "telegram");
        sendTelegramMessage(ok ? "🤖 Przełączam tryb…" : "[ERR] Kolejka sterowania pełna.");
        menuReturnInTask = millis() + 30000UL;
        break;
      }

      case TG_CLR_CONFIRM:
        logPrintf("lvl=INFO tag=CMD-RUN cmd=CLR_CONFIRM faza=start\n");
        sendTelegramConfirmClear();
        logPrintf("lvl=INFO tag=CMD-RUN cmd=CLR_CONFIRM faza=done czas=%lums\n", millis()-_cmdT);
        break;

      case TG_CLR_DO:
        logPrintf("lvl=INFO tag=CMD-RUN cmd=CLR_DO faza=start fH=%lu\n", (unsigned long)ESP.getFreeHeap());
        // [v225] DIAG-FSGUARD-STALE-PENDING: patrz komentarz w logClearPending (~12689).
        if (fsGuardPending) {
          logPrintf("lvl=WARN tag=FS-GUARD msg=\"Reczne czyszczenie logow (Telegram) z pending guardem w locie\"\n");
        }
        if (g_logBuf[0]) { g_logBuf[0][0] = '\0'; g_logBufLen[0] = 0; }
        if (g_logBuf[1]) { g_logBuf[1][0] = '\0'; g_logBufLen[1] = 0; }
        logFlushPending[1] = false;
        // [v102] DUAL-FILE: usuń oba pliki. Wykonywane z tgTask (Core 0) bez HTTP FD.
        if (littlefsReady) {
          if (LittleFS.exists(LOG_FILE_B)) LittleFS.remove(LOG_FILE_B);
          if (LittleFS.exists(LOG_FILE_A)) LittleFS.remove(LOG_FILE_A);
        }
        g_logFileSize = 0; g_logFileSizeA = 0;  // [v102]
        sendTelegramMessage("🗑️ Logi zostały <b>wyczyszczone</b> [OK]");
        menuReturnInTask = millis() + 30000UL;
        logPrintf("lvl=INFO tag=CMD-RUN cmd=CLR_DO faza=done czas=%lums fH=%lu\n", millis()-_cmdT, (unsigned long)ESP.getFreeHeap());
        break;

      case TG_RESTART_CONFIRM:
        logPrintf("lvl=INFO tag=CMD-RUN cmd=RESTART_CONFIRM faza=start\n");
        sendTelegramConfirmRestart();
        logPrintf("lvl=INFO tag=CMD-RUN cmd=RESTART_CONFIRM faza=done czas=%lums\n", millis()-_cmdT);
        break;

      case TG_RESTART_DO:
        logPrintf("lvl=INFO tag=CMD-RUN cmd=RESTART_DO msg=\"Wysylam wiadomosc i restartuje\"\n");
        sendTelegramMessage("🔄 <b>Restartuję ESP32</b>... urządzenie wróci za ~10 sekund.");
        vTaskDelay(pdMS_TO_TICKS(500));
        restartRequestedAt = millis();
        break;

      case TG_NOTIF_TOGGLE:
        logPrintf("lvl=INFO tag=CMD-RUN cmd=NOTIF_TOGGLE faza=start tgEnabled=%d fH=%lu\n", (int)tgEnabled, (unsigned long)ESP.getFreeHeap());
        tgEnabled = !tgEnabled;
        saveTelegramConfig();
        if (tgEnabled) {
          sendTelegramMessage("🔔 Powiadomienia Telegram <b>WŁĄCZONE</b> [OK]");
          menuReturnInTask = millis() + 30000UL;
        } else {
          sendTelegramMessage("🔕 Powiadomienia Telegram <b>WYŁĄCZONE</b>.\nWyślij /start aby włączyć.", false);
        }
        logPrintf("lvl=INFO tag=CMD-RUN cmd=NOTIF_TOGGLE faza=done tgEnabled=%d czas=%lums\n", (int)tgEnabled, millis()-_cmdT);
        break;

      case TG_TEST_DAILY_SUMMARY:
        // [v227] TEST-ONLY (plan flash-freeze, Partia 4) - do usunięcia po testach.
        // Ustawia flagę sprawdzaną w logDailySummary() (Core 1, loop()) - samo
        // wykonanie logiki zostaje w oryginalnym miejscu/rdzeniu, tu tylko żądanie.
        logPrintf("lvl=INFO tag=CMD-RUN cmd=TEST_DAILY_SUMMARY msg=\"Wymuszam test logDailySummary\"\n");
        g_forceDailySummaryTest = true;
        sendTelegramMessage("🧪 Wymuszam test logDailySummary() przy najbliższej iteracji loop() - sprawdź logi (tag=SUMMARY, tag=ENERGIA).", false);
        break;

      case TG_OTA_UPDATE: {
        // [4.1.0 OTA-GITHUB] Etap 1 planu upgrade: OTA przez GitHub Releases.
        // otaGithubRequest() jest nieblokujące — tylko startuje task OTA na
        // Core 0 (otaGithubTaskFn). Sam flash/restart obsłuży ten task; tu
        // jedynie potwierdzamy użytkownikowi start i odsyłamy do statusu.
        logPrintf("lvl=INFO tag=CMD-RUN cmd=OTA_UPDATE faza=start\n");
        String _otaErr;
        bool _otaOk = otaGithubRequest(false, _otaErr);
        if (_otaOk) {
          sendTelegramMessage("⬇️ <b>Sprawdzam aktualizację</b> (GitHub Releases)...\n"
                              "Status: <code>GET /api/ota-status</code>. Po udanym flashu ESP sam się zrestartuje.", false);
        } else {
          sendTelegramMessage(String("[ERR] Nie można wystartować OTA: ") + _otaErr, false);
        }
        logPrintf("lvl=INFO tag=CMD-RUN cmd=OTA_UPDATE faza=done ok=%d err=%s czas=%lums\n",
                  (int)_otaOk, _otaErr.c_str(), millis()-_cmdT);
        break;
      }

      default:
        logPrintf("lvl=WARN tag=CMD-RUN msg=\"NIEZNANA komenda, ignoruje\" komenda=%d\n", (int)cmd);
        break;
    }
    _tg_task_iter_end:
    esp_task_wdt_reset();  // [v131] zawsze na końcu iteracji
  }
  // Task nigdy nie powinien tu dotrzeć - na wszelki wypadek:
  esp_task_wdt_delete(NULL);  // wyrejestruj z TWDT przed usunięciem tasku
  vTaskDelete(nullptr);
}

// ─────────────────────────────────────────────────────────────────
// PSRAM HELPER: bezpieczna alokacja z fallbackiem na DRAM
//
// Próbuje alokować z PSRAM (ps_malloc). Jeśli PSRAM niedostępny
// lub brak miejsca – fallback na wewnętrzny DRAM (MALLOC_CAP_INTERNAL).
//
// ZAKAZ użycia dla: stosów tasków FreeRTOS, zmiennych ISR,
// buforów DMA (SPI/I2S), zmiennych OneWire/DS18B20,
// portMUX_TYPE, SemaphoreHandle_t.
//
// MOŻNA użyć dla: dużych buforów logów (>4 kB), buforów HTTP/HTTPS,
// buforów TLS do wysyłania plików, dużych JSON-ów Firebase/Telegram,
// buforów WebSocket.
// ─────────────────────────────────────────────────────────────────
void* psramAllocSafe(size_t size) {
    void* ptr = ps_malloc(size);
    if (!ptr) {
        ptr = heap_caps_malloc(size,
                  MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    return ptr;
}

// [v93] FIX-WDT-LIBRARY: mathieucarbou/AsyncTCP (>=3.3.2) obsługuje TCPIP core locking
// wewnętrznie — webserialBeginCb() i tcpip_callback_with_block() USUNIĘTE.
// webserialServer.begin() wywołuje się bezpośrednio; biblioteka sama zajmuje się lockiem.
// (v92 webserialBeginCb + tcpip_callback_with_block były workaroundem dla me-no-dev/AsyncTCP)

// [v102] DUAL-FILE: czyta ostatnie `cap` bajtów z [log_a + log_b] do PSRAM.
// Kolejność: log_a (starsze) → log_b (nowsze) = chronologicznie poprawna.
// Zwraca bufor PSRAM (caller musi zwolnić) i faktyczny rozmiar w outLen.
// Zwraca nullptr gdy brak PSRAM.
static char* logReadCombinedTail(size_t cap, size_t& outLen) {
  outLen = 0;
  char* buf = (char*)ps_malloc(cap + 1);
  if (!buf) return nullptr;

  size_t szA = 0, szB = 0;
  if (LittleFS.exists(LOG_FILE_B)) {
    File f = LittleFS.open(LOG_FILE_B, "r");
    if (f) { szB = f.size(); f.close(); }
  }
  if (LittleFS.exists(LOG_FILE_A)) {
    File f = LittleFS.open(LOG_FILE_A, "r");
    if (f) { szA = f.size(); f.close(); }
  }
  size_t total = szA + szB;
  if (total == 0) { buf[0] = '\0'; return buf; }

  // Ile pomijamy od początku żeby zmieścić się w cap?
  size_t skip = (total > cap) ? (total - cap) : 0;
  char* ptr = buf;

  // Czytaj ogon log_a (jeśli potrzebny)
  size_t skipA = (skip < szA) ? skip : szA;
  if (szA > skipA) {
    File fa = LittleFS.open(LOG_FILE_A, "r");
    if (fa) {
      fa.seek(skipA);
      size_t n = fa.read((uint8_t*)ptr, szA - skipA);
      ptr += n; outLen += n;
      fa.close();
    }
  }

  // Czytaj ogon log_b (jeśli potrzebny)
  size_t skipB = (skip > szA) ? (skip - szA) : 0;
  if (szB > skipB) {
    File fb = LittleFS.open(LOG_FILE_B, "r");
    if (fb) {
      fb.seek(skipB);
      size_t n = fb.read((uint8_t*)ptr, szB - skipB);
      ptr += n; outLen += n;
      fb.close();
    }
  }

  buf[outLen] = '\0';
  return buf;
}

// [v222] FIX-OTA-NEVER-STARTED: ten sam root cause i ten sam fix co
// FIX-WWW-NEVER-STARTED (patrz komentarz w WiFi monitor, ~linia 17330).
// Log z urządzenia: tag=SETUP "Brak WiFi w setup() po 2 probach,
// webserialServer I ArduinoOTA NIE wystartowaly w tym boocie" — ArduinoOTA
// miał dokładnie tę samą lukę co webserialServer, tylko nikt jej wcześniej
// nie zauważył, bo objaw (nie można wgrać firmware przez OTA) ujawnia się
// dopiero przy PRÓBIE wgrania, nie od razu jak brak strony WWW.
// Wydzielone z setup() do osobnej funkcji, żeby wywołać ją z dwóch miejsc
// (setup() przy udanym połączeniu, WiFi monitor w loop() gdy połączenie
// przyszło później) bez duplikowania 40 linii callbacków - jedno źródło
// prawdy zamiast kopii, którą trzeba pamiętać synchronizować.
bool otaReady = false;
void startOtaIfNeeded() {
  // Hasło i hostname muszą zgadzać się z platformio.ini:
  //   upload_protocol = espota
  //   upload_port     = ryby-led-s3.local
  //   upload_flags    = --auth=AkwPanel2026! --timeout=30
  //
  // WDT: firmware 1.7 MB ~17s przy 100 KB/s → przekracza limit 15s.
  // onStart: usuwa task z watchlisty WDT (esp_task_wdt_delete).
  // onError: przywraca WDT jeśli upload nieudany.
  // onEnd:   ESP restartuje automatycznie — WDT nie wraca.
  ArduinoOTA.setHostname("ryby-led-s3");
  ArduinoOTA.setPassword("AkwPanel2026!");
  ArduinoOTA.onStart([]() {
    esp_task_wdt_delete(NULL);   // [v98-OTA] wyłącz WDT na czas uploadu (>15s)
    logPrintln("lvl=INFO tag=OTA msg=\"Wgrywanie firmware, WDT wylaczony\"");
    logForceFlush();
  });
  ArduinoOTA.onEnd([]() {
    // [v169] FIX-OTA-SRC-MISLABEL: brakowało tego, co robią oba pozostałe
    // jawne restarty (linie ~14683, ~17342) — g_restartPending=true (blokuje
    // heartbeat tgTaskFn przed nadpisaniem src=2/WDT, wzorzec z v154
    // PRERESET-FLIPFLOP) + PRE_RESET_UPDATE(4) (src=OTA zamiast domyślnego
    // "WDT(brak danych)" z ostatniego heartbeatu). Biblioteka ArduinoOTA
    // wywołuje restart samodzielnie zaraz po tym callbacku — nie ma tu
    // własnego ESP.restart(), stąd wcześniej nikt tego nie ustawiał.
    g_restartPending = true;
    PRE_RESET_UPDATE(4);  // [v169] src=4 -> OTA
    logPrintln("lvl=INFO tag=OTA msg=\"Upload zakonczony, restart\"");
    logForceFlush();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    static unsigned int _lastPct = 0;
    unsigned int pct = (total > 0) ? (progress * 100 / total) : 0;
    if (pct != _lastPct && pct % 20 == 0) {
      _lastPct = pct;
      logPrintf("lvl=INFO tag=OTA procent=%u progress=%u total=%u\n", pct, progress, total);
    }
  });
  ArduinoOTA.onError([](ota_error_t error) {
    const char* msg = "nieznany";
    if      (error == OTA_AUTH_ERROR)    msg = "Auth failed (złe hasło?)";
    else if (error == OTA_BEGIN_ERROR)   msg = "Begin failed (za mało miejsca?)";
    else if (error == OTA_CONNECT_ERROR) msg = "Connect failed";
    else if (error == OTA_RECEIVE_ERROR) msg = "Receive failed";
    else if (error == OTA_END_ERROR)     msg = "End failed";
    logPrintf("lvl=ERR tag=OTA kod=%u opis=\"%s\"\n", (unsigned)error, msg);
    logForceFlush();
    esp_task_wdt_add(NULL);   // [v98-OTA] przywróć WDT po nieudanym OTA
  });
  ArduinoOTA.begin();
  logPrintf("lvl=INFO tag=OTA msg=\"Gotowy\" host=ryby-led-s3.local port=3232 ip=%s\n",
            WiFi.localIP().toString().c_str());
}


// ═══════════════════════════════════════════════════════════════════════════
// [v240] COREDUMP ARCHIVE + RETRY TELEGRAM
// ═══════════════════════════════════════════════════════════════════════════

static size_t   g_lastArchivedCoredumpSize = 0;
static uint32_t g_lastArchivedCoredumpSeq  = 0;
static uint32_t g_coredumpBootId           = 0;

static uint32_t coredumpNvsGet(const char* key, uint32_t def) {
  Preferences prefs;
  if (!prefs.begin(COREDUMP_NVS_NAMESPACE, true)) return def;
  uint32_t v = prefs.getUInt(key, def);
  prefs.end();
  return v;
}

static bool coredumpNvsPut(const char* key, uint32_t value) {
  Preferences prefs;
  if (!prefs.begin(COREDUMP_NVS_NAMESPACE, false)) return false;
  size_t rc = prefs.putUInt(key, value);
  prefs.end();
  // Preferences::putUInt() zwraca liczbę zapisanych bajtów, nie zapisaną wartość.
  return rc == sizeof(value);
}

static uint32_t coredumpAlokujBootId() {
  if (g_coredumpBootId != 0) return g_coredumpBootId;

  uint32_t last = coredumpNvsGet(COREDUMP_NVS_KEY_BOOT, 0);
  if (last == 0xFFFFFFFFu) last = 0;
  uint32_t next = last + 1;
  if (next == 0) next = 1;
  if (!coredumpNvsPut(COREDUMP_NVS_KEY_BOOT, next)) return 0;

  g_coredumpBootId = next;
  return g_coredumpBootId;
}

static bool coredumpCzyResetCrashowy(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_PANIC:
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:
      return true;
    default:
      return false;
  }
}

static bool coredumpParsujNazwe(const String& pelnaNazwa, uint32_t& seq, uint32_t& boot) {
  String nm = pelnaNazwa;
  int slash = nm.lastIndexOf('/');
  if (slash >= 0) nm = nm.substring(slash + 1);
  seq = 0;
  boot = 0;
  unsigned long parsedSeq = 0;
  unsigned long parsedBoot = 0;
  if (sscanf(nm.c_str(), "dump_seq%lu_boot%lu.bin", &parsedSeq, &parsedBoot) != 2) return false;
  char expected[80];
  snprintf(expected, sizeof(expected), "dump_seq%010lu_boot%lu.bin",
           parsedSeq, parsedBoot);
  if (nm != String(expected)) return false;
  seq = (uint32_t)parsedSeq;
  boot = (uint32_t)parsedBoot;
  return seq != 0;
}

static bool coredumpZbudujSciezke(uint32_t seq, uint32_t boot, char* out, size_t outCap) {
  if (!out || outCap == 0 || seq == 0) return false;
  int n = snprintf(out, outCap, "%s/dump_seq%010lu_boot%lu.bin", COREDUMP_DIR,
                   (unsigned long)seq, (unsigned long)boot);
  return n > 0 && (size_t)n < outCap;
}

static uint32_t coredumpNajwyzszeSeqZFS() {
  if (!littlefsReady || !LittleFS.exists(COREDUMP_DIR)) return 0;
  uint32_t maxSeq = 0;
  File dir = LittleFS.open(COREDUMP_DIR);
  if (!dir || !dir.isDirectory()) return 0;
  File e = dir.openNextFile();
  while (e) {
    if (!e.isDirectory()) {
      uint32_t seq = 0, boot = 0;
      if (coredumpParsujNazwe(String(e.name()), seq, boot) && seq > maxSeq) maxSeq = seq;
    }
    e.close();
    e = dir.openNextFile();
  }
  dir.close();
  return maxSeq;
}

static uint32_t coredumpAlokujSeq() {
  uint32_t nvsLast = coredumpNvsGet(COREDUMP_NVS_KEY_SEQ, 0);
  uint32_t fsLast  = coredumpNajwyzszeSeqZFS();
  uint32_t last = (nvsLast > fsLast) ? nvsLast : fsLast;
  if (last == 0xFFFFFFFFu) return 0;
  uint32_t next = last + 1;
  if (!coredumpNvsPut(COREDUMP_NVS_KEY_SEQ, next)) return 0;
  return next;
}

static uint16_t coredumpPoliczPliki() {
  if (!littlefsReady || !LittleFS.exists(COREDUMP_DIR)) return 0;
  uint16_t count = 0;
  File dir = LittleFS.open(COREDUMP_DIR);
  if (!dir || !dir.isDirectory()) return 0;
  File e = dir.openNextFile();
  while (e) {
    uint32_t seq = 0, boot = 0;
    if (!e.isDirectory() && e.size() > 0 && coredumpParsujNazwe(String(e.name()), seq, boot)) count++;
    e.close();
    e = dir.openNextFile();
  }
  dir.close();
  return count;
}

static bool coredumpZnajdzNajstarszy(bool tylkoNiewyslany, String& path, uint32_t& seq, uint32_t& boot, size_t& size) {
  path = String(); seq = 0; boot = 0; size = 0;
  if (!littlefsReady || !LittleFS.exists(COREDUMP_DIR)) return false;
  uint32_t sentText = coredumpNvsGet(COREDUMP_NVS_KEY_TEXT_SEQ, 0);
  uint32_t sentFile = coredumpNvsGet(COREDUMP_NVS_KEY_FILE_SEQ, 0);
  File dir = LittleFS.open(COREDUMP_DIR);
  if (!dir || !dir.isDirectory()) return false;
  File e = dir.openNextFile();
  bool found = false;
  while (e) {
    if (!e.isDirectory() && e.size() > 0) {
      uint32_t s = 0, b = 0;
      if (coredumpParsujNazwe(String(e.name()), s, b)) {
        bool pending = !tylkoNiewyslany || s > sentText || s > sentFile;
        if (pending && (!found || s < seq)) {
          found = true;
          seq = s; boot = b; size = e.size();
          path = String(COREDUMP_DIR) + "/" + String(e.name()).substring(String(e.name()).lastIndexOf('/') + 1);
        }
      }
    }
    e.close();
    e = dir.openNextFile();
  }
  dir.close();
  return found;
}

static void coredumpPrzytnijArchiwum() {
  uint16_t count = coredumpPoliczPliki();
  while (count > COREDUMP_MAX_FILES) {
    String victim;
    uint32_t victimSeq = 0, victimBoot = 0;
    size_t victimSize = 0;
    if (!coredumpZnajdzNajstarszy(false, victim, victimSeq, victimBoot, victimSize)) break;
    // Nie usuwaj najnowszego, dopiero co zarchiwizowanego pliku, gdyby skan zwrocil go przy anomalii.
    if (victimSeq == g_lastArchivedCoredumpSeq && count > 1) {
      String alt;
      uint32_t seq2 = 0, boot2 = 0; size_t size2 = 0;
      if (!coredumpZnajdzNajstarszy(false, alt, seq2, boot2, size2) || seq2 == victimSeq) break;
      victim = alt; victimSeq = seq2; victimBoot = boot2; victimSize = size2;
    }
    if (!LittleFS.remove(victim)) {
      logPrintf("lvl=WARN tag=COREDUMP zdarzenie=przycinanie_usun_fail plik=%s seq=%lu rozmiar=%uB\n",
                victim.c_str(), (unsigned long)victimSeq, (unsigned)victimSize);
      break;
    }
    logPrintf("lvl=INFO tag=COREDUMP zdarzenie=usunieto_najstarszy plik=%s seq=%lu boot=%lu rozmiar=%uB\n",
              victim.c_str(), (unsigned long)victimSeq, (unsigned long)victimBoot, (unsigned)victimSize);
    count = coredumpPoliczPliki();
  }
}

void archiwizujCoredumpJesliCrash(esp_reset_reason_t powodResetu) {
  g_lastArchivedCoredumpSize = 0;
  g_lastArchivedCoredumpSeq = 0;
  if (!littlefsReady || !coredumpCzyResetCrashowy(powodResetu)) return;
#if CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH
  esp_err_t checkErr = esp_core_dump_image_check();
  if (checkErr != ESP_OK) {
    if (checkErr != ESP_ERR_NOT_FOUND) {
      logPrintf("lvl=ERR tag=COREDUMP zdarzenie=obraz_niewazny err=%d akcja=erase\n", (int)checkErr);
      (void)esp_core_dump_image_erase();
    }
    return;
  }

  size_t flashAddr = 0;
  size_t dumpSize = 0;
  esp_err_t err = esp_core_dump_image_get(&flashAddr, &dumpSize);
  if (err != ESP_OK || dumpSize == 0) {
    logPrintf("lvl=ERR tag=COREDUMP zdarzenie=image_get_fail err=%s rozmiar=%u\n",
              esp_err_to_name(err), (unsigned)dumpSize);
    return;
  }

  // Czytamy przez API partycji, a nie esp_flash_read() z absolutnego adresu.
  // esp_partition_read() prawidłowo obsługuje również partycje objęte
  // szyfrowaniem flash; surowy esp_flash_read() mógłby skopiować ciphertext.
  const esp_partition_t* corePart = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_COREDUMP, NULL);
  if (!corePart || flashAddr < corePart->address ||
      flashAddr + dumpSize > corePart->address + corePart->size) {
    logPrintf("lvl=ERR tag=COREDUMP zdarzenie=partition_invalid addr=0x%08lx size=%u\n",
              (unsigned long)flashAddr, (unsigned)dumpSize);
    return;
  }

  if (!LittleFS.exists(COREDUMP_DIR) && !LittleFS.mkdir(COREDUMP_DIR)) {
    logPrintf("lvl=ERR tag=COREDUMP zdarzenie=mkdir_fail\n");
    return;
  }

  uint32_t seq = coredumpAlokujSeq();
  if (seq == 0) {
    logPrintf("lvl=ERR tag=COREDUMP zdarzenie=seq_alloc_fail\n");
    return;
  }

  uint32_t bootId = coredumpAlokujBootId();
  if (bootId == 0) {
    logPrintf("lvl=ERR tag=COREDUMP zdarzenie=boot_id_alloc_fail seq=%lu\n",
              (unsigned long)seq);
    return;
  }

  char pathBuf[COREDUMP_PATH_MAX_LEN];
  if (!coredumpZbudujSciezke(seq, bootId, pathBuf, sizeof(pathBuf))) {
    logPrintf("lvl=ERR tag=COREDUMP zdarzenie=path_fail seq=%lu boot=%lu\n",
              (unsigned long)seq, (unsigned long)bootId);
    return;
  }
  String path(pathBuf);
  File f = LittleFS.open(path, "w");
  if (!f) {
    logPrintf("lvl=ERR tag=COREDUMP zdarzenie=open_fail plik=%s\n", path.c_str());
    return;
  }

  uint8_t buf[COREDUMP_COPY_CHUNK];
  size_t left = dumpSize;
  size_t partOffset = flashAddr - corePart->address;
  bool ok = true;
  while (left > 0) {
    size_t chunk = left > sizeof(buf) ? sizeof(buf) : left;
    err = esp_partition_read(corePart, partOffset, buf, chunk);
    if (err != ESP_OK) {
      logPrintf("lvl=ERR tag=COREDUMP zdarzenie=flash_read_fail err=%s\n", esp_err_to_name(err));
      ok = false;
      break;
    }
    size_t written = f.write(buf, chunk);
    if (written != chunk) {
      logPrintf("lvl=ERR tag=COREDUMP zdarzenie=write_partial oczekiwano=%u zapisano=%u\n",
                (unsigned)chunk, (unsigned)written);
      ok = false;
      break;
    }
    partOffset += chunk;
    left -= chunk;
    esp_task_wdt_reset();
  }
  f.close();

  if (!ok) {
    (void)LittleFS.remove(path);
    return;
  }

  File verify = LittleFS.open(path, "r");
  size_t finalSize = verify ? verify.size() : 0;
  if (verify) verify.close();
  if (finalSize != dumpSize) {
    (void)LittleFS.remove(path);
    logPrintf("lvl=ERR tag=COREDUMP zdarzenie=verify_size_fail plik=%s oczekiwano=%u zapisano=%u\n",
              path.c_str(), (unsigned)dumpSize, (unsigned)finalSize);
    return;
  }

  esp_err_t eraseErr = esp_core_dump_image_erase();
  if (eraseErr != ESP_OK) {
    // Plik jest zweryfikowany, więc nie tracimy go. Zostawiamy source dump, a przy kolejnym boocie
    // ewentualny duplikat dostanie nowe seq — bezpieczniej niż utrata diagnostyki.
    logPrintf("lvl=WARN tag=COREDUMP zdarzenie=source_erase_fail err=%s\n", esp_err_to_name(eraseErr));
  }

  g_lastArchivedCoredumpSize = dumpSize;
  g_lastArchivedCoredumpSeq = seq;
  logPrintf("lvl=ERR tag=COREDUMP zdarzenie=zarchiwizowano plik=%s boot=%lu seq=%lu rozmiar=%uB\n",
            path.c_str(), (unsigned long)bootId, (unsigned long)seq, (unsigned)dumpSize);
  coredumpPrzytnijArchiwum();
#else
  (void)powodResetu;
#endif
}

static String coredumpCrashMessage(uint32_t boot, uint32_t seq, size_t size) {
  String s;
  s.reserve(420);
  s += "💥 <b>RYBY CRASH</b>\n";
  s += "boot=" + String((unsigned long)boot) + "\n";
  s += "reset=" + String((int)bootResetReason) + "\n";
  s += "coredump_seq=" + String((unsigned long)seq) + "\n";
  s += "rozmiar=" + String((unsigned long)size) + " B\n";
  s += "Plik coredump zostanie dolaczony ponizej.";
  return s;
}

void setup() {
  logPrintln("lvl=INFO tag=FW version=" RYBY_FW_VERSION);

  // [4.1.0 OTA-GITHUB] Anty-rollback: jeśli poprzedni boot zakonczyl sie
  // aktualizacja OTA, potwierdz nowa partycje (bez tego kolejny restart
  // cofnalby firmware na stara partycje). Wersja biezaca do porownan semver.
  otaGithubConfirmPartition();
  otaGithubSetCurrentVersion(FW_VERSION);

  // =====================================================
  //  SERIAL + START
  // =====================================================
  // OPT-v44-A: ESP32-S3 Native USB CDC (jeśli port USB-OTG, nie UART/CH340)
  //   W Arduino IDE: Narzędzia -> USB CDC On Boot -> "Enabled"
  //                  Narzędzia -> USB Mode       -> "USB-OTG (TinyUSB)"
  //   Serial.begin() działa wtedy przez USB-OTG - wyższe prędkości, brak CH340.
  Serial.begin(115200);
  // [OK] POPRAWKA: Timeout 3s - bez tego ESP zawisa na wieki na urządzeniu bez kabla USB
  unsigned long _t = millis();
  while (!Serial && millis() - _t < 3000) { }
  
  // [OK] WATCHDOG - zrestartuje ESP jeśli zawiesi się na >8 sekund
  // S3 / ESP-IDF v5: nowe API watchdoga
  // [v101] FIX-1: timeout 10s → 15s.
  // TCP SYN backoff lwIP może trwać do ~14s przy niestabilnym WiFi (potwierdzone testem).
  // WiFiClientSecure.setTimeout(4) dotyczy tylko TLS read, NIE TCP connect().
  // Margines: TCP(max 7s) + TLS(4s) = 11s < 15s → 4s zapasu nawet przy wolnej sieci.
  // WiFi guard (fbConnect/tgEnsureConnected) chroni gdy WiFi offline (status!=WL_CONNECTED).
  const esp_task_wdt_config_t wdt_cfg = {
    .timeout_ms     = 15000,    // [v101] FIX-1: było 10000
    .idle_core_mask = 0,
    .trigger_panic  = true
  };
  // [v138] FIX-WDT-SILENT-FAIL: esp_task_wdt_init() z customowym configiem
  // ZAWSZE zwracał ESP_ERR_INVALID_STATE i nic nie robił, bo arduino-esp32 core
  // inicjalizuje TWDT automatycznie PRZED setup() (domyślnie
  // CONFIG_ESP_TASK_WDT_TIMEOUT_S=5s). Return value nigdy nie było sprawdzane,
  // więc od v101 "15s timeout" NIGDY realnie nie obowiązywał w produkcji -
  // firmware działał z domyślnym ~5s oknem TWDT, mimo że cała logika marginesów
  // (ITER_BUDGET_MS, timeouty FB/TG, itd.) zakładała 15s.
  // POTWIERDZONE TESTEM: esp_task_wdt_reconfigure() poprawnie ustawia nowy
  // timeout (zmierzony realny czas do restartu ~14.5-15.3s przy configu 15000ms,
  // zgodnie z oczekiwaniem) - dlatego jest tu użyty jako fallback.
  esp_err_t wdtInitRet = esp_task_wdt_init(&wdt_cfg);
  if (wdtInitRet == ESP_ERR_INVALID_STATE) {
    esp_err_t wdtRcRet = esp_task_wdt_reconfigure(&wdt_cfg);
    Serial.printf("lvl=INFO tag=WDT-FIX msg=\"TWDT juz aktywny, reconfigure 15000ms\" ret=%d err=%s\n",
                  (int)wdtRcRet, esp_err_to_name(wdtRcRet));
  } else if (wdtInitRet != ESP_OK) {
    Serial.printf("lvl=ERR tag=WDT-FIX msg=\"esp_task_wdt_init niespodziewany blad\" ret=%d err=%s\n",
                  (int)wdtInitRet, esp_err_to_name(wdtInitRet));
  }
  esp_task_wdt_add(NULL);      // Dodaj główny task

  // [v50] FIX-DFS: min_freq_mhz zmienione z 80 na 240 (DFS wyłączone).
  //   Przy min=80MHz: podczas oczekiwania na sieć CPU opada do 80MHz.
  //   mbedTLS AES + lwIP timers przy 80MHz mogą nie zmieścić się w oknie WDT 10s.
  //   Potwierdzono crashami o 01:16 (brak logu przez 29s) i 02:11 (TG-POLL blok 37s).
  //   Przy stałym WiFi oszczędność DFS jest marginalna - WiFi stack i tak trzyma 80MHz.
  {
    esp_pm_config_t pm_cfg = {
      .max_freq_mhz    = 240,
      .min_freq_mhz    = 240,   // [v50] FIX: było 80 - powodowało WDT przy TLS handshake
      .light_sleep_enable = false
    };
    if (esp_pm_configure(&pm_cfg) != ESP_OK) {
      logPrintln("lvl=WARN tag=DFS msg=\"esp_pm_configure failed, praca na stalych 240 MHz\"");
    }
  }
  
  // ── [FIX-v132-LOGBUF-EARLY] Alokacja buforów logów i mutexa ─────────────
  // MUSI być przed pierwszym logPrintln/logPrintf — bez tego logToFile() robi
  // early-return (if(!logBuf[idx])return) i PRE_RESET/bootResetReason giną cicho.
  // Poprzednia lokalizacja: linia ~12 535 (za WiFi/NTP/sensors) — za późno.
  // psramAllocSafe() nie wymaga żadnej wcześniejszej inicjalizacji.
  logMutex = xSemaphoreCreateMutex();
  if (!logMutex) Serial.println("lvl=ERR tag=LOG-MUTEX msg=\"Brak mutexa, logowanie bez synchronizacji\"");
  for (int _lbi = 0; _lbi < 2; _lbi++) {
    g_logBuf[_lbi] = (char*)psramAllocSafe(LOG_BUF_SIZE);
    if (g_logBuf[_lbi]) {
      g_logBuf[_lbi][0] = '\0';
      g_logBufLen[_lbi] = 0;
    } else {
      g_logBuf[_lbi] = (char*)malloc(2048);
      if (g_logBuf[_lbi]) { g_logBuf[_lbi][0] = '\0'; g_logBufLen[_lbi] = 0; }
      // brak else — logBuf[i]==nullptr → logToFile() pominie, Serial działa dalej
    }
  }
  // ─────────────────────────────────────────────────────────────────────────

  // [v218] LOGS-AI-STEP50: baner startowy (kategoria (c) z briefingu) - usunięto
  // 41 logPrintln z archiwalną historią wersji (v97-v170). To była czysto
  // archiwalna dokumentacja bez wartości zdarzeniowej dla parsera AI - pełna
  // historia i tak żyje w CHANGELOGU na górze pliku .cpp (źródło prawdy),
  // powielanie jej w runtime logu przy KAŻDYM boocie tylko zwiększało szum
  // (41 linii nie-zdarzeniowych na restart) bez informacji przydatnej do
  // debugowania konkretnego uruchomienia. Zgodne z zasadą canonical log line
  // (Stripe/brandur.org) - log ma nieść kontekst zdarzenia, nie statyczną
  // dokumentację. Baner spłaszczony do jednej linii startu boot + wersja +
  // WDT (dawne "Watchdog Timer: 10 sekund" złączone jako pole wdt=).
  // [v169] FIX-FW-VERSION-DRIFT: FW_VERSION (~linia 3942) to jedyne źródło
  // prawdy dla numeru wersji w tej linii - nie kopiować ręcznie.
  logPrintf("lvl=INFO tag=BOOT akcja=start version=%s wdt=10s\n", FW_VERSION);

  // [OK] FIX-6 / [FIX-v128-RESETREASON-LOG] Zachowaj przyczynę restartu ESP32.
  // Pełne logowanie (switch+logPrintf+/last_reset.txt+WDT warn) PRZENIESIONE
  // po LittleFS.begin() — tutaj littlefsReady==false → logToFile() early-return,
  // if(littlefsReady) zawsze false → zapis /last_reset.txt był martwym kodem.
  {
    bootResetReason = esp_reset_reason();  // [OK] FIX-v34b: zachowaj dla powiadomienia TG
  }

  // [FIX-v134-RTC-NOINIT] RTC_NOINIT_ATTR nie ma automatycznej inicjalizacji —
  // przy PIERWSZYM zasilaniu (lub po flash) pamięć RTC ma niezdefiniowaną wartość.
  // Zeruj jawnie TYLKO przy POWER_ON/UNKNOWN — przy WDT/PANIC/SW dane muszą przeżyć,
  // więc NIE wolno ich tu czyścić. Walidacja "magic" i tak chroniłaby przed odczytem
  // śmieci (1:4mld szansy na fałszywe trafienie), ale jawne zero jest tańsze niż ryzyko.
  // [FIX-v147-PRERESET-TIMING] Samo zerowanie zostaje TUTAJ (wczesne, nic go nie
  // wymaga później) — ale ODCZYT+DRUK black-boxa PRZENIESIONY niżej, patrz komentarz
  // przy LittleFS.begin(). Tu zerowanie jest bezpieczne: nie zależy od littlefsReady/
  // wsReady, a musi zdążyć PRZED ewentualnym odczytem niżej.
  if (bootResetReason == ESP_RST_POWERON || bootResetReason == ESP_RST_UNKNOWN) {
    memset(&g_preReset, 0, sizeof(g_preReset));
    memset(&g_wdtHunt,  0, sizeof(g_wdtHunt));
    g_tgLastUpdateId = -1;  // [FIX-v149-TG-OFFSET-RTC] realny power-on = brak historii offsetu
  }
  // [FIX-v149-TG-OFFSET-RTC] Przy SW/WDT/PANIC g_tgLastUpdateId przetrwał restart
  // w RTC RAM (powyższy blok go nie tknął) — synchronizujemy zmienną roboczą RAM
  // (zerowaną przy każdym restarcie) z wartością z RTC RAM, ZANIM pollTelegramCommands()
  // zdąży ją użyć. Dla POWER_ON obie strony są już -1, więc przypisanie jest no-op.
  tgLastUpdateId = g_tgLastUpdateId;

  // =====================================================
  //  SPRZĘT
  // =====================================================
  sensors1.begin();
  sensors2.begin();
  sensors3.begin();

  // ── [v67-Z3] TLS-MUTEX — serializacja handshake'ów TG i FB ───────────────
  tlsMutex = xSemaphoreCreateMutex();
  if (!tlsMutex) {
    logPrintln("lvl=WARN tag=TLS-MUTEX msg=\"Nie udalo sie utworzyc mutexa TLS, handshake'i nieserializowane\"");
  }

  // ── [v69-Z0/Z4] DS-DIAG — aktywna diagnostyka czujnika wody przy starcie ──
  // Z0: INPUT_PULLDOWN zamiast INPUT — deterministyczny odczyt bez zewnętrznego pull-up.
  //     Zewnętrzny pull-up 4.7kΩ jest silniejszy niż wewnętrzny ~45kΩ → HIGH (pull-up istnieje).
  //     Brak pull-up → wewnętrzny pull-down wygrywa → LOW (deterministyczne, nie floating).
  //     GPIO39 na ESP32-S3 obsługuje INPUT_PULLDOWN (w odróżnieniu od klasycznego ESP32).
  // Z4: reset pulse + ROM scan dla pełnej diagnozy.
  {
    logPrintf("lvl=INFO tag=DS-DIAG msg=\"Start diagnozy DS18B20 wody\" pin=GPIO%d\n", ONE_WIRE_BUS3);
    int ds3Devices = sensors3.getDeviceCount();

    // [v69-Z0] INPUT_PULLDOWN: deterministyczny odczyt napięcia na DATA
    pinMode(ONE_WIRE_BUS3, INPUT_PULLDOWN);
    int ds3PinLevel = digitalRead(ONE_WIRE_BUS3);
    g_ds3PinLevel = (int8_t)ds3PinLevel;  // [v69-Z0] przekaż do tgTaskFn przez zmienną globalną
    // OneWire przywróci pin do właściwego trybu przy requestTemperatures()
    logPrintf("lvl=INFO tag=DS-DIAG pin=GPIO%d voltage=%s devices=%d\n",
              ONE_WIRE_BUS3,
              ds3PinLevel == HIGH ? "HIGH (pull-up OK)" : "LOW (brak pull-up lub zwarcie!)",
              ds3Devices);

    // [v69-Z4] Test reset pulse — urządzenie odpowiada presence pulse?
    bool ds3ResetOk = oneWire3.reset();
    g_ds3ResetOk = ds3ResetOk;
    logPrintf("lvl=INFO tag=DS-DIAG reset_pulse=%s\n",
              ds3ResetOk ? "OK (urzadzenie odpowiedzialo)" : "BRAK (brak urzadzenia lub zwarcie)");

    // [v69-Z4] ROM scan — odczyt adresu 64-bit
    DeviceAddress ds3Addr;
    bool ds3AddrOk = sensors3.getAddress(ds3Addr, 0);
    g_ds3AddrOk = ds3AddrOk;
    if (ds3AddrOk) {
      logPrintf("lvl=INFO tag=DS-DIAG rom=%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
                ds3Addr[0], ds3Addr[1], ds3Addr[2], ds3Addr[3],
                ds3Addr[4], ds3Addr[5], ds3Addr[6], ds3Addr[7]);
    } else {
      logPrintf("lvl=INFO tag=DS-DIAG rom=BRAK getAddress=false msg=\"CRC error lub brak urzadzenia\"\n");
    }

    bool ds3Ok = false;
    for (int attempt = 1; attempt <= 3; attempt++) {
      esp_task_wdt_reset();  // [v94-FIX-WDT] 3 próby x ~1.25s = do 3.75s bez karmienia WDT
      sensors3.setWaitForConversion(true);
      sensors3.requestTemperatures();
      float t = sensors3.getTempCByIndex(0);
      bool valid = (t != -127.0f && t != 85.0f && t >= -10.0f && t <= 90.0f);
      logPrintf("lvl=INFO tag=DS-DIAG pin=GPIO%d voltage=%s devices=%d attempt=%d/3 t=%.1fC wynik=%s\n",
                ONE_WIRE_BUS3,
                ds3PinLevel == HIGH ? "HIGH" : "LOW",
                ds3Devices, attempt, t, valid ? "OK" : "FAIL");
      if (valid) { ds3Ok = true; break; }
      esp_task_wdt_reset();  // przed delay — delay(500) nie karmi WDT
      delay(500);
    }
    sensors3.setWaitForConversion(false); // przywróć tryb nieblokujący
    if (!ds3Ok) {
      ds3BootFail = true;
      logPrintf("lvl=ERR tag=DS-DIAG msg=\"FAIL - czujnik wody nie odpowiada\" pin=GPIO%d poziom=%s reset_pulse=%s (Sprawdz: kabel DATA, pull-up 4.7kOhm przy VCC 3.3V, zasilanie VCC 3.3V/5V pasozytnicze -> alert TG po polaczeniu WiFi)\n",
                ONE_WIRE_BUS3,
                ds3PinLevel == HIGH ? "HIGH (zasilanie OK, problem z czujnikiem)" : "LOW (sprawdz pull-up i okablowanie!)",
                ds3ResetOk ? "OK" : "BRAK");
    } else {
      logPrintf("lvl=INFO tag=DS-DIAG msg=\"DS18B20 wody OK\"\n");
    }
  }

  // ── [FIX-v132-DS18B20-ROMADDR] Dump ROM adresów czujników Płyta1 i Płyta2 ──
  // DIAG-20 trend rosnący (18→29→65 KOLIZJA? eventów). Bez logowania ROM
  // nie można odróżnić kolizji 1-Wire od termicznego wyrównania temperatur.
  // Adresy drukowane raz przy starcie — sprawdź log po deployu.
  {
    DeviceAddress ds1Addr, ds2Addr;
    bool ds1Ok = sensors1.getAddress(ds1Addr, 0);
    bool ds2Ok = sensors2.getAddress(ds2Addr, 0);
    if (ds1Ok)
      logPrintf("lvl=INFO tag=DS-ADDR plyta=1 rom=%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
                ds1Addr[0], ds1Addr[1], ds1Addr[2], ds1Addr[3],
                ds1Addr[4], ds1Addr[5], ds1Addr[6], ds1Addr[7]);
    else
      logPrintf("lvl=INFO tag=DS-ADDR plyta=1 rom=BRAK msg=\"CRC error lub brak urzadzenia\"\n");
    if (ds2Ok)
      logPrintf("lvl=INFO tag=DS-ADDR plyta=2 rom=%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
                ds2Addr[0], ds2Addr[1], ds2Addr[2], ds2Addr[3],
                ds2Addr[4], ds2Addr[5], ds2Addr[6], ds2Addr[7]);
    else
      logPrintf("lvl=INFO tag=DS-ADDR plyta=2 rom=BRAK msg=\"CRC error lub brak urzadzenia\"\n");
    if (ds1Ok && ds2Ok) {
      bool collision = (memcmp(ds1Addr, ds2Addr, 8) == 0);
      logPrintf("lvl=%s tag=DS-ADDR rom1_vs_rom2=%s msg=\"%s\"\n",
                collision ? "WARN" : "INFO",
                collision ? "IDENTYCZNE" : "rozne",
                collision ? "KOLIZJA 1-WIRE POTWIERDZONA" : "OK");
    }
  }
  // ──────────────────────────────────────────────────────────────────────────

  pinMode(pumpPin, OUTPUT);
  pinMode(ledSupplyPin, OUTPUT);
  pinMode(cameraPin, OUTPUT);
  digitalWrite(pumpPin, HIGH);       // aktywny LOW: HIGH = wyłączona (stan bezpieczny)
  digitalWrite(ledSupplyPin, HIGH);  // aktywny LOW: HIGH = wyłączony (stan bezpieczny)
  digitalWrite(cameraPin, LOW);

  for (int i = 0; i < 5; i++) {
    ledcAttach(pinyLED[i], 25000, 10);  // S3 core v3: pin-based API
  }

  // =====================================================
  //  EEPROM
  // =====================================================
  EEPROM.begin(EEPROM_SIZE);
  fbLoadPersistentState();  // [v241] nie powtarzaj starych komend/config po restarcie

  loadAutoBrightnessFromEEPROM();
  loadBackupBrightnessFromEEPROM();  // FIX-v70-B: rzeczywista jasność sprzed restartu
  loadFadeFromEEPROM();
  loadScheduleFromEEPROM();
  loadPumpScheduleFromEEPROM();
  loadMinLuxModeFromEEPROM();
  loadKwhPrice();
  odczytajAdaptacjeZEEPROM();
  loadPowerTrybFromEEPROM();
  prevPowerGlobal = power;
  prevTrybGlobal = tryb;
  loadChangeCounterFromEEPROM();  // ← wczytaj licznik wersji
  loadLogModeFromEEPROM();
  loadAdaptiveSettingsFromEEPROM();
  loadEmaFilterFromEEPROM();  // [OK] Wczytaj współczynnik filtra EMA
  loadSensIntFromEEPROM();    // [OK] Wczytaj interwał odczytu czujnika
  loadRampSecFromEEPROM();    // [OK] Wczytaj czas rampy PWM
  if (Komentarze) {
    char _eepromSekBuf[128] = "";
    for (int i = 0; i < 5; i++) {
      char _tmp[24];
      snprintf(_tmp, sizeof(_tmp), "sek%d=%u(%.0f%%) ", i,
               getBrightnessForSection(backupAutoBrightnessComposite, i),
               (getBrightnessForSection(backupAutoBrightnessComposite, i) / 1023.0f) * 100.0f);
      strncat(_eepromSekBuf, _tmp, sizeof(_eepromSekBuf) - strlen(_eepromSekBuf) - 1);
    }
    logPrintf(
      "lvl=INFO tag=EEPROM-READOUT %sraw=0x%016llX rampaMin=%d ranoRobocze=%04dmin ranoWeekend=%04dmin poludnieOff=%04dmin wieczorOff=%04dmin\n",
      _eepromSekBuf,
      backupAutoBrightnessComposite,
      fadeMinutes,
      MORNING_ON_START_WEEKDAY,
      MORNING_ON_START_WEEKEND,
      MIDDAY_OFF_LOCAL,
      EVENING_OFF_START
    );
  }

  logPrintln("lvl=INFO tag=SETUP msg=\"Konfiguracja sprzetu zakonczona\"");

// =====================================================
  //  [DIR] LITTLEFS - SYSTEM PLIKÓW
  // =====================================================
  logPrintln("lvl=INFO tag=DIR msg=\"Inicjalizacja LittleFS\"");
  if (!LittleFS.begin(false)) {
    // Pierwsze uruchomienie lub uszkodzenie - spróbuj jednorazowo sformatować
    logPrintln("lvl=WARN tag=DIR msg=\"LittleFS nie zamontowany, proba formatowania\"");
    if (!LittleFS.begin(true)) {
      logPrintln("lvl=ERR tag=DIR msg=\"LittleFS blad inicjalizacji\"");
      littlefsReady = false;
    } else {
      logPrintln("lvl=INFO tag=DIR msg=\"LittleFS sformatowany i gotowy (pierwsze uruchomienie)\"");
      littlefsReady = true;
    }
  } else {
    logPrintln("lvl=INFO tag=DIR msg=\"LittleFS OK, dane zachowane\"");
    littlefsReady = true;
  }
  if (littlefsReady) {
    // [v152] FIX-FS-CAPACITY: odczytaj PRAWDZIWY rozmiar wolumenu zamiast polegać
    // na zaszytej na sztywno wartości 10420224 B (błędnej od v113 - zmiana partycji coredump o 65536 B).
    {
      size_t realTotal = LittleFS.totalBytes();
      if (realTotal > 0 && realTotal != g_fsTotalBytes) {
        logPrintf("lvl=WARN tag=FS-CAPACITY msg=\"Rzeczywisty rozmiar LittleFS rozni sie od zalozonego, uzywam rzeczywistego\" rzeczywisty=%uB zalozony=%uB\n",
                  (unsigned)realTotal, (unsigned)g_fsTotalBytes);
      }
      if (realTotal > 0) g_fsTotalBytes = realTotal;
    }
    loadTelegramConfig();  // 📨 Wczytaj TG po LittleFS (poprawna kolejnosc)
    logToFile("=== SYSTEM START ===");
    // [FIX-v128-RESETREASON-LOG] Logowanie przyczyny restartu PO LittleFS.begin()
    // — teraz littlefsReady==true → trafia do log_b.txt I do /last_reset.txt.
    {
      const char* rstStr = "NIEZNANY";
      switch (bootResetReason) {
        case ESP_RST_POWERON:   rstStr = "POWER_ON";        break;
        case ESP_RST_EXT:       rstStr = "EXTERNAL_PIN";    break;
        case ESP_RST_SW:        rstStr = "SOFTWARE";        break;
        case ESP_RST_PANIC:     rstStr = "PANIC/CRASH";     break;
        case ESP_RST_INT_WDT:   rstStr = "WDT_INTERRUPT";   break;
        case ESP_RST_TASK_WDT:  rstStr = "WDT_TASK";        break;
        case ESP_RST_WDT:       rstStr = "WDT_OTHER";       break;
        case ESP_RST_DEEPSLEEP: rstStr = "DEEP_SLEEP";      break;
        case ESP_RST_BROWNOUT:  rstStr = "BROWNOUT";        break;
        case ESP_RST_SDIO:      rstStr = "SDIO";            break;
        default: break;
      }
      logPrintf("lvl=INFO tag=RESTART przyczyna=%s kod=%d\n", rstStr, (int)bootResetReason);
      File rf = LittleFS.open("/last_reset.txt", "w");  // [v64-P2a] teraz działa
      if (rf) {
        rf.printf("reason=%s(%d) uptime_prev=%lus\n",
                  rstStr, (int)bootResetReason, millis() / 1000UL);
        rf.close();
      }
      if (bootResetReason == ESP_RST_TASK_WDT || bootResetReason == ESP_RST_INT_WDT
          || bootResetReason == ESP_RST_WDT) {
        logPrintln("lvl=WARN tag=WDT msg=\"Glowna petla byla zablokowana >10s (WebSocket, derating lub inna dluga operacja)\"");
        logPrintln("lvl=WARN tag=WDT msg=\"Sprawdz logi tuz przed restartem, poszukaj dlugich operacji sieciowych\"");
      } else if (bootResetReason == ESP_RST_PANIC) {
        logPrintln("lvl=WARN tag=PANIC msg=\"Sprawdz blok COREDUMP kilka linii nizej (jesli partycja miala zapisany zrzut), konkretny task/PC/backtrace zamiast zgadywania\"");
      } else if (bootResetReason == ESP_RST_EXT) {
        logPrintln("lvl=INFO tag=RESTART msg=\"reset przyciskiem lub zewnetrzny, normalny\"");
      }
    }

    // [FIX-v147-PRERESET-TIMING] Odczyt PRE-RESET BLACK BOX PRZENIESIONY tutaj.
    // ROOT CAUSE: blok był wcześniej w setup() PRZED LittleFS.begin() (linia ~10438)
    // i PRZED startem serwera WWW/WebSocket. W tym momencie logPrintln/logPrintf mają:
    //   - littlefsReady == false -> logToFile() robi early-return (if(!littlefsReady) return;)
    //   - wsReady == false        -> gałąź WiFi Terminal w logPrintln/logPrintf pomijana
    // Jedyny kanał, który wtedy faktycznie działał, to Serial (LOG_BOTH domyślnie) —
    // czyli linia "📦 [PRE-RESET]" (i wariant "Brak danych") leciała WYŁĄCZNIE na USB,
    // nigdy do log_b.txt ani do zakładki Logi/WiFi Terminal. Stąd w eksportowanych
    // logach (log_combined.txt) ta linia nie pojawiała się ANI RAZU — niezależnie od
    // tego, czy dane w RTC RAM były poprawne (fix v134 to osobno naprawił). To był
    // drugi, niezależny bug na tej samej ścieżce: dane były już dobre od v134, ale
    // print wciąż trafiał w martwy kanał.
    // FIX: przeniesiono odczyt+druk tutaj (littlefsReady już == true, logToFile()
    // działa) — teraz [PRE-RESET] i ewentualny [WDT-HUNT] trafią do log_b.txt.
    // g_preReset/g_wdtHunt to RTC_NOINIT_ATTR — dane przetrwały bez zmian te ~250
    // linii kodu między starym a nowym miejscem odczytu (nic ich po drodze nie rusza).
    if (g_preReset.magic == PRE_RESET_MAGIC) {
      // [v169] FIX-OTA-SRC-MISLABEL: dodano "OTA" jako src=4. Wcześniej restart
      // po zakończonym OTA (ArduinoOTA.onEnd() -> restart wewnątrz biblioteki)
      // nigdy nie wołał PRE_RESET_UPDATE(...) ani nie ustawiał g_restartPending —
      // ostatni zapis do g_preReset przed takim restartem pochodził więc z
      // periodycznego heartbeatu tgTaskFn (PRE_RESET_UPDATE(2), co 2s), który
      // zawsze wpisuje src=2="WDT(brak danych)". Efekt potwierdzony w logu
      // z urządzenia: restart poprawnie rozpoznany jako SOFTWARE(3) przy starcie
      // (bootResetReason, patrz "🔄 RESTART: przyczyna="), ale ten sam wpis
      // [PRE-RESET] dla POPRZEDNIEGO cyklu mylnie pokazywał "src=WDT(brak danych)"
      // — czyli zwykła, zamierzona aktualizacja OTA wyglądała w logu jak crash.
      // Fix: ArduinoOTA.onEnd() woła teraz g_restartPending=true + PRE_RESET_UPDATE(4)
      // (patrz tam), więc kolejny odczyt poprawnie pokaże src=OTA.
      const char* srcStr[] = {"HTTP/soft", "DIAG-17", "WDT(brak danych)", "PANIC(brak danych)", "OTA"};
      uint8_t src = g_preReset.resetSrc < 5 ? g_preReset.resetSrc : 3;
      logPrintf("lvl=INFO tag=PRE-RESET uptime=%us src=%s freeH=%uB maxAlloc=%uB psramUsed=%uB wifi=%u fbReady=%u tgReady=%u checkpoint=%s\n",
                g_preReset.uptime_s, srcStr[src],
                g_preReset.freeHeap, g_preReset.maxAlloc, g_preReset.psramUsed,
                g_preReset.wifiState, g_preReset.fbReady, g_preReset.tgReady,
                g_preReset.checkpoint[0] ? g_preReset.checkpoint : "BRAK");
      // [FIX-v133-WDT-HUNT] Wykryj powtarzający się crash w tym samym checkpoincie.
      // Tylko przy potwierdzonym WDT/PANIC (bootResetReason) — normalny restart
      // (HTTP/DIAG-17/POWER_ON) nie rusza streak. Drukuje się TYLKO gdy streak>=2,
      // czyli pierwszy crash w danym miejscu nie dodaje żadnej linii do logu.
      {
        bool _isCrashReset = (bootResetReason == ESP_RST_TASK_WDT ||
                               bootResetReason == ESP_RST_INT_WDT ||
                               bootResetReason == ESP_RST_WDT     ||
                               bootResetReason == ESP_RST_PANIC);
        if (_isCrashReset) {
          bool _samecp = (g_wdtHunt.magic == WDT_HUNT_MAGIC) &&
                          (strncmp(g_wdtHunt.lastCp, g_preReset.checkpoint,
                                   sizeof(g_wdtHunt.lastCp)) == 0);
          g_wdtHunt.streak = _samecp ? (uint16_t)(g_wdtHunt.streak + 1) : (uint16_t)1;
          strncpy(g_wdtHunt.lastCp, g_preReset.checkpoint, sizeof(g_wdtHunt.lastCp) - 1);
          g_wdtHunt.lastCp[sizeof(g_wdtHunt.lastCp) - 1] = '\0';
          g_wdtHunt.magic = WDT_HUNT_MAGIC;
          if (g_wdtHunt.streak >= 2) {
            logPrintf("lvl=WARN tag=WDT-HUNT msg=\"powtarzajacy sie crash w tym samym miejscu\" streak=%u checkpoint=%s\n",
                      (unsigned)g_wdtHunt.streak, g_wdtHunt.lastCp);
          }
        }
      }
      g_preReset.magic = 0;  // skasuj po odczycie
    } else {
      logPrintln("lvl=INFO tag=PRE-RESET msg=\"brak danych, power-on lub pierwsza wersja z black boxem\"");
    }

    // [v171] FEATURE-COREDUMP-SUMMARY: pkt 3 z RYBY_LED_dalsze_kroki_v169.md.
    // Partycja "coredump" (64 KB, patrz partitions.csv) istnieje od [v113], ale
    // do tej pory nic jej nie odczytywało. Ten blok czyta ją PROGRAMOWO, zaraz
    // obok [PRE-RESET]/[WDT-HUNT] — to ta sama rodzina pytań ("co się działo
    // tuż przed crashem"), tylko z dużo większą szczegółowością (nazwa tasku +
    // PC + backtrace zamiast ogólnego "Stack overflow lub null pointer").
#if CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH
    {
      esp_err_t _cdErr = esp_core_dump_image_check();
      if (_cdErr == ESP_OK) {
        esp_core_dump_summary_t _cdSum;
        if (esp_core_dump_get_summary(&_cdSum) == ESP_OK) {
          char _cdTask[17];
          strncpy(_cdTask, _cdSum.exc_task, 16);
          _cdTask[16] = '\0';
          logPrintf("lvl=ERR tag=COREDUMP task=%s pc=0x%08X depth=%u corrupted=%s\n",
                    _cdTask, (unsigned)_cdSum.exc_pc,
                    (unsigned)_cdSum.exc_bt_info.depth,
                    _cdSum.exc_bt_info.corrupted ? "tak" : "nie");
          String _cdBt = "lvl=ERR tag=COREDUMP msg=\"backtrace\" adresy=\"";
          for (uint32_t _i = 0; _i < _cdSum.exc_bt_info.depth && _i < 16; _i++) {
            char _pcbuf[12];
            snprintf(_pcbuf, sizeof(_pcbuf), " 0x%08X", (unsigned)_cdSum.exc_bt_info.bt[_i]);
            _cdBt += _pcbuf;
          }
          _cdBt += "\"";
          logPrintln(_cdBt);
          logPrintln("lvl=INFO tag=COREDUMP msg=\"Dekodowanie\" polecenie=\"xtensa-esp32s3-elf-addr2line -e firmware.elf <adresy_wyzej>\"");
        } else {
          logPrintln("lvl=ERR tag=COREDUMP msg=\"Obraz w partycji OK, ale odczyt summary zawiodl\"");
        }

        // [v240] Najpierw zarchiwizuj RAW obraz. Funkcja sama wykona erase
        // zrodla dopiero po poprawnym zapisaniu i zweryfikowaniu pliku.
        archiwizujCoredumpJesliCrash(bootResetReason);
      } else if (_cdErr != ESP_ERR_NOT_FOUND) {
        logPrintf("lvl=ERR tag=COREDUMP msg=\"Obraz uszkodzony lub nieodczytywalny\" err=%d\n", (int)_cdErr);
        (void)esp_core_dump_image_erase();
      }
    }
#else
    logPrintln("lvl=INFO tag=COREDUMP msg=\"CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH niedostepne, pomijam odczyt\"");
#endif

    // [OK] FIX-v29-NTP: loadEnergyStats() przeniesione ZA synchronizację NTP.
    // Poprzednio wywoływane tutaj (przed WiFi.begin/NTP) -> getLocalTimePL() zwracało false
    // -> funkcja robiła early return -> wszystkie liczniki zostawały na 0 (kasowane przy restarcie).
    // loadEnergyStats() wywołujemy teraz po udanym NTP, w sekcji WiFi+NTP poniżej.
    // [OK] FIX CRASH-LOOP: loadAdaptStats() przeniesione za WiFi.begin() - wczytywanie
    // adapt_stats.json PRZED WiFi.begin() powoduje deterministyczny IntegerDivideByZero
    // podczas inicjalizacji stosu WiFi (prawdopodobnie przez fragmentację heapu po operacjach
    // na String z LittleFS, co korumpuje wewnętrzny stan dzielenia w bibliotece WiFi).
    // Statystyki adaptacji nie są potrzebne PRZED połączeniem z WiFi - używane tylko w loop().
    // [OK] ULT-B: Wstępna alokacja bufora logów - eliminuje fragmentację heapu przy logowaniu
    // (reserve() na static String w logToFile() nie jest możliwe z zewnątrz, więc logujemy
    //  długi ciąg inicjalizujący bufor od razu; drugi sposób to flag w logToFile - zob. poniżej)
  }
  // [OK] ULT-B: Sierota history_tmp.csv po crashu - wyczyść przy starcie
  // [v170] FIX-ORPHAN-REMOVE-UNCHECKED: littlefs potrafi udokumentowanie odmówić
  // usunięcia pliku, gdy dysk jest bliski pełnego - usunięcie też wymaga wolnych
  // bloków na commit zmiany w metadanych (littlefs-project/littlefs#1007
  // "LFS_ERR_NOSPC po udanym lfs_remove()"; #545 zaleca rezerwę ≥8 wolnych bloków
  // blisko pełnego dysku). To dokładnie scenariusz, w którym te sieroty realnie
  // powstają - czyli tam, gdzie najbardziej liczy się, żeby sprzątanie faktycznie
  // zadziałało, poprzednia wersja logowała "Usunięto" NIEZALEŻNIE od wyniku
  // remove(). FIX: sprawdzamy wynik + logujemy rozmiar pliku PRZED usunięciem, żeby
  // dało się odróżnić "faktycznie sprzątnięte" od "zostało, bo brak wolnych bloków".
  if (littlefsReady && LittleFS.exists("/history_tmp.csv")) {
    File _of = LittleFS.open("/history_tmp.csv", "r");
    size_t _osz = _of ? _of.size() : 0;
    if (_of) _of.close();
    bool _ok = LittleFS.remove("/history_tmp.csv");
    logPrintf("lvl=WARN tag=SETUP-ORPHAN plik=history_tmp.csv bajtow=%u remove=%s\n",
              (unsigned)_osz, _ok ? "true" : "false");
  }
  // [v140] FIX-WIFI-ATOMIC-SAVE: sierota po zapisie listy WiFi przerwanym crashem.
  // WIFI_LIST_OLD_FILE obecny = poprzedni rename() (tmp->docelowy) nie zdążył się
  // dokończyć PRZED restartem, ale docelowy plik już powinien być kompletny (rename
  // jest operacją atomową w LittleFS) - więc tylko sprzątamy, nic nie odtwarzamy.
  if (littlefsReady && LittleFS.exists(WIFI_LIST_TMP_FILE)) {
    File _of = LittleFS.open(WIFI_LIST_TMP_FILE, "r");
    size_t _osz = _of ? _of.size() : 0;
    if (_of) _of.close();
    bool _ok = LittleFS.remove(WIFI_LIST_TMP_FILE);
    logPrintf("lvl=WARN tag=SETUP-ORPHAN plik=wifi_list_tmp.txt bajtow=%u remove=%s\n",
              (unsigned)_osz, _ok ? "true" : "false");
  }
  if (littlefsReady && LittleFS.exists(WIFI_LIST_OLD_FILE)) {
    File _of = LittleFS.open(WIFI_LIST_OLD_FILE, "r");
    size_t _osz = _of ? _of.size() : 0;
    if (_of) _of.close();
    bool _ok = LittleFS.remove(WIFI_LIST_OLD_FILE);
    logPrintf("lvl=WARN tag=SETUP-ORPHAN plik=wifi_list_old.txt bajtow=%u remove=%s\n",
              (unsigned)_osz, _ok ? "true" : "false");
  }

  // [v170] FIX-BOOT-LOGFLUSH: analiza aquarium_log.txt (RYBY_LED_dalsze_kroki_v169.md,
  // pkt 1) znalazła policzalny dowód: linie zalogowane w bloku wyżej (FS-CAPACITY,
  // SYSTEM START, przyczyna restartu, diagnoza PANIC, [PRE-RESET]/[WDT-HUNT], sieroty)
  // fizycznie lądują W PLIKU PO liniach zalogowanych ~10s później (np. "TG-TASK
  // START"), mimo że chronologicznie były pierwsze. ROOT CAUSE: setup()/loop() działają
  // na Core 1 -> logToFile() idzie do g_logBuf[1] i (FIX-v46) NIGDY nie pisze
  // bezpośrednio, tylko ustawia logFlushPending[1]=true - jedynym konsumentem tej flagi
  // jest tgTaskFn (Core 0), a ten startuje dopiero xTaskCreatePinnedToCore(tgTaskFn...)
  // niżej, po sekcji WIFI+NTP (czeka na WiFi, ~10s po boot). Cały blok powyżej nie ma
  // więc, w tym momencie, żadnego konsumenta swojego bufora.
  // FIX: wymuszony, jednorazowy flush g_logBuf[1] TUTAJ - bezpieczne, bo logMutex już
  // istnieje (utworzony wyżej), a tgTaskFn JESZCZE NIE ISTNIEJE (dopiero zostanie
  // utworzony niżej) -> zero ryzyka wyścigu o logMutex/g_logBuf[1] w tym miejscu.
  // Ten sam wzorzec (mutex + logFlushCore1() + zerowanie flagi) co przy zwykłej
  // obsłudze logFlushPending[1] w tgTaskFn (~linia 10268) - nie duplikuje logiki,
  // tylko woła ją wcześniej, zanim ktokolwiek inny zdąży się o bufor ubiegać.
  if (littlefsReady) {
    if (logMutex && xSemaphoreTake(logMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
      logFlushCore1();
      logFlushPending[1] = false;
      xSemaphoreGive(logMutex);
    }
  }

// =====================================================
//  WIFI + NTP
// =====================================================

// Wyłącz Bluetooth (jeśli nie używasz) - stabilizuje WiFi
btStop();

WiFi.mode(WIFI_STA);          // Ustaw tryb klienta
WiFi.disconnect(true);        // Wyczyść stare sesje
delay(200);

WiFi.persistent(false);       // Nie zapisuj do flash
WiFi.setAutoReconnect(true);  // Auto reconnect
WiFi.setHostname("ryby-led-s3");  // [v98-OTA] mDNS hostname — wymagany przez espota

// [v139] FEATURE-WIFI-MULTI: wczytaj zapisane sieci PRZED pierwszą próbą
// połączenia (musi być po LittleFS.begin(), stąd tutaj - littlefsReady
// jest już ustawione wcześniej w setup()).
loadWifiNetworks();
wifiBeginBest();
logPrintln("lvl=INFO tag=WIFI-CONNECT akcja=start");

// [OK] FIX-v52-WDT-BOOT: retry < 20 * 500ms = 10s = granica WDT timeout!
// Zmniejszono do 15 prób (7.5s) + esp_task_wdt_reset() przed delay()
// żeby reset WDT następował co iterację, nie tylko po delay().
// Efekt: bezpieczny margines 2.5s poniżej limitu WDT nawet przy wolnym WiFi.
{
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 15) {
    esp_task_wdt_reset();  // reset PRZED delay() - maksymalny margines
    delay(500);
    retry++;
  }
}

// [v129] FIX-WIFI-RETRY-2: "druga próba" gdy pierwsza (7.5s) się nie powiodła.
// Znane zachowanie ESP32 (arduino-esp32 #2501): po serii szybkich restartów
// WiFi.begin() czasem nie łączy przy 1. próbie. disconnect(false) NIE zatrzymuje
// stosu WiFi (w przeciwieństwie do disconnect(true) użytego wcześniej) - tu jest
// to bezpieczne, bo stos już działa, tylko sesja/handshake się nie nawiązał.
// WDT bezpieczny: esp_task_wdt_reset() w każdej iteracji obu pętli (limit 15s).
if (WiFi.status() != WL_CONNECTED) {
  logPrintln("lvl=WARN tag=BOOT msg=\"1. proba WiFi nieudana (7.5s), druga proba\"");
  WiFi.disconnect(false);
  delay(100);
  wifiBeginBest();  // [v139] ponowny wybór najlepszej znanej sieci (skan może się zmienić)
  int retry2 = 0;
  while (WiFi.status() != WL_CONNECTED && retry2 < 15) {
    esp_task_wdt_reset();
    delay(500);
    retry2++;
  }
}

if (WiFi.status() == WL_CONNECTED) {
  // [v113-FIX-C] Wyłącz modem-sleep WiFi — zapobiega WDT crash Crash C (ppTask/
  // pm_tbtt_process/pm_coex_tbtt_process, CPU0, ~3% resetów).
  // Zdekodowany backtrace: ppTask → pm_tbtt_process → ppCheckTxConnTrafficIdle.
  // Mechanizm: ppTask (wysoki priorytet) zajmował CPU0 na tyle długo, że
  // tgTask (niższy priorytet, CPU0) nie resetował WDT → timeout po 10s.
  // WiFi.setSleep(false) = wyłącza IEEE 802.11 power-save / modem-sleep,
  // eliminuje pm_tbtt_process/TBTT housekeeping z CPU0.
  // Koszt: ~20–30 mA wyższe zużycie prądu (nieistotne przy zasilaniu 5V/2A).
  // Ref: espressif/esp-idf#6744 (identyczny wzorzec ppTask/WDT w społeczności).
  WiFi.setSleep(false);
  logPrintln("lvl=INFO tag=BOOT msg=\"Polaczono z WiFi\" ip=" + WiFi.localIP().toString());
} else {
  logPrintln("lvl=WARN tag=BOOT msg=\"WiFi timeout, brak sieci, kontynuuje bez WiFi\"");
}

// [OK] FIX CRASH-LOOP: loadAdaptStats() wczytywane TUTAJ (po WiFi wait-loop), nie przed WiFi.begin().
// Wczytywanie adapt_stats.json (z String + LittleFS) przed WiFi.begin() powodowało
// deterministyczny IntegerDivideByZero podczas inicjalizacji stosu WiFi (EXCCAUSE=6, PC=0x40131251).
// Statystyki adaptacji nie są wymagane przed połączeniem z WiFi.
if (littlefsReady) {
  loadAdaptStats();
}

// ── [v129] FIX-PSRAM-TLS-CRASHLOOP: hook mbedTLS przeniesiony tutaj z wnętrza
// bloku if(WiFi.status()==WL_CONNECTED) (był wewnątrz sekcji webserialServer/OTA).
// ROOT CAUSE crashloopa 2026-06-22 20:51–20:58: gdy WiFi nie połączyło się w 7.5s
// (po serii szybkich SW_CPU_RESET), CAŁY blok if(WiFi) był pomijany razem z hookiem
// mbedtls_platform_set_calloc_free() → pierwszy TLS handshake alokował ~44 kB z DRAM
// zamiast PSRAM → heap spadał do ~80 kB → kolejny crash → kolejny SW_CPU_RESET → pętla.
// NAPRAWA: hook nie zależy od WiFi — psramFound() sprawdza tylko flagę bootloadera
// PSRAM (stała od pierwszych ms boot), a mbedtls_psram_calloc() ma własny fallback
// do DRAM gdyby PSRAM był niedostępny (patrz definicja funkcji, ~linia 3635).
// WiFi/DMA używa MALLOC_CAP_DMA i ignoruje ten hook (patrz komentarz ~linia 3633)
// → przeniesienie nie niesie żadnego ryzyka dla stosu WiFi.
// ────────────────────────────────────────────────────────────────
if (psramFound()) {
    logPrintf("lvl=INFO tag=PSRAM sizeKB=%u freeKB=%u\n",
        (unsigned)(ESP.getPsramSize() / 1024),
        (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));

    mbedtls_platform_set_calloc_free(mbedtls_psram_calloc, mbedtls_psram_free);
    logPrintf("lvl=INFO tag=PSRAM-TLS msg=\"alokator mbedTLS nadpisany, bufory w PSRAM niezaleznie od WiFi\" progB=%u\n",
              (unsigned)PSRAM_TLS_THRESHOLD);
} else {
    logPrintln("lvl=WARN tag=PSRAM msg=\"PSRAM niedostepny, uzywa DRAM fallback\"");
}
// ────────────────────────────────────────────────────────────────────────



  if (WiFi.status() == WL_CONNECTED) {

    logPrintln("\nlvl=INFO tag=WiFi msg=\"Polaczono\" ip=" + WiFi.localIP().toString());







wsTerminal.onEvent(onWsEvent);
webserialServer.addHandler(&wsTerminal);


// [v146] USUNIETO: stary alias /webserial -> /terminal (nieużywany)

// ═══════════════════════════════════════════════════════════
//  TERMINAL WiFi - /terminal
// ═══════════════════════════════════════════════════════════
webserialServer.on("/terminal", HTTP_GET, [](AsyncWebServerRequest *request) {
  AsyncWebServerResponse *resp = request->beginResponse(  // [v98-FIX] beginResponse_P deprecated
    200, "text/html; charset=utf-8",
    TERMINAL_HTML, TERMINAL_HTML_LEN
  );
  request->send(resp);
});

// [v146] USUNIETO: cały panel /control (GŁÓWNY PANEL STEROWANIA) - nieużywany,
// zdublowany przez /terminal.

// ═════════════════════════════════════════════════════════════════
//  API: USTAW PWM (teraz - MANUAL/bieżące)
// ═════════════════════════════════════════════════════════════════
webserialServer.on("/set-pwm", HTTP_GET, [](AsyncWebServerRequest *request) {
  bool changed = false;
  // [v236] FIX-RAMPA-SUWAK-2: zapamiętaj stan SPRZED zmiany, zanim pętla niżej
  // nadpisze backupBrightnessComposite nową wartością suwaka. Bez tego każdy kod
  // niżej próbujący użyć backupBrightnessComposite jako "aktualnej" wartości LED
  // (do policzenia od czego rampować) w rzeczywistości widzi już NOWĄ wartość —
  // start rampy wychodzi równy celowi, więc rampa nie ma czego przejechać.
  uint16_t _preClickBBC[5];
  for (int i = 0; i < 5; i++) _preClickBBC[i] = getBrightnessForSection(backupBrightnessComposite, i);
  for (int i = 0; i < 5; i++) {
    String pName = "pwm" + String(i);
    if (request->hasParam(pName.c_str())) {
      uint16_t val = constrain(request->getParam(pName.c_str())->value().toInt(), 0, 1023);
      setBrightnessForSection(backupBrightnessComposite, i, val);

      // [OK] FIX PANEL-AUTO: W trybie AUTO sekcja5 liczy korekcję adaptacyjną
      // zawsze od backupAutoBrightnessComposite (harmonogram).
      // Jeśli nie zaktualizujemy też backupAuto, sekcja5 natychmiast wystartuje
      // rampę z powrotem do starej wartości harmonogramu - ignorując to co ustawił user.
      if (tryb) {
        setBrightnessForSection(backupAutoBrightnessComposite, i, val);
      }

      changed = true;
    }
  }
  if (changed) {
    if (!tryb) {
      backupManualBrightnessComposite = backupBrightnessComposite;
    } else {
      // [OK] PRIORYTET: Suwak w trybie AUTO podczas rampy adaptacyjnej
      // -> przerwij rampę, 30s płynnie do nowej wartości
      bool bylaRampaAdaptacyjna = rampaAdaptacyjnaAktywna;
      bool bylaRampaHarmonogramu = rampScheduleActive;
      bool bylaMinLux = rampaMinLuxPriorytet;  // [v237] zapamiętaj PRZED resetem
      rampaAdaptacyjnaAktywna = false;
      rampaMinLuxPriorytet    = false;
      transitionActive        = false;
      softStartActive         = false;

      if (bylaRampaHarmonogramu) {
        // [OK] Rampa harmonogramu: nie przerywaj — przelicz cel z pozostałym czasem.
        // Resetuj tylko rampInitialized -> rampa reinicjalizuje się z nowym celem
        // ale lastPwmStepMillis zostaje -> pozostały czas zachowany.
        // [v236] FIX-RAMPA-SUWAK-1: przechwyć wartość suwaka TERAZ, zanim kolejne
        // wywołania trybAuto() zdążą nadpisać backupBrightnessComposite czymś innym
        // (np. własnym przeliczeniem harmonogramu) — bez tego rampa startowała
        // z przypadkowej wartości sprzed nadpisania, nie z tego co ustawił user.
        for (int i = 0; i < 5; i++) {
          pendingManualRampStart[i] = (int16_t)getBrightnessForSection(backupBrightnessComposite, i);
          rampInitialized[i] = false;
        }
        logPrintln("lvl=INFO tag=PANEL-AUTO msg=\"Nowy cel suwaka, rampa harmonogramu przelicza do nowej wartosci\"");
      } else if (bylaRampaAdaptacyjna) {
        // Rampa adaptacyjna była aktywna - startuj soft-start 30s od aktualnego PWM do nowej wartości
        rampScheduleActive = false;
        for (int i = 0; i < 5; i++) rampInitialized[i] = false;
        if (!rampArbiterTryStart(RAMP_AUTO_SOFTSTART, true)) return;
        softStartActive = true;
        gPowerScale = 1.0f; gPowerScaleLastLogged = 1.0f;
        unsigned long nowMs = millis();
        unsigned long totalMs = SOFTSTARTSECONDS * 1000UL;
        for (int i = 0; i < 5; i++) {
          // [v236] FIX-RAMPA-SUWAK-2: currentVal z _preClickBBC (stan SPRZED tego
          // /set-pwm), nie z backupBrightnessComposite — to ostatnie już zawiera
          // NOWĄ wartość suwaka (nadpisane w pętli na górze handlera), więc start
          // rampy wychodził równy celowi = zero dystansu do przejechania.
          uint16_t currentVal = _preClickBBC[i];
          uint16_t targetVal  = getBrightnessForSection(backupAutoBrightnessComposite, i);
          softStartPwm[i]        = currentVal;
          softStartTargetPwm[i]  = targetVal;
          softStartStepMillis[i] = nowMs;
        }
        // [FIX-FLASH3] Kap target do limitu mocy
        {
          float prevPwr = getTotalLEDPower(softStartTargetPwm);
          float appliedScale = capTargetsToPowerLimit(softStartTargetPwm);
          if (appliedScale < 0.999f)
            logPrintf("lvl=WARN tag=SSTART-INIT kontekst=ADAPT moc=%.1f limit=%d skala=%.3f\n",
              prevPwr, LED_MAX_POWER_W, appliedScale);
        }
        for (int i = 0; i < 5; i++) {
          int diff = abs((int)softStartTargetPwm[i] - (int)softStartPwm[i]);
          softStartIntervalMs[i] = (diff > 0) ? (totalMs / diff) : 1000;
        }
        logPrintln("lvl=INFO tag=PANEL-AUTO msg=\"Rampa adaptacyjna przerwana, 30s do nowej wartosci suwaka\"");
        // [v237] FEATURE-MINLUX-MANUAL-LOCK: tylko gdy to był konkretnie MIN LUX
        // (nie inna, ogólna rampa adaptacyjna) — Twoja wartość trzyma się do
        // końca przerwy, applyMinLuxMode() jej nie nadpisze co 30s.
        if (bylaMinLux) {
          minLuxManualOverrideUntilBreakEnd = true;
          logPrintln("lvl=INFO tag=PANEL-AUTO msg=\"MIN LUX zablokowany reczna wartoscia do konca przerwy\"");
        }
      } else {
        // ═══════════════════════════════════════════════════════════════════
        // [OK] [v9 FIX-PANEL-BREAK] — chroniony harmonogram przed przypadkowym
        //    nadpisaniem przez panel WWW podczas przerwy/nocy
        //
        // PROBLEM (log_1.txt, 13:25):
        //   Użytkownik kliknął panel w przerwie południowej (LEDy WYŁ).
        //   Klik 1: ch0=1003, reszta=0 -> DIAG-3 SKOK + FLASH#4
        //   Klik 2: 0,0,0,0,0 -> backupAuto zerowane + EEPROM zapisany
        //   Po restarcie 14:05: autoB=[0,0,0,0,0] -> DIAG-18
        //   -> wieczorna rampa nigdy by nie weszła na 294 PWM.
        //
        // PRZYCZYNA (v8 i wcześniej):
        //   Blok /set-pwm bezwarunkowo nadpisywał backupAuto i EEPROM
        //   niezależnie od tego czy LEDy powinny aktualnie świecić.
        //   Użytkownik testujący suwaki w przerwie nieświadomie kasował
        //   harmonogram na stałe.
        //
        // ZMIANA:
        //   saveAutoBrightnessToEEPROM() wywoływane tylko gdy
        //   inLightingWindowGlobal=true (LEDy są w oknie świecenia).
        //   Podczas przerwy/nocy zmiany panelu są tymczasowe - RAM
        //   zmieniony, EEPROM chroniony. Log informuje użytkownika.
        // ═══════════════════════════════════════════════════════════════════
        rampScheduleActive = false;
        for (int i = 0; i < 5; i++) rampInitialized[i] = false;
        if (inLightingWindowGlobal) {
          saveAutoBrightnessToEEPROM();
          logPrintln("lvl=INFO tag=PANEL-AUTO msg=\"backupAuto zaktualizowany, EEPROM zapisany\"");
        } else {
          logPrintln("lvl=WARN tag=PANEL-AUTO msg=\"Poza oknem swiecenia, backupAuto NIE zapisano do EEPROM (chroni harmonogram)\"");
        }
        // [v236] FIX-RAMPA-SUWAK-2: dotąd ta gałąź (żadna rampa nie trwała w
        // momencie kliknięcia) nie miała ŻADNEGO soft-startu — wartość leciała
        // prosto na sprzęt, co kod sam sobie diagnozował jako
        // tag=DIAG-3 "SKOK BACKUP (bez rampy)" / tag=FLASH "skok hardware PWM
        // wykryty" (widoczne w logu: 76->237->424->80->655->104->0->223...).
        // Ten sam soft-start co w gałęzi rampy adaptacyjnej (30s, kapowany do
        // limitu mocy) — start z _preClickBBC (stan sprzed kliknięcia), cel
        // z nowej wartości suwaka (już w backupAutoBrightnessComposite).
        {
          if (!rampArbiterTryStart(RAMP_AUTO_SOFTSTART, true)) return;
          softStartActive = true;
          gPowerScale = 1.0f; gPowerScaleLastLogged = 1.0f;
          unsigned long nowMs = millis();
          unsigned long totalMs = SOFTSTARTSECONDS * 1000UL;
          for (int i = 0; i < 5; i++) {
            softStartPwm[i]        = _preClickBBC[i];
            softStartTargetPwm[i]  = getBrightnessForSection(backupAutoBrightnessComposite, i);
            softStartStepMillis[i] = nowMs;
          }
          float prevPwr = getTotalLEDPower(softStartTargetPwm);
          float appliedScale = capTargetsToPowerLimit(softStartTargetPwm);
          if (appliedScale < 0.999f)
            logPrintf("lvl=WARN tag=SSTART-INIT kontekst=PANEL-BEZ-RAMPY moc=%.1f limit=%d skala=%.3f\n",
              prevPwr, LED_MAX_POWER_W, appliedScale);
          for (int i = 0; i < 5; i++) {
            int diff = abs((int)softStartTargetPwm[i] - (int)softStartPwm[i]);
            softStartIntervalMs[i] = (diff > 0) ? (totalMs / diff) : 1000;
          }
          logPrintln("lvl=INFO tag=PANEL-AUTO msg=\"Suwak bez aktywnej rampy - dodano soft-start 30s\"");
        }
        // [v237] FEATURE-MINLUX-MANUAL-LOCK: ten sam przypadek co w gałęzi rampy
        // adaptacyjnej, ale tu żadna rampa akurat nie trwała w chwili kliknięcia.
        // [v238] FIX-MINLUX-LOCK-GAP: sprawdzane TERAZ przez (minLuxModeEnabled &&
        // !inLightingWindowGlobal) - "MIN LUX dotyczy tej przerwy" - zamiast samego
        // minLuxModeActive. Powód (znaleziony w realnym logu): MIN LUX mógł chwilę
        // wcześniej SAM się wyłączyć (czujnik zgłosił "wystarczy światła",
        // minLuxModeActive=false), więc suwak klikany w tym momencie nie ustawiał
        // blokady wcale (warunek na samym minLuxModeActive nie przechodził) — a
        // potem calculateMinLuxPWM() (czujnik odłączony = niestabilny fallback)
        // ZNOWU uznawał że trzeba dołożyć światła i bezwarunkowo reaktywował MIN
        // LUX (linia ~10431: minLuxModeActive=true, bez sprawdzenia blokady),
        // nadpisując dopiero co ustawioną przez usera wartość z powrotem na 0.
        if (minLuxModeEnabled && !inLightingWindowGlobal) {
          minLuxManualOverrideUntilBreakEnd = true;
          logPrintln("lvl=INFO tag=PANEL-AUTO msg=\"MIN LUX zablokowany reczna wartoscia do konca przerwy\"");
        }
      }
    }
    updateLEDs();
    logPrintln("lvl=INFO tag=PANEL-PWM msg=\"Ustawiono recznie z panelu WWW\"");
  }
  request->redirect("/terminal");
});

// ═════════════════════════════════════════════════════════════════
//  API: ZAPISZ DO AUTO (harmonogram)
// ═════════════════════════════════════════════════════════════════
webserialServer.on("/api/save-auto", HTTP_GET, [](AsyncWebServerRequest *request) {
  bool changed = false;
  for (int i = 0; i < 5; i++) {
    String pName = "pwm" + String(i);
    if (request->hasParam(pName.c_str())) {
      uint16_t val = constrain(request->getParam(pName.c_str())->value().toInt(), 0, 1023);
      setBrightnessForSection(backupAutoBrightnessComposite, i, val);
      changed = true;
    }
  }
  if (changed) {
    saveAutoBrightnessToEEPROM();
    logPrintln("lvl=INFO tag=PANEL-SAVE-AUTO msg=\"AUTO PWM zapisane do EEPROM z panelu WWW\"");
    request->send(200, "text/plain", "[OK] Zapisano do AUTO EEPROM!");
  } else {
    request->send(400, "text/plain", "Brak parametrów PWM");
  }
});

// ═════════════════════════════════════════════════════════════════
//  API: PRZEŁĄCZ POWER
// ═════════════════════════════════════════════════════════════════
webserialServer.on("/set-power", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (request->hasParam("v")) {
    bool requested = (request->getParam("v")->value() == "1");
    bool ok = enqueueAppCommand(AppCommandType::POWER_SET, requested, nullptr, "http");
    logPrintf("lvl=INFO tag=PANEL-POWER akcja=enqueue stan=%s ok=%d\n", requested ? "WL" : "WYL", ok ? 1 : 0);
    request->send(ok ? 200 : 503, "text/plain", ok ? "OK" : "QUEUE FULL");
    return;
  }
  request->send(400, "text/plain", "Brak parametru v");
});

// ═════════════════════════════════════════════════════════════════
//  API: PRZEŁĄCZ TRYB
// ═════════════════════════════════════════════════════════════════
webserialServer.on("/set-tryb", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (request->hasParam("v")) {
    bool requested = (request->getParam("v")->value() == "1");
    bool ok = enqueueAppCommand(AppCommandType::TRYB_SET, requested, nullptr, "http");
    logPrintf("lvl=INFO tag=PANEL-TRYB akcja=enqueue tryb=%s ok=%d\n", requested ? "AUTO" : "MANUAL", ok ? 1 : 0);
    request->send(ok ? 200 : 503, "text/plain", ok ? "OK" : "QUEUE FULL");
    return;
  }
  request->send(400, "text/plain", "Brak parametru v");
});

// ═════════════════════════════════════════════════════════════════
//  API: USTAW RAMPĘ
// ═════════════════════════════════════════════════════════════════
webserialServer.on("/set-fade", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (request->hasParam("f")) {
    fadeMinutes = constrain(request->getParam("f")->value().toInt(), 1, 180);
    saveFadeToEEPROM();
    logPrintf("lvl=INFO tag=PANEL-FADE czas=%d\n", fadeMinutes);
  }
  request->send(200, "text/plain", "OK");
});

webserialServer.on("/set-ema-filter", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (request->hasParam("v")) {
    float v = request->getParam("v")->value().toFloat();
    if (!isnan(v) && v >= 0.05f && v <= 0.5f) {
      emaFilterAlpha = v;
      saveEmaFilterToEEPROM();
      // Reset wygładzonej wartości żeby filtr zaczął od nowa z nowym alpha
      wygladzonyLuxPokojowy = 0;
      simLuxSmoothed = 0;
      logPrintf("lvl=INFO tag=EMA alpha=%.2f\n", emaFilterAlpha);
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Zakres: 0.05 - 0.50");
    }
  } else {
    request->send(400, "text/plain", "Brak parametru v");
  }
});

// ── SENS_INT - interwał odczytu czujnika ────────────────
webserialServer.on("/set-sens-int", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (request->hasParam("v")) {
    int v = request->getParam("v")->value().toInt();
    if (v >= 5 && v <= 120) {
      sensIntSec = (uint16_t)v;
      intervalOdczytu = (unsigned long)sensIntSec * 1000UL;
      saveSensIntToEEPROM();
      logPrintf("lvl=INFO tag=SENS sek=%u\n", sensIntSec);
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Zakres: 5 - 120");
    }
  } else {
    request->send(400, "text/plain", "Brak parametru v");
  }
});

// ── RAMP_SEC - czas rampy PWM ───────────────────────────
webserialServer.on("/set-ramp-sec", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (request->hasParam("v")) {
    int v = request->getParam("v")->value().toInt();
    if (v >= 5 && v <= 120) {
      rampSec = (uint16_t)v;
      saveRampSecToEEPROM();
      logPrintf("lvl=INFO tag=RAMP sek=%u\n", rampSec);
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Zakres: 5 - 120");
    }
  } else {
    request->send(400, "text/plain", "Brak parametru v");
  }
});

webserialServer.on("/set-kwh-price", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (request->hasParam("v")) {
    float v = request->getParam("v")->value().toFloat();
    if (!isnan(v) && v >= 0.01f && v <= 99.0f) {
      kwhPrice = v;
      saveKwhPrice();
      logPrintf("lvl=INFO tag=ENERGIA msg=\"Cena kWh zmieniona z panelu\" cena=%.2f\n", kwhPrice);
    }
  }
  request->send(200, "text/plain", "OK");
});

// ═════════════════════════════════════════════════════════════════
//  API: USTAW HARMONOGRAM
// ═════════════════════════════════════════════════════════════════
webserialServer.on("/set-schedule", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (request->hasParam("mw")) {
    String val = request->getParam("mw")->value();
    int h = val.substring(0, 2).toInt();
    int m = val.substring(3, 5).toInt();
    MORNING_ON_START_WEEKDAY = h * 60 + m;
  }
  if (request->hasParam("me")) {
    String val = request->getParam("me")->value();
    int h = val.substring(0, 2).toInt();
    int m = val.substring(3, 5).toInt();
    MORNING_ON_START_WEEKEND = h * 60 + m;
  }
  if (request->hasParam("mo")) {
    String val = request->getParam("mo")->value();
    int h = val.substring(0, 2).toInt();
    int m = val.substring(3, 5).toInt();
    MIDDAY_OFF_LOCAL = h * 60 + m;
  }
  if (request->hasParam("eo")) {
    String val = request->getParam("eo")->value();
    int h = val.substring(0, 2).toInt();
    int m = val.substring(3, 5).toInt();
    EVENING_OFF_START = h * 60 + m;
  }
  if (request->hasParam("eb")) {
    EVENING_ON_BEFORE_SUNSET_MIN = constrain(request->getParam("eb")->value().toInt(), -180, 180);
  }
  
  saveScheduleToEEPROM();
  // [OK] FIX-v19: Po zmianie harmonogramu zresetuj stan inicjalizacji ramp.
  // Bez tego: system uruchomiony poza starym oknem świecenia nie wykrywa,
  // że jest teraz W nowym oknie (np. Południe przesunięte z 10:08 -> 12:12
  // o 12:01 -> LED pozostają w trybie przerwy zamiast pójść do pełnej mocy).
  // ── [FIX-v70-C] FLASH przy zapisie harmonogramu w trakcie aktywnej rampy ──
  // autoRampInitialized=false powoduje, że FIX-v16 startuje rampę od backupAuto=275.
  // Przy elapsed≈54/60min target=0: currentPwm = 275×0.1 = 27 → skok z 233 → 27 (BŁYSK!).
  // Naprawa: przed resetem wpisz currentRampPwm do backupBrightnessComposite,
  // żeby FIX-v16 startował z realnej bieżącej pozycji rampy zamiast z max.
  if (rampScheduleActive) {
    for (int i = 0; i < 5; i++) {
      setBrightnessForSection(backupBrightnessComposite, i, currentRampPwm[i]);
    }
  }
  autoRampInitialized = false;
  softStartActive     = false;
  rampScheduleActive  = false;
  minLuxModeActive    = false;  // MIN LUX re-ewaluuje się wg nowego okna
  for (int i = 0; i < 5; i++) rampInitialized[i] = false;
  logPrintln("lvl=INFO tag=HARMONOGRAM msg=\"Zapisany z panelu WWW\"");
  request->send(200, "text/plain", "OK");
});

webserialServer.on("/set-pump-schedule", HTTP_POST,
  [](AsyncWebServerRequest *request) {},
  NULL,
  [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String body = "";
    body.reserve(total + 1);  // FIX-v44: eliminuje realokacje przy budowie body
    for (size_t i = 0; i < len; i++) body += (char)data[i];

    // Parse JSON array: [{"s":"08:00","e":"11:00"}, ...]
    int count = 0;
    int pos = 0;
    for (int i = 0; i < PUMP_MAX_SLOTS; i++) {
      pumpSlots[i] = {0, 0};
    }
    while (count < PUMP_MAX_SLOTS) {
      int si = body.indexOf("\"s\":\"", pos);
      int ei = body.indexOf("\"e\":\"", pos);
      if (si < 0 || ei < 0) break;
      String sv = body.substring(si + 5, si + 10);
      String ev = body.substring(ei + 5, ei + 10);
      int sh = sv.substring(0,2).toInt(), sm = sv.substring(3,5).toInt();
      int eh = ev.substring(0,2).toInt(), em = ev.substring(3,5).toInt();
      pumpSlots[count].start = constrain(sh*60+sm, 0, 1439);
      pumpSlots[count].end   = constrain(eh*60+em, 0, 1439);
      count++;
      pos = max(si, ei) + 10;
    }
    pumpSlotCount = count;
    savePumpScheduleToEEPROM();
    logPrintf("lvl=INFO tag=POMPA msg=\"Zapisano przedzialy z panelu\" liczba=%d\n", pumpSlotCount);
    for (int i = 0; i < pumpSlotCount; i++) {
      logPrintf("lvl=INFO tag=POMPA-SLOT slot=%d start=%02d:%02d koniec=%02d:%02d\n", i+1,
        pumpSlots[i].start/60, pumpSlots[i].start%60,
        pumpSlots[i].end/60, pumpSlots[i].end%60);
    }
    request->send(200, "text/plain", "OK");
  });

// ═════════════════════════════════════════════════════════════════
//  API: ZAPISZ REGULACJĘ ADAPTACYJNĄ
// ═════════════════════════════════════════════════════════════════
webserialServer.on("/api/adaptive/save", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
  [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String body = "";
    body.reserve(total + 1);  // FIX-v44: eliminuje realokacje przy budowie body
    for (size_t i = 0; i < len; i++) body += (char)data[i];
    
    int pos;
    if ((pos = body.indexOf("\"sensorMode\":")) > 0) {
      int mode = body.substring(pos+13, pos+14).toInt();
      // [v71] SENSOR-OFF: zapamiętaj poprzedni tryb żeby wykryć OFF→ON
      bool bylOffPrzedZmiana = (!uzywajCzujnikaPokojowego && !uzywajCzujnikaNadWoda);
      if (mode == 0) {
        uzywajCzujnikaPokojowego = false;
        uzywajCzujnikaNadWoda = false;
        regulacjaAdaptacyjnaWlaczona = false;
        // Wyczyść stan czujników - nie będą odpytywane
        czujnikPokojowyAktywny = false;
        czujnikNadWodaAktywny  = false;
        tslFailCount = 0;
        logPrintln("lvl=INFO tag=TSL msg=\"Tryb OFF, czujniki wylaczone, odpytywanie I2C wstrzymane\"");
      } else if (mode == 1) {
        uzywajCzujnikaPokojowego = true;
        uzywajCzujnikaNadWoda = false;
        // [v71] Jeśli poprzednio był OFF - wyzwól reinit bez restartu
        if (bylOffPrzedZmiana) {
          czujnikPokojowyAktywny = false;  // wymuś reinit
          czujnikNadWodaAktywny  = false;
          tslFailCount = 0;
          tslReinitPending = true;
          logPrintln("lvl=INFO tag=TSL msg=\"Tryb OFF->Pokoj, wyzwalam reinit I2C\"");
        }
      } else if (mode == 2) {
        uzywajCzujnikaPokojowego = true;
        uzywajCzujnikaNadWoda = true;
        // [v71] Jeśli poprzednio był OFF - wyzwól reinit bez restartu
        if (bylOffPrzedZmiana) {
          czujnikPokojowyAktywny = false;  // wymuś reinit
          czujnikNadWodaAktywny  = false;
          tslFailCount = 0;
          tslReinitPending = true;
          logPrintln("lvl=INFO tag=TSL msg=\"Tryb OFF->Pokoj+Woda, wyzwalam reinit I2C\"");
        }
      }
    }
    
    if ((pos = body.indexOf("\"adaptEnabled\":")) > 0) {
      regulacjaAdaptacyjnaWlaczona = (body.substring(pos+15, pos+16) == "1");
    }
    
    if ((pos = body.indexOf("\"learningEnabled\":")) > 0) {
      uczenieSieWlaczone = (body.substring(pos+18, pos+19) == "1");
    }
    
    saveAdaptiveSettingsToEEPROM();
    logPrintln("lvl=INFO tag=ADAPT-CFG msg=\"Zapisana z panelu WWW\"");
    request->send(200, "text/plain", "Regulacja zapisana");
  }
);

// ═════════════════════════════════════════════════════════════════
//  API: ZAPISZ MIN LUX
// ═════════════════════════════════════════════════════════════════
webserialServer.on("/api/minlux/save", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
  [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String body = "";
    body.reserve(total + 1);  // FIX-v44: eliminuje realokacje przy budowie body
    for (size_t i = 0; i < len; i++) body += (char)data[i];
    
    int pos;
    if ((pos = body.indexOf("\"enabled\":")) > 0) {
      minLuxModeEnabled = (body.substring(pos+10, pos+11) == "1");
    }
    
    if ((pos = body.indexOf("\"target\":")) > 0) {
      minLuxDayTarget = constrain(body.substring(pos+9, pos+14).toInt(), 500, 8000);
    }

    // [OK] FEATURE-v28: interwał MIN LUX
    if ((pos = body.indexOf("\"interval\":")) >= 0) {
      int iv = body.substring(pos+11, pos+15).toInt();
      iv = constrain(iv, 10, 300);
      minLuxIntervalSec = (uint16_t)iv;
      minLuxUpdateInterval = (unsigned long)minLuxIntervalSec * 1000UL;
    }
    
    if (!minLuxModeEnabled) {
      minLuxModeActive = false;
      for (int i = 0; i < 5; i++) minLuxCurrentPWM[i] = 0;
    }
    
    saveMinLuxModeToEEPROM();
    logPrintf("lvl=INFO tag=MIN-LUX stan=%s cel=%.0f\n", minLuxModeEnabled ? "WL" : "WYL", minLuxDayTarget);
    request->send(200, "text/plain", "MIN LUX zapisany");
  }
);

// =====================================================
//  API: /api/log-settings - sterowanie logami z panelu
// =====================================================
webserialServer.on("/api/log-settings", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
  [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String body = "";
    body.reserve(total + 1);  // FIX-v44: eliminuje realokacje przy budowie body
    for (size_t i = 0; i < len; i++) body += (char)data[i];

    int pos;
    String response = "";

    // LittleFS ON/OFF
    if ((pos = body.indexOf("\"littlefs\":")) >= 0) {
      int v = body.substring(pos+11, pos+12).toInt();
      littlefsReady = (v == 1);
      response += String("LittleFS: ") + (littlefsReady ? "WŁ" : "WYŁ") + " | ";
      logPrintf("lvl=INFO tag=DIR msg=\"LittleFS zmienione z panelu\" stan=%s\n", littlefsReady ? "WL" : "WYL");
    }

    // Komentarze ON/OFF
    if ((pos = body.indexOf("\"komentarze\":")) >= 0) {
      int v = body.substring(pos+13, pos+14).toInt();
      Komentarze = (v == 1);
      response += String("Komentarze: ") + (Komentarze ? "WŁ" : "WYŁ") + " | ";
      logPrintf("lvl=INFO tag=PANEL msg=\"Komentarze zmienione z panelu\" stan=%s\n", Komentarze ? "WL" : "WYL");
    }

    // logMode 0-3
    if ((pos = body.indexOf("\"logMode\":")) >= 0) {
      int v = body.substring(pos+10, pos+11).toInt();
      if (v >= 0 && v <= 3) {
        logMode = (LogDestination)v;
        saveLogModeToEEPROM();
        response += String("LogMode: ") + getLogModeString() + " | ";
        logPrintf("lvl=INFO tag=PANEL msg=\"LogMode zmieniony z panelu\" tryb=%s\n", getLogModeString().c_str());
      }
    }

    if (response.length() == 0) response = "Brak zmian";
    request->send(200, "text/plain", response);
  }
);

// =====================================================
//  API: /api/log-clear - czyszczenie logów z panelu
// =====================================================
webserialServer.on("/api/log-clear", HTTP_POST, [](AsyncWebServerRequest *request) {
  if (littlefsReady) {
    // [v98] FIX-LOG-CLEAR: ustaw flagę + reset statusów → tgTask wykonuje usunięcie
    // po vTaskDelay(150ms) + retry 5×. JS odpytuje /api/log-clear-status.
    logClearDone    = false;
    logClearFailed  = false;
    logClearPending = true;
    request->send(200, "text/plain", "PENDING");
  } else {
    request->send(200, "text/plain", "[WARN] LittleFS wylaczone - brak logow do usuniecia");
  }
});

// =====================================================
//  API: /api/log-clear-status - status operacji czyszczenia [v98]
// =====================================================
webserialServer.on("/api/log-clear-status", HTTP_GET, [](AsyncWebServerRequest *request) {
  const char* status;
  if      (logClearDone)       status = "DONE";
  else if (logClearFailed)     status = "ERROR";
  else if (logClearPending || logClearInProgress) status = "PENDING";
  else                         status = "IDLE";
  request->send(200, "text/plain", status);
});

// [v146] USUNIETO: stary alias /view-log -> /control (nieużywany)

// =====================================================
//  API: Ostatnie N linii
// =====================================================
webserialServer.on("/api/log-lines", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (!littlefsReady) {
    request->send(404, "text/plain", "Brak pliku");
    return;
  }

  int linesToShow = 100;
  if (request->hasParam("lines")) linesToShow = request->getParam("lines")->value().toInt();
  if (linesToShow > 1000) linesToShow = 1000;

  // [v102] DUAL-FILE: guard identyczny jak wcześniej
  if (logClearInProgress) { request->send(200, "text/plain", ""); return; }

  // [v102] Wczytaj ostatnie 80KB z [log_a + log_b] do PSRAM
  size_t bufLen = 0;
  char* buf = logReadCombinedTail(81920, bufLen);
  if (!buf) { request->send(500, "text/plain", "Brak pamieci PSRAM"); return; }
  if (bufLen == 0) { free(buf); request->send(200, "text/plain", "(plik pusty)"); return; }

  if (linesToShow == 0) {
    // [v106-LOG-CHUNK] beginResponse z callbackiem — bez StreamString O(n²)
    // shared_ptr trzyma bufor PSRAM aż do końca transferu (nawet przy zerwaniu połączenia)
    struct _PB { char* p; size_t len; ~_PB(){ if(p){ free(p); p=nullptr; } } };
    auto sp = std::make_shared<_PB>();
    sp->p = buf; sp->len = bufLen;
    auto resp = request->beginResponse("text/plain; charset=utf-8", bufLen,
      [sp](uint8_t* out, size_t maxLen, size_t idx) -> size_t {
        if (idx >= sp->len) return 0;
        size_t n = std::min(maxLen, sp->len - idx);
        memcpy(out, reinterpret_cast<const uint8_t*>(sp->p) + idx, n);
        return n;
      });
    request->send(resp);
    return;
  }

  // Wyszukaj wstecz N-tą linię od końca w buforze
  int linesFound = 0;
  long startPos = 0;
  for (long i = (long)bufLen - 1; i >= 0 && linesFound < linesToShow; i--) {
    if (buf[i] == '\n') {
      linesFound++;
      if (linesFound == linesToShow) {
        startPos = i + 1;
        break;
      }
    }
  }

  if ((size_t)startPos >= bufLen) {
    free(buf); request->send(200, "text/plain", "(brak wystarczajaco linii)"); return;
  }

  // [v106-LOG-CHUNK] beginResponse z callbackiem — bez StreamString O(n²)
  // Content-Length znany → przeglądarka może pokazać postęp pobierania
  {
    size_t sendLen = bufLen - (size_t)startPos;
    struct _PBN { char* base; char* start; size_t len;
                  ~_PBN(){ if(base){ free(base); base=nullptr; } } };
    auto sp = std::make_shared<_PBN>();
    sp->base  = buf;
    sp->start = buf + startPos;
    sp->len   = sendLen;
    auto resp = request->beginResponse("text/plain; charset=utf-8", sendLen,
      [sp](uint8_t* out, size_t maxLen, size_t idx) -> size_t {
        if (idx >= sp->len) return 0;
        size_t n = std::min(maxLen, sp->len - idx);
        memcpy(out, reinterpret_cast<const uint8_t*>(sp->start) + idx, n);
        return n;
      });
    request->send(resp);
  }
});

// =====================================================
//  API: Pobierz cały
// =====================================================
webserialServer.on("/api/log-download", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (!littlefsReady) {
    request->send(404, "text/plain", "Brak");
    return;
  }
  // [v102] DUAL-FILE: guard
  if (logClearInProgress) { request->send(503, "text/plain", "Czyszczenie w toku"); return; }

  // [v118] STREAM-LOG: Bezpośrednie strumieniowanie log_a + log_b bez bufora PSRAM.
  // Content-Length = szA + szB. Callback seek()+read() co ~1460B (TCP MTU) z LittleFS.
  // Brak ograniczenia rozmiaru — pobiera WSZYSTKIE logi z FS (do ~7.8MB przy FS_LIMIT=79%).
  {
    size_t szA = g_logFileSizeA, szB = g_logFileSize;
    // Weryfikacja cache (może być nieaktualne po nieoczekiwanej rotacji)
    if (szA == 0 && LittleFS.exists(LOG_FILE_A)) {
      File _f = LittleFS.open(LOG_FILE_A, "r"); if (_f) { szA = _f.size(); _f.close(); }
    }
    if (szB == 0 && LittleFS.exists(LOG_FILE_B)) {
      File _f = LittleFS.open(LOG_FILE_B, "r"); if (_f) { szB = _f.size(); _f.close(); }
    }
    size_t totalSz = szA + szB;
    if (totalSz == 0) { request->send(200, "text/plain", "(plik pusty)"); return; }

    // Przechowaj otwarte pliki w shared_ptr — żywotność = czas trwania transferu HTTP
    struct _LS {
      File fa, fb;
      size_t szA, szB;
      ~_LS() { if (fa) fa.close(); if (fb) fb.close(); }
    };
    auto ls = std::make_shared<_LS>();
    ls->szA = szA; ls->szB = szB;
    if (szA > 0) ls->fa = LittleFS.open(LOG_FILE_A, "r");
    if (szB > 0) ls->fb = LittleFS.open(LOG_FILE_B, "r");

    auto hfResp = request->beginResponse("text/plain; charset=utf-8", totalSz,
      [ls](uint8_t* out, size_t maxLen, size_t idx) -> size_t {
        if (idx < ls->szA) {
          // Czytaj z log_a (archiwum, starsze)
          size_t toRead = std::min(maxLen, ls->szA - idx);
          if (ls->fa) { ls->fa.seek(idx); return ls->fa.read(out, toRead); }
          return 0;
        } else {
          // Czytaj z log_b (aktywny, nowsze)
          size_t offB   = idx - ls->szA;
          size_t toRead = std::min(maxLen, ls->szB - offB);
          if (ls->fb) { ls->fb.seek(offB); return ls->fb.read(out, toRead); }
          return 0;
        }
      });
    hfResp->addHeader("Content-Disposition", "attachment; filename=\"log_combined.txt\"");
    request->send(hfResp);
  }
});

// ═══════════════════════════════════════════════════════════
//  API: /api/telegram/save - zapisz konfigurację Telegram
// ═══════════════════════════════════════════════════════════
webserialServer.on("/api/telegram/save", HTTP_POST,
  [](AsyncWebServerRequest *request){}, NULL,
  [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String body;
    body.reserve(len + 1);
    for (size_t i = 0; i < len; i++) body += (char)data[i];
    auto extractStr = [&](const char* key) -> String {
      String k = String("\"") + key + "\":\"";
      int p = body.indexOf(k);
      if (p < 0) return "";
      p += k.length();
      int e = body.indexOf("\"", p);
      return (e < 0) ? "" : body.substring(p, e);
    };
    String newToken  = extractStr("token");
    String newChatId = extractStr("chatId");
    int ep = body.indexOf("\"enabled\":");
    bool newEnabled = false;
    if (ep >= 0) {
      String ev = body.substring(ep + 10, ep + 16);
      ev.trim();
      newEnabled = (ev.startsWith("true") || ev.startsWith("1") || ev.startsWith("\"1\""));
    }
    if (newToken.length()  > 0) tgBotToken = newToken;
    if (newChatId.length() > 0) tgChatId   = newChatId;
    tgEnabled = newEnabled;
    saveTelegramConfig();
    request->send(200, "application/json",
      "{\"ok\":true,\"enabled\":" + String(tgEnabled ? "true" : "false") + "}");
  }
);

// ═══════════════════════════════════════════════════════════
//  API: /api/telegram/send - wyślij raport natychmiast
// ═══════════════════════════════════════════════════════════
webserialServer.on("/api/telegram/send", HTTP_POST,
  [](AsyncWebServerRequest *request) {
    if (!tgEnabled || tgBotToken.length() < 10 || tgChatId.length() < 3) {
      request->send(400, "application/json",
        "{\"ok\":false,\"error\":\"Telegram nie skonfigurowany lub wyłączony\"}");
      return;
    }
    bool ok = sendTelegramDocument();
    request->send(200, "application/json",
      ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"Sprawdź token i chatId\"}");
  }
);

// ═══════════════════════════════════════════════════════════
//  API: /api/telegram/status - odczyt konfiguracji
// ═══════════════════════════════════════════════════════════
webserialServer.on("/api/telegram/status", HTTP_GET,
  [](AsyncWebServerRequest *request) {
    String json = "{\"enabled\":" + String(tgEnabled ? "true" : "false");
    json += ",\"configured\":" + String((tgBotToken.length()>6 && tgChatId.length()>2) ? "true":"false");
    json += ",\"chatId\":\"" + tgChatId + "\"";
    json += ",\"tokenPrefix\":\"" + (tgBotToken.length()>6 ? tgBotToken.substring(0,6)+"..." : String("---")) + "\"}";
    request->send(200, "application/json", json);
  }
);

// ═══════════════════════════════════════════════════════════
//  [v139] FEATURE-WIFI-MULTI: API zarządzania listą sieci WiFi
// ═══════════════════════════════════════════════════════════

// GET /api/wifi/list - zapisane sieci (BEZ haseł) + aktualne połączenie
webserialServer.on("/api/wifi/list", HTTP_GET,
  [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", wifiNetworksListJson());
  }
);

// POST /api/wifi/scan/start - uruchamia ASYNCHRONICZNY skan eteru.
// WAŻNE: celowo async (WiFi.scanNetworks(true)) a nie blokujący -
// handler biegnie w tasku "async_tcp", monitorowanym przez TWDT (patrz
// FIX-API-HISTORY-WDT / FIX-FS-VIEW-WDT w changelogu). Blokujący skan
// (~1-4s) w tym kontekście byłby dokładnie tym samym błędem, który już
// wielokrotnie powodował crashe WDT w tym projekcie.
webserialServer.on("/api/wifi/scan/start", HTTP_POST,
  [](AsyncWebServerRequest *request) {
    WiFi.scanNetworks(true /* async */);
    request->send(200, "application/json", "{\"ok\":true,\"status\":\"started\"}");
  }
);

// GET /api/wifi/scan/result - wynik skanu (poll co ~500ms z panelu WWW,
// ten sam wzorzec co /api/log-clear-status: PENDING -> DONE).
webserialServer.on("/api/wifi/scan/result", HTTP_GET,
  [](AsyncWebServerRequest *request) {
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_RUNNING) {
      request->send(200, "application/json", "{\"status\":\"running\"}");
      return;
    }
    if (n == WIFI_SCAN_FAILED || n < 0) {
      request->send(200, "application/json", "{\"status\":\"idle\"}");
      return;
    }
    String json = "{\"status\":\"done\",\"networks\":[";
    for (int i = 0; i < n; i++) {
      if (i > 0) json += ",";
      bool known = false;
      for (auto &net : wifiNetworks) if (net.ssid == WiFi.SSID(i)) { known = true; break; }
      json += "{\"ssid\":\"" + WiFi.SSID(i) + "\"";
      json += ",\"rssi\":" + String(WiFi.RSSI(i));
      json += ",\"known\":" + String(known ? "true" : "false") + "}";
    }
    json += "]}";
    WiFi.scanDelete();  // zwolnij pamięć wyników skanu po odczycie
    request->send(200, "application/json", json);
  }
);

// POST /api/wifi/add - body JSON {"ssid":"...","pass":"..."} - dodaje nową
// sieć lub aktualizuje hasło, jeśli SSID już jest na liście.
webserialServer.on("/api/wifi/add", HTTP_POST,
  [](AsyncWebServerRequest *request){}, NULL,
  [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String body;
    body.reserve(len + 1);
    for (size_t i = 0; i < len; i++) body += (char)data[i];
    auto extractStr = [&](const char* key) -> String {
      String k = String("\"") + key + "\":\"";
      int p = body.indexOf(k);
      if (p < 0) return "";
      p += k.length();
      int e = body.indexOf("\"", p);
      return (e < 0) ? "" : body.substring(p, e);
    };
    String ssid = extractStr("ssid");
    String pass = extractStr("pass");
    if (ssid.length() == 0) {
      request->send(400, "application/json", "{\"ok\":false,\"error\":\"Brak SSID\"}");
      return;
    }
    bool ok = wifiAddNetwork(ssid, pass);
    if (ok) {
      request->send(200, "application/json", "{\"ok\":true}");
    } else {
      request->send(400, "application/json",
        "{\"ok\":false,\"error\":\"Osiągnięto limit " + String(WIFI_MAX_NETWORKS) + " sieci\"}");
    }
  }
);

// POST /api/wifi/delete - body JSON {"ssid":"..."} - usuwa sieć (min. 1 zostaje)
webserialServer.on("/api/wifi/delete", HTTP_POST,
  [](AsyncWebServerRequest *request){}, NULL,
  [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String body;
    body.reserve(len + 1);
    for (size_t i = 0; i < len; i++) body += (char)data[i];
    auto extractStr = [&](const char* key) -> String {
      String k = String("\"") + key + "\":\"";
      int p = body.indexOf(k);
      if (p < 0) return "";
      p += k.length();
      int e = body.indexOf("\"", p);
      return (e < 0) ? "" : body.substring(p, e);
    };
    String ssid = extractStr("ssid");
    bool ok = wifiDeleteNetwork(ssid);
    if (ok) {
      request->send(200, "application/json", "{\"ok\":true}");
    } else {
      request->send(400, "application/json",
        "{\"ok\":false,\"error\":\"Nie można usunąć (ostatnia sieć lub SSID nie znaleziony)\"}");
    }
  }
);

// [v146] USUNIETO: /wifi-settings (funkcja wbudowana teraz w /terminal)
//        oraz /clear-log (stara wersja GET; używamy /api/log-clear POST)

// ══════════════════════════════════════════════════════════
//  API: /api/history - historia 24h (CSV)
// ══════════════════════════════════════════════════════════
webserialServer.on("/api/history", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (!littlefsReady || !LittleFS.exists(HISTORY_FILE)) {
    request->send(200, "text/plain", "");
    return;
  }
  // [v87] OPT-B: jeśli brak martwych linii → wyślij cały plik (szybka ścieżka)
  if (histStartLine == 0) {
    request->send(LittleFS, HISTORY_FILE, "text/plain");
    return;
  }
  // [v87] OPT-B: pomiń pierwsze histStartLine "martwych" linii
  // Buforowane w pamięci (~16 KB max) — bezpieczne dla ESP32-S3 z 225 KB heap
  AsyncResponseStream* resp = request->beginResponseStream("text/plain");
  File _hf = LittleFS.open(HISTORY_FILE, "r");
  if (_hf) {
    // Nagłówek zawsze wysyłamy
    String _hdr = _hf.readStringUntil('\n');
    resp->print(_hdr); resp->print('\n');
    // Pomiń martwe linie
    // [v136] FIX-API-HISTORY-WDT: esp_task_wdt_reset() co 64 linie.
    // ROOT CAUSE: ten handler biegnie w kontekście tasku "async_tcp" (ESPAsyncWebServer/
    // AsyncTCP), który sam rejestruje się w TWDT (CONFIG_ASYNC_TCP_USE_WDT domyślnie = 1,
    // projekt tego nie nadpisuje). Ten sam wzorzec "pętla I/O bez wdt_reset", który już
    // raz crashował tgTask (HIST-COMPACT), tu grozi crashem samego async_tcp — a to
    // blokuje WSZYSTKIE połączenia sieciowe (WiFi/OTA/mDNS/inne requesty), więc otwarcie
    // zakładki "Historia" zaraz po restarcie (gdy histStartLine bywa >1700) może zawiesić
    // całe urządzenie. Reset działa poprawnie, bo async_tcp JEST monitorowany przez TWDT.
    for (int _i = 0; _i < histStartLine; _i++) {
      _hf.readStringUntil('\n');
      if ((_i & 0x3F) == 0) esp_task_wdt_reset();  // co 64 linie
    }
    // Wyślij żywe linie buforami
    uint8_t _buf[512]; int _r;
    while (_hf.available()) {
      _r = _hf.read(_buf, sizeof(_buf));
      if (_r > 0) resp->write(_buf, _r);
      esp_task_wdt_reset();
    }
    _hf.close();
  }
  request->send(resp);
});

// ══════════════════════════════════════════════════════════
//  API: /api/history/clear - wyczyść historię
// ══════════════════════════════════════════════════════════
webserialServer.on("/api/history/clear", HTTP_POST, [](AsyncWebServerRequest *request) {
  historyClearPending = true;  // zablokuj saveHistoryPoint na czas usuwania
  if (littlefsReady && LittleFS.exists(HISTORY_FILE)) LittleFS.remove(HISTORY_FILE);
  if (littlefsReady && LittleFS.exists("/history_old.csv")) LittleFS.remove("/history_old.csv");
  if (littlefsReady && LittleFS.exists("/history_tmp.csv")) LittleFS.remove("/history_tmp.csv");
  histLineCount       = 0;     // [OK] FIX-v33f: reset cache po wyczyszczeniu
  histStartLine       = 0;     // [v87] OPT-B: reset offset po wyczyszczeniu
  compactHistPending  = false; // [v87] OPT-B: anuluj ewentualne kompaktowanie
  historyClearPending = false;  // [OK] FIX-v33f: bez tego saveHistoryPoint nie zapisuje po clear (do restartu)
  request->send(200, "text/plain", "OK");
});  // [OK] FIX: brakujące zamknięcie handlera /api/history/clear

// ═══════════════════════════════════════════════════════════
//  API: /api/fs-list - lista plików LittleFS z rozmiarami
// ═══════════════════════════════════════════════════════════
webserialServer.on("/api/fs-list", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (!littlefsReady) {
    request->send(500, "application/json", "{\"error\":\"LittleFS nie gotowy\"}");
    return;
  }
  String json = "{\"files\":[";
  File root = LittleFS.open("/");
  if (root && root.isDirectory()) {
    bool first = true;
    File entry = root.openNextFile();
    while (entry) {
      if (!entry.isDirectory()) {
        if (!first) json += ",";
        first = false;
        String nm = String(entry.name());
        if (!nm.startsWith("/")) nm = "/" + nm;
        size_t sz = entry.size();
        entry.close();
        json += "{\"name\":\"" + nm + "\",\"size\":" + String(sz) + "}";
      } else {
        entry.close();
      }
      entry = root.openNextFile();
    }
    root.close();
  }
  json += "]";
  json += ",\"used\":"  + String(LittleFS.usedBytes());
  json += ",\"total\":" + String(LittleFS.totalBytes());
  json += ",\"limit\":" + String((unsigned)g_fsTotalBytes) + "}";  // [v152] FIX-FS-CAPACITY: realny rozmiar zamiast stałej
  AsyncWebServerResponse *resp = request->beginResponse(200, "application/json", json);
  resp->addHeader("Cache-Control", "no-cache");
  request->send(resp);
});

// ═══════════════════════════════════════════════════════════
//  API: /api/fs-view - podgląd dowolnego pliku (ostatnie N linii)
// ═══════════════════════════════════════════════════════════
webserialServer.on("/api/fs-view", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (!littlefsReady) { request->send(500, "text/plain", "LittleFS nie gotowy"); return; }
  if (!request->hasParam("file")) { request->send(400, "text/plain", "Brak parametru file"); return; }
  String fname = request->getParam("file")->value();
  if (fname.indexOf("..") >= 0) { request->send(400, "text/plain", "Niedozwolona sciezka"); return; }
  if (!fname.startsWith("/")) fname = "/" + fname;
  if (!LittleFS.exists(fname)) { request->send(404, "text/plain", "Plik nie istnieje"); return; }
  int lines = 0;
  if (request->hasParam("lines")) lines = request->getParam("lines")->value().toInt();
  File f = LittleFS.open(fname, "r");
  if (!f) { request->send(500, "text/plain", "Blad otwarcia"); return; }
  size_t sz = f.size();
  if (sz == 0) { f.close(); request->send(200, "text/plain; charset=utf-8", "(plik pusty)"); return; }
  if (lines <= 0) {
    // Cały plik
    f.close();
    request->send(LittleFS, fname, "text/plain; charset=utf-8");
    return;
  }
  // Ostatnie N linii - czytaj max 64 KB od końca
  const size_t CHUNK = 65536;
  size_t readFrom = (sz > CHUNK) ? sz - CHUNK : 0;
  f.seek(readFrom);
  String content = "";
  size_t wantLen = min(sz, CHUNK);
  content.reserve(wantLen + 4);
  // [v136] FIX-FS-VIEW-WDT: odczyt buforami 512B (zamiast bajt-po-bajcie) + esp_task_wdt_reset().
  // ROOT CAUSE: identyczny wzorzec jak /api/history — handler biegnie w tasku "async_tcp"
  // (zarejestrowanym w TWDT domyślnie, patrz FIX-API-HISTORY-WDT wyżej), a stara wersja
  // czytała do 65536 pojedynczych bajtów przez virtual f.read() BEZ ŻADNEGO resetu WDT —
  // to gorszy wariant tego samego problemu niż /api/history (brak buforowania w ogóle,
  // działa na DOWOLNYM pliku podanym w ?file=, nie tylko na history.csv). Realny odczyt
  // 12KB z LittleFS potrafił trwać nawet ~1.9s w niesprzyjających warunkach (patrz
  // forum ESP32) — 64KB bajt-po-bajcie realnie mogło przekroczyć okno WDT=15s.
  {
    uint8_t _buf[512]; int _r;
    while (f.available()) {
      _r = f.read(_buf, sizeof(_buf));
      if (_r > 0) content.concat((const char*)_buf, _r);
      esp_task_wdt_reset();
    }
  }
  f.close();
  // Znajdź n-tą linię od końca
  int lineCount = 0;
  int pos = (int)content.length() - 1;
  if (pos >= 0 && content[pos] == '\n') pos--;  // pomij końcowy \n
  while (pos >= 0 && lineCount < lines) {
    if (content[pos] == '\n') lineCount++;
    pos--;
  }
  if (pos > 0) content = content.substring(pos + 2);
  request->send(200, "text/plain; charset=utf-8", content);
});

// ═══════════════════════════════════════════════════════════
//  API: /api/fs-download - pobierz dowolny plik jako attachment
// ═══════════════════════════════════════════════════════════
webserialServer.on("/api/fs-download", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (!littlefsReady) { request->send(500, "text/plain", "LittleFS nie gotowy"); return; }
  if (!request->hasParam("file")) { request->send(400, "text/plain", "Brak parametru file"); return; }
  String fname = request->getParam("file")->value();
  if (fname.indexOf("..") >= 0) { request->send(400, "text/plain", "Niedozwolona sciezka"); return; }
  if (!fname.startsWith("/")) fname = "/" + fname;
  if (!LittleFS.exists(fname)) { request->send(404, "text/plain", "Plik nie istnieje"); return; }
  request->send(LittleFS, fname, "application/octet-stream", true);
});

// ═══════════════════════════════════════════════════════════
//  API: /api/fs-delete - usuń plik z LittleFS (POST ?file=xxx)
// ═══════════════════════════════════════════════════════════
webserialServer.on("/api/fs-delete", HTTP_POST, [](AsyncWebServerRequest *request) {
  if (!littlefsReady) { request->send(500, "text/plain", "LittleFS nie gotowy"); return; }
  if (!request->hasParam("file")) { request->send(400, "text/plain", "Brak parametru file"); return; }
  String fname = request->getParam("file")->value();
  if (fname.indexOf("..") >= 0) { request->send(400, "text/plain", "Niedozwolona sciezka"); return; }
  if (!fname.startsWith("/")) fname = "/" + fname;
  // [v102] log_tmp.txt usunięty (dual-file) — chroń tylko history_tmp.csv
  if (fname == "/history_tmp.csv") {
    request->send(403, "text/plain", "Plik tymczasowy - nie mozna usunac");
    return;
  }
  if (!LittleFS.exists(fname)) { request->send(404, "text/plain", "Plik nie istnieje"); return; }
  bool ok = LittleFS.remove(fname);
  if (ok) {
    logPrintf("lvl=INFO tag=DIR msg=\"Usunieto plik z panelu\" plik=%s\n", fname.c_str());
    request->send(200, "text/plain", "OK");
  } else {
    request->send(500, "text/plain", "Blad usuwania");
  }
});

// ═══════════════════════════════════════════════════════════
//  API: /api/status - STATUS JSON
// ═══════════════════════════════════════════════════════════
webserialServer.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
  uint16_t pwm0 = getBrightnessForSection(backupBrightnessComposite, 0);
  uint16_t pwm1 = getBrightnessForSection(backupBrightnessComposite, 1);
  uint16_t pwm2 = getBrightnessForSection(backupBrightnessComposite, 2);
  uint16_t pwm3 = getBrightnessForSection(backupBrightnessComposite, 3);
  uint16_t pwm4 = getBrightnessForSection(backupBrightnessComposite, 4);
  float pct0 = (pwm0 / 1023.0f) * 100.0f;
  
  int sensorMode = 0;
  if (uzywajCzujnikaPokojowego && uzywajCzujnikaNadWoda) sensorMode = 2;
  else if (uzywajCzujnikaPokojowego) sensorMode = 1;

  // [v102] DUAL-FILE: suma obu plików — cache bez open/read/close
  size_t logSz = logClearInProgress ? 0 : (g_logFileSizeA + g_logFileSize);

  // Czas harmonogramu
  char mwStr[6], meStr[6], moStr[6], eoStr[6];
  snprintf(mwStr, 6, "%02d:%02d", MORNING_ON_START_WEEKDAY/60, MORNING_ON_START_WEEKDAY%60);
  snprintf(meStr, 6, "%02d:%02d", MORNING_ON_START_WEEKEND/60, MORNING_ON_START_WEEKEND%60);
  snprintf(moStr, 6, "%02d:%02d", MIDDAY_OFF_LOCAL/60, MIDDAY_OFF_LOCAL%60);
  snprintf(eoStr, 6, "%02d:%02d", EVENING_OFF_START/60, EVENING_OFF_START%60);

  // Sloty pompki jako JSON array
  String pumpArr = "[";
  for (int i = 0; i < pumpSlotCount && i < PUMP_MAX_SLOTS; i++) {
    if (i > 0) pumpArr += ",";
    char s[6], e[6];
    snprintf(s, 6, "%02d:%02d", pumpSlots[i].start/60, pumpSlots[i].start%60);
    snprintf(e, 6, "%02d:%02d", pumpSlots[i].end/60,   pumpSlots[i].end%60);
    pumpArr += "{\"s\":\"" + String(s) + "\",\"e\":\"" + String(e) + "\"}";
  }
  pumpArr += "]";

  // Pompka - tablica liczb {start,end} dla loadStatus()
  String pumpArrNum = "[";
  for (int i = 0; i < pumpSlotCount && i < PUMP_MAX_SLOTS; i++) {
    if (i > 0) pumpArrNum += ",";
    pumpArrNum += "{\"start\":" + String(pumpSlots[i].start) + ",\"end\":" + String(pumpSlots[i].end) + "}";
  }
  pumpArrNum += "]";

  String json = "{";
  json.reserve(2048);  // [OK] FIX-v16-JSON: Pre-alokuj bufor - 81 konkatenacji += bez reserve()
                       // powoduje ~10 realokacji heapu przy każdym /api/status (co 2s).
                       // 2048 bajtów mieści pełny JSON z zapasem (faktyczna długość ~1600 B).
  // Podstawowe
  json += "\"fwVersion\":\"" + String(FW_VERSION) + "\",";
  json += "\"power\":" + String(power ? "true" : "false") + ",";
  json += "\"tryb\":" + String(tryb ? "true" : "false") + ",";
  json += "\"mode\":" + String(tryb ? "true" : "false") + ",";
  json += "\"fade\":" + String(fadeMinutes) + ",";
  json += "\"fadeMinutes\":" + String(fadeMinutes) + ",";
  json += "\"emaFilter\":" + String(emaFilterAlpha, 2) + ",";
  json += "\"sensInt\":" + String(sensIntSec) + ",";
  json += "\"rampSec\":" + String(rampSec) + ",";
  // IP, czas, WiFi/NTP - do nagłówka panelu
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\"," ;
  json += "\"wifiOK\":" + String(WiFi.status()==WL_CONNECTED ? "true" : "false") + ",";
  { struct tm _ti; bool _ok = getLocalTimePL(&_ti);
    json += "\"ntpOK\":" + String(_ok ? "true" : "false") + ",";
    char _tb[10]; if(_ok) snprintf(_tb,10,"%02d:%02d:%02d",_ti.tm_hour,_ti.tm_min,_ti.tm_sec); else strcpy(_tb,"--:--:--");
    json += "\"localTime\":\"" + String(_tb) + "\"," ; }
  // Temperatury - stare pola + tablica temps[]
  json += "\"temp1\":" + String(tempPlate1, 1) + ",";
  json += "\"temp2\":" + String(tempPlate2, 1) + ",";
  json += "\"water\":" + String(tempWater, 1) + ",";
  json += "\"temps\":[" + String(tempPlate1,1) + "," + String(tempPlate2,1) + "," + String(tempWater,1) + "],";
  // PWM - tablica pwm[] + osobne pola
  json += "\"pwm\":[" + String(pwm0) + "," + String(pwm1) + "," + String(pwm2) + "," + String(pwm3) + "," + String(pwm4) + "],";
  json += "\"pct\":" + String((int)pct0) + ",";
  json += "\"pwm0\":" + String(pwm0) + ",";
  json += "\"pwm1\":" + String(pwm1) + ",";
  json += "\"pwm2\":" + String(pwm2) + ",";
  json += "\"pwm3\":" + String(pwm3) + ",";
  json += "\"pwm4\":" + String(pwm4) + ",";
  // Lux
  json += "\"luxPokojowy\":" + String(luxPokojowy, 1) + ",";
  json += "\"luxRoom\":" + String(luxPokojowy, 1) + ",";
  json += "\"luxNadWoda\":" + String(luxNadWoda, 1) + ",";
  json += "\"czujnikPokojowyOK\":" + String(czujnikPokojowyAktywny ? "true" : "false") + ",";
  json += "\"czujnikNadWodaOK\":" + String(czujnikNadWodaAktywny ? "true" : "false") + ",";
  // Adaptacja
  json += "\"sensorMode\":" + String(sensorMode) + ",";
  json += "\"adaptEnabled\":" + String(regulacjaAdaptacyjnaWlaczona ? "true" : "false") + ",";
  json += "\"learningEnabled\":" + String(uczenieSieWlaczone ? "true" : "false") + ",";
  json += "\"adaptProbki\":" + String(adaptacja.liczbaProbek) + ",";
  json += "\"adaptTransmisja\":" + String((int)(adaptacja.nauczonaTransmisja * 100)) + ",";
  json += "\"luxPerPwm\":" + String(LUX_PER_PWM, 2) + ",";      // kalibracja: lux na 1 PWM (czujnik pokojowy)
  json += "\"glassTransm\":" + String(GLASS_TRANSMITTANCE, 2) + ",";  // transmisja szyby (domyślna)
  // MinLux
  json += "\"pumpOn\":" + String(currentPumpState ? "true" : "false") + "," +
  "\"minLuxEnabled\":" + String(minLuxModeEnabled ? "true" : "false") + ",";
  json += "\"minLuxTarget\":" + String((int)minLuxDayTarget) + ",";
  json += "\"minLuxInterval\":" + String((int)minLuxIntervalSec) + ",";
  json += "\"minLuxActive\":"   + String(minLuxModeActive        ? "true" : "false") + ",";
  // [OK] FIX-v22: flagi potrzebne do dynamicznej skali paska lux
  json += "\"inLightingWindow\":"  + String(inLightingWindowGlobal ? "true" : "false") + ",";
  json += "\"autoPWM\":"          + String((uint16_t)getBrightnessForSection(backupAutoBrightnessComposite, 0)) + ",";
  json += "\"minLuxPWM\":" + String(minLuxCurrentPWM[0]) + ",";
  // Harmonogram - stare pola string + obiekt schedule{} z liczbami (dla loadStatus())
  json += "\"schedMW\":\"" + String(mwStr) + "\",";
  json += "\"schedME\":\"" + String(meStr) + "\",";
  json += "\"schedMO\":\"" + String(moStr) + "\",";
  json += "\"schedEB\":" + String(EVENING_ON_BEFORE_SUNSET_MIN) + ",";
  json += "\"schedEO\":\"" + String(eoStr) + "\",";
  json += "\"schedule\":{\"morningWD\":" + String(MORNING_ON_START_WEEKDAY) + ",\"morningWE\":" + String(MORNING_ON_START_WEEKEND) + ",\"middayOff\":" + String(MIDDAY_OFF_LOCAL) + ",\"eveningBefore\":" + String(EVENING_ON_BEFORE_SUNSET_MIN) + ",\"eveningOff\":" + String(EVENING_OFF_START) + ",\"sunsetMin\":" + String(sunsetMinutes) + "},";
  // Pompka - format z liczbami {start,end} dla loadStatus()
  json += "\"pumpSlots\":" + pumpArrNum + ",";
  // Logi
  json += "\"logMode\":" + String((int)logMode) + ",";
  json += "\"littlefsReady\":" + String(littlefsReady ? "true" : "false") + ",";
  json += "\"komentarze\":" + String(Komentarze ? "true" : "false") + ",";
  json += "\"logSize\":" + String(logSz);
  json += ",\"fsUsed\":" + String(LittleFS.usedBytes());
  json += ",\"fsTotal\":" + String(LittleFS.totalBytes());
  json += ",\"ledMaxW\":" + String(LED_MAX_POWER_W);
  // ── Ramp widget JSON fields ──
  {
    bool rw = rampWidgetWasActive;
    // [OK] Elapsed ze zegara ściennego (minuty harmonogramu) - odporny na restart i stary startMs
    int32_t elapsedMin = 0;
    if (rw && rampWidgetRampStartMin >= 0 && lastNowMin >= 0) {
      elapsedMin = lastNowMin - rampWidgetRampStartMin;
      if (elapsedMin < 0) elapsedMin += 1440;   // obsługa przekroczenia północy
      elapsedMin = constrain(elapsedMin, 0, (int32_t)fadeMinutes);
    }
    unsigned long elapsed = (unsigned long)elapsedMin * 60000UL;
    unsigned long total   = (unsigned long)fadeMinutes * 60UL * 1000UL;
    uint16_t pwmNow = (uint16_t)getBrightnessForSection(backupBrightnessComposite, 0);
    json += ",\"rampActive\":"    + String(rw ? "true" : "false");
    json += ",\"rampUp\":"        + String(rampWidgetUp ? "true" : "false");
    json += ",\"rampElapsedMs\":" + String(elapsed);
    json += ",\"rampTotalMs\":"   + String(total);
    json += ",\"rampPwmStart\":"  + String(rampWidgetPwmStart);
    json += ",\"rampPwmTarget\":" + String(rampWidgetPwmTarget);
    json += ",\"rampPwmNow\":"    + String(pwmNow);
  }
  json += ",\"statKorektyMin\":" + String(statystyki.korektyMinimum);
  json += ",\"statKorektyMax\":" + String(statystyki.korektyMaksimum);
  json += ",\"statPomiary\":" + String(statystyki.liczbaPomiarow);
  json += ",\"statSredniaRedukcja\":" + String(statystyki.liczbaPomiarow>0 ? statystyki.sredniaRedukcja/statystyki.liczbaPomiarow : 0.0f, 1);
  // ── Nowe statystyki energii / LED / LUX ──
  json += ",\"energyTodayWh\":" + String(energyTodayWh, 1);
  json += ",\"energyWeekWh\":" + String(energyWeekWh, 1);
  json += ",\"energyMonthWh\":" + String(energyMonthWh, 1);
  // [OK] FIX-v29-11: historia dni tygodnia (Pn=d0..Nd=d6)
  json += ",\"dayHistory\":["
       + String(energyDayHistory[0],1)+","+String(energyDayHistory[1],1)+","
       + String(energyDayHistory[2],1)+","+String(energyDayHistory[3],1)+","
       + String(energyDayHistory[4],1)+","+String(energyDayHistory[5],1)+","
       + String(energyDayHistory[6],1)+"]";
  // [OK] FIX-v29-11: historia tygodni miesiąca (w0..w4)
  json += ",\"weekHistory\":["
       + String(energyWeekHistory[0],1)+","+String(energyWeekHistory[1],1)+","
       + String(energyWeekHistory[2],1)+","+String(energyWeekHistory[3],1)+","
       + String(energyWeekHistory[4],1)+"]";
  json += ",\"ledOnMinutesToday\":" + String(ledOnMinutesToday);
  json += ",\"ledOnMinutesWeek\":"  + String(ledOnMinutesWeek);
  json += ",\"ledOnMinutesMonth\":" + String(ledOnMinutesMonth);
  json += ",\"luxHoursTodayWater\":" + String(luxHoursTodayWater, 0);
  json += ",\"minLuxActivToday\":" + String(minLuxActivToday);
  json += ",\"peakPowerWToday\":" + String(peakPowerWToday, 1);
  json += ",\"minLuxActivWeek\":" + String(minLuxActivWeek);
  json += ",\"kwhPrice\":" + String(kwhPrice, 2);
  // ── Aktualna moc realna (z tabel pomiarowych CSV) ──
  {
    uint16_t curPwm[5];
    for (int i = 0; i < 5; i++) {
      curPwm[i] = (uint16_t)getBrightnessForSection(backupBrightnessComposite, i);
    }
    float chW[5];
    float totW = 0.0f;
    for (int i = 0; i < 5; i++) {
      chW[i] = getPowerForChannel(i, curPwm[i]);
      totW += chW[i];
    }
    json += ",\"powerNowW\":" + String(totW, 1);
    json += ",\"powerNowCh\":[" + String(chW[0],1) + "," + String(chW[1],1) + ","
                                + String(chW[2],1) + "," + String(chW[3],1) + ","
                                + String(chW[4],1) + "]";
    json += ",\"powerLimitW\":" + String(LED_MAX_POWER_W);
    json += ",\"powerLimitActive\":" + String(totW > (float)LED_MAX_POWER_W ? "true" : "false");
  }
  // ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  // [SIM-UI] Stan symulacji czujnika światła - dla panelu web.
  // Aby usunąć symulację: usuń ten blok + kartę HTML [SIM-UI]
  // + endpoint /api/sim [SIM-UI] + funkcję JS simLoad() [SIM-UI]
  // ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  json += ",\"simEnabled\":"   + String(simLuxEnabled ? "true" : "false");
  json += ",\"simAuto\":"      + String(simLuxAuto    ? "true" : "false");
  json += ",\"simValue\":"     + String((int)simLuxValue);
  json += ",\"simSmoothed\":"  + String((int)simLuxSmoothed);
  json += ",\"simMin\":"       + String((int)simLuxAutoMin);
  json += ",\"simMax\":"       + String((int)simLuxAutoMax);
  json += ",\"simPeriodS\":"   + String((int)(simLuxAutoPeriodMs / 1000UL));
  // ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  json += "}";
  
  request->send(200, "application/json", json);
});

// ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
// [SIM-UI] API: /api/sim - sterowanie symulacją z panelu web.
//
// Parametry POST (application/x-www-form-urlencoded):
//   action=on        - włącz symulację stałą wartością
//   action=off       - wyłącz symulację (hardware przejmuje)
//   action=auto      - włącz tryb sinusoidalny
//   value=[0-50000]  - ustaw stałą wartość lux
//   min=[val]        - dolna granica sinusoidy
//   max=[val]        - górna granica sinusoidy
//   period=[sek]     - okres sinusoidy w sekundach (10-86400)
//
// Aby USUNĄĆ symulację z kodu:
//   1. Usuń ten endpoint (cały blok od [SIM-UI] API do następnego //)
//   2. Usuń kartę HTML oznaczoną [SIM-UI] w p-ustawienia
//   3. Usuń funkcje JS simLoad() i simSave() oznaczone [SIM-UI]
//   4. Usuń pola simEnabled/simAuto/... z /api/status (oznaczone [SIM-UI])
//   5. Usuń zmienne simLux* i blok if(simLuxEnabled) w odczytajSwiatlo...()
// ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
webserialServer.on("/api/sim", HTTP_GET,
  [](AsyncWebServerRequest *request){
    String action = request->hasParam("action") ? request->getParam("action")->value() : "";
    String valStr = request->hasParam("value")  ? request->getParam("value")->value()  : "";
    String minStr = request->hasParam("min")    ? request->getParam("min")->value()    : "";
    String maxStr = request->hasParam("max")    ? request->getParam("max")->value()    : "";
    String perStr = request->hasParam("period") ? request->getParam("period")->value() : "";

    if (valStr.length() > 0) {
      float v = valStr.toFloat();
      if (v >= 0 && v <= 50000) { simLuxValue = v; simLuxSmoothed = v; }
    }
    if (minStr.length() > 0) {
      float v = minStr.toFloat();
      if (v >= 0 && v < simLuxAutoMax) simLuxAutoMin = v;
    }
    if (maxStr.length() > 0) {
      float v = maxStr.toFloat();
      if (v > simLuxAutoMin && v <= 50000) simLuxAutoMax = v;
    }
    if (perStr.length() > 0) {
      int s = perStr.toInt();
      if (s >= 10 && s <= 86400) simLuxAutoPeriodMs = (unsigned long)s * 1000UL;
    }

    if (action == "on") {
      simLuxEnabled = true;
      simLuxAuto    = false;
      czujnikPokojowyAktywny = true;
      logPrintf("lvl=INFO tag=SIM-UI msg=\"Symulacja WL, stala\" lux=%.0f\n", simLuxValue);
      request->send(200, "text/plain", "ok");
    } else if (action == "off") {
      simLuxEnabled  = false;
      simLuxAuto     = false;
      simLuxSmoothed = 0.0f;
      logPrintf("lvl=INFO tag=SIM-UI msg=\"Symulacja WYL, hardware aktywny\"\n");
      request->send(200, "text/plain", "ok");
    } else if (action == "auto") {
      simLuxEnabled  = true;
      simLuxAuto     = true;
      simLuxSmoothed = 0.0f;
      czujnikPokojowyAktywny = true;
      logPrintf("lvl=INFO tag=SIM-UI msg=\"Symulacja AUTO-SIN WL\" min=%.0f max=%.0f okres=%lus\n",
                simLuxAutoMin, simLuxAutoMax, simLuxAutoPeriodMs / 1000UL);
      request->send(200, "text/plain", "ok");
    } else if (action == "set") {
      logPrintf("lvl=INFO tag=SIM-UI msg=\"Parametry zaktualizowane\" val=%.0f min=%.0f max=%.0f okres=%lus\n",
                simLuxValue, simLuxAutoMin, simLuxAutoMax, simLuxAutoPeriodMs / 1000UL);
      request->send(200, "text/plain", "ok");
    } else {
      request->send(400, "text/plain", "Nieznana akcja");
    }
  }
);
// ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
// [SIM-UI] koniec endpointu /api/sim
// ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

// ═══════════════════════════════════════════════════════════
//  API: /api/quick/* - SZYBKIE AKCJE
// ═══════════════════════════════════════════════════════════
webserialServer.on("/api/quick/camera_on", HTTP_POST, [](AsyncWebServerRequest *request) {
  wlaczKamera();
  request->send(200, "text/plain", "📷 Kamera ON");
});

webserialServer.on("/api/quick/camera_off", HTTP_POST, [](AsyncWebServerRequest *request) {
  wylaczKamera();
  request->send(200, "text/plain", "📷 Kamera OFF");
});

webserialServer.on("/api/quick/pump_on", HTTP_POST, [](AsyncWebServerRequest *request) {
  // [OK] FIX BUG-H: Ustaw override 30 minut - updatePump() nie nadpisze stanu
  digitalWrite(pumpPin, LOW);   // aktywny LOW: LOW = włączona
  currentPumpState = true;
  pumpManualOverrideUntil = millis() + 30UL * 60UL * 1000UL;
  if (Komentarze) logPrintln("lvl=INFO tag=POMPA msg=\"Manual ON, override 30 min\"");
  request->send(200, "text/plain", "💦 Pompa ON (override 30min)");
});

webserialServer.on("/api/quick/pump_off", HTTP_POST, [](AsyncWebServerRequest *request) {
  // [OK] FIX BUG-H: Kasuj override i wyłącz pompkę
  pumpManualOverrideUntil = 0;
  digitalWrite(pumpPin, HIGH);  // aktywny LOW: HIGH = wyłączona
  currentPumpState = false;
  if (Komentarze) logPrintln("lvl=INFO tag=POMPA msg=\"Manual OFF, override skasowany\"");
  request->send(200, "text/plain", "💦 Pompa OFF");
});

webserialServer.on("/api/quick/led_test", HTTP_POST, [](AsyncWebServerRequest *request) {
  for (int i = 0; i < 5; i++) {
    setBrightnessForSection(backupBrightnessComposite, i, 1023);
  }
  updateLEDs();
  request->send(200, "text/plain", "💡 LED 100%");
});

webserialServer.on("/api/quick/led_off", HTTP_POST, [](AsyncWebServerRequest *request) {
  for (int i = 0; i < 5; i++) {
    setBrightnessForSection(backupBrightnessComposite, i, 0);
  }
  updateLEDs();
  request->send(200, "text/plain", "🌑 LED OFF");
});

// [OK] FIX BUG-V: Flaga opóźnionego restartu - nie używaj delay() w handlerze HTTP
// który blokuje główny task (WebSocket, derating) przez 1 sekundę.

webserialServer.on("/api/quick/restart", HTTP_POST, [](AsyncWebServerRequest *request) {
  restartRequestedAt = millis();
  request->send(200, "text/plain", "🔄 Restartuje za 1s...");
  // restart wykona loop() przy następnym obrocie
});

// ═══════════════════════════════════════════════════════════════════════════
// [4.1.0 OTA-GITHUB] Etap 1 planu upgrade: GET /api/ota-status + POST
// /api/ota-start — OTA przez GitHub Releases (port 1:1 z Centrali Pieca).
// Wyzwalacze: panel WWW, komenda /update (Telegram), "update" (Firebase).
// ═══════════════════════════════════════════════════════════════════════════
otaGithubRegisterEndpoints();






    // CORS - pozwala narzędziu migracji łączyć się z panelem z lokalnego pliku HTML
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin",  "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
    // [v93] FIX-WDT-LIBRARY: mathieucarbou/AsyncTCP obsługuje locking wewnętrznie.
    // Prosty begin() bez żadnego wrappera. esp_task_wdt_reset() zachowane defensywnie.
    esp_task_wdt_reset();
    webserialServer.begin();
    esp_task_wdt_reset();
    wsReady = true;
    String terminalURL = "http://" + WiFi.localIP().toString() + ":8080/terminal";
    logPrintln("lvl=INFO tag=BOOT msg=\"Terminal dostepny\" url=" + terminalURL);

    // ── [v98-OTA] ArduinoOTA — wgrywanie przez espota na port 3232 ──────────
    startOtaIfNeeded();
    otaReady = true;
    // ─────────────────────────────────────────────────────────────────────────
    // ═══════════════════════════════════════════════════

    // [v228] HARDEN-NTP-DHCP-GUARD — patrz PLAN_WDROZENIOWY_lwip_sntp_crash.md.
    // Kanarek kompilacyjny: jeśli CONFIG_LWIP_DHCP_GET_NTP_SRV kiedykolwiek
    // wróci do "y" (custom_sdkconfig w platformio.ini cofnięty/nadpisany bez
    // pełnego czystego rebuildu), poniższy #warning pojawi się w logu builda,
    // a log startowy poniżej zapisze to jawnie w aquarium_log.txt — wraca
    // wtedy realne ryzyko PANIC/CRASH w sntp_servermode_dhcp (assert
    // "Required to lock TCPIP core functionality!", esphome/issues#6591,
    // nienaprawione upstream). UWAGA: configTzTime() poniżej MUSI zostać
    // wołane dokładnie RAZ w całym programie — druga, niezależna, ZNANA
    // przyczyna tego samego assertu (arduino-esp32 #10675) to podwójne
    // wywołanie configTzTime()/sntp_stop() z kodu aplikacji.
#ifdef CONFIG_LWIP_DHCP_GET_NTP_SRV
    #warning "CONFIG_LWIP_DHCP_GET_NTP_SRV wlaczone - ryzyko PANIC/CRASH w sntp_servermode_dhcp (esphome/issues#6591, brak fixu upstream). Sprawdz custom_sdkconfig w platformio.ini + zrob pelny czysty rebuild (usun .pio/build/<env>)."
    logPrintln("lvl=WARN tag=NTP msg=\"CONFIG_LWIP_DHCP_GET_NTP_SRV=WLACZONE - ryzyko PANIC/CRASH, patrz PLAN_WDROZENIOWY_lwip_sntp_crash.md\"");
#else
    logPrintln("lvl=INFO tag=NTP msg=\"CONFIG_LWIP_DHCP_GET_NTP_SRV=wylaczone (fix aktywny)\"");
#endif

    configTzTime("UTC0", "pool.ntp.org", "time.nist.gov");

    struct tm ti;
    int retry = 0;
    // BUG#9 FIX: getLocalTimePL zamiast getLocalTime - configTzTime=UTC0
    // getLocalTime zwraca UTC, obliczZachodSlonca potrzebuje lokalnego roku/miesiaca/dnia
    while (!getLocalTimePL(&ti) && retry++ < 10) {
      logPrintln("lvl=INFO tag=NTP msg=\"Oczekiwanie na czas NTP\"");
      delay(1000);
      esp_task_wdt_reset(); // [OK] informuj watchdog
    }

    if (retry >= 10) {
      logPrintln("lvl=ERR tag=NTP msg=\"Brak czasu NTP, uzywam 19:00\"");
      sunsetMinutes = 19 * 60;
    } else {

      logPrintf(
        "lvl=INFO tag=NTP msg=\"Czas UTC\" godz=%02d:%02d:%02d\n",
        ti.tm_hour, ti.tm_min, ti.tm_sec
      );

      int zachodUTC = obliczZachodSlonca(
        ti.tm_year + 1900,
        ti.tm_mon + 1,
        ti.tm_mday,
        latitude,
        longitude
      );

      if (zachodUTC >= 0) {
        bool dst = isDaylightSaving(time(nullptr));
        // [OK] FIX B: Używaj stałej timezoneOffsetMinutes zamiast hardcoded 120/60
        int offset = dst ? timezoneOffsetMinutes : (timezoneOffsetMinutes - 60);
        sunsetMinutes = (zachodUTC + offset + 1440) % 1440;
      } else {
        sunsetMinutes = 19 * 60;
      }

      logPrintf(
        "lvl=INFO tag=NTP msg=\"Zachod slonca lokalnie\" zachod=%02d:%02d\n",
        sunsetMinutes / 60,
        sunsetMinutes % 60
      );
    }

    lastNtpSync = millis();

    // [OK] FIX-v29-NTP: Wczytaj statystyki energii TUTAJ, po potwierdzeniu czasu NTP.
    // getLocalTimePL() zwraca teraz poprawną datę -> sameDay/sameWeek/sameMon działają poprawnie.
    if (littlefsReady) {
      loadEnergyStats();
      logPrintln("lvl=INFO tag=ENERGIA msg=\"Statystyki wczytane po synchronizacji NTP\"");
    }

    // [v239] FIX-SNTP-FB-RACE: zdiagnozowane przez addr2line na prawdziwym crashu
    // (assert w lwIP udp_new_ip_type, wywołany z fbInitialize()->hostByName(),
    // w tym samym momencie co SNTP próbujące w tle kolejnego serwera NTP —
    // patrz Ryby_LED_S3_plan_fix_sntp_race.md). Krótka przerwa osadzająca PRZED
    // pierwszym fbInitialize() (poniżej, ~linia 16502) — zmniejsza (nie
    // gwarantuje ze 100% pewnością, patrz plan) szansę zazębienia się w czasie
    // z SNTP wciąż aktywnym w tle. NIE zatrzymuje/restartuje SNTP (to osobna,
    // znana przyczyna tego samego assertu z v228 - configTzTime/sntp_stop
    // wołane więcej niż raz) - tylko czekamy, aż samo się uspokoi.
    logPrintln("lvl=INFO tag=NTP msg=\"Przerwa osadzajaca przed fbInitialize (FIX-SNTP-FB-RACE)\"");
    for (int i = 0; i < 30; i++) {  // 3s w kawałkach po 100ms, WDT-safe
      delay(100);
      esp_task_wdt_reset();
    }

  } else {

    logPrintln("lvl=WARN tag=SETUP msg=\"Brak WiFi w setup() po 2 probach, webserialServer i ArduinoOTA NIE wystartowaly w tym boocie (wystartuja po nastepnym restarcie z dostepnym WiFi)\"");
    logPrintln("\nlvl=ERR tag=SETUP msg=\"Brak WiFi, uzywam domyslnego zachodu 19:00\"");
    sunsetMinutes = 19 * 60;

    // [OK] FIX-v29-NOWIFI: Brak WiFi = brak NTP = nie możemy zweryfikować daty.
    // Statystyki zostaną wczytane przy następnym restarcie z WiFi.
    if (littlefsReady) {
      logPrintln("lvl=WARN tag=ENERGIA msg=\"Brak WiFi/NTP, statystyki energii nie wczytane (brak weryfikacji daty)\"");
    }
  }

  // ═══════════════════════════════════════════════════════════════
  //  RESET MAGISTRALI I2C przed inicjalizacją TSL2561
  //  Konieczne po crashu/saturacji - odblokowuje SDA/SCL
  //  Dostosuj piny SDA/SCL jeśli inne niż 21/22
  // ═══════════════════════════════════════════════════════════════
  logPrintln("lvl=INFO tag=TSL-INIT msg=\"Reset I2C start\""); Serial.flush();
  Wire.end();
  delay(50);
  // 9 impulsów CLK ręcznie - wymusza zwolnienie urządzeń trzymających SDA
  pinMode(20, OUTPUT);  // SCL (S3: GPIO 22 nie istnieje -> 20)
  for (int i = 0; i < 9; i++) {
    digitalWrite(20, HIGH); delayMicroseconds(10);
    digitalWrite(20, LOW);  delayMicroseconds(10);
  }
  // STOP condition: SCL HIGH, SDA LOW->HIGH
  pinMode(21, OUTPUT);  // SDA
  digitalWrite(20, HIGH);
  delayMicroseconds(10);
  digitalWrite(21, HIGH);
  delayMicroseconds(10);
  pinMode(20, INPUT);
  pinMode(21, INPUT);
  delay(50);
  Wire.begin(21, 20);  // SDA=21, SCL=20 (S3: GPIO 22 nie istnieje)
  Wire.setTimeOut(50);
  delay(100);
  logPrintln("lvl=INFO tag=TSL-INIT msg=\"I2C gotowy\""); Serial.flush();

  logPrintln("lvl=INFO tag=TSL-INIT msg=\"Inicjalizacja czujnikow TSL2561\""); Serial.flush();
  
  if (czujnikPokojowy.begin()) {
    czujnikPokojowy.enableAutoRange(true);
    czujnikPokojowy.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);
    czujnikPokojowyAktywny = true;
    logPrintln("lvl=INFO tag=TSL-INIT czujnik=pokojowy addr=0x39"); Serial.flush();
  } else {
    logPrintln("lvl=WARN tag=TSL-INIT czujnik=pokojowy addr=0x39 msg=\"Nie znaleziony\""); Serial.flush();
  }
  
  esp_task_wdt_reset();  // reset WDT przed drugim czujnikiem
  if (czujnikNadWoda.begin()) {
    czujnikNadWoda.enableAutoRange(true);
    czujnikNadWoda.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);
    czujnikNadWodaAktywny = true;
    logPrintln("lvl=INFO tag=TSL-INIT czujnik=woda addr=0x29"); Serial.flush();
  } else {
    logPrintln("lvl=WARN tag=TSL-INIT czujnik=woda addr=0x29 msg=\"Nie znaleziony, pin ADDR podlaczony do GND?\""); Serial.flush();
  }
  
  // [v33] Arduino IoT Cloud usunięty - initProperties() i ArduinoCloud.begin() usunięte

  // KLUCZOWE: BLOKADA RAMP PO RESECIE
  autoRampInitialized = false;
  softStartActive = false;

  // =====================================================
  //  KONIEC
  // =====================================================
  logPrintf(
    "lvl=INFO tag=BOOT msg=\"System gotowy\" zachod=%02d:%02d\n",
    sunsetMinutes / 60,
    sunsetMinutes % 60
  );
  
  // 🔁 synchronizacja suwaka po starcie
  Serial.println(">> przed onSectionsChange()"); Serial.flush();
  onSectionsChange();

  // [FIX-v132-LOGBUF-EARLY] logMutex + g_logBuf przeniesione na początek setup()
  // (przed pierwszym logPrintln) — duplikat usunięty.

  // ── [FIX-v153-CRASH-BACKOFF] Backoff przed FB+TG reinit po wykrytym crash-loopie ──
  // Plan Priorytet 3b, pkt 3: "krótkie opóźnienie/backoff przed pełną re-inicjalizacją
  // FB+TG zaraz po restarcie spowodowanym przez sieć — żeby nie wchodzić od razu w tę
  // samą zdegradowaną sieć, która spowodowała poprzedni crash."
  //
  // DOWÓD z logu 17 (patrz plan_poprawek_WDT.md, Priorytet 3b): po WDT crashu urządzenie
  // potrzebowało 4 kolejnych rebootów; tylko OSTATNI zdążył zalogować "[WIFI] status
  // zmieniony" (reconnect) — pozostałe 3 padły, ZANIM WiFi w ogóle wróciło. To pokazuje
  // restart wchodzący w kółko w tę samą wciąż niestabilną sieć, nie 4 niezależne przyczyny.
  //
  // WARUNEK: backoff tylko gdy bootResetReason to WDT/PANIC (nie ręczny SW-restart z
  // Telegrama/przycisku) ORAZ g_wdtHunt.streak>=2 — czyli ten sam checkpoint powtórzył
  // się co najmniej drugi raz z rzędu (ten sam próg, co istniejący alarm [WDT-HUNT]
  // wyżej, ~linia 10742). Pojedynczy, odosobniony crash NIE dostaje opóźnienia — tylko
  // potwierdzona pętla. g_wdtHunt.streak jest tu już policzony (blok PRE-RESET/WDT-HUNT
  // wykonuje się wcześniej w setup(), przed WiFi.begin()).
  //
  // SKALOWANIE: 1000ms << streak, streak clampowany do [2,5] → 4s/8s/16s/32s(cap).
  // Rośnie z liczbą powtórzeń, żeby dać sieci realnie więcej czasu przy uporczywej
  // pętli, ale nie karać nadmiernie pojedynczego, szybko mijającego zdarzenia.
  // RESEARCH: standardowy wzorzec exponential backoff przy niestabilnej sieci
  // (rekomendowany m.in. w dokumentacji troubleshootingowej ESP32 WiFi reconnect —
  // "immediately hammering... can worsen congestion, use exponential backoff capped").
  // esp_task_wdt_reset() w pętli — main task ma 15s budżet (patrz wdt_cfg wyżej),
  // więc backoff musi karmić WDT, nie może być pojedynczym delay() >15s.
  {
    bool _isCrashReset = (bootResetReason == ESP_RST_TASK_WDT ||
                           bootResetReason == ESP_RST_INT_WDT ||
                           bootResetReason == ESP_RST_WDT     ||
                           bootResetReason == ESP_RST_PANIC);
    if (_isCrashReset && g_wdtHunt.streak >= 2) {
      uint16_t _streakClamped = g_wdtHunt.streak > 5 ? 5 : g_wdtHunt.streak;
      unsigned long _backoffMs = 1000UL << _streakClamped;  // streak2=4s..streak5+=32s
      logPrintf("lvl=WARN tag=BACKOFF streak=%u checkpoint=%s reason=%d opoznienie=%lums msg=\"Crash-loop wykryty, backoff przed FB+TG reinit\"\n",
                (unsigned)g_wdtHunt.streak, g_wdtHunt.lastCp, (int)bootResetReason, _backoffMs);
      unsigned long _boT = millis();
      while (millis() - _boT < _backoffMs) {
        esp_task_wdt_reset();
        delay(200);
      }
      logPrintln("lvl=INFO tag=BACKOFF msg=\"Zakonczony, kontynuuje FB+TG reinit\"");
    }
  }

  // ── [v105-FC] Inicjalizacja FirebaseClient PRZED uruchomieniem tgTask ────────────────
  // fbInitialize() wywoływana raz po WiFi.connected() i przed startem tgTask (Core 0).
  // Konfiguruje: fbSSLClient.setInsecure() + LegacyToken auth + fbDatabase.url().
  // tgTask używa gotowego fbApp przez fbDatabase.set/get/remove w sendStatusToFirebase()
  // checkFirebaseCommands() i checkFirebaseConfig().
  fbInitialize();

  // ── v43: start Telegram task na Core 0 - stos z PSRAM (nie obciąża wewnętrznego heap) ──
  tgCmdQueue = xQueueCreate(8, sizeof(TgDeferCmd));  // max 8 komend w kolejce
  appCmdQueue = xQueueCreate(16, sizeof(AppCommand));  // [v250-ARCH]
  fbAckQueue = xQueueCreate(4, sizeof(FirebaseAck));  // [v250-ARCH] ACK Core 1 -> Core 0
  storageQueue = xQueueCreate(8, sizeof(StorageRequest));  // [v257] Core0 storage owner
  fbConfigApplyQueue = xQueueCreate(4, sizeof(FirebaseConfigSnapshot));  // [v257] Core0->Core1
  fbConfigPersistQueue = xQueueCreate(2, sizeof(FirebaseConfigSnapshot)); // [v258] Core1->Core0 durable config snapshots
  fbConfigAckQueue = xQueueCreate(4, sizeof(FirebaseConfigAck));            // [v257] Core1->Core0
  fbConfigPersistAckQueue = xQueueCreate(2, sizeof(FirebaseConfigPersistAck)); // [v258] Core0->Core1
  if (tgCmdQueue) {
    // Stos 16KB alokowany z PSRAM - wewnętrzny heap zyskuje ~12KB ciągłego bloku.
    // Fallback: jeśli ps_malloc niedostępny, standard xTaskCreate z 12KB (jak v42).
    // KRYTYCZNE: stos taska MUSI być w DRAM, nigdy w PSRAM.
    // PSRAM wymaga cache SPI. Operacje flash (LittleFS, EEPROM, TLS)
    // wyłączają cache -> PSRAM niedostępny -> stos niedostępny ->
    // assert: esp_task_stack_is_sane_cache_disabled() -> crash co ~16s.
    // heap_caps_malloc z MALLOC_CAP_INTERNAL wymusza alokację z DRAM.
    static StaticTask_t tgTaskTCB;
    static uint8_t* tgStackBuf = nullptr;
    // [v62-3] Stack 16384→12288: watermark z logów ~4888 B użyte, margines ~7 kB (58%).
    // [v122] Plan "zmniejszyć do 10240 w v63" ODRZUCONY: zużycie wzrosło do ~6112B
    // (margines ~50%, nie 58%) — zmniejszanie byłoby działaniem wbrew danym z logów.
    // 12288B zostaje. Ręczna obserwacja zastąpiona automatycznym alarmem 🚨 w
    // diagHeap() gdy tgStack_free<3000B (patrz CHANGELOG v122) — nie trzeba już
    // ręcznie skanować logów żeby wychwycić degradację.
    const uint32_t TG_STACK = 12288;
    tgStackBuf = (uint8_t*)heap_caps_malloc(TG_STACK, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (tgStackBuf) {
      tgTaskHandle = xTaskCreateStaticPinnedToCore(
        tgTaskFn, "tgTask", TG_STACK, nullptr, 1,
        tgStackBuf, &tgTaskTCB, 0
      );
      logPrintln("lvl=INFO tag=TG-TASK msg=\"Telegram task uruchomiony na Core 0\" stos=DRAM12KB");
    } else {
      // Fallback - standardowy xTaskCreate (alokuje z DRAM)
      xTaskCreatePinnedToCore(tgTaskFn, "tgTask", 12288, nullptr, 1, &tgTaskHandle, 0);
      logPrintln("lvl=INFO tag=TG-TASK msg=\"Telegram task uruchomiony na Core 0\" stos=DRAM12KB-fallback");
    }
  } else {
    logPrintln("lvl=ERR tag=TG-TASK msg=\"Nie udalo sie utworzyc kolejki, Telegram wylaczony\"");
  }

  Serial.println(">> SETUP KOMPLETNY"); Serial.flush();

  // [NETDIAG] Inicjalizacja testu routera/łącza - PO całej sekcji WiFi/SNTP
  // (zgodnie z zasadą v239: nic nowego sieciowego tuż przed/w trakcie SNTP).
  // Sama funkcja tylko czyta WiFi.gatewayIP() (lokalnie, bez DNS) i zeruje
  // liczniki - realne testy TCP ruszają dopiero z tgTaskFn (Core 0).
  netDiagInit();

  // [OK] Start w trybie IDLE
}













// ══════════════════════════════════════════════════════════
//  [FIX race-audit 2026-08-13] Single-writer rollover energii - Core 0
// ══════════════════════════════════════════════════════════
// Wołane wyłącznie z tgTaskFn (Core 0), w reakcji na energyRolloverPending
// ustawione przez logDailySummary() (Core 1). Logika 1:1 przeniesiona z
// dawnego bloku w logDailySummary() - patrz komentarz przy ustawieniu flagi.
// Czas pobierany lokalnie (getLocalTimePL) zamiast przekazywany z Core 1:
// to czysty odczyt zegara systemowego (bezpieczny z obu rdzeni), a okno między
// detekcją na Core 1 a konsumpcją tutaj to najwyżej pojedyncze wywołanie
// tgTaskFn (~do 200 ms) - nieporównywalnie krócej niż doba, więc nie ma ryzyka
// złapania innego dnia niż ten wykryty przez logDailySummary().
void applyEnergyRollover() {
  struct tm ti;
  if (!getLocalTimePL(&ti)) return;  // zabezpieczenie - w praktyce Core 1 już to sprawdził

  // [OK] FIX-v29-11: Zapisz energię zamkniętego dnia do tablicy historii (Pn=0..Nd=6)
  {
    int isoDay = (ti.tm_wday + 6) % 7;  // 0=Pn..6=Nd
    energyDayHistory[isoDay] = energyTodayWh;
  }
  esp_task_wdt_reset();          // [v227] reset między zapisem historii a saveEnergyStats
  saveEnergyStats();          // Zapisz tygodniowe/miesięczne do LittleFS
  esp_task_wdt_reset();          // [v227] reset po całości bloku (spójnie z histSavePending)
  // [OK] FIX-v16-DAILY-LOG: Loguj energię PRZED zerowaniem - poprzednio energyTodayWh
  // był już = 0.0f w momencie logPrintf -> podsumowanie zawsze pokazywało 0.00 Wh.
  logPrintf("lvl=INFO tag=ENERGIA msg=\"raport dzienny\" dzisWh=%.1f tydzienWh=%.1f miesiacWh=%.1f\n",
            energyTodayWh, energyWeekWh, energyMonthWh);
  energyTodayWh      = 0.0f;
  ledOnMinutesToday  = 0;
  luxHoursTodayWater = 0.0f;
  minLuxActivToday   = 0;
  peakPowerWToday    = 0.0f;
  _minLuxWasActive   = false;
  memset(powerHourWh, 0, sizeof(powerHourWh));  // reset wykresu godzinowego

  // [OK] FIX: Reset tygodniowy w poniedziałek (tm_wday==1)
  if (ti.tm_wday == 1) {
    logPrintln("lvl=INFO tag=ENERGIA msg=\"Nowy tydzien, reset licznikow tygodniowych\"");
    // [OK] FIX-v29-11: zapisz zamknięty tydzień do historii tygodniowej (indeks = numer tygodnia miesiąca)
    {
      int wkIdx = (ti.tm_mday - 1) / 7;  // 0-bazowany numer tygodnia w miesiącu
      if (wkIdx < 0) wkIdx = 0;
      if (wkIdx > 4) wkIdx = 4;
      energyWeekHistory[wkIdx] = energyWeekWh;
    }
    for (int i = 0; i < 7; i++) energyDayHistory[i] = 0.0f;
    energyWeekWh      = 0.0f;
    ledOnMinutesWeek  = 0;
    minLuxActivWeek   = 0;
  }
  // [OK] FIX: Reset miesięczny 1. dnia miesiąca
  if (ti.tm_mday == 1) {
    logPrintln("lvl=INFO tag=ENERGIA msg=\"Nowy miesiac, reset licznikow miesiecznych\"");
    for (int i = 0; i < 5; i++) energyWeekHistory[i] = 0.0f;  // [OK] FIX-v29-11: wyczyść historię tygodni przy nowym miesiącu
    energyMonthWh     = 0.0f;
    ledOnMinutesMonth = 0;
  }
}

// ══════════════════════════════════════════════════════════
//  ENERGIA - zapis / wczytanie do LittleFS
// ══════════════════════════════════════════════════════════
#define ENERGY_STATS_FILE "/energy_stats.json"

void saveEnergyStats() {
  if (!littlefsReady) return;
  struct tm ti;
  if (!getLocalTimePL(&ti)) return;
  File f = LittleFS.open(ENERGY_STATS_FILE, "w");
  if (!f) return;
  char buf[320];
  // [OK] FIX: klucz tygodnia = dzień roku poniedziałku tego tygodnia (ISO)
  //        zamiast yday/7, który dzielił rok na bloki 7-dniowe od 1 stycznia
  int isoMon = ti.tm_yday - ((ti.tm_wday + 6) % 7);  // yday poniedziałku bieżącego tygodnia
  snprintf(buf, sizeof(buf),
    "{\"todayWh\":%.2f,\"weekWh\":%.2f,\"monthWh\":%.2f"
    ",\"ledMin\":%u,\"ledMinWk\":%u,\"ledMinMo\":%u"
    ",\"luxH\":%.2f,\"mlAct\":%u"
    ",\"peakW\":%.1f,\"mlWk\":%u"
    ",\"day\":%d,\"wk\":%d,\"mo\":%d,\"yr\":%d}",
    energyTodayWh, energyWeekWh, energyMonthWh,
    ledOnMinutesToday, ledOnMinutesWeek, ledOnMinutesMonth,
    luxHoursTodayWater, minLuxActivToday,
    peakPowerWToday, minLuxActivWeek,
    ti.tm_yday, isoMon, ti.tm_mon, ti.tm_year);
  // [OK] FIX-v29-11: dopisz tablicę historii dni (Pn=d0..Nd=d6)
  // [FIX-v166-JSON-DUP] Poprzednio dayBuf (z domykającym "}") był drukowany
  // W CAŁOŚCI, a zaraz potem ponownie jako "dayStr" (ten sam bufor, tylko bez
  // ostatniego "}") - d0..d6 trafiało do pliku DWA razy, a JSON miał dodatkowy
  // "}" w środku (obiekt zamykał się przedwcześnie po pierwszym dayBuf, po czym
  // plik dopisywał kolejne klucze POZA już zamkniętym obiektem = invalid JSON).
  // Dowód z energy_stats.json: {...,"yr":126,"d0":0.00,...,"d6":0.00},"d0":0.00,...
  // Teraz: każdy bufor jest budowany BEZ własnego zamykającego "}" i drukowany
  // dokładnie raz; jedyny "}" na końcu pliku pochodzi z wkBuf.
  char dayBuf[120];
  snprintf(dayBuf, sizeof(dayBuf), ",\"d0\":%.2f,\"d1\":%.2f,\"d2\":%.2f,\"d3\":%.2f,\"d4\":%.2f,\"d5\":%.2f,\"d6\":%.2f",
           energyDayHistory[0], energyDayHistory[1], energyDayHistory[2],
           energyDayHistory[3], energyDayHistory[4], energyDayHistory[5], energyDayHistory[6]);
  // [OK] FIX-v29-11: dopisz tablicę historii tygodni (w0..w4) - jedyny "}" na końcu
  char wkBuf[80];
  snprintf(wkBuf, sizeof(wkBuf), ",\"w0\":%.2f,\"w1\":%.2f,\"w2\":%.2f,\"w3\":%.2f,\"w4\":%.2f}",
           energyWeekHistory[0], energyWeekHistory[1], energyWeekHistory[2],
           energyWeekHistory[3], energyWeekHistory[4]);
  // Zamień zamykające "}" głównego bufora na rozszerzony JSON
  String saved = String(buf);
  saved = saved.substring(0, saved.length()-1); // usuń ostatni }
  f.print(saved);
  f.print(dayBuf);
  f.print(wkBuf);
  // FIX-v44: usunięto f.print(buf) - powodowało podwójny zapis (malformed JSON, plik 2x dłuższy).
  f.close();
}

// [v88] PATCH-2: parametr zmieniony z const String& na const char* — zero heap alloc
static float parseJsonFloat(const char* s, const char* key) {
  // FIX-v44: strstr na c_str() - zero alokacji (było: String k + substring = 2 heap alloc/wywołanie)
  char needle[48]; snprintf(needle, sizeof(needle), "\"%s\":", key);
  const char* p = strstr(s, needle);
  if (!p) return 0.0f;
  return atof(p + strlen(needle));
}
// [v88] PATCH-2: parametr zmieniony z const String& na const char* — zero heap alloc
static int parseJsonInt(const char* s, const char* key) {
  // FIX-v44: strstr na c_str() - zero alokacji (było: String k + substring = 2 heap alloc/wywołanie)
  char needle[48]; snprintf(needle, sizeof(needle), "\"%s\":", key);
  const char* p = strstr(s, needle);
  if (!p) return -1;
  return atoi(p + strlen(needle));
}

void loadEnergyStats() {
  if (!littlefsReady) return;
  if (!LittleFS.exists(ENERGY_STATS_FILE)) {
    logPrintln("lvl=INFO tag=ENERGIA msg=\"Brak pliku energy_stats.json, pierwsze uruchomienie\"");
    return;
  }
  File f = LittleFS.open(ENERGY_STATS_FILE, "r");
  if (!f) {
    logPrintln("lvl=ERR tag=ENERGIA msg=\"Nie mozna otworzyc energy_stats.json\"");
    return;
  }
  // [v88] PATCH-2: bufor na stosie zamiast f.readString() — zero heap alloc (było: 640 realloc/odczyt)
  char s_buf[768];  // 640 B plik + zapas; na stosie, zero heap alloc
  int _n = f.read((uint8_t*)s_buf, sizeof(s_buf) - 1);
  s_buf[_n > 0 ? _n : 0] = '\0';
  f.close();
  const char* s = s_buf;

  // [OK] FIX-v29-NTP: Ta funkcja MUSI być wywoływana po synchronizacji NTP.
  // Bez poprawnego czasu getLocalTimePL() zwraca false -> early return -> liczniki = 0.
  // Wywołanie przeniesione do sekcji WiFi+NTP w setup(), po potwierdzeniu czasu.
  struct tm ti;
  if (!getLocalTimePL(&ti)) {
    logPrintln("lvl=ERR tag=ENERGIA msg=\"Brak czasu NTP, nie wczytano statystyk (wywolaj po NTP!)\"");
    return;
  }

  int savedDay = parseJsonInt(s, "day");
  int savedWk  = parseJsonInt(s, "wk");
  int savedMo  = parseJsonInt(s, "mo");
  int savedYr  = parseJsonInt(s, "yr");

  // [OK] FIX-v29-YR: Jeśli savedYr == -1 (brak pola "yr" w starym formacie pliku),
  // traktujemy plik jako nieważny - zapisany przez starszą wersję firmware bez pola roku.
  if (savedYr < 0) {
    logPrintln("lvl=WARN tag=ENERGIA msg=\"Plik energy_stats.json bez pola 'yr' (stary format), pomijam\"");
    return;
  }

  // [OK] FIX-v28b-WEEK: Porównanie zakresu dni zamiast klucza ISO poniedziałku.
  int curIsoMon = ti.tm_yday - ((ti.tm_wday + 6) % 7);
  bool sameDay  = (savedDay == ti.tm_yday && savedYr == ti.tm_year);
  bool sameWeek = (savedYr == ti.tm_year)
                  && (savedDay >= curIsoMon)
                  && (savedDay <= curIsoMon + 6);
  bool sameMon  = (savedMo  == ti.tm_mon  && savedYr == ti.tm_year);

  // [OK] FIX-v29-DIAG: Pełna diagnostyka przy każdym wczytaniu
  logPrintf("lvl=INFO tag=ENERGIA msg=\"LOAD\" savedDay=%d curDay=%d savedYr=%d curYr=%d "
            "sameDay=%d sameWeek=%d sameMon=%d\n",
            savedDay, ti.tm_yday, savedYr, ti.tm_year,
            (int)sameDay, (int)sameWeek, (int)sameMon);

  energyTodayWh = sameDay  ? parseJsonFloat(s, "todayWh") : 0.0f;
  energyWeekWh  = sameWeek ? parseJsonFloat(s, "weekWh")  : 0.0f;
  energyMonthWh = sameMon  ? parseJsonFloat(s, "monthWh") : 0.0f;

  // [OK] Dzienne liczniki - wczytaj tylko jeśli ten sam dzień
  if (sameDay) {
    int ledMin = parseJsonInt(s, "ledMin");
    ledOnMinutesToday  = (ledMin  >= 0) ? (uint32_t)ledMin  : 0;
    float luxH = parseJsonFloat(s, "luxH");
    luxHoursTodayWater = (luxH >= 0.0f) ? luxH : 0.0f;
    int mlAct  = parseJsonInt(s, "mlAct");
    minLuxActivToday   = (mlAct  >= 0) ? (uint32_t)mlAct  : 0;
    float pkW  = parseJsonFloat(s, "peakW");
    peakPowerWToday    = (pkW >= 0.0f) ? pkW : 0.0f;
  }
  if (sameWeek) {
    int mlWk = parseJsonInt(s, "mlWk");
    minLuxActivWeek = (mlWk >= 0) ? (uint32_t)mlWk : 0;
    int ledMinWk = parseJsonInt(s, "ledMinWk");
    ledOnMinutesWeek = (ledMinWk >= 0) ? (uint32_t)ledMinWk : 0;
    // [OK] FIX-v29-11: wczytaj historię dni tygodnia
    const char* dkeys[7] = {"d0","d1","d2","d3","d4","d5","d6"};
    for (int i = 0; i < 7; i++) {
      float dv = parseJsonFloat(s, dkeys[i]);
      energyDayHistory[i] = (dv >= 0.0f) ? dv : 0.0f;
    }
  } else {
    // Nowy tydzień - wyczyść historię
    for (int i = 0; i < 7; i++) energyDayHistory[i] = 0.0f;
  }
  if (sameMon) {
    int ledMinMo = parseJsonInt(s, "ledMinMo");
    ledOnMinutesMonth = (ledMinMo >= 0) ? (uint32_t)ledMinMo : 0;
    // [OK] FIX-v29-11: wczytaj historię tygodni miesiąca
    const char* wkeys[5] = {"w0","w1","w2","w3","w4"};
    for (int i = 0; i < 5; i++) {
      float wv = parseJsonFloat(s, wkeys[i]);
      energyWeekHistory[i] = (wv >= 0.0f) ? wv : 0.0f;
    }
  } else {
    for (int i = 0; i < 5; i++) energyWeekHistory[i] = 0.0f;
  }

  logPrintf("lvl=INFO tag=ENERGIA msg=\"Wczytano\" dzis=%.1fWh ledMin=%u tydz=%.1fWh ledMinWk=%u mies=%.1fWh ledMinMo=%u\n",
            energyTodayWh, ledOnMinutesToday,
            energyWeekWh,  ledOnMinutesWeek,
            energyMonthWh, ledOnMinutesMonth);
  // Jeśli nowy dzień/tydzień/miesiąc - zostają 0 (inicjalizowane przy deklaracji)
}

// ══════════════════════════════════════════════════════════
//  HISTORIA 24H - zapis punktu do LittleFS co 5 minut
// ══════════════════════════════════════════════════════════
// ══════════════════════════════════════════════════════════
//  STATYSTYKI ADAPTACJI - zapis/odczyt z LittleFS
// ══════════════════════════════════════════════════════════
#define ADAPT_STATS_FILE "/adapt_stats.json"

void saveAdaptStats() {
  if (!littlefsReady) return;
  File f = LittleFS.open(ADAPT_STATS_FILE, "w");
  if (!f) return;
  char buf[160];
  float avg = (statystyki.liczbaPomiarow > 0)
    ? (statystyki.sredniaRedukcja / statystyki.liczbaPomiarow)
    : 0.0f;
  // Upewnij się że avg jest liczbą skończoną przed zapisem
  if (isnan(avg) || isinf(avg) || avg < 0.0f) avg = 0.0f;
  snprintf(buf, sizeof(buf),
    "{\"korMin\":%u,\"korMax\":%u,\"pomiary\":%u,\"sredniaRed\":%.4f,\"zaoszcz\":%.4f}",
    statystyki.korektyMinimum,
    statystyki.korektyMaksimum,
    statystyki.liczbaPomiarow,
    avg,  // [OK] FIX: zapisujemy ŚREDNIĄ (avg), a nie surową sumę (sredniaRedukcja)!
          //    Poprzednio: statystyki.sredniaRedukcja (sumę) - powodowało nieograniczony wzrost
          //    wartości po każdym restarcie i potencjalne problemy przy ładowaniu.
    statystyki.zaoszczedzonaEnergia);
  f.print(buf);
  f.close();
}

void loadAdaptStats() {
  if (!littlefsReady) return;
  if (!LittleFS.exists(ADAPT_STATS_FILE)) return;
  File f = LittleFS.open(ADAPT_STATS_FILE, "r");
  if (!f) return;
  String s = f.readString();
  f.close();
  uint32_t korMin   = (uint32_t)parseJsonInt(s.c_str(), "korMin");
  uint32_t korMax   = (uint32_t)parseJsonInt(s.c_str(), "korMax");
  uint32_t pomiary  = (uint32_t)parseJsonInt(s.c_str(), "pomiary");
  float    srednia  = parseJsonFloat(s.c_str(), "sredniaRed");
  float    zaoszcz  = parseJsonFloat(s.c_str(), "zaoszcz");

  // [OK] Walidacja - jeśli wartości nierealne (uszkodzony plik), zignoruj i skasuj
  const uint32_t MAX_ROZSADNE = 50000UL;  // >50k korekt to na pewno błąd
  if (korMin > MAX_ROZSADNE || korMax > MAX_ROZSADNE || pomiary > MAX_ROZSADNE
      || isnan(srednia) || isinf(srednia)   // [OK] FIX: dodano isinf(srednia) - wcześniej brakowało!
      || isinf(zaoszcz) || isnan(zaoszcz)   // [OK] FIX: dodano isnan(zaoszcz)
      || srednia < 0.0f || srednia > 100.0f) {  // [OK] FIX: srednia (avg %) nie może być >100%
    logPrintf("lvl=WARN tag=ADAPT msg=\"Plik statystyk uszkodzony, kasowanie\" korMin=%u\n", korMin);
    LittleFS.remove(ADAPT_STATS_FILE);
    return;  // zmienne pozostają = 0
  }

  statystyki.korektyMinimum       = korMin;
  statystyki.korektyMaksimum      = korMax;
  statystyki.liczbaPomiarow       = pomiary;
  // [OK] FIX: Po naprawie saveAdaptStats(), plik teraz zawiera ŚREDNIĄ (avg), a nie sumę.
  // Przywracamy sumę jako avg * pomiary, żeby dalsze kumulowanie działało poprawnie.
  statystyki.sredniaRedukcja      = (pomiary > 0) ? (srednia * (float)pomiary) : 0.0f;
  statystyki.zaoszczedzonaEnergia = zaoszcz;
  logPrintf("lvl=INFO tag=ADAPT msg=\"Wczytano statystyki\" korMin=%u korMax=%u pomiary=%u\n",
    statystyki.korektyMinimum,
    statystyki.korektyMaksimum,
    statystyki.liczbaPomiarow);
}

void saveHistoryPoint() {
  PRE_RESET_CP("HIST-SAVE");  // [v71]
  if (!littlefsReady) return;
  if (historyClearPending) return;  // czyszczenie w toku - nie nadpisuj

  struct tm ti;
  char timeBuf[6] = "--:--";
  if (getLocalTimePL(&ti)) {
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", ti.tm_hour, ti.tm_min);
  }

  int pwmPct[5];
  float chPowerW[5];    // moc realna każdego kanału [W]
  float totalPowerW = 0.0f;
  bool anyLedOn = false;
  for (int i = 0; i < 5; i++) {
    uint16_t pwmRaw = getBrightnessForSection(backupBrightnessComposite, i);
    pwmPct[i]  = (int)(pwmRaw * 100 / 1023);
    chPowerW[i] = getPowerForChannel(i, pwmRaw);
    totalPowerW += chPowerW[i];
    if (pwmRaw > 0) anyLedOn = true;
  }

  // ── Akumulacja energii / LED-czasu / lux-godzin (co 5 minut = 1/12 godziny) ──
  const float INTERVAL_H = 5.0f / 60.0f;
  float pointWh = totalPowerW * INTERVAL_H;   // Wh = W × h (realna moc z pomiarów)
  energyTodayWh      += pointWh;
  energyWeekWh       += pointWh;
  energyMonthWh      += pointWh;
  if (anyLedOn) { ledOnMinutesToday += 5; ledOnMinutesWeek += 5; ledOnMinutesMonth += 5; }
  luxHoursTodayWater += (luxNadWoda * INTERVAL_H);

  // ── Detekcja aktywacji MIN LUX (krawędź narastająca) ──
  if (minLuxModeActive && !_minLuxWasActive) { minLuxActivToday++; minLuxActivWeek++; }
  _minLuxWasActive = minLuxModeActive;
  // Aktualizuj szczytową moc dziś
  if (totalPowerW > peakPowerWToday) peakPowerWToday = totalPowerW;

  // ── Wykres godzinowy mocy: akumuluj Wh w bieżącej godzinie ──
  {
    struct tm tiH;
    if (getLocalTimePL(&tiH)) {
      int hSlot = tiH.tm_hour;  // 0..23
      if (hSlot >= 0 && hSlot < 24) powerHourWh[hSlot] += pointWh;
    }
  }

  // [OK] Dodatkowe kolumny: tryb, minLuxAktywny, regulacjaAdaptacyjna, lux_cel
  // Format: czas,t1,t2,tw,lp,lw,pwm0..4,tryb,minLux,adapt,lux_cel
  char line[160];
  snprintf(line, sizeof(line),
    "%s,%.1f,%.1f,%.1f,%.0f,%.0f,%d,%d,%d,%d,%d,%s,%s,%s,%.0f\n",
    timeBuf,
    tempPlate1, tempPlate2, tempWater,
    luxPokojowy, luxNadWoda,
    pwmPct[0], pwmPct[1], pwmPct[2], pwmPct[3], pwmPct[4],
    tryb ? "AUTO" : "MAN",
    minLuxModeActive ? "1" : "0",
    regulacjaAdaptacyjnaWlaczona ? "1" : "0",
    minLuxDayTarget
  );

  // [v87] OPT-B: cache liczby linii + odtworzenie histStartLine po restarcie
  bool hasHeader = (histLineCount > 0);  // po pierwszym zapisie nagłówek zawsze jest
  if (histLineCount < 0) {
    // Pierwszy zapis po starcie — skanuj plik raz (potem zawsze cache)
    int lineCount = 0;
    hasHeader = false;
    if (LittleFS.exists(HISTORY_FILE)) {
      File f = LittleFS.open(HISTORY_FILE, "r");
      if (f) {
        String firstLine = f.readStringUntil('\n');
        hasHeader = firstLine.startsWith("czas,");
        uint8_t buf[512];
        int _chunkN = 0;
        while (f.available()) {
          int r = f.read(buf, sizeof(buf));
          for (int i = 0; i < r; i++) { if (buf[i] == '\n') lineCount++; }
          // [v136] FIX-HIST-SAVE-WDT: esp_task_wdt_reset() co ~16 KB (32 × 512 B).
          // ROOT CAUSE: to jest PIERWSZA operacja na history.csv po restarcie (jeszcze
          // przed HIST-COMPACT) — ten sam wzorzec "pętla I/O bez wdt_reset" co już raz
          // crashował tgTask (patrz FIX-v135-HIST-COMPACT-WDT wyżej). Plik po restarcie
          // może mieć kilkaset KB, więc bez resetu to potencjalnie WCZEŚNIEJSZY punkt
          // crashu niż HIST-COMPACT. Bezpieczne: ta funkcja biegnie w tgTaskFn, który
          // jest zarejestrowany w TWDT (esp_task_wdt_add(NULL) na starcie taska).
          if ((++_chunkN & 0x1F) == 0) esp_task_wdt_reset();  // co 32 chunki (~16 KB)
        }
        f.close();
      }
    }
    histLineCount = lineCount;
    // [v87] Odtwórz histStartLine: ile linii jest "martwych" po restarcie
    // (plik mógł mieć 288–576 linii gdy urządzenie się zresetowało)
    histStartLine = max(0, histLineCount - HISTORY_MAX_POINTS);
    if (histStartLine > 0)
      logPrintf("lvl=INFO tag=HIST msg=\"Start po resecie\" linecount=%d startline=%d live=%d\n",
                histLineCount, histStartLine, histLineCount - histStartLine);
  }

  // [v87] OPT-B: Rolling buffer BEZ kopiowania pliku
  // "liveLines" = ile punktów aktualnie w oknie 288
  int liveLines = histLineCount - histStartLine;
  if (liveLines >= HISTORY_MAX_POINTS) {
    histStartLine++;   // "zapomnij" najstarszą linię — zero I/O!
    // Gdy połowa pliku to martwe linie → zaplanuj kompaktowanie (raz na ~24h)
    if (histStartLine >= HISTORY_MAX_POINTS && !compactHistPending) {
      compactHistPending = true;
      logPrintf("lvl=INFO tag=HIST msg=\"Kompaktowanie zaplanowane\" startline=%d linecount=%d\n",
                histStartLine, histLineCount);
    }
  }

  // Dopisz nagłówek jeśli brak, potem nową linię (zawsze tylko append — szybkie)
  File f = LittleFS.open(HISTORY_FILE, "a");
  if (f) {
    if (!hasHeader && histLineCount == 0) {
      f.println("czas,t_plyta1,t_plyta2,t_woda,lux_pokoj,lux_woda,pwm0_biale,pwm1_fs,pwm2_fs_biale,pwm3_nieb,pwm4_czerw,tryb,minLux_aktywny,adapt_aktywna,lux_cel");
    }
    f.print(line);
    f.close();
    histLineCount++;  // [OK] FIX-v33f: inkrementuj cache
  }
}

// ─── FIREBASE persistent client — fbConnect() [v58] ──────────────────────────
// Analogia do tgConnect() dla Telegrama (v32). Jeden kontekst TLS na cały
// czas działania; reconnect tylko gdy idle >55s lub połączenie zerwane.
// Eliminuje spike ~47 kB DRAM przy każdym wywołaniu Firebase.
// ─────────────────────────────────────────────────────────────────────────────
// [v105-FC] fbInitialize() — zastąpił bool fbConnect() (307 linii, v65→v104)
//
// ARCHITEKTURA (przed → po):
//   PRZED: WiFiClientSecure fbClient + HTTPClient + ręczne batch-reuse, TCP keepalive,
//     DNS cache, SO_SNDTIMEO, TLS mutex, heap tracking, 8 generacji łatek (v65→v104)
//   PO:    FirebaseClient z wbudowanym ESP_SSLClient. Prawidłowy connected() check.
//     Brak konieczności ręcznego zarządzania TCP/TLS. Jedno wywołanie w setup().
//
// WYWOŁANIE:
//   1. Raz w setup() po WiFi.connected() (przed startem tgTask)
//   2. Ponownie z wnętrza sendStatusToFirebase/checkFirebaseCommands/checkFirebaseConfig
//      gdy fbAppReady==false lub fbApp.ready()==false (automatyczna reinicjalizacja)
//
// UWAGA: LegacyToken (FIREBASE_SECRET) nie wygasa — initializeApp() dla tego
//   typu nie wykonuje HTTP roundtrip do serwera auth. fbApp.ready() wraca true
//   prawie natychmiast (< 100ms). Nie ma potrzeby długiego czekania.
// ─────────────────────────────────────────────────────────────────────────────
void fbInitialize() {
  if (fbAppReady && fbApp.ready()) {
    // Już zainicjalizowany i gotowy — idempotent, skip
    if (Komentarze) logPrintf("lvl=INFO tag=FB-INIT msg=\"juz gotowy - skip\"\n");
    return;
  }

  unsigned long _fbInitT = millis();
  logPrintf("lvl=INFO tag=FB-INIT msg=\"FirebaseClient v105-FC inicjalizacja\" fH=%luB\n",
            (unsigned long)ESP.getFreeHeap());

  // [v227] FIX-TLS-HANDSHAKE-TIMEOUT: ta sama luka i ten sam fix co po stronie TG
  // (patrz komentarz w tgEnsureConnected, ~linia 8309) — fbSSLClient to zwykły
  // WiFiClientSecure (deklaracja ~linia 7003), więc ma tę samą metodę. FirebaseClient
  // wywołuje connect() na fbSSLClient WEWNĘTRZNIE (nie mamy bezpośredniego dostępu do
  // tego wywołania), ale ustawienie tutaj — w jedynym miejscu, gdzie faktycznie
  // odtwarzamy połączenie po fbSSLClient.stop() — obejmuje każdą kolejną próbę.
  // Uwaga (issue #6077): handshake_timeout resetuje się do 0 po każdym stop(), więc
  // MUSI być ustawiane tutaj (przy każdym realnym reinit), nie tylko raz w setup().
  fbSSLClient.setHandshakeTimeout(4000);

  // [v113-FIX-1] DNS pre-warm: WiFi.hostByName(FIREBASE_HOST) przed initializeApp().
  // Cel: załadować IP do cache DNS lwIP z kontekstu taska (safe — hostByName
  // dispatchuje do tcpip_thread wewnętrznie, nie wymaga TCPIP_CORE_LOCK z user-taska).
  // FirebaseClient zarządza własnymi połączeniami, więc NIE przechowujemy IP
  // (g_fbServerIP usunięte w v105-FC) — samo wywołanie wypełnia cache lwIP.
  // Gdy FirebaseClient wewnętrznie wywoła getaddrinfo()/udp_new_ip_type() przy
  // pierwszym connect(), IP jest już w cache → brak synchronicznego DNS → brak blokady.
  // Wzorzec sprawdzony dla Telegrama od v75 (g_tgServerIP, TTL 5 min).
  // Ref: assert "udp_new_ip_type failed" w logach (12% resetów, Crash B).
  // [FIX-v153-FB-INIT-RETRY] DNS pre-warm z retry+backoffem (Priorytet 3b, pkt 1).
  // RESEARCH: "DNS Failed" tuż po (re)connect WiFi to znany, wielokrotnie zgłaszany
  // wzorzec w arduino-esp32 (issue #4320, #2778, #1595, #8857) — lwIP/DHCP potrafi
  // nie mieć jeszcze w pełni gotowego stosu (serwer DNS z DHCP, routing) w pierwszych
  // ułamkach sekundy po WL_CONNECTED, mimo że WiFi.status() już raportuje połączenie.
  // Jedno nieudane hostByName() tuż po starcie/reconnect nie musi więc oznaczać
  // realnego braku sieci — może to być przejściowy stan. Poprzednio: jedna próba,
  // brak retry → fbInitialize() od razu szedł dalej bez ciepłego cache DNS, ryzykując
  // późniejszy synchroniczny getaddrinfo()/udp_new_ip_type() (Crash B z historii, 12%).
  // FIX: max 2 próby, 500ms odstępu — małe, ograniczone opóźnienie (WDT main task=15s,
  // więc 500ms nie zagraża budżetowi), ale realnie łapie przejściowy przypadek.
  {
    IPAddress _fbResolved;
    bool _dnsOk = false;
    for (int _dnsAttempt = 1; _dnsAttempt <= 2 && !_dnsOk; _dnsAttempt++) {
      esp_task_wdt_reset();  // świeże okno WDT przed każdą próbą DNS (może trwać ~1–4s)
      // [v162] DIAG-DNSSTALL: ten sam pomiar co w tgEnsureConnected/
      // sendTelegramDocument — patrz komentarz tam. site=fbInitialize.
      unsigned long _fbDnsT0 = millis();
      _dnsOk = WiFi.hostByName(FIREBASE_HOST, _fbResolved);
      // [v164] FIX-DNS-ZERO-IP: log_combined__23_.txt (21:38-22:05) pokazał
      // WiFi.hostByName() zwracające true=sukces, mimo że zwrócony adres to
      // 0.0.0.0 (pusta/nieprawidłowa odpowiedź DNS) — w tym samym pliku poza
      // tym oknem DNS zawsze poprawnie zwracał 34.107.226.223, więc to była
      // chwilowa, realna awaria DNS w sieci lokalnej, nie stały problem.
      // Kod nigdy nie sprawdzał, czy zwrócony adres ma sens — traktował
      // 0.0.0.0 jak pełny sukces i próbował łączyć się z nieroutowalnym
      // adresem, co przyczyniło się do ~20 restartów WDT z rzędu. Fix:
      // odrzucić 0.0.0.0 jako fałszywy sukces, wymusić retry tak samo jak
      // przy realnym błędzie hostByName().
      if (_dnsOk && _fbResolved == IPAddress(0, 0, 0, 0)) {
        _dnsOk = false;
        logPrintf("lvl=WARN tag=DNS-ZERO-IP site=fbInitialize msg=\"hostByName() zwrocil 0.0.0.0, traktuje jako blad\"\n");
      }
      unsigned long _fbDnsMs = millis() - _fbDnsT0;
      if (_fbDnsMs >= DNS_STALL_THRESHOLD_MS) {
        g_dnsStallCount = g_dnsStallCount + 1;
        if (_fbDnsMs > g_dnsStallMaxMs) g_dnsStallMaxMs = _fbDnsMs;
        logPrintf("lvl=WARN tag=DNS-STALL site=fbInitialize ms=%lu ok=%d attempt=%d cnt=%lu maxMs=%lu\n",
                  _fbDnsMs, (int)_dnsOk, _dnsAttempt,
                  (unsigned long)g_dnsStallCount, (unsigned long)g_dnsStallMaxMs);
      }
      if (_dnsOk) {
        logPrintf("lvl=INFO tag=FB-INIT msg=\"DNS pre-warm OK\" proba=%d/2 host=%s ip=%s\n",
                  _dnsAttempt, FIREBASE_HOST, _fbResolved.toString().c_str());
      } else if (_dnsAttempt < 2) {
        logPrintf("lvl=WARN tag=FB-INIT msg=\"DNS pre-warm fail, retry za 500ms\" proba=%d/2 host=%s\n",
                  _dnsAttempt, FIREBASE_HOST);
        esp_task_wdt_reset();
        delay(500);
      } else {
        logPrintf("lvl=WARN tag=FB-INIT msg=\"DNS pre-warm fail po probach, brak sieci? kontynuuje\" proby=%d host=%s\n",
                  _dnsAttempt, FIREBASE_HOST);
      }
    }
    esp_task_wdt_reset();  // reset po DNS, świeże okno dla initializeApp()
  }

  // [v105-FC] setInsecure() na fbSSLClient — brak weryfikacji cert (jak fbConnect() poprzednio).
  // FirebaseClient opakowuje fbSSLClient wewnątrz ESP_SSLClient z prawidłowym peek().
  fbSSLClient.setInsecure();
  // UWAGA v111: WiFiClientSecure nie ma setSessionTimeout() (ta metoda należy do ESP_SSLClient).
  // Zamiast niej: fbSSLClient.stop() po TIMEOUT w każdej funkcji FB → connected()=false →
  // następne fbApp.loop() nie blokuje na SSL_read() martwego gniazda.

  // [v105-FC] initializeApp() — konfiguruje autentykację LegacyToken.
  // LegacyToken = FIREBASE_SECRET (Database Secret). Token nie wygasa.
  // Dla LegacyToken: nie wykonuje HTTP roundtrip do auth.googleapis.com.
  // fbApp.ready() wraca true natychmiast po skonfigurowaniu wewnętrznego stanu.
  initializeApp(fbAsyncClient, fbApp, getAuth(fbLegacyToken));

  // [v105-FC] Powiąż RealtimeDatabase z fbApp i ustaw URL bazy
  fbApp.getApp<RealtimeDatabase>(fbDatabase);
  String _fbUrl = "https://";
  _fbUrl += FIREBASE_HOST;
  fbDatabase.url(_fbUrl);

  // [v105-FC] Czekaj na gotowość (max 3s z WDT feed).
  // Dla LegacyToken powinno być <100ms — pętla to tylko bezpiecznik.
  unsigned long _t = millis();
  while (!fbApp.ready() && millis() - _t < 3000UL) {
    fbApp.loop();
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  esp_task_wdt_reset();

  fbAppReady = fbApp.ready();

  if (fbAppReady) {
    logPrintf("lvl=INFO tag=FB-INIT msg=\"ready\" t=%lums fH=%luB\n",
              millis() - _fbInitT, (unsigned long)ESP.getFreeHeap());
  } else {
    logPrintf("lvl=ERR tag=FB-INIT msg=\"timeout, fbAppReady=false\" t=%lums fH=%luB\n",
              millis() - _fbInitT, (unsigned long)ESP.getFreeHeap());
  }
}


// ─── FIREBASE-v2: sendStatusToFirebase() — ROZSZERZONY ────────────────────────
// Zastąp całą funkcję sendStatusToFirebase() w Ryby_LED_fi.ino tym kodem.
// Wysyła wszystkie dane potrzebne do Firebase panelu (harmonogram, energia,
// historia tygodnia, MIN LUX, adaptacja, moc per kanał, local IP).
// [FIX-v46] Jawny WiFiClientSecure + http.setTimeout(8000) + esp_task_wdt_reset()
//   - bez tego HTTPClient blokował tgTask >10s -> WDT crash (Watchdog timeout).
//   - http.begin(fbClient, url) zamiast http.begin(url) omija ścieżkę
//     udp_new() bez blokady TCPIP_CORE_LOCK -> eliminuje assert crash w IDF 5.x.
// [v58] Lokalny WiFiClientSecure zastąpiony globalnym fbClient (fbConnect()).

void sendStatusToFirebase() {
  // [v259] Compatibility wrapper. Active transport is owned by the native async scheduler.
  fbSendPending = true;
}
// ─────────────────────────────────────────────────────────────────────────────


// ─── FIREBASE-v1: checkFirebaseCommands() ────────────────────────────────────
static bool fbParseU64Field(const String& body, int valueIndex, uint64_t& out) {
  if (valueIndex < 0 || valueIndex >= body.length()) return false;
  const char* p = body.c_str() + valueIndex;
  while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
  if (!*p) return false;
  char* endp = nullptr;
  unsigned long long v = strtoull(p, &endp, 10);
  if (endp == p) return false;
  out = (uint64_t)v;
  return true;
}



static bool wykonajKomendeFirebase(const String& cmd, uint64_t firebaseTs = 0, const char* firebaseKey = nullptr) {
  if (cmd.length() == 0) return false;
  if (cmd == "config_refresh") {
    if (Komentarze) logPrintln("lvl=INFO tag=FB msg=\"config_refresh\"");
    fbCfgPending = true;
    return true;
  }
  fbTurboAktywuj(cmd.c_str());
  if (Komentarze) logPrintf("lvl=INFO tag=FB cmd=%s\n", cmd.c_str());
  if (cmd == "power_on")    return enqueueAppCommand(AppCommandType::POWER_SET, true, nullptr, "firebase", firebaseTs, firebaseKey);
  if (cmd == "power_off")   return enqueueAppCommand(AppCommandType::POWER_SET, false, nullptr, "firebase", firebaseTs, firebaseKey);
  if (cmd == "mode_auto")   return enqueueAppCommand(AppCommandType::TRYB_SET, true, nullptr, "firebase", firebaseTs, firebaseKey);
  if (cmd == "mode_manual") return enqueueAppCommand(AppCommandType::TRYB_SET, false, nullptr, "firebase", firebaseTs, firebaseKey);
  if (cmd == "pump_on")     return enqueueAppCommand(AppCommandType::PUMP_SET, true, nullptr, "firebase", firebaseTs, firebaseKey);
  if (cmd == "pump_off")    return enqueueAppCommand(AppCommandType::PUMP_SET, false, nullptr, "firebase", firebaseTs, firebaseKey);
  if (cmd == "led_test" || cmd == "led_100") return enqueueAppCommand(AppCommandType::LED_TEST, true, nullptr, "firebase", firebaseTs, firebaseKey);
  if (cmd == "led_off")     return enqueueAppCommand(AppCommandType::LED_OFF, false, nullptr, "firebase", firebaseTs, firebaseKey);
  if (cmd == "restart")     return enqueueAppCommand(AppCommandType::RESTART, true, nullptr, "firebase", firebaseTs, firebaseKey);
  // [4.1.0 OTA-GITHUB] Etap 1 planu upgrade — zdalne OTA przez GitHub Releases,
  // wzór: komenda "update"/"update_centrala" Centrali Pieca (v3.32.0 FB-OTA).
  // otaGithubRequest() jest nieblokujące (startuje task OTA na Core 0), więc
  // jest bezpieczne w tym kontekście (Core 0, tgTaskFn).
  if (cmd == "update" || cmd == "update_firmware" || cmd == "ota") {
    String _otaErr;
    bool _otaOk = otaGithubRequest(false, _otaErr);
    if (Komentarze) logPrintf("lvl=INFO tag=OTA-GH msg=\"Zadanie OTA z Firebase\" ok=%d err=%s\n",
                              (int)_otaOk, _otaErr.c_str());
    return true;   // komenda rozpoznana niezależnie od wyniku startu
  }
  if (cmd.startsWith("pwm ")) {
    uint16_t arr[5] = {0,0,0,0,0};
    String r = cmd.substring(4);
    for (int i=0;i<5;i++) {
      arr[i] = constrain(r.toInt(), 0, 1023);
      int sp = r.indexOf(' ');
      if (sp < 0) break;
      r = r.substring(sp+1);
    }
    return enqueueAppCommand(AppCommandType::PWM_SET, false, arr, "firebase", firebaseTs, firebaseKey);
  }
  if (Komentarze) logPrintf("lvl=WARN tag=FB msg=\"nieznana komenda: %s\"\n", cmd.c_str());
  return false;
}

void checkFirebaseCommands() {
  // [v259] Compatibility wrapper. No blocking Firebase I/O is allowed here.
  fbCmdPending = true;
}
// ─────────────────────────────────────────────────────────────────────────────

// ─── FIREBASE-v3: checkFirebaseConfig() ──────────────────────────────────────
// Czyta /aquarium/config.json zapisywany przez panel Firebase.
// Stosuje: harmonogram, adaptację, MIN LUX, parametry systemu -> EEPROM.
// Fallback co 120s; po config_refresh wykonuje odczyt natychmiast. Ignoruje duplikaty przez cfgTs.
void checkFirebaseConfig() {
  // [v259] Compatibility wrapper. No blocking Firebase I/O is allowed here.
  fbCfgPending = true;
}

static bool fbParseConfigSnapshot(const String& body, FirebaseConfigSnapshot& out) {
  out = FirebaseConfigSnapshot{};
  if (body == "null" || body.length() < 5) return false;

  int tIdx = body.indexOf("\"token\":\"");
  if (tIdx < 0) return false;
  String token = body.substring(tIdx + 9);
  token = token.substring(0, token.indexOf("\""));
  if (token != CMD_TOKEN) {
    if (Komentarze) logPrintf("lvl=WARN tag=FB-CFG msg=\"ODRZUCONO: bledny token\"\n");
    return false;
  }

  int tsIdx = body.indexOf("\"cfgTs\":");
  if (tsIdx < 0) return false;
  if (!fbParseU64Field(body, tsIdx + 8, out.cfgTs) || out.cfgTs == 0) return false;

  int ctIdx = body.indexOf("\"configType\":\"");
  if (ctIdx >= 0) {
    String cfgType = body.substring(ctIdx + 14);
    cfgType = cfgType.substring(0, cfgType.indexOf("\""));
    strncpy(out.configType, cfgType.c_str(), sizeof(out.configType)-1);
  }

  auto extractInt32 = [&](const char* key, int32_t& dst) -> bool {
    char needle[64]; snprintf(needle, sizeof(needle), "\"%s\":", key);
    int idx = body.indexOf(needle); if (idx < 0) return false;
    dst = (int32_t)body.substring(idx + (int)strlen(needle)).toInt(); return true;
  };
  auto extractBool = [&](const char* key, bool& dst) -> bool {
    char needle[64]; snprintf(needle, sizeof(needle), "\"%s\":", key);
    int idx = body.indexOf(needle); if (idx < 0) return false;
    String val = body.substring(idx + (int)strlen(needle), idx + (int)strlen(needle) + 5); val.trim();
    if (val.startsWith("true")) { dst = true; return true; }
    if (val.startsWith("false")) { dst = false; return true; }
    return false;
  };
  auto extractFloat = [&](const char* key, float& dst) -> bool {
    char needle[64]; snprintf(needle, sizeof(needle), "\"%s\":", key);
    int idx = body.indexOf(needle); if (idx < 0) return false;
    dst = body.substring(idx + (int)strlen(needle)).toFloat(); return true;
  };

  String cfgType = String(out.configType);
  bool any = false;
  if (cfgType == "" || cfgType == "schedule") {
    int32_t v;
    bool h = false;
    if (extractInt32("morningWD", v) && v >= 0 && v < 1440) { out.morningWD=v; out.fieldMask|=CFG_MWD; h=true; }
    if (extractInt32("morningWE", v) && v >= 0 && v < 1440) { out.morningWE=v; out.fieldMask|=CFG_MWE; h=true; }
    if (extractInt32("middayOff", v) && v >= 0 && v < 1440) { out.middayOff=v; out.fieldMask|=CFG_MIDOFF; h=true; }
    if (extractInt32("eveningBefore", v) && v >= -180 && v <= 180) { out.eveningBefore=v; out.fieldMask|=CFG_EVENB; h=true; }
    if (extractInt32("eveningOff", v) && v >= 0 && v < 1440) { out.eveningOff=v; out.fieldMask|=CFG_EVENOFF; h=true; }
    if (extractInt32("fadeMinutes", v) && v >= 1 && v <= 180) { out.fadeMinutesValue=v; out.fieldMask|=CFG_FADE; h=true; }
    out.hasSchedule=h; any |= h;
  }
  if (cfgType == "" || cfgType == "adapt") {
    int32_t sm; bool h=false;
    if (extractInt32("sensorMode", sm) && sm >= 0 && sm <= 2) { out.sensorMode=(uint8_t)sm; out.fieldMask|=CFG_SENSOR; h=true; }
    bool b;
    if (extractBool("adaptEnabled", b)) { out.adaptEnabled=b; out.fieldMask|=CFG_ADAPT; h=true; }
    if (extractBool("learningEnabled", b)) { out.learningEnabled=b; out.fieldMask|=CFG_LEARN; h=true; }
    out.hasAdapt=h; any |= h;
  }
  if (cfgType == "" || cfgType == "minlux") {
    bool h=false,b; float f; int32_t iv;
    if (extractBool("minLuxEnabled", b)) { out.minLuxEnabledValue=b; out.fieldMask|=CFG_ML_ENABLED; h=true; }
    if (extractFloat("minLuxTarget", f) && f >= 500.0f && f <= 8000.0f) { out.minLuxTargetValue=f; out.fieldMask|=CFG_ML_TARGET; h=true; }
    if (extractInt32("minLuxInterval", iv) && iv >= 5 && iv <= 300) { out.minLuxIntervalValue=(uint16_t)iv; out.fieldMask|=CFG_ML_INTERVAL; h=true; }
    out.hasMinLux=h; any |= h;
  }
  if (cfgType == "" || cfgType == "params") {
    bool h=false; float f; int32_t iv;
    if (extractFloat("kwhPrice", f) && f >= 0.01f && f <= 10.0f) { out.kwhPriceValue=f; out.fieldMask|=CFG_KWH; h=true; }
    if (extractInt32("rampSec", iv) && iv >= 1 && iv <= 600) { out.rampSecValue=(uint16_t)iv; out.fieldMask|=CFG_RAMP; h=true; }
    if (extractFloat("emaFilter", f) && f >= 0.05f && f <= 0.5f) { out.emaFilterValue=f; out.fieldMask|=CFG_EMA; h=true; }
    if (extractInt32("sensInt", iv) && iv >= 5 && iv <= 300) { out.sensIntValue=(uint16_t)iv; out.fieldMask|=CFG_SENSINT; h=true; }
    out.hasParams=h; any |= h;
  }
  if (cfgType == "pump") {
    int psIdx = body.indexOf("\"pumpSlots\":");
    if (psIdx >= 0) {
      int arrStart = body.indexOf("[", psIdx), arrEnd = body.indexOf("]", arrStart);
      if (arrStart >= 0 && arrEnd > arrStart) {
        String arr = body.substring(arrStart, arrEnd + 1); int pos=0; uint8_t count=0;
        while (count < PUMP_MAX_SLOTS) {
          int obj=arr.indexOf("{",pos); if(obj<0)break; int objEnd=arr.indexOf("}",obj); if(objEnd<0)break;
          String slot=arr.substring(obj,objEnd+1);
          auto parseHHMM=[&](const char* key)->int{ String k=String("\"")+key+"\":\""; int ki=slot.indexOf(k); if(ki<0)return -1; ki+=k.length(); return slot.substring(ki,ki+2).toInt()*60+slot.substring(ki+3,ki+5).toInt(); };
          int st=parseHHMM("s"), en=parseHHMM("e");
          if(st>=0&&st<1440&&en>=0&&en<1440){ out.pumpSlotsValue[count].start=st; out.pumpSlotsValue[count].end=en; ++count; }
          pos=objEnd+1;
        }
        if(count>0){ out.hasPump=true; out.pumpCount=count; any=true; }
      }
    }
  }
  if (cfgType == "telegram") {
    bool h=false;
    auto extractStr=[&](const char* key, char* dst, size_t cap)->bool{ String k=String("\"")+key+"\":\""; int p=body.indexOf(k); if(p<0)return false; p+=k.length(); int e=body.indexOf("\"",p); if(e<0)return false; String v=body.substring(p,e); strncpy(dst,v.c_str(),cap-1); dst[cap-1]='\0'; return v.length()>0; };
    h |= extractStr("tgBotToken", out.telegramToken, sizeof(out.telegramToken));
    h |= extractStr("tgChatId", out.telegramChatId, sizeof(out.telegramChatId));
    bool b;
    if (extractBool("tgEnabled", b)) { out.telegramEnabledValue=b; out.telegramEnabledPresent=true; h=true; }
    out.hasTelegram=h; any |= h;
  }
  if (!any) return false;
  if (Komentarze) logPrintf("lvl=INFO tag=FB-CFG state=PARSED ts=%llu typ=%s\n", (unsigned long long)out.cfgTs, out.configType);
  return true;
}

static bool enqueueFirebaseConfigSnapshot(const FirebaseConfigSnapshot& snap) {
  if (!fbConfigApplyQueue) {
    logPrintf("lvl=ERR tag=FB-CFG state=QUEUE_UNAVAILABLE ts=%llu\n", (unsigned long long)snap.cfgTs);
    return false;
  }
  if (xQueueSend(fbConfigApplyQueue, &snap, 0) != pdPASS) {
    logPrintf("lvl=ERR tag=FB-CFG state=QUEUE_FULL ts=%llu\n", (unsigned long long)snap.cfgTs);
    return false;
  }
  return true;
}



static uint8_t firebaseConfigStorageRequestCount(const FirebaseConfigSnapshot& c) {
  // Kept as a static audit helper: one dedicated snapshot queue item now replaces
  // N individual storageQueue requests.
  uint8_t n = 0;
  if (c.hasSchedule) n++;
  if (c.hasAdapt) n++;
  if (c.hasMinLux) n++;
  if (c.hasParams) {
    if (c.fieldMask & CFG_KWH) n++;
    if (c.fieldMask & CFG_RAMP) n++;
    if (c.fieldMask & CFG_EMA) n++;
    if (c.fieldMask & CFG_SENSINT) n++;
  }
  if (c.hasPump) n++;
  if (c.hasTelegram) n++;
  return n;
}

static bool enqueueFirebaseConfigPersist(const FirebaseConfigSnapshot& c) {
  if (!fbConfigPersistQueue) {
    logPrintf("lvl=ERR tag=FB-CFG state=PERSIST_QUEUE_UNAVAILABLE ts=%llu\n", (unsigned long long)c.cfgTs);
    return false;
  }
  if (xQueueSend(fbConfigPersistQueue, &c, 0) != pdPASS) {
    logPrintf("lvl=ERR tag=FB-CFG state=PERSIST_QUEUE_FULL ts=%llu\n", (unsigned long long)c.cfgTs);
    return false;
  }
  return true;
}

static void applyFirebaseConfigSnapshotRuntime(const FirebaseConfigSnapshot& c) {
  if (c.hasSchedule) {
    if (c.fieldMask & CFG_MWD) MORNING_ON_START_WEEKDAY = c.morningWD;
    if (c.fieldMask & CFG_MWE) MORNING_ON_START_WEEKEND = c.morningWE;
    if (c.fieldMask & CFG_MIDOFF) MIDDAY_OFF_LOCAL = c.middayOff;
    if (c.fieldMask & CFG_EVENB) EVENING_ON_BEFORE_SUNSET_MIN = c.eveningBefore;
    if (c.fieldMask & CFG_EVENOFF) EVENING_OFF_START = c.eveningOff;
    if (c.fieldMask & CFG_FADE) fadeMinutes = c.fadeMinutesValue;
  }
  if (c.hasAdapt) {
    if (c.fieldMask & CFG_SENSOR) {
      uzywajCzujnikaPokojowego = (c.sensorMode >= 1);
      uzywajCzujnikaNadWoda = (c.sensorMode == 2);
    }
    if (c.fieldMask & CFG_ADAPT) regulacjaAdaptacyjnaWlaczona = c.adaptEnabled;
    if (c.fieldMask & CFG_LEARN) uczenieSieWlaczone = c.learningEnabled;
  }
  if (c.hasMinLux) {
    if (c.fieldMask & CFG_ML_ENABLED) { minLuxModeEnabled = c.minLuxEnabledValue; if (!minLuxModeEnabled) minLuxModeActive = false; }
    if (c.fieldMask & CFG_ML_TARGET) minLuxDayTarget = c.minLuxTargetValue;
    if (c.fieldMask & CFG_ML_INTERVAL) { minLuxIntervalSec = c.minLuxIntervalValue; minLuxUpdateInterval = (unsigned long)minLuxIntervalSec * 1000UL; }
  }
  if (c.hasParams) {
    if (c.fieldMask & CFG_KWH) kwhPrice = c.kwhPriceValue;
    if (c.fieldMask & CFG_RAMP) rampSec = c.rampSecValue;
    if (c.fieldMask & CFG_EMA) emaFilterAlpha = c.emaFilterValue;
    if (c.fieldMask & CFG_SENSINT) { sensIntSec = c.sensIntValue; intervalOdczytu = (unsigned long)sensIntSec * 1000UL; }
  }
  if (c.hasPump) {
    pumpSlotCount = c.pumpCount;
    for (int i=0;i<PUMP_MAX_SLOTS;i++) pumpSlots[i] = (i<c.pumpCount) ? c.pumpSlotsValue[i] : PumpSlot{0,0};
  }
  if (c.hasTelegram) {
    if (c.telegramToken[0]) tgBotToken = String(c.telegramToken);
    if (c.telegramChatId[0]) tgChatId = String(c.telegramChatId);
    if (c.telegramEnabledPresent) tgEnabled = c.telegramEnabledValue;
  }
}

static void queueFinalFirebaseConfigAck() {
  if (!g_fbConfigFinalAckPendingValid || !fbConfigAckQueue) return;
  if (xQueueSend(fbConfigAckQueue, &g_fbConfigFinalAckPending, 0) == pdPASS) {
    g_fbConfigFinalAckPendingValid = false;
  }
}

static FirebaseConfigSnapshot g_fbConfigRuntimePending{};
static bool g_fbConfigRuntimePendingValid = false;

static void processFirebaseConfigApply() {
  if (!fbConfigApplyQueue || !fbConfigPersistQueue || !fbConfigPersistAckQueue) return;
  queueFinalFirebaseConfigAck();

  if (g_fbConfigPersistPending) {
    FirebaseConfigPersistAck ack;
    if (xQueueReceive(fbConfigPersistAckQueue, &ack, 0) == pdPASS) {
      if (ack.success && g_fbConfigRuntimePendingValid && ack.cfgTs == g_fbConfigRuntimePending.cfgTs) {
        applyFirebaseConfigSnapshotRuntime(g_fbConfigRuntimePending);
        g_fbConfigFinalAckPending.success = true;
        g_fbConfigFinalAckPending.cfgTs = g_fbConfigRuntimePending.cfgTs;
        strncpy(g_fbConfigFinalAckPending.firebaseKey, g_fbConfigPendingFirebaseKey, sizeof(g_fbConfigFinalAckPending.firebaseKey)-1);
        g_fbConfigFinalAckPending.firebaseKey[sizeof(g_fbConfigFinalAckPending.firebaseKey)-1] = '\0';
        g_fbConfigFinalAckPendingValid = true;
        logPrintfNoFile("lvl=INFO tag=FB-CFG state=APPLIED_CORE1_AFTER_PERSIST ts=%llu\n", (unsigned long long)ack.cfgTs);
      } else if (!ack.success) {
        logPrintf("lvl=ERR tag=FB-CFG state=PERSIST_NACK_CORE1 ts=%llu\n", (unsigned long long)ack.cfgTs);
      }
      if (ack.success && g_fbConfigRuntimePendingValid && ack.cfgTs == g_fbConfigRuntimePending.cfgTs) {
        g_fbConfigRuntimePendingValid = false;
        g_fbConfigPersistPending = false;
      } else if (!ack.success) {
        // Core 0 owns retry; keep pending state until success.
      }
    }
    return;
  }

  FirebaseConfigSnapshot c;
  if (xQueueReceive(fbConfigApplyQueue, &c, 0) != pdPASS) return;
  if (xQueueSend(fbConfigPersistQueue, &c, 0) != pdPASS) {
    logPrintf("lvl=ERR tag=FB-CFG state=PERSIST_QUEUE_FULL ts=%llu\n", (unsigned long long)c.cfgTs);
    return;
  }
  g_fbConfigRuntimePending = c;
  g_fbConfigRuntimePendingValid = true;
  strncpy(g_fbConfigPendingFirebaseKey, c.firebaseKey, sizeof(g_fbConfigPendingFirebaseKey)-1);
  g_fbConfigPendingFirebaseKey[sizeof(g_fbConfigPendingFirebaseKey)-1] = '\0';
  g_fbConfigPersistPending = true;
  logPrintfNoFile("lvl=INFO tag=FB-CFG state=QUEUED_CORE0_PERSIST ts=%llu key=%s\n", (unsigned long long)c.cfgTs, c.firebaseKey);
}

// ─────────────────────────────────────────────────────────────────────────────

// [v127] LOOP-CP-GLOBAL: wspólny timer dla makra LOOP_CP
static unsigned long _lprofT = 0;      // czas początku bieżącej sekcji
static const char*   _lprofN = nullptr; // nazwa bieżącej sekcji

static void processAppCommands() {
  if (!appCmdQueue) return;
  AppCommand c;
  uint8_t processed = 0;
  while (processed < 4 && xQueueReceive(appCmdQueue, &c, 0) == pdPASS) {
    ++processed;
    bool commandApplied = true;
    switch (c.type) {
      case AppCommandType::POWER_SET:
        if (power != c.value) { power = c.value; onPowerChange(); prevPowerGlobal = power; }
        break;
      case AppCommandType::POWER_TOGGLE:
        power = !power; onPowerChange(); prevPowerGlobal = power;
        break;
      case AppCommandType::TRYB_SET:
        if (tryb != c.value) {
          if (!c.value && tryb) {
            rampaAdaptacyjnaAktywna = false; rampaMinLuxPriorytet = false; transitionActive = false;
            softStartActive = false; rampScheduleActive = false;
            for (int i=0;i<5;i++) rampInitialized[i] = false;
          }
          tryb = c.value; prevTrybGlobal = tryb; onTrybChange();
        }
        break;
      case AppCommandType::TRYB_TOGGLE:
        if (!tryb) {
          tryb = true;
        } else {
          rampaAdaptacyjnaAktywna = false; rampaMinLuxPriorytet = false; transitionActive = false;
          softStartActive = false; rampScheduleActive = false;
          for (int i=0;i<5;i++) rampInitialized[i] = false;
          tryb = false;
        }
        prevTrybGlobal = tryb; onTrybChange();
        break;
      case AppCommandType::PUMP_SET:
        pumpManualOverrideUntil = c.value ? millis() + 30UL*60UL*1000UL : 0;
        digitalWrite(pumpPin, c.value ? LOW : HIGH); currentPumpState = c.value;
        logPrintf("lvl=INFO tag=APP-CMD cmd=PUMP_SET value=%d source=%s\n", c.value ? 1 : 0, c.source);
        break;
      case AppCommandType::LED_TEST:
        for (int i=0;i<5;i++) setBrightnessForSection(backupBrightnessComposite, i, 1023);
        updateLEDs(); logPrintf("lvl=INFO tag=APP-CMD cmd=LED_TEST source=%s\n", c.source); break;
      case AppCommandType::LED_OFF:
        for (int i=0;i<5;i++) setBrightnessForSection(backupBrightnessComposite, i, 0);
        updateLEDs(); logPrintf("lvl=INFO tag=APP-CMD cmd=LED_OFF source=%s\n", c.source); break;
      case AppCommandType::RESTART:
        restartRequestedAt = millis(); logPrintf("lvl=INFO tag=APP-CMD cmd=RESTART source=%s\n", c.source); break;
      case AppCommandType::PWM_SET:
        commandApplied = zastosujRampeAdaptacyjna(c.pwm, c.source); break;
    }
    if (c.firebaseAck && commandApplied) {
      FirebaseAck ack;
      ack.success = true;
      ack.ts = c.firebaseTs;
      strncpy(ack.key, c.firebaseKey, sizeof(ack.key)-1);
      ack.key[sizeof(ack.key)-1] = '\0';
      if (!fbAckQueue || xQueueSend(fbAckQueue, &ack, 0) != pdPASS) {
        logPrintf("lvl=ERR tag=FB-ACK msg=\"QUEUE_FULL\" key=%s\n", ack.key);
      }
    } else if (c.firebaseAck) {
      logPrintf("lvl=WARN tag=FB-ACK msg=\"COMMAND_NOT_APPLIED_RETRY\" key=%s\n", c.firebaseKey);
    }
  }
}

void loop() {
  static bool firstLoop = true;
  if (firstLoop) { firstLoop = false; Serial.println(">> LOOP START"); Serial.flush(); }

  // [v257] immutable config snapshot: runtime apply wyłącznie na Core 1
  processFirebaseConfigApply();

  // ── [v126] DIAG-LOOP + LOOP-CP-ALWAYS ────────────────────────────────────────
  // LOOP_CP(label): mierzy czas sekcji i loguje NATYCHMIAST gdy >100ms.
  // Nie wymaga żadnej flagi aktywacji — działa zawsze, w każdej iteracji.
  // Efekt: winowajca LOOP-SLOW pojawia się w logu w momencie wystąpienia.
  // Koszt: ~1µs na checkpoint (millis() + porównanie) — pomijalny.
  // LOOP-SLOW nadal raportuje całkowity czas iteracji przy >500ms.
  // ─────────────────────────────────────────────────────────────────────────────

  // [v127] LOOP_CP — jeden wspólny timer globalny (_lprofT / _lprofN).
  // Mierzy czas od POPRZEDNIEGO LOOP_CP do BIEŻĄCEGO (czas sekcji między dwoma).
  // Loguje NATYCHMIAST gdy >100ms — bez flagi aktywacji.
  // Globalne zmienne zadeklarowane przed void loop() — widoczne przez makro.
  // WAŻNE: NIE używaj LOOP_CP wewnątrz bloków if() z długim interwałem wywołań
  //        (np. co 15s) — umieszczaj go PRZED blokiem if(), nie w środku.
#define LOOP_CP(label) \
  do { \
    unsigned long _lprofNow = millis(); \
    if (_lprofN && _lprofT && (_lprofNow - _lprofT) > 100) \
      logPrintf("lvl=WARN tag=LOOP-CP label=%s ms=%lu\n", \
                _lprofN, _lprofNow - _lprofT); \
    _lprofN = (label); \
    _lprofT = _lprofNow; \
    PRE_RESET_CP_L(label);  /* [FIX-v133-WDT-HUNT] silny, bezlogowy zapis do black-box */ \
  } while(0)

  {
    static unsigned long _prevLoopMs    = 0;
    static unsigned long _maxLoopMs     = 0;
    static unsigned long _slowLoopCount = 0;
    static unsigned long _lastLoopReport= 0;
    unsigned long _now = millis();
    if (_prevLoopMs > 0) {
      unsigned long _delta = _now - _prevLoopMs;
      if (_delta > _maxLoopMs) _maxLoopMs = _delta;
      if (_delta > 500) {
        _slowLoopCount++;
        logPrintf("lvl=WARN tag=LOOP-SLOW msg=\"cos blokuje loop\" ms=%lu count=%lu\n",
                  _delta, _slowLoopCount);
      }
    }
    _prevLoopMs = _now;
    if (_now - _lastLoopReport >= 300000UL) {
      _lastLoopReport = _now;
      logPrintf("lvl=INFO tag=LOOP-STAT maxIter=%lu slowCount=%lu okres=5min\n",
                _maxLoopMs, _slowLoopCount);
      _maxLoopMs     = 0;
      _slowLoopCount = 0;
    }
  }
  LOOP_CP("start");

  // [v41] FIX HEAP-LEAK-1
  LOOP_CP("wsFlush");
  flushWsQueue();

  LOOP_CP("appCmd");
  processAppCommands();

  // [v252] jeden właściciel zasobu PWM przed rozpoczęciem logiki LED
  rampArbiterReconcile();

  // [v41] FIX HEAP-LEAK-2: wsTerminal.cleanupClients() - wymagane przez ESPAsyncWebServer.
  LOOP_CP("wsCleanup");  // [v125] podejrzany #1 — co 30s, potencjalne 200-500ms
  // Bez tego martwe połączenia WebSocket zajmują pamięć na zawsze -> fragmentacja HEAP po 2-6h.
  // [v255] cleanup co 1s zgodnie z praktyką ESPAsyncWebServer; koszt mierzymy diagnostyką.
  // [v156] DIAG-WSCLEANUP: pomiar czasu trwania cleanupClients() (millis() przed/po),
  // NIE fix — tylko sprawdzenie, czy podejrzenie z [v125] ("200-500ms") realnie
  // się materializuje na żywym urządzeniu. Zachowanie bez zmian względem v155.
  {
    static unsigned long _lastWsCleanup = 0;
    if (millis() - _lastWsCleanup >= 1000UL) {
      _lastWsCleanup = millis();
      unsigned long _wsCleanupStartMs = millis();
      wsTerminal.cleanupClients();
      unsigned long _wsCleanupMs = millis() - _wsCleanupStartMs;
      if (_wsCleanupMs > WS_CLEANUP_STALL_THRESHOLD_MS) {
        g_wsCleanupStallCount = g_wsCleanupStallCount + 1;
        if (_wsCleanupMs > g_wsCleanupStallMaxMs) g_wsCleanupStallMaxMs = _wsCleanupMs;
        logPrintf("lvl=WARN tag=WS-CLEANUP-STALL msg=\"cleanupClients trwal dlugo\" ms=%lu cnt=%lu maxMs=%lu\n",
                  (unsigned long)_wsCleanupMs, (unsigned long)g_wsCleanupStallCount,
                  (unsigned long)g_wsCleanupStallMaxMs);
      }
    }
  }

  LOOP_CP("fbFlags");
  // ─── FIREBASE-v2: polling -> flagi dla Core 0 ────────────────────────────────
  // [v42] FIX-ARCH: Wszystkie Firebase HTTP przeniesione na Core 0 (tgTaskFn).
  // loop() (Core 1) TYLKO ustawia flagi przez timer - zero alokacji sieciowych
  // na Core 1. tgTaskFn sprawdza flagi i wykonuje HTTPClient sekwencyjnie,
  // razem z Telegram SSL - jeden rdzeń, cała sieć, zero wyścigów na HEAP.
  // [v240+] FIREBASE TURBO: szybki tryb steruje jedynie ustawieniem flag pending;
  // właściwy scheduler utrzymuje dokładnie jeden request Firebase in-flight.
  const bool _fbTurbo = fbTurboAktywne();
  const unsigned long _fbSendInterval = _fbTurbo
      ? FB_TURBO_SEND_INTERVAL_MS : FIREBASE_SEND_INTERVAL;
  const unsigned long _fbCmdInterval = _fbTurbo
      ? FB_TURBO_CMD_INTERVAL_MS : FIREBASE_CMD_INTERVAL;
  {
    static bool _turboLogged = false;
    if (_fbTurbo && Komentarze) {
      if (!_turboLogged) {
        _turboLogged = true;
        logPrintln("lvl=INFO tag=FB-TURBO event=active mode=1s_cmd_5s_status");
      }
    } else {
      _turboLogged = false;
    }
  }

  if (millis() - lastFirebaseSend >= _fbSendInterval) {
    lastFirebaseSend = millis();
    fbSendPending = true;   // [v42] Core 0 wykona sendStatusToFirebase()
  }
  if (millis() - lastFirebaseCmd >= _fbCmdInterval) {
    lastFirebaseCmd = millis();
    fbCmdPending = true;    // [v42] Core 0 wykona checkFirebaseCommands()
  }
  if (millis() - lastFirebaseConfig >= FIREBASE_CFG_INTERVAL) {
    lastFirebaseConfig = millis();
    fbCfgPending = true;    // [v42] Core 0 wykona checkFirebaseConfig()
  }
  // ─────────────────────────────────────────────────────────────────────────

  // [NETDIAG] Test routera/łącza - TEN SAM wzorzec co FIREBASE-v2 wyżej:
  // loop() (Core 1) tylko sprawdza timer i ustawia flagę, zero sieci tutaj.
  // Realne WiFiClient.connect() wykonuje tgTaskFn (Core 0), patrz NetDiag.h.
  netDiagLoop();

  LOOP_CP("waterLeak");
  // ─── CZUJNIK OBECNOŚCI WODY (analogowy, GPIO6) — sprawdzenie co 60s ───────
  // [v234] FIX-LOG-WATERLEAK-ONCHANGE: linia "odczyt" logowała się bezwarunkowo
  // co 60s (potwierdzone w wielu logach produkcyjnych: adc=0 state=0 zawsze,
  // zero informacji w tej postaci). WAŻNE: częstotliwość SAMEGO SPRAWDZANIA
  // (60s) zostaje BEZ ZMIAN — to czujnik bezpieczeństwa, wykrycie realnego
  // przecieku (branże "woda wykryta"/"zanik wody" niżej) musi zostać równie
  // szybkie jak dziś. Zmienia się WYŁĄCZNIE częstość linii "odczyt" — z 60s
  // na heartbeat co 1h, bo to i tak tylko potwierdzenie "monitoring żyje",
  // a stan faktycznie zmieniający się już ma własne, osobne, nietknięte linie
  // (WARN "woda wykryta" / INFO "zanik wody") logowane natychmiast przy zmianie.
  {
    static unsigned long _lastWLCheck = 0;
    static unsigned long _lastWLLogMs = 0;
    static bool _wlState = false;   // true = woda wykryta

    if (millis() - _lastWLCheck >= 60000UL) {
      _lastWLCheck = millis();
      int wlRaw = analogRead(WATER_LEAK_PIN);
      if (millis() - _lastWLLogMs >= 3600000UL) {  // [v234] heartbeat 1h zamiast co 60s
        _lastWLLogMs = millis();
        logPrintf("lvl=INFO tag=WATER-LEAK msg=\"odczyt\" adc=%d state=%d\n", wlRaw, _wlState);
      }

      if (!_wlState && wlRaw >= WATER_LEAK_THRESHOLD_DETECT) {
        _wlState = true;
        waterLeakDetectedNotifyPending = true;
        logPrintf("lvl=WARN tag=WATER-LEAK msg=\"woda wykryta\" adc=%d\n", wlRaw);
      } else if (_wlState && wlRaw <= WATER_LEAK_THRESHOLD_CLEAR) {
        _wlState = false;
        waterLeakClearedNotifyPending = true;
        logPrintf("lvl=INFO tag=WATER-LEAK msg=\"zanik wody\" adc=%d\n", wlRaw);
      }
    }
  }
  // ─────────────────────────────────────────────────────────────────────────

  LOOP_CP("wdtReset");
  // [OK] WATCHDOG RESET - "jestem żywy"
  static unsigned long lastWdt = 0;
  if (millis() - lastWdt >= 1000) {
    esp_task_wdt_reset();
    lastWdt = millis();
  }

  LOOP_CP("OTA");  // [v125] podejrzany #2 — przechodzi przez TCP stack
  ArduinoOTA.handle();  // [v98-OTA] obsługa espota — non-blocking normalnie,
                        //           blokuje loop() tylko podczas aktywnego uploadu

  LOOP_CP("debugPower");
  // ═══════════════════════════════════════════════════════
  // 🔍 DEBUG POWER/TRYB - detekcja zmian + raport co 10s
  // ═══════════════════════════════════════════════════════
  {
    // [v250-ARCH] globalny snapshot dla single-writer control plane.
    // [FIX-v128-DOUBLE-CB] prevTryb przeniesione na globalną prevTrybGlobal
    // (zdefiniowana ~3800) — handlery set-tryb/Firebase/TG synchronizują ją
    // przy każdej zmianie, więc DEBUG-block nie wykryje "cichej zmiany" gdy
    // handler już wywołał onTrybChange() — eliminuje podwójne wywołanie.
    static bool firstDebug = true;
    static unsigned long lastDebugDump = 0;

    // Wykryj CICHĄ zmianę (bez callbacka) - biblioteka może zmienić zmienną z pominięciem callbacka
    if (power != prevPowerGlobal) {
      logPrintf("lvl=WARN tag=CICHA-ZMIANA msg=\"power zmieniony bez callbacka\" z=%s na=%s czas=%lus\n",
        prevPowerGlobal ? "ON" : "OFF", power ? "ON" : "OFF", millis() / 1000);
      prevPowerGlobal = power;
      // [v250-ARCH] awaryjnie obsłuż zmianę spoza kolejki.
      onPowerChange();
    }
    if (tryb != prevTrybGlobal) {  // [FIX-v128-DOUBLE-CB] global zamiast static
      logPrintf("lvl=WARN tag=CICHA-ZMIANA msg=\"tryb zmieniony bez callbacka\" z=%s na=%s czas=%lus\n",
        prevTrybGlobal ? "AUTO" : "MANUAL", tryb ? "AUTO" : "MANUAL", millis() / 1000);
      // [OK] FIX FLASH-2: AUTO->MANUAL - wyczyść rampy AUTO, żeby nie walczyły z manualTransition
      if (!tryb && prevTrybGlobal) {
        rampaAdaptacyjnaAktywna = false;
        rampaMinLuxPriorytet    = false;
        transitionActive        = false;
        softStartActive         = false;
        rampScheduleActive      = false;
        for (int i = 0; i < 5; i++) rampInitialized[i] = false;
      }
      prevTrybGlobal = tryb;  // [FIX-v128-DOUBLE-CB]
      // Wywołaj callback ręcznie, skoro biblioteka go pominęła
      onTrybChange();
    }

    if (firstDebug) {
      firstDebug = false;
      lastDebugDump = millis();
    }
  }

  // [v41] FIX: loadPowerTrybFromEEPROM() była wywoływana przy KAŻDEJ iteracji loop() (co ~100ms).
  // EEPROM.get() na ESP32 czyta z RAM cache (nie flash), ale wywołanie funkcji i porównania
  // co 100ms to niepotrzebny overhead. Wystarczy jeden raz przy starcie (setup() już to robi).
  // Usunięto z loop() - stan jest zarządzany przez onPowerChange() / onTrybChange() / Firebase.
  // loadPowerTrybFromEEPROM(); ← USUNIĘTE (wywoływane tylko w setup())


  LOOP_CP("camera+restart");
  // [OK] FIX LOGIC-E: Auto-wyłączenie kamery po 30 minutach
  if (cameraOnTime != 0 && millis() - cameraOnTime > 30UL * 60UL * 1000UL) {
    wylaczKamera();
    if (Komentarze) logPrintln("lvl=INFO tag=KAMERA msg=\"Auto-wylaczenie po 30 minut\"");
  }

  // [OK] FIX BUG-V: Asynchroniczny restart - wykonaj tu, nie w handlerze HTTP (delay blokował main task)
  if (restartRequestedAt != 0 && millis() - restartRequestedAt >= 1000) {
    logPrintln("lvl=INFO tag=RESTART msg=\"Restart na zadanie HTTP\"");
    g_restartPending = true;  // [FIX-v154-PRERESET-FLIPFLOP] blokuje heartbeat tgTask
    PRE_RESET_UPDATE(0);  // [v71] src=0 -> soft/HTTP
    logForceFlush();
    delay(100);           // [v71] bylo 50ms - za malo na LittleFS write+close 8KB
    ESP.restart();
  }

  static bool timeSynced = false;
  static bool timeDebugged = false;
  static unsigned long lastSunsetRecalc = 0;
  static unsigned long lastLogicRun = 0;

  const unsigned long recalcInterval = 24UL * 60UL * 60UL * 1000UL; // 24h
  const unsigned long logicInterval  = 100; // [OK] 10 Hz - płynna rampa

  unsigned long nowMs = millis();

  LOOP_CP("ntpSync");
  // ======================================================
  // 1️⃣ Synchronizacja czasu NTP + zachód słońca
  // ======================================================
  if (WiFi.status() == WL_CONNECTED &&
      (!timeSynced || nowMs - lastNtpSync >= recalcInterval)) {

    struct tm ti;
    if (getLocalTimePL(&ti)) {

      int zachodUTC = obliczZachodSlonca(
        ti.tm_year + 1900,
        ti.tm_mon + 1,
        ti.tm_mday,
        latitude,
        longitude
      );

      if (zachodUTC >= 0 && zachodUTC <= 1440) {

        bool dst = isDaylightSaving(time(nullptr));

        int offset = dst ? timezoneOffsetMinutes
                         : (timezoneOffsetMinutes - 60);

        sunsetMinutes = (zachodUTC + offset + 1440) % 1440;

        timeSynced = true;
        lastNtpSync = nowMs;
        lastSunsetRecalc = nowMs;  // zsynchronizuj z codziennym przeliczeniem

        if (Komentarze) {
          char buf[6];
          snprintf(buf, sizeof(buf), "%02d:%02d",
                   sunsetMinutes / 60,
                   sunsetMinutes % 60);
          logPrintf("lvl=INFO tag=NTP msg=\"Zachod slonca przeliczony\" zachod=%s\n", buf);
        }
      }
    }
  }

  // ======================================================
  // 2️⃣ Debug zegara (raz / minuta)
  // ======================================================
  if (timeSynced) {
    if (!timeDebugged) {
      if (Komentarze) logPrintln("lvl=INFO tag=NTP msg=\"Czas zsynchronizowany\"");
      timeDebugged = true;
    }
  }

  LOOP_CP("wifiCheck+sensors");
  // ======================================================
  // 3️⃣ czujniki
  // ======================================================
  // [v33] lastCloudSync / ArduinoCloud.update() co 500ms - usunięte

  // WiFi monitor co 60s + hard-reconnect po 3 min braku połączenia
  // [OK] FIX-v39-5: setAutoReconnect(true) przestaje działać po długim braku sieci
  //   (IDF wchodzi w stan ESP_ERR_WIFI_STATE i ignoruje esp_wifi_connect).
  //   Jedynym wyjściem jest WiFi.disconnect(true) + WiFi.begin().
  {
    static unsigned long _wifiCheckMs  = 0;
    static unsigned long _wifiLostMs   = 0;
    static uint8_t       _wifiRetries  = 0;

    if (nowMs - _wifiCheckMs >= 60000UL) {
      _wifiCheckMs = nowMs;
      if (WiFi.status() != WL_CONNECTED) {
        if (_wifiLostMs == 0) _wifiLostMs = nowMs;
        unsigned long lostSec = (nowMs - _wifiLostMs) / 1000UL;
        logPrintf("lvl=WARN tag=WiFi msg=\"Brak polaczenia\" czas=%lus probyReconnect=%u\n",
                  lostSec, _wifiRetries);
        // [OK] FIX-v52-WDT-BOOT: Guard uptime < 180s - nie uruchamiaj hard-reconnect
        // zaraz po bocie (setup() właśnie skończył próby WiFi - disconnect()+begin() teraz
        // mogłoby zakłócić trwający esp_wifi_connect() z setAutoReconnect(true)).
        if (nowMs - _wifiLostMs >= 180000UL && nowMs > 180000UL) {   // >3 minuty I uptime >3 min
          _wifiRetries++;
          logPrintf("lvl=WARN tag=WiFi msg=\"Hard-reconnect\" proba=%u\n", _wifiRetries);
          // ************************************************FIX-v56-WDT-RECONNECT
          // ROOT CAUSE: WiFi.disconnect(true) wywołuje esp_wifi_stop() — pełne
          //   zatrzymanie stosu WiFi. Blokuje loop() (Core 1) na ~5–9s bez
          //   esp_task_wdt_reset(). WiFi.begin() po zatrzymanym stosie dokłada
          //   kolejne ~2–4s. Razem >10s → WDT_TASK crash (uptime≈241s, widoczne
          //   w logach co ~4 min przy niedostępnym WiFi).
          //   Dodanie esp_task_wdt_reset() byłoby MASKOWANIEM — watchdog jest po
          //   to, by wykrywać zablokowaną pętlę; omijanie go ukrywa problem.
          //
          // NAPRAWA: disconnect(false) = wifioff=false → tylko esp_wifi_disconnect()
          //   (rozłącza od AP, NIE zatrzymuje stosu WiFi). Wraca w <1ms.
          //   WiFi.begin() z działającym stosem też wraca w <1ms.
          //   Blokowanie loop(): <5ms zamiast ~9s — WDT nie ma czego wykrywać.
          //   Cel FIX-v39-5 zachowany: begin() resetuje ESP_ERR_WIFI_STATE.
          //   esp_task_wdt_reset() poniżej = defense-in-depth, nie główny fix.
          // ************************************************FIX-v56-WDT-RECONNECT
          esp_task_wdt_reset();                              // margines przed IDF
          WiFi.disconnect(false);                            // NIE zatrzymuj stosu WiFi
          esp_task_wdt_reset();                              // margines między wywołaniami
          wifiBeginBest(false);  // [v139] bez skanu (WDT-safe) - round-robin po znanych sieciach
          esp_task_wdt_reset();                              // margines po begin()
          _wifiLostMs = nowMs;   // reset timera - kolejna próba po 3 min
        }
      } else {
        // [v222] FIX-WWW-NEVER-STARTED: v52 (wyżej) naprawił tylko przypadek
        // "WiFi było, zniknęło, wróciło" (_wifiLostMs != 0). Nie obejmował
        // przypadku "WiFi nie połączyło się WCALE w setup() (obie sieci
        // nieudane w 7.5s+timeout), serwer nigdy nie wystartował, WiFi
        // połączyło się dopiero teraz" — wtedy _wifiLostMs był cały czas 0
        // (bo nigdy nie zaszła gałąź "utracone", która go ustawia), więc
        // `if (_wifiLostMs != 0)` był fałszywy i webserialServer.begin()
        // nigdy się nie wykonywał. Log z urządzenia to potwierdza dosłownie:
        // tag=SETUP "Brak WiFi w setup() po 2 probach, webserialServer i
        // ArduinoOTA NIE wystartowaly w tym boocie (wystartuja po nastepnym
        // restarcie z dostepnym WiFi)" — a w praktyce NIE wystartowywały
        // wcale, dopóki ktoś ręcznie nie zrestartował urządzenia.
        // NAPRAWA: `!wsReady` jako dodatkowy, niezależny warunek — wsReady
        // jednoznacznie mówi "serwer faktycznie wystartował", niezależnie od
        // tego, czy trafiliśmy tu przez odzyskanie po utracie, czy przez
        // pierwsze udane połączenie po nieudanym boocie.
        if (_wifiLostMs != 0 || !wsReady) {
          if (_wifiLostMs != 0) {
            logPrintf("lvl=INFO tag=WiFi msg=\"Ponownie polaczony\" proby=%u przerwa=%lus\n",
                      _wifiRetries, (nowMs - _wifiLostMs) / 1000UL);
          } else {
            // [v222] Nowa gałąź: WiFi nigdy nie było utracone (_wifiLostMs==0),
            // bo nigdy nie połączyło się w setup() - to pierwsze połączenie
            // w tym cyklu pracy, nie "powrót po utracie". Osobny komunikat,
            // żeby nie logować mylącego "przerwa=Xs" liczonego od _wifiLostMs=0
            // (co dałoby czas od 1970/boot, nie realny czas przerwy).
            logPrintf("lvl=INFO tag=WiFi msg=\"Polaczono pierwszy raz po nieudanym boocie\" uptime_s=%lus\n",
                      nowMs / 1000UL);
          }
          // [OK] FIX-v52-WWW-RECONNECT: AsyncWebServer NIE rebinduje automatycznie
          // gdy WiFi wróci po utracie połączenia. server.begin() w setup() binduje gniazdo
          // do interfejsu który nie miał IP -> port 8080 martwy do restartu.
          // Naprawa: ponowny server.begin() przy każdym odzyskaniu WiFi.
          // webserialServer.end() nie istnieje w ESPAsyncWebServer - begin() jest idempotentny
          // (nie tworzy drugiego listenera, tylko rebinduje jeśli interfejs jest gotowy).
          if (!wsReady) {
            esp_task_wdt_reset();  // [v93] defensywnie przed begin()
            webserialServer.begin();
            esp_task_wdt_reset();  // [v93] reset po begin()
            wsReady = true;
            logPrintf("lvl=INFO tag=FIX-v52 akcja=rebind ip=%s:8080\n",
                      WiFi.localIP().toString().c_str());
          } else {
            // wsReady=true ale połączenie zerwało się i wróciło: rebind dla pewności
            esp_task_wdt_reset();  // [v93] defensywnie przed begin()
            webserialServer.begin();
            esp_task_wdt_reset();  // [v93] reset po begin()
            logPrintf("lvl=INFO tag=FIX-v52 akcja=begin ip=%s:8080\n",
                      WiFi.localIP().toString().c_str());
          }
          // [v222] FIX-OTA-NEVER-STARTED: ten sam mechanizm co webserwer wyżej,
          // dla ArduinoOTA — patrz komentarz przy definicji startOtaIfNeeded().
          // ArduinoOTA.begin() jest bezpieczne wywołać ponownie (rejestruje
          // callbacki i binduje UDP/TCP na porcie 3232, tak samo idempotentne
          // jak webserialServer.begin()).
          if (!otaReady) {
            esp_task_wdt_reset();
            startOtaIfNeeded();
            otaReady = true;
            esp_task_wdt_reset();
            logPrintf("lvl=INFO tag=FIX-v222 akcja=ota-start ip=%s:3232\n",
                      WiFi.localIP().toString().c_str());
          }
        }
        _wifiLostMs  = 0;
        _wifiRetries = 0;
      }
    }
  }

  #define _SECTION_T(label, code) { unsigned long _st = millis(); code; unsigned long _sd = millis()-_st; \
    if (_sd > 1000) logPrintf("lvl=WARN tag=SECTION label=%s ms=%lu\n", label, _sd); }
  
  // [OK] FIX-v33i: DS18B20 2-fazowy odczyt - zero delay() w loop()
  // Faza 1 (co 15s): wyślij rozkaz CONVERT_T, wróć natychmiast (~3ms).
  // Faza 2 (>=100ms po Faza1): odczytaj gotowy wynik (~3ms).
  // readTemperatures() samo decyduje którą fazę wykonać na podstawie tempConvRequested.
  {
    unsigned long _tnow = millis();
    // Faza 1: inicjuj nową konwersję co 15s (gdy Faza2 już skonsumowała poprzednią)
    // [v127] LOOP_CP przed if() — nie wewnątrz (uniknięcie fałszywego 15000ms)
    LOOP_CP("tempCheck");
    if (!tempConvRequested && _tnow - tempLastRead >= 15000UL) {
      tempLastRead = _tnow;
      readTemperatures();  // wykona Faza1 i wróci
    }
    // Faza 2: odczytaj gdy konwersja gotowa (>=100ms od Faza1)
    if (tempConvRequested && _tnow - tempConvRequestedAt >= 100UL) {
      readTemperatures();  // wykona Faza2 i wróci
    }
  }
  // 🆕 Czytaj czujniki światła co 30 sekund (throttling wbudowany w odczytajSwiatloZFiltrem)
  // [OK] POPRAWKA: Usunięto zewnętrzny timer - funkcja ma własny wewnętrzny (ostatniOdczytSwiatla)
  LOOP_CP("luxRead");
  _SECTION_T("odczytajSwiatloZFiltrem()", odczytajSwiatloZFiltrem());

  LOOP_CP("histFlag");
  // 📊 Zapis punktu historii: pierwsze po 30s, następne co 5 minut
  // [OK] FIX-v33g: ustawia flagę -> zapis wykonuje tgTask na Core 0 (nie blokuje loop())
  static unsigned long lastHistorySave = 0;
  static bool firstHistorySaved = false;
  if (!firstHistorySaved && millis() >= 30000UL) {
    lastHistorySave = millis();
    firstHistorySaved = true;
    histSavePending = true;
  } else if (firstHistorySaved && millis() - lastHistorySave >= 300000UL) {
    lastHistorySave = millis();
    histSavePending = true;
  }


  LOOP_CP("ledLogic");
  // ======================================================
  // 4️⃣ + 7️⃣ CENTRALNA LOGIKA LED -> potem dopiero derating/emergency
  // [OK] POPRAWKA: trybAuto/trybManual muszą być PRZED deratingiem,
  // żeby derating działał na finalnych wartościach harmonogramu.
  // ======================================================
  // [OK] FIX-TRYBAUT-v15: Flaga zapobiegająca podwójnemu wywołaniu trybAuto()
  // w tej samej iteracji loop(). Resetuje się automatycznie przy każdym obiegu pętli.
  bool _trybAutoCalledNormal = false;

  // [OK] FIX-v27-ADAPT-GUARD: Fast-loop adaptacyjny musi być PRZED normalną ścieżką.
  // Gdy aktualizujRampeAdaptacyjna() zakończy rampę (rampaAdaptacyjnaAktywna->false),
  // normalna ścieżka 100ms w TEJ SAMEJ iteracji widzi false i woła trybAuto().
  // Δms = 0-1 -> TRYBAUT alarm. Flaga blokuje normalną ścieżkę jeśli fast-loop
  // adaptacyjny już uruchomił się w tej iteracji.
  bool _adaptFastCalledThisLoop = false;

  // ======================================================
  // 🚀 SZYBKA PĘTLA dla RAMPY ADAPTACYJNEJ - 100 Hz  (przeniesiona PRZED normalną ścieżką)
  // ======================================================
  if (rampaAdaptacyjnaAktywna && power && tryb &&
      (gRampOwner == RAMP_AUTO_ADAPTIVE || gRampOwner == RAMP_NONE)) {
    static unsigned long lastAdaptiveUpdate = 0;
    if (nowMs - lastAdaptiveUpdate >= 10) {
      lastAdaptiveUpdate = nowMs;
      _adaptFastCalledThisLoop = true;  // [OK] FIX-v27: zaznacz, że fast-loop adaptacji działał
      aktualizujRampeAdaptacyjna();  // [OK] Płynna rampa 30s - może ustawić rampaAdaptacyjnaAktywna=false
      updateLEDs();
    }
  }

  if (nowMs - lastLogicRun >= logicInterval) {
    // [OK] FIX-v37-TRYBAUT-ROOT: użyj millis() nie nowMs.
    // nowMs jest przechwycone RAZ na początku loop() - gdy loop() trwa ~350ms,
    // lastLogicRun=nowMs(stare) -> następna iteracja: millis()-nowMs=350ms>=100
    // -> 100ms timer strzela NATYCHMIAST -> trybAuto Δ=1-2ms -> TRYBAUT alarm.
    // Z millis(): lastLogicRun=aktualny czas -> następna iteracja czeka prawdziwe 100ms.
    lastLogicRun = millis();

    // [OK] BUG#1 FIX: nie wywoluj trybAuto gdy szybka petla obsluguje rampe (unikamy podwojnego kroku)
    // [OK] FIX-FLASH: nie wywoluj trybAuto gdy rampaAdaptacyjnaAktywna - trybAuto() co 100ms
    //    zeruje backup w isNightExtended, rampa (co 10ms) go ustawia -> 915/0 -> blyski
    // [OK] FIX-v27-ADAPT-GUARD: dodano !_adaptFastCalledThisLoop - blokuje wywołanie gdy
    //    fast-loop adaptacyjny zakończył rampę w tej samej iteracji (rampaAdaptacyjnaAktywna
    //    jest już false, ale Δms ≈ 0 -> TRYBAUT alarm).
    if (power && tryb && !softStartActive && !transitionActive && !rampaAdaptacyjnaAktywna
        && !_adaptFastCalledThisLoop) {
      trybAuto("loop-normal-100ms");
      _trybAutoCalledNormal = true;  // [OK] FIX-TRYBAUT-v15: zaznacz wywołanie normalnej ścieżki
    } else if (power && !tryb) {
      trybManual();
    }

    // [OK] POPRAWKA: updateLEDs() tylko jeśli brak aktywnej szybkiej rampy
    // (szybkie pętle poniżej same wywołują updateLEDs())
    bool fastRampRunning = (softStartActive || transitionActive) && power && tryb;
    bool fastRampManual  = (manualSoftStartActive || manualTransitionActive) && power && !tryb;
    bool adaptiveRunning = rampaAdaptacyjnaAktywna && power && tryb;
    if (!fastRampRunning && !fastRampManual && !adaptiveRunning) {
      updateLEDs();
    }
  }

  // v252: ponowna arbitrażacja po całej ścieżce trybAuto()/MIN LUX.
  // Chroni przed sytuacją, w której ta sama iteracja uruchomi soft-start,
  // a później adaptacja spróbuje przejąć backupBrightnessComposite.
  rampArbiterReconcile();

  // Dopiero PO obliczeniu docelowych wartości LED - derating i emergency
  emergencyThermalShutdown();
  multiZoneLinearDerating();

  LOOP_CP("sunsetRecalc");
  // ======================================================
  // 5️⃣ Codzienne przeliczenie zachodu słońca
  // ======================================================
  if (nowMs - lastSunsetRecalc >= recalcInterval && timeSynced) {

    struct tm ti;
    if (getLocalTimePL(&ti)) {

      int zachodUTC = obliczZachodSlonca(
        ti.tm_year + 1900,
        ti.tm_mon + 1,
        ti.tm_mday,
        latitude,
        longitude
      );

      if (zachodUTC >= 0 && zachodUTC <= 1440) {

        time_t t = mktime(&ti);
        bool dst = isDaylightSaving(t);
        int offset = dst ? timezoneOffsetMinutes
                         : (timezoneOffsetMinutes - 60);

        sunsetMinutes = (zachodUTC + offset + 1440) % 1440;

        if (Komentarze) {
          logPrintf("lvl=INFO tag=NTP msg=\"Zachod slonca przeliczony\" zachod=%02d:%02d\n",
                    sunsetMinutes / 60, sunsetMinutes % 60);
        }
      }
    }
    lastSunsetRecalc = nowMs;
  }

  LOOP_CP("events");
  // ======================================================
  // 6️⃣ EVENTY - ZMIANY STANÓW
  // ======================================================

  // 🟡 ZMIANA SEKCJI
  if (sections != lastSections) {
    // [v158] DIAG-EVENTSSTALL: zmierz realny czas onSectionsChange() —
    // patrz komentarz przy deklaracji g_eventsStallCount wyżej.
    unsigned long _eventsT0 = millis();
    onSectionsChange();
    unsigned long _eventsMs = millis() - _eventsT0;
    if (_eventsMs >= EVENTS_STALL_THRESHOLD_MS) {
      g_eventsStallCount = g_eventsStallCount + 1;
      if (_eventsMs > g_eventsStallMaxMs) g_eventsStallMaxMs = _eventsMs;
      logPrintf("lvl=WARN tag=EVENTS-STALL msg=\"onSectionsChange trwalo dlugo\" ms=%lu sections=%d cnt=%lu maxMs=%lu\n",
                _eventsMs, sections,
                (unsigned long)g_eventsStallCount, (unsigned long)g_eventsStallMaxMs);
    }
    lastSections = sections;
  }

  LOOP_CP("adaptacja");
  // ======================================================
  // 7.5️⃣ UCZENIE I ZAPIS ADAPTACJI
  // ======================================================
  if (uczenieSieWlaczone && czujnikPokojowyAktywny) {
    static unsigned long lastLearnUpdate = 0;
    if (nowMs - lastLearnUpdate >= 30000) { // co 30s
      lastLearnUpdate = nowMs;
      aktualizujUczenie();
      uczSieTransmisji();
    }
  }
  
  // Inteligentny zapis do EEPROM (co 1h lub gdy duża zmiana)
  if (adaptacja.liczbaProbek > 0) {
    inteligentyZapisAdaptacji();
  }

  // [OK] Podsumowanie dnia (co 24h)
  logDailySummary();
  // ── v34: poll i wykonanie komend TG przeniesione do tgTask (Core 0) ──
  // loop() tylko przekazuje tgDeferredCmd do kolejki - zero blokowania sieci tutaj.
  if (tgDeferredCmd != TG_NONE && tgCmdQueue) {
    TgDeferCmd _cmd = tgDeferredCmd;
    tgDeferredCmd = TG_NONE;
    xQueueSend(tgCmdQueue, &_cmd, 0);  // non-blocking; jeśli kolejka pełna - komenda gubi się
  }

  LOOP_CP("minLux");
  // 7.6 MIN LUX MODE - utrzymywanie minimalnego światła
  // [OK] POPRAWKA: Wywołaj tylko gdy NIE jest aktywna rampa harmonogramu
  // [OK] FIX-v16-SENSOR-FALLBACK: Usunięto guard (czujnikPokojowyAktywny || czujnikNadWodaAktywny)
  // Poprzednio: gdy czujnik odpadł, applyMinLuxMode nie była w ogóle wywoływana ->
  // calculateMinLuxPWM() (które ma własny fallback) nigdy nie było osiągane -> LED martwe.
  // Teraz: applyMinLuxMode wywołuje się zawsze gdy minLuxModeEnabled; wewnętrzna logika
  // calculateMinLuxPWM() obsługuje brak czujnika przez fallback do ostatniego PWM.
  if (minLuxModeEnabled && power && tryb
      && !softStartActive && !transitionActive && !rampaAdaptacyjnaAktywna
      && !rampScheduleActive
      && !inLightingWindowGlobal) {
    // [OK] FIX-v25-HEAP-DIAG: Snapshot heap przed/po pierwszym wywołaniu applyMinLuxMode.
    // W log_19 zaobserwowano permanentny spadek minHeap o ~14 kB dokładnie przy pierwszej
    // rampie minLux. Snapshot pozwala potwierdzić lub wykluczyć to miejsce jako źródło.
    static bool _firstMinLuxCall = true;
    if (_firstMinLuxCall && Komentarze) {
      _firstMinLuxCall = false;
      uint32_t _heapBefore = ESP.getFreeHeap();
      uint32_t _minBefore  = ESP.getMinFreeHeap();
      applyMinLuxMode();
      uint32_t _heapAfter  = ESP.getFreeHeap();
      uint32_t _minAfter   = ESP.getMinFreeHeap();
      logPrintf("lvl=INFO tag=HEAP-DIAG msg=\"applyMinLuxMode pierwsze wywolanie\" przedKB=%lu minPrzedKB=%lu poKB=%lu minPoKB=%lu deltaB=%+ld\n",
                _heapBefore/1024, _minBefore/1024, _heapAfter/1024, _minAfter/1024,
                (long)_heapAfter - (long)_heapBefore);
    } else {
      applyMinLuxMode();
    }
  }

  // ======================================================
  // 🚀 SZYBKA PĘTLA dla AUTO (soft-start + transition) - 100 Hz
  // [OK] POPRAWKA: Wywołaj tylko gdy minęło min. 10ms OD OSTATNIEGO wywołania
  //    w szybkiej pętli (niezależnie od logicInterval w normalnej pętli).
  // [OK] FIX-TRYBAUT-v15: Dodano !_trybAutoCalledNormal - blokuje wywołanie fast-loop
  //    gdy normalna ścieżka już wywołała trybAuto() w tej samej iteracji loop().
  //    Zapobiega Δms=1 race condition: normalna ścieżka ustawia softStartActive=true
  //    wewnątrz trybAuto() -> fast-loop odpala je raz jeszcze zanim iteracja się skończy.
  // ======================================================
  if (!_trybAutoCalledNormal && (softStartActive || transitionActive) && power && tryb) {
    static unsigned long lastFastUpdate = 0;
    if (nowMs - lastFastUpdate >= 10) {
      lastFastUpdate = nowMs;
      trybAuto("loop-fast-softstart/trans");
      updateLEDs();
    }
  }

  // ======================================================
  // 🚀 SZYBKA PĘTLA dla MANUAL (soft-start + transition) - 100 Hz
  // ======================================================
  if ((manualSoftStartActive || manualTransitionActive) && power && !tryb) {
    static unsigned long lastFastUpdateManual = 0;
    if (nowMs - lastFastUpdateManual >= 10) {
      lastFastUpdateManual = nowMs;
      trybManual();    // [OK] Obsługuje wszystkie rampy manualne
      updateLEDs();    // aktualizuje PWM
    }
  }

  LOOP_CP("pompa");
  // ======================================================
  // 8️⃣ Pompa
  // ======================================================
  updatePump();

  LOOP_CP("diag");
  // ======================================================
  // 9️⃣ DIAGNOSTYKA SYSTEMOWA (nieblokująca)
  // ======================================================
  runDiagnostics();
}






void trybAuto(const char* caller) {
  // ── Detekcja zbyt częstych wywołań + pełny caller trace ──────────────────
  {
    static unsigned long _lastCallMs  = 0;
    static uint32_t      _callCount   = 0;
    static uint32_t      _fastCount   = 0;
    static unsigned long _lastWarnMs  = 0;
    static bool          _warnedOnce  = false;
    static bool          _prevWasQuick = false;
    static char          _lastCaller[32] = "?"; // [OK] FIX-v27: poprzedni wywołujący
    unsigned long _nowMs = millis();

    // [OK] FIX-3: Pomiń identyczne ms
    if (_nowMs == _lastCallMs) return;
    _callCount++;
    unsigned long _delta = (_lastCallMs > 0) ? (_nowMs - _lastCallMs) : 9999;

    // [OK] FIX-v27-CALLER: Pełny trace wywołań - za osobną flagą, NIE za Komentarze.
    // Komentarze=true domyślnie -> przy Komentarze logowało ~100 linii/s -> LittleFS pełne w minuty.
    // trybAutoTrace włączany ręcznie tylko na czas diagnozy (panel WWW lub Serial).
    if (trybAutoTrace) {
      logPrintf("lvl=INFO tag=TRYBAUT-CALL nr=%lu caller='%s' delta=%lums ss=%d ta=%d ra=%d rs=%d\n",
                _callCount, caller, _delta,
                (int)softStartActive, (int)transitionActive,
                (int)rampaAdaptacyjnaAktywna, (int)rampScheduleActive);
    }

    bool szybkieNormalne = softStartActive || transitionActive || rampaAdaptacyjnaAktywna
                         || rampScheduleActive;

    // [OK] FIX-TRYBAUT: Race condition na granicy fast-loop -> normal-loop
    bool czyFalszywAlarm = _prevWasQuick;
    _prevWasQuick = szybkieNormalne;

    if (_lastCallMs > 0 && _delta < 10 && !szybkieNormalne && !czyFalszywAlarm && !isNightGlobal) {  // [OK] FIX-v29-10: obniżono próg 50->10ms; Δ=1-2ms to jitter millis() ESP32, nie prawdziwy double-call
      _fastCount++;
      if (!_warnedOnce) {
        // [OK] FIX-v27: caller bieżący i poprzedni widoczne w alarmie
        logPrintf("lvl=WARN tag=TRYBAUT msg=\"Zbyt czeste wywolania\" delta=%lums caller='%s' prev='%s'\n",
                  _delta, caller, _lastCaller);
        logPrintf("lvl=WARN tag=TRYBAUT-DBG ss=%d ta=%d ra=%d rs=%d prevwasquick=%d\n",
                  (int)softStartActive, (int)transitionActive,
                  (int)rampaAdaptacyjnaAktywna, (int)rampScheduleActive, (int)_prevWasQuick);
        logPrintf("lvl=WARN tag=TRYBAUT-DBG minlux=%d inwin=%d isnight=%d pwr=%d tryb=%d\n",
                  (int)minLuxModeActive, (int)inLightingWindowGlobal,
                  (int)isNightGlobal, (int)power, (int)tryb);
        _warnedOnce  = true;
        _lastWarnMs  = _nowMs;
      } else if (_nowMs - _lastWarnMs > 600000UL) {
        logPrintf("lvl=WARN tag=TRYBAUT msg=\"Wciaz zbyt czeste\" razy10min=%lu caller='%s' prev='%s'\n",
                  _fastCount, caller, _lastCaller);
        logPrintf("lvl=WARN tag=TRYBAUT-DBG ss=%d ta=%d ra=%d rs=%d\n",
                  (int)softStartActive, (int)transitionActive,
                  (int)rampaAdaptacyjnaAktywna, (int)rampScheduleActive);
        _fastCount   = 0;
        _lastWarnMs  = _nowMs;
      }
    } else if (_warnedOnce && _fastCount == 0 && (_nowMs - _lastWarnMs) > 60000UL) {
      logPrintln("lvl=INFO tag=TRYBAUT msg=\"Czestosc wywolan wrocila do normy\"");
      _warnedOnce = false;
    }

    strncpy(_lastCaller, caller, sizeof(_lastCaller) - 1);
    _lastCaller[sizeof(_lastCaller) - 1] = '\0';
    _lastCallMs = _nowMs;
  }

  // =====================================
  // BLOKADY GLOBALNE
  // =====================================
  if (!power) return;

  int nowMin = getLocalMinutes();
  lastNowMin = nowMin;  // [OK] aktualizuj globalny zegar dla widgetu rampy
  int morningStart = isWeekend() ? MORNING_ON_START_WEEKEND : MORNING_ON_START_WEEKDAY;

  // =====================================
  // WYLICZENIE GRANIC
  // =====================================
  int eveningOnStart = sunsetMinutes + EVENING_ON_BEFORE_SUNSET_MIN;
  if (eveningOnStart < 0) eveningOnStart += 1440;
  if (eveningOnStart >= 1440) eveningOnStart -= 1440;

  int eveningOffEnd = EVENING_OFF_START + fadeMinutes;
  // [OK] FIX #1: Jeśli rampa wieczorna kończy się po północy (np. 23:40 + 30min = 00:10),
  // eveningOffEnd może przekroczyć 1439. Bez tej poprawki isNight nigdy nie staje się true
  // -> LED nie gaszą się w nocy.
  bool eveningOffWraps = (eveningOffEnd >= 1440);
  if (eveningOffWraps) eveningOffEnd -= 1440;

  // =====================================
  // 1️⃣ STAN NOCNY
  // =====================================
  // [OK] FIX-v29-5: Flagi muszą być PRZED early-return nocnym - inaczej noc nigdy
  //    ich nie resetuje -> rano rampScheduleFinished=true blokuje rampę poranną.
  static bool rampFinishPrinted    = false;
  static bool wasRampActive        = false;
  static bool rampScheduleFinished = false;

  bool isNight;
  if (eveningOffWraps) {
    // Rampa kończy się rano (np. 00:10) -> noc to czas od końca rampy do rana
    isNight = (nowMin >= eveningOffEnd) && (nowMin < morningStart);
  } else if (morningStart < EVENING_OFF_START) {
    // Normalny przypadek (rano np. 7:00, wieczór 21:00)
    isNight = (nowMin < morningStart) || (nowMin >= eveningOffEnd);
  } else {
    // Rzadki przypadek (morningStart >= EVENING_OFF_START, np. całonocne świecenie)
    isNight = (nowMin >= eveningOffEnd) && (nowMin < morningStart);
  }

  if (isNight) {
    // [OK] FIX FLASH: Rampa adaptacyjna już prowadzi do 0 - nie zeruj backup,
    // bo trybAuto() wywoływany co 100ms walczyłby z rampą (co 33ms) -> migotanie.
    if (rampaAdaptacyjnaAktywna) return;

    // [OK] FIX BUG-S: Wyczyść backupBrightnessComposite - bez tego multiZoneLinearDerating()
    // wylicza newVal ze snapshot (originalBrightness1/2 > 0) i woła updateLEDs()
    // który re-włącza LED w nocy gdy derating był aktywny w ciągu dnia.
    backupBrightnessComposite = 0;
    rampScheduleActive = false;  // [OK] FIX BUG-Q: Reset flagi rampy na wejściu w noc
    inLightingWindowGlobal = false;  // [OK] FIX MINLUX-WINDOW: Reset flagi okna na wejściu w noc
    isNightGlobal = true;
    for (int i = 0; i < 5; i++) {
      ledcWrite(pinyLED[i], 0);
      rampInitialized[i] = false;
    }
    autoRampInitialized = false;
    softStartActive = false;
    rampScheduleFinished = false;  // [OK] FIX-v29-5
    wasRampActive        = false;  // [OK] FIX-v29-5
    return;
  }

  // [OK] FIX-v121-ISNIGHT-RESET: jedyny dotychczasowy reset isNightGlobal=false
  // znajdował się w onTrybChange() (zmiana trybu MANUAL<->AUTO) - w trybie AUTO
  // bez przełączeń trybu nigdy nie jest wywoływany. Po pierwszej nocy isNightGlobal
  // zostawał TRWALE true przez WSZYSTKIE kolejne dni.
  // SKUTEK: calculateMinLuxPWM() ("if (isNightGlobal) return 0;") zwracało 0 cały dzień
  // -> rampa południowa (rampStart==MIDDAY_OFF_LOCAL) zjeżdżała do PWM=0 (nie do MIN LUX),
  // a applyMinLuxMode() w czasie poprzedniej nocy już wyzerował minLuxCurrentPWM[] - więc
  // fallback FIX-v51 też nie ratował -> LED=0 przez całą przerwę południową drugiego
  // i każdego kolejnego dnia.
  // NAPRAWA: gdy !isNight (jesteśmy w dniu), wymuś isNightGlobal=false na każdej
  // iteracji trybAuto() (~co 100ms) - dokładnie tak jak zakłada komentarz w
  // applyMinLuxMode() ("isNightGlobal aktualizowany co 100ms przez trybAuto").
  isNightGlobal = false;

  // =====================================
  // 2️⃣ WYBÓR AKTYWNEJ RAMPY
  // =====================================
  bool rampActive = false;
  bool rampUp = true;
  int rampStart = 0;

  // 🌙 WIECZÓR OFF
  // [OK] BUG#2 FIX: obsługa przekroczenia północy dla rampy wieczornej OFF
  // Oblicz przed lancuchem if-else zeby nie lamac else-if
  int _evOffEnd = EVENING_OFF_START + fadeMinutes;
  bool _inEvOff = (_evOffEnd >= 1440)
    ? ((nowMin >= EVENING_OFF_START) || (nowMin <= (_evOffEnd - 1440)))
    : (nowMin >= EVENING_OFF_START && nowMin <= _evOffEnd);

  if (_inEvOff) {
    rampActive = true;
    rampUp = false;
    rampStart = EVENING_OFF_START;
  }
  // 🌅 PORANEK
  else if (nowMin >= morningStart && nowMin <= morningStart + fadeMinutes) {
    rampActive = true;
    rampUp = true;
    rampStart = morningStart;
  }
  // 🌤 POŁUDNIE
  else if (MIDDAY_OFF_LOCAL > morningStart &&
           MIDDAY_OFF_LOCAL < EVENING_OFF_START &&
           nowMin >= MIDDAY_OFF_LOCAL &&
           nowMin <= MIDDAY_OFF_LOCAL + fadeMinutes) {
    // [OK] FIX-v13: rampa gaśnięcia ZACZYNA się o MIDDAY_OFF_LOCAL (zgodnie z dashboardem JS)
    //            i kończy fadeMinutes później -> przerwa startuje o MIDDAY_OFF_LOCAL + fadeMinutes
    rampActive = true;
    rampUp = false;
    rampStart = MIDDAY_OFF_LOCAL;
  }
  // 🌇 WIECZÓR
  else if (nowMin >= eveningOnStart && nowMin <= eveningOnStart + fadeMinutes) {
    rampActive = true;
    rampUp = true;
    rampStart = eveningOnStart;
  }

// ═════════════════════════════════════════════════════════════
// 3️⃣ RAMPA AKTYWNA - OBLICZANIE BEZPOŚREDNIE
// ═════════════════════════════════════════════════════════════

if (rampActive) {
  wasRampActive = true;  // [OK] FIX-v20: wasRampActive był zawsze false - reset-block nigdy
  // nie widział przejścia true->false - rampScheduleFinished nie był kasowany między
  // rampami - każda rampa po pierwszej wchodziła w early-exit bez działania.
  // [OK] FIX-v19b: Użyj dedykowanej flagi rampScheduleFinished zamiast
  // autoRampInitialized&&rampInitialized[0] — tamto spełniało się już po pierwszej
  // iteracji rampy i blokowało wszystkie kolejne aktualizacje PWM -> rampa stała w miejscu.
  if (rampScheduleFinished) {
    rampScheduleActive = false;  // rampa skończona - nie blokuj applyMinLuxMode
    inLightingWindowGlobal = rampUp;  // UP=okno świecenia; DOWN=przerwa (minLux rządzi)
    updateLEDs();
    return;
  }
  if (!rampArbiterTryStart(RAMP_AUTO_SCHEDULE, false)) {
    inLightingWindowGlobal = rampUp;
    return;
  }
  rampScheduleActive = true;  // [OK] FIX A: Informuj loop() że rampa harmonogramu jest aktywna
  inLightingWindowGlobal = true;  // [OK] FIX-v18 BUG#3: rampActive zawsze w oknie świecenia - bez tego
  // adaptacja, zapis EEPROM z panelu i applyMinLuxMode widziały false przez całą rampę ranną/wieczorną.
  if (!wasRampActive) { gPowerScale = 1.0f; gPowerScaleLastLogged = 1.0f; }  // balancer nie walczy z rampą
  unsigned long nowMs = millis();
  unsigned long totalRampMs = (unsigned long)fadeMinutes * 60UL * 1000UL;

  // ── Ramp widget tracking: zapisz dane przy STARCIE rampy lub zmianie okna ──
  if (!rampWidgetWasActive || rampWidgetRampStartMin != rampStart) {
    rampWidgetStartMs      = nowMs;
    rampWidgetUp           = rampUp;
    rampWidgetWasActive    = true;
    rampWidgetRampStartMin = rampStart;
    // [OK] FIX-v16: Po resecie backupBrightnessComposite=0 -> widget pokazywałby 0->295 zamiast
    // rzeczywistego punktu rampy. Odtwórz z backupAuto * progress (UP) lub (1-progress) (DOWN).
    uint16_t rawStart = (uint16_t)getBrightnessForSection(backupBrightnessComposite, 0);
    if (rawStart == 0 && !autoRampInitialized &&
        getBrightnessForSection(backupAutoBrightnessComposite, 0) > 0) {
      uint16_t autoVal = (uint16_t)getBrightnessForSection(backupAutoBrightnessComposite, 0);
      int elapsedW = nowMin - rampStart;
      if (elapsedW < 0) elapsedW += 1440;
      elapsedW = constrain(elapsedW, 0, fadeMinutes);
      float progW = (fadeMinutes > 0) ? ((float)elapsedW / (float)fadeMinutes) : 0.0f;
      rawStart = rampUp ? (uint16_t)(autoVal * progW) : (uint16_t)(autoVal * (1.0f - progW));
    }
    rampWidgetPwmStart  = rawStart;
    // [OK] FIX-v16b: Rampa południowa opada do minLux (nie do 0) - widget musi znać właściwy cel.
    // Bez tej poprawki: pasek postępu na dashboardzie liczył do 0 zamiast do minLuxPWM.
    uint16_t _widgetTarget = 0;
    if (!rampUp) {
      if (rampStart == MIDDAY_OFF_LOCAL && minLuxModeEnabled) {
        _widgetTarget = calculateMinLuxPWM();  // [OK] FIX-v18 BUG#6: usunięto guard czujnika - calculateMinLuxPWM() ma wbudowany fallback (spójność z FIX-v16 w pętli target)
      }
      // rampa wieczorna OFF: cel=0 (domyślne)
    }
    rampWidgetPwmTarget = rampUp
      ? (uint16_t)getBrightnessForSection(backupAutoBrightnessComposite, 0)
      : _widgetTarget;
  }

  bool allFinished = true;

  for (int i = 0; i < 5; i++) {
    uint16_t target;
    if (rampUp) {
      target = getBrightnessForSection(backupAutoBrightnessComposite, i);
    } else {
      // ⭐ CEL RAMPY W DÓŁ - MIN LUX lub 0
target = 0;

// Jeśli tryb MIN LUX włączony i to rampa POŁUDNIOWA, celuj w wartość MIN LUX
// [OK] FIX-v16-SENSOR-FALLBACK: Usunięto guard (czujnikPokojowyAktywny || czujnikNadWodaAktywny)
// calculateMinLuxPWM() ma teraz wbudowany fallback do ostatniego PWM gdy czujnik odpada.
if (minLuxModeEnabled) {
  // Sprawdź czy to rampa południowa (nie wieczorna)
  if (rampStart == MIDDAY_OFF_LOCAL) {
    target = calculateMinLuxPWM();
    // Log tylko raz dla kanału 0
    if (i == 0 && Komentarze) {
      static unsigned long lastRampTargetLog = 0;
      if (millis() - lastRampTargetLog > 60000) { // Log co 1 min
        logPrintf("lvl=INFO tag=RAMPA-POLUDNIE msg=\"Cel MIN LUX\" pwm=%d\n", target);
        lastRampTargetLog = millis();
      }
    }
  }
}

    }

    currentRampTarget[i] = target;
// ⭐ SPECJALNA OBSŁUGA: Rampa wieczorna startuje z MIN LUX
if (rampUp && !rampInitialized[i]) {
  // Sprawdź czy to start rampy wieczornej
  if (rampStart == eveningOnStart) {
    // Jeśli tryb MIN LUX był aktywny - startuj z jego wartości
    if (minLuxModeActive && minLuxCurrentPWM[0] > 0) {
      // [OK] FIX JUMP: użyj AKTUALNEJ wartości backupBrightness, nie zamrożonego minLuxCurrentPWM.
      // Adaptacyjna regulacja może podbić PWM wyżej niż minLuxCurrentPWM -> mrugnięcie przy starcie.
      uint16_t actualPwm = getBrightnessForSection(backupBrightnessComposite, i);
      currentRampPwm[i] = actualPwm;
      // [OK] FIX MINLUX-RAMP: Zapamiętaj wartość startową dla formuły rampy UP
      rampDownStartValue[i] = actualPwm;
      if (i == 0 && Komentarze) {
        logPrintf("lvl=INFO tag=RAMPA-WIECZOR msg=\"Start z aktualnej wartosci\" actual=%d minluxTarget=%d cel=%d\n",
                  actualPwm, minLuxCurrentPWM[0], target);
      }
      rampInitialized[i] = true;
      lastPwmStepMillis[i] = nowMs;
      
      // Oblicz interwał dla rampy (płynne przejście do target)
      if (target > currentRampPwm[i]) {
        int diff = target - currentRampPwm[i];
        if (diff > 0) {
          stepIntervalMs[i] = ((unsigned long)fadeMinutes * 60UL * 1000UL) / diff;
        } else {
          stepIntervalMs[i] = 1000;
        }
      }
      // [OK] FIX BUG-FLASH: Zastosuj aktualną wartość minLux PRZED continue
      setBrightnessForSection(backupBrightnessComposite, i, currentRampPwm[i]);
      // [OK] FIX BUG-FLASH: continue pomijało allFinished=false -> rampa natychmiast "kończyła się"
      // przy target=317 (błysk), a następna iteracja liczyła progress≈0 -> PWM=0 (zgaszenie).
      allFinished = false;
      continue; // Pomiń normalną inicjalizację poniżej
    }
  }
}

    // Inicjalizacja przy starcie rampy
if (!rampInitialized[i]) {
  // Oblicz ile minut minęło od początku rampy
  int elapsedMinutes = nowMin - rampStart;
  
  // Obsługa przejścia przez północ
  if (elapsedMinutes < 0) {
    elapsedMinutes += 1440;
  }
  
  // Ogranicz do czasu rampy
  elapsedMinutes = constrain(elapsedMinutes, 0, fadeMinutes);
  
// Oblicz proporcjonalną wartość dla UP i DOWN

if (fadeMinutes <= 0) {
    return;
}

float progress = (float)elapsedMinutes / (float)fadeMinutes;

  if (rampUp) {
    // [OK] FIX #1 - RESTART W DZIEŃ: Jeśli LED już świeci (≥ target), rampa UP jest zbędna.
    // Bez tej poprawki: po restarcie w ciągu dnia soft-start doprowadza LED do pełnej jasności,
    // a chwilę później rampa wieczorna (eveningOnStart) zeruje backlight i zaczyna rampę od 0
    // - powodując pozorne "przesuniecie harmonogramu" o kilka godzin.
    uint16_t currentBrightness = getBrightnessForSection(backupBrightnessComposite, i);
    if (currentBrightness >= target && target > 0) {
      currentRampPwm[i] = target;
      rampInitialized[i] = true;
      // Ustaw timestamp tak, żeby progress=1.0 -> pętla rampowa nie zmieni wartości
      unsigned long fullRampMs = (unsigned long)fadeMinutes * 60UL * 1000UL;
      lastPwmStepMillis[i] = nowMs - fullRampMs;  // [OK] FIX-v17: unsigned wrap - guard (nowMs >= fullRampMs) był fałszywy przy restarcie
      if (Komentarze && i == 0) {
        logPrintf("lvl=INFO tag=RAMPA-INIT faza=WZNOSZENIE msg=\"Pominieto, LED juz swieci, prawdopodobnie po restarcie\" pwm=%d cel=%d\n",
                  currentBrightness, target);
      }
      continue;  // pomiń resztę inicjalizacji dla tego kanału
    }
    // RAMPA UP: 0 -> target (normalny przypadek - restart w nocy/rano lub start po przerwie)
    currentRampPwm[i] = (uint16_t)(target * progress);
    // [OK] FIX FLASH-MORNING: Zeruj rampDownStartValue aby nie dziedziczyć
    // wartości z poprzedniej wieczornej rampy minLux (np. 296).
    // Bez tego: UPDATE widzi startVal=296, target=296 -> expectedPwm=296 od razu -> BŁYSK.
    rampDownStartValue[i] = 0;
    
    if (Komentarze && i == 0) {
      logPrintf("lvl=INFO tag=RAMPA-INIT faza=WZNOSZENIE minelo=%d z=%dmin start=%d cel=%d procent=%.1f\n",
        elapsedMinutes, fadeMinutes, currentRampPwm[i], target, progress * 100.0f);
    }
  } else {
    // RAMPA DOWN: startValue -> target
    // [OK] POPRAWKA: Startuj z AKTUALNEJ wartości backupBrightness (może być zderedowana termicznie),
    // NIE z backupAutoBrightnessComposite - inaczej LED skacze na początku rampy gdy derating aktywny
    // [v236] FIX-RAMPA-SUWAK-1: jeśli suwak WŁAŚNIE ustawił nową wartość (patrz /set-pwm),
    // użyj jej jako startu rampy zamiast na nowo czytać backupBrightnessComposite — to
    // ostatnie mogło już zostać nadpisane przez inne przeliczenie harmonogramu w
    // międzyczasie (wiele wywołań trybAuto() między kliknięciem a tym momentem).
    uint16_t startValue;
    if (pendingManualRampStart[i] >= 0) {
      startValue = (uint16_t)pendingManualRampStart[i];
      pendingManualRampStart[i] = -1;  // zużyta - nie używaj ponownie w kolejnych tickach
    } else {
      startValue = getBrightnessForSection(backupBrightnessComposite, i);
    }

    // [OK] FIX-RESET-RAMP-DOWN (v16): Po restarcie backupBrightnessComposite=0 bo nie jest w EEPROM.
    // Bez tej poprawki: startValue=0, rampDownStartValue=0, expectedPwm=0 przez całe okno
    // -> natychmiastowe zgaśnięcie (rampa wieczorna) lub skok do minLux (rampa południowa).
    // Naprawa: gdy startValue==0 i !autoRampInitialized (=restart w trakcie rampy),
    // odtwórz startValue z backupAuto - punkt t=0 rampy. lastPwmStepMillis[i] ustawiony
    // wstecz o elapsedMin daje identyczny progress w pętli UPDATE -> płynna kontynuacja.
    // Dodatkowy warunek backupAuto>0: chroni przed fałszywym wejściem gdy użytkownik
    // celowo ustawił zerową jasność (wtedy startValue=0 jest poprawne).
    if (startValue == 0 && !autoRampInitialized &&
        getBrightnessForSection(backupAutoBrightnessComposite, i) > 0) {
      startValue = getBrightnessForSection(backupAutoBrightnessComposite, i);
      if (Komentarze && i == 0) {
        logPrintf("lvl=WARN tag=FIX-v16 msg=\"Reset w trakcie rampy DOWN, startValue odtworzony z backupAuto\" backupauto=%d elapsed=%d z=%dmin\n",
                  startValue, elapsedMinutes, fadeMinutes);
      }
    }

    rampDownStartValue[i] = startValue;  // [OK] FIX #2: Zapamiętaj do użycia w pętli rampowej
    currentRampPwm[i] = (startValue >= target)
      ? startValue - (uint16_t)((startValue - target) * progress)
      : target;  // zabezpieczenie przed underflow gdy startValue < target
    
    if (Komentarze && i == 0) {
      logPrintf("lvl=INFO tag=RAMPA-INIT faza=OPADANIE minelo=%d z=%dmin start=%d cel=%d procent=%.1f\n",
        elapsedMinutes, fadeMinutes, currentRampPwm[i], target, progress * 100.0f);
    }
  }
  
  rampInitialized[i] = true;
  
  // [OK] KLUCZOWA POPRAWKA: Ustaw timestamp WSTECZ z ochroną przed underflow
  unsigned long elapsedMs = (unsigned long)elapsedMinutes * 60UL * 1000UL;
  // [OK] POPRAWKA: Jeśli elapsedMs > nowMs (po overflow millis ~49dni lub bardzo długa rampa),
  // nie rób ujemnego unsigned - po prostu ustaw na 0 (rampa zacznie od początku)
  lastPwmStepMillis[i] = nowMs - elapsedMs;  // [OK] FIX-v17: unsigned wrap (wzorzec millis()) - guard niszczył INIT rampy UP po restarcie
}




    // [OK] OBLICZ BEZPOŚREDNIO gdzie PWM POWINNO BYĆ
    unsigned long elapsedMs = nowMs - lastPwmStepMillis[i];
    float progress = (float)elapsedMs / (float)totalRampMs;
    if (progress > 1.0f) progress = 1.0f;

    uint16_t expectedPwm;
    if (rampUp) {
      // [OK] FIX SUPPLY: Jeśli rampa UP startowała z minLux (nie z 0),
      // użyj rampDownStartValue[i] jako punktu startowego (tam zapisujemy minLux).
      // [OK] FIX BUG-SUPPLY: Poprzedni warunek `target > startVal` był niespełniony gdy
      // startVal==target (np. rampa wieczorna startuje z minLux=38, cel=38) ->
      // wpadał w else: `target * progress` ≈ 0 -> backup zerował się -> zasilacz WYŁ.
      // Poprawka: gdy startVal >= target (ramp już na celu lub powyżej) -> trzymaj target.
      uint16_t startVal = rampDownStartValue[i];
      if (startVal > 0 && target >= startVal) {
        // startVal <= target: interpolacja od startVal do target (lub stay gdy equal)
        expectedPwm = startVal + (uint16_t)((target - startVal) * progress);
      } else if (startVal == 0) {
        // Normalny start od zera
        expectedPwm = (uint16_t)(target * progress);
      } else {
        // startVal > target: LED świeci mocniej niż cel -> opuść płynnie
        expectedPwm = startVal - (uint16_t)((startVal - target) * progress);
        if (expectedPwm < target) expectedPwm = target;
      }
    } else {
      // [OK] FIX #2: Użyj wartości startowej zapisanej przy inicjalizacji (uwzględnia derating termiczny).
      // Poprzedni kod używał backupAutoBrightnessComposite -> LED "skakał" w górę gdy derating był aktywny.
      // Dodatkowo spójna formuła: startValue - (startValue - target) * progress (zamiast startValue * progress)
      uint16_t startValue = rampDownStartValue[i];
      int diff = (int)startValue - (int)target;
      expectedPwm = (diff >= 0)
        ? (uint16_t)(startValue - (uint16_t)(diff * progress))
        : target;  // Zabezpieczenie: jeśli startValue < target (np. derating zmienił wartości) -> zostań przy target
    }

    currentRampPwm[i] = expectedPwm;
    setBrightnessForSection(backupBrightnessComposite, i, currentRampPwm[i]);

    // Sprawdź czy osiągnięto target
    if (progress < 1.0f) {
      allFinished = false;
    }

  }

 // [OK] POPRAWKA - po zakończeniu rampy NIE resetuj flag, tylko wyjdź
if (allFinished) {
  autoRampInitialized = true;
  rampScheduleFinished = true;  // [OK] FIX-v19b: ustaw tylko tu - po faktycznym zakończeniu
  
  // [OK] POPRAWKA: Pokaż tylko raz
  if (!rampFinishPrinted && Komentarze) {
    logPrintf("%s lvl=INFO tag=RAMPA msg=\"Zakonczona\" kierunek=%s pwm=%d proc=%.0f%%\n",
              logTime().c_str(),
              rampUp ? "gora" : "dol",
              currentRampTarget[0],
              currentRampTarget[0] * 100.0f / 1023.0f);
    rampFinishPrinted = true;
  }
  
// Ustaw wartości docelowe i wyjdź - nie liczy w kółko
for (int i = 0; i < 5; i++) {
  setBrightnessForSection(backupBrightnessComposite, i, currentRampTarget[i]);
  currentRampTarget[i] = 0;  // [OK] DODAJ TĘ LINIĘ
}
rampScheduleActive = false;  // [OK] BUG#4 FIX: zeruj flagę natychmiast po zakończeniu rampy
rampWidgetWasActive = false;   // ── Ramp widget: reset po zakończeniu
// [OK] FIX-RACE: Po zakończeniu rampy zjazdu (np. 11:00) resetuj timer MIN LUX.
// Bez tego: rampScheduleActive=false ale minLuxModeActive=false przez do 30s
// -> trybAuto() widzi isNightExtended=true, minLuxModeActive=false -> zeruje LED -> błysk.
// Z tym: applyMinLuxMode() odpala NATYCHMIAST w następnym loop(), ustawia Active=true
// ZANIM trybAuto() zdąży wyzerować backup.
lastMinLuxUpdate = 0;

// [OK] FIX-v13-MINLUX-RACE: Po zakończeniu rampy południe natychmiast aktywuj MIN LUX.
// [OK] FIX-v16-SENSOR-FALLBACK: Usunięto guard (czujnikPokojowyAktywny || czujnikNadWodaAktywny)
// calculateMinLuxPWM() ma własny fallback gdy czujnik niedostępny.
if (!rampUp && rampStart == MIDDAY_OFF_LOCAL && minLuxModeEnabled && !minLuxModeActive) {
  // [OK] FIX-v19: dodano !minLuxModeActive - bez tego blok uruchamiał się ponownie
  // przy każdym wywołaniu trybAuto() (spam setek logów gdy okno rampy jeszcze trwało)
  uint16_t mlPwm = calculateMinLuxPWM();
  // [OK] FIX-v51-MINLUX-INIT: Gdy calculateMinLuxPWM() zwraca 0 (czujnik chwilowo odpada
  // dokładnie w momencie zakończenia rampy południowej), minLuxCurrentPWM[0] zostaje 0,
  // a log pokazuje "Rampa południe cel MIN LUX 0 PWM".
  // Pierwsze wywołanie applyMinLuxMode() widzi minLuxModeActive=false i minLuxCurrentPWM[0]=0
  // -> Fallback 1 nie zachodzi -> Fallback 2 oblicza wartość, ale nie inicjalizuje
  // minLuxCurrentPWM[] przed powrotem (FIX-v51 w calculateMinLuxPWM naprawia to),
  // jednak minLuxModeActive nadal nie jest ustawione tutaj -> aktywacja jest opóźniona
  // do następnego wywołania applyMinLuxMode (do 30s przerwy).
  // Naprawa: gdy mlPwm==0 ale calculateMinLuxPWM() zapisało fallback do minLuxCurrentPWM[],
  // użyj tej wartości by aktywować MIN LUX natychmiast.
  if (mlPwm == 0 && minLuxCurrentPWM[0] > 0) {
    mlPwm = minLuxCurrentPWM[0];
    if (Komentarze) logPrintf("lvl=WARN tag=FIX-v51 msg=\"Rampa poludnie: fallback PWM po zaniku czujnika\" pwm=%d\n", mlPwm);
  }
  if (mlPwm > 0) {
    for (int i = 0; i < 5; i++) {
      minLuxCurrentPWM[i] = mlPwm;
      setBrightnessForSection(backupBrightnessComposite, i, mlPwm);
    }
    minLuxModeActive = true;
    if (Komentarze) logPrintf("lvl=INFO tag=FIX-v13 msg=\"Rampa poludnie zakonczona, minLuxModeActive aktywowany\" pwm=%d\n", mlPwm);
  }
}
updateLEDs();
return;

}


  // Rampa w trakcie
  autoRampInitialized = true;
  softStartActive = false;
  updateLEDs();
  return;
}

// Reset flagi gdy rampa się zmieni
if (rampActive != wasRampActive) {
  rampFinishPrinted = false;
  rampScheduleFinished = false;  // [OK] FIX-v19b: nowe okno rampy = nowa rampa
  wasRampActive = rampActive;
}

// [OK] FIX A: Rampa harmonogramu nieaktywna - pozwól applyMinLuxMode działać
rampScheduleActive = false;
rampWidgetWasActive = false;   // ── Ramp widget: reset gdy rampa nieaktywna

// [OK] Reset flag TYLKO gdy rampa nieaktywna (poza oknem czasowym)
for (int i = 0; i < 5; i++) {
  rampInitialized[i] = false;
}


 // =====================================
// 4️⃣ SOFT-START PO RESECIE
// =====================================
if (softStartActive) {
  unsigned long nowMs = millis();
  bool allFinished = true;

  for (int i = 0; i < 5; i++) {
    uint16_t target = softStartTargetPwm[i];  // używaj overridu (mid-ramp lub pełny cel)

    if (nowMs - softStartStepMillis[i] >= softStartIntervalMs[i]) {
      if (softStartPwm[i] < target) {
        softStartPwm[i]++;
        softStartStepMillis[i] = nowMs;
      }
    }

    setBrightnessForSection(backupBrightnessComposite, i, softStartPwm[i]);

    if (softStartPwm[i] < target) {
      allFinished = false;
    }
  }

  if (allFinished) {
    softStartActive = false;
  }
  updateLEDs();  // [OK] DODANE
  return;
}

// Inicjalizacja soft-startu (tylko raz po resecie)
if (!autoRampInitialized) {
  // [OK] WYKRYWANIE NOCY - jeśli NOC, ustaw na 0 i wyjdź
  // [OK] FIX #1: Użyj eveningOffWraps obliczonego powyżej (ta sama logika co w isNight)
  bool isNightTime;
  if (eveningOffWraps) {
    isNightTime = (nowMin >= eveningOffEnd) && (nowMin < morningStart);
  } else if (morningStart < EVENING_OFF_START) {
    isNightTime = (nowMin < morningStart) || (nowMin >= eveningOffEnd);
  } else {
    isNightTime = (nowMin >= eveningOffEnd) && (nowMin < morningStart);
  }

  if (isNightTime) {
    autoRampInitialized = true;
    for (int i = 0; i < 5; i++) {
      setBrightnessForSection(backupBrightnessComposite, i, 0);
    }
    if (Komentarze) {
      logPrintln("lvl=INFO tag=AUTO msg=\"Reset w nocy, swiatla wylaczone\"");
    }
    updateLEDs();  // [OK] DODANE
    return;
  }

  // Sprawdź czy są jakieś targety
  bool anyTarget = false;
  for (int i = 0; i < 5; i++) {
    if (getBrightnessForSection(backupAutoBrightnessComposite, i) > 0) {
      anyTarget = true;
      break;
    }
  }

  // Sprawdź czy jesteśmy w oknie świecenia
  bool shouldBeOn = false;
  int morningEnd = morningStart + fadeMinutes;
  int middayOffEnd = MIDDAY_OFF_LOCAL + fadeMinutes;  // [OK] FIX-v13: przerwa zaczyna się PO rampie
  int eveningOnEnd = eveningOnStart + fadeMinutes;

  if (anyTarget) {
    // [OK] FIX #2 - RESTART W DZIEŃ: inMorningWindow musi być ograniczone przez eveningOnStart,
    // żeby restart po południu (np. 16:29) nie "zaliczał się" do okna porannego
    // gdy MIDDAY_OFF_LOCAL jest późniejsze niż eveningOnStart.
    // Użyj min(MIDDAY_OFF_LOCAL - fadeMinutes, eveningOnStart) - rampa południe zaczyna fadeMinutes przed MIDDAY_OFF.
    int morningWindowEnd = min((int)MIDDAY_OFF_LOCAL, eveningOnStart);  // [OK] FIX-v13: rano trwa do startu rampy południe
    bool inMorningWindow = (nowMin > morningEnd && nowMin < morningWindowEnd);

    // [OK] FIX-v28-MINLUX-WINDOW: Okno wieczorne = od startu rampy wieczornej do EVENING_OFF_START.
    // Scalone z poprzednim inEveningRampWindow - przerwa (middayOffEnd..eveningOnStart)
    // daje shouldBeOn=false -> inLightingWindowGlobal=false -> MIN LUX aktywny. [OK]
    bool inEveningWindow    = (nowMin >= eveningOnStart && nowMin < EVENING_OFF_START);

    shouldBeOn = inMorningWindow || inEveningWindow;
  }

  // [OK] FIX-A: Aktualizuj flagę okna świecenia co wywołanie trybAuto (było tylko przy zmianie trybu)
  inLightingWindowGlobal = shouldBeOn;

  if (shouldBeOn) {
    if (!rampArbiterTryStart(RAMP_AUTO_SOFTSTART, false)) return;
    softStartActive = true;
    gPowerScale = 1.0f; gPowerScaleLastLogged = 1.0f;  // balancer nie walczy z soft-startem
    unsigned long nowMs = millis();
    unsigned long totalSoftStartMs = SOFTSTARTSECONDS * 1000UL;

    for (int i = 0; i < 5; i++) {
      uint16_t target = getBrightnessForSection(backupAutoBrightnessComposite, i);
      softStartPwm[i] = 0;
      softStartTargetPwm[i] = target;  // pełny cel (przed kapowaniem)
      softStartStepMillis[i] = nowMs;
    }

    // [FIX-FLASH3] Kap cele do limitu mocy zasilacza PRZED startem rampy.
    // Bez tego balancer po zakończeniu soft-startu widzi 118.7W > 90W i skacze
    // natychmiast (skala 1.0 -> 0.758 = delta -247 PWM = widoczny błysk).
    {
      float prevPwr = getTotalLEDPower(softStartTargetPwm);
      float appliedScale = capTargetsToPowerLimit(softStartTargetPwm);
      if (Komentarze) {
        if (appliedScale < 0.999f)
          logPrintf("lvl=WARN tag=SSTART-INIT kontekst=BASE target_przed=%d moc=%.1f limit=%d skala=%.3f target_po=%d\n",
            getBrightnessForSection(backupAutoBrightnessComposite, 0),
            prevPwr, LED_MAX_POWER_W, appliedScale, softStartTargetPwm[0]);
        else
          logPrintf("lvl=INFO tag=SSTART-INIT kontekst=BASE target=%d moc=%.1f limit=%d msg=\"brak ograniczenia\"\n",
            softStartTargetPwm[0], prevPwr, LED_MAX_POWER_W);
      }
    }

    // Przelicz interwały kroku po ewentualnym kapowaniu targetów
    for (int i = 0; i < 5; i++)
      softStartIntervalMs[i] = (softStartTargetPwm[i] > 0) ? (totalSoftStartMs / softStartTargetPwm[i]) : 1000;

    autoRampInitialized = true;
    if (Komentarze)
      logPrintf("lvl=INFO tag=AUTO msg=\"Miekki start po restarcie\" nowMin=%d morningEnd=%d eveningOnStart=%d\n",
                nowMin, morningEnd, eveningOnStart);
    updateLEDs();  // [OK] DODANE
    return;
  }

  // ── UWAGA v16: Martwy kod - reset w trakcie aktywnej rampy ──
  // Ten blok jest NIEOSIĄGALNY gdy rampActive=true, ponieważ blok
  // `if (rampActive) { ... return; }` (powyżej) zawsze wykonuje return przed
  // dotarciem tutaj. autoRampInitialized=false jest warunkiem wejścia w
  // `if (!autoRampInitialized)`, ale przy rampActive=true kod nigdy tu nie trafia.
  //
  // Dla ramp DOWN naprawa resetu jest w bloku init RAMPA DOWN (FIX-RESET-RAMP-DOWN v16).
  // Dla ramp UP (rano/wieczór ON) reset jest bezpieczny (formuła target*progress nie
  // używa backupBrightnessComposite).
  //
  // Blok pozostawiony celowo - nie usuwać bez refaktoryzacji przepływu trybAuto().
  {
    bool inMorningRamp   = (nowMin >= morningStart && nowMin <= morningStart + fadeMinutes);
    bool inMiddayRamp    = (MIDDAY_OFF_LOCAL > morningStart && MIDDAY_OFF_LOCAL < EVENING_OFF_START &&
                            nowMin >= MIDDAY_OFF_LOCAL && nowMin <= MIDDAY_OFF_LOCAL + fadeMinutes);  // [OK] FIX-v13
    bool inEveningOnRamp = (nowMin >= eveningOnStart && nowMin <= eveningOnStart + fadeMinutes);
    int  _evOffEnd2      = EVENING_OFF_START + fadeMinutes;
    bool inEveningOffRamp = (_evOffEnd2 >= 1440)
      ? (nowMin >= EVENING_OFF_START || nowMin <= (_evOffEnd2 - 1440))
      : (nowMin >= EVENING_OFF_START && nowMin <= _evOffEnd2);

    if (anyTarget && (inMorningRamp || inMiddayRamp || inEveningOnRamp || inEveningOffRamp)) {
      // Oblicz gdzie w rampie jesteśmy TERAZ
      bool rampGoingUp;
      int  rampStartMin;
      if (inMorningRamp)    { rampGoingUp = true;  rampStartMin = morningStart; }
      else if (inMiddayRamp)    { rampGoingUp = false; rampStartMin = MIDDAY_OFF_LOCAL; }  // [OK] FIX-v13
      else if (inEveningOnRamp) { rampGoingUp = true;  rampStartMin = eveningOnStart; }
      else                      { rampGoingUp = false; rampStartMin = EVENING_OFF_START; }

      int elapsedMin = nowMin - rampStartMin;
      if (elapsedMin < 0) elapsedMin += 1440;
      elapsedMin = constrain(elapsedMin, 0, fadeMinutes);
      float progress = (fadeMinutes > 0) ? ((float)elapsedMin / (float)fadeMinutes) : 1.0f;

      // Cel soft-startu = aktualny punkt rampy (nie pełna jasność!)
      unsigned long nowMs = millis();
      const unsigned long MID_RAMP_SOFT_MS = 30000UL;  // [OK] 30 sekund dobiegu (było 10s)

      if (!rampArbiterTryStart(RAMP_AUTO_SOFTSTART, false)) return;
      softStartActive = true;
      gPowerScale = 1.0f; gPowerScaleLastLogged = 1.0f;

      for (int i = 0; i < 5; i++) {
        uint16_t fullTarget = getBrightnessForSection(backupAutoBrightnessComposite, i);
        uint16_t midTarget;
        if (rampGoingUp) {
          midTarget = (uint16_t)(fullTarget * progress);
        } else {
          midTarget = (uint16_t)(fullTarget * (1.0f - progress));
        }
        if (midTarget < 1) midTarget = 1;  // zawsze co najmniej 1 krok żeby soft-start ruszył

        softStartPwm[i]       = 0;
        softStartTargetPwm[i] = midTarget;  // CEL = punkt rampy, nie pełna jasność
        softStartStepMillis[i]= nowMs;
      }

      // [FIX-FLASH3] Kap mid-ramp target do limitu mocy
      {
        float prevPwr = getTotalLEDPower(softStartTargetPwm);
        float appliedScale = capTargetsToPowerLimit(softStartTargetPwm);
        if (Komentarze && appliedScale < 0.999f)
          logPrintf("lvl=WARN tag=SSTART-INIT kontekst=MID moc=%.1f limit=%d skala=%.3f\n",
            prevPwr, LED_MAX_POWER_W, appliedScale);
      }

      // Przelicz interwały kroku po ewentualnym kapowaniu
      for (int i = 0; i < 5; i++)
        softStartIntervalMs[i] = (softStartTargetPwm[i] > 0) ? (MID_RAMP_SOFT_MS / softStartTargetPwm[i]) : 1000;

      autoRampInitialized = true;
      if (Komentarze) {
        logPrintf("lvl=INFO tag=AUTO msg=\"Mid-ramp restart, dobieg 30s\" rampStart=%d elapsed=%d/%dmin progress=%.1f%%\n",
                  rampStartMin, elapsedMin, fadeMinutes, progress * 100.0f);
      }
      updateLEDs();
      return;
    }
  }

  // Żadne okno nie pasuje - reset poza oknem świecenia
  {
    autoRampInitialized = true;
    // [OK] FIX-v20-MINLUX-ZERO: nie zeruj backup gdy MIN LUX aktywnie steruje LED
    if (!minLuxModeActive) {
      for (int i = 0; i < 5; i++) {
        setBrightnessForSection(backupBrightnessComposite, i, 0);
      }
      if (Komentarze) {
        logPrintln("lvl=INFO tag=AUTO msg=\"Reset poza oknem swiecenia, swiatla WYL\"");
      }
      updateLEDs();
    }
    return;
  }
}

// =====================================
// 4.5️⃣ RAMPA PRZEJŚCIOWA (10s) - POPRAWIONA
// =====================================
if (transitionActive) {
  unsigned long nowMs = millis();
  bool allFinished = true;

  for (int i = 0; i < 5; i++) {
    uint16_t target = transitionTargetPwm[i];

    if (nowMs - transitionStepMillis[i] >= transitionIntervalMs[i]) {
      if (transitionCurrentPwm[i] < target) {
        transitionCurrentPwm[i]++;
      } else if (transitionCurrentPwm[i] > target) {
        transitionCurrentPwm[i]--;
      }
      transitionStepMillis[i] = nowMs;
      

    }

    setBrightnessForSection(backupBrightnessComposite, i, transitionCurrentPwm[i]);

    if (transitionCurrentPwm[i] != target) {
      allFinished = false;
    }
  }

  if (allFinished) {
    transitionActive = false;
    // ZAPIS DO AUTO tylko jeśli MANUAL->AUTO
    // [OK] FIX-BACKUPAUTO-ZERO: Nie zeruj backupAuto gdy cel=0 i jesteśmy w oknie świecenia.
    // Przypadek: cloud pushuje brightnessComposite=0 po reconnect -> transycja 294->0
    // -> backupAuto=[0,0,0,0,0] -> po soft-starcie LEDy nie wiedzą od jakiego PWM zaczynać.
    // Bezpiecznik: jeśli cel=0 i inLightingWindowGlobal=true -> nie nadpisuj backupAuto
    // (harmonogram i tak ma poprawne wartości; guard niżej nie pozwoli świecić na 0).
    if (tryb) {
      bool targetIsZero = (transitionTargetPwm[0] == 0);
      bool shouldProtect = targetIsZero && inLightingWindowGlobal;
      if (shouldProtect) {
        logPrintf("lvl=WARN tag=GUARD msg=\"Pominieto zapis backupAuto=0, okno swiecenia aktywne\" target=%d\n",
                  transitionTargetPwm[0]);
      } else {
        for (int i = 0; i < 5; i++) {
          setBrightnessForSection(backupAutoBrightnessComposite, i, transitionTargetPwm[i]);
        }
      }
    }
    if (Komentarze) logPrintf("lvl=INFO tag=PRZEJSCIE msg=\"Zakonczone\" pwm=%d\n", transitionTargetPwm[0]);
    updateLEDs();  // [OK] POPRAWKA: odśwież LED po zakończeniu transition
    return;        // [OK] POPRAWKA: wyjdź - nie pozwól sekcji 5 od razu nadpisać wartości (mrugnięcie)
  }
  
  // transition w trakcie - wyjdź, sekcja 5 nie powinna działać
  updateLEDs();
  return;
}  // koniec if (transitionActive)

// =====================================
// 5️⃣ NORMALNA PRACA + REGULACJA ADAPTACYJNA
// =====================================

// [OK] Sprawdź czy jesteśmy w nocy
int middayOffEnd = MIDDAY_OFF_LOCAL + fadeMinutes;  // [OK] FIX-v13: przerwa zaczyna się PO zakończeniu rampy

bool isNightExtended = isNight;
if (nowMin >= middayOffEnd && nowMin < eveningOnStart) {
  isNightExtended = true;
}

// [OK] FIX BUG-F: lastNightLog na scope funkcji (nie w bloku if) - reset gdy shouldBeOn
static bool lastNightLog = false;

if (isNightExtended) {
  // [OK] FIX-FLASH: Rampa adaptacyjna prowadzi backup do 0 samodzielnie -
  // nie zeruj tutaj, bo trybAuto() (co 100ms) walczyłoby z rampą (co 10ms) -> blyski
  if (rampaAdaptacyjnaAktywna) return;
  // [OK] FIX-PRZERWA v2: Przerwa południowa + minLux AKTYWNIE świeci -> nie zeruj backupu.
  if (!isNight && minLuxModeActive) {
    return;
  }
  // [OK] FIX-BREAK-FLASH v15: Proaktywna aktywacja MIN LUX przy wejściu w przerwę.
  // [OK] FIX-v16-SENSOR-FALLBACK: Usunięto guard (czujnikPokojowyAktywny || czujnikNadWodaAktywny).
  // Gdy czujnik odpada w trakcie przerwy, poprzedni kod wpadał w gałąź "zeruj normalnie"
  // -> LED gasły mimo włączonego MIN LUX. calculateMinLuxPWM() ma teraz własny fallback
  // do ostatniego znane PWM gdy czujnik jest niedostępny.
  if (!isNight && minLuxModeEnabled) {
    uint16_t mlPwm = calculateMinLuxPWM();
    if (mlPwm > 0) {
      for (int i = 0; i < 5; i++) {
        minLuxCurrentPWM[i] = mlPwm;
        setBrightnessForSection(backupBrightnessComposite, i, mlPwm);
      }
      minLuxModeActive = true;
      lastMinLuxUpdate = 0;  // Wymuś natychmiastowy update w applyMinLuxMode
      if (Komentarze) {
        logPrintf("lvl=INFO tag=FIX-BREAK-v15 msg=\"proaktywna aktywacja MIN LUX w przerwie\" pwm=%d\n", mlPwm);
      }
    }
    return;  // Nie zeruj - MIN LUX przejmuje sterowanie
  }
  // Noc lub MIN LUX wyłączony / brak czujnika - wyzeruj normalnie
  if (!lastNightLog && Komentarze) {
    logPrintf("%s lvl=INFO tag=AUTO msg=\"przerwa/noc, LED wylaczone\" minLuxActive=%s\n",
              logTime().c_str(), minLuxModeActive ? "TAK" : "NIE");
    lastNightLog = true;
  }
  for (int i = 0; i < 5; i++) {
    setBrightnessForSection(backupBrightnessComposite, i, 0);
  }
  return;
}
// Reset flagi logu gdy wychodzimy z przerwy nocnej
lastNightLog = false;

// ═══════════════════════════════════════════════════════
// REGULACJA ADAPTACYJNA
// ═══════════════════════════════════════════════════════
// [OK] FIX-B: ADAPT działa tylko w oknie świecenia (rano/wieczór)
// Poza oknem (przerwa połudn.) walczyłby z MIN LUX i nadpisywał backupAuto złą wartością
if (regulacjaAdaptacyjnaWlaczona && !rampaAdaptacyjnaAktywna && inLightingWindowGlobal) {
  
  uint16_t noweWartosci[5];
  bool znaczacaZmiana = false;

  // [OK] FIX-v23-ADAPT-HYST: Porównuj nowy cel vs OSTATNI ZATWIERDZONY cel adaptacji,
  // nie vs aktualny backup (który zmienia się w trakcie rampy i po jej zakończeniu).
  // Bez tego: ramp kończy się na np. 240, lux się zmienia, nowe=231, stare=240 -> delta=9>8
  // -> nowa rampa 240->231 natychmiast. Potem lux wraca, nowe=240, stare=231 -> delta=9>8
  // -> rampa 231->240. Pętla co ~5s w nieskończoność, spam TRYBAUT.
  // Z hysterezą: po zatwierdzeniu celu=240, kolejna rampa odpali się dopiero gdy
  // nowy cel różni się od 240 o więcej niż MIN_ZMIANA_PWM.
  static uint16_t _lastAdaptCommitted[5] = {0, 0, 0, 0, 0};

  for (int i = 0; i < 5; i++) {
    uint16_t harmonogram = getBrightnessForSection(backupAutoBrightnessComposite, i);
    uint16_t stare = getBrightnessForSection(backupBrightnessComposite, i);
    // Użyj ostatniego zatwierdzonego celu jako punktu odniesienia (0 = pierwszy raz -> fallback do stare)
    uint16_t ref = (_lastAdaptCommitted[i] > 0) ? _lastAdaptCommitted[i] : stare;
    
    noweWartosci[i] = obliczAdaptacyjnaJasnosc(harmonogram);
    
    if (abs((int)noweWartosci[i] - (int)ref) > MIN_ZMIANA_PWM) {
      // ── DEBUG: log przyczyny zmiany ──
      if (Komentarze) {
        static unsigned long _lastSekc5Log = 0;
        if (millis() - _lastSekc5Log > 5000) {  // nie spamuj - max co 5s
          logPrintf("lvl=INFO tag=SEKCJA5-DBG ch=%d harm=%d ref=%d nowe=%d delta=%d prog=%d\n",
                    i, harmonogram, ref, noweWartosci[i],
                    abs((int)noweWartosci[i] - (int)ref), MIN_ZMIANA_PWM);
          _lastSekc5Log = millis();
        }
      }
      znaczacaZmiana = true;
    }
  }
  
  if (znaczacaZmiana) {
    // [OK] FIX-v35-TRYBAUT: Sprawdź czy jest REALNA różnica vs aktualny backup.
    // znaczacaZmiana porównuje vs _lastAdaptCommitted (histereza), ale backup
    // mógł już dojść do celu przez wcześniejszą rampę -> roznica=0 dla wszystkich
    // kanałów -> zastosujRampeAdaptacyjna() ustawia rampaAdaptacyjnaAktywna=true,
    // aktualizujRampeAdaptacyjna() widzi wszystkieGotowe=true natychmiast ->
    // rampaAdaptacyjnaAktywna=false w tej samej iteracji -> 3ms później
    // 100ms timer strzela ponownie -> podwójny trybAuto -> TRYBAUT alarm.
    bool realDiff = false;
    for (int i = 0; i < 5; i++) {
      uint16_t aktualne = getBrightnessForSection(backupBrightnessComposite, i);
      if (aktualne != noweWartosci[i]) { realDiff = true; break; }
    }
    if (realDiff) {
      // Zatwierdź nowe cele jako punkt odniesienia dla kolejnych sprawdzeń
      for (int i = 0; i < 5; i++) _lastAdaptCommitted[i] = noweWartosci[i];
      // [DIAG-v72] Log przed rampą - wyjaśnia skąd pochodzi zmiana PWM
      if (Komentarze) {
        uint16_t _curr0 = getBrightnessForSection(backupBrightnessComposite, 0);
        uint16_t _harm0 = getBrightnessForSection(backupAutoBrightnessComposite, 0);
        logPrintf("lvl=INFO tag=RAMP-ADAPT-START curr=%d nowe=%d harm=%d czujnik=%s adapt=%s czas_s=30\n",
          _curr0, noweWartosci[0], _harm0,
          (czujnikPokojowyAktywny || czujnikNadWodaAktywny) ? "aktywny" : "BRAK",
          regulacjaAdaptacyjnaWlaczona ? "ON" : "OFF");
      }
      zastosujRampeAdaptacyjna(noweWartosci, "trybAuto-sekcja5");
    } else {
      // Brak realnej różnicy - tylko zaktualizuj committed żeby histereza
      // nie powodowała kolejnego false-positive za 100ms
      for (int i = 0; i < 5; i++) _lastAdaptCommitted[i] = noweWartosci[i];
    }
  }
  
} else if (!regulacjaAdaptacyjnaWlaczona) {
  // Regulacja wyłączona - kopiuj z uwzględnieniem limitu mocy zasilacza
  // [FIX-SECT5 v73] Bez tego kapu backupAuto=1023 nadpisywało backup po każdym soft-starcie,
  // wymuszając permanentną pracę balansera przez ogranicznik (gPowerScale=0.758 stale aktywny).
  uint16_t cappedTargets[5];
  for (int i = 0; i < 5; i++) {
    cappedTargets[i] = getBrightnessForSection(backupAutoBrightnessComposite, i);
  }
  float capScale = capTargetsToPowerLimit(cappedTargets);
  for (int i = 0; i < 5; i++) {
    setBrightnessForSection(backupBrightnessComposite, i, cappedTargets[i]);
  }
  // Loguj jednorazowo gdy kap zadziałał (powodował bug)
  static bool _sect5CapLogged = false;
  if (capScale < 0.999f && !_sect5CapLogged) {
    _sect5CapLogged = true;
    logPrintf("lvl=WARN tag=SECT5-FIX adapt=OFF skala=%.3f backupAuto0=%d nowe0=%d\n",
      capScale,
      getBrightnessForSection(backupAutoBrightnessComposite, 0),
      cappedTargets[0]);
  }
}

// [DIAG-v72] LOG SEKCJA5: pełny stan decyzji PWM w trybie AUTO.
// Cel: wyjaśnić skąd pochodzi rozbieżność harm vs curr (275 vs 314).
// [v231] FIX-LOG-SCHED5-ONCHANGE: poprzednio logowane bezwarunkowo co 60s —
// w praktyce (potwierdzone w wielu logach produkcyjnych) diff=0 i cała
// reszta stanu bez zmiany niemal zawsze, więc log nie niósł informacji.
// Teraz: log tylko gdy COKOLWIEK z obserwowanego stanu się zmieni (nie
// tylko diff — adapt/rampAdapt/trans/sSt/rampSch/inWin to kontekst
// PROWADZĄCY do rozbieżności, nie mniej ważny niż sam jej moment) —
// plus zapasowy heartbeat co 1h, żeby potwierdzić że monitoring wciąż żyje
// nawet gdy stan idealnie stabilny.
{
  static unsigned long _lastSched5DiagMs = 0;
  static bool _sched5HasPrev = false;
  static uint16_t _prevHarmPWM = 0, _prevCurrPWM = 0;
  static bool _prevAdapt = false, _prevRampAdapt = false, _prevTrans = false,
              _prevSSt = false, _prevRampSch = false, _prevInWin = false;
  static float _prevScale = -1.0f;
  if (Komentarze && millis() - _lastSched5DiagMs >= 1000UL) {  // sprawdzaj stan raz/s, loguj wg warunku niżej
    uint16_t harmPWM = getBrightnessForSection(backupAutoBrightnessComposite, 0);
    uint16_t currPWM = getBrightnessForSection(backupBrightnessComposite, 0);
    bool adapt    = regulacjaAdaptacyjnaWlaczona;
    bool rampAdapt= rampaAdaptacyjnaAktywna;
    bool trans    = transitionActive;
    bool sSt      = softStartActive;
    bool rampSch  = rampScheduleActive;
    bool inWin    = inLightingWindowGlobal;
    float scale   = gPowerScale;
    bool changed = !_sched5HasPrev ||
      harmPWM != _prevHarmPWM || currPWM != _prevCurrPWM ||
      adapt != _prevAdapt || rampAdapt != _prevRampAdapt || trans != _prevTrans ||
      sSt != _prevSSt || rampSch != _prevRampSch || inWin != _prevInWin ||
      fabsf(scale - _prevScale) > 0.0005f;
    bool heartbeatDue = millis() - _lastSched5DiagMs >= 3600000UL;  // 1h zapasowy heartbeat
    if (changed || heartbeatDue) {
      _lastSched5DiagMs = millis();
      _sched5HasPrev = true;
      _prevHarmPWM = harmPWM; _prevCurrPWM = currPWM;
      _prevAdapt = adapt; _prevRampAdapt = rampAdapt; _prevTrans = trans;
      _prevSSt = sSt; _prevRampSch = rampSch; _prevInWin = inWin; _prevScale = scale;
      logPrintf("lvl=INFO tag=SCHED5 harm=%d curr=%d diff=%d adapt=%s rampAdapt=%s trans=%s sSt=%s rampSch=%s scale=%.3f inWin=%s hb=%s\n",
        harmPWM, currPWM, (int)harmPWM - (int)currPWM,
        adapt ? "ON" : "OFF", rampAdapt ? "T" : "F", trans ? "T" : "F",
        sSt ? "T" : "F", rampSch ? "T" : "F", scale, inWin ? "T" : "F",
        (!changed && heartbeatDue) ? "T" : "F");
    }
  }
}

} 





//=======================================================================================================


//=======================================================================================================

void readTemperatures() {
  // ═══════════════════════════════════════════════════════════════════
  // [OK] FIX-v33i: DS18B20 2-FAZOWY ODCZYT - zero delay() w loop()
  //
  // Protokół DS18B20 składa się z dwóch faz rozdzielonych w czasie:
  //   Faza 1 - CONVERT_T: wyślij rozkaz konwersji do sensora (~1-2ms per bus).
  //            Sensor wykonuje konwersję SPRZĘTOWO przez 94ms (9-bit).
  //            ESP32 nie czeka - wraca natychmiast z setWaitForConversion(false).
  //   Faza 2 - READ: odczytaj gotowy wynik po >=100ms od rozkazu (~1ms per bus).
  //
  // readTemperatures() wywołuje się co 15s. Każde wywołanie obsługuje
  // dokładnie jedną fazę i wraca natychmiast. Sensor pracuje sprzętowo
  // między wywołaniami - loop() nie blokuje ani przez chwilę.
  //
  // HISTORIA POPRZEDNICH FIXÓW:
  //   v9:  setWaitForConversion(false) + delay(750) - usunął blokadę konwersji,
  //        dodał ręczny delay(750ms) -> nadal blokowało 750ms.
  //   v26: 9-bit (94ms konwersja) + delay(120ms) - zredukował do ~120ms delay
  //        + czas requestTemperatures() (~100-600ms) -> nadal 600-750ms blokady.
  //   v33i: 2 fazy rozdzielone w czasie - 0ms blokady.
  // ═══════════════════════════════════════════════════════════════════

  unsigned long _now = millis();

  // ── FAZA 1: Wyślij rozkaz konwersji (co 15s, razem z wywołaniem z loop()) ──
  if (!tempConvRequested) {
    // Rozdzielczość 9-bit = 94ms konwersja, ±0.5°C - wystarczające do deratingu.
    // Ustawiane za każdym razem bo sensors.begin() w setup() resetuje rozdzielczość.
    sensors1.setResolution(9);
    sensors2.setResolution(9);
    sensors3.setResolution(9);
    sensors1.setWaitForConversion(false);
    sensors2.setWaitForConversion(false);
    sensors3.setWaitForConversion(false);
    // requestTemperatures() z WaitForConversion=false wraca po wysłaniu rozkazu (~1-2ms).
    // Nie czeka na presence pulse ani na wynik konwersji.
    // [FIX-v132-DS18B20-LOOPSLOW] esp_task_wdt_reset() przed każdym request (bus stall ochrona).
    // Przy problemach z bus (kolizja ROM, zły pull-up, EMI od PWM) OneWire::reset()
    // może blokować do ~1s. S3 log: LOOP-SLOW 916ms z tempCheck. Każde esp_task_wdt_reset()
    // daje 15s świeżego okna — bus stall nie zabije WDT. Log stall >200ms do diagnozy.
    {
      unsigned long _ts;
      esp_task_wdt_reset();
      _ts = millis(); sensors1.requestTemperatures();
      if (millis() - _ts > 200)
        logPrintf("lvl=WARN tag=DS-STALL bus=1 msg=\"Stall (pull-up? EMI? kolizja ROM?)\" czas=%lums\n", millis()-_ts);

      esp_task_wdt_reset();
      _ts = millis(); sensors2.requestTemperatures();
      if (millis() - _ts > 200)
        logPrintf("lvl=WARN tag=DS-STALL bus=2 msg=\"Stall\" czas=%lums\n", millis()-_ts);

      esp_task_wdt_reset();
      _ts = millis(); sensors3.requestTemperatures();
      if (millis() - _ts > 200)
        logPrintf("lvl=WARN tag=DS-STALL bus=3 msg=\"Stall\" czas=%lums\n", millis()-_ts);
    }
    tempConvRequested   = true;
    tempConvRequestedAt = _now;
    return;  // wróć natychmiast - sensor pracuje sprzętowo
  }

  // ── FAZA 2: Odczytaj wynik gdy konwersja zakończona (>=100ms od Fazy 1) ──
  if (_now - tempConvRequestedAt < 100UL) {
    return;  // jeszcze za wcześnie - nie blokuj loop()
  }
  tempConvRequested = false;  // reset flagi - następne wywołanie wyśle nowy request

  // Odczyt gotowego wyniku - 1-Wire zajęty tylko przez czas transmisji (~1ms per sensor)
  float t1 = sensors1.getTempCByIndex(0);
  float t2 = sensors2.getTempCByIndex(0);
  float t3 = sensors3.getTempCByIndex(0);

  // [OK] POPRAWKA: Walidacja odczytów DS18B20
  // -127.0 = czujnik odłączony, 85.0 = błąd zasilania (power-on reset)
  // Przy błędnym odczycie zachowaj ostatnią prawidłową wartość

  // ═══════════════════════════════════════════════════════════════════
  // [OK] [v9 FIX-DS18B20-LOGSPAM] — eskalacja throttle logu błędów
  //
  // PROBLEM (log_1.txt, 03:53-05:23):
  //   Przy trwale odłączonych DS18B20 logi błędów zapełniały terminal
  //   wpisem co 5 minut przez całą noc (×3 sensory = 3 linie co 5 min).
  //
  // ZMIANA:
  //   Po 6 kolejnych błędach sensor jest uznany za trwale odłączony
  //   -> throttle eskaluje z 5 min do 30 min.
  //   Licznik resetuje się przy pierwszym poprawnym odczycie.
  // ═══════════════════════════════════════════════════════════════════
  static unsigned long _lastErrLog1 = 0, _lastErrLog2 = 0, _lastErrLog3 = 0;
  static uint8_t _errCount1 = 0, _errCount2 = 0, _errCount3 = 0;
  const unsigned long ERR_THROTTLE_SHORT = 300000UL;  // 5 min
  const unsigned long ERR_THROTTLE_LONG  = 1800000UL; // 30 min (po 6+ błędach)
  // _now zadeklarowane wcześniej w tej funkcji (Faza 1/2)

  if (t1 != -127.0f && t1 != 85.0f && t1 >= -10.0f && t1 <= 90.0f) {
    tempPlate1 = t1;
    _lastErrLog1 = 0; _errCount1 = 0; // reset - przy kolejnym błędzie od razu zaloguj
  } else {
    _errCount1++;
    unsigned long throttle = (_errCount1 > 6) ? ERR_THROTTLE_LONG : ERR_THROTTLE_SHORT;
    if (Komentarze && _now - _lastErrLog1 > throttle) {
      logPrintf("lvl=WARN tag=DS18B20 czujnik=plyta1 odczyt=%.1fC zachowano=%.1fC throttle=%s\n",
                t1, tempPlate1, _errCount1 > 6 ? "30min" : "5min");
      _lastErrLog1 = _now;
    }
  }
  if (t2 != -127.0f && t2 != 85.0f && t2 >= -10.0f && t2 <= 90.0f) {
    tempPlate2 = t2;
    _lastErrLog2 = 0; _errCount2 = 0;
  } else {
    _errCount2++;
    unsigned long throttle = (_errCount2 > 6) ? ERR_THROTTLE_LONG : ERR_THROTTLE_SHORT;
    if (Komentarze && _now - _lastErrLog2 > throttle) {
      logPrintf("lvl=WARN tag=DS18B20 czujnik=plyta2 odczyt=%.1fC zachowano=%.1fC throttle=%s\n",
                t2, tempPlate2, _errCount2 > 6 ? "30min" : "5min");
      _lastErrLog2 = _now;
    }
  }
  if (t3 != -127.0f && t3 != 85.0f && t3 >= -10.0f && t3 <= 90.0f) {
    tempWater = t3;
    _lastErrLog3 = 0; _errCount3 = 0;
  } else {
    _errCount3++;
    unsigned long throttle = (_errCount3 > 6) ? ERR_THROTTLE_LONG : ERR_THROTTLE_SHORT;
    if (Komentarze && _now - _lastErrLog3 > throttle) {
      logPrintf("lvl=WARN tag=DS18B20 czujnik=woda odczyt=%.1fC zachowano=%.1fC throttle=%s\n",
                t3, tempWater, _errCount3 > 6 ? "30min" : "5min");
      _lastErrLog3 = _now;
    }
    // [v50] FIX-DS18B20: po 10 błędach z rzędu zaloguj wyraźne ostrzeżenie o sprzęcie
    // (analiza logów: czujnik wody miał 19 błędów w ciągu nocy - prawdopodobnie zły pull-up)
    if (_errCount3 == 10) {
      logPrintf("lvl=WARN tag=DS18B20 czujnik=woda msg=\"10 bledow z rzedu, sprawdz kabel/pullup 4.7kOhm\"\n");
    }
  }
  
  // ═══════════════════════════════════════════════
  // LOGI TEMP - raz po 5 min stabilności, potem cicho aż do zmiany
  // ═══════════════════════════════════════════════
  static int lastTempFullDegree1 = -999;
  static int lastTempFullDegree2 = -999;
  static int lastTempFullDegreeW = -999;
  static unsigned long tempStableStartTime = 0;
  static bool tempStableLogged = false;
  const unsigned long TEMP_STABLE_DURATION = 5UL * 60UL * 1000UL; // 5 minut

  // Pobierz pełne stopnie (floor)
  int currentFullDegree1 = (int)tempPlate1;
  int currentFullDegree2 = (int)tempPlate2;
  int currentFullDegreeW = (int)tempWater;

  bool allInSameRange = (currentFullDegree1 == lastTempFullDegree1) &&
                        (currentFullDegree2 == lastTempFullDegree2) &&
                        (currentFullDegreeW == lastTempFullDegreeW);

  if (!allInSameRange) {
    // Zmiana temperatury - reset timera i flagi
    lastTempFullDegree1 = currentFullDegree1;
    lastTempFullDegree2 = currentFullDegree2;
    lastTempFullDegreeW = currentFullDegreeW;
    tempStableStartTime = millis();
    tempStableLogged = false;
  } else if (!tempStableLogged) {
    // Stabilna - zapisz RAZ po 5 minutach
    if (millis() - tempStableStartTime >= TEMP_STABLE_DURATION) {
      if (Komentarze) {
        logPrintf("lvl=INFO tag=TEMP plyta1=%.1f plyta2=%.1f woda=%.1f\n",
                  tempPlate1, tempPlate2, tempWater);
      }
      tempStableLogged = true;  // już zapisano, cisza do następnej zmiany
    }
  }
}




//=======================================================================================================

float calculateLinearDerating(float currentTemp) {
  // [OK] FIX BUG-I: Usunięto martwą gałąź TEMP_FULL_POWER (40°C) - obie zwracały 1.0.
  // Redukcja zaczyna się od TEMP_START_DERATING (45°C).

  // Pełna moc poniżej progu deratingu
  if (currentTemp <= TEMP_START_DERATING) {
    return 1.0;
  }
  
  // Wyłączenie powyżej maksymalnej temperatury
  if (currentTemp >= TEMP_ZERO_POWER) {
    return 0.0;
  }
  
  // Liniowa redukcja między progami
  float tempRange = TEMP_ZERO_POWER - TEMP_START_DERATING;
  float tempDelta = currentTemp - TEMP_START_DERATING;
  float derating = 1.0 - (tempDelta / tempRange);
  
  // Ograniczenie do zakresu 0.0 - 1.0
  return max(0.0f, min(1.0f, derating));
}

//=======================================================================================================

void multiZoneLinearDerating() {
  if (millis() - lastTempCheck < tempInterval) return;

  // === STANY ===
  static bool derating1Active = false;
  static bool derating2Active = false;
  static uint64_t originalBrightness1 = 0;
  static uint64_t originalBrightness2 = 0;
  static float lastDerating1 = 1.0f;
  static float lastDerating2 = 1.0f;
  
  // TRACKING dla debugowania
  static uint16_t lastDebugValue1 = 0;
  static uint16_t lastDebugValue2[4] = {0, 0, 0, 0};
  const uint16_t DEBUG_THRESHOLD = 10; // Pokaż zmianę gdy różnica ≥10 (1%)

  bool needsUpdate = false;

  // =====================================================
  // === ZONE 1 — FS BIAŁE (sekcja 2)
  // =====================================================
  if (!derating1Active && tempPlate1 > TEMP_START_DERATING) {
    derating1Active = true;
    derating1ActiveGlobal = true;  // [OK] FIX BUG-T: Eksponuj status dla trybManual()
    // [OK] POPRAWKA: Snapshot z backupAUTO (harmonogram), nie z backup (może być już zderedowany z poprzedniego cyklu)
    if (originalBrightness1 == 0)
      originalBrightness1 = backupAutoBrightnessComposite;

    if (Komentarze) {
      logPrintf("lvl=WARN tag=STREFA1 faza=START temp=%.1f\n", tempPlate1);
    }
  }
  // [OK] FIX BUG-Y: Odśwież snapshot jeśli harmonogram zmienił się podczas aktywnego deratingu.
  // Poprzednio: jeśli user zmieniał jasność AUTO przy aktywnym deratingu, snapshot był stary.
  // Po ochłodzeniu derating przywracał STARE wartości - nowe ustawienie było tracone.
  else if (derating1Active && originalBrightness1 != 0 &&
           getBrightnessForSection(backupAutoBrightnessComposite, 2) != getBrightnessForSection(originalBrightness1, 2)) {
    originalBrightness1 = backupAutoBrightnessComposite;  // zaktualizuj snapshot
    if (Komentarze) logPrintln("lvl=INFO tag=STREFA1 msg=\"Snapshot harmonogramu zaktualizowany podczas deratingu\"");
  }
  else if (derating1Active && tempPlate1 < (TEMP_START_DERATING - HYSTERESIS)) {
    derating1Active = false;
    derating1ActiveGlobal = false;  // [OK] FIX BUG-T
    uint16_t orig = getBrightnessForSection(originalBrightness1, 2);
    setBrightnessForSection(backupBrightnessComposite, 2, orig);
    originalBrightness1 = 0;
    lastDerating1 = 1.0f;
    lastDebugValue1 = 0;
    needsUpdate = true;

    if (Komentarze) {
      logPrintf("lvl=WARN tag=STREFA1 faza=KONIEC temp=%.1f\n", tempPlate1);
    }
  }

  // =====================================================
  // === ZONE 2 — MULTI (0,1,3,4)
  // =====================================================
  int secs[] = {0, 1, 3, 4};

  if (!derating2Active && tempPlate2 > TEMP_START_DERATING) {
    derating2Active = true;
    derating2ActiveGlobal = true;  // [OK] FIX BUG-T
    // [OK] POPRAWKA: Snapshot z backupAUTO (harmonogram), nie z backup (może być już zderedowany)
    if (originalBrightness2 == 0)
      originalBrightness2 = backupAutoBrightnessComposite;

    if (Komentarze) {
      logPrintf("lvl=WARN tag=STREFA2 faza=START temp=%.1f\n", tempPlate2);
    }
  }
  // [OK] FIX BUG-Y: Odśwież snapshot zone2 gdy harmonogram zmienił się podczas deratingu
  else if (derating2Active && originalBrightness2 != 0 &&
           backupAutoBrightnessComposite != originalBrightness2) {
    originalBrightness2 = backupAutoBrightnessComposite;
    if (Komentarze) logPrintln("lvl=INFO tag=STREFA2 msg=\"Snapshot harmonogramu zaktualizowany podczas deratingu\"");
  }
  else if (derating2Active && tempPlate2 < (TEMP_START_DERATING - HYSTERESIS)) {
    derating2Active = false;
    derating2ActiveGlobal = false;  // [OK] FIX BUG-T
    for (int i = 0; i < 4; i++) {
      uint16_t orig = getBrightnessForSection(originalBrightness2, secs[i]);
      setBrightnessForSection(backupBrightnessComposite, secs[i], orig);
      lastDebugValue2[i] = 0;
    }
    originalBrightness2 = 0;
    lastDerating2 = 1.0f;
    needsUpdate = true;

    if (Komentarze) {
      logPrintf("lvl=WARN tag=STREFA2 faza=KONIEC temp=%.1f\n", tempPlate2);
    }
  }

  // =====================================================
  // === APLIKACJA DERATINGU — ZONE 1
  // =====================================================
  if (derating1Active) {
    float d = calculateLinearDerating(tempPlate1);
    float fd = FILTER_ALPHA * lastDerating1 + (1.0f - FILTER_ALPHA) * d;
    lastDerating1 = fd;

    uint16_t orig = getBrightnessForSection(originalBrightness1, 2);
    uint16_t newVal = (uint16_t)(orig * fd);
    setBrightnessForSection(backupBrightnessComposite, 2, newVal);
    needsUpdate = true;

    // DEBUG - tylko przy znaczącej zmianie
    if (abs((int)newVal - (int)lastDebugValue1) >= DEBUG_THRESHOLD && Komentarze) {
      logPrintf("lvl=INFO tag=STREFA1 faza=DEBUG orig=%d orig_proc=%.1f teraz=%d teraz_proc=%.1f ciecie_proc=%.1f\n",
                    orig, orig * 100.0f / 1023.0f,
                    newVal, newVal * 100.0f / 1023.0f,
                    (1.0f - fd) * 100.0f);
      lastDebugValue1 = newVal;
    }
  }

  // =====================================================
  // === APLIKACJA DERATINGU — ZONE 2
  // =====================================================
  if (derating2Active) {
    float d = calculateLinearDerating(tempPlate2);
    float fd = FILTER_ALPHA * lastDerating2 + (1.0f - FILTER_ALPHA) * d;
    lastDerating2 = fd;

    for (int i = 0; i < 4; i++) {
      uint16_t orig = getBrightnessForSection(originalBrightness2, secs[i]);
      uint16_t newVal = (uint16_t)(orig * fd);
      setBrightnessForSection(backupBrightnessComposite, secs[i], newVal);

      // DEBUG - tylko przy znaczącej zmianie
      if (abs((int)newVal - (int)lastDebugValue2[i]) >= DEBUG_THRESHOLD && Komentarze) {
        logPrintf("lvl=INFO tag=STREFA2 faza=DEBUG sek=%d orig=%d orig_proc=%.1f teraz=%d teraz_proc=%.1f\n",
                      secs[i], orig, orig * 100.0f / 1023.0f,
                      newVal, newVal * 100.0f / 1023.0f);
        lastDebugValue2[i] = newVal;
      }
    }
    needsUpdate = true;
  }

  if (needsUpdate) updateLEDs();
  lastTempCheck = millis();
}






// === DODAJ ZABEZPIECZENIE AWARYJNE ===
void emergencyThermalShutdown() {
  const float EMERGENCY_TEMP = 85.0;
  static bool emergencyActive = false;
  
  float maxTemp = max(tempPlate1, tempPlate2);
  
  if(!emergencyActive && maxTemp > EMERGENCY_TEMP) {
    emergencyActive = true;
    power = false;
    savePowerTrybToEEPROM();  // Zapisz power=false do EEPROM - po restarcie LED nie wlacza sie automatycznie
    // BUG#3 FIX: wymus zapis logow przed shutdown
    if (littlefsReady) {
      File _ef = LittleFS.open(LOG_FILE_B, "a");  // [v102]
      if (_ef) {
        _ef.print("[EMERGENCY] Wymuszone zapisanie logow przed shutdownem\n");
        _ef.close();
      }
    }
    
    for(int i = 0; i < 5; i++) {
      setBrightnessForSection(backupBrightnessComposite, i, 0);
    }
    updateLEDs();

    // [v33] cloud usunięty - sync z chmurą nie jest potrzebny

    // [OK] FIX D: Zresetuj flagi ramp - po przywróceniu zasilania (power=true) system
    // wykona ponowny soft-start zamiast skokowego włączenia na pełną jasność.
    autoRampInitialized = false;
    softStartActive     = false;
    rampScheduleActive  = false;
    for (int i = 0; i < 5; i++) rampInitialized[i] = false;
    
    if(Komentarze) {
      logPrintf("lvl=WARN tag=AWARIA msg=\"Wylaczenie awaryjne, zasilanie zapisane do EEPROM\" temp=%.1f\n", maxTemp);
    }
  } 
  else if(emergencyActive && maxTemp < (EMERGENCY_TEMP - 10.0)) {
    emergencyActive = false;
    
    if(Komentarze) {
      logPrintf("lvl=INFO tag=AWARIA msg=\"System gotowy do pracy\" temp=%.1f\n", maxTemp);
    }
  }
}




int obliczZachodSlonca(int rok, int miesiac, int dzien, float szerokosc, float dlugosc) {
  int N = dzien
        + (153 * (miesiac + 12 * ((14 - miesiac) / 12) - 3) + 2) / 5
        + 365 * (rok + 4800 - ((14 - miesiac) / 12))
        + (rok + 4800 - ((14 - miesiac) / 12)) / 4
        - (rok + 4800 - ((14 - miesiac) / 12)) / 100
        + (rok + 4800 - ((14 - miesiac) / 12)) / 400
        - 32045;
  N -= 2451545;

  float lngHour = dlugosc / 15.0;
  float t = N + ((18 - lngHour) / 24.0);
  float M = (0.9856 * t) - 3.289;
  float L = fmod(M
               + (1.916 * sin(radians(M)))
               + (0.020 * sin(radians(2 * M)))
               + 282.634,
               360);
  float RA = fmod(degrees(atan(0.91764 * tan(radians(L)))), 360);
  float Lquadrant = floor(L / 90) * 90;
  float RAquadrant = floor(RA / 90) * 90;
  RA = RA + (Lquadrant - RAquadrant);
  RA /= 15.0;

  float sinDec = 0.39782 * sin(radians(L));
  float cosDec = cos(asin(sinDec));

  float cosHraw = (
    cos(radians(90.833))
    - sinDec * sin(radians(szerokosc))
  ) / (cosDec * cos(radians(szerokosc)));

  if (cosHraw < -1.0 || cosHraw > 1.0) return -1; // Słońce nie zachodzi lub nie wschodzi tego dnia

  float H = degrees(acos(cosHraw)) / 15.0;
  float T = H + RA - (0.06571 * t) - 6.622;

  float UT = fmod((T - lngHour), 24.0);
  if (UT < 0.0) UT += 24.0;

  return int(UT * 60.0);
}



uint16_t gammaCorrect(uint16_t value10bit, float gamma) {
  float normalized = (float)value10bit / 1023.0f;
  float corrected = powf(normalized, gamma);
  return (uint16_t)(corrected * 1023.0f + 0.5f);
}




void trybManual() {
  static bool firstCall = true;
  static uint64_t lastManualBrightness = 0;
  
  if (manualSoftStartActive) {
    unsigned long nowMs = millis();
    bool allFinished = true;
    
    for (int i = 0; i < 5; i++) {
      uint16_t target = getBrightnessForSection(backupManualBrightnessComposite, i);
      unsigned long elapsedMs = nowMs - manualSoftStartStepMillis[i];
      unsigned long totalSoftStartMs = (unsigned long)MANUALSOFTSTARTSECONDS * 1000UL;
      float progress = (float)elapsedMs / (float)totalSoftStartMs;
      if (progress > 1.0f) progress = 1.0f;
      uint16_t expectedPwm = (uint16_t)(target * progress);
      manualSoftStartPwm[i] = expectedPwm;
      setBrightnessForSection(backupBrightnessComposite, i, manualSoftStartPwm[i]);
      if (progress < 1.0f) allFinished = false;
    }
    
    if (allFinished) {
      manualSoftStartActive = false;
      if (Komentarze) logPrintln("lvl=INFO tag=MANUAL msg=\"miekki start zakonczony\"");
    }
    updateLEDs();
    return;
  }

  if (manualTransitionActive) {
    unsigned long nowMs = millis();
    bool allFinished = true;
    
    for (int i = 0; i < 5; i++) {
      uint16_t start  = manualTransitionCurrentPwm[i];
      uint16_t target = manualTransitionTargetPwm[i];
      unsigned long elapsedMs = nowMs - manualTransitionStepMillis[i];
      unsigned long totalTransitionMs = (unsigned long)MANUALTRANSITIONSECONDS * 1000UL;
      float progress = (float)elapsedMs / (float)totalTransitionMs;
      if (progress > 1.0f) progress = 1.0f;

      uint16_t expectedPwm;
      if (target > start) {
        expectedPwm = start + (uint16_t)((target - start) * progress);
      } else {
        expectedPwm = start - (uint16_t)((start - target) * progress);
      }

      setBrightnessForSection(backupBrightnessComposite, i, expectedPwm);
      if (progress < 1.0f) allFinished = false;
    }

    if (allFinished) {
      manualTransitionActive = false;
      for (int i = 0; i < 5; i++) {
        setBrightnessForSection(backupManualBrightnessComposite, i, manualTransitionTargetPwm[i]);
      }
      if (Komentarze) logPrintln("lvl=INFO tag=PRZEJSCIE tryb=MANUAL faza=KONIEC");
    }
    updateLEDs();
    return;
  }

  bool hasChanged = (backupManualBrightnessComposite != lastManualBrightness) || firstCall;

  // [OK] FIX BUG-T: Kopiuj kanał-po-kanale zamiast bezpośredniego przypisania całego composite.
  // Poprzednio: backupBrightnessComposite = backupManualBrightnessComposite (pełna jasność zawsze).
  // multiZoneLinearDerating() redukowała brightness PO trybManual(), ale PRZY NASTĘPNYM
  // obrocie pętli trybManual() przywracał pełną jasność -> derating nie działał w trybie MANUAL.
  // Poprawka: kopiuj tylko kanały NIEZMIENIONE przez aktywny derating. Sprawdzamy czy derating
  // jest aktywny przez globalne flagi (externe z multiZoneLinearDerating). Derating zone1=sec2,
  // zone2=sec{0,1,3,4}. Kanały zarządzane przez derating zostawiamy bez zmian - derating
  // sam wyliczy właściwą wartość w tym samym obrocie pętli.
  for (int i = 0; i < 5; i++) {
    // Derating zone1 kontroluje sekcję 2, zone2 kontroluje sekcje 0,1,3,4
    bool derating1Controls = (i == 2) && derating1ActiveGlobal;
    bool derating2Controls = (i == 0 || i == 1 || i == 3 || i == 4) && derating2ActiveGlobal;
    if (!derating1Controls && !derating2Controls) {
      uint16_t manualVal = getBrightnessForSection(backupManualBrightnessComposite, i);
      setBrightnessForSection(backupBrightnessComposite, i, manualVal);
    }
  }
  updateLEDs();
  if (hasChanged && Komentarze) {
    logPrintln("lvl=INFO tag=MANUAL-SYNC msg=\"zachowywanie indywidualnych wartosci jasnosci\"");
  }
  lastManualBrightness = backupManualBrightnessComposite;
  firstCall = false;
}


void wlaczKamera() {
  // Tutaj umieść kod do włączenia kamery, np. ustawienie odpowiedniego pinu na HIGH
  digitalWrite(cameraPin, HIGH);
  // [OK] FIX LOGIC-E: Zapisz czas włączenia - automatyczny wyłącznik po 30 min
  cameraOnTime = millis();
  if (Komentarze) {
    logPrintln("lvl=INFO tag=KAMERA akcja=start msg=\"auto-wylaczenie po 30min\"");
  }
}

void wylaczKamera() {
  digitalWrite(cameraPin, LOW);
  cameraOnTime = 0;  // [OK] FIX LOGIC-E: Skasuj timer
  if (Komentarze) {
    logPrintln("lvl=INFO tag=KAMERA akcja=stop");
  }
}

// [v33] onChangeCounterChange() usunięta - Arduino IoT Cloud usunięty

void onPowerChange() {
  // [v257] callback runtime korzysta wyłącznie z RAM; storage owner działa na Core 0.
  logPrintf("lvl=INFO tag=WYWOLANIE zdarzenie=zasilanie power=%s czas=%lus\n",
    power ? "ON" : "OFF", millis() / 1000);

  // [v33] Guard cloudInitialSyncDone usunięty - cloud usunięty

  logPrintf("lvl=INFO tag=ZAPIS cel=EEPROM zasilanie=%s\n", power ? "WL" : "WYL");
  savePowerTrybToEEPROM();
  bumpChangeCounter();  // ← oznacz że to realna zmiana (nie echo Cloud po restarcie)
  // BUG#7 FIX: uruchom miekki start gdy wlaczamy zasilanie w trybie MANUAL
  if (power && !tryb) {
    for (int i = 0; i < 5; i++) {
      manualSoftStartPwm[i] = 0;
      manualSoftStartStepMillis[i] = millis();
    }
    if (!rampArbiterTryStart(RAMP_MANUAL_SOFTSTART, true)) return;
    manualSoftStartActive = true;
    gPowerScale = 1.0f; gPowerScaleLastLogged = 1.0f;  // balancer nie walczy z soft-startem
    if (Komentarze) logPrintln("lvl=INFO tag=MANUAL msg=\"miekki start po wlaczeniu zasilania\"");
  }
  updateLEDs();
}


// ╔══════════════════════════════════════════════════════════════════╗
// ║              🔬 MODUŁ DIAGNOSTYCZNY - runDiagnostics()          ║
// ║  Wywołanie: koniec każdej iteracji loop()                        ║
// ║  Zasada: każdy strażnik ma własny throttle, max 1 log/minutę    ║
// ║  Flagi wewnętrzne są statyczne - nie zajmują RAM poza funkcją   ║
// ╚══════════════════════════════════════════════════════════════════╝
void runDiagnostics() {

  // ── Wspólny timer - nie wchodź częściej niż co 200ms ──
  static unsigned long _lastDiagRun = 0;
  unsigned long _now = millis();
  if (_now - _lastDiagRun < 200) return;
  _lastDiagRun = _now;

  // ─────────────────────────────────────────────────────────────────
  // DIAG-1: Kolizja ramp - wiele systemów rampujących jednocześnie
  // Normalny stan: TYLKO JEDNA z poniższych flag może być true naraz.
  // v252: kolizję rozwiązuje rampArbiterReconcile(), a DIAG-1 pozostaje
  // licznikiem kontrolnym na wypadek regresji.
  // ─────────────────────────────────────────────────────────────────
  {
    static unsigned long _lastLog = 0;
    int activeRamps = (int)softStartActive
                    + (int)transitionActive
                    + (int)manualSoftStartActive
                    + (int)manualTransitionActive
                    + (int)rampaAdaptacyjnaAktywna
                    + (int)rampScheduleActive;

    // Kolizja: więcej niż 1 rampa aktywna
    if (activeRamps > 1 && _now - _lastLog > 60000) {
      _lastLog = _now;
      logPrintf(
        "lvl=WARN tag=DIAG-1 msg=\"KOLIZJA RAMP\" Aktywnych: %d | owner=%s blocked=%lu preempted=%lu | ss=%s tr=%s mss=%s mtr=%s rAdpt=%s rSch=%s\n",
        activeRamps,
        rampOwnerName(gRampOwner),
        (unsigned long)gRampArbiterBlocked,
        (unsigned long)gRampArbiterPreempted,
        softStartActive ? "T" : "F",
        transitionActive ? "T" : "F",
        manualSoftStartActive ? "T" : "F",
        manualTransitionActive ? "T" : "F",
        rampaAdaptacyjnaAktywna ? "T" : "F",
        rampScheduleActive ? "T" : "F"
      );
    }
  }

  // ─────────────────────────────────────────────────────────────────
  // DIAG-2: Tryb MANUAL z aktywną rampą AUTO
  // trybManual() i rampaAdaptacyjna piszą niezależnie do backup -
  // skutkuje "ping-pong" PWM -> widoczne mrugnięcia.
  // ─────────────────────────────────────────────────────────────────
  {
    static unsigned long _lastLog = 0;
    if (!tryb && (rampaAdaptacyjnaAktywna || rampScheduleActive || transitionActive || softStartActive)) {
      if (_now - _lastLog > 60000) {
        _lastLog = _now;
        logPrintf(
          "lvl=WARN tag=DIAG-2 msg=\"TRYB MANUAL + aktywna rampa AUTO\" rAdpt=%s rSch=%s tr=%s ss=%s\n",
          rampaAdaptacyjnaAktywna ? "T" : "F",
          rampScheduleActive ? "T" : "F",
          transitionActive ? "T" : "F",
          softStartActive ? "T" : "F"
        );
      }
    }
  }

  // ─────────────────────────────────────────────────────────────────
  // DIAG-3: Skok backup PWM bez aktywnej rampy
  // Jeśli żadna rampa nie jest aktywna, backup nie powinien zmieniać
  // się o więcej niż ~20 PWM między iteracjami loop() (10ms).
  // Duży skok wskazuje na niechronione przypisanie.
  // ─────────────────────────────────────────────────────────────────
  {
    static uint64_t _prevBackup = 0;
    static unsigned long _lastLog = 0;
    bool anyRamp = softStartActive || transitionActive || manualSoftStartActive
                 || manualTransitionActive || rampaAdaptacyjnaAktywna || rampScheduleActive;

    if (!anyRamp && power) {
      // Sprawdź każdy kanał
      for (int i = 0; i < 5; i++) {
        int cur  = (int)getBrightnessForSection(backupBrightnessComposite, i);
        int prev = (int)getBrightnessForSection(_prevBackup, i);
        int delta = abs(cur - prev);
        if (delta > 50 && _now - _lastLog > 60000) {
          _lastLog = _now;
          logPrintf(
            "lvl=WARN tag=DIAG-3 msg=\"SKOK BACKUP (bez rampy)\" ch%d: %d->%d (delta=%d) | tryb=%s\n",
            i, prev, cur, delta, tryb ? "AUTO" : "MANUAL"
          );
          break;  // jeden log na iterację
        }
      }
    }
    _prevBackup = backupBrightnessComposite;
  }

  // ─────────────────────────────────────────────────────────────────
  // DIAG-4: Sprzeczność LED-zasilacz
  // ledSupplyPin LOW gdy PWM=0 lub odwrotnie to marnotrawstwo
  // i może wskazywać na błąd logiki updateLEDs(). (aktywny LOW: LOW=WŁ)
  // ─────────────────────────────────────────────────────────────────
  {
    static unsigned long _lastLog = 0;
    if (_now - _lastLog > 120000) {  // sprawdzaj co 2 minuty
      bool supplyOn = (digitalRead(ledSupplyPin) == LOW);  // aktywny LOW
      bool anyPwmOn = false;
      for (int i = 0; i < 5; i++) {
        if (getBrightnessForSection(backupBrightnessComposite, i) > 0) {
          anyPwmOn = true; break;
        }
      }
      // Zasilacz WŁ ale PWM=0 dłużej niż 2 minuty
      if (supplyOn && !anyPwmOn && power) {
        _lastLog = _now;
        logPrintf("lvl=WARN tag=DIAG-4 msg=\"ZASILACZ WL ale PWM=0 przez >2min\" Sprawdz updateLEDs().\n");
      }
      // Zasilacz WYŁ ale PWM>0 (LED "świeci" bez zasilania - nieprawidłowe sterowanie)
      if (!supplyOn && anyPwmOn && power) {
        _lastLog = _now;
        logPrintf("lvl=WARN tag=DIAG-4 msg=\"PWM>0 ale ZASILACZ WYL\" backup[0]=%d | updateLEDs() pominiete?\n",
          (int)getBrightnessForSection(backupBrightnessComposite, 0));
      }
    }
  }

  // ─────────────────────────────────────────────────────────────────
  // DIAG-5: Temperatura - wykrycie nienaturalnego skoku
  // DS18B20 zwraca czasem poprawną wartość przez 1 odczyt, potem
  // wraca do -127 lub 85. Warto wykryć też skok między odczytami.
  // Próg: >15°C / 15s = niemożliwa zmiana fizyczna.
  // ─────────────────────────────────────────────────────────────────
  {
    static float _prevT1 = 25.0f, _prevT2 = 25.0f, _prevTW = 20.0f;
    static unsigned long _lastLog = 0;
    static unsigned long _lastCheck = 0;

    if (_now - _lastCheck >= 15000) {  // co 15s (interwał odczytu)
      _lastCheck = _now;
      float dT1 = fabsf(tempPlate1 - _prevT1);
      float dT2 = fabsf(tempPlate2 - _prevT2);
      float dTW = fabsf(tempWater  - _prevTW);

      if ((dT1 > 15.0f || dT2 > 15.0f || dTW > 15.0f) && _now - _lastLog > 60000) {
        _lastLog = _now;
        logPrintf(
          "lvl=WARN tag=DIAG-5 msg=\"SKOK TEMPERATURY\" P1: %.1f->%.1f (delta=%.1f) | P2: %.1f->%.1f (delta=%.1f) | W: %.1f->%.1f (delta=%.1f) C\n",
          _prevT1, tempPlate1, dT1,
          _prevT2, tempPlate2, dT2,
          _prevTW, tempWater, dTW
        );
      }
      _prevT1 = tempPlate1;
      _prevT2 = tempPlate2;
      _prevTW = tempWater;
    }
  }

  // ─────────────────────────────────────────────────────────────────
  // DIAG-6: MinLux aktywny w nocy lub poza oknem świecenia
  // minLuxModeActive powinno być false gdy isNightGlobal=true
  // lub gdy harmonogram wyłączył LED (MIDDAY_OFF).
  // ─────────────────────────────────────────────────────────────────
  {
    static unsigned long _lastLog = 0;
    if (minLuxModeActive && isNightGlobal && _now - _lastLog > 120000) {
      _lastLog = _now;
      logPrintf(
        "lvl=WARN tag=DIAG-6 msg=\"MIN LUX aktywny w nocy\" isNightGlobal=T, minLuxPWM[0]=%d - wyczysc stan.\n",
        minLuxCurrentPWM[0]
      );
      // Auto-naprawa: wyczyść stan minLux
      minLuxModeActive = false;
      for (int i = 0; i < 5; i++) minLuxCurrentPWM[i] = 0;
    }
  }

  // ─────────────────────────────────────────────────────────────────
  // DIAG-7: Czas NTP - wykrycie cofnięcia lub skoku zegara
  // Skok >1h do tyłu lub >24h do przodu to prawdopodobnie
  // błąd resynchronizacji NTP lub błąd w getLocalTimePL().
  // ─────────────────────────────────────────────────────────────────
  {
    static int _prevMinutes = -1;
    static unsigned long _lastLog = 0;
    static unsigned long _lastCheck = 0;

    if (_now - _lastCheck >= 30000) {  // co 30s
      _lastCheck = _now;
      int curMin = getLocalMinutes();

      if (_prevMinutes >= 0) {
        int diff = curMin - _prevMinutes;
        // Obsłuż przejście przez północ
        if (diff >  720) diff -= 1440;
        if (diff < -720) diff += 1440;

        // Skok do tyłu > 2 minuty lub do przodu > 90 minut
        bool backwards = (diff < -2);
        bool tooFast   = (diff > 90);

        if ((backwards || tooFast) && _now - _lastLog > 120000) {
          _lastLog = _now;
          logPrintf(
            "lvl=WARN tag=DIAG-7 msg=\"SKOK CZASU NTP\" %02d:%02d -> %02d:%02d (delta=%d min) %s\n",
            _prevMinutes / 60, _prevMinutes % 60,
            curMin / 60, curMin % 60,
            diff,
            backwards ? "<- COFNIECIE" : "-> SKOK DO PRZODU"
          );
        }
      }
      _prevMinutes = curMin;
    }
  }

  // ─────────────────────────────────────────────────────────────────
  // DIAG-8: Watchdog pętli loop() - wykrycie zablokowania
  // Próg 2000ms - reinit I2C TSL (~200ms) jest OK i nie trafia tutaj.
  // ─────────────────────────────────────────────────────────────────
  {
    static unsigned long _lastCall = 0;
    static unsigned long _lastLog  = 0;
    static uint32_t _blockCount    = 0;

    if (_lastCall > 0) {
      unsigned long gap = _now - _lastCall;
      if (gap > 2000) {
        _blockCount++;
        if (_now - _lastLog > 300000UL) {  // max raz na 5 min
          _lastLog = _now;
          logPrintf(
            "lvl=ERR tag=DIAG-8 msg=\"LOOP ZABLOKOWANY %lums\" (lacznie %lu razy) - przyczyna: delay()/I2C/EEPROM/siec\n",
            gap, _blockCount
          );
        }
      }
    }
    _lastCall = _now;
  }

  // ─────────────────────────────────────────────────────────────────
  // DIAG-9: EEPROM niespójność power/tryb
  // Sprawdza co 5 minut, czy wartości w pamięci odpowiadają RAM.
  // Niespójność oznacza, że savePowerTrybToEEPROM() nie został
  // wywołany po ostatniej zmianie.
  // ─────────────────────────────────────────────────────────────────
  {
    static unsigned long _lastCheck = 0;
    static unsigned long _lastLog   = 0;

    if (_now - _lastCheck > 300000) {  // co 5 minut
      _lastCheck = _now;
      uint8_t eePower = 0, eeTryb = 0;
      EEPROM.get(EEPROM_ADDR_POWER, eePower);
      EEPROM.get(EEPROM_ADDR_TRYB,  eeTryb);

      bool mismatchPower = ((bool)eePower != power);
      bool mismatchTryb  = ((bool)eeTryb  != tryb);

      if ((mismatchPower || mismatchTryb) && _now - _lastLog > 120000) {
        _lastLog = _now;
        logPrintf(
          "lvl=WARN tag=DIAG-9 msg=\"EEPROM niespojny\" RAM: power=%s tryb=%s | EEPROM: power=%d tryb=%d - zapisuje.\n",
          power ? "ON" : "OFF", tryb ? "AUTO" : "MAN",
          (int)eePower, (int)eeTryb
        );
        savePowerTrybToEEPROM();  // auto-naprawa
      }
    }
  }

  // ─────────────────────────────────────────────────────────────────
  // DIAG-10: Power Balancer - zbyt długo ogranicza moc
  // gPowerScale < 0.7 przez >5 minut = albo za mocne LED albo błąd
  // w powerTable. Informacja, nie auto-naprawa.
  // ─────────────────────────────────────────────────────────────────
  {
    static unsigned long _scaleLowSince = 0;
    static unsigned long _lastLog = 0;

    if (gPowerScale < 0.7f && power) {
      if (_scaleLowSince == 0) _scaleLowSince = _now;
      if (_now - _scaleLowSince > 300000 && _now - _lastLog > 300000) {
        _lastLog = _now;
        uint16_t curPwm[5];
        for (int i = 0; i < 5; i++) curPwm[i] = (uint16_t)getBrightnessForSection(backupBrightnessComposite, i);
        float totalW = getTotalLEDPower(curPwm);
        logPrintf(
          "lvl=WARN tag=DIAG-10 msg=\"BALANCER ogranicza >5min\" scale=%.2f, moc=%.1fW/%dW | Rozwaz obnizenie AUTO PWM.\n",
          gPowerScale, totalW, LED_MAX_POWER_W
        );
      }
    } else {
      _scaleLowSince = 0;
    }
  }

  // ─────────────────────────────────────────────────────────────────
  // DIAG-11: LittleFS prawie pełny
  // Sprawdza raz na godzinę dostępne miejsce.
  // Przy circular logu (max 600 KB) i historii (~150 KB) plik nie powinien zająć >80% partycji.
  // ─────────────────────────────────────────────────────────────────
  {
    static unsigned long _lastCheck = 0;
    if (littlefsReady && _now - _lastCheck > 3600000) {
      _lastCheck = _now;
      size_t total = LittleFS.totalBytes();
      size_t used  = LittleFS.usedBytes();
      float pct = (total > 0) ? ((float)used / total * 100.0f) : 0.0f;
      if (pct > 85.0f) {
        logPrintf(
          "lvl=WARN tag=DIAG-11 msg=\"LittleFS PRAWIE PELNY\" %.0f%% zajete (%u / %u B). Sprawdz pliki.\n",
          pct, (unsigned)used, (unsigned)total
        );
      } else if (Komentarze && pct > 60.0f) {
        logPrintf("lvl=INFO tag=DIAG-11 msg=\"LittleFS zajete\" %.0f%% (%u / %u B)\n",
          pct, (unsigned)used, (unsigned)total);
      }
    }
  }

  // ─────────────────────────────────────────────────────────────────
  // DIAG-12: Pompa włączona dłużej niż max slot w harmonogramie
  // Ochrona przed "zapomnianą" pompą gdy harmonogram się zmieni.
  // Max ciągłej pracy: obliczany z najdłuższego slotu + 10 min margines.
  // ─────────────────────────────────────────────────────────────────
  {
    static unsigned long _pumpOnSince = 0;
    static unsigned long _lastLog     = 0;

    bool pumpNow = (digitalRead(pumpPin) == LOW);  // aktywny LOW: LOW = włączona
    if (pumpNow && _pumpOnSince == 0) _pumpOnSince = _now;
    if (!pumpNow) _pumpOnSince = 0;

    if (_pumpOnSince > 0) {
      // Znajdź najdłuższy slot pompy
      unsigned long maxSlotMs = 0;
      for (int i = 0; i < pumpSlotCount; i++) {
        int dur = pumpSlots[i].end - pumpSlots[i].start;
        if (dur < 0) dur += 1440;
        if ((unsigned long)dur > maxSlotMs) maxSlotMs = (unsigned long)dur;
      }
      maxSlotMs = (maxSlotMs + 10) * 60000UL;  // minuty -> ms + 10 min margines

      if (maxSlotMs > 0 && (_now - _pumpOnSince) > maxSlotMs && _now - _lastLog > 300000) {
        _lastLog = _now;
        logPrintf(
          "lvl=WARN tag=DIAG-12 msg=\"POMPA WL za dlugo\" >%lu min (max slot pompy). Sprawdz harmonogram.\n",
          (_now - _pumpOnSince) / 60000UL
        );
      }
    }
  }

  // ─────────────────────────────────────────────────────────────────
  // DIAG-13: Rampa adaptacyjna trwa za długo
  // Normalnie kończy się w ADAPTIVE_RAMP_SECONDS (30s).
  // Jeśli trwa >2× dłużej - prawdopodobnie utknęła (target == current
  // lub interval=0 z powodu /0 przy diff==0).
  // ─────────────────────────────────────────────────────────────────
  {
    static unsigned long _rampStartDiag = 0;
    static unsigned long _lastLog       = 0;

    if (rampaAdaptacyjnaAktywna) {
      if (_rampStartDiag == 0) _rampStartDiag = _now;
      unsigned long elapsed = _now - _rampStartDiag;
      unsigned long maxMs   = (unsigned long)ADAPTIVE_RAMP_SECONDS * 2000UL; // 2× limit

      if (elapsed > maxMs && _now - _lastLog > 60000) {
        _lastLog = _now;
        logPrintf(
          "lvl=WARN tag=DIAG-13 msg=\"RAMPA ADAPTACYJNA trwa za dlugo\" %lus (max=%lus) | cel[0]=%d aktual[0]=%d\n",
          elapsed / 1000UL, maxMs / 1000UL,
          (int)rampaAdaptacyjnaCel[0],
          (int)rampaAdaptacyjnaAktualne[0]
        );
        // Auto-naprawa: zakończ rampę, ustaw wartości docelowe
        for (int i = 0; i < 5; i++) {
          setBrightnessForSection(backupBrightnessComposite, i, rampaAdaptacyjnaCel[i]);
        }
        rampaAdaptacyjnaAktywna = false;
        rampaMinLuxPriorytet    = false;
        minLuxModeActive        = false;  // zapobiegaj natychmiastowemu restart w nocy
        logPrintln("lvl=INFO tag=DIAG-13 msg=\"Rampa adaptacyjna wymuszona do celu i zamknieta\"");
      }
    } else {
      _rampStartDiag = 0;
    }
  }

  // ─────────────────────────────────────────────────────────────────
  // DIAG-14: WiFi odłączony dłużej niż 10 minut -> log + eskalacja hard-reconnect
  // (hard-reconnect co 3 min robiony już w bloku WiFi monitor w loop;
  //  tutaj dodatkowa eskalacja gdyby tamten nie zadziałał po 10 min)
  // [OK] FIX-v39-5: usunięto błędny komentarz "ConnectionHandler sam reconnektuje"
  //   - ConnectionHandler nie istnieje od czasu usunięcia ArduinoCloud.
  // ─────────────────────────────────────────────────────────────────
  {
    static unsigned long _wifiLostSince = 0;
    static unsigned long _lastLog       = 0;

    bool wifiOk = (WiFi.status() == WL_CONNECTED);
    if (!wifiOk && _wifiLostSince == 0) _wifiLostSince = _now;
    if (wifiOk)  _wifiLostSince = 0;

    if (_wifiLostSince > 0 && (_now - _wifiLostSince) > 600000 && _now - _lastLog > 300000) {
      _lastLog = _now;
      logPrintf(
        "lvl=WARN tag=DIAG-14 msg=\"WiFi odlaczony od %lu min\" Eskalacja: hard-reconnect.\n",
        (_now - _wifiLostSince) / 60000UL
      );
      // Eskalacja: wymuś hard-reconnect niezależnie od timera w bloku WiFi monitor
      // [FIX-v56] disconnect(false) zamiast disconnect(true) — nie zatrzymuj stosu WiFi
      esp_task_wdt_reset();
      WiFi.disconnect(false);
      esp_task_wdt_reset();
      wifiBeginBest(false);  // [v139] bez skanu (WDT-safe) - round-robin po znanych sieciach
      esp_task_wdt_reset();
    }
  }

  // ── Strażnicy wydzielone jako osobne funkcje (DIAG-15..31) ──
  diagTslFrozenCheck();
  diagManualTransitionStuck();
  diagHeap();
  diagAutoBackupZero();
  diagSunsetRange();
  diagMinLuxOscillation();
  diagNtpTimeout();
  diagPwmRange();
  diagCameraTimeout();
  diagScheduleSanity();
  diagHealthReport();
  diagDeratingDuration();
  diagWaterTemp();
  diagTransmissionSanity();
  diagSensorSwapped();
  diagLearningStuck();
  fsSizeGuard();       // [OK] FS-GUARD: pilnuj łącznego limitu ~9.9 MB

  // [v258] EEPROM.commit() nie jest wykonywany przez ścieżkę config na Core 1.
  // Storage worker na Core 0 obsługuje queue + retry po błędzie commit().
}

// ─────────────────────────────────────────────────────────────────
// DIAG-15: Czujnik TSL „zamrożony" - ta sama wartość >10 minut
// Filtr EMA zmienia luxPokojowy nawet przy stabilnym świetle.
// Brak jakiejkolwiek zmiany = I²C zawieszone lub czujnik odłączony
// bez zgłoszenia błędu (zwraca wewnętrznie ostatnią wartość).
// ─────────────────────────────────────────────────────────────────
void diagTslFrozenCheck() {
  static float         _prevLux      = -1.0f;
  static unsigned long _frozenSince  = 0;
  static unsigned long _lastLog      = 0;
  unsigned long _now = millis();

  if (!czujnikPokojowyAktywny || isNightGlobal) { _frozenSince = 0; return; }

  if (_prevLux < 0.0f) {
    _prevLux = luxPokojowy; return;
  }
  if (fabsf(luxPokojowy - _prevLux) < 0.5f) {
    if (_frozenSince == 0) _frozenSince = _now;
    if (_now - _frozenSince > 600000 && _now - _lastLog > 300000) {
      _lastLog = _now;
      logPrintf("lvl=WARN tag=DIAG-15 msg=\"TSL ZAMROZONY\" luxPokojowy=%.1f lux przez >10 min bez zmiany. I2C OK?\n",
                luxPokojowy);
    }
  } else {
    _frozenSince = 0;
    _prevLux = luxPokojowy;
  }
}

// ─────────────────────────────────────────────────────────────────
// DIAG-16: Rampa manualna zablokowana
// manualTransitionActive powinna trwać MANUALTRANSITIONSECONDS (3s).
// Przy pętli 10ms = ~300 iteracji. Limit: 5× normalny czas = 15s.
// ─────────────────────────────────────────────────────────────────
void diagManualTransitionStuck() {
  static unsigned long _manTrSince = 0;
  static unsigned long _lastLog    = 0;
  unsigned long _now = millis();

  if (manualTransitionActive) {
    if (_manTrSince == 0) _manTrSince = _now;
    unsigned long maxMs = (unsigned long)MANUALTRANSITIONSECONDS * 5000UL;
    if (_now - _manTrSince > maxMs && _now - _lastLog > 60000) {
      _lastLog = _now;
      logPrintf(
        "lvl=WARN tag=DIAG-16 msg=\"MANUAL TRANSITION ZA DLUGA\" %lus (max=%dsx5) | cur[0]=%d -> cel[0]=%d\n",
        (_now - _manTrSince) / 1000UL, MANUALTRANSITIONSECONDS,
        (int)manualTransitionCurrentPwm[0], (int)manualTransitionTargetPwm[0]
      );
      for (int i = 0; i < 5; i++) {
        manualTransitionCurrentPwm[i] = manualTransitionTargetPwm[i];
        setBrightnessForSection(backupBrightnessComposite, i, manualTransitionTargetPwm[i]);
      }
      manualTransitionActive = false;
      logPrintln("lvl=INFO tag=DIAG-16 msg=\"Rampa manualna wymuszona do celu i zamknieta\"");
    }
  } else {
    _manTrSince = 0;
  }
}

// ─────────────────────────────────────────────────────────────────
// DIAG-17: Heap ESP32 - ostrzeżenie, alarm i auto-restart
// <20 kB: ostrzeżenie co 5 min.  <8 kB: alarm przy każdej okazji.
// [v41] AUTO-RESTART: gdy maxAlloc < 8 kB przez >5 min i uptime > 10 min
//   -> system i tak nie odpowiada (WWW/Firebase nie działa) -> restart ratunkowy.
//   Restart tylko między 03:00-04:00 gdy LEDy są wyłączone (noc) LUB
//   gdy sytuacja krytyczna trwa >15 min (niezależnie od godziny).
// ─────────────────────────────────────────────────────────────────
void diagHeap() {
  static unsigned long _lastCheck       = 0;
  static unsigned long _lastWarn        = 0;
  static unsigned long _lastHeartbeat   = 0;
  static unsigned long _fragSince       = 0;   // [v41] kiedy fragmentacja się zaczęła
  static unsigned long _critSince       = 0;   // [v41] kiedy heap krytyczny się zaczął
  unsigned long _now = millis();
  if (_now - _lastCheck < 30000) return;
  _lastCheck = _now;

  uint32_t freeH    = ESP.getFreeHeap();
  uint32_t minH     = ESP.getMinFreeHeap();
  uint32_t maxAlloc = ESP.getMaxAllocHeap();  // największy ciągły wolny blok

  // [v62-1A] Aktualizuj własny tracker minimum z dokładnym znacznikiem czasu
  // [v124-HEAP-MINLOG] Wcześniej ta aktualizacja była cicha — nowe minimum było
  // widoczne dopiero w najbliższym 💓[HEAP] heartbeat (do 60s później). Teraz,
  // analogicznie do loopStack (v70-DIAG) i tgStack (v122-TGSTACK-DIAG), logujemy
  // [INFO] natychmiast. Guard "!= UINT32_MAX" pomija pierwszy pomiar po boocie —
  // sentinel startowy z deklaracji, brak sensownego punktu odniesienia.
  if (freeH < g_heapMinValue) {
    if (g_heapMinValue != UINT32_MAX) {
      logPrintf("lvl=INFO tag=HEAP-MIN poprzednie=%luB nowe=%luB delta=-%luB uptime_s=%lus\n",
                (unsigned long)g_heapMinValue, (unsigned long)freeH,
                (unsigned long)(g_heapMinValue - freeH), (unsigned long)(_now / 1000UL));
    }
    g_heapMinValue     = freeH;
    g_heapMinTimestamp = _now / 1000UL;
  }

  // [v41] Śledź czas trwania krytycznego stanu HEAP
  bool heapCrit = (freeH < 8192);
  bool heapFrag = (maxAlloc < 8192 && freeH >= 8192);

  if (heapCrit) {
    if (_critSince == 0) _critSince = _now;
    if (_fragSince == 0) _fragSince = _now;
  } else if (heapFrag) {
    _critSince = 0;
    if (_fragSince == 0) _fragSince = _now;
  } else {
    _critSince = 0;
    _fragSince = 0;
  }

  // Alarm krytyczny (zawsze)
  if (heapCrit) {
    logPrintf("lvl=ERR tag=DIAG-17 msg=\"HEAP KRYTYCZNY\" free=%luB min=%luB maxAlloc=%luB (crash nieuchronny!)\n",
              (unsigned long)freeH, (unsigned long)minH, (unsigned long)maxAlloc);
  }
  // Alarm fragmentacji
  else if (heapFrag && _now - _lastWarn > 120000) {
    _lastWarn = _now;
    logPrintf("lvl=ERR tag=DIAG-17-FRAG msg=\"FRAGMENTACJA HEAP\" free=%luB maxAlloc=%luB (free OK, ale maxAlloc<8kB - WWW moze nie dzialac!)\n",
              (unsigned long)freeH, (unsigned long)maxAlloc);
  }
  else if (freeH < 50000 && _now - _lastWarn > 60000) {  // [v64-P2c] 20→50 kB, 300→60 s
    _lastWarn = _now;
    logPrintf("lvl=WARN tag=DIAG-17 msg=\"Heap niski (<50kB)\" free=%luB min=%luB maxAlloc=%luB"
              " (TLS handshake potrzebuje ~47kB!)\n",
              (unsigned long)freeH, (unsigned long)minH, (unsigned long)maxAlloc);
  }

  // [v41] AUTO-RESTART gdy heap krytyczny/sfragmentowany przez zbyt długo
  // Minimalne uptime 10 min - nie restartuj przy starcie
  if (_now > 600000UL && (heapCrit || heapFrag) && _fragSince > 0) {
    unsigned long fragDurMs = _now - _fragSince;
    bool restartNow = false;
    struct tm _ti;
    bool timeOk = getLocalTimePL(&_ti);

    // Scenariusz A: fragmentacja > 15 min -> restart natychmiastowy (system nie działa)
    if (fragDurMs > 900000UL) {
      restartNow = true;
      logPrintf("lvl=ERR tag=DIAG-17-RESTART msg=\"HEAP zly - restart ratunkowy\" od=%lu min (niezaleznie od godziny)\n",
                fragDurMs / 60000UL);
    }
    // Scenariusz B: fragmentacja > 5 min + noc (03:00-04:00) + LEDy wyłączone
    else if (fragDurMs > 300000UL && timeOk &&
             _ti.tm_hour == 3 && isNightGlobal) {
      restartNow = true;
      logPrintf("lvl=ERR tag=DIAG-17-RESTART msg=\"HEAP zly - restart nocny\" od=%lu min (03:xx, LEDy wyl.)\n",
                fragDurMs / 60000UL);
    }

    if (restartNow) {
      logPrintf("lvl=ERR tag=DIAG-17-RESTART free=%luB maxAlloc=%luB frag=%s crit=%s uptime=%lus\n",
                (unsigned long)freeH, (unsigned long)maxAlloc,
                heapFrag ? "TAK" : "NIE", heapCrit ? "TAK" : "NIE",
                _now / 1000UL);
      g_restartPending = true;  // [FIX-v154-PRERESET-FLIPFLOP] blokuje heartbeat tgTask
      PRE_RESET_UPDATE(1);  // [v71] src=1 -> DIAG-17
      logForceFlush();
      delay(100);           // [v71] jawne 100ms po flush (bylo 200ms przed)
      ESP.restart();
    }
  }

  // [v97-LOG] Heartbeat co 60s (było 5s [v81-DIAG]) — alarm heapCrit w diagHeap() loguje zawsze
  if (_now - _lastHeartbeat >= 60000) {
    _lastHeartbeat = _now;
    // ── DBG-HEAP: largest_free_block pokazuje fragmentację niezależnie od free ──
    // [v88] PATCH-4: jedno przejście listy heap zamiast dwóch (BM-15: 36µs → 30µs)
    uint32_t _hbFree, _hbLargest;
    getHeapStats(&_hbFree, &_hbLargest);
    size_t largestBlock = (size_t)_hbLargest;
    UBaseType_t tgStack = tgTaskHandle
      ? uxTaskGetStackHighWaterMark(tgTaskHandle) * sizeof(StackType_t)
      : 0xFFFF;
    // ── [v122-TGSTACK-DIAG] tgStack HWM — monitoring analogiczny do loopStack (v70-DIAG) ──
    // Do v121: tgStack_free był tylko liczbą w 💓[HEAP] (heartbeat), bez śledzenia
    // nowego minimum i bez alarmu — w przeciwieństwie do loopStack. tgTask (Core 0:
    // WiFi/Firebase/Telegram/TLS, stos 12288B DRAM, setup() ~12166) ma to samo
    // ryzyko przepełnienia co loop(), więc dostaje ten sam mechanizm.
    // Próg 3000B = ta sama marża % co loopStack/2000B (3000/12288 = 2000/8192 = 24.4%).
    // CANARY (domyślny check stosu w arduino-esp32) wykrywa przepełnienie tylko
    // REAKTYWNIE, w momencie crashu — ten polling ostrzega WCZEŚNIEJ. Pełny research
    // i uzasadnienie progu: patrz CHANGELOG v122 na górze pliku.
    if (tgTaskHandle) {
      static UBaseType_t _prevTgStack = 0xFFFF;
      if (_prevTgStack == 0xFFFF) _prevTgStack = tgStack;
      if (tgStack < _prevTgStack) {
        logPrintf("lvl=INFO tag=STACK msg=\"tgStack_free nowe minimum, HWM dziala w jedna strone - nie oznacza wycieku\" "
                  "poprzednie=%uB nowe=%uB delta=-%uB\n",
                  (unsigned)_prevTgStack, (unsigned)tgStack,
                  (unsigned)(_prevTgStack - tgStack));
        _prevTgStack = tgStack;
      }
      if (tgStack < 3000) {
        logPrintf("lvl=WARN tag=STACK msg=\"Ryzyko stack overflow w tgTask (WiFi/FB/TG)\" tgStack_free=%uB prog=3000\n",
                  (unsigned)tgStack);
      }
    }
    // [v64-P2b] watermark stosu loop() (Core 1) — wykrywa ryzyko stack overflow
    UBaseType_t loopStack = uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t);
    // ── [v70-DIAG] loopStack HWM — jednorazowy log gdy spada do nowego minimum ──
    // HWM (High Water Mark) działa w jedną stronę: rejestruje historyczne minimum
    // stosu, jakie kiedykolwiek wystąpiło. Spadek NIE oznacza wycieku — stos
    // jest zwalniany poprawnie, ale FreeRTOS nie resetuje licznika do góry.
    // Potwierdzono testem loopStack_diag.ino (2026-05-20): wszystkie wzorce
    // (char[], String, struct, buildReport) zwalniają stos po powrocie z funkcji.
    // Obserwuj trend między sesjami; alarm poniżej 2000B.
    {
      static UBaseType_t _prevLoopStack = 0xFFFF;
      if (_prevLoopStack == 0xFFFF) _prevLoopStack = loopStack;
      if (loopStack < _prevLoopStack) {
        logPrintf("lvl=INFO tag=STACK msg=\"loopStack_free nowe minimum, HWM dziala w jedna strone - nie oznacza wycieku\" "
                  "poprzednie=%uB nowe=%uB delta=-%uB\n",
                  (unsigned)_prevLoopStack, (unsigned)loopStack,
                  (unsigned)(_prevLoopStack - loopStack));
        _prevLoopStack = loopStack;
      }
      if (loopStack < 2000) {
        logPrintf("lvl=WARN tag=STACK msg=\"Ryzyko stack overflow w loop()\" loopStack_free=%uB prog=2000\n",
                  (unsigned)loopStack);
      }
    }
    // [v62-1C] psramUsed = całkowity rozmiar PSRAM minus wolne
    size_t psramFree = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t psramUsed = ESP.getPsramSize() - psramFree;
    // [v62-1A] min= z własnego trackera (g_heapMinValue) + czas wystąpienia
    // [v67-Z1] heapSrc= — źródło ostatniego spike'u DRAM
    // [v161] FIX-HEARTBEAT-TRUNC: pojedyncza linia 💓[HEAP] ze wszystkimi polami
    // (v155-v160) ma ~448 bajtów po rozwinięciu, a logPrintf() używa STAŁEGO
    // bufora char[384] (vsnprintf ucina po cichu, bez ostrzeżenia — patrz
    // definicja logPrintf, ~linia 9336). Skutek: fsGuardStallCnt/MaxMs oraz
    // heapSrc=%s były NIEWIDOCZNE w KAŻDYM logu od v158 wzwyż, mimo że kod
    // je poprawnie liczył — samo dopisywanie kolejnych pól bez sprawdzenia
    // całkowitej długości linii cicho je obcinało. Naprawiono: dwie osobne
    // linie logu zamiast jednej, obie bezpiecznie < 384B, wszystkie pola
    // zachowane 1:1. Sufiks "-b" w drugiej linii ułatwia grepowanie razem.
    logPrintf("lvl=INFO tag=HEAP int=%luB psram=%luB psramUsed=%uB min=%luB minAt_s=%lus maxAlloc=%luB largestBlk=%luB uptime_s=%lus tgStack_free=%uB loopStack_free=%uB tlsMutexTotalTG=%lu tlsMutexTotalFB=%lu tlsMutexPeakWait_ms=%lu heapSrc=%s\n",
              (unsigned long)_hbFree, // [v88] PATCH-4: z getHeapStats() — jedno przejście listy
              (unsigned long)psramFree,
              (unsigned)psramUsed,
              (unsigned long)g_heapMinValue, (unsigned long)g_heapMinTimestamp,
              (unsigned long)maxAlloc,
              (unsigned long)largestBlock,
              _now / 1000UL,
              (unsigned)tgStack,
              (unsigned)loopStack,
              (unsigned long)g_tlsMutexWaitTG,
              (unsigned long)g_tlsMutexWaitFB,
              (unsigned long)g_tlsMutexMaxWaitMs,
              heapSrcName((HeapSrcTag)g_heapSrcAccum));
    logPrintf("lvl=INFO tag=HEAP-b tgSendStallCnt=%lu tgSendStallMaxMs=%lu wsCleanupStallCnt=%lu wsCleanupStallMaxMs=%lu flashStallCnt=%lu flashStallMaxMs=%lu eventsStallCnt=%lu eventsStallMaxMs=%lu fsGuardStallCnt=%lu fsGuardStallMaxMs=%lu dnsStallCnt=%lu dnsStallMaxMs=%lu\n",
              (unsigned long)g_tgSendStallCount, // [v155] DIAG-TGSENDSTALL
              (unsigned long)g_tgSendStallMaxMs, // [v155] DIAG-TGSENDSTALL
              (unsigned long)g_wsCleanupStallCount, // [v156] DIAG-WSCLEANUP
              (unsigned long)g_wsCleanupStallMaxMs, // [v156] DIAG-WSCLEANUP
              (unsigned long)g_flashStallCount, // [v157] DIAG-FLASHSTALL
              (unsigned long)g_flashStallMaxMs, // [v157] DIAG-FLASHSTALL
              (unsigned long)g_eventsStallCount, // [v158] DIAG-EVENTSSTALL
              (unsigned long)g_eventsStallMaxMs, // [v158] DIAG-EVENTSSTALL
              (unsigned long)g_fsGuardStallCount, // [v160] DIAG-FSGUARDSTALL
              (unsigned long)g_fsGuardStallMaxMs, // [v160] DIAG-FSGUARDSTALL
              (unsigned long)g_dnsStallCount, // [v162] DIAG-DNSSTALL
              (unsigned long)g_dnsStallMaxMs);       // [v162] DIAG-DNSSTALL
    // [v169] DIAG-FSGROWTH: LittleFS used/total do heartbeatu, w OSOBNEJ linii
    // (patrz komentarz [v161] FIX-HEARTBEAT-TRUNC wyżej — bufor logPrintf to
    // stały char[384], dopisywanie kolejnych pól do już napiętych linii [HEAP]/
    // [HEAP-b] cicho je ucina). Cel: złapać czy widmowy narzut FS (DIAG-FSBREAKDOWN,
    // v168) narasta w czasie/z restartami, zamiast czekać tylko na kolejny trigger
    // FS-GUARD (który loguje used/total tylko przy wyzwoleniu, ~raz na dużo godzin).
    // g_fsUsedLastHb — poprzedni odczyt, do liczenia delty; 0 przy pierwszym
    // wywołaniu po boot (delta wtedy pokazuje used od startu, nie od poprzedniego
    // restartu — to i tak najbardziej interesujący przypadek, patrz plan H2).
    {
      static uint32_t g_fsUsedLastHb = 0;
      size_t fsUsed  = LittleFS.usedBytes();
      size_t fsTotal = g_fsTotalBytes;
      long   fsDelta = g_fsUsedLastHb ? ((long)fsUsed - (long)g_fsUsedLastHb) : 0;
      float  fsPct   = fsTotal ? ((float)fsUsed * 100.0f / (float)fsTotal) : 0.0f;
      logPrintf("lvl=INFO tag=HEAP-c fsUsed=%luB fsTotal=%luB fsPct=%.1f fsDeltaOdOstatniegoHB=%ldB\n",
                (unsigned long)fsUsed, (unsigned long)fsTotal, fsPct, fsDelta);
      g_fsUsedLastHb = (uint32_t)fsUsed;
    }
    g_heapSrcAccum = 0;  // [v69-Z1] reset akumulatora po zalogowaniu

    // ── [FIX-v70-B] Zapis backupBrightnessComposite do EEPROM co minutę ──
    // backupBrightnessComposite nie był zapisywany wcześniej (opisano w komentarzu v16).
    // Po restarcie FIX-v16 używał backupAuto=275 → widoczny skok LED.
    // Zapis co 60s (w heartbeat) = max 1440 zapisów/dobę < EEPROM limit 100k cykli.
    {
      static uint64_t _lastSavedBB = 0xFFFFFFFFFFFFFFFFULL;
      if (backupBrightnessComposite != _lastSavedBB) {
        // [v257] Heartbeat tylko enqueue; zapis + commit wykonuje storage worker Core 0.
        saveBackupBrightnessToEEPROM();
        _lastSavedBB = backupBrightnessComposite;
      }
    }

    // ── [v69-Z3] HEAP-DRIFT: rolling window 60 minut + min60 + numer cyklu ──
    // Push aktualny freeH (kB, uint16) do kołowego bufora
    uint16_t freeH_kB = (uint16_t)(freeH / 1024);
    g_heapHist60[g_heapHistIdx] = freeH_kB;
    if (freeH_kB < g_heapMin60) g_heapMin60 = freeH_kB;  // [v69-Z3a] aktualizuj minimum bieżącej godziny
    g_heapHistIdx = (g_heapHistIdx + 1) % 60;
    if (g_heapHistFill < 60) g_heapHistFill++;

    // Analiza co godzinę: gdy wskaźnik wrócił na 0 (obrót) i bufor pełny.
    if (g_heapHistIdx == 0 && g_heapHistFill >= 60) {
      g_heapDriftCycle++;  // [v69-Z3b] numer godziny od startu
      // Oblicz średnią z ostatnich 60 min
      uint32_t sum = 0;
      for (uint8_t i = 0; i < 60; i++) sum += g_heapHist60[i];
      float avg60 = sum / 60.0f;

      if (g_heapPrevAvg > 0.0f) {
        float drift = avg60 - g_heapPrevAvg;  // kB/h (ujemne = wyciek)
        if (drift < -5.0f) {
          g_heapDriftCount++;
          logPrintf("lvl=WARN tag=HEAP-DRIFT cycle=%u trend_kbh=%.1f avg60_kb=%.1f min60_kb=%u prevavg_kb=%.1f driftcount=%u\n",
                    g_heapDriftCycle, drift, avg60, g_heapMin60, g_heapPrevAvg, g_heapDriftCount);
          if (g_heapDriftCount >= 3) {
            logPrintf("lvl=ERR tag=HEAP-DRIFT msg=\"DRAM spada przez kolejne godziny z rzedu\" godziny=%u cycle=%u min60_kb=%u\n",
                      g_heapDriftCount, g_heapDriftCycle, g_heapMin60);
          }
        } else {
          if (g_heapDriftCount > 0) {
            logPrintf("lvl=INFO tag=HEAP-DRIFT msg=\"drift wyzerowany\" cycle=%u avg60_kb=%.1f min60_kb=%u prevavg_kb=%.1f\n",
                      g_heapDriftCycle, avg60, g_heapMin60, g_heapPrevAvg);
          }
          g_heapDriftCount = 0;
        }
      }
      g_heapMin60   = 65535;  // [v69-Z3a] reset minimum na nową godzinę
      g_heapPrevAvg = avg60;
    }
  }
}

// ─────────────────────────────────────────────────────────────────
// DIAG-18: backupAuto = 0 gdy LED powinny być włączone
// Objaw: brakujący zapis EEPROM lub kolizja z synchronizacją chmury.
// ─────────────────────────────────────────────────────────────────
void diagAutoBackupZero() {
  static unsigned long _lastLog = 0;
  unsigned long _now = millis();
  if (!tryb || !power || isNightGlobal) return;
  if (_now - _lastLog < 600000) return;
  if (backupAutoBrightnessComposite == 0) {
    _lastLog = _now;
    logPrintf("lvl=WARN tag=DIAG-18 msg=\"backupAuto=0 w trybie AUTO z zasilaniem\" LED nigdy sie nie wlacza.\n"
              " Sprawdz zapis EEPROM lub ustaw jasnosc i kliknij 'Zapisz do AUTO'.\n");
  }
}

// ─────────────────────────────────────────────────────────────────
// DIAG-19: Zachód słońca poza zakresem realistycznym dla Polski
// 52°N: zachód mieści się ~16:00-22:00 (960-1320 min).
// ─────────────────────────────────────────────────────────────────
void diagSunsetRange() {
  static unsigned long _lastLog = 0;
  unsigned long _now = millis();
  if (lastNtpSync == 0 || _now - _lastLog < 3600000) return;
  _lastLog = _now;
  if (sunsetMinutes < 960 || sunsetMinutes > 1320) {
    logPrintf("lvl=WARN tag=DIAG-19 msg=\"ZACHOD SLONCA podejrzany\" %02d:%02d (oczekiwany 16:00-22:00 dla PL).\n"
              " Sprawdz szer./dlug. geograficzna i poprawnosc daty NTP.\n",
              sunsetMinutes / 60, sunsetMinutes % 60);
  }
}

// ─────────────────────────────────────────────────────────────────
// DIAG-21: Oscylacja MinLux - zbyt częste uruchamianie rampy
// >5 ramp z powodu minLux w ciągu 10 min = lux ≈ target i filtr
// EMA zbyt czuły -> widoczne cykliczne zmiany jasności.
// ─────────────────────────────────────────────────────────────────
void diagMinLuxOscillation() {
  static bool          _wasActive        = false;
  static uint32_t      _countIn10min     = 0;
  static unsigned long _windowStart      = 0;
  static unsigned long _lastLog          = 0;
  unsigned long _now = millis();

  bool nowActive = (rampaAdaptacyjnaAktywna && rampaMinLuxPriorytet);
  if (nowActive && !_wasActive) {
    _countIn10min++;
    if (_windowStart == 0) _windowStart = _now;
    if (_now - _windowStart > 600000) {
      _countIn10min = 1; _windowStart = _now;
    }
    if (_countIn10min > 5 && _now - _lastLog > 300000) {
      _lastLog = _now;
      logPrintf("lvl=WARN tag=DIAG-21 msg=\"OSCYLACJA MIN LUX\" %u ramp w 10 min | lux=%.0f cel=%.0f\n"
                " Zwieksz minLuxUpdateInterval lub prog MIN_ZMIANA_PWM.\n",
                _countIn10min, luxPokojowy, minLuxDayTarget);
    }
  }
  _wasActive = nowActive;
}

// ─────────────────────────────────────────────────────────────────
// DIAG-22: NTP nie odpowiada >5 minut po połączeniu WiFi
// ─────────────────────────────────────────────────────────────────
void diagNtpTimeout() {
  static unsigned long _wifiOkSince = 0;
  static bool          _warned      = false;
  static unsigned long _lastLog     = 0;
  unsigned long _now = millis();
  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  bool ntpOk  = (time(nullptr) > 1700000000L);
  if (wifiOk && !ntpOk) {
    if (_wifiOkSince == 0) _wifiOkSince = _now;
    if (!_warned && _now - _wifiOkSince > 300000 && _now - _lastLog > 300000) {
      _lastLog = _now; _warned = true;
      logPrintf("lvl=WARN tag=DIAG-22 msg=\"NTP niezsynch\" >5 min po WiFi! Sprawdz serwer NTP / DNS.\n"
                " Harmonogram i pompa pracuja na czasie=0 - ryzyko bledow!\n");
    }
  } else {
    _wifiOkSince = 0; _warned = false;
  }
}

// ─────────────────────────────────────────────────────────────────
// DIAG-23: PWM out-of-range w backupBrightnessComposite
// Każde pole 10-bit: 0-1023. Wartość > 1023 -> integer overflow
// lub błąd przesunięcia bitów -> ledcWrite() dostaje śmieć.
// ─────────────────────────────────────────────────────────────────
void diagPwmRange() {
  static unsigned long _lastLog = 0;
  unsigned long _now = millis();
  if (_now - _lastLog < 10000) return;
  bool bad = false;
  for (int i = 0; i < 5; i++)
    if ((uint16_t)getBrightnessForSection(backupBrightnessComposite, i) > 1023) { bad = true; break; }
  if (!bad) return;
  _lastLog = _now;
  logPrintf("lvl=WARN tag=DIAG-23 msg=\"PWM OUT-OF-RANGE\" val=[%u %u %u %u %u] (clamp do 1023)\n",
    (uint16_t)getBrightnessForSection(backupBrightnessComposite, 0),
    (uint16_t)getBrightnessForSection(backupBrightnessComposite, 1),
    (uint16_t)getBrightnessForSection(backupBrightnessComposite, 2),
    (uint16_t)getBrightnessForSection(backupBrightnessComposite, 3),
    (uint16_t)getBrightnessForSection(backupBrightnessComposite, 4));
  for (int i = 0; i < 5; i++) {
    uint16_t v = (uint16_t)getBrightnessForSection(backupBrightnessComposite, i);
    if (v > 1023) setBrightnessForSection(backupBrightnessComposite, i, 1023);
  }
}

// ─────────────────────────────────────────────────────────────────
// DIAG-24: Kamera aktywna za długo - backup timera auto-wyłącznika
// Jeśli loop() zablokuje się lub cameraOnTime zostanie skasowane,
// kamera zostanie włączona w nieskończoność.
// ─────────────────────────────────────────────────────────────────
void diagCameraTimeout() {
  static unsigned long _lastLog = 0;
  unsigned long _now = millis();
  bool camOn = (digitalRead(cameraPin) == HIGH);
  if (!camOn || cameraOnTime == 0) return;
  unsigned long running = _now - cameraOnTime;
  if (running > 2100000UL && _now - _lastLog > 300000) {  // >35 min
    _lastLog = _now;
    logPrintf("lvl=WARN tag=DIAG-24 msg=\"KAMERA WL >35min (%lu min)\" Auto-wylacznik nie zadzialal - wylaczam.\n",
              running / 60000UL);
    wylaczKamera();
  }
}

// ─────────────────────────────────────────────────────────────────
// DIAG-25: Harmonogram logicznie niespójny w runtime - raz na dobę
// Sprawdza nakładanie się okien po obliczeniu sunsetMinutes.
// ─────────────────────────────────────────────────────────────────
void diagScheduleSanity() {
  static unsigned long _lastCheck = 0;
  unsigned long _now = millis();
  if (lastNtpSync == 0 || _now - _lastCheck < 86400000UL) return;
  _lastCheck = _now;
  int morningStart  = isWeekend() ? (int)MORNING_ON_START_WEEKEND : (int)MORNING_ON_START_WEEKDAY;
  int eveningOnStart = (int)sunsetMinutes + (int)EVENING_ON_BEFORE_SUNSET_MIN;
  if (eveningOnStart < 0)    eveningOnStart += 1440;
  if (eveningOnStart >= 1440) eveningOnStart -= 1440;
  if ((int)MIDDAY_OFF_LOCAL > eveningOnStart)
    logPrintf("lvl=WARN tag=DIAG-25 msg=\"MIDDAY_OFF (%02d:%02d) po wieczorze (%02d:%02d)\" LED wieczorem sie nie wlacza.\n",
      (int)MIDDAY_OFF_LOCAL/60,(int)MIDDAY_OFF_LOCAL%60, eveningOnStart/60, eveningOnStart%60);
  if (morningStart >= (int)EVENING_OFF_START)
    logPrintf("lvl=WARN tag=DIAG-25 msg=\"MORNING_START (%02d:%02d) >= EVENING_OFF (%02d:%02d)\" Brak okna wieczornego.\n",
      morningStart/60, morningStart%60, (int)EVENING_OFF_START/60, (int)EVENING_OFF_START%60);
  if (eveningOnStart < morningStart)
    logPrintf("lvl=WARN tag=DIAG-25 msg=\"wieczor (%02d:%02d) przed rankiem (%02d:%02d) - brak okna swiecenia\"\n",
      eveningOnStart/60, eveningOnStart%60, morningStart/60, morningStart%60);
}

// ─────────────────────────────────────────────────────────────────
// DIAG-26: Raport zdrowia systemu - pełny snapshot co 6 godzin
// Wszystkie kluczowe flagi, PWM, temp, lux, heap - w jednej linii
// blokowej, żeby móc odtworzyć stan systemu z pliku log.
// ─────────────────────────────────────────────────────────────────
void diagHealthReport() {
  static unsigned long _lastReport = 0;
  unsigned long _now = millis();
  if (!Komentarze || _now - _lastReport < 21600000UL) return;
  _lastReport = _now;

  struct tm ti;
  if (!getLocalTimePL(&ti)) return;

  uint16_t pwm[5], autoPwm[5];
  for (int i = 0; i < 5; i++) {
    pwm[i]     = (uint16_t)getBrightnessForSection(backupBrightnessComposite, i);
    autoPwm[i] = (uint16_t)getBrightnessForSection(backupAutoBrightnessComposite, i);
  }
  logPrintf(
    "lvl=INFO tag=HEALTH tryb=%s power=%s noc=%s pwmB=%u,%u,%u,%u,%u pwmA=%u,%u,%u,%u,%u "
    "tempP1=%.1f tempP2=%.1f tempW=%.1f luxPok=%.0f luxWoda=%.0f luxCel=%.0f "
    "minlux=%s adapt=%s ucz=%s scale=%.2f heap=%lu wifi=%s "
    "ss=%s tr=%s mss=%s mtr=%s rA=%s rS=%s zachod=%02d:%02d ntp=%s uptime=%lum\n",
    tryb?"AUTO":"MAN", power?"ON":"OFF", isNightGlobal?"TAK":"NIE",
    pwm[0],pwm[1],pwm[2],pwm[3],pwm[4],
    autoPwm[0],autoPwm[1],autoPwm[2],autoPwm[3],autoPwm[4],
    tempPlate1, tempPlate2, tempWater,
    luxPokojowy, luxNadWoda, minLuxDayTarget,
    minLuxModeEnabled?"WL":"WYL",
    regulacjaAdaptacyjnaWlaczona?"WL":"WYL",
    uczenieSieWlaczone?"WL":"WYL",
    gPowerScale,
    (unsigned long)ESP.getFreeHeap(),
    WiFi.status()==WL_CONNECTED ? WiFi.localIP().toString().c_str() : "BRAK",
    softStartActive?"T":"F", transitionActive?"T":"F",
    manualSoftStartActive?"T":"F", manualTransitionActive?"T":"F",
    rampaAdaptacyjnaAktywna?"T":"F", rampScheduleActive?"T":"F",
    sunsetMinutes/60, sunsetMinutes%60,
    lastNtpSync>0?"OK":"BRAK",
    _now / 60000UL
  );
}

// ─────────────────────────────────────────────────────────────────
// DIAG-27: Derating termiczny ciągły >2h
// Płyta LED nie powinna być stale powyżej 45°C przez wiele godzin.
// Ciągły derating = problem z chłodzeniem LUB zbyt wysokie AUTO PWM.
// Info: jak długo trwa redukcja + ile procent ucinamy.
// ─────────────────────────────────────────────────────────────────
void diagDeratingDuration() {
  static unsigned long _zon1Since = 0;
  static unsigned long _zon2Since = 0;
  static unsigned long _lastLog   = 0;
  unsigned long _now = millis();

  if (derating1ActiveGlobal) {
    if (_zon1Since == 0) _zon1Since = _now;
    if (_now - _zon1Since > 7200000UL && _now - _lastLog > 1800000UL) {
      _lastLog = _now;
      logPrintf(
        "lvl=WARN tag=DIAG-27 msg=\"DERATING ZONE1 ciagly od %lu min\" Temp=%.1fC (prog %.0fC).\n"
        " Sprawdz chlodzenie plyty LED lub obniz AUTO PWM kanalu FS-biale.\n",
        (_now - _zon1Since) / 60000UL, tempPlate1, TEMP_START_DERATING
      );
    }
  } else {
    _zon1Since = 0;
  }

  if (derating2ActiveGlobal) {
    if (_zon2Since == 0) _zon2Since = _now;
    if (_now - _zon2Since > 7200000UL && _now - _lastLog > 1800000UL) {
      _lastLog = _now;
      logPrintf(
        "lvl=WARN tag=DIAG-27 msg=\"DERATING ZONE2 ciagly od %lu min\" Temp=%.1fC (prog %.0fC).\n"
        " Sprawdz chlodzenie plyty LED lub obniz AUTO PWM pozostalych kanalow.\n",
        (_now - _zon2Since) / 60000UL, tempPlate2, TEMP_START_DERATING
      );
    }
  } else {
    _zon2Since = 0;
  }
}

// ─────────────────────────────────────────────────────────────────
// DIAG-28: Temperatura wody poza bezpiecznym zakresem dla ryb
// >28°C: ostrzeżenie (stres u ryb ciepłolubnych).
// >32°C: alarm (niebezpieczne dla większości gatunków).
// <15°C: ostrzeżenie (zbyt zimna woda lub błąd czujnika).
// ─────────────────────────────────────────────────────────────────
void diagWaterTemp() {
  static unsigned long _lastLog = 0;
  unsigned long _now = millis();
  // Sprawdź tylko jeśli NTP OK i czujnik działa (wartość >0)
  if (tempWater <= 0.0f || _now - _lastLog < 1800000UL) return;

  if (tempWater > 32.0f) {
    _lastLog = _now;
    logPrintf(
      "lvl=ERR tag=DIAG-28 msg=\"Woda przegrzana\" %.1fC (>32C). Niebezpieczne dla ryb! Sprawdz chlodzenie akwarium.\n",
      tempWater
    );
  } else if (tempWater > 28.0f) {
    _lastLog = _now;
    logPrintf(
      "lvl=WARN tag=DIAG-28 msg=\"Woda ciepla\" %.1fC (>28C). Ryby moga byc zestresowane.\n",
      tempWater
    );
  } else if (tempWater < 15.0f) {
    _lastLog = _now;
    logPrintf(
      "lvl=WARN tag=DIAG-28 msg=\"Woda zimna\" %.1fC (<15C). Sprawdz czujnik DS18B20 lub grzalke.\n",
      tempWater
    );
  }
}

// ─────────────────────────────────────────────────────────────────
// DIAG-29: Nauczona transmisja poza realnym zakresem fizycznym
// Typowe akwarium: 5-35% (gruba szyba + woda).
// <2%: prawie nic nie przechodzi = czujnik na ścianie za akwarium?
// >50%: prawie wszystko przechodzi = czujniki zamienione miejscami
//        lub czujnik "nad wodą" jest tak naprawdę obok LED.
// ─────────────────────────────────────────────────────────────────
void diagTransmissionSanity() {
  static unsigned long _lastLog = 0;
  unsigned long _now = millis();
  // Sprawdzaj tylko gdy uczenie ma wystarczająco próbek
  if (adaptacja.liczbaProbek < 200 || _now - _lastLog < 3600000UL) return;
  _lastLog = _now;

  float t = adaptacja.nauczonaTransmisja;
  if (isnan(t) || isinf(t)) {
    logPrintf("lvl=ERR tag=DIAG-29 msg=\"Transmisja NaN/Inf\" dane_uczenia=uszkodzone reset_do=10%%\n");
    adaptacja.nauczonaTransmisja = 0.10f;
  } else if (t < 0.02f) {
    logPrintf(
      "lvl=WARN tag=DIAG-29 msg=\"Transmisja bardzo niska\" %.1f%%.\n"
      " Czujnik pokojowy moze byc zamontowany z dala od akwarium (sciana, szuflada).\n",
      t * 100.0f
    );
  } else if (t > 0.50f) {
    logPrintf(
      "lvl=WARN tag=DIAG-29 msg=\"Transmisja bardzo wysoka\" %.1f%%.\n"
      " Mozliwe: czujniki pokojowy i nad-woda zamienione miejscami,\n"
      " lub czujnik 'nad woda' jest obok diod LED (mierzy LED, nie slonce).\n",
      t * 100.0f
    );
  }
}

// ─────────────────────────────────────────────────────────────────
// DIAG-30: luxNadWoda > luxPokojowy - fizycznie niemożliwe
// Czujnik nad wodą ZAWSZE mierzy mniej (szyba + woda tłumią >60%).
// Jeśli luxNadWoda > luxPokojowy -> czujniki zamienione miejscami
// lub czujnik nad-wodą jest bezpośrednio nad diodami LED.
// ─────────────────────────────────────────────────────────────────
void diagSensorSwapped() {
  static unsigned long _swappedSince = 0;
  static unsigned long _lastLog      = 0;
  unsigned long _now = millis();

  // Sprawdzaj tylko gdy oba czujniki aktywne I wybrane, w dzień, przy minimalnym świetle
  if (!czujnikPokojowyAktywny || !czujnikNadWodaAktywny || !uzywajCzujnikaNadWoda
      || isNightGlobal || luxPokojowy < 50.0f) {  // [OK] FIX-v26
    _swappedSince = 0;
    return;
  }

  bool impossible = (luxNadWoda > luxPokojowy * 1.2f);  // 20% margines na szum

  if (impossible) {
    if (_swappedSince == 0) _swappedSince = _now;
    // Zgłoś dopiero po 5 minutach (wyklucz chwilowe skoki przy załączaniu LED)
    if (_now - _swappedSince > 300000 && _now - _lastLog > 600000) {
      _lastLog = _now;
      logPrintf(
        "lvl=WARN tag=DIAG-30 msg=\"CZUJNIKI ZAMIENIONE? luxNadWoda=%.0f > luxPokojowy=%.0f przez >5 min\"\n"
        " Fizycznie niemozliwe - sprawdz montaz i adresy I2C (0x29 vs 0x49).\n",
        luxNadWoda, luxPokojowy
      );
    }
  } else {
    _swappedSince = 0;
  }
  LOOP_CP("end");  // [v125] zamknij ostatnią sekcję profilowania
}

// ─────────────────────────────────────────────────────────────────
// DIAG-31: Uczenie stoi w miejscu
// Gdy uczenieSieWlaczone=true i jest dzień, liczbaProbek powinna
// rosnąć co 30s. Brak wzrostu przez >30 min = czujnik nie dostarcza
// danych (luxPokojowy=0 lub czujnikPokojowyAktywny=false mimo
// że flagę ustawiono true).
// ─────────────────────────────────────────────────────────────────
void diagLearningStuck() {
  static uint32_t      _lastSampleCount = 0;
  static unsigned long _stuckSince      = 0;
  static unsigned long _lastLog         = 0;
  unsigned long _now = millis();

  // Warunki uczenia: włączone, dzień, godziny 8-18
  struct tm ti;
  if (!uczenieSieWlaczone || !czujnikPokojowyAktywny || isNightGlobal) {
    _stuckSince = 0; _lastSampleCount = adaptacja.liczbaProbek; return;
  }
  if (!getLocalTimePL(&ti)) return;
  if (ti.tm_hour < godzinaStartUczenia || ti.tm_hour >= godzinaKoniecUczenia) {
    _stuckSince = 0; _lastSampleCount = adaptacja.liczbaProbek; return;
  }

  if (adaptacja.liczbaProbek != _lastSampleCount) {
    // Próbki rosną - OK
    _stuckSince = 0;
    _lastSampleCount = adaptacja.liczbaProbek;
  } else {
    // Brak wzrostu
    if (_stuckSince == 0) _stuckSince = _now;
    if (_now - _stuckSince > 1800000UL && _now - _lastLog > 600000) {
      _lastLog = _now;
      logPrintf(
        "lvl=WARN tag=DIAG-31 msg=\"UCZENIE STOI\" liczbaProbek=%u bez zmiany przez >30 min.\n"
        " luxPokojowy=%.0f | czujnikAktywny=%s - sprawdz czujnik TSL i warunek uczenia.\n",
        adaptacja.liczbaProbek,
        luxPokojowy,
        czujnikPokojowyAktywny ? "TAK" : "NIE"
      );
    }
  }
}

// ─────────────────────────────────────────────────────────────────
// FS-GUARD: Pilnuj łącznego limitu dla wszystkich plików na LittleFS.
//
// Strategia przycinania (gdy suma > limitu):
//  1. Rotuj log: remove(log_a) + rename(log_b→log_a) — 0 B kopiowania [v102]
//  2. Usuń /history_old.csv (plik rotacji - niepotrzebny)
//  3. Jeśli nadal za dużo - przytnij history.csv (zostaw 128 KB)
//
// WAŻNE - próg FS_LIMIT = 8232000 B (~79% z ~9.9MB dysku) [v102]:
//  Brak log_tmp.txt (dual-file) eliminuje potrzebę 512KB buforu na plik tymczasowy.
//  Bufor 1.8MB teraz pokrywa: CMD-RUN LOGS (33s, ~3KB), oczekiwanie na tgTask,
//  i zapas na wear-leveling LittleFS (2 reserved blocks = 8KB).
//  Poprzedni próg: 76.6% (7983820 B) — zmiana daje ~248KB więcej miejsca.
//
// Wywołanie: raz na minutę z pętli diagów.
// ─────────────────────────────────────────────────────────────────
void fsSizeGuard() {
  static unsigned long _lastCheck = 0;
  unsigned long _now = millis();
  if (!littlefsReady || _now - _lastCheck < 60000UL) return;
  _lastCheck = _now;

  // [v102] FS_LIMIT 76.6% → 79% — brak log_tmp.txt zwalnia 512KB buforu tymczasowego.
  // Bufor 1.8MB nadal pokrywa: CMD-RUN LOGS (33s), tgTask, wear-leveling (2 bloki = 8KB).
  // [v224] FIX-FSGUARD-RACE: zastąp LittleFS.usedBytes() (nie-thread-safe gdy Core 0
  // aktywnie pisze do log_b — potwierdzony bug: joltwallet/esp_littlefs PR#73,
  // lorol/LITTLEFS PR#45, komentarz: "Without it, serious filesystem corruption can occur")
  // cache'owanymi rozmiarami logów. g_logFileSize / g_logFileSizeA to volatile size_t
  // aktualizowane w Core 0 po każdym zamknięciu pliku (logToFile/logFlushCore0,
  // linia ~11900: g_logFileSize = f.size() PRZED f.close()) — odczyt z Core 1 jest
  // bezpieczny (32-bit aligned read na Xtensa = atomowy).
  // Nowy próg: log_a + log_b > LOG_GUARD_LIMIT (7 MB) → guard.
  // Uzasadnienie: logi to jedyna zmienna składowa FS (historia/konfigy są bounded <200KB).
  // 7 MB → ~7.05 MB total, zostawia 1.18 MB buforu do FS_LIMIT (8.23 MB).
  // Poprzedni mechanizm: usedBytes() z Core 1 zwracał losowe/zawyżone wartości rzędu 8+ MB
  // gdy Core 0 miał otwarty plik (wyjaśnia zagadkę v168: "logi tylko 4MB ale guard strzelił").
#ifdef FSGUARD_DRY_RUN
  const size_t LOG_GUARD_LIMIT = 50000UL;       // [DRY-RUN] 50 KB — odpali natychmiast; USUŃ po teście
#else
  const size_t LOG_GUARD_LIMIT = 7UL * 1024UL * 1024UL;  // 7 MB log_a+log_b łącznie
#endif
  size_t _logBytes = g_logFileSize + g_logFileSizeA;      // volatile, thread-safe do odczytu
  if (_logBytes <= LOG_GUARD_LIMIT) return;

  // [OK] FIX-v39-4: nie wykonuj kopiowania plików z loop() (Core 1).
  // Kopiowanie 500KB na LittleFS blokuje loop() przez 4+ sekund -> LOOP-SLOW.
  // Rozwiązanie: ustaw flagę -> tgTaskFn (Core 0) wykona rotację bezpiecznie,
  // tak jak histSavePending i tslReinitPending.
  if (!fsGuardPending) {
    fsGuardPending = true;
    g_fsGuardTriggerUsed = _logBytes;  // [v224] log_a+log_b bytes (nie racy usedBytes())
    logPrintf("lvl=WARN tag=FS-GUARD msg=\"Logi za duze, przycinanie startuje\""
              " log_a=%uB log_b=%uB razem=%uB prog=%uB\n",
              (unsigned)g_logFileSizeA, (unsigned)g_logFileSize,
              (unsigned)_logBytes, (unsigned)LOG_GUARD_LIMIT);
  }
  return;  // powrót do loop() bez blokowania - tgTask wykona resztę

  // FIX-v44: DEAD CODE usunięty - blok kroków 1-3 nigdy nie był osiągalny
  //   (znajdował się za 'return;' powyżej). Faktyczna logika przycinania
  //   jest w tgTaskFn (Core 0), gdzie bezpiecznie blokuje bez wpływu na loop().
}
