package pl.aquarium.lednative.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.clickable
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.ui.draw.clip
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Sensors
import androidx.compose.material.icons.filled.Thermostat
import androidx.compose.material.icons.filled.Tune
import androidx.compose.material.icons.filled.WbSunny
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.SwitchDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.Alignment
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import pl.aquarium.lednative.core.DeviceSettings
import pl.aquarium.lednative.core.Logic
import pl.aquarium.lednative.ui.AppViewModel
import pl.aquarium.lednative.ui.Border
import pl.aquarium.lednative.ui.Cyan
import pl.aquarium.lednative.ui.FsBlue
import pl.aquarium.lednative.ui.Green
import pl.aquarium.lednative.ui.Surface3
import pl.aquarium.lednative.ui.TextDim
import pl.aquarium.lednative.ui.TextHi
import pl.aquarium.lednative.ui.Warm
import pl.aquarium.lednative.ui.components.CardHeader
import pl.aquarium.lednative.ui.components.InfoBanner
import pl.aquarium.lednative.ui.components.LineChart
import pl.aquarium.lednative.ui.components.NumberField
import pl.aquarium.lednative.ui.components.Pill
import pl.aquarium.lednative.ui.components.PrimaryButton
import pl.aquarium.lednative.ui.components.ScreenTitle
import pl.aquarium.lednative.ui.components.SectionCard
import pl.aquarium.lednative.ui.components.SegmentedTabs
import pl.aquarium.lednative.ui.components.SettingRow
import pl.aquarium.lednative.ui.components.StatTile

