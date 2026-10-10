package com.rybyled.panel.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Bolt
import androidx.compose.material.icons.filled.Home
import androidx.compose.material.icons.filled.Lightbulb
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.WaterDrop
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.rybyled.panel.core.Logic
import kotlin.math.roundToInt

/* Ekrany aplikacji Ryby LED — 5 zakładek jak w panel/ryby-mobile.html, wygląd ciemny i spokojny. */

private val Bg = Color(0xFF09131B)
private val PanelColor = Color(0xFF111E29)
private val PanelRaised = Color(0xFF162834)
private val Stroke = Color(0xFF1F3240)
private val Muted = Color(0xFF8DA2B5)
private val Accent = Color(0xFF38BDF8)
private val Good = Color(0xFF34D399)
private val Warn = Color(0xFFFBBF24)
private val Bad = Color(0xFFF87171)
private val Ink = Color(0xFFE8F0F7)

private val RybyColors = darkColorScheme(
    primary = Accent,
    onPrimary = Color(0xFF00212E),
    primaryContainer = Color(0xFF0E3A4F),
    onPrimaryContainer = Color(0xFFBDEBFF),
    background = Bg,
    onBackground = Ink,
    surface = PanelColor,
    onSurface = Ink,
    surfaceVariant = PanelRaised,
    onSurfaceVariant = Muted,
    outline = Stroke,
    error = Bad,
    onError = Color(0xFF3B0A0A),
    secondaryContainer = PanelRaised
)

private val TAB_LABELS = listOf("Główna", "Światło", "Pompa", "Energia", "Ustawienia")
private val TAB_TITLES = listOf("Przegląd", "Oświetlenie", "Pompa", "Energia", "Ustawienia")
private val TAB_ICONS = listOf(
    Icons.Filled.Home,
    Icons.Filled.Lightbulb,
    Icons.Filled.WaterDrop,
    Icons.Filled.Bolt,
    Icons.Filled.Settings
)

@Composable
fun RybyApp(vm: AppViewModel = viewModel()) {
    MaterialTheme(colorScheme = RybyColors) {
        var tab by rememberSaveable { mutableIntStateOf(0) }
        Scaffold(
            containerColor = Bg,
            bottomBar = {
                NavigationBar(containerColor = PanelColor) {
                    TAB_LABELS.forEachIndexed { i, label ->
                        NavigationBarItem(
                            selected = tab == i,
                            onClick = { tab = i },
                            icon = { Icon(TAB_ICONS[i], contentDescription = label) },
                            label = { Text(label, maxLines = 1, style = MaterialTheme.typography.labelSmall) }
                        )
                    }
                }
            }
        ) { pad ->
            Column(Modifier.padding(pad).fillMaxSize().padding(horizontal = 16.dp)) {
                Header(TAB_TITLES[tab], vm.conn)
                val err = vm.error
                val msg = vm.notice
                if (err != null) Banner(err, Bad, null)
                if (msg != null) Banner(msg, Accent, { vm.clearNotice() })
                Column(
                    modifier = Modifier
                        .weight(1f)
                        .fillMaxWidth()
                        .verticalScroll(rememberScrollState()),
                    verticalArrangement = Arrangement.spacedBy(12.dp)
                ) {
                    when (tab) {
                        0 -> MainTab(vm)
                        1 -> LightTab(vm)
                        2 -> PumpTab(vm)
                        3 -> EnergyTab(vm)
                        else -> SettingsTab(vm)
                    }
                    Spacer(Modifier.height(8.dp))
                }
            }
        }
    }
}

// ── Ekrany ──

