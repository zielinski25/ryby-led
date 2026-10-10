package com.rybyled.panel.ui

import androidx.compose.foundation.BorderStroke
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
import androidx.compose.foundation.layout.statusBarsPadding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.rememberScrollState
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
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.NavigationBarItemDefaults
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.OutlinedTextFieldDefaults
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Switch
import androidx.compose.material3.SwitchDefaults
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
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
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.rybyled.panel.core.Logic
import com.rybyled.panel.core.RybyStatus
import kotlin.math.roundToInt

/* Ekrany Ryby LED — 5 zakładek jak w panel/ryby-mobile.html, wygląd według systemu z Pieca (ui/Theme.kt). */

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
    RybyTheme {
        var tab by rememberSaveable { mutableIntStateOf(0) }
        Scaffold(
            containerColor = Pal.Bg,
            bottomBar = { BottomNav(tab) { tab = it } }
        ) { pad ->
            Column(Modifier.padding(pad).fillMaxSize()) {
                TopBar(TAB_TITLES[tab], vm.conn)
                Column(
                    modifier = Modifier
                        .weight(1f)
                        .fillMaxWidth()
                        .verticalScroll(rememberScrollState())
                        .padding(horizontal = Dimens.pagePad)
                        .padding(top = 12.dp),
                    verticalArrangement = Arrangement.spacedBy(Dimens.gap)
                ) {
                    val err = vm.error
                    val msg = vm.notice
                    if (err != null) Banner(err, Pal.Err)
                    if (msg != null) Banner(msg, Pal.Cyan) { vm.clearNotice() }
                    when (tab) {
                        0 -> MainTab(vm)
                        1 -> LightTab(vm)
                        2 -> PumpTab(vm)
                        3 -> EnergyTab(vm)
                        else -> SettingsTab(vm)
                    }
                    Spacer(Modifier.height(12.dp))
                }
            }
        }
    }
}

// ── Pasek i nawigacja ──

@Composable
private fun TopBar(title: String, conn: Logic.Conn) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .background(Brush.verticalGradient(listOf(Pal.Top, Pal.Top2)))
            .statusBarsPadding()
            .padding(horizontal = Dimens.pagePad, vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Column {
            Text("RYBY LED", style = Txt.brand)
            Text(title, style = Txt.title)
        }
        StatusChip(conn)
    }
}

@Composable
private fun StatusChip(conn: Logic.Conn) {
    val (text, color) = when (conn) {
        Logic.Conn.ONLINE -> "● LIVE" to Pal.Live
        Logic.Conn.STALE -> "● OPÓŹNIONE" to Pal.Warn
        Logic.Conn.NONE -> "○ BRAK DANYCH" to Pal.TextDim
    }
    Pill(text, color)
}

@Composable
private fun Pill(text: String, color: Color) {
    Box(
        modifier = Modifier
            .clip(RoundedCornerShape(Dimens.radiusPill))
            .background(color.copy(alpha = 0.13f))
            .border(1.dp, color.copy(alpha = 0.35f), RoundedCornerShape(Dimens.radiusPill))
            .padding(horizontal = 10.dp, vertical = 5.dp)
    ) {
        Text(text, style = Txt.chip, color = color)
    }
}

@Composable
private fun BottomNav(selected: Int, onSelect: (Int) -> Unit) {
    NavigationBar(containerColor = Pal.Nav) {
        TAB_LABELS.forEachIndexed { i, label ->
            NavigationBarItem(
                selected = selected == i,
                onClick = { onSelect(i) },
                icon = { Icon(TAB_ICONS[i], contentDescription = label) },
                label = { Text(label, style = Txt.nav, maxLines = 1) },
                colors = NavigationBarItemDefaults.colors(
                    selectedIconColor = Pal.Cyan,
                    selectedTextColor = Pal.Cyan,
                    indicatorColor = Pal.Cyan.copy(alpha = 0.14f),
                    unselectedIconColor = Pal.TextDim2,
                    unselectedTextColor = Pal.TextDim2
                )
            )
        }
    }
}

// ── Ekrany ──

