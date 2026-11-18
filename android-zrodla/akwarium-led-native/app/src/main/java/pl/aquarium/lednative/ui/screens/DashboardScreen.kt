package pl.aquarium.lednative.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.AutoMode
import androidx.compose.material.icons.filled.Bolt
import androidx.compose.material.icons.filled.DeviceThermostat
import androidx.compose.material.icons.filled.Lightbulb
import androidx.compose.material.icons.filled.PowerSettingsNew
import androidx.compose.material.icons.filled.RestartAlt
import androidx.compose.material.icons.filled.ShowChart
import androidx.compose.material.icons.filled.Thermostat
import androidx.compose.material.icons.filled.Waves
import androidx.compose.material.icons.filled.WbSunny
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import pl.aquarium.lednative.core.Logic
import pl.aquarium.lednative.ui.AppViewModel
import pl.aquarium.lednative.ui.Blue
import pl.aquarium.lednative.ui.Cyan
import pl.aquarium.lednative.ui.FsBlue
import pl.aquarium.lednative.ui.Green
import pl.aquarium.lednative.ui.Red
import pl.aquarium.lednative.ui.Warm
import pl.aquarium.lednative.ui.components.AquariumView
import pl.aquarium.lednative.ui.components.CardHeader
import pl.aquarium.lednative.ui.components.ControlTile
import pl.aquarium.lednative.ui.components.InfoBanner
import pl.aquarium.lednative.ui.components.KeyValue
import pl.aquarium.lednative.ui.components.Pill
import pl.aquarium.lednative.ui.components.QuickTile
import pl.aquarium.lednative.ui.components.ScreenTitle
import pl.aquarium.lednative.ui.components.SectionCard
import pl.aquarium.lednative.ui.components.StatTile

@Composable
fun DashboardScreen(vm: AppViewModel, onNavigate: (String) -> Unit) {
    val s = vm.status
    Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
        ScreenTitle("Pulpit", "Akwarium LED · sterowanie i podgląd") {
            Pill(
                when (vm.connection) {
                    Logic.Conn.ONLINE -> "ONLINE"
                    Logic.Conn.UNVERIFIED -> "SPRAWDZANIE"
                    Logic.Conn.STALE -> "BRAK ŚWIEŻYCH"
                    Logic.Conn.NONE -> "OFFLINE"
                },
                when (vm.connection) {
                    Logic.Conn.ONLINE -> Green
                    Logic.Conn.STALE -> Red
                    Logic.Conn.UNVERIFIED -> Warm
                    Logic.Conn.NONE -> Warm
                }
            )
        }

        if (!vm.tokenReady) {
            InfoBanner(
                "Ten APK nie ma wstrzykniętego CMD_TOKEN — komendy zostaną odrzucone " +
                    "jako reason=invalid. Zbuduj aplikację skryptem BUDUJ-APK.ps1.",
                warn = true
            )
        }
        if (!vm.signedIn && vm.lanBase == null) {
            InfoBanner(
                "Brak połączenia. Zaloguj się do Firebase lub wykryj ESP w sieci lokalnej " +
                    "(zakładka Więcej).",
                warn = true
            )
        }

        SectionCard { AquariumView(s) }

        Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
            ControlTile(
                label = "ŚWIATŁO",
                value = if (s?.power == true) "Włączone" else "Wyłączone",
                hint = if (s == null) "brak danych" else "${s.pwmPercent}% jasności",
                icon = Icons.Default.Lightbulb,
                active = s?.power == true,
                accent = Warm,
                enabled = !vm.busy,
                modifier = Modifier.weight(1f)
            ) { vm.togglePower() }
            ControlTile(
                label = "TRYB",
                value = if (s?.auto != false) "Automatyczny" else "Ręczny",
                hint = if (s?.auto != false) "harmonogram" else "PWM z aplikacji",
                icon = Icons.Default.AutoMode,
                active = s?.auto != false,
                accent = Cyan,
                enabled = !vm.busy,
                modifier = Modifier.weight(1f)
            ) { vm.toggleMode() }
        }

        ControlTile(
            label = "POMPKA / NAPOŁĄTRZACZ",
            value = if (s?.pumpOn == true) "Pracuje" else "Zatrzymana",
            hint = s?.pumpSlots?.let {
                if (it.isEmpty()) "brak okien czasowych" else "${it.size} okien w harmonogramie"
            } ?: "brak danych",
            icon = Icons.Default.Waves,
            active = s?.pumpOn == true,
            accent = Blue,
            enabled = !vm.busy,
            modifier = Modifier.fillMaxWidth()
        ) { vm.togglePump() }

        Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
            StatTile(
                "TEMP. WODY", Logic.fmtTemp(s?.tempWater) + "°C", "sonda DS18B20",
                Icons.Default.Thermostat, Cyan, Modifier.weight(1f)
            )
            StatTile(
                "MOC TERAZ", Logic.fmtWatts(s?.powerNowW) + " W",
                s?.powerLimitW?.let { "limit ${Logic.fmtWatts(it)} W" } ?: "pomiar z ESP",
                Icons.Default.Bolt, Warm, Modifier.weight(1f)
            )
        }
        Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
            StatTile(
                "LUX NAD WODĄ", Logic.fmtLux(s?.luxWater),
                if (s?.minLuxActive == true) "min-lux aktywny" else "czujnik BH1750",
                Icons.Default.WbSunny, FsBlue, Modifier.weight(1f)
            )
            StatTile(
                "ENERGIA DZIŚ", Logic.fmtWh(s?.energyTodayWh),
                "koszt ${Logic.costPln(s?.energyTodayWh, s?.kwhPrice)} zł",
                Icons.Default.Bolt, Green, Modifier.weight(1f)
            )
        }

        SectionCard {
            CardHeader("Szybkie akcje", Icons.Default.PowerSettingsNew)
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                QuickTile("LED 100%", Icons.Default.WbSunny, Modifier.weight(1f), Warm) { vm.ledFull() }
                QuickTile("LED OFF", Icons.Default.Lightbulb, Modifier.weight(1f)) { vm.ledOff() }
                QuickTile("Wykresy", Icons.Default.ShowChart, Modifier.weight(1f), FsBlue) { onNavigate("charts") }
                QuickTile("Restart", Icons.Default.RestartAlt, Modifier.weight(1f), Red) { vm.restartDevice() }
            }
        }

        SectionCard {
            CardHeader("Stan sterownika", Icons.Default.DeviceThermostat, FsBlue)
            KeyValue("Firmware", s?.fwVersion ?: "—")
            KeyValue("Źródło danych", vm.source)
            KeyValue("Ostatni odczyt", vm.lastSync ?: "—")
            KeyValue("Adres IP", s?.ip ?: vm.lanBase?.removePrefix("http://") ?: "—")
            KeyValue("Czas ESP", s?.localTime ?: "—", if (s?.ntpOk == true) Green else Warm)
            KeyValue("Uptime", Logic.fmtUptime(s?.uptimeSec))
            KeyValue("WiFi RSSI", s?.wifiRssi?.let { "$it dBm" } ?: "—")
            KeyValue("Okno świetlne", when (s?.inLightingWindow) {
                true -> "aktywne"; false -> "poza oknem"; null -> "—"
            })
            KeyValue("Rampa", if (s?.rampActive == true) "w trakcie" else "bezczynna")
            KeyValue(
                "CMD_TOKEN w APK",
                if (vm.tokenReady) "wstrzyknięty" else "BRAK",
                if (vm.tokenReady) Green else Red
            )
        }
    }
}