@Composable
private fun MainTab(vm: AppViewModel) {
    val st = vm.status
    if (st == null) {
        Panel { Text("Czekam na pierwszy odczyt z bazy…", color = Muted) }
        return
    }
    Panel {
        Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.SpaceBetween) {
            Column {
                Text("Płytka 1", style = MaterialTheme.typography.labelMedium, color = Muted)
                Text(Logic.fmtTemp(st.temps.getOrNull(0)), style = MaterialTheme.typography.displaySmall, fontWeight = FontWeight.SemiBold)
            }
            Badge(if (st.mode == "MANUAL") "MANUAL" else "AUTO", Accent)
        }
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(10.dp)) {
            Tile("Płytka 2", Logic.fmtTemp(st.temps.getOrNull(1)), Modifier.weight(1f))
            Tile("Płytka 3", Logic.fmtTemp(st.temps.getOrNull(2)), Modifier.weight(1f))
        }
    }
    Panel(title = "Sterowanie") {
        SwitchRow("Zasilanie", st.power) { on -> vm.sendPlain(if (on) "power_on" else "power_off") }
        HorizontalDivider(color = Stroke)
        SwitchRow("Pompa", st.pumpOn) { on -> vm.sendPlain(if (on) "pump_on" else "pump_off") }
        HorizontalDivider(color = Stroke)
        Text("Tryb pracy", style = MaterialTheme.typography.bodyMedium, color = Muted)
        val sel = if (st.mode == "MANUAL") 1 else 0
        Segmented(listOf("Automatyczny", "Ręczny"), sel) { i ->
            if (i != sel) vm.sendPlain(if (i == 0) "mode_auto" else "mode_manual")
        }
    }
    Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(10.dp)) {
        Tile("Lux pokój", Logic.fmtLux(st.luxRoom), Modifier.weight(1f))
        Tile("Lux nad wodą", Logic.fmtLux(st.luxNadWoda), Modifier.weight(1f))
    }
    Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(10.dp)) {
        Tile("Moc LED", st.pct?.let { "${it.roundToInt()} %" } ?: "—", Modifier.weight(1f))
        Tile("Pobór teraz", Logic.fmtWatts(st.powerNowW), Modifier.weight(1f))
    }
    Panel(title = "Stan sterownika") {
        KV("Min LUX", when {
            !st.minLuxEnabled -> "wyłączony"
            st.minLuxActive -> "aktywny · cel ${Logic.fmtLux(st.minLuxTarget)}"
            else -> "czeka"
        })
        KV("Czas pracy", Logic.fmtUptime(st.uptimeS))
        KV("Sygnał Wi-Fi", st.wifiRssi?.let { "${it.roundToInt()} dBm" } ?: "—")
    }
}

@Composable
private fun LightTab(vm: AppViewModel) {
    val st = vm.status
    var pwm by remember { mutableStateOf(List(Logic.PWM_CHANNELS) { 0f }) }
    var dirty by remember { mutableStateOf(false) }
    var askAutosave by remember { mutableStateOf(false) }

    // Odczyt z ESP nie nadpisuje suwaków, dopóki użytkownik ich nie zmienił („Wczytaj z ESP”).
    LaunchedEffect(st) {
        if (st != null && !dirty) pwm = st.pwm.map { it.toFloat() }
    }

    Panel(title = "Kanały PWM · 0–1023") {
        pwm.forEachIndexed { i, v ->
            Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.SpaceBetween) {
                Text("Kanał ${i + 1}", style = MaterialTheme.typography.bodyLarge)
                Text("${v.roundToInt()}", style = MaterialTheme.typography.titleMedium, fontWeight = FontWeight.SemiBold, color = Accent)
            }
            Slider(
                value = v,
                onValueChange = { nv ->
                    pwm = pwm.toMutableList().also { it[i] = nv }
                    dirty = true
                },
                valueRange = 0f..Logic.PWM_MAX.toFloat(),
                colors = SliderDefaults.colors(
                    thumbColor = Accent,
                    activeTrackColor = Accent,
                    inactiveTrackColor = Stroke
                )
            )
        }
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(10.dp)) {
            ActionButton("Zastosuj", Modifier.weight(1f)) {
                vm.sendPwm(pwm.map { it.toDouble() })
                dirty = false
            }
            OutlinedButton(
                onClick = {
                    if (st != null) pwm = st.pwm.map { it.toFloat() }
                    dirty = false
                },
                modifier = Modifier.weight(1f).heightIn(min = 48.dp)
            ) { Text("Wczytaj z ESP") }
        }
    }
    Panel(title = "Szybkie ustawienia") {
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(10.dp)) {
            ActionButton("LED 100 %", Modifier.weight(1f)) { vm.sendPlain("led_100") }
            OutlinedButton(
                onClick = { vm.sendPlain("led_off") },
                modifier = Modifier.weight(1f).heightIn(min = 48.dp)
            ) { Text("LED wyłączony") }
        }
    }
    Panel(title = "Zapis") {
        Text("Zapisuje obecne suwaki jako wartości domyślne na sterowniku.", color = Muted, style = MaterialTheme.typography.bodyMedium)
        ActionButton("Zapisz jako domyślne", Modifier.fillMaxWidth()) { askAutosave = true }
    }
    if (askAutosave) {
        ConfirmDialog(
            title = "Zapisać jako domyślne?",
            text = "Obecne wartości suwaków zostaną zapisane na sterowniku.",
            onConfirm = {
                vm.sendAutosave(pwm.map { it.toDouble() })
                askAutosave = false
            },
            onDismiss = { askAutosave = false }
        )
    }
}

