package pl.aquarium.lednative.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Article
import androidx.compose.material.icons.filled.Folder
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import pl.aquarium.lednative.ui.AppViewModel
import pl.aquarium.lednative.ui.Cyan
import pl.aquarium.lednative.ui.FsBlue
import pl.aquarium.lednative.ui.Green
import pl.aquarium.lednative.ui.Red
import pl.aquarium.lednative.ui.Surface2
import pl.aquarium.lednative.ui.TextDim
import pl.aquarium.lednative.ui.TextHi
import pl.aquarium.lednative.ui.TextMid
import pl.aquarium.lednative.ui.Warm
import pl.aquarium.lednative.ui.components.CardHeader
import pl.aquarium.lednative.ui.components.EmptyState
import pl.aquarium.lednative.ui.components.GhostButton
import pl.aquarium.lednative.ui.components.InfoBanner
import pl.aquarium.lednative.ui.components.KeyValue
import pl.aquarium.lednative.ui.components.Pill
import pl.aquarium.lednative.ui.components.ScreenTitle
import pl.aquarium.lednative.ui.components.SectionCard
import pl.aquarium.lednative.ui.components.SegmentedTabs

@Composable
fun LogsScreen(vm: AppViewModel) {
    var tab by remember { mutableStateOf("log") }
    var filter by remember { mutableStateOf("all") }
    var started by remember { mutableStateOf(false) }

    LaunchedEffect(Unit) {
        if (!started && vm.lanBase != null) {
            started = true
            vm.loadLogs()
            vm.loadFs()
        }
    }

    val shown = vm.logs.filter { filter == "all" || it.category == filter }

    Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
        ScreenTitle("Logi", "Dziennik ESP32 i pliki LittleFS") {
            Pill(if (vm.lanBase != null) "LAN" else "BRAK LAN", if (vm.lanBase != null) Green else Warm)
        }

        if (vm.lanBase == null) {
            InfoBanner(
                "Logi i pliki są dostępne wyłącznie po LAN. Wejdź w Więcej → Wykryj ESP " +
                    "będąc w tej samej sieci WiFi co sterownik.",
                warn = true
            )
            GhostButton("Przejdź do wykrywania", Modifier.fillMaxWidth()) { vm.discoverLan() }
        }

        SegmentedTabs(listOf("log" to "Dziennik", "fs" to "Pliki"), tab) { tab = it }

        if (tab == "log") {
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                GhostButton(
                    if (vm.logsLoading) "Wczytywanie…" else "Odśwież",
                    Modifier.weight(1f), enabled = !vm.logsLoading
                ) { vm.loadLogs() }
                GhostButton("Eksport", Modifier.weight(1f), color = Cyan) { vm.exportLogs() }
                GhostButton("Wyczyść", Modifier.weight(1f), color = Red) { vm.clearLogs() }
            }
            SegmentedTabs(
                listOf(
                    "all" to "Wszystko", "err" to "Błędy", "warn" to "Ostrz.",
                    "led" to "LED", "temp" to "Temp."
                ),
                filter
            ) { filter = it }

            SectionCard {
                CardHeader("Wpisy (${shown.size})", Icons.Default.Article) {
                    Pill("${vm.logs.size} linii", FsBlue)
                }
                if (shown.isEmpty()) {
                    EmptyState(
                        "Brak wpisów",
                        "Naciśnij Odśwież. Log pochodzi z /api/log-lines na ESP (tekst, nie JSON)."
                    )
                } else {
                    LazyColumn(
                        Modifier.fillMaxWidth().heightIn(max = 460.dp),
                        verticalArrangement = Arrangement.spacedBy(4.dp)
                    ) {
                        items(shown) { entry ->
                            val c = when (entry.category) {
                                "err" -> Red
                                "warn" -> Warm
                                else -> TextMid
                            }
                            Column(
                                Modifier.fillMaxWidth().clip(RoundedCornerShape(8.dp))
                                    .background(Surface2).padding(8.dp)
                            ) {
                                Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                                    Text(entry.time.ifBlank { "—" }, fontSize = 9.sp, color = TextDim)
                                    Text(
                                        entry.level, fontSize = 9.sp, color = c,
                                        fontFamily = FontFamily.Monospace
                                    )
                                }
                                Text(
                                    entry.raw, fontSize = 10.sp, color = TextHi,
                                    fontFamily = FontFamily.Monospace, lineHeight = 14.sp
                                )
                            }
                        }
                    }
                }
            }
        } else {
            GhostButton(
                if (vm.fsLoading) "Wczytywanie…" else "Odśwież listę plików",
                Modifier.fillMaxWidth(), enabled = !vm.fsLoading
            ) { vm.loadFs() }
            SectionCard {
                CardHeader("LittleFS", Icons.Default.Folder, FsBlue)
                KeyValue("Zajęte", formatBytes(vm.fs.used))
                KeyValue("Razem", formatBytes(vm.fs.total))
                KeyValue("Plików", vm.fs.files.size.toString())
            }
            SectionCard {
                CardHeader("Pliki", Icons.Default.Article)
                if (vm.fs.files.isEmpty()) {
                    EmptyState("Brak plików", "Lista pochodzi z /api/fs-list na ESP.")
                } else {
                    vm.fs.files.forEach { f ->
                        Row(
                            Modifier.fillMaxWidth().clip(RoundedCornerShape(10.dp))
                                .background(Surface2).padding(10.dp),
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.spacedBy(8.dp)
                        ) {
                            Column(Modifier.weight(1f)) {
                                Text(f.name, fontSize = 12.sp, color = TextHi)
                                Text(formatBytes(f.size), fontSize = 10.sp, color = TextDim)
                            }
                            GhostButton("Podgląd") { vm.openFile(f.name) }
                        }
                    }
                }
            }
            vm.filePreview?.let { (name, text) ->
                SectionCard {
                    CardHeader(name, Icons.Default.Article, Cyan) {
                        Pill("${text.length} B", Cyan)
                    }
                    val scroll = rememberScrollState()
                    Box(
                        Modifier.fillMaxWidth().heightIn(max = 320.dp)
                            .clip(RoundedCornerShape(10.dp)).background(Surface2)
                            .verticalScroll(scroll).padding(10.dp)
                    ) {
                        Text(
                            text, fontSize = 9.sp, color = TextMid,
                            fontFamily = FontFamily.Monospace, lineHeight = 13.sp
                        )
                    }
                    GhostButton("Zamknij podgląd", Modifier.fillMaxWidth()) { vm.closeFile() }
                }
            }
        }
    }
}

private fun formatBytes(b: Long): String = when {
    b <= 0 -> "—"
    b < 1024 -> "$b B"
    b < 1024 * 1024 -> String.format(java.util.Locale.US, "%.1f KB", b / 1024.0)
    else -> String.format(java.util.Locale.US, "%.2f MB", b / (1024.0 * 1024.0))
}
