package com.rybyled.panel.core

import org.json.JSONArray
import org.json.JSONObject

/*
 * Model statusu z /aquarium/status.json (port normalizeStatus z panel/ryby-mobile.html).
 * Brakujące pola dostają bezpieczne wartości (null / 0 / pusta lista) — UI nie może się wywalić
 * na starszym firmware.
 */

data class Energy(val today: Double, val week: Double, val month: Double)

data class PumpSlot(val start: Double?, val end: Double?)

data class RybyStatus(
    val mode: String?,
    val power: Boolean,
    val pumpOn: Boolean,
    val temps: List<Double?>,
    val luxRoom: Double?,
    val luxNadWoda: Double?,
    val pwm: List<Int>,
    val pct: Double?,
    val autoPwm: Double?,
    val powerNowW: Double?,
    val powerLimitW: Double?,
    val powerNowCh: List<Double>,
    val peakPowerWToday: Double?,
    val energy: Energy,
    val ledMin: Energy,
    val kwhPrice: Double?,
    val dayHistory: List<Double>,
    val weekHistory: List<Double>,
    val pumpSlots: List<PumpSlot>,
    val fadeMinutes: Double?,
    val rampSec: Double?,
    val minLuxEnabled: Boolean,
    val minLuxActive: Boolean,
    val minLuxTarget: Double?,
    val inLightingWindow: Boolean,
    val isNight: Boolean,
    val rampActive: Boolean,
    val adaptEnabled: Boolean,
    val wifiRssi: Double?,
    val uptimeS: Double?,
    val updatedAt: Long?
) {
    companion object {
        fun parse(d: JSONObject): RybyStatus {
            val temps = d.optJSONArray("temps").items()
            val pwm = d.optJSONArray("pwm").items()
            val ch = d.optJSONArray("powerNowCh").items()
            return RybyStatus(
                mode = when (d.optString("mode")) {
                    "MANUAL" -> "MANUAL"
                    "AUTO" -> "AUTO"
                    else -> null
                },
                power = d.opt("power") == true,
                pumpOn = d.opt("pumpOn") == true,
                temps = List(3) { numOf(temps.getOrNull(it)) },
                luxRoom = d.num("luxRoom") ?: d.num("lux"),
                luxNadWoda = d.num("luxNadWoda"),
                pwm = List(Logic.PWM_CHANNELS) { Logic.clampPwm(numOf(pwm.getOrNull(it))) },
                pct = d.num("pct"),
                autoPwm = d.num("autoPWM"),
                powerNowW = d.num("powerNowW"),
                powerLimitW = d.num("powerLimitW"),
                powerNowCh = List(Logic.PWM_CHANNELS) { numOf(ch.getOrNull(it)) ?: 0.0 },
                peakPowerWToday = d.num("peakPowerWToday"),
                energy = Energy(
                    today = d.num("energyTodayWh") ?: 0.0,
                    week = d.num("energyWeekWh") ?: 0.0,
                    month = d.num("energyMonthWh") ?: 0.0
                ),
                ledMin = Energy(
                    today = d.num("ledOnMinutesToday") ?: 0.0,
                    week = d.num("ledOnMinutesWeek") ?: 0.0,
                    month = d.num("ledOnMinutesMonth") ?: 0.0
                ),
                kwhPrice = d.num("kwhPrice"),
                dayHistory = d.optJSONArray("dayHistory").items().map { numOf(it) ?: 0.0 },
                weekHistory = d.optJSONArray("weekHistory").items().map { numOf(it) ?: 0.0 },
                pumpSlots = d.optJSONArray("pumpSlots").items().map { s ->
                    val o = s as? JSONObject
                    PumpSlot(o?.num("start"), o?.num("end"))
                },
                fadeMinutes = d.num("fadeMinutes"),
                rampSec = d.num("rampSec"),
                minLuxEnabled = d.opt("minLuxEnabled") == true,
                minLuxActive = d.opt("minLuxActive") == true,
                minLuxTarget = d.num("minLuxTarget"),
                inLightingWindow = d.opt("inLightingWindow") == true,
                isNight = d.opt("isNight") == true,
                rampActive = d.opt("rampActive") == true,
                adaptEnabled = d.opt("adaptEnabled") == true,
                wifiRssi = d.num("wifi_rssi"),
                uptimeS = d.num("uptime"),
                updatedAt = d.num("updatedAt")?.toLong()
            )
        }
    }
}

// Liczba z JSON; akceptuje też liczby zapisane jako tekst (jak num() w JS). Nie-skończone -> null.
internal fun numOf(v: Any?): Double? = when (v) {
    is Number -> v.toDouble().takeIf { it.isFinite() }
    is String -> v.trim().toDoubleOrNull()?.takeIf { it.isFinite() }
    else -> null
}

internal fun JSONObject.num(key: String): Double? = if (isNull(key)) null else numOf(opt(key))

internal fun JSONArray?.items(): List<Any?> =
    if (this == null) emptyList() else (0 until length()).map { opt(it) }