@Composable
private fun PumpTab(vm: AppViewModel) {
    val st = vm.status
    Panel(title = "Pompa") {
        SwitchRow("Pompa obiegowa", st?.pumpOn == true) { on -> vm.sendPlain(if (on) "pump_on" else "pump_off") }
        Text(
            "Stan z ostatniego odczytu. Zmiana trafia do sterownika w ciągu kilkunastu sekund.",
            color = Muted,
            style = MaterialTheme.typography.bodyMedium
        )
    }
    Panel(title = "Przedziały pracy (tylko odczyt)") {
        val slots = st?.pumpSlots.orEmpty()
        if (slots.isEmpty()) {
            Text("Brak przedziałów", color = Muted)
        } else {
            slots.forEachIndexed { i, s ->
                KV("Przedział ${i + 1}", "${Logic.fmtHHMM(s.start)} – ${Logic.fmtHHMM(s.end)}")
            }
        }
    }
}

@Composable
private fun EnergyTab(vm: AppViewModel) {
    val st = vm.status
    if (st == null) {
        Panel { Text("Czekam na pierwszy odczyt z bazy…", color = Muted) }
        return
    }
    Panel(title = "Dziś") {
        Text(Logic.fmtWh(st.energy.today), style = MaterialTheme.typography.displaySmall, fontWeight = FontWeight.SemiBold)
        Text("Koszt: ${Logic.fmtPln(Logic.costPln(st.energy.today, st.kwhPrice))}", color = Muted)
    }
    Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(10.dp)) {
        Tile("Tydzień", Logic.fmtWh(st.energy.week), Modifier.weight(1f))
        Tile("Miesiąc", Logic.fmtWh(st.energy.month), Modifier.weight(1f))
    }
    Panel(title = "Ostatnie dni") {
        val days = st.dayHistory.takeLast(7)
        if (days.isEmpty()) Text("Brak historii", color = Muted) else BarChart(days)
    }
    Panel(title = "Czas świecenia LED") {
        KV("Dziś", Logic.fmtMinutes(st.ledMin.today))
        KV("Tydzień", Logic.fmtMinutes(st.ledMin.week))
        KV("Miesiąc", Logic.fmtMinutes(st.ledMin.month))
    }
    Panel(title = "Taryfa") {
        KV("Cena kWh", Logic.fmtPln(st.kwhPrice))
        KV("Szczyt mocy dziś", Logic.fmtWatts(st.peakPowerWToday))
    }
}

