package pl.aquarium.lednative.ui

import androidx.compose.animation.Crossfade
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.asPaddingValues
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.navigationBars
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.statusBars
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Article
import androidx.compose.material.icons.filled.Dashboard
import androidx.compose.material.icons.filled.MoreHoriz
import androidx.compose.material.icons.filled.Schedule
import androidx.compose.material.icons.filled.Sensors
import androidx.compose.material.icons.filled.ShowChart
import androidx.compose.material.icons.filled.Code
import androidx.compose.material.icons.filled.WbSunny
import androidx.compose.material3.Icon
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Snackbar
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import pl.aquarium.lednative.ui.screens.ChartsScreen
import pl.aquarium.lednative.ui.screens.DashboardScreen
import pl.aquarium.lednative.ui.screens.LightScreen
import pl.aquarium.lednative.ui.screens.LogsScreen
import pl.aquarium.lednative.ui.screens.MoreScreen
import pl.aquarium.lednative.ui.screens.ScheduleScreen
import pl.aquarium.lednative.ui.screens.SensorsScreen
import pl.aquarium.lednative.ui.screens.TerminalScreen

private data class Tab(val key: String, val label: String, val icon: ImageVector)

private val TABS = listOf(
    Tab("dash", "Pulpit", Icons.Default.Dashboard),
    Tab("light", "Światło", Icons.Default.WbSunny),
    Tab("plan", "Plan", Icons.Default.Schedule),
    Tab("sensors", "Czujniki", Icons.Default.Sensors),
    Tab("charts", "Wykresy", Icons.Default.ShowChart),
    Tab("logs", "Logi", Icons.Default.Article),
    Tab("terminal", "Terminal", Icons.Default.Code),
    Tab("more", "Więcej", Icons.Default.MoreHoriz)
)

