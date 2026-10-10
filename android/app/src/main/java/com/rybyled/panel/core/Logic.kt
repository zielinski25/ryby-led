package com.rybyled.panel.core

import java.util.Locale
import kotlin.math.abs
import kotlin.math.max
import kotlin.math.roundToInt

/*
 * Ryby LED — logika bez UI (port 1:1 z LOGIC-BEGIN w panel/ryby-mobile.html).
 *
 * Kontrakt z firmware (4.7.2+build.272):
 *   - odczyt:  GET  {DB}/aquarium/status.json?auth=SECRET   (firmware pisze co ~60 s)
 *   - komendy: POST {DB}/aquarium/commands.json?auth=SECRET  body {cmd, token, ts}
 *              firmware czyta kolejkę co ~14 s; ts MUSI rosnąć (inaczej STALE).
 *   - komendy z argumentami: "pwm a b c d e" (0–1023), "autosave a b c d e".
 */
object Logic {
    const val DEFAULT_DB_URL = "https://akwarium-367be-default-rtdb.europe-west1.firebasedatabase.app"

    // Status zapisywany co 60 s; po 150 s bez zmiany updatedAt uznajemy, że ESP nie pisze.
    const val STALE_MS = 150_000L
    const val PWM_MAX = 1023
    const val PWM_CHANNELS = 5

    // Komendy bez argumentów, które firmware obsługuje (dispatcher wykonajKomendeFirebase).
    val PLAIN_CMDS = listOf(
        "power_on", "power_off", "mode_auto", "mode_manual",
        "pump_on", "pump_off", "led_100", "led_test", "led_off", "restart"
    )

    fun clampPwm(v: Double?): Int {
        if (v == null || !v.isFinite()) return 0
        return v.coerceIn(0.0, PWM_MAX.toDouble()).roundToInt()
    }

    // 5 kanałów -> "a b c d e" (kolejność jak w firmware)
    fun pwmArgs(values: List<Double?>): String =
        (0 until PWM_CHANNELS).joinToString(" ") { clampPwm(values.getOrNull(it)).toString() }

    fun pwmCommand(values: List<Double?>): String = "pwm " + pwmArgs(values)

    fun autosaveCommand(values: List<Double?>): String = "autosave " + pwmArgs(values)

    // ts musi być rosnący względem ostatnio wysłanego (firmware odrzuca STALE)
    fun nextTs(nowMs: Long, lastTs: Long): Long = max(nowMs, lastTs + 1)

    // Nazwa komendy z UI musi być na liście obsługiwanych przez firmware.
    fun checkPlainCommand(cmd: String): String {
        require(cmd in PLAIN_CMDS) { "nieznana komenda: $cmd" }
        return cmd
    }

    enum class Conn { NONE, ONLINE, STALE }

    // changedAt = czas telefonu (ms) z chwili ostatniej zmiany updatedAt; null = brak danych.
    fun connectionState(changedAt: Long?, nowMs: Long): Conn {
        if (changedAt == null) return Conn.NONE
        return if (nowMs - changedAt > STALE_MS) Conn.STALE else Conn.ONLINE
    }

    fun fmtTemp(v: Double?): String = if (v == null) "—" else String.format(Locale.US, "%.1f °C", v)

    fun fmtLux(v: Double?): String = if (v == null) "—" else "${v.roundToInt()} lx"

    fun fmtWatts(v: Double?): String = if (v == null) "—" else String.format(Locale.US, "%.1f W", v)

    // Wh -> "123 Wh" / "1.23 kWh"
    fun fmtWh(v: Double?): String {
        if (v == null) return "—"
        if (abs(v) >= 1000) return String.format(Locale.US, "%.2f kWh", v / 1000)
        return "${v.roundToInt()} Wh"
    }

    // minuty -> "2 h 05 min"
    fun fmtMinutes(v: Double?): String {
        if (v == null) return "—"
        val n = max(0, v.roundToInt())
        val h = n / 60
        val m = n % 60
        if (h == 0) return "$m min"
        return String.format(Locale.US, "%d h %02d min", h, m)
    }

    // minuty od północy -> "HH:MM"
    fun fmtHHMM(minutes: Double?): String {
        if (minutes == null || minutes < 0 || minutes > 1439) return "—"
        val n = minutes.roundToInt()
        return String.format(Locale.US, "%02d:%02d", n / 60, n % 60)
    }

    // Koszt energii w PLN (cena z ESP; brak ceny -> null)
    fun costPln(wh: Double?, pricePerKwh: Double?): Double? {
        if (wh == null || pricePerKwh == null) return null
        return Math.round((wh / 1000) * pricePerKwh * 100) / 100.0
    }

    fun fmtPln(v: Double?): String = if (v == null) "—" else String.format(Locale.US, "%.2f zł", v)

    // Uptime sekundy -> "3 d 4 h" / "12 min"
    fun fmtUptime(seconds: Double?): String {
        if (seconds == null) return "—"
        val s = max(0L, seconds.toLong())
        val d = s / 86400
        val h = (s % 86400) / 3600
        val m = (s % 3600) / 60
        if (d > 0) return "$d d $h h"
        if (h > 0) return "$h h $m min"
        return "$m min"
    }
}
