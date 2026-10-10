package com.rybyled.panel.core

import android.content.Context

/*
 * Ustawienia tylko na tym telefonie (prywatne SharedPreferences aplikacji).
 * Sekrety NIE są w kodzie ani w repo — wpisujesz je raz w zakładce Ustawienia.
 */
class AppPrefs(context: Context) {
    private val sp = context.getSharedPreferences("ryby_prefs", Context.MODE_PRIVATE)

    var dbUrl: String
        get() = sp.getString(KEY_DB_URL, Logic.DEFAULT_DB_URL) ?: Logic.DEFAULT_DB_URL
        set(v) { sp.edit().putString(KEY_DB_URL, v.trim()).apply() }

    // Database Secret (?auth=)
    var secret: String
        get() = sp.getString(KEY_SECRET, "") ?: ""
        set(v) { sp.edit().putString(KEY_SECRET, v.trim()).apply() }

    // CMD_TOKEN z firmware
    var token: String
        get() = sp.getString(KEY_TOKEN, "") ?: ""
        set(v) { sp.edit().putString(KEY_TOKEN, v.trim()).apply() }

    // Ostatnio wysłany ts komendy (rośnie, firmware odrzuca STALE)
    var lastTs: Long
        get() = sp.getLong(KEY_LAST_TS, 0L)
        set(v) { sp.edit().putLong(KEY_LAST_TS, v).apply() }

    fun clearSecrets() {
        sp.edit().remove(KEY_SECRET).remove(KEY_TOKEN).apply()
    }

    companion object {
        private const val KEY_DB_URL = "ryby_db_url"
        private const val KEY_SECRET = "akw_db_secret"
        private const val KEY_TOKEN = "akw_cmd_token"
        private const val KEY_LAST_TS = "ryby_last_ts"
    }
}
