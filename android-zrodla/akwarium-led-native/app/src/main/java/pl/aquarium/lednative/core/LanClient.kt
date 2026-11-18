package pl.aquarium.lednative.core

import android.content.Context
import android.net.ConnectivityManager
import android.net.LinkAddress
import org.json.JSONArray
import org.json.JSONObject
import java.io.IOException
import java.net.Inet4Address
import java.net.URL
import java.util.concurrent.Callable
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit

/**
 * Lokalny transport LAN do ESP (HTTP, port 8080).
 * Chronione trasy dostaja naglowek X-Cmd-Token.
 */
class LanClient(private val context: Context, private val prefs: AppPrefs) {

    @Volatile var base: String? = prefs.localBase
        private set

    val available: Boolean get() = base != null

    private fun normalize(value: String): String? = try {
        val raw = value.trim().trimEnd('/')
        val candidate = if (raw.startsWith("http://") || raw.startsWith("https://")) raw else "http://$raw"
        val parsed = URL(candidate)
        val port = if (parsed.port >= 0) parsed.port else 8080
        val path = parsed.path.orEmpty().trimEnd('/')
        "${parsed.protocol}://${parsed.host}:$port$path"
    } catch (_: Exception) {
        null
    }

    fun clear() {
        base = null
        prefs.localBase = null
    }

    fun setBase(value: String?) {
        base = value
        prefs.localBase = value
    }

    private fun request(
        b: String,
        path: String,
        method: String,
        body: String? = null,
        maxBytes: Int = Http.MAX_LAN_BYTES
    ): String = Http.request(
        b.trimEnd('/') + path, method, body,
        headers = mapOf("X-Cmd-Token" to prefs.commandToken),
        maxBytes = maxBytes,
        connectTimeoutMs = 2500, readTimeoutMs = 6000
    )

    fun api(path: String, method: String = "GET", body: String? = null): String {
        val b = base ?: throw IOException("ESP nie jest dostepne lokalnie")
        return request(b, path, method, body)
    }

    private fun probe(b: String): Boolean = try {
        val body = request(b, "/api/status", "GET", maxBytes = 64 * 1024)
        body.contains("fwVersion") && body.contains("pwm") && body.contains("power")
    } catch (_: Exception) {
        false
    }

    /** Zwraca adres wykrytego ESP albo null. Najpierw kandydaci, potem skan podsieci. */
    fun discover(): String? {
        val candidates = linkedSetOf<String>()
        prefs.manualHost.takeIf { it.isNotBlank() }?.let { normalize(it)?.let(candidates::add) }
        base?.let { normalize(it)?.let(candidates::add) }
        prefs.localBase?.let { normalize(it)?.let(candidates::add) }
        candidates.add("http://ryby-led-s3.local:8080")
        for (c in candidates) {
            if (probe(c)) { setBase(c); return c }
        }
        val hosts = subnetHosts()
        if (hosts.isEmpty()) return null
        val pool = Executors.newFixedThreadPool(32)
        val found = try {
            val tasks = hosts.map { host ->
                Callable<String?> {
                    val b = "http://$host:8080"
                    if (probe(b)) b else null
                }
            }
            pool.invokeAll(tasks, 8, TimeUnit.SECONDS).firstNotNullOfOrNull { f ->
                try { if (!f.isCancelled) f.get() else null } catch (_: Exception) { null }
            }
        } finally {
            pool.shutdownNow()
        }
        if (found != null) setBase(found)
        return found
    }

    private fun subnetHosts(): List<String> {
        val cm = context.applicationContext
            .getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager
        val network = cm.activeNetwork ?: return emptyList()
        val props = cm.getLinkProperties(network) ?: return emptyList()
        val links: List<LinkAddress> = props.linkAddresses
        val out = linkedSetOf<String>()
        for (link in links) {
            val address = link.address as? Inet4Address ?: continue
            if (address.isLoopbackAddress || address.isLinkLocalAddress) continue
            val prefix = link.prefixLength
            if (prefix !in 16..30) continue
            val b = address.address
            val current = ((b[0].toInt() and 255) shl 24) or ((b[1].toInt() and 255) shl 16) or
                ((b[2].toInt() and 255) shl 8) or (b[3].toInt() and 255)
            val mask = (-1 shl (32 - prefix))
            val net = current and mask
            val broadcast = net or mask.inv()
            var start = net + 1
            var end = broadcast - 1
            if (end - start > 510) {
                start = (current - 255).coerceAtLeast(net + 1)
                end = (current + 255).coerceAtMost(broadcast - 1)
            }
            for (n in start..end) {
                if (n == current) continue
                out.add("${(n ushr 24) and 255}.${(n ushr 16) and 255}.${(n ushr 8) and 255}.${n and 255}")
            }
        }
        return out.toList()
    }