@Composable
private fun SettingsTab(vm: AppViewModel) {
    var dbUrl by remember { mutableStateOf(vm.prefs.dbUrl) }
    var secret by remember { mutableStateOf(vm.prefs.secret) }
    var token by remember { mutableStateOf(vm.prefs.token) }
    var askRestart by remember { mutableStateOf(false) }

    Panel(title = "Połączenie z bazą") {
        Text(
            "Sekrety zapisują się tylko na tym telefonie, nie w kodzie aplikacji.",
            color = Muted,
            style = MaterialTheme.typography.bodyMedium
        )
        OutlinedTextField(
            value = dbUrl,
            onValueChange = { dbUrl = it },
            label = { Text("Adres bazy (HTTPS)") },
            singleLine = true,
            modifier = Modifier.fillMaxWidth()
        )
        OutlinedTextField(
            value = secret,
            onValueChange = { secret = it },
            label = { Text("Database Secret") },
            singleLine = true,
            visualTransformation = PasswordVisualTransformation(),
            modifier = Modifier.fillMaxWidth()
        )
        OutlinedTextField(
            value = token,
            onValueChange = { token = it },
            label = { Text("CMD_TOKEN") },
            singleLine = true,
            visualTransformation = PasswordVisualTransformation(),
            modifier = Modifier.fillMaxWidth()
        )
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(10.dp)) {
            ActionButton("Zapisz", Modifier.weight(1f)) { vm.saveSettings(dbUrl, secret, token) }
            ActionButton("Sprawdź połączenie", Modifier.weight(1f)) { vm.testConnection(dbUrl, secret, token) }
        }
        OutlinedButton(
            onClick = {
                vm.forgetSecrets()
                secret = ""
                token = ""
            },
            modifier = Modifier.fillMaxWidth().heightIn(min = 48.dp)
        ) { Text("Wyczyść sekrety z tego telefonu") }
    }
    Panel(title = "Sterownik") {
        Button(
            onClick = { askRestart = true },
            modifier = Modifier.fillMaxWidth().heightIn(min = 48.dp),
            colors = ButtonDefaults.buttonColors(containerColor = Bad, contentColor = Color(0xFF3B0A0A))
        ) { Text("Restart ESP") }
    }
    if (askRestart) {
        ConfirmDialog(
            title = "Zrestartować sterownik?",
            text = "ESP zrestartuje się. Światło i pompa zgasną na chwilę.",
            onConfirm = {
                vm.sendPlain("restart")
                askRestart = false
            },
            onDismiss = { askRestart = false }
        )
    }
}

// ── Elementy wspólne ──

@Composable
private fun Header(title: String, conn: Logic.Conn) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(top = 12.dp, bottom = 14.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Column {
            Text("RYBY LED", style = MaterialTheme.typography.labelMedium, color = Accent, fontWeight = FontWeight.Bold)
            Text(title, style = MaterialTheme.typography.headlineMedium, fontWeight = FontWeight.SemiBold)
        }
        StatusChip(conn)
    }
}

@Composable
private fun StatusChip(conn: Logic.Conn) {
    val (label, color) = when (conn) {
        Logic.Conn.ONLINE -> "Online" to Good
        Logic.Conn.STALE -> "Opóźnione" to Warn
        Logic.Conn.NONE -> "Brak danych" to Muted
    }
    Row(
        modifier = Modifier
            .clip(RoundedCornerShape(50))
            .background(color.copy(alpha = 0.14f))
            .padding(horizontal = 12.dp, vertical = 7.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Box(Modifier.size(8.dp).clip(CircleShape).background(color))
        Spacer(Modifier.width(7.dp))
        Text(label, style = MaterialTheme.typography.labelLarge, color = color)
    }
}

@Composable
private fun Banner(text: String, color: Color, onDismiss: (() -> Unit)?) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(bottom = 10.dp)
            .clip(RoundedCornerShape(14.dp))
            .background(color.copy(alpha = 0.14f))
            .padding(horizontal = 14.dp, vertical = 10.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(text, color = color, style = MaterialTheme.typography.bodyMedium, modifier = Modifier.weight(1f))
        if (onDismiss != null) TextButton(onClick = onDismiss) { Text("OK", color = color) }
    }
}

