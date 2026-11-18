package pl.aquarium.lednative.core

import org.json.JSONArray
import org.json.JSONObject

/** Harmonogram LED w minutach od polnocy. */
data class LedSchedule(
    val morningWD: Int = 390,
    val morningWE: Int = 420,
    val middayOff: Int = 840,
    val eveningBefore: Int = -45,
    val eveningOff: Int? = null,
    val sunsetMin: Int? = null
)

/** Okno pracy pompki. */
data class PumpSlot(val start: String, val end: String)

/**
 * Status urzadzenia. Jedna reprezentacja dla dwoch zrodel:
 * Firebase RTDB (`mode` jako tekst, `temps[]`) oraz LAN `/api/status`
 * (`tryb` jako boolean, `temp1/temp2/water`).
 */
data class DeviceStatus(
    val power: Boolean = false,
    val auto: Boolean = true,
    val pumpOn: Boolean = false,
    val adaptEnabled: Boolean = false,
    val learningEnabled: Boolean = false,
    val pwm: List<Int> = List(Logic.PWM_CHANNELS) { 0 },
    val tempPlate1: Double? = null,
    val tempPlate2: Double? = null,
    val tempWater: Double? = null,
    val luxRoom: Double? = null,
    val luxWater: Double? = null,
    val sensorMode: Int = 2,
    val minLuxEnabled: Boolean = false,
    val minLuxTarget: Int = 2000,
    val minLuxInterval: Int = 30,
    val minLuxActive: Boolean = false,
    val emaFilter: Double = 0.25,
    val sensInt: Int = 30,
    val rampSec: Int = 30,
    val fadeMinutes: Int = 30,
    val schedule: LedSchedule = LedSchedule(),
    val pumpSlots: List<PumpSlot> = emptyList(),
    val energyTodayWh: Double? = null,
    val energyWeekWh: Double? = null,
    val energyMonthWh: Double? = null,
    val powerNowW: Double? = null,
    val powerNowCh: List<Double> = emptyList(),
    val powerLimitW: Double? = null,
    val kwhPrice: Double? = null,
    val hourPowerWh: List<Double> = emptyList(),
    val dayHistory: List<Double> = emptyList(),
    val weekHistory: List<Double> = emptyList(),
    val ledOnMinutesToday: Int? = null,
    val ip: String? = null,
    val fwVersion: String? = null,
    val wifiRssi: Int? = null,
    val uptimeSec: Long? = null,
    val ntpOk: Boolean? = null,
    val localTime: String? = null,
    val inLightingWindow: Boolean? = null,
    val rampActive: Boolean? = null,
    val isNight: Boolean? = null,
    val fsUsed: Long? = null,
    val fsTotal: Long? = null,
    val updatedAt: Long? = null
) {
    val pwmAverage: Int
        get() = if (pwm.isEmpty()) 0 else pwm.sum() / pwm.size

    val pwmPercent: Int
        get() = (pwmAverage * 100.0 / Logic.PWM_MAX).toInt()

    val hasEnergy: Boolean
        get() = energyTodayWh != null

    companion object {
        private fun JSONObject.num(key: String): Double? =
            if (has(key) && !isNull(key)) optDouble(key).takeIf { it.isFinite() } else null

        private fun JSONObject.int(key: String): Int? =
            if (has(key) && !isNull(key)) optInt(key) else null

        private fun JSONObject.bool(key: String): Boolean? =
            if (has(key) && !isNull(key)) optBoolean(key) else null

        private fun JSONObject.str(key: String): String? =
            if (has(key) && !isNull(key)) optString(key).takeIf { it.isNotBlank() } else null

        private fun JSONObject.long(key: String): Long? =
            if (has(key) && !isNull(key)) optLong(key) else null

        private fun JSONArray.doubles(): List<Double> =
            (0 until length()).mapNotNull { i -> optDouble(i).takeIf { it.isFinite() } }

        fun parse(json: JSONObject): DeviceStatus {
            val temps = json.optJSONArray("temps")
            fun tempAt(i: Int): Double? =
                if (temps != null && i < temps.length() && !temps.isNull(i))
                    temps.optDouble(i).takeIf { it.isFinite() }
                else null

            // mode: Firebase zwraca "AUTO"/"MANUAL", LAN zwraca boolean w tryb/mode.
            val auto = when (val m = json.opt("mode")) {
                is String -> m.equals("AUTO", ignoreCase = true)
                is Boolean -> json.bool("tryb") ?: m
                else -> json.bool("tryb") ?: true
            }

            val pwmArr = json.optJSONArray("pwm")
            val pwm = if (pwmArr != null) {
                (0 until Logic.PWM_CHANNELS).map { i ->
                    if (i < pwmArr.length()) Logic.clampPwm(pwmArr.optInt(i)) else 0
                }
            } else List(Logic.PWM_CHANNELS) { 0 }

            val schedObj = json.optJSONObject("schedule")
            val schedule = if (schedObj != null) {
                LedSchedule(
                    morningWD = schedObj.int("morningWD") ?: 390,
                    morningWE = schedObj.int("morningWE") ?: 420,
                    middayOff = schedObj.int("middayOff") ?: 840,
                    eveningBefore = schedObj.int("eveningBefore") ?: -45,
                    eveningOff = schedObj.int("eveningOff"),
                    sunsetMin = schedObj.int("sunsetMin")
                )
            } else LedSchedule()

            val slotsArr = json.optJSONArray("pumpSlots")
            val slots = mutableListOf<PumpSlot>()
            if (slotsArr != null) {
                for (i in 0 until slotsArr.length()) {
                    val o = slotsArr.optJSONObject(i) ?: continue
                    val s = o.str("s") ?: o.int("start")?.let { Logic.fmtHHMM(it) }
                    val e = o.str("e") ?: o.int("end")?.let { Logic.fmtHHMM(it) }
                    if (s != null && e != null) slots.add(PumpSlot(s, e))
                }
            }

            val chArr = json.optJSONArray("powerNowCh")

            return DeviceStatus(
                power = json.bool("power") ?: false,
                auto = auto,
                pumpOn = json.bool("pumpOn") ?: false,
                adaptEnabled = json.bool("adaptEnabled") ?: false,
                learningEnabled = json.bool("learningEnabled") ?: false,
                pwm = pwm,
                tempPlate1 = json.num("temp1") ?: tempAt(0),
                tempPlate2 = json.num("temp2") ?: tempAt(1),
                tempWater = json.num("water") ?: tempAt(2),
                luxRoom = json.num("luxRoom") ?: json.num("luxPokojowy"),
                luxWater = json.num("luxNadWoda") ?: json.num("lux"),
                sensorMode = json.int("sensorMode") ?: 2,
                minLuxEnabled = json.bool("minLuxEnabled") ?: false,
                minLuxTarget = json.int("minLuxTarget") ?: 2000,
                minLuxInterval = json.int("minLuxInterval") ?: 30,
                minLuxActive = json.bool("minLuxActive") ?: false,
                emaFilter = json.num("emaFilter") ?: json.num("emaFilterAlpha") ?: 0.25,
                sensInt = json.int("sensInt") ?: json.int("sensIntSec") ?: 30,
                rampSec = json.int("rampSec") ?: 30,
                fadeMinutes = json.int("fadeMinutes") ?: json.int("fade") ?: 30,
                schedule = schedule,
                pumpSlots = slots,
                energyTodayWh = json.num("energyTodayWh"),
                energyWeekWh = json.num("energyWeekWh"),
                energyMonthWh = json.num("energyMonthWh"),
                powerNowW = json.num("powerNowW") ?: json.num("powerW"),
                powerNowCh = chArr?.doubles() ?: emptyList(),
                powerLimitW = json.num("powerLimitW") ?: json.num("ledMaxW"),
                kwhPrice = json.num("kwhPrice"),
                hourPowerWh = json.optJSONArray("hourPowerWh")?.doubles() ?: emptyList(),
                dayHistory = json.optJSONArray("dayHistory")?.doubles() ?: emptyList(),
                weekHistory = json.optJSONArray("weekHistory")?.doubles() ?: emptyList(),
                ledOnMinutesToday = json.int("ledOnMinutesToday"),
                ip = json.str("ip") ?: json.str("localIp"),
                fwVersion = json.str("fwVersion"),
                wifiRssi = json.int("wifi_rssi") ?: json.int("wifiRssi"),
                uptimeSec = json.long("uptime"),
                ntpOk = json.bool("ntpOK"),
                localTime = json.str("localTime"),
                inLightingWindow = json.bool("inLightingWindow"),
                rampActive = json.bool("rampActive"),
                isNight = json.bool("isNight"),
                fsUsed = json.long("fsUsed"),
                fsTotal = json.long("fsTotal"),
                updatedAt = json.long("updatedAt")
            )
        }
    }
}