    fun status(): DeviceStatus = DeviceStatus.parse(JSONObject(api("/api/status")))

    fun setPower(on: Boolean) { api("/set-power?v=${if (on) 1 else 0}") }
    fun setMode(auto: Boolean) { api("/set-tryb?v=${if (auto) 1 else 0}") }
    fun setPump(on: Boolean) { api("/api/quick/pump_${if (on) "on" else "off"}", "POST") }
    fun restart() { api("/api/quick/restart", "POST") }

    fun setPwm(values: List<Int>) {
        val q = (0 until Logic.PWM_CHANNELS).joinToString("&") { i ->
            "pwm$i=${Logic.clampPwm(values.getOrNull(i))}"
        }
        api("/set-pwm?$q")
    }

    fun saveSchedule(s: LedSchedule, fadeMinutes: Int) {
        val q = "mw=${Http.enc(Logic.fmtHHMM(s.morningWD))}" +
            "&me=${Http.enc(Logic.fmtHHMM(s.morningWE))}" +
            "&mo=${Http.enc(Logic.fmtHHMM(s.middayOff))}" +
            "&eb=${s.eveningBefore.coerceIn(-180, 180)}"
        api("/set-schedule?$q")
        api("/set-fade?f=${fadeMinutes.coerceIn(1, 180)}")
    }

    fun savePumpSlots(slots: List<PumpSlot>) {
        val arr = JSONArray()
        slots.forEach { arr.put(JSONObject().put("s", it.start).put("e", it.end)) }
        api("/set-pump-schedule", "POST", arr.toString())
    }

    fun saveSettings(s: DeviceSettings) {
        api("/set-ema-filter?v=${Http.enc(s.emaFilter.toString())}")
        // Uwaga: lokalne endpointy /set-sens-int i /set-ramp-sec przyjmuja 5..120
        // (wegziej niz chmura: 5..300 i 1..600). Przycinamy, aby ESP nie zwrocil 400.
        api("/set-sens-int?v=${s.sensInt.coerceIn(5, 120)}")
        api("/set-ramp-sec?v=${s.rampSec.coerceIn(5, 120)}")
        api("/set-fade?f=${s.fadeMinutes.coerceIn(1, 180)}")
        val adapt = JSONObject()
            .put("sensorMode", s.sensorMode.coerceIn(0, 2))
            .put("adaptEnabled", if (s.adaptEnabled) 1 else 0)
            .put("learningEnabled", if (s.learningEnabled) 1 else 0)
        api("/api/adaptive/save", "POST", adapt.toString())
        val minLux = JSONObject()
            .put("enabled", if (s.minLuxEnabled) 1 else 0)
            .put("target", s.minLuxTarget.coerceIn(500, 8000))
            .put("interval", s.minLuxInterval.coerceIn(5, 300))
        api("/api/minlux/save", "POST", minLux.toString())
    }

    fun logLines(lines: Int = 150): List<LogEntry> =
        LogEntry.parseAll(api("/api/log-lines?lines=$lines"))

    fun fsList(): FsListing = FsListing.parse(api("/api/fs-list"))

    fun fsView(name: String, lines: Int = 200): String =
        api("/api/fs-view?file=${Http.enc(name)}&lines=$lines")

    fun logDownload(): String = api("/api/log-download")

    fun logClear(): String = api("/api/log-clear", "POST")

    /** Adres WebSocket terminala firmware (ws://host:8080/ws). */
    fun terminalWsUrl(): String? {
        val b = base ?: return null
        return try {
            val u = URL(b)
            val scheme = if (u.protocol == "https") "wss" else "ws"
            val port = if (u.port >= 0) u.port else 8080
            "$scheme://${u.host}:$port/ws"
        } catch (_: Exception) {
            null
        }
    }
}
