package pl.aquarium.lednative.core

import java.io.ByteArrayOutputStream
import java.io.IOException
import java.io.InputStream
import java.net.HttpURLConnection
import java.net.URL
import java.net.URLEncoder

class HttpError(val code: Int, message: String) : IOException(message)
class ResponseTooLarge(val limitBytes: Int) :
    IOException("Odpowiedz przekracza limit $limitBytes B")

/**
 * Wspolny klient HTTP. Bez przekierowan (token nie moze wyciec na inny host),
 * z twardym limitem rozmiaru odpowiedzi i rozroznieniem bledu HTTP od sieciowego.
 */
object Http {
    const val MAX_AUTH_BYTES = 16 * 1024
    const val MAX_RTDB_BYTES = 256 * 1024
    const val MAX_LAN_BYTES = 1024 * 1024

    fun enc(value: String): String = URLEncoder.encode(value, "UTF-8")

    private fun readBounded(input: InputStream?, maxBytes: Int): String {
        if (input == null) return ""
        input.use { stream ->
            val out = ByteArrayOutputStream(minOf(maxBytes, 8 * 1024))
            val buf = ByteArray(4 * 1024)
            while (true) {
                val n = stream.read(buf)
                if (n < 0) break
                if (out.size().toLong() + n > maxBytes.toLong()) throw ResponseTooLarge(maxBytes)
                out.write(buf, 0, n)
            }
            return out.toString(Charsets.UTF_8.name())
        }
    }

    /** Zwraca kod i tresc; nie rzuca przy 4xx/5xx (potrzebne do retry 401/403). */
    fun raw(
        url: String,
        method: String,
        body: String?,
        contentType: String = "application/json; charset=utf-8",
        headers: Map<String, String> = emptyMap(),
        maxBytes: Int = MAX_RTDB_BYTES,
        connectTimeoutMs: Int = 8000,
        readTimeoutMs: Int = 10000
    ): Pair<Int, String> {
        val c = URL(url).openConnection() as HttpURLConnection
        try {
            c.requestMethod = method
            c.connectTimeout = connectTimeoutMs
            c.readTimeout = readTimeoutMs
            c.instanceFollowRedirects = false
            c.useCaches = false
            c.setRequestProperty("Accept", "application/json, text/plain, */*")
            headers.forEach { (k, v) -> c.setRequestProperty(k, v) }
            if (body != null) {
                c.doOutput = true
                c.setRequestProperty("Content-Type", contentType)
                c.outputStream.use { it.write(body.toByteArray(Charsets.UTF_8)) }
            }
            val code = c.responseCode
            val declared = c.getHeaderFieldLong("Content-Length", -1L)
            val stream = if (code in 200..299) c.inputStream else c.errorStream
            if (declared > maxBytes.toLong()) {
                try { stream?.close() } catch (_: Exception) {}
                if (code in 200..299) throw ResponseTooLarge(maxBytes)
                return code to ""
            }
            val text = try {
                readBounded(stream, maxBytes)
            } catch (e: ResponseTooLarge) {
                if (code in 200..299) throw e
                ""
            }
            return code to text
        } finally {
            c.disconnect()
        }
    }

    /** Rzuca HttpError przy kodzie innym niz 2xx. */
    fun request(
        url: String,
        method: String,
        body: String?,
        contentType: String = "application/json; charset=utf-8",
        headers: Map<String, String> = emptyMap(),
        maxBytes: Int = MAX_RTDB_BYTES,
        connectTimeoutMs: Int = 8000,
        readTimeoutMs: Int = 10000
    ): String {
        val (code, text) = raw(
            url, method, body, contentType, headers, maxBytes, connectTimeoutMs, readTimeoutMs
        )
        if (code !in 200..299) throw HttpError(code, "HTTP $code: ${text.take(240)}")
        return text
    }
}
