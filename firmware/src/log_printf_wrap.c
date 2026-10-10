// log_printf_wrap.c
//
// FIX (2026-08-04): Rdzeń frameworka (pioarduino/framework-arduinoespressif32,
// IDF_VER v5.5.4, PLATFORMIO=60119) dodaje globalnie do linkera flagę
// -Wl,--wrap=log_printf (widoczna w build_log.txt obok -Wl,--wrap=longjmp),
// przez co KAŻDE wywołanie log_printf() w całym projekcie — łącznie z
// wywołaniami wewnątrz samego rdzenia (chip-debug-report.cpp) oraz w
// AsyncTCP.cpp (przez makra log_e/log_w z esp32-hal-log.h) — zostaje
// przekierowane do symbolu __wrap_log_printf.
//
// Problem: w tej wersji frameworka implementacja __wrap_log_printf
// NIGDZIE nie istnieje (esp32-hal-log-wrapper.c dostarcza tylko
// __wrap_esp_log_write / __wrap_esp_log_writev — inny symbol).
// Efekt: "undefined reference to `__wrap_log_printf'" przy linkowaniu.
//
// To NIE jest błąd w naszym kodzie ani kwestia CORE_DEBUG_LEVEL —
// to brakująca implementacja w samym frameworku. Rozwiązanie: dostarczyć
// minimalny wrapper samodzielnie. Przekazuje po prostu dalej do vprintf,
// więc logi nadal trafiają na Serial przez standardowe przekierowanie
// stdout, które ESP32 Arduino ustawia przy starcie.
//
// Nie ma to nic wspólnego z autorskim systemem logowania (logToFile,
// [WARN][FLASH-STALL] itd. z main.cpp) — to zupełnie inny, niezależny
// mechanizm (logi frameworka na Serial), zostaje nietknięty.

#include <stdarg.h>
#include <stdio.h>

int __wrap_log_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vprintf(fmt, args);
    va_end(args);
    return ret;
}