@Composable
private fun MainTab(vm: AppViewModel) {
    val st = vm.status
    if (st == null) {
        Panel { Text("Czekam na pierwszy odczyt z bazy…", style = Txt.note) }
        return
    }
    Hero(st, stale = vm.conn == Logic.Conn.STALE)
    Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(Dimens.gap)) {
        Tile("Płytka 2", Logic.fmtTemp(st.temps.getOrNull(1)), Modifier.weight(1f))
        Tile("Płytka 3", Logic.fmtTemp(st.temps.getOrNull(2)), Modifier.weight(1f))
    }
    Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(Dimens.gap)) {
        Tile("Lux pokój", Logic.fmtLux(st.luxRoom), Modifier.weight(1f))
        Tile("Lux nad wodą", Logic.fmtLux(st.luxNadWoda), Modifier.weight(1f))
    }
    Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(Dimens.gap)) {
        Tile("Moc LED", st.pct?.let { "${it.roundToInt()} %" } ?: "—", Modifier.weight(1f))
        Tile("Pobór teraz", Logic.fmtWatts(st.powerNowW), Modifier.weight(1f))
    }
    Panel(title = "Sterowanie") {
        SwitchRow("Zasilanie", st.power) { on -> vm.sendPlain(if (on) "power_on" else "power_off") }
        HorizontalDivider(color = Pal.Border)
        SwitchRow("Pompa", st.pumpOn) { on -> vm.sendPlain(if (on) "pump_on" else "pump_off") }
        HorizontalDivider(color = Pal.Border)
        Text("Tryb pracy", style = Txt.note)
        val sel = if (st.mode == "MANUAL") 1 else 0
        Segmented(listOf("Automatyczny", "Ręczny"), sel) { i ->
            if (i != sel) vm.sendPlain(if (i == 0) "mode_auto" else "mode_manual")
        }
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
private fun Hero(st: RybyStatus, stale: Boolean) {
    val edge = if (stale) Pal.Warn.copy(alpha = 0.45f) else Pal.Cyan.copy(alpha = 0.22f)
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(Dimens.radiusTile))
            .background(Brush.linearGradient(listOf(Pal.HeroA, Pal.HeroB)))
            .border(1.dp, edge, RoundedCornerShape(Dimens.radiusTile))
            .padding(18.dp),
        verticalArrangement = Arrangement.spacedBy(10.dp)
    ) {
        Text("TEMPERATURA · PŁYTKA 1", style = Txt.heroLabel)
        Text(Logic.fmtTemp(st.temps.getOrNull(0)), style = Txt.heroTemp)
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            Pill(if (st.power) "ZASILANIE WŁ." else "ZASILANIE WYŁ.", if (st.power) Pal.Live else Pal.TextDim)
            Pill(if (st.mode == "MANUAL") "TRYB RĘCZNY" else "TRYB AUTO", Pal.Cyan)
        }
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
                Text("Kanał ${i + 1}", style = Txt.body)
                Text("${v.roundToInt()}", style = Txt.bodyBold, color = Pal.Cyan)
            }
            Slider(
                value = v,
                onValueChange = { nv ->
                    pwm = pwm.toMutableList().also { it[i] = nv }
                    dirty = true
                },
                valueRange = 0f..Logic.PWM_MAX.toFloat(),
                colors = SliderDefaults.colors(
                    thumbColor = Pal.Cyan,
                    activeTrackColor = Pal.Cyan,
                    inactiveTrackColor = Pal.Surface3
                )
            )
        }
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(Dimens.gap)) {
            PrimaryButton("Zastosuj", Modifier.weight(1f)) {
                vm.sendPwm(pwm.map { it.toDouble() })
                dirty = false
            }
            SecondaryButton("Wczytaj z ESP", Modifier.weight(1f)) {
                if (st != null) pwm = st.pwm.map { it.toFloat() }
                dirty = false
            }
        }
    }
    Panel(title = "Szybkie ustawienia") {
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(Dimens.gap)) {
            PrimaryButton("LED 100 %", Modifier.weight(1f)) { vm.sendPlain("led_100") }
            SecondaryButton("LED wyłączony", Modifier.weight(1f)) { vm.sendPlain("led_off") }
        }
    }
    Panel(title = "Zapis") {
        Text("Zapisuje obecne suwaki jako wartości domyślne na sterowniku.", style = Txt.note)
        PrimaryButton("Zapisz jako domyślne", Modifier.fillMaxWidth()) { askAutosave = true }
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
            style = Txt.note
        )
    }
    Panel(title = "Przedziały pracy · tylko odczyt") {
        val slots = st?.pumpSlots ?: emptyList()
        if (slots.isEmpty()) {
            Text("Brak przedziałów", style = Txt.note)
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
        Panel { Text("Czekam na pierwszy odczyt z bazy…", style = Txt.note) }
        return
    }
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(Dimens.radiusTile))
            .background(Brush.linearGradient(listOf(Pal.HeroA, Pal.HeroB)))
            .border(1.dp, Pal.Cyan.copy(alpha = 0.22f), RoundedCornerShape(Dimens.radiusTile))
            .padding(18.dp),
        verticalArrangement = Arrangement.spacedBy(6.dp)
    ) {
        Text("ENERGIA DZIŚ", style = Txt.heroLabel)
        Text(Logic.fmtWh(st.energy.today), style = Txt.heroTemp)
        Text("Koszt: ${Logic.fmtPln(Logic.costPln(st.energy.today, st.kwhPrice))}", style = Txt.note)
    }
    Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(Dimens.gap)) {
        Tile("Tydzień", Logic.fmtWh(st.energy.week), Modifier.weight(1f))
        Tile("Miesiąc", Logic.fmtWh(st.energy.month), Modifier.weight(1f))
    }
    Panel(title = "Ostatnie dni") {
        val days = st.dayHistory.takeLast(7)
        if (days.isEmpty()) Text("Brak historii", style = Txt.note) else BarChart(days)
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
        Text("Sekrety zapisują się tylko na tym telefonie, nie w kodzie aplikacji.", style = Txt.note)
        OutlinedTextField(
            value = dbUrl,
            onValueChange = { dbUrl = it },
            label = { Text("Adres bazy (HTTPS)") },
            singleLine = true,
            colors = fieldColors(),
            modifier = Modifier.fillMaxWidth()
        )
        OutlinedTextField(
            value = secret,
            onValueChange = { secret = it },
            label = { Text("Database Secret") },
            singleLine = true,
            visualTransformation = PasswordVisualTransformation(),
            colors = fieldColors(),
            modifier = Modifier.fillMaxWidth()
        )
        OutlinedTextField(
            value = token,
            onValueChange = { token = it },
            label = { Text("CMD_TOKEN") },
            singleLine = true,
            visualTransformation = PasswordVisualTransformation(),
            colors = fieldColors(),
            modifier = Modifier.fillMaxWidth()
        )
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(Dimens.gap)) {
            PrimaryButton("Zapisz", Modifier.weight(1f)) { vm.saveSettings(dbUrl, secret, token) }
            PrimaryButton("Sprawdź połączenie", Modifier.weight(1f)) { vm.testConnection(dbUrl, secret, token) }
        }
        SecondaryButton("Wyczyść sekrety z tego telefonu", Modifier.fillMaxWidth()) {
            vm.forgetSecrets()
            secret = ""
            token = ""
        }
    }
    Panel(title = "Sterownik") {
        DangerButton("Restart ESP", Modifier.fillMaxWidth()) { askRestart = true }
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
private fun Panel(title: String? = null, content: @Composable ColumnScope.() -> Unit) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(Dimens.radiusCard))
            .background(Pal.Surface)
            .border(1.dp, Pal.Border, RoundedCornerShape(Dimens.radiusCard))
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        if (title != null) Text(title.uppercase(), style = Txt.section)
        content()
    }
}

