package com.rybyled.panel.ui

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Slider
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
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.rybyled.panel.core.Logic
import kotlin.math.roundToInt

/* Ekrany aplikacji — 5 zakładek jak w panel/ryby-mobile.html. */

private val RybyColors = darkColorScheme(
    primary = Color(0xFF38BDF8),
    onPrimary = Color(0xFF00212E),
    background = Color(0xFF0B1620),
    onBackground = Color(0xFFE6EEF5),
    surface = Color(0xFF12202C),
    onSurface = Color(0xFFE6EEF5),
    onSurfaceVariant = Color(0xFF8FA3B5),
    surfaceVariant = Color(0xFF1A2D3D),
    secondaryContainer = Color(0xFF1E3A4C),
    error = Color(0xFFF87171)
)

private val TAB_LABELS = listOf("Główna", "Światło", "Pompa", "Energia", "Ustawienia")

@Composable
fun RybyApp(vm: AppViewModel = viewModel()) {
    MaterialTheme(colorScheme = RybyColors) {
        var tab by rememberSaveable { mutableIntStateOf(0) }
        Scaffold(
            bottomBar = {
                NavigationBar {
                    TAB_LABELS.forEachIndexed { i, label ->
                        NavigationBarItem(
                            selected = tab == i,
                            onClick = { tab = i },
                            icon = { Text("•") },
                            label = { Text(label, maxLines = 1) }
                        )
                    }
                }
            }
        ) { pad ->
            Column(
                modifier = Modifier
                    .padding(pad)
                    .fillMaxSize()
                    .verticalScroll(rememberScrollState())
                    .padding(horizontal = 12.dp, vertical = 8.dp)
            ) {
                NoticeBar(vm)
                when (tab) {
                    0 -> MainTab(vm)
                    1 -> LightTab(vm)
                    2 -> PumpTab(vm)
                    3 -> EnergyTab(vm)
                    else -> SettingsTab(vm)
                }
            }
        }
    }
}

@Composable
private fun NoticeBar(vm: AppViewModel) {
    val msg = vm.notice
    val err = vm.error
    if (msg != null) {
        Section("Komunikat") {
            Text(msg)
            TextButton(onClick = { vm.clearNotice() }) { Text("OK") }
        }
    }
    if (err != null) {
        Text(err, color = MaterialTheme.colorScheme.error, modifier = Modifier.padding(vertical = 4.dp))
    }
}

