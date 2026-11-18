package pl.aquarium.lednative.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Sensors
import androidx.compose.material.icons.filled.Thermostat
import androidx.compose.material.icons.filled.Tune
import androidx.compose.material.icons.filled.WbSunny
import androidx.compose.material3.Switch
import androidx.compose.material3.SwitchDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import pl.aquarium.lednative.core.DeviceSettings
import pl.aquarium.lednative.core.Logic
import pl.aquarium.lednative.ui.AppViewModel
import pl.aquarium.lednative.ui.Border
import pl.aquarium.lednative.ui.Cyan
import pl.aquarium.lednative.ui.FsBlue
import pl.aquarium.lednative.ui.Green
import pl.aquarium.lednative.ui.Surface3
import pl.aquarium.lednative.ui.TextDim
import pl.aquarium.lednative.ui.Warm
import pl.aquarium.lednative.ui.components.CardHeader
import pl.aquarium.lednative.ui.components.InfoBanner
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

        Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
            StatTile(
                "PLYTA 1", Logic.fmtTemp(s?.tempPlate1) + "°C", "radiator LED",
                Icons.Default.Thermostat, Warm, Modifier.weight(1f)
            )
            StatTile(
                "PLYTA 2", Logic.fmtTemp(s?.tempPlate2) + "°C", "radiator LED",
                Icons.Default.Thermostat, Warm, Modifier.weight(1f)
            )
        }
        Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
            StatTile(
                "LUX POKOJOWY", Logic.fmtLux(s?.luxRoom), "BH1750 #1",
                Icons.Default.WbSunny, FsBlue, Modifier.weight(1f)
            )
            StatTile(
                "LUX NAD WODĄ", Logic.fmtLux(s?.luxWater), "BH1750 #2",
                Icons.Default.WbSunny, Cyan, Modifier.weight(1f)
            )
        }

        SectionCard {
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
