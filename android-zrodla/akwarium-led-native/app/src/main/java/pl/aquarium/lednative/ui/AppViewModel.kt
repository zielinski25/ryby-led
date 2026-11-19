package pl.aquarium.lednative.ui

import android.app.Application
import android.content.ContentValues
import android.os.Build
import android.os.Environment
import android.provider.MediaStore
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import pl.aquarium.lednative.core.DeviceRepository
import pl.aquarium.lednative.core.DeviceSettings
import pl.aquarium.lednative.core.DeviceStatus
import pl.aquarium.lednative.core.FsListing
import pl.aquarium.lednative.core.LedSchedule
import pl.aquarium.lednative.core.LogEntry
import pl.aquarium.lednative.core.Logic
import pl.aquarium.lednative.core.PumpSlot
import pl.aquarium.lednative.core.TerminalClient

/** Jedna probka historii rysowana na wykresach (zbierana lokalnie co odswiezenie). */
data class Sample(
    val atMs: Long,
    val pwm: List<Int>,
    val tempWater: Double?,
    val lux: Double?,
    val powerW: Double?
)

data class TerminalLine(val at: String, val text: String, val outgoing: Boolean = false)

class AppViewModel(app: Application) : AndroidViewModel(app) {

    private val repo = DeviceRepository(app)
    val prefs get() = repo.prefs

    // --- Stan widoczny w Compose ---
    var status by mutableStateOf<DeviceStatus?>(null)
        private set
    var connection by mutableStateOf(Logic.Conn.NONE)
        private set
    var source by mutableStateOf("—")
        private set
    var lastSync by mutableStateOf<String?>(null)
        private set
    var busy by mutableStateOf(false)
        private set
    var message by mutableStateOf<String?>(null)
    var refreshing by mutableStateOf(false)
        private set
    var discovering by mutableStateOf(false)
        private set
    var lanBase by mutableStateOf(repo.lan.base)
        private set
    var espLocation by mutableStateOf<Pair<Double, Double>?>(null)
        private set
    var signedIn by mutableStateOf(repo.rtdb.signedIn)
        private set
    val tokenReady: Boolean get() = prefs.tokenValid

    val logs = mutableStateListOf<LogEntry>()
    var logsLoading by mutableStateOf(false)
        private set
    var fs by mutableStateOf(FsListing())
        private set
    var fsLoading by mutableStateOf(false)
        private set
    var filePreview by mutableStateOf<Pair<String, String>?>(null)
        private set

    val terminalLines = mutableStateListOf<TerminalLine>()
    var terminalConnected by mutableStateOf(false)
        private set
    var terminalState by mutableStateOf("Rozlaczony")
        private set

    val samples = mutableStateListOf<Sample>()

    private var track: Logic.TrackState? = null
    private var pollJob: Job? = null
    private var terminal: TerminalClient? = null
    private var pendingPwm: List<Int>? = null

    init {
        status = repo.cachedStatus()
        startPolling()
    }

    // --- Powiadomienia ---

    fun notify(text: String) { message = text }
    fun clearMessage() { message = null }

    private fun stamp(): String =
        java.text.SimpleDateFormat("HH:mm:ss", java.util.Locale.US).format(java.util.Date())

    private fun fail(e: Exception, fallback: String) {
        val text = e.message?.takeIf { it.isNotBlank() } ?: fallback
        message = text
    }

    // --- Status ---

    private fun startPolling() {
        pollJob?.cancel()
        pollJob = viewModelScope.launch {
            while (isActive) {
                refresh(silent = true)
                delay(15_000L)
            }
        }
    }

    fun refresh(silent: Boolean = false) {
        if (refreshing) return
        viewModelScope.launch {
            refreshing = true
            try {
                val s = withContext(Dispatchers.IO) { repo.status() }
                applyStatus(s)
                if (!silent) message = "Odswiezono (${source})"
            } catch (e: Exception) {
                connection = Logic.connectionState(track, System.currentTimeMillis())
                    .let { if (it == Logic.Conn.NONE) Logic.Conn.NONE else it }
                if (!silent) fail(e, "Nie udalo sie pobrac statusu")
            } finally {
                refreshing = false
            }
        }
    }

    private fun applyStatus(s: DeviceStatus) {
        status = s
        val now = System.currentTimeMillis()
        track = Logic.trackUpdate(track, s.updatedAt, now)
        connection = if (repo.lastStatusFromLan) Logic.Conn.ONLINE
        else Logic.connectionState(track, now)
        source = if (repo.lastStatusFromLan) "LAN" else "Firebase"
        lastSync = stamp()
        samples.add(Sample(now, s.pwm, s.tempWater, s.luxWater ?: s.luxRoom, s.powerNowW))
        while (samples.size > 240) samples.removeAt(0)
    }

    // --- Komendy ---

