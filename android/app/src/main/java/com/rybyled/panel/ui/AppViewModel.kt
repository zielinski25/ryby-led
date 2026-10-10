package com.rybyled.panel.ui

import android.app.Application
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.rybyled.panel.core.AppPrefs
import com.rybyled.panel.core.Logic
import com.rybyled.panel.core.Rtdb
import com.rybyled.panel.core.RybyStatus
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/*
 * Stan aplikacji: odczyt statusu co 5 s, wysyłka komend, stan połączenia.
 * Odpowiednik pollStatus / sendCmd z panel/ryby-mobile.html.
 */
class AppViewModel(app: Application) : AndroidViewModel(app) {

    val prefs = AppPrefs(app)

    var status by mutableStateOf<RybyStatus?>(null)
        private set
    var conn by mutableStateOf(Logic.Conn.NONE)
        private set
    var error by mutableStateOf<String?>(null)
        private set
    var notice by mutableStateOf<String?>(null)
        private set

    // Czas telefonu, kiedy ESP ostatnio zmienił updatedAt (licznik millis z ESP, nie czas unix).
    private var changedAt: Long? = null
    private var lastUpdatedAt: Long? = null

    init {
        viewModelScope.launch {
            while (isActive) {
                refresh()
                delay(POLL_MS)
            }
        }
    }

    suspend fun refresh() {
        if (prefs.secret.isBlank()) {
            error = "Wpisz Database Secret w Ustawieniach"
            conn = Logic.Conn.NONE
            return
        }
        try {
            val json = withContext(Dispatchers.IO) { Rtdb.fetchStatus(prefs.dbUrl, prefs.secret) }
            val st = RybyStatus.parse(json)
            status = st
            val upd = st.updatedAt
            if (upd != null && upd != lastUpdatedAt) {
                lastUpdatedAt = upd
                changedAt = System.currentTimeMillis()
            }
            error = null
        } catch (e: Rtdb.HttpError) {
            error = if (e.code == 401) "BŁĄD HTTP 401 — zły Database Secret" else "BŁĄD HTTP ${e.code}"
        } catch (e: Exception) {
            error = "Brak połączenia: ${e.message ?: e.javaClass.simpleName}"
        }
        conn = Logic.connectionState(changedAt, System.currentTimeMillis())
    }

    // Wysyła komendę bez argumentów (np. "power_on"). Wartość musi być na liście PLAIN_CMDS.
    fun sendPlain(cmd: String) {
        if (cmd !in Logic.PLAIN_CMDS) {
            notice = "Nieznana komenda: $cmd"
            return
        }
        viewModelScope.launch { send(cmd) }
    }

    fun sendPwm(values: List<Double?>) {
        viewModelScope.launch { send(Logic.pwmCommand(values)) }
    }

    fun sendAutosave(values: List<Double?>) {
        viewModelScope.launch { send(Logic.autosaveCommand(values)) }
    }

    private suspend fun send(text: String) {
        val token = prefs.token
        if (token.isBlank()) {
            notice = "Wpisz CMD_TOKEN w Ustawieniach"
            return
        }
        val ts = Logic.nextTs(System.currentTimeMillis(), prefs.lastTs)
        try {
            withContext(Dispatchers.IO) {
                Rtdb.postCommand(prefs.dbUrl, prefs.secret, text, token, ts)
            }
            prefs.lastTs = ts
            notice = "Wysłano: $text. ESP odbierze w ciągu ~14 s."
            delay(REFRESH_AFTER_CMD_MS)
            refresh()
        } catch (e: Rtdb.HttpError) {
            notice = "Komenda NIE wysłana: HTTP ${e.code}"
        } catch (e: Exception) {
            notice = "Komenda NIE wysłana: ${e.message ?: e.javaClass.simpleName}"
        }
    }

    // Test połączenia z ustawień (po zapisaniu pól).
    fun testConnection(dbUrl: String, secret: String, token: String) {
        saveSettings(dbUrl, secret, token)
        viewModelScope.launch {
            refresh()
            notice = when (conn) {
                Logic.Conn.ONLINE -> "Połączono — dane świeże"
                Logic.Conn.STALE -> "Połączono, ale ESP dawno nie zapisał statusu"
                Logic.Conn.NONE -> error ?: "Brak danych"
            }
        }
    }

    fun clearNotice() {
        notice = null
    }

    // Zapis ustawień z zakładki Ustawienia (tylko na tym telefonie).
    fun saveSettings(dbUrl: String, secret: String, token: String) {
        prefs.dbUrl = dbUrl.ifBlank { Logic.DEFAULT_DB_URL }
        prefs.secret = secret
        prefs.token = token
        notice = "Zapisano ustawienia na tym telefonie"
    }

    // Kasuje Database Secret i CMD_TOKEN z tego telefonu.
    fun forgetSecrets() {
        prefs.clearSecrets()
        status = null
        conn = Logic.Conn.NONE
        changedAt = null
        lastUpdatedAt = null
        error = "Sekrety usunięte z tego telefonu"
        notice = null
    }

    companion object {
        const val POLL_MS = 5_000L
        const val REFRESH_AFTER_CMD_MS = 15_000L
    }
}
