package pl.aquarium.lednative.core

import org.json.JSONObject

/**
 * Firebase Authentication + RTDB REST.
 * ID token odswiezany po HTTP 401/403. Wartosci auth nigdy nie sa logowane.
 */
class Rtdb(private val prefs: AppPrefs) {

    class SessionRejected(message: String) : Exception(message)

    private val lock = Any()

    @Volatile private var idToken: String? = null
    @Volatile private var expiresAt: Long = 0L

    private fun errorFrom(text: String, fallback: String): String = try {
        when (val e = JSONObject(text).opt("error")) {
            is JSONObject -> e.optString("message").takeIf { it.isNotBlank() } ?: fallback
            is String -> e.takeIf { it.isNotBlank() } ?: fallback
            else -> fallback
        }
    } catch (_: Exception) {
        fallback
    }

    fun signOut() {
        synchronized(lock) {
            idToken = null
            expiresAt = 0L
            prefs.refreshToken = ""
        }
    }

    val signedIn: Boolean get() = prefs.refreshToken.isNotBlank()

    fun signIn(email: String, password: String) {
        require(prefs.apiKey.isNotBlank()) { "Brak Firebase Web API Key" }
        Logic.validateDbUrl(prefs.dbUrl)
        require(email.trim().contains('@') && password.isNotEmpty()) { "Podaj e-mail i haslo Firebase" }
        val body = JSONObject()
            .put("email", email.trim())
            .put("password", password)
            .put("returnSecureToken", true)
            .toString()
        val (code, text) = Http.raw(
            "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=${Http.enc(prefs.apiKey)}",
            "POST", body, maxBytes = Http.MAX_AUTH_BYTES
        )
        if (code !in 200..299) {
            throw SessionRejected(errorFrom(text, "Blad logowania Firebase (HTTP $code)"))
        }
        val json = JSONObject(text)
        val id = json.optString("idToken")
        val refresh = json.optString("refreshToken")
        if (id.isBlank() || refresh.isBlank()) {
            throw SessionRejected("Firebase nie zwrocil tokenu sesji")
        }
        synchronized(lock) {
            idToken = id
            expiresAt = System.currentTimeMillis() + json.optLong("expiresIn", 3600) * 1000L - 60_000L
            prefs.refreshToken = refresh
            prefs.email = email.trim()
        }
    }

    private fun ensureIdToken(): String = synchronized(lock) {
        val current = idToken
        if (!current.isNullOrBlank() && System.currentTimeMillis() < expiresAt) return current
        val refresh = prefs.refreshToken
        if (refresh.isBlank()) throw SessionRejected("Zaloguj sie do Firebase w Ustawieniach")
        val form = "grant_type=refresh_token&refresh_token=${Http.enc(refresh)}"
        val (code, text) = Http.raw(
            "https://securetoken.googleapis.com/v1/token?key=${Http.enc(prefs.apiKey)}",
            "POST", form,
            contentType = "application/x-www-form-urlencoded",
            maxBytes = Http.MAX_AUTH_BYTES
        )
        if (code !in 200..299) {
            val msg = errorFrom(text, "Nie udalo sie odswiezyc sesji Firebase")
            if (code == 400 || code == 401 || code == 403) {
                idToken = null
                expiresAt = 0L
                prefs.refreshToken = ""
                throw SessionRejected("$msg — zaloguj sie ponownie")
            }
            throw HttpError(code, msg)
        }
        val json = JSONObject(text)
        val id = json.optString("id_token")
        if (id.isBlank()) throw SessionRejected("Firebase nie zwrocil odswiezonego tokenu")
        idToken = id
        json.optString("refresh_token").takeIf { it.isNotBlank() }?.let { prefs.refreshToken = it }
        expiresAt = System.currentTimeMillis() + json.optLong("expires_in", 3600) * 1000L - 60_000L
        id
    }

    private fun call(path: String, method: String, body: String?): String {
        val db = Logic.validateDbUrl(prefs.dbUrl)
        var token = ensureIdToken()
        repeat(2) { attempt ->
            val url = "$db${Logic.DEVICE_ROOT}$path.json?auth=${Http.enc(token)}"
            val (code, text) = Http.raw(url, method, body, maxBytes = Http.MAX_RTDB_BYTES)
            if (code in 200..299) return text
            if ((code == 401 || code == 403) && attempt == 0) {
                synchronized(lock) { idToken = null; expiresAt = 0L }
                token = ensureIdToken()
            } else {
                throw HttpError(code, errorFrom(text, "Blad Firebase HTTP $code"))
            }
        }
        throw HttpError(401, "Firebase: autoryzacja nieudana")
    }

    fun fetchStatusRaw(): String = call("/status", "GET", null)

    fun fetchStatus(): DeviceStatus? {
        val text = fetchStatusRaw().trim()
        if (text.isEmpty() || text == "null") return null
        return DeviceStatus.parse(JSONObject(text))
    }

    /** POST {cmd, token, ts} do kolejki /commands. */
    fun postCommand(cmd: String) {
        require(prefs.tokenValid) {
            "Brak poprawnego CMD_TOKEN w APK. Zbuduj aplikacje skryptem BUDUJ-APK.ps1 z aktualnym secrets.h."
        }
        val body = JSONObject()
            .put("cmd", cmd)
            .put("token", prefs.commandToken)
            .put("ts", prefs.nextCommandTs())
            .toString()
        call("/commands", "POST", body)
    }

    /** PATCH /config z tokenem i rosnacym cfgTs. */
    fun patchConfig(fields: JSONObject) {
        require(prefs.tokenValid) {
            "Brak poprawnego CMD_TOKEN w APK. Zbuduj aplikacje skryptem BUDUJ-APK.ps1 z aktualnym secrets.h."
        }
        fields.put("token", prefs.commandToken)
        fields.put("cfgTs", prefs.nextConfigTs())
        call("/config", "PATCH", fields.toString())
    }

    fun fetchConfig(): JSONObject? {
        val text = call("/config", "GET", null).trim()
        if (text.isEmpty() || text == "null") return null
        return JSONObject(text)
    }
}