@Composable
private fun Panel(title: String? = null, content: @Composable ColumnScope.() -> Unit) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(18.dp))
            .background(PanelColor)
            .border(1.dp, Stroke, RoundedCornerShape(18.dp))
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        if (title != null) {
            Text(title.uppercase(), style = MaterialTheme.typography.labelMedium, color = Muted, fontWeight = FontWeight.SemiBold)
        }
        content()
    }
}

@Composable
private fun Tile(label: String, value: String, modifier: Modifier = Modifier) {
    Column(
        modifier = modifier
            .clip(RoundedCornerShape(16.dp))
            .background(PanelRaised)
            .padding(14.dp),
        verticalArrangement = Arrangement.spacedBy(6.dp)
    ) {
        Text(label, style = MaterialTheme.typography.labelMedium, color = Muted)
        Text(value, style = MaterialTheme.typography.titleLarge, fontWeight = FontWeight.SemiBold)
    }
}

@Composable
private fun Badge(text: String, color: Color) {
    Box(
        modifier = Modifier
            .clip(RoundedCornerShape(50))
            .background(color.copy(alpha = 0.16f))
            .padding(horizontal = 12.dp, vertical = 6.dp)
    ) {
        Text(text, style = MaterialTheme.typography.labelLarge, color = color, fontWeight = FontWeight.SemiBold)
    }
}

@Composable
private fun KV(label: String, value: String) {
    Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
        Text(label, style = MaterialTheme.typography.bodyLarge, color = Muted)
        Text(value, style = MaterialTheme.typography.bodyLarge)
    }
}

@Composable
private fun SwitchRow(label: String, checked: Boolean, onChange: (Boolean) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(label, style = MaterialTheme.typography.titleMedium)
        Switch(checked = checked, onCheckedChange = onChange)
    }
}

@Composable
private fun Segmented(options: List<String>, selected: Int, onSelect: (Int) -> Unit) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(14.dp))
            .background(Bg)
            .padding(4.dp),
        horizontalArrangement = Arrangement.spacedBy(4.dp)
    ) {
        options.forEachIndexed { i, label ->
            val isSel = i == selected
            Box(
                modifier = Modifier
                    .weight(1f)
                    .clip(RoundedCornerShape(11.dp))
                    .background(if (isSel) Accent.copy(alpha = 0.18f) else Color.Transparent)
                    .clickable { onSelect(i) }
                    .padding(vertical = 12.dp),
                contentAlignment = Alignment.Center
            ) {
                Text(
                    label,
                    style = MaterialTheme.typography.labelLarge,
                    color = if (isSel) Accent else Muted,
                    fontWeight = if (isSel) FontWeight.SemiBold else FontWeight.Normal
                )
            }
        }
    }
}

@Composable
private fun BarChart(values: List<Double>) {
    val max = values.maxOrNull() ?: 0.0
    Row(
        modifier = Modifier.fillMaxWidth().height(130.dp),
        verticalAlignment = Alignment.Bottom,
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        values.forEach { v ->
            val frac = if (max > 0.0) (v / max).toFloat().coerceIn(0.04f, 1f) else 0.04f
            Box(
                modifier = Modifier
                    .weight(1f)
                    .fillMaxHeight(frac)
                    .clip(RoundedCornerShape(topStart = 6.dp, topEnd = 6.dp))
                    .background(Accent)
            )
        }
    }
}

@Composable
private fun ActionButton(label: String, modifier: Modifier = Modifier, onClick: () -> Unit) {
    Button(onClick = onClick, modifier = modifier.heightIn(min = 48.dp)) {
        Text(label, fontWeight = FontWeight.SemiBold)
    }
}

@Composable
private fun ConfirmDialog(title: String, text: String, onConfirm: () -> Unit, onDismiss: () -> Unit) {
    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text(title) },
        text = { Text(text) },
        confirmButton = { TextButton(onClick = onConfirm) { Text("Tak") } },
        dismissButton = { TextButton(onClick = onDismiss) { Text("Anuluj") } }
    )
}
