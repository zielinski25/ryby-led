// Atrapa Arduino.h dla testów hostowych (tylko to, czego używa telemetry_spool.cpp).
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
uint32_t millis();
inline void* ps_malloc(size_t n) { return malloc(n); }