/** Ustawienia edytowalne w aplikacji (wysylane do /config jako PATCH). */
data class DeviceSettings(
    val sensorMode: Int = 2,
    val adaptEnabled: Boolean = false,
    val learningEnabled: Boolean = false,
    val emaFilter: Double = 0.25,
    val sensInt: Int = 30,
    val rampSec: Int = 30,
    val fadeMinutes: Int = 30,
    val minLuxEnabled: Boolean = false,
    val minLuxTarget: Int = 2000,
    val minLuxInterval: Int = 30
) {
    companion object {
        fun from(s: DeviceStatus) = DeviceSettings(
            sensorMode = s.sensorMode,
            adaptEnabled = s.adaptEnabled,
            learningEnabled = s.learningEnabled,
            emaFilter = s.emaFilter,
            sensInt = s.sensInt,
            rampSec = s.rampSec,
            fadeMinutes = s.fadeMinutes,
            minLuxEnabled = s.minLuxEnabled,
            minLuxTarget = s.minLuxTarget,
            minLuxInterval = s.minLuxInterval
        )
    }
}

/** Wpis logu z LittleFS. */
data class LogEntry(
    val time: String,
    val level: String,
    val category: String,
    val raw: String
) {
    companion object {
        // Firmware pisze "lvl=INFO tag=..."; starsze linie moga miec "[INFO]" lub "INFO".
        private val LEVEL_KV = Regex("\\blvl\\s*=\\s*([A-Za-z]+)", RegexOption.IGNORE_CASE)
        private val LEVEL = Regex("(?:^|\\s)\\[?(INFO|WARN|ERROR|ERR|DEBUG)\\]?", RegexOption.IGNORE_CASE)
        private val TAG = Regex("tag=([^\\s]+)", RegexOption.IGNORE_CASE)
        private val TIME = Regex("\\d{2}:\\d{2}:\\d{2}")

        fun parse(line: String): LogEntry {
            val level = (
                LEVEL_KV.find(line)?.groupValues?.get(1)
                    ?: LEVEL.find(line)?.groupValues?.get(1)
                )?.uppercase() ?: "INFO"
            val tag = TAG.find(line)?.groupValues?.get(1) ?: ""
            val time = TIME.find(line)?.value ?: ""
            return LogEntry(time, level, Logic.logCategory(level, tag, line), line)
        }

        fun parseAll(text: String): List<LogEntry> =
            text.split('\n').map { it.trimEnd('\r') }.filter { it.isNotBlank() }.map { parse(it) }
    }
}

/** Plik w LittleFS. */
data class FsFile(val name: String, val size: Long)

data class FsListing(
    val files: List<FsFile> = emptyList(),
    val used: Long = 0,
    val total: Long = 0
) {
    companion object {
        fun parse(text: String): FsListing {
            val o = JSONObject(text)
            val arr = o.optJSONArray("files")
            val files = mutableListOf<FsFile>()
            if (arr != null) {
                for (i in 0 until arr.length()) {
                    val f = arr.optJSONObject(i) ?: continue
                    files.add(FsFile(f.optString("name"), f.optLong("size")))
                }
            }
            return FsListing(files, o.optLong("used"), o.optLong("total"))
        }
    }
}
