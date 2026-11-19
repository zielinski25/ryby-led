package pl.aquarium.lednative.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Article
import androidx.compose.material.icons.filled.AutoMode
import androidx.compose.material.icons.filled.Bolt
import androidx.compose.material.icons.filled.DeviceThermostat
import androidx.compose.material.icons.filled.Dashboard
import androidx.compose.material.icons.filled.Lightbulb
import androidx.compose.material.icons.filled.Schedule
import androidx.compose.material.icons.filled.Sensors
import androidx.compose.material.icons.filled.Waves
import androidx.compose.material.icons.filled.WbSunny
import androidx.compose.material3.Checkbox
import androidx.compose.material3.Icon
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlin.math.roundToInt
import pl.aquarium.lednative.core.DeviceSettings
import pl.aquarium.lednative.core.DeviceStatus
import pl.aquarium.lednative.core.Logic
import pl.aquarium.lednative.ui.AppViewModel
import pl.aquarium.lednative.ui.Border
import pl.aquarium.lednative.ui.Cyan
import pl.aquarium.lednative.ui.FsBlue
import pl.aquarium.lednative.ui.Green
import pl.aquarium.lednative.ui.TextDim
import pl.aquarium.lednative.ui.TextHi
import pl.aquarium.lednative.ui.TextMid
import pl.aquarium.lednative.ui.Warm
import pl.aquarium.lednative.ui.components.AquariumView
import pl.aquarium.lednative.ui.components.ControlTile
import pl.aquarium.lednative.ui.components.LineChart
import pl.aquarium.lednative.ui.components.Pill
import pl.aquarium.lednative.ui.components.PrimaryButton
import pl.aquarium.lednative.ui.components.PwmSlider
import pl.aquarium.lednative.ui.components.QuickTile
import pl.aquarium.lednative.ui.components.SectionCard
import pl.aquarium.lednative.ui.components.SegmentedTabs

@Composable
private fun ModeSwitch(onClick: () -> Unit) {
    // HTML .modeSwitch: padding 5px 6px, border #00d4f544, radius 7px, bg #0a2735, color #a1e8ef
    Box(
        Modifier
            .clickable(onClick = onClick)
            .padding(horizontal = 6.dp, vertical = 5.dp)
            .background(Color(0xFF0A2735))
            .border(1.dp, Color(0xFF00D4F5).copy(alpha = 0.27f), RoundedCornerShape(7.dp))
    ) {
        Text(
            "Zmień ↻", fontSize = 6.sp, color = Color(0xFFA1E8EF),
            fontWeight = FontWeight.Bold, fontFamily = FontFamily.Monospace
        )
    }
}

@Composable
private fun LightTile(
    label: String, value: String, sub: String, icon: ImageVector, accent: Color,
    modifier: Modifier = Modifier
) {
    // HTML .lightTile w .lightGrid
    Box(
        modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(9.dp))
            .background(Color(0xFF0B1726))
            .border(1.dp, Color(0xFFFFFFFF).copy(alpha = 0.07f), RoundedCornerShape(9.dp))
            .padding(horizontal = 9.dp, vertical = 10.dp)
    ) {
        Column(
            Modifier.fillMaxWidth(),
            verticalArrangement = Arrangement.spacedBy(5.dp)
        ) {
            Row(
                Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Icon(icon, null, tint = accent, modifier = Modifier.size(13.dp))
                Text(label, fontSize = 6.sp, color = TextDim, fontWeight = FontWeight.Medium, fontFamily = FontFamily.Monospace)
            }
            Text(value, fontSize = 12.sp, fontWeight = FontWeight.Bold, color = accent, fontFamily = FontFamily.Monospace, maxLines = 1)
            Text(sub, fontSize = 7.sp, color = TextMid, maxLines = 2)
        }
    }
}

/** Jeden panel energii (HTML .energyStats>div). */
@Composable
private fun EnergyStat(label: String, value: String, unit: String, modifier: Modifier = Modifier) {
    Column(modifier) {
        Text(label, fontSize = 6.sp, color = TextDim, fontWeight = FontWeight.Bold, fontFamily = FontFamily.Monospace)
        Text(value, fontSize = 12.sp, fontWeight = FontWeight.Bold, color = TextHi, maxLines = 1)
        Text(unit, fontSize = 8.sp, color = TextMid, fontStyle = androidx.compose.ui.text.font.FontStyle.Italic)
    }
}