@Composable
private fun Tile(label: String, value: String, modifier: Modifier = Modifier, tone: Color = Pal.Border) {
    Column(
        modifier = modifier
            .clip(RoundedCornerShape(Dimens.radiusTile))
            .background(Brush.linearGradient(listOf(Pal.TileA, Pal.TileB)))
            .border(1.dp, tone, RoundedCornerShape(Dimens.radiusTile))
            .padding(14.dp),
        verticalArrangement = Arrangement.spacedBy(6.dp)
    ) {
        Text(label, style = Txt.tileTitle)
        Text(value, style = Txt.tileValue)
    }
}

@Composable
private fun KV(label: String, value: String) {
    Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
        Text(label, style = Txt.body, color = Pal.TextDim)
        Text(value, style = Txt.body)
    }
}

@Composable
private fun SwitchRow(label: String, checked: Boolean, onChange: (Boolean) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().heightIn(min = Dimens.controlH),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(label, style = Txt.bodyBold)
        Switch(
            checked = checked,
            onCheckedChange = onChange,
            colors = SwitchDefaults.colors(
                checkedThumbColor = Pal.OnCyan,
                checkedTrackColor = Pal.Cyan,
                uncheckedThumbColor = Pal.TextDim,
                uncheckedTrackColor = Pal.Surface3,
                uncheckedBorderColor = Pal.BorderStrong
            )
        )
    }
}

