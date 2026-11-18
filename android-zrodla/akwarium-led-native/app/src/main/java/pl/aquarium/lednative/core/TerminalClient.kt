package pl.aquarium.lednative.core

import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.Response
import okhttp3.WebSocket
import okhttp3.WebSocketListener
import java.util.concurrent.TimeUnit

/**
 * Natywny terminal szeregowy ESP32 po WebSocket (`ws://host:8080/ws`).
 * Firmware: AsyncWebSocket wsTerminal("/ws"), komendy tekstowe (status, adaptive,
 * minlux, logsize, save, serial/wifi/both/off...). Bez WebView.
 */
class TerminalClient(
    private val onLine: (String) -> Unit,
    private val onState: (Boolean, String) -> Unit
) {
    private val client = OkHttpClient.Builder()
        .connectTimeout(6, TimeUnit.SECONDS)
        .readTimeout(0, TimeUnit.MILLISECONDS)
        .pingInterval(20, TimeUnit.SECONDS)
        .build()

    @Volatile private var socket: WebSocket? = null
    @Volatile var connected: Boolean = false
        private set

    fun connect(wsUrl: String) {
        disconnect()
        onState(false, "Laczenie…")
        val request = Request.Builder().url(wsUrl).build()
        socket = client.newWebSocket(request, object : WebSocketListener() {
            override fun onOpen(webSocket: WebSocket, response: Response) {
                connected = true
                onState(true, "Polaczono")
            }

            override fun onMessage(webSocket: WebSocket, text: String) {
                text.split('\n').filter { it.isNotBlank() }.forEach { onLine(it.trimEnd('\r')) }
            }

            override fun onClosing(webSocket: WebSocket, code: Int, reason: String) {
                webSocket.close(1000, null)
                connected = false
                onState(false, "Rozlaczono")
            }

            override fun onClosed(webSocket: WebSocket, code: Int, reason: String) {
                connected = false
                onState(false, "Rozlaczono")
            }

            override fun onFailure(webSocket: WebSocket, t: Throwable, response: Response?) {
                connected = false
                onState(false, "Blad: ${t.message ?: "polaczenie nieudane"}")
            }
        })
    }

    fun send(command: String): Boolean {
        val ws = socket ?: return false
        if (!connected) return false
        return ws.send(command)
    }

    fun disconnect() {
        socket?.close(1000, "closed")
        socket = null
        connected = false
    }

    fun shutdown() {
        disconnect()
        client.dispatcher.executorService.shutdown()
        client.connectionPool.evictAll()
    }
}