@Composable
fun DashboardScreen(vm: AppViewModel, onNavigate: (String) -> Unit) {
    val s = vm.status
    val power = s?.power ?: false
    val pumpOn = s?.pumpOn ?: false
    val auto = s?.auto ?: true
    val adapt = s?.adaptEnabled ?: false
    val pwm = s?.pwm ?: List(Logic.PWM_CHANNELS) { 0 }
    val pwmPercent = s?.pwmPercent ?: 0
    val pwmAvg = s?.pwmAverage ?: 0

    // Lokalna kopia edytowalnych ustawień (zapisywana dopiero po "Zapisz", jak w makiecie HTML).
    // Inicjalizacja raz przy pierwszym złożeniu — kolejne odczyty z ESP nie nadpisują edycji.
    var settings by remember { mutableStateOf(DeviceSettings.from(s ?: DeviceStatus())) }
    // Lokalna kopia wartości PWM (suwaki działają na podglądzie i wysyłają zmiany na żywo).
    var pwmValues by remember { mutableStateOf(pwm.toMutableList()) }
    var pwmExpanded by remember { mutableStateOf(false) }

    Column(modifier = Modifier.fillMaxWidth(), verticalArrangement = Arrangement.spacedBy(10.dp)) {
        // HTML home() zaczyna sie bez pageHead i banerow; pierwsza jest karta zbiornika.
        // Karta zbiornika — HTML section.card > cardHead "Podgląd zbiornika" + 2 pill + tankVisual + blendLabels
        SectionCard {
            Row(
                modifier = Modifier.fillMaxWidth(),
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text(
                    "Podgląd zbiornika", fontSize = 10.sp, fontWeight = FontWeight.Bold, color = TextHi
                )
                Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                    Pill(if (power) "LED AKTYWNE" else "LED WYŁ.", if (power) Green else Warm)
                    Pill(if (pumpOn) "POMPKA WŁ." else "POMPKA WYŁ.", if (pumpOn) Green else Warm)
                }
            }
            AquariumView(s)
            Text(
                "JASNOŚĆ LAMPY $pwmPercent% · PWM $pwmAvg/1023",
                fontSize = 8.sp, color = TextDim, fontWeight = FontWeight.Medium,
                fontFamily = FontFamily.Monospace
            )
        }

        // Primary controls — HTML .primaryControls: repeat(2, minmax(0,1fr)), gap 8px
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            ControlTile(
                label = "OŚWIETLENIE LED",
                value = if (power) "WŁĄCZONE" else "WYŁĄCZONE",
                hint = if (s == null) "brak danych" else "${s.pwmPercent}% jasności",
                icon = Icons.Default.Bolt,
                active = power,
                accent = Warm,
                enabled = !vm.busy,
                modifier = Modifier.weight(1f),
                onClick = { vm.togglePower() }
            )
            ControlTile(
                label = "POMPA / FILTR",
                value = if (pumpOn) "WŁĄCZONA" else "WYŁĄCZONA",
                hint = s?.pumpSlots?.let {
                    if (it.isEmpty()) "brak okien czasowych" else "${it.size} okien w harmonogramie"
                } ?: "brak danych",
                icon = Icons.Default.Waves,
                active = pumpOn,
                accent = Cyan,
                enabled = !vm.busy,
                modifier = Modifier.weight(1f),
                onClick = { vm.togglePump() }
            )
        }
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            ControlTile(
                label = "TRYB PRACY",
                value = if (auto) "AUTO" else "MANUAL",
                hint = if (auto) "harmonogram" else "stałe PWM",
                icon = Icons.Default.AutoMode,
                active = auto,
                accent = Cyan,
                enabled = !vm.busy,
                modifier = Modifier.weight(1f),
                trailing = { ModeSwitch { vm.toggleMode() } },
                onClick = { vm.toggleMode() }
            )
            val adaptHint = when (s?.sensorMode) {
                1 -> "Pokój"
                2 -> "Pokój + woda"
                else -> "Czujniki wył."
            }
            ControlTile(
                label = "ADAPTACJA",
                value = if (adapt) "AKTYWNA" else "WYŁĄCZONA",
                hint = adaptHint,
                icon = Icons.Default.Sensors,
                active = adapt,
                accent = Cyan,
                enabled = !vm.busy,
                modifier = Modifier.weight(1f),
                onClick = {
                    val updated = settings.copy(adaptEnabled = !adapt)
                    settings = updated
                    vm.saveSettings(updated)
                }
            )
        }

        // Ustawienia czujników i MIN LUX — HTML <details> "Ustawienia czujników i MIN LUX"
        SectionCard {
            var expanded by remember { mutableStateOf(false) }
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .clickable { expanded = !expanded },
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Icon(Icons.Default.DeviceThermostat, null, tint = Cyan, modifier = Modifier.size(16.dp))
                Text(
                    "Ustawienia czujników i MIN LUX", fontSize = 10.sp, fontWeight = FontWeight.Bold,
                    color = TextHi, modifier = Modifier.weight(1f)
                )
                Text(if (expanded) "▲" else "▼", fontSize = 10.sp, color = TextDim, fontFamily = FontFamily.Monospace)
            }
            if (expanded) {
                Column(modifier = Modifier.fillMaxWidth(), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
                        Column(Modifier.weight(1f)) {
                            Text("Źródło czujnika", fontSize = 8.sp, color = TextDim, fontWeight = FontWeight.Bold, fontFamily = FontFamily.Monospace)
                            Text("Wybierz używane pomiary", fontSize = 6.sp, color = TextMid)
                        }
                        SegmentedTabs(
                            options = listOf("0" to "Wyłączone", "1" to "Pokój", "2" to "Pokój + woda"),
                            selected = settings.sensorMode.toString(),
                            modifier = Modifier.width(160.dp),
                            onSelect = { selected -> settings = settings.copy(sensorMode = selected.toInt()) }
                        )
                    }
                    Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
                        Column(Modifier.weight(1f)) {
                            Text("Filtr EMA", fontSize = 8.sp, color = TextDim, fontWeight = FontWeight.Bold, fontFamily = FontFamily.Monospace)
                            Text("0.05 wolniej · 0.50 szybciej", fontSize = 6.sp, color = TextMid)
                        }
                        Text("${settings.emaFilter.toString()}", fontSize = 12.sp, fontWeight = FontWeight.Bold, color = TextHi, fontFamily = FontFamily.Monospace)
                    }
                    Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
                        Column(Modifier.weight(1f)) {
                            Text("Cel MIN. LUX", fontSize = 8.sp, color = TextDim, fontWeight = FontWeight.Bold, fontFamily = FontFamily.Monospace)
                            Text("Utrzymywany poziom światła", fontSize = 6.sp, color = TextMid)
                        }
                        Text("${settings.minLuxTarget} lx", fontSize = 12.sp, fontWeight = FontWeight.Bold, color = TextHi, fontFamily = FontFamily.Monospace)
                    }
                    Row(
                        modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Column(Modifier.weight(1f)) {
                            Text("MIN. LUX aktywne", fontSize = 8.sp, color = TextDim, fontWeight = FontWeight.Bold, fontFamily = FontFamily.Monospace)
                            Text("Uzupełnianie światła poza fazą", fontSize = 6.sp, color = TextMid)
                        }
                        Checkbox(
                            checked = settings.minLuxEnabled,
                            onCheckedChange = { settings = settings.copy(minLuxEnabled = it) },
                            colors = androidx.compose.material3.CheckboxDefaults.colors(
                                checkedColor = Cyan, uncheckedColor = TextDim
                            )
                        )
                    }
                    PrimaryButton("Zapisz", enabled = !vm.busy, modifier = Modifier.fillMaxWidth(), onClick = {
                        vm.saveSettings(settings)
                    })
                }
            }
        }

        // Regulacja mocy LED — HTML pwmControl: master suwak + kanały + Zapisz
        SectionCard {
            Column(modifier = Modifier.fillMaxWidth(), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Text("Regulacja mocy LED", fontSize = 10.sp, fontWeight = FontWeight.Bold, color = TextHi)
                    Pill("0–1023", Cyan)
                }
                Box(
                    Modifier
                        .fillMaxWidth()
                        .padding(8.dp, 9.dp, 8.dp, 7.dp)
                        .background(Color(0xFF0A1727))
                        .border(1.dp, Color(0xFF13CCE3).copy(alpha = 0.20f), RoundedCornerShape(9.dp))
                ) {
                    Column(Modifier.fillMaxWidth(), verticalArrangement = Arrangement.spacedBy(6.dp)) {
                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.SpaceBetween
                        ) {
                            Column(Modifier.weight(1f)) {
                                Text("Moc wspólna · wszystkie kanały", fontSize = 8.sp, color = TextHi, fontWeight = FontWeight.Bold)
                                Text("${pwmValues.sum() / pwmValues.size} / 1023", fontSize = 11.sp, fontWeight = FontWeight.Bold, color = Cyan, fontFamily = FontFamily.Monospace)
                            }
                        }
                        Slider(
                            value = (pwmValues.sum() / pwmValues.size).toFloat(),
                            onValueChange = {
                                val v = it.roundToInt().coerceIn(0, Logic.PWM_MAX)
                                pwmValues = pwmValues.map { v }.toMutableList()
                            },
                            onValueChangeFinished = { vm.applyPwm(pwmValues) },
                            valueRange = 0f..Logic.PWM_MAX.toFloat(),
                            enabled = !vm.busy,
                            colors = SliderDefaults.colors(
                                thumbColor = Cyan, activeTrackColor = Cyan,
                                inactiveTrackColor = Color(0xFF1E3350),
                                disabledThumbColor = TextDim, disabledActiveTrackColor = TextDim,
                                disabledInactiveTrackColor = Color(0xFF1E3350)
                            )
                        )
                        Text("Przesuń, aby ustawić ogólną jasność wszystkich kanałów", fontSize = 6.sp, color = TextMid)
                    }
                }
                Row(
                    modifier = Modifier.fillMaxWidth().clickable { pwmExpanded = !pwmExpanded }.padding(vertical = 7.dp),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Text(
                        "Rozwiń regulację poszczególnych kanałów",
                        fontSize = 8.sp, color = TextMid, fontWeight = FontWeight.SemiBold
                    )
                    Text(
                        "5 kanałów  ${if (pwmExpanded) "⌃" else "⌄"}",
                        fontSize = 6.sp, color = TextDim, fontFamily = FontFamily.Monospace
                    )
                }
                if (pwmExpanded) {
                    pwmValues.forEachIndexed { index, value ->
                        PwmSlider(
                            index = index, value = value, enabled = !vm.busy,
                            onChange = { newValue ->
                                pwmValues = pwmValues.toMutableList().apply { set(index, newValue) }
                            },
                            onChangeFinished = { vm.applyPwm(pwmValues) }
                        )
                    }
                    PrimaryButton("Zapisz PWM", enabled = !vm.busy, modifier = Modifier.fillMaxWidth(), onClick = {
                        vm.applyPwm(pwmValues)
                    })
                }
            }
        }

        // Energia i historia zużycia — HTML energyCard: energyStats + energyChart
        SectionCard {
            val watts = s?.powerNowW ?: 0.0
            val todayWh = s?.energyTodayWh ?: 0.0
            Column(modifier = Modifier.fillMaxWidth(), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Column(Modifier.weight(1f)) {
                        Text("Energia i historia zużycia", fontSize = 10.sp, fontWeight = FontWeight.Bold, color = TextHi)
                        Text("Szacunek LED + pompka · bez fizycznego miernika", fontSize = 6.sp, color = TextDim)
                    }
                    Pill("ESTYMACJA", Color(0xFF67DFF2))
                }
                Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                    EnergyStat("TERAZ", Logic.fmtWatts(watts), "W", Modifier.weight(1f))
                    EnergyStat("DZIŚ · EST.", "${(todayWh / 1000.0).format("%.2f")}", "kWh", Modifier.weight(1f))
                    EnergyStat("MIESIĄC · EST.", Logic.costPln(todayWh * 30.0, s?.kwhPrice), "zł", Modifier.weight(1f))
                }
                LineChart(
                    values = s?.hourPowerWh ?: emptyList(),
                    color = Cyan,
                    modifier = Modifier.fillMaxWidth().height(110.dp),
                    unit = " W"
                )
                Text(
                    "Historia poglądowa · pojemność lampy przyjęta: 120 W",
                    fontSize = 6.sp, color = TextDim
                )
            }
        }

        // Światło i LUX — HTML karta z lightTiles (SŁOŃCE / DOŚWIETL. / NAD WODĄ)
        SectionCard {
            Column(modifier = Modifier.fillMaxWidth(), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Text("Światło i LUX", fontSize = 10.sp, fontWeight = FontWeight.Bold, color = TextHi)
                    Text("pomiary ›", fontSize = 8.sp, color = TextDim, fontStyle = androidx.compose.ui.text.font.FontStyle.Italic)
                }
                val sunLux = s?.luxRoom?.toInt() ?: 0
                val ledLux = if (power) (850.0 * pwmAvg / 650.0).roundToInt() else 0
                val luxScale = (s?.minLuxTarget ?: 2000).coerceAtLeast(500)
                val sunShare = (sunLux.toFloat() / luxScale).coerceIn(0f, 1f)
                val ledShare = (ledLux.toFloat() / luxScale).coerceIn(0f, 1f - sunShare)
                Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    LightTile(
                        label = "SŁOŃCE",
                        value = s?.luxRoom?.let { "${it.toInt()} lx" } ?: "—",
                        sub = "przy roślinach",
                        icon = Icons.Default.WbSunny,
                        accent = Warm,
                        modifier = Modifier.weight(1f)
                    )
                    LightTile(
                        label = "DOŚWIETL.",
                        value = if (s?.luxRoom == null && !power) "—" else "$ledLux lx",
                        sub = "LED · $pwmPercent%",
                        icon = Icons.Default.Lightbulb,
                        accent = Cyan,
                        modifier = Modifier.weight(1f)
                    )
                    LightTile(
                        label = "NAD WODĄ",
                        value = s?.luxWater?.let { if (it >= 1000) "${(it / 1000.0).format("%.1f")}k" else "${it.toInt()}" } ?: "—",
                        sub = "lx · czujnik",
                        icon = Icons.Default.DeviceThermostat,
                        accent = FsBlue,
                        modifier = Modifier.weight(1f)
                    )
                }
                Row(
                    modifier = Modifier.fillMaxWidth().padding(top = 8.dp, bottom = 2.dp),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Text("ŚWIATŁO ŁĄCZNIE PRZY ROŚLINACH", fontSize = 6.sp, color = TextDim, fontFamily = FontFamily.Monospace)
                    Text("${sunLux + ledLux} / $luxScale lx", fontSize = 7.sp, color = TextHi, fontFamily = FontFamily.Monospace, fontWeight = FontWeight.Bold)
                }
                Row(
                    modifier = Modifier.fillMaxWidth().height(7.dp).clip(RoundedCornerShape(99.dp)).background(Border)
                ) {
                    if (sunShare > 0f) Box(Modifier.weight(sunShare).fillMaxWidth().height(7.dp).background(Warm))
                    if (ledShare > 0f) Box(Modifier.weight(ledShare).fillMaxWidth().height(7.dp).background(Cyan))
                }
                Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
                    Text("☀ SŁOŃCE", fontSize = 6.sp, color = TextDim, fontFamily = FontFamily.Monospace)
                    Text("💡 LED", fontSize = 6.sp, color = TextDim, fontFamily = FontFamily.Monospace)
                    Text("SKALA 0–$luxScale lx", fontSize = 6.sp, color = TextDim, fontFamily = FontFamily.Monospace)
                }
            }
        }

        // Szybkie akcje — HTML .sectionTitle + .quickGrid (3 quickTile)
        Text(
            "Szybkie akcje", fontSize = 8.sp, fontWeight = FontWeight.Bold, color = TextDim,
            fontFamily = FontFamily.Monospace, modifier = Modifier.padding(top = 4.dp)
        )
        Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            QuickTile("Sterowanie PWM", Icons.Default.Dashboard, modifier = Modifier.weight(1f), accent = Warm) { onNavigate("light") }
            QuickTile("Harmonogram", Icons.Default.Schedule, modifier = Modifier.weight(1f), accent = FsBlue) { onNavigate("plan") }
            QuickTile("Ostatnie logi", Icons.Default.Article, modifier = Modifier.weight(1f), accent = Cyan) { onNavigate("logs") }
        }
    }
}

private fun Double.format(pattern: String): String =
    java.lang.String.format(java.util.Locale.US, pattern, this)

