package pl.aquarium.lednative.core

import android.content.Context
import pl.aquarium.lednative.BuildConfig

/** Preferencje aplikacji. CMD_TOKEN wylacznie z BuildConfig (nieedytowalny w UI). */
class AppPrefs(context: Context) {
    private val appContext = context.applicationContext
    private val sp = appContext.getSharedPreferences("aquarium_native", Context.MODE_PRIVATE)

    var apiKey: String
        get() = sp.getString("apiKey", null)?.takeIf { it.isNotBlank() } ?: Logic.DEFAULT_API_KEY
        set(v) { sp.edit().putString("apiKey", v.trim()).apply() }

    var dbUrl: String
        get() = sp.getString("dbUrl", null)?.takeIf { it.isNotBlank() } ?: Logic.DEFAULT_DB_URL
        set(v) { sp.edit().putString("dbUrl", v.trim().trimEnd('/')).apply() }

    var email: String
        get() = sp.getString("email", "") ?: ""
        set(v) { sp.edit().putString("email", v.trim()).apply() }

    var manualHost: String
        get() = sp.getString("manualHost", "") ?: ""
        set(v) { sp.edit().putString("manualHost", v.trim()).apply() }

    var localBase: String?
        get() = sp.getString("localBase", null)
        set(v) {
            if (v == null) sp.edit().remove("localBase").apply()
            else sp.edit().putString("localBase", v).apply()
        }

    var lastTs: Long
        get() = sp.getLong("lastTs", 0L)
        set(v) { sp.edit().putLong("lastTs", v).apply() }

    var lastCfgTs: Long
        get() = sp.getLong("lastCfgTs", 0L)
        set(v) { sp.edit().putLong("lastCfgTs", v).apply() }

    var cachedStatus: String?
        get() = sp.getString("cachedStatus", null)
        set(v) { sp.edit().putString("cachedStatus", v).apply() }

    /** Token urzadzenia wkompilowany w APK z firmware secrets.h. */
    val commandToken: String get() = BuildConfig.RYBY_CMD_TOKEN.trim()

    val tokenValid: Boolean
        get() = commandToken.isNotBlank() && commandToken != "ZMIEN_MNIE" && commandToken.length <= 128

    var refreshToken: String
        get() = SecretBox.read(appContext, "refreshToken")
        set(v) {
            if (v.isBlank()) SecretBox.clear(appContext, "refreshToken")
            else SecretBox.write(appContext, "refreshToken", v)
        }

    val hasCloudConfig: Boolean
        get() = apiKey.isNotBlank() && dbUrl.isNotBlank() && email.isNotBlank()

    fun nextCommandTs(): Long =
        Logic.nextTs(System.currentTimeMillis(), lastTs).also { lastTs = it }

    fun nextConfigTs(): Long =
        Logic.nextTs(System.currentTimeMillis(), lastCfgTs).also { lastCfgTs = it }
}