    private fun act(success: String, block: () -> Unit) {
        if (busy) return
        viewModelScope.launch {
            busy = true
            try {
                withContext(Dispatchers.IO) { block() }
                message = success
                delay(900L)
                if (pendingPwm == null) refresh(silent = true)
            } catch (e: Exception) {
                fail(e, "Komenda nieudana")
            } finally {
                busy = false
                val latestPwm = pendingPwm
                pendingPwm = null
                if (latestPwm != null) applyPwm(latestPwm)
            }
        }
    }

    fun togglePower() {
        val on = !(status?.power ?: false)
        act(if (on) "Wyslano: swiatlo ON" else "Wyslano: swiatlo OFF") { repo.setPower(on) }
    }

    fun toggleMode() {
        val auto = !(status?.auto ?: true)
        act(if (auto) "Wyslano: tryb AUTO" else "Wyslano: tryb MANUAL") { repo.setMode(auto) }
    }

    fun togglePump() {
        val on = !(status?.pumpOn ?: false)
        act(if (on) "Wyslano: pompka ON" else "Wyslano: pompka OFF") { repo.setPump(on) }
    }

    fun ledFull() = act("Wyslano: LED 100%") { repo.ledTest() }
    fun ledOff() = act("Wyslano: LED OFF") { repo.ledOff() }
    fun restartDevice() = act("Wyslano: restart ESP") { repo.restart() }

    fun applyPwm(values: List<Int>) {
        val requested = values.toList()
        if (busy) {
            // Zostaw tylko najnowszą wartość z gestów wykonanych podczas trwającej komendy.
            pendingPwm = requested
            return
        }
        act("Wyslano PWM: ${Logic.pwmArgs(requested)}") { repo.setPwm(requested) }
    }

    fun saveSchedule(s: LedSchedule, fade: Int) =
        act("Wyslano harmonogram swiatla") { repo.saveSchedule(s, fade) }

    fun savePumpSlots(slots: List<PumpSlot>) =
        act("Wyslano harmonogram pompki") { repo.savePumpSlots(slots) }

    fun saveSettings(s: DeviceSettings) =
        act("Wyslano ustawienia czujnikow") { repo.saveSettings(s) }

    // --- LAN ---

    fun discoverLan() {
        if (discovering) return
        viewModelScope.launch {
            discovering = true
            try {
                val found = withContext(Dispatchers.IO) { repo.lan.discover() }
                lanBase = found
                message = found?.let { "Znaleziono ESP: $it" }
                    ?: "Nie znaleziono ESP w sieci lokalnej"
            } catch (e: Exception) {
                fail(e, "Wykrywanie nieudane")
            } finally {
                discovering = false
            }
        }
    }

    fun setManualHost(host: String) {
        prefs.manualHost = host
        message = if (host.isBlank()) "Wyczyszczono adres reczny" else "Zapisano adres: $host"
    }

    fun loadEspLocation() {
        viewModelScope.launch {
            try {
                if (repo.lan.base == null) {
                    val found = withContext(Dispatchers.IO) { repo.lan.discover() }
                    lanBase = found
                }
                val coords = withContext(Dispatchers.IO) { repo.lan.location() }
                espLocation = coords
            } catch (e: Exception) {
                fail(e, "Nie udało się pobrać lokalizacji z ESP. Sprawdź połączenie LAN.")
            }
        }
    }

    fun saveEspLocation(latitude: Double, longitude: Double) {
        if (busy) return
        viewModelScope.launch {
            busy = true
            try {
                if (repo.lan.base == null) {
                    val found = withContext(Dispatchers.IO) { repo.lan.discover() }
                    lanBase = found
                }
                withContext(Dispatchers.IO) { repo.lan.setLocation(latitude, longitude) }
                espLocation = latitude to longitude
                message = "Lokalizacja zapisana w ESP: %.4f, %.4f".format(latitude, longitude)
            } catch (e: Exception) {
                fail(e, "Nie udało się wysłać lokalizacji do ESP. Połącz telefon i ESP z tą samą siecią Wi-Fi.")
            } finally {
                busy = false
            }
        }
    }

    fun forgetLan() {
        repo.lan.clear()
        lanBase = null
        message = "Wyczyszczono adres LAN"
    }

    // --- Firebase ---

    fun signIn(email: String, password: String, dbUrl: String, apiKey: String) {
        if (busy) return
        viewModelScope.launch {
            busy = true
            try {
                withContext(Dispatchers.IO) {
                    prefs.dbUrl = Logic.validateDbUrl(dbUrl)
                    prefs.apiKey = apiKey.trim()
                    repo.rtdb.signIn(email, password)
                }
                signedIn = true
                message = "Zalogowano do Firebase"
                refresh(silent = true)
            } catch (e: Exception) {
                signedIn = repo.rtdb.signedIn
                fail(e, "Logowanie nieudane")
            } finally {
                busy = false
            }
        }
    }

    fun signOut() {
        repo.rtdb.signOut()
        signedIn = false
        status = null
        connection = Logic.Conn.NONE
        track = null
        message = "Wylogowano z Firebase"
    }

    // --- Logi / pliki (LAN) ---