@Composable
private fun Segmented(options: List<String>, selected: Int, onSelect: (Int) -> Unit) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(Dimens.radiusBanner))
            .background(Pal.Bg)
            .padding(4.dp),
        horizontalArrangement = Arrangement.spacedBy(4.dp)
    ) {
        options.forEachIndexed { i, label ->
            val isSel = i == selected
            Box(
                modifier = Modifier
                    .weight(1f)
                    .clip(RoundedCornerShape(10.dp))
                    .background(if (isSel) Pal.Cyan else Color.Transparent)
                    .clickable { onSelect(i) }
                    .heightIn(min = Dimens.controlH)
                    .padding(vertical = 12.dp),
                contentAlignment = Alignment.Center
            ) {
                Text(
                    label,
                    style = Txt.btn,
                    color = if (isSel) Pal.OnCyan else Pal.TextDim
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
        values.forEachIndexed { i, v ->
            val frac = if (max > 0.0) (v / max).toFloat().coerceIn(0.04f, 1f) else 0.04f
            val isToday = i == values.lastIndex
            Box(
                modifier = Modifier
                    .weight(1f)
                    .fillMaxHeight(frac)
                    .clip(RoundedCornerShape(topStart = 6.dp, topEnd = 6.dp))
                    .background(if (isToday) Pal.Cyan else Pal.Cyan.copy(alpha = 0.35f))
            )
        }
    }
}

@Composable
private fun Banner(text: String, color: Color, onDismiss: (() -> Unit)? = null) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(Dimens.radiusBanner))
            .background(color.copy(alpha = 0.10f))
            .border(1.dp, color.copy(alpha = 0.38f), RoundedCornerShape(Dimens.radiusBanner))
            .padding(horizontal = 14.dp, vertical = 10.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(text, style = Txt.note, color = color, modifier = Modifier.weight(1f))
        if (onDismiss != null) TextButton(onClick = onDismiss) { Text("OK", color = color) }
    }
}

@Composable
private fun PrimaryButton(label: String, modifier: Modifier = Modifier, onClick: () -> Unit) {
    Button(
        onClick = onClick,
        modifier = modifier.heightIn(min = Dimens.controlH),
        colors = ButtonDefaults.buttonColors(containerColor = Pal.Cyan, contentColor = Pal.OnCyan)
    ) {
        Text(label, style = Txt.btn)
    }
}

@Composable
private fun SecondaryButton(label: String, modifier: Modifier = Modifier, onClick: () -> Unit) {
    OutlinedButton(
        onClick = onClick,
        modifier = modifier.heightIn(min = Dimens.controlH),
        border = BorderStroke(1.dp, Pal.BorderStrong),
        colors = ButtonDefaults.outlinedButtonColors(contentColor = Pal.Text)
    ) {
        Text(label, style = Txt.btn)
    }
}

@Composable
private fun DangerButton(label: String, modifier: Modifier = Modifier, onClick: () -> Unit) {
    Button(
        onClick = onClick,
        modifier = modifier.heightIn(min = Dimens.controlH),
        colors = ButtonDefaults.buttonColors(containerColor = Pal.Err, contentColor = Pal.OnCyan)
    ) {
        Text(label, style = Txt.btn)
    }
}

@Composable
private fun fieldColors() = OutlinedTextFieldDefaults.colors(
    focusedTextColor = Pal.Text,
    unfocusedTextColor = Pal.Text,
    focusedBorderColor = Pal.Cyan,
    unfocusedBorderColor = Pal.BorderStrong,
    focusedLabelColor = Pal.Cyan,
    unfocusedLabelColor = Pal.TextDim,
    cursorColor = Pal.Cyan
)

@Composable
private fun ConfirmDialog(title: String, text: String, onConfirm: () -> Unit, onDismiss: () -> Unit) {
    AlertDialog(
        onDismissRequest = onDismiss,
        containerColor = Pal.Surface2,
        title = { Text(title, style = Txt.bodyBold) },
        text = { Text(text, style = Txt.note) },
        confirmButton = { TextButton(onClick = onConfirm) { Text("Tak", color = Pal.Cyan) } },
        dismissButton = { TextButton(onClick = onDismiss) { Text("Anuluj", color = Pal.TextDim) } }
    )
}
