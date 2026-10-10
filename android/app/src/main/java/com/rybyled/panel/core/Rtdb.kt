package com.rybyled.panel.core

import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URL
import java.net.URLEncoder

/*
 * Klient Firebase RTDB (REST) — port fetchStatus / postCommand z panel/ryby-mobile.html.
 * Tylko HTTPS (cleartext zablokowany w manifeście). Sekret idzie w query ?auth=.
 */
object Rtdb {

    class HttpError(val code: Int) : Exception("HTTP $code")

    private fun base(dbUrl: String): String = dbUrl.trim().trimEnd('/')

    private fun enc(s: String): String = URLEncoder.encode(s, "UTF-8")

    private fun request(url: String, method: String, body: String?): String {
        val conn = URL(url).openConnection() as HttpURLConnection
        try {
            conn.requestMethod = method
            conn.connectTimeout = 8_000
            conn.readTimeout = 10_000
            if (body != null) {
                conn.doOutput = true
                conn.setRequestProperty("Content-Type", "application/json; charset=utf-8")
                conn.outputStream.use { it.write(body.toByteArray(Charsets.UTF_8)) }
            }
            val code = conn.responseCode
            if (code !in 200..299) throw HttpError(code)
            return conn.inputStream.bufferedReader(Charsets.UTF_8).use { it.readText() }
        } finally {
            conn.disconnect()
        }
    }

    // GET /aquarium/status.json. Gdy węzeł nie istnieje, RTDB zwraca "null" -> pusty obiekt.
    fun fetchStatus(dbUrl: String, secret: String): JSONObject {
        val text = request("${base(dbUrl)}/aquarium/status.json?auth=${enc(secret)}", "GET", null).trim()
        return if (text.isEmpty() || text == "null") JSONObject() else JSONObject(text)
    }

    // POST /aquarium/commands.json body {cmd, token, ts}. ts musi rosnąć (firmware odrzuca STALE).
    fun postCommand(dbUrl: String, secret: String, cmd: String, token: String, ts: Long) {
        val body = JSONObject().put("cmd", cmd).put("token", token).put("ts", ts).toString()
        request("${base(dbUrl)}/aquarium/commands.json?auth=${enc(secret)}", "POST", body)
    }
}
