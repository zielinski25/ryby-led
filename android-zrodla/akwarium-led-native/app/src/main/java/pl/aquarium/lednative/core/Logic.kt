package pl.aquarium.lednative.core

import java.net.URI
import java.util.Locale
import kotlin.math.abs
import kotlin.math.max
import kotlin.math.roundToInt

/**
 * Logika bez UI i bez Androida — testowalna na JVM.
 *
 * Kontrakt z firmware:
 *  - odczyt:  GET  {DB}/aquarium/ryby-led-s3-01/status.json?auth=ID_TOKEN
 *  - komendy: POST {DB}/aquarium/ryby-led-s3-01/commands.json?auth=ID_TOKEN
 *             body {cmd, token, ts}; firmware czyta kolejke co ~14 s
 *  - komendy z argumentami: "pwm a b c d e" (0-1023)
 */
object Logic {
    const val DEFAULT_DB_URL =
        "https://akwarium-367be-default-rtdb.europe-west1.firebasedatabase.app"
    const val DEFAULT_API_KEY = "AIzaSyDcheuRNcNo4mzNpaTzn-19Ntw62djkfVU"
    const val DEVICE_ROOT = "/aquarium/ryby-led-s3-01"

    const val STALE_MS = 150_000L
    const val PWM_MAX = 1023
    const val PWM_CHANNELS = 5

    val PLAIN_CMDS = listOf(
        "power_on", "power_off", "mode_auto", "mode_manual",
        "pump_on", "pump_off", "led_100", "led_test", "led_off", "restart"
    )

    val HHMM = Regex("^([01][0-9]|2[0-3]):[0-5][0-9]$")

    /** Wymusza HTTPS i domene Firebase, aby ID token nie trafil na obcy serwer. */
    fun validateDbUrl(raw: String): String {
        val trimmed = raw.trim().trimEnd('/')
        require(trimmed.isNotEmpty()) { "Podaj adres bazy Firebase" }
        val uri = try {
            URI(trimmed)
        } catch (_: Exception) {
            throw IllegalArgumentException("Nieprawidlowy adres URL bazy")
        }
        require(uri.scheme?.lowercase(Locale.ROOT) == "https") {
            "Adres bazy musi zaczynac sie od https://"
        }
        val host = uri.host?.lowercase(Locale.ROOT)
            ?: throw IllegalArgumentException("Brak nazwy serwera w adresie bazy")
        require(uri.userInfo == null && uri.rawQuery == null && uri.rawFragment == null) {
            "Adres bazy nie moze zawierac parametrow zapytania ani poswiadczen"
        }
        require(host.endsWith(".firebasedatabase.app") || host.endsWith(".firebaseio.com")) {
            "Dozwolona wylacznie domena *.firebasedatabase.app lub *.firebaseio.com"
        }
        return trimmed
    }

    fun clampPwm(v: Int?): Int = v?.coerceIn(0, PWM_MAX) ?: 0

    fun pwmArgs(values: List<Int>): String =
        (0 until PWM_CHANNELS).joinToString(" ") { clampPwm(values.getOrNull(it)).toString() }

    fun pwmCommand(values: List<Int>): String = "pwm " + pwmArgs(values)

    /** ts musi byc rosnacy wzgledem ostatniego wyslanego (inaczej firmware odrzuca STALE). */
    fun nextTs(nowMs: Long, lastTs: Long): Long = max(nowMs, lastTs + 1)

    fun checkPlainCommand(cmd: String): String {
        require(cmd in PLAIN_CMDS) { "nieznana komenda: $cmd" }
        return cmd
    }

    fun toMinutes(t: String): Int {
        require(HHMM.matches(t)) { "Godzina musi miec format HH:MM" }
        return t.substringBefore(':').toInt() * 60 + t.substringAfter(':').toInt()
    }

    fun fmtHHMM(minutes: Int?): String {
        if (minutes == null) return "--:--"
        val n = ((minutes % 1440) + 1440) % 1440
        return String.format(Locale.US, "%02d:%02d", n / 60, n % 60)
    }

    /** Zachod slonca + offset, zawiniete do doby. */
    fun shiftedTime(sunsetMin: Int?, offsetMin: Int): String {
        if (sunsetMin == null) return "--:--"
        return fmtHHMM(sunsetMin + offsetMin)
    }