@Composable
private fun MainTab(vm: AppViewModel) {
    ConnPill(vm.conn)
    val st = vm.status
    if (st == null) {
        Section("Brak danych") { Text("Czekam na pierwszy odczyt z bazy…") }
        return
    }
    Section("Zasilanie") {
        KV("Zasilanie", if (st.power) "WŁ." else "WYŁ.")
        KV("Tryb", st.mode ?: "—")
        KV("Pompa", if (st.pumpOn) "WŁ." else "WYŁ.")
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            ActionButton("Włącz", Modifier.weight(1f)) { vm.sendPlain("power_on") }
            ActionButton("Wyłącz", Modifier.weight(1f)) { vm.sendPlain("power_off") }
        }
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            ActionButton("Tryb AUTO", Modifier.weight(1f)) { vm.sendPlain("mode_auto") }
            ActionButton("Tryb MANUAL", Modifier.weight(1f)) { vm.sendPlain("mode_manual") }
        }
    }
    Section("Temperatury") {
        st.temps.forEachIndexed { i, t -> KV("Płytka ${i + 1}", Logic.fmtTemp(t)) }
    }
    Section("Światło") {
        KV("Lux pokój", Logic.fmtLux(st.luxRoom))
        KV("Lux nad wodą", Logic.fmtLux(st.luxNadWoda))
        KV("Moc LED", st.pct?.let { "${it.roundToInt()} %" } ?: "—")
        KV("Pobór teraz", Logic.fmtWatts(st.powerNowW))
        KV(
            "Min LUX",
            when {
                !st.minLuxEnabled -> "wyłączony"
                st.minLuxActive -> "aktywny (cel ${Logic.fmtLux(st.minLuxTarget)})"
                else -> "czeka"
            }
        )
    }
    Section("ESP") {
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

    Section("Kanały PWM (0–1023)") {
        pwm.forEachIndexed { i, v ->
            Text("Kanał ${i + 1}: ${v.roundToInt()}")
            Slider(
                value = v,
                onValueChange = { nv ->
                    pwm = pwm.toMutableList().also { it[i] = nv }
                    dirty = true
                },
                valueRange = 0f..Logic.PWM_MAX.toFloat()
            )
        }
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            ActionButton("Wyślij PWM", Modifier.weight(1f)) {
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
    Section("Szybkie") {
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            ActionButton("LED 100 %", Modifier.weight(1f)) { vm.sendPlain("led_100") }
            ActionButton("LED OFF", Modifier.weight(1f)) { vm.sendPlain("led_off") }
        }
    }
    Section("Zapis") {
        Text("Zapisuje obecne wartości suwaków jako domyślne na sterowniku.")
        ActionButton("Zapisz jako domyślne (autosave)", Modifier.fillMaxWidth()) { askAutosave = true }
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
    Section("Pompa") {
        KV("Stan", if (st?.pumpOn == true) "WŁ." else "WYŁ.")
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            ActionButton("Włącz pompę", Modifier.weight(1f)) { vm.sendPlain("pump_on") }
            ActionButton("Wyłącz pompę", Modifier.weight(1f)) { vm.sendPlain("pump_off") }
        }
    }
    Section("Przedziały pompy (tylko odczyt)") {
        val slots = st?.pumpSlots.orEmpty()
        if (slots.isEmpty()) {
            Text("Brak przedziałów")
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
        Section("Brak danych") { Text("Czekam na pierwszy odczyt z bazy…") }
        return
    }
    Section("Energia") {
        KV("Dziś", Logic.fmtWh(st.energy.today))
        KV("Tydzień", Logic.fmtWh(st.energy.week))
        KV("Miesiąc", Logic.fmtWh(st.energy.month))
        KV("Szczyt dziś", Logic.fmtWatts(st.peakPowerWToday))
    }
    Section("Koszt (cena z ESP)") {
        KV("Cena kWh", Logic.fmtPln(st.kwhPrice))
        KV("Dziś", Logic.fmtPln(Logic.costPln(st.energy.today, st.kwhPrice)))
        KV("Tydzień", Logic.fmtPln(Logic.costPln(st.energy.week, st.kwhPrice)))
    }
    Section("Czas świecenia LED") {
        KV("Dziś", Logic.fmtMinutes(st.ledMin.today))
        KV("Tydzień", Logic.fmtMinutes(st.ledMin.week))
        KV("Miesiąc", Logic.fmtMinutes(st.ledMin.month))
    }
    Section("Ostatnie dni (Wh)") {
        val days = st.dayHistory.takeLast(7)
        if (days.isEmpty()) Text("Brak historii") else days.forEach { KV("Dzień", Logic.fmtWh(it)) }
    }
}

@Composable
private fun SettingsTab(vm: AppViewModel) {
    var dbUrl by remember { mutableStateOf(vm.prefs.dbUrl) }
    var secret by remember { mutableStateOf(vm.prefs.secret) }
    var token by remember { mutableStateOf(vm.prefs.token) }
    var askRestart by remember { mutableStateOf(false) }

    Section("Połączenie z bazą") {
        Text("Sekrety zapisują się tylko na tym telefonie, nie w kodzie aplikacji.")
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
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
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
    Section("Sterowanie") {
        ActionButton("Restart ESP", Modifier.fillMaxWidth()) { askRestart = true }
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
private fun Section(title: String, content: @Composable ColumnScope.() -> Unit) {
    Card(modifier = Modifier.fillMaxWidth().padding(vertical = 6.dp)) {
        Column(
            modifier = Modifier.padding(14.dp),
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Text(title, style = MaterialTheme.typography.titleMedium)
            content()
        }
    }
}

@Composable
private fun KV(label: String, value: String) {
    Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
        Text(label, color = MaterialTheme.colorScheme.onSurfaceVariant)
        Text(value)
    }
}

@Composable
private fun ConnPill(conn: Logic.Conn) {
    val (text, color) = when (conn) {
        Logic.Conn.ONLINE -> "ONLINE" to Color(0xFF34D399)
        Logic.Conn.STALE -> "OPÓŹNIONE" to Color(0xFFFBBF24)
        Logic.Conn.NONE -> "BRAK DANYCH" to MaterialTheme.colorScheme.onSurfaceVariant
    }
    Text(text, color = color, style = MaterialTheme.typography.labelLarge, modifier = Modifier.padding(vertical = 4.dp))
}

@Composable
private fun ActionButton(label: String, modifier: Modifier = Modifier, onClick: () -> Unit) {
    Button(onClick = onClick, modifier = modifier.heightIn(min = 48.dp)) { Text(label) }
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