@Composable
fun AppRoot(vm: AppViewModel) {
    var tab by rememberSaveable { mutableStateOf("dash") }
    val snackbar = remember { SnackbarHostState() }
    val scroll = rememberScrollState()

    LaunchedEffect(vm.message) {
        vm.message?.let {
            snackbar.showSnackbar(it)
            vm.clearMessage()
        }
    }
    LaunchedEffect(tab) { scroll.scrollTo(0) }

    // Tło aplikacji (jak w HTML - ciemnoszare #e9eff1)
    Box(
        modifier = Modifier.fillMaxSize().background(Color(0xFFE9EFF1))
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 24.dp, vertical = 20.dp)
                .verticalScroll(scroll),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(20.dp)
        ) {
            // Tytuł i opis (jak w HTML intro)
            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                Text(
                    "Akwarium LED",
                    fontSize = 28.sp,
                    fontWeight = FontWeight.Bold,
                    color = Color(0xFF172A31)
                )
                Text(
                    "Aplikacja kafelkowa · sterowanie i podgląd",
                    fontSize = 13.sp,
                    color = Color(0xFF607279)
                )
            }

            // Urządzenie (jak w HTML device - czarne, zaokrąglone rogi)
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(850.dp)
                    .clip(RoundedCornerShape(39.dp))
                    .background(Color(0xFF111820))
                    .border(1.dp, Color(0xFF77868A), RoundedCornerShape(39.dp))
            ) {
                Column(
                    modifier = Modifier.fillMaxSize().padding(8.dp)
                ) {
                    // Pasek statusu (jak w HTML sysline)
                    Row(
                        modifier = Modifier.fillMaxWidth().padding(horizontal = 14.dp, vertical = 4.dp),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Text(
                            "13:27",
                            fontSize = 9.sp,
                            color = Color(0xFF9CB5C3),
                            fontWeight = FontWeight.Medium,
                            fontFamily = FontFamily.Monospace
                        )
                        Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.End) {
                            Text(
                                "WiFi · Bateria",
                                fontSize = 9.sp,
                                color = Color(0xFF9CB5C3),
                                fontFamily = FontFamily.Monospace
                            )
                        }
                    }

                    // AppBar (jak w HTML appbar)
                    Row(
                        modifier = Modifier.fillMaxWidth().padding(horizontal = 14.dp, vertical = 10.dp),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.spacedBy(9.dp)
                    ) {
                        Box(
                            Modifier.size(34.dp).clip(RoundedCornerShape(11.dp))
                                .background(Cyan.copy(alpha = 0.16f)),
                            contentAlignment = Alignment.Center
                        ) {
                            Icon(Icons.Default.WbSunny, null, tint = Cyan, modifier = Modifier.size(19.dp))
                        }
                        Column(Modifier.weight(1f)) {
                            Text(
                                "Akwarium LED",
                                fontSize = 15.sp,
                                fontWeight = FontWeight.Bold,
                                color = Cyan
                            )
                            Text(
                                "100% natywnie · ${vm.source}",
                                fontSize = 8.sp,
                                color = Color(0xFF66829B),
                                fontFamily = FontFamily.Monospace
                            )
                        }
                        Box(
                            Modifier.clip(RoundedCornerShape(10.dp))
                                .background(Surface2)
                                .clickable(enabled = !vm.refreshing) { vm.refresh() }
                                .padding(horizontal = 10.dp, vertical = 7.dp)
                        ) {
                            Text(
                                if (vm.refreshing) "…" else "Odśwież",
                                fontSize = 10.sp,
                                color = Cyan,
                                fontWeight = FontWeight.SemiBold
                            )
                        }
                    }

                    // Zawartość ekranu (jak w HTML screen)
                    Column(
                        modifier = Modifier
                            .fillMaxSize()
                            .padding(13.dp)
                            .verticalScroll(scroll)
                    ) {
                        Crossfade(targetState = tab, animationSpec = tween(180), label = "tab") { current ->
                            when (current) {
                                "dash" -> DashboardScreen(vm) { tab = it }
                                "light" -> LightScreen(vm)
                                "plan" -> ScheduleScreen(vm)
                                "sensors" -> SensorsScreen(vm)
                                "charts" -> ChartsScreen(vm)
                                "logs" -> LogsScreen(vm)
                                "terminal" -> TerminalScreen(vm)
                                else -> MoreScreen(vm)
                            }
                        }
                    }
                }

                // Nawigacja dolna (jak w HTML bottomNav - osobny pasek pod urządzeniem)
                BottomNav(
                    selected = tab,
                    onSelect = { tab = it },
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(horizontal = 14.dp, vertical = 10.dp)
                )
            }
        }
    }
}

@Composable
private fun BottomNav(selected: String, onSelect: (String) -> Unit, modifier: Modifier = Modifier) {
    // Osiem zakładek w dwóch rzędach po cztery — czytelne na telefonie (jak w HTML).
    Column(
        modifier.fillMaxWidth().background(Surface1)
            .padding(top = 1.dp)
    ) {
        Box(Modifier.fillMaxWidth().height(1.dp).background(Border))
        TABS.chunked(4).forEach { row ->
            Row(Modifier.fillMaxWidth()) {
                row.forEach { t ->
                    val active = t.key == selected
                    Column(
                        Modifier.weight(1f)
                            .clickable { onSelect(t.key) }
                            .padding(vertical = 7.dp),
                        horizontalAlignment = Alignment.CenterHorizontally,
                        verticalArrangement = Arrangement.spacedBy(2.dp)
                    ) {
                        Box(
                            Modifier.clip(RoundedCornerShape(9.dp))
                                .background(if (active) Cyan.copy(alpha = 0.16f) else androidx.compose.ui.graphics.Color.Transparent)
                                .padding(horizontal = 12.dp, vertical = 3.dp)
                        ) {
                            Icon(
                                t.icon, t.label,
                                tint = if (active) Cyan else TextDim,
                                modifier = Modifier.size(18.dp)
                            )
                        }
                        Text(
                            t.label, fontSize = 9.sp,
                            color = if (active) Cyan else TextDim,
                            fontWeight = if (active) FontWeight.Bold else FontWeight.Medium,
                            textAlign = TextAlign.Center, maxLines = 1
                        )
                    }
                }
            }
        }
    }
}