@Composable
fun SensorsScreen(vm: AppViewModel) {
    val s = vm.status
    var cfg by remember { mutableStateOf(DeviceSettings()) }
    var ema by remember { mutableStateOf("0.25") }
    var sensInt by remember { mutableStateOf("30") }
    var ramp by remember { mutableStateOf("30") }
    var fade by remember { mutableStateOf("30") }
    var luxTarget by remember { mutableStateOf("2000") }
    var luxInterval by remember { mutableStateOf("30") }
    var loaded by remember { mutableStateOf(false) }
    var settingsExpanded by remember { mutableStateOf(false) }

    LaunchedEffect(s) {
        if (!loaded && s != null) {
            cfg = DeviceSettings.from(s)
            ema = s.emaFilter.toString()
            sensInt = s.sensInt.toString()
            ramp = s.rampSec.toString()
            fade = s.fadeMinutes.toString()
            luxTarget = s.minLuxTarget.toString()
            luxInterval = s.minLuxInterval.toString()
            loaded = true
        }
    }

    val switchColors = SwitchDefaults.colors(
        checkedThumbColor = Cyan, checkedTrackColor = Cyan.copy(alpha = 0.35f),
        uncheckedThumbColor = TextDim, uncheckedTrackColor = Surface3,
        uncheckedBorderColor = Border
    )

    Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
        ScreenTitle("Czujniki", "Temperatura, światło i adaptacja") {
            Pill(when (cfg.sensorMode) { 0 -> "POKÓJ"; 1 -> "WODA"; else -> "OBA" }, FsBlue)
        }

        SectionCard {
            CardHeader("Temperatury", Icons.Default.Thermostat, Warm) {
                Text("aktualizacja teraz", fontSize = 7.sp, color = TextDim)
            }
            Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                StatTile(
                    "PŁYTA LED 1", Logic.fmtTemp(s?.tempPlate1) + "°C", "radiator",
                    Icons.Default.Thermostat, Warm, Modifier.weight(1f)
                )
                StatTile(
                    "PŁYTA LED 2", Logic.fmtTemp(s?.tempPlate2) + "°C", "radiator",
                    Icons.Default.Thermostat, Warm, Modifier.weight(1f)
                )
                StatTile(
                    "WODA", Logic.fmtTemp(s?.tempWater) + "°C", "akwarium",
                    Icons.Default.Thermostat, FsBlue, Modifier.weight(1f)
                )
            }
        }
        val ledLux = if (s?.power == true) (850.0 * (s.pwmPercent / 64.0)).toInt() else 0
        val sunLux = s?.luxRoom?.toInt() ?: 0
        val luxScale = (s?.minLuxTarget ?: 2000).coerceAtLeast(500)
        val sunShare = (sunLux.toFloat() / luxScale).coerceIn(0f, 1f)
        val ledShare = (ledLux.toFloat() / luxScale).coerceIn(0f, 1f - sunShare)
        SectionCard {
            CardHeader("Natężenie światła", Icons.Default.WbSunny, Cyan) { Pill("TSL2561", Green) }
            Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                StatTile(
                    "SŁOŃCE", Logic.fmtLux(s?.luxRoom), "przy roślinach",
                    Icons.Default.WbSunny, Warm, Modifier.weight(1f)
                )
                StatTile(
                    "DOŚWIETL.", if (s == null) "—" else "$ledLux lx", "LED · ${s?.pwmPercent ?: 0}%",
                    Icons.Default.WbSunny, Cyan, Modifier.weight(1f)
                )
                StatTile(
                    "NAD WODĄ", Logic.fmtLux(s?.luxWater), "czujnik TSL2561",
                    Icons.Default.WbSunny, FsBlue, Modifier.weight(1f)
                )
            }
            Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.SpaceBetween) {
                Text("ŚWIATŁO ŁĄCZNIE PRZY ROŚLINACH", fontSize = 6.sp, color = TextDim)
                Text("${sunLux + ledLux} / $luxScale lx", fontSize = 7.sp, color = TextHi)
            }
            Row(Modifier.fillMaxWidth().height(6.dp).clip(RoundedCornerShape(99.dp)).background(Surface3)) {
                if (sunShare > 0f) Box(Modifier.weight(sunShare).fillMaxWidth().height(6.dp).background(Warm))
                if (ledShare > 0f) Box(Modifier.weight(ledShare).fillMaxWidth().height(6.dp).background(Cyan))
            }
            Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
                Text("☀ SŁOŃCE", fontSize = 6.sp, color = TextDim)
                Text("💡 LED", fontSize = 6.sp, color = TextDim)
                Text("SKALA 0–$luxScale lx", fontSize = 6.sp, color = TextDim)
            }
        }
        SectionCard {
            CardHeader("Historia pomiarów", Icons.Default.Tune, Cyan) {
                Text("12 h", fontSize = 7.sp, color = TextDim)
            }
            LineChart(vm.samples.mapNotNull { it.tempWater }, Cyan, unit = "°C")
            Row(horizontalArrangement = Arrangement.SpaceBetween, modifier = Modifier.fillMaxWidth()) {
                Text("— PŁYTA LED", fontSize = 6.sp, color = Warm)
                Text("— WODA", fontSize = 6.sp, color = FsBlue)
                Text("— LUX", fontSize = 6.sp, color = Cyan)
            }
        }
        SectionCard {
            Row(
                Modifier.fillMaxWidth().clickable { settingsExpanded = !settingsExpanded },
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text("Ustawienia czujników i adaptacji", fontSize = 9.sp, color = TextHi, modifier = Modifier.weight(1f))
                Text(if (settingsExpanded) "▲" else "▼", fontSize = 8.sp, color = TextDim)
            }
        }
        if (settingsExpanded) SectionCard {
            CardHeader("Źródło adaptacji", Icons.Default.Sensors, FsBlue)
            SegmentedTabs(
                listOf("0" to "Pokojowy", "1" to "Nad wodą", "2" to "Oba"),
                cfg.sensorMode.toString()
            ) { cfg = cfg.copy(sensorMode = it.toInt()) }
            SettingRow("Adaptacja jasności", "Dostosowuje LED do światła w otoczeniu") {
                Switch(
                    checked = cfg.adaptEnabled,
                    onCheckedChange = { cfg = cfg.copy(adaptEnabled = it) },
                    colors = switchColors
                )
            }
            SettingRow("Tryb uczenia", "ESP zapamiętuje reakcję akwarium") {
                Switch(
                    checked = cfg.learningEnabled,
                    onCheckedChange = { cfg = cfg.copy(learningEnabled = it) },
                    colors = switchColors
                )
            }
        }

        if (settingsExpanded) {
        SectionCard {
            CardHeader("Minimalne światło (min-lux)", Icons.Default.WbSunny, Green) {
                if (s?.minLuxActive == true) Pill("AKTYWNY", Green)
            }
            SettingRow("Doświetlanie przy braku światła", "Utrzymuje zadany poziom lux") {
                Switch(
                    checked = cfg.minLuxEnabled,
                    onCheckedChange = { cfg = cfg.copy(minLuxEnabled = it) },
                    colors = switchColors
                )
            }
            Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                NumberField(
                    "Cel (lux)", luxTarget, { luxTarget = it }, Modifier.weight(1f),
                    hint = "500–8000 lx"
                )
                NumberField(
                    "Interwał (s)", luxInterval, { luxInterval = it }, Modifier.weight(1f),
                    hint = "5–300 s"
                )
            }
        }

        SectionCard {
            CardHeader("Parametry sterownika", Icons.Default.Tune)
            Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                NumberField(
                    "Filtr EMA", ema, { ema = it }, Modifier.weight(1f),
                    hint = "0.05–0.50", decimal = true
                )
                NumberField(
                    "Odczyt czujników (s)", sensInt, { sensInt = it }, Modifier.weight(1f),
                    hint = "5–300 s"
                )
            }
            Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                NumberField(
                    "Rampa PWM (s)", ramp, { ramp = it }, Modifier.weight(1f),
                    hint = "1–600 s"
                )
                NumberField(
                    "Fade (min)", fade, { fade = it }, Modifier.weight(1f),
                    hint = "1–180 min"
                )
            }
            PrimaryButton(
                if (vm.busy) "Wysyłanie…" else "Zapisz ustawienia czujników",
                enabled = !vm.busy && vm.tokenReady
            ) {
                val e = ema.toDoubleOrNull()
                val si = sensInt.toIntOrNull()
                val r = ramp.toIntOrNull()
                val f = fade.toIntOrNull()
                val lt = luxTarget.toIntOrNull()
                val li = luxInterval.toIntOrNull()
                when {
                    e == null || e !in 0.05..0.5 -> vm.notify("Filtr EMA musi być w zakresie 0.05–0.50")
                    si == null || si !in 5..300 -> vm.notify("Interwał czujników: 5–300 s")
                    r == null || r !in 1..600 -> vm.notify("Rampa PWM: 1–600 s")
                    f == null || f !in 1..180 -> vm.notify("Fade: 1–180 min")
                    lt == null || lt !in 500..8000 -> vm.notify("Cel lux: 500–8000")
                    li == null || li !in 5..300 -> vm.notify("Interwał min-lux: 5–300 s")
                    else -> vm.saveSettings(
                        cfg.copy(
                            emaFilter = e, sensInt = si, rampSec = r, fadeMinutes = f,
                            minLuxTarget = lt, minLuxInterval = li
                        )
                    )
                }
            }
        }
        InfoBanner(
            "Ustawienia są wysyłane jako config do Firebase (adapt / minlux / params / schedule) " +
                "i potwierdzane komendą config_refresh. Bez LAN nie ma podglądu logów."
        )
        }
    }
}
