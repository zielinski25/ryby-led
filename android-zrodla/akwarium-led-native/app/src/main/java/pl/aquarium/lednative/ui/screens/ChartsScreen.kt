package pl.aquarium.lednative.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Bolt
import androidx.compose.material.icons.filled.ShowChart
import androidx.compose.material.icons.filled.Thermostat
import androidx.compose.material.icons.filled.WbSunny
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import pl.aquarium.lednative.core.Logic
import pl.aquarium.lednative.ui.AppViewModel
import pl.aquarium.lednative.ui.Cyan
import pl.aquarium.lednative.ui.FsBlue
import pl.aquarium.lednative.ui.Green
import pl.aquarium.lednative.ui.Warm
import pl.aquarium.lednative.ui.TextDim
import pl.aquarium.lednative.ui.components.BarChart
import pl.aquarium.lednative.ui.components.CardHeader
import pl.aquarium.lednative.ui.components.ChannelBars
import pl.aquarium.lednative.ui.components.GhostButton
import pl.aquarium.lednative.ui.components.InfoBanner
import pl.aquarium.lednative.ui.components.KeyValue
import pl.aquarium.lednative.ui.components.LineChart
import pl.aquarium.lednative.ui.components.Pill
import pl.aquarium.lednative.ui.components.ScreenTitle
import pl.aquarium.lednative.ui.components.SectionCard
import pl.aquarium.lednative.ui.components.SegmentedTabs
import pl.aquarium.lednative.ui.components.StatTile

@Composable
fun ChartsScreen(vm: AppViewModel) {
    val s = vm.status
    var tab by remember { mutableStateOf("live") }
    val samples = vm.samples.toList()

    Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
        ScreenTitle("Wykresy", "Energia, jasność i temperatura") {
            Pill("${samples.size} próbek", Cyan)
        }
        SegmentedTabs(
            listOf("live" to "Na żywo", "energy" to "Energia", "channels" to "Kanały"),
            tab
        ) { tab = it }

        when (tab) {
            "live" -> {
                SectionCard {
                    CardHeader("Wykresy", Icons.Default.ShowChart, Cyan) {
                        Text("ostatnie 24 h", fontSize = 7.sp, color = TextDim)
                    }
                    Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                        StatTile(
                            "PŁYTA LED 1", Logic.fmtTemp(s?.tempPlate1) + "°C", "",
                            Icons.Default.Thermostat, Warm, Modifier.weight(1f)
                        )
                        StatTile(
                            "PŁYTA LED 2", Logic.fmtTemp(s?.tempPlate2) + "°C", "",
                            Icons.Default.Thermostat, Warm, Modifier.weight(1f)
                        )
                        StatTile(
                            "WODA", Logic.fmtTemp(s?.tempWater) + "°C", "",
                            Icons.Default.Thermostat, FsBlue, Modifier.weight(1f)
                        )
                    }
                    Text("JASNOŚĆ PWM", fontSize = 6.sp, color = TextDim)
                    LineChart(
                        samples.map { it.pwm.average() * 100.0 / Logic.PWM_MAX },
                        Warm, unit = "%", heightDp = 78
                    )
                    Text("TEMPERATURA WODY", fontSize = 6.sp, color = TextDim)
                    LineChart(samples.mapNotNull { it.tempWater }, Cyan, unit = "°C", heightDp = 78)
                }
            }
            "energy" -> {
                Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                    StatTile(
                        "DZIŚ", Logic.fmtWh(s?.energyTodayWh),
                        "${Logic.costPln(s?.energyTodayWh, s?.kwhPrice)} zł",
                        Icons.Default.Bolt, Green, Modifier.weight(1f)
                    )
                    StatTile(
                        "TYDZIEŃ", Logic.fmtWh(s?.energyWeekWh),
                        "${Logic.costPln(s?.energyWeekWh, s?.kwhPrice)} zł",
                        Icons.Default.Bolt, Cyan, Modifier.weight(1f)
                    )
                }
                Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                    StatTile(
                        "MIESIĄC", Logic.fmtWh(s?.energyMonthWh),
                        "${Logic.costPln(s?.energyMonthWh, s?.kwhPrice)} zł",
                        Icons.Default.Bolt, FsBlue, Modifier.weight(1f)
                    )
                    StatTile(
                        "CZAS ŚWIECENIA", Logic.fmtMinutes(s?.ledOnMinutesToday),
                        "dziś", Icons.Default.WbSunny, Warm, Modifier.weight(1f)
                    )
                }
                SectionCard {
                    CardHeader("Energia godzinowa (Wh)", Icons.Default.Bolt, Green)
                    BarChart(
                        s?.hourPowerWh ?: emptyList(), Green,
                        labels = listOf("00:00", "teraz")
                    )
                }
                SectionCard {
                    CardHeader("Historia dni (Wh)", Icons.Default.ShowChart, Cyan)
                    BarChart(
                        s?.dayHistory ?: emptyList(), Cyan,
                        labels = listOf("wcześniej", "dziś")
                    )
                }
                SectionCard {
                    CardHeader("Podsumowanie", Icons.Default.Bolt)
                    KeyValue("Cena za kWh", s?.kwhPrice?.let { "$it zł" } ?: "—")
                    KeyValue("Limit mocy", s?.powerLimitW?.let { "${Logic.fmtWatts(it)} W" } ?: "—")
                    KeyValue("Moc teraz", "${Logic.fmtWatts(s?.powerNowW)} W")
                    GhostButton("Odśwież dane", Modifier.fillMaxWidth()) { vm.refresh() }
                }
                if (s?.hasEnergy != true) {
                    InfoBanner(
                        "Firmware nie raportuje jeszcze liczników energii — kafelki pokazują „—”. " +
                            "To nie jest błąd aplikacji.",
                        warn = true
                    )
                }
            }
            else -> {
                SectionCard {
                    CardHeader("Moc na kanałach (W)", Icons.Default.Bolt, Warm)
                    if (s != null && s.powerNowCh.isNotEmpty()) ChannelBars(s.powerNowCh)
                    else InfoBanner("ESP nie raportuje mocy per kanał w tym odczycie.", warn = true)
                }
                SectionCard {
                    CardHeader("Wypełnienie PWM", Icons.Default.ShowChart, Cyan)
                    BarChart(
                        (s?.pwm ?: emptyList()).map { it * 100.0 / Logic.PWM_MAX },
                        Cyan, labels = listOf("Białe", "Czerwone")
                    )
                    KeyValue("Średnie wypełnienie", "${s?.pwmPercent ?: 0}%")
                }
            }
        }
    }
}