    fun durationBetween(start: String, end: String): Int {
        if (!HHMM.matches(start) || !HHMM.matches(end)) return 0
        val d = toMinutes(end) - toMinutes(start)
        return if (d >= 0) d else d + 1440
    }

    /**
     * Swieznosc danych z ESP. updatedAt to millis() sterownika, wiec pojedynczy
     * odczyt nie dowodzi, ze ESP zyje teraz — potrzebna jest obserwacja zmiany.
     */
    enum class Conn { NONE, UNVERIFIED, ONLINE, STALE }

    data class TrackState(
        val updatedAt: Long? = null,
        val changedAt: Long? = null,
        val firstSeenAt: Long? = null
    )

    fun trackUpdate(prev: TrackState?, updatedAt: Long?, nowMs: Long): TrackState {
        val p = prev ?: TrackState()
        if (updatedAt == null) return p
        if (p.updatedAt == null) {
            return TrackState(updatedAt = updatedAt, changedAt = null, firstSeenAt = nowMs)
        }
        if (updatedAt != p.updatedAt) {
            return TrackState(
                updatedAt = updatedAt,
                changedAt = nowMs,
                firstSeenAt = p.firstSeenAt ?: nowMs
            )
        }
        return p
    }

    fun connectionState(track: TrackState?, nowMs: Long): Conn {
        if (track == null || track.updatedAt == null) return Conn.NONE
        val changed = track.changedAt
        if (changed != null) {
            return if (nowMs - changed > STALE_MS) Conn.STALE else Conn.ONLINE
        }
        val first = track.firstSeenAt ?: return Conn.NONE
        return if (nowMs - first > STALE_MS) Conn.STALE else Conn.UNVERIFIED
    }

    fun fmtTemp(v: Double?): String =
        if (v == null || !v.isFinite() || v <= -50.0) "—"
        else String.format(Locale.US, "%.1f", v)

    fun fmtLux(v: Double?): String =
        if (v == null || !v.isFinite() || v < 0.0) "—" else v.roundToInt().toString()

    fun fmtWatts(v: Double?): String =
        if (v == null || !v.isFinite()) "—" else String.format(Locale.US, "%.1f", v)

    fun fmtWh(v: Double?): String {
        if (v == null || !v.isFinite()) return "—"
        return if (abs(v) >= 1000) String.format(Locale.US, "%.2f kWh", v / 1000)
        else "${v.roundToInt()} Wh"
    }

    fun fmtKwh(v: Double?): String =
        if (v == null || !v.isFinite()) "—" else String.format(Locale.US, "%.2f", v / 1000)

    fun costPln(wh: Double?, pricePerKwh: Double?): String {
        if (wh == null || !wh.isFinite() || pricePerKwh == null || !pricePerKwh.isFinite()) return "—"
        return String.format(Locale.US, "%.2f", wh / 1000.0 * pricePerKwh)
    }

    fun fmtMinutes(minutes: Int?): String {
        if (minutes == null) return "—"
        val n = max(0, minutes)
        val h = n / 60
        val m = n % 60
        return if (h == 0) "$m min" else "$h h ${String.format(Locale.US, "%02d", m)} min"
    }

    fun fmtUptime(seconds: Long?): String {
        if (seconds == null || seconds < 0) return "—"
        val d = seconds / 86400
        val h = (seconds % 86400) / 3600
        val m = (seconds % 3600) / 60
        return when {
            d > 0 -> "$d d $h h"
            h > 0 -> "$h h $m min"
            else -> "$m min"
        }
    }

    /** Klasyfikacja linii logu do filtra kategorii. */
    fun logCategory(level: String, tag: String, line: String): String {
        val hay = (tag + " " + line).lowercase(Locale.ROOT)
        return when {
            level == "ERROR" || level == "ERR" -> "err"
            level == "WARN" -> "warn"
            "temp" in hay || "ds18" in hay -> "temp"
            "pwm" in hay || "led" in hay -> "led"
            "ramp" in hay -> "ramp"
            "pump" in hay || "pomp" in hay -> "pump"
            "adapt" in hay -> "adapt"
            "ntp" in hay -> "ntp"
            else -> "all"
        }
    }
}
