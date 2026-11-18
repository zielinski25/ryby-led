package pl.aquarium.lednative.core

import android.content.Context
import org.json.JSONObject

/**
 * Orkiestracja transportu.
 *
 * Sterowanie i status ida NAJPIERW do Firebase (komendy sa widoczne w kolejce
 * /commands). LAN jest fallbackiem, gdy chmura zawiedzie, oraz jedynym zrodlem
 * logow, plikow LittleFS i terminala WebSocket.
 */
class DeviceRepository(context: Context) {

    val prefs = AppPrefs(context)
    val rtdb = Rtdb(prefs)
    val lan = LanClient(context, prefs)

    @Volatile var lastStatusFromLan: Boolean = false
        private set

    class CommandResult(val viaLan: Boolean)

    private fun requireCloud() {
        require(prefs.hasCloudConfig) { "Skonfiguruj Firebase w Ustawieniach" }
    }

    /** Cloud-first z fallbackiem LAN. Zwraca informacje o uzytym transporcie. */
    private fun command(cloud: () -> Unit, local: (() -> Unit)?): CommandResult {
        if (prefs.hasCloudConfig && rtdb.signedIn) {
            try {
                cloud()
                return CommandResult(false)
            } catch (e: Exception) {
                if (local != null && lan.available) {
                    try {
                        local()
                        return CommandResult(true)
                    } catch (_: Exception) {
                        throw e
                    }
                }
                throw e
            }
        }
        if (local != null && lan.available) {
            local()
            return CommandResult(true)
        }
        requireCloud()
        throw IllegalStateException("Zaloguj sie do Firebase w Ustawieniach")
    }

    fun status(): DeviceStatus {
        if (prefs.hasCloudConfig && rtdb.signedIn) {
            try {
                val raw = rtdb.fetchStatusRaw()
                val text = raw.trim()
                if (text.isNotEmpty() && text != "null") {
                    prefs.cachedStatus = text
                    lastStatusFromLan = false
                    return DeviceStatus.parse(JSONObject(text))
                }
            } catch (e: Exception) {
                if (!lan.available) throw e
            }
        }
        if (lan.available) {
            val s = lan.status()
            lastStatusFromLan = true
            return s
        }
        throw IllegalStateException("Brak polaczenia: skonfiguruj Firebase lub podaj adres ESP")
    }

    fun cachedStatus(): DeviceStatus? = prefs.cachedStatus?.let {
        try { DeviceStatus.parse(JSONObject(it)) } catch (_: Exception) { null }
    }

    fun setPower(on: Boolean) = command(
        cloud = { rtdb.postCommand(if (on) "power_on" else "power_off") },
        local = { lan.setPower(on) }
    )

    fun setMode(auto: Boolean) = command(
        cloud = { rtdb.postCommand(if (auto) "mode_auto" else "mode_manual") },
        local = { lan.setMode(auto) }
    )

    fun setPump(on: Boolean) = command(
        cloud = { rtdb.postCommand(if (on) "pump_on" else "pump_off") },
        local = { lan.setPump(on) }
    )

    fun restart() = command(
        cloud = { rtdb.postCommand("restart") },
        local = { lan.restart() }
    )

    fun ledTest() = command(
        cloud = { rtdb.postCommand("led_100") },
        local = { lan.api("/api/quick/led_test", "POST"); Unit }
    )

    fun ledOff() = command(
        cloud = { rtdb.postCommand("led_off") },
        local = { lan.api("/api/quick/led_off", "POST"); Unit }
    )

    fun setPwm(values: List<Int>) = command(
        cloud = { rtdb.postCommand(Logic.pwmCommand(values)) },
        local = { lan.setPwm(values) }
    )

    fun saveSchedule(s: LedSchedule, fadeMinutes: Int) = command(
        cloud = {
            val o = JSONObject()
                .put("configType", "schedule")
                .put("morningWD", s.morningWD.coerceIn(0, 1439))
                .put("morningWE", s.morningWE.coerceIn(0, 1439))
                .put("middayOff", s.middayOff.coerceIn(0, 1439))
                .put("eveningBefore", s.eveningBefore.coerceIn(-180, 180))
                .put("fadeMinutes", fadeMinutes.coerceIn(1, 180))
            rtdb.patchConfig(o)
            rtdb.postCommand("config_refresh")
        },
        local = { lan.saveSchedule(s, fadeMinutes) }
    )

    fun savePumpSlots(slots: List<PumpSlot>) = command(
        cloud = {
            require(slots.isNotEmpty()) { "Dodaj co najmniej jedno okno pompki" }
            require(slots.size <= 6) { "Maksymalnie 6 okien pompki" }
            // Klucze "0".."5": dokladnie tego wymagaja reguly ($slotIndex).
            // RTDB przy GET zwroci to jako tablice [{s,e},...] — tak parsuje firmware.
            val obj = JSONObject()
            slots.forEachIndexed { i, slot ->
                require(Logic.HHMM.matches(slot.start) && Logic.HHMM.matches(slot.end)) {
                    "Godziny pompki musza miec format HH:MM"
                }
                obj.put(i.toString(), JSONObject().put("s", slot.start).put("e", slot.end))
            }
            val o = JSONObject()
                .put("configType", "pump")
                .put("pumpSlots", obj)
                .put("pumpSlotCount", slots.size)
            rtdb.patchConfig(o)
            rtdb.postCommand("config_refresh")
        },
        local = { lan.savePumpSlots(slots) }
    )

    /**
     * Ustawienia czujnikow. Firebase przyjmuje je w trzech osobnych PATCH-ach
     * (adapt / minlux / params) — zgodnie z regulami i parserem firmware.
     */
    fun saveSettings(s: DeviceSettings) = command(
        cloud = {
            rtdb.patchConfig(
                JSONObject()
                    .put("configType", "adapt")
                    .put("sensorMode", s.sensorMode.coerceIn(0, 2))
                    .put("adaptEnabled", s.adaptEnabled)
                    .put("learningEnabled", s.learningEnabled)
            )
            rtdb.patchConfig(
                JSONObject()
                    .put("configType", "minlux")
                    .put("minLuxEnabled", s.minLuxEnabled)
                    .put("minLuxTarget", s.minLuxTarget.coerceIn(500, 8000))
                    .put("minLuxInterval", s.minLuxInterval.coerceIn(5, 300))
            )
            rtdb.patchConfig(
                JSONObject()
                    .put("configType", "params")
                    .put("emaFilter", s.emaFilter.coerceIn(0.05, 0.5))
                    .put("sensInt", s.sensInt.coerceIn(5, 300))
                    .put("rampSec", s.rampSec.coerceIn(1, 600))
            )
            // fadeMinutes nalezy do bloku "schedule" w parserze firmware.
            rtdb.patchConfig(
                JSONObject()
                    .put("configType", "schedule")
                    .put("fadeMinutes", s.fadeMinutes.coerceIn(1, 180))
            )
            rtdb.postCommand("config_refresh")
        },
        local = { lan.saveSettings(s) }
    )

    // --- Wylacznie LAN ---

    fun logLines(lines: Int = 150): List<LogEntry> = lan.logLines(lines)
    fun fsList(): FsListing = lan.fsList()
    fun fsView(name: String): String = lan.fsView(name)
    fun logDownload(): String = lan.logDownload()
    fun logClear(): String = lan.logClear()
    fun terminalUrl(): String? = lan.terminalWsUrl()
}
