package pl.aquarium.lednative.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Cloud
import androidx.compose.material.icons.filled.Info
import androidx.compose.material.icons.filled.Key
import androidx.compose.material.icons.filled.Router
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import pl.aquarium.lednative.ui.components.SegmentedTabs
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import pl.aquarium.lednative.BuildConfig
import pl.aquarium.lednative.core.Logic
import pl.aquarium.lednative.ui.AppViewModel
import pl.aquarium.lednative.ui.Cyan
import pl.aquarium.lednative.ui.FsBlue
import pl.aquarium.lednative.ui.Green
import pl.aquarium.lednative.ui.Red
import pl.aquarium.lednative.ui.TextMid
import pl.aquarium.lednative.ui.Warm
import pl.aquarium.lednative.ui.components.CardHeader
import pl.aquarium.lednative.ui.components.GhostButton
import pl.aquarium.lednative.ui.components.InfoBanner
import pl.aquarium.lednative.ui.components.KeyValue
import pl.aquarium.lednative.ui.components.Pill
import pl.aquarium.lednative.ui.components.PrimaryButton
import pl.aquarium.lednative.ui.components.ScreenTitle
import pl.aquarium.lednative.ui.components.SectionCard
import pl.aquarium.lednative.ui.components.TextFieldStyled

@Composable
fun MoreScreen(vm: AppViewModel) {
    var email by remember { mutableStateOf(vm.prefs.email) }
    var password by remember { mutableStateOf("") }
    var dbUrl by remember { mutableStateOf(vm.prefs.dbUrl) }
    var apiKey by remember { mutableStateOf(vm.prefs.apiKey) }
    var host by remember { mutableStateOf(vm.prefs.manualHost) }
    var confirmRestart by remember { mutableStateOf(false) }
    var confirmLogout by remember { mutableStateOf(false) }
    var pane by remember { mutableStateOf("settings") }

    Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
        ScreenTitle("Więcej", "Połączenie, konto i diagnostyka") {
            Pill("v${BuildConfig.VERSION_NAME}", FsBlue)
        }
        SegmentedTabs(
            listOf("settings" to "Ustawienia", "terminal" to "Terminal", "charts" to "Wykresy", "logs" to "Logi"),
            pane
        ) { pane = it }

        if (pane == "settings") {
        SectionCard {
            CardHeader("CMD_TOKEN", Icons.Default.Key, if (vm.tokenReady) Green else Red) {
                Pill(if (vm.tokenReady) "OK" else "BRAK", if (vm.tokenReady) Green else Red)
            }
            KeyValue(
                "Token w APK",
                if (vm.tokenReady) "wstrzyknięty przy budowie" else "pusty — komendy odrzucane",
                if (vm.tokenReady) Green else Red
            )
            KeyValue("Długość tokenu", "${vm.prefs.commandToken.length} znaków")
            InfoBanner(
                "Token jest wkompilowany w APK ze secrets.h firmware i nie jest widoczny " +
                    "w interfejsie. Jeśli ESP odpowiada reason=invalid, przebuduj aplikację " +
                    "skryptem BUDUJ-APK.ps1 z aktualnym plikiem secrets.h.",
                warn = !vm.tokenReady
            )
        }

        SectionCard {
            CardHeader("Konto Firebase", Icons.Default.Cloud) {
                Pill(if (vm.signedIn) "ZALOGOWANY" else "WYLOGOWANY", if (vm.signedIn) Green else Warm)
            }
            TextFieldStyled("E-mail", email, { email = it }, hint = "Konto operatora z dostępem do bazy")
            TextFieldStyled(
                "Hasło", password, { password = it }, password = true,
                hint = "Hasło nie jest zapisywane — tylko token odświeżania (Android Keystore)"
            )
            TextFieldStyled("Adres bazy (RTDB)", dbUrl, { dbUrl = it }, hint = "https://…firebasedatabase.app")
            TextFieldStyled("Web API Key", apiKey, { apiKey = it }, hint = "Klucz publiczny projektu Firebase")
            PrimaryButton(
                if (vm.busy) "Logowanie…" else if (vm.signedIn) "Zaloguj ponownie" else "Zaloguj do Firebase",
                enabled = !vm.busy
            ) { vm.signIn(email, password, dbUrl, apiKey) }
            if (vm.signedIn) {
                GhostButton("Wyloguj", Modifier.fillMaxWidth(), color = Red) { confirmLogout = true }
            }
            KeyValue("Ścieżka urządzenia", Logic.DEVICE_ROOT)
        }

        SectionCard {
            CardHeader("Sieć lokalna (LAN)", Icons.Default.Router, Cyan) {
                Pill(if (vm.lanBase != null) "POŁĄCZONY" else "BRAK", if (vm.lanBase != null) Green else Warm)
            }
            KeyValue("Wykryty adres", vm.lanBase ?: "—")
            TextFieldStyled(
                "Adres ręczny ESP", host, { host = it },
                hint = "np. 192.168.1.50:8080 lub ryby-led-s3.local:8080"
            )
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                GhostButton("Zapisz adres", Modifier.weight(1f)) { vm.setManualHost(host.trim()) }
                GhostButton("Wyczyść LAN", Modifier.weight(1f), color = Red) { vm.forgetLan() }
            }
            PrimaryButton(
                if (vm.discovering) "Skanowanie sieci…" else "Wykryj ESP w sieci",
                enabled = !vm.discovering
            ) { vm.discoverLan() }
            InfoBanner(
                "LAN jest wymagany dla logów, plików LittleFS i terminala WebSocket. " +
                    "Telefon musi być w tej samej sieci WiFi co ESP32."
            )
        }

        SectionCard {
            CardHeader("Sterownik", Icons.Default.Settings, Warm)
            KeyValue("Firmware", vm.status?.fwVersion ?: "—")
            KeyValue("Źródło statusu", vm.source)
            KeyValue("Pamięć plików", vm.status?.fsUsed?.let { "$it B" } ?: "—")
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                GhostButton("Odśwież status", Modifier.weight(1f), enabled = !vm.refreshing) { vm.refresh() }
                GhostButton("Restart ESP", Modifier.weight(1f), color = Red) { confirmRestart = true }
            }
        }

        SectionCard {
            CardHeader("O aplikacji", Icons.Default.Info, FsBlue)
            KeyValue("Wersja", BuildConfig.VERSION_NAME)
            KeyValue("Pakiet", BuildConfig.APPLICATION_ID)
            KeyValue("Interfejs", "100% Jetpack Compose")
            KeyValue("WebView / HTML", "nie występuje")
            KeyValue("Terminal", "natywny WebSocket (OkHttp)")
            KeyValue("Wykresy i akwarium", "Compose Canvas")
            InfoBanner(
                "Aplikacja w pełni natywna: Kotlin + Jetpack Compose, bez WebView, bez HTML " +
                    "i bez mostka JavaScript. Wygląd kafelkowy odtworzony natywnymi komponentami."
            )
        }
        } else {
            when (pane) {
                "terminal" -> TerminalScreen(vm)
                "charts" -> ChartsScreen(vm)
                "logs" -> LogsScreen(vm)
                else -> Unit
            }
        }
    }

    if (confirmRestart) {
        AlertDialog(
            onDismissRequest = { confirmRestart = false },
            title = { Text("Zrestartować ESP32?") },
            text = { Text("Sterownik przerwie pracę na kilka sekund. Akwarium zostanie przywrócone zgodnie z harmonogramem.") },
            confirmButton = {
                TextButton(onClick = { confirmRestart = false; vm.restartDevice() }) {
                    Text("Restartuj", color = Red)
                }
            },
            dismissButton = {
                TextButton(onClick = { confirmRestart = false }) { Text("Anuluj", color = TextMid) }
            }
        )
    }
    if (confirmLogout) {
        AlertDialog(
            onDismissRequest = { confirmLogout = false },
            title = { Text("Wylogować z Firebase?") },
            text = { Text("Sterowanie chmurowe zostanie wyłączone do ponownego zalogowania.") },
            confirmButton = {
                TextButton(onClick = { confirmLogout = false; vm.signOut() }) {
                    Text("Wyloguj", color = Red)
                }
            },
            dismissButton = {
                TextButton(onClick = { confirmLogout = false }) { Text("Anuluj", color = TextMid) }
            }
        )
    }
}