    fun loadLogs(lines: Int = 200) {
        if (logsLoading) return
        viewModelScope.launch {
            logsLoading = true
            try {
                val list = withContext(Dispatchers.IO) { repo.logLines(lines) }
                logs.clear()
                logs.addAll(list)
                if (list.isEmpty()) message = "Log jest pusty"
            } catch (e: Exception) {
                fail(e, "Logi wymagaja polaczenia LAN z ESP")
            } finally {
                logsLoading = false
            }
        }
    }

    fun clearLogs() {
        viewModelScope.launch {
            busy = true
            try {
                withContext(Dispatchers.IO) { repo.logClear() }
                logs.clear()
                message = "Wyslano polecenie czyszczenia logu"
            } catch (e: Exception) {
                fail(e, "Czyszczenie logu nieudane")
            } finally {
                busy = false
            }
        }
    }

    /** Zapis logu do Pobrane przez MediaStore (natywnie, bez WebView). */
    fun exportLogs() {
        viewModelScope.launch {
            busy = true
            try {
                val text = withContext(Dispatchers.IO) { repo.logDownload() }
                val name = "ryby-led-log-${System.currentTimeMillis()}.txt"
                val where = withContext(Dispatchers.IO) { saveLog(name, text) }
                message = where?.let { "Zapisano: $it" } ?: "Nie udalo sie zapisac pliku"
            } catch (e: Exception) {
                fail(e, "Eksport logu wymaga LAN")
            } finally {
                busy = false
            }
        }
    }

    /**
     * Android 10+: MediaStore Downloads (bez uprawnien).
     * Android 8/9: katalog Pobrane w pamieci aplikacji (bez WRITE_EXTERNAL_STORAGE).
     * Zwraca opis lokalizacji albo null.
     */
    private fun saveLog(name: String, text: String): String? {
        val ctx = getApplication<Application>()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            val resolver = ctx.contentResolver
            val values = ContentValues().apply {
                put(MediaStore.Downloads.DISPLAY_NAME, name)
                put(MediaStore.Downloads.MIME_TYPE, "text/plain")
                put(MediaStore.Downloads.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS)
            }
            val uri = resolver.insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, values)
                ?: return null
            val ok = resolver.openOutputStream(uri)?.use { out ->
                out.write(text.toByteArray(Charsets.UTF_8))
                true
            } ?: false
            return if (ok) "Pobrane/$name" else null
        }
        return try {
            val dir = java.io.File(
                ctx.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS), ""
            )
            if (!dir.exists()) dir.mkdirs()
            val file = java.io.File(dir, name)
            file.writeText(text, Charsets.UTF_8)
            file.absolutePath
        } catch (_: Exception) {
            null
        }
    }

    fun loadFs() {
        if (fsLoading) return
        viewModelScope.launch {
            fsLoading = true
            try {
                fs = withContext(Dispatchers.IO) { repo.fsList() }
            } catch (e: Exception) {
                fail(e, "Lista plikow wymaga LAN")
            } finally {
                fsLoading = false
            }
        }
    }

    fun openFile(name: String) {
        viewModelScope.launch {
            try {
                val text = withContext(Dispatchers.IO) { repo.fsView(name) }
                filePreview = name to text
            } catch (e: Exception) {
                fail(e, "Nie udalo sie otworzyc pliku")
            }
        }
    }

    fun closeFile() { filePreview = null }

    // --- Terminal WebSocket ---

    private fun term(): TerminalClient {
        terminal?.let { return it }
        val t = TerminalClient(
            onLine = { line ->
                viewModelScope.launch(Dispatchers.Main.immediate) {
                    terminalLines.add(TerminalLine(stamp(), line, false))
                    while (terminalLines.size > 400) terminalLines.removeAt(0)
                }
            },
            onState = { connected, text ->
                viewModelScope.launch(Dispatchers.Main.immediate) {
                    terminalConnected = connected
                    terminalState = text
                }
            }
        )
        terminal = t
        return t
    }

    fun connectTerminal() {
        val url = repo.terminalUrl()
        if (url == null) {
            message = "Terminal wymaga LAN — najpierw wykryj ESP"
            return
        }
        term().connect(url)
    }

    fun disconnectTerminal() {
        terminal?.disconnect()
        terminalConnected = false
        terminalState = "Rozlaczony"
    }

    fun sendTerminal(command: String) {
        val cmd = command.trim()
        if (cmd.isEmpty()) return
        val t = term()
        if (!t.connected) {
            message = "Terminal nie jest polaczony"
            return
        }
        if (t.send(cmd)) {
            terminalLines.add(TerminalLine(stamp(), cmd, true))
            while (terminalLines.size > 400) terminalLines.removeAt(0)
        } else {
            message = "Nie udalo sie wyslac komendy"
        }
    }

    fun clearTerminal() { terminalLines.clear() }

    override fun onCleared() {
        pollJob?.cancel()
        terminal?.shutdown()
        super.onCleared()
    }
}
