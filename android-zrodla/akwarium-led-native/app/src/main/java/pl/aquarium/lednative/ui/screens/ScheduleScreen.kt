package pl.aquarium.lednative.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.Schedule
import androidx.compose.material.icons.filled.Waves
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import pl.aquarium.lednative.core.LedSchedule
import pl.aquarium.lednative.core.Logic
import pl.aquarium.lednative.core.PumpSlot
import pl.aquarium.lednative.ui.AppViewModel
import pl.aquarium.lednative.ui.Blue
import pl.aquarium.lednative.ui.Cyan
import pl.aquarium.lednative.ui.Red
import pl.aquarium.lednative.ui.TextDim
import pl.aquarium.lednative.ui.Warm
import pl.aquarium.lednative.ui.components.CardHeader
import pl.aquarium.lednative.ui.components.GhostButton
import pl.aquarium.lednative.ui.components.InfoBanner
import pl.aquarium.lednative.ui.components.KeyValue
import pl.aquarium.lednative.ui.components.NumberField
import pl.aquarium.lednative.ui.components.Pill
import pl.aquarium.lednative.ui.components.PrimaryButton
import pl.aquarium.lednative.ui.components.ScreenTitle
import pl.aquarium.lednative.ui.components.SectionCard
import pl.aquarium.lednative.ui.components.SegmentedTabs
import pl.aquarium.lednative.ui.components.TextFieldStyled
import pl.aquarium.lednative.ui.components.TimeField

@Composable
fun ScheduleScreen(vm: AppViewModel) {
    val s = vm.status
    var tab by remember { mutableStateOf("led") }
    var sched by remember { mutableStateOf(LedSchedule()) }
    var fade by remember { mutableStateOf("30") }
    var slots by remember { mutableStateOf(listOf(PumpSlot("08:00", "10:00"))) }
    var ledLoaded by remember { mutableStateOf(false) }
    var pumpLoaded by remember { mutableStateOf(false) }

    LaunchedEffect(s?.schedule) {
        if (!ledLoaded && s != null) {
            sched = s.schedule
            fade = s.fadeMinutes.toString()
            ledLoaded = true
        }
    }
    LaunchedEffect(s?.pumpSlots) {
        if (!pumpLoaded && s != null && s.pumpSlots.isNotEmpty()) {
            slots = s.pumpSlots
            pumpLoaded = true
        }
    }

    Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
        ScreenTitle("Plan", "Harmonogram światła i pompki") {
            Pill(if (s?.auto != false) "AUTO" else "MANUAL", if (s?.auto != false) Cyan else Warm)
        }
        SegmentedTabs(listOf("led" to "Światło", "pump" to "Pompka"), tab) { tab = it }

        if (tab == "led") {
            SectionCard {
                CardHeader("Harmonogram dobowy", Icons.Default.Schedule)
                TimeField("Start w tygodniu (WD)", sched.morningWD) {
                    sched = sched.copy(morningWD = it)
                }
                TimeField("Start w weekend (WE)", sched.morningWE) {
                    sched = sched.copy(morningWE = it)
                }
                TimeField("Wyłączenie w południe", sched.middayOff) {
                    sched = sched.copy(middayOff = it)
                }
                NumberField(
                    "Wieczór: minuty względem zachodu",
                    sched.eveningBefore.toString(),
                    { v -> v.toIntOrNull()?.let { sched = sched.copy(eveningBefore = it.coerceIn(-180, 180)) } },
                    hint = "Zakres −180…180. Ujemne = przed zachodem słońca."
                )
                NumberField(
                    "Rampa (fade) w minutach", fade, { fade = it },
                    hint = "Zakres 1–180 min. Czas narastania i wygaszania."
                )
                KeyValue("Zachód słońca (z ESP)", Logic.fmtHHMM(s?.schedule?.sunsetMin))
                KeyValue(
                    "Start wieczorny",
                    Logic.shiftedTime(s?.schedule?.sunsetMin, sched.eveningBefore)
                )
                KeyValue(
                    "Wyłączenie wieczorne",
                    s?.schedule?.eveningOff?.let { Logic.fmtHHMM(it) } ?: "—"
                )
                PrimaryButton(
                    if (vm.busy) "Wysyłanie…" else "Zapisz harmonogram światła",
                    enabled = !vm.busy && vm.tokenReady
                ) {
                    val f = fade.toIntOrNull()?.coerceIn(1, 180)
                    if (f == null) vm.notify("Rampa musi być liczbą 1–180")
                    else vm.saveSchedule(sched, f)
                }
            }
            InfoBanner(
                "Zmiana trafia do Firebase jako config (configType=schedule) plus komenda " +
                    "config_refresh. ESP pobiera kolejkę mniej więcej co 14 sekund."
            )
        } else {
            SectionCard {
                CardHeader("Okna pracy pompki", Icons.Default.Waves, Blue) {
                    Pill("${slots.size}/6", Blue)
                }
                slots.forEachIndexed { i, slot ->
                    Row(
                        Modifier.fillMaxWidth(),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.spacedBy(6.dp)
                    ) {
                        TextFieldStyled(
                            "Okno ${i + 1} — od", slot.start,
                            { v -> slots = slots.toMutableList().also { it[i] = slot.copy(start = v) } },
                            Modifier.weight(1f)
                        )
                        TextFieldStyled(
                            "do", slot.end,
                            { v -> slots = slots.toMutableList().also { it[i] = slot.copy(end = v) } },
                            Modifier.weight(1f)
                        )
                        IconButton(
                            onClick = { slots = slots.filterIndexed { idx, _ -> idx != i } },
                            enabled = slots.size > 1
                        ) {
                            Icon(Icons.Default.Delete, "Usuń okno", tint = Red)
                        }
                    }
                    Text(
                        "Czas pracy: ${Logic.fmtMinutes(Logic.durationBetween(slot.start, slot.end))}",
                        fontSize = 10.sp, color = TextDim,
                        modifier = Modifier.padding(start = 2.dp, bottom = 4.dp)
                    )
                }
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    GhostButton("+ Dodaj okno", Modifier.weight(1f), enabled = slots.size < 6) {
                        slots = slots + PumpSlot("12:00", "13:00")
                    }
                    GhostButton("Z ESP", Modifier.weight(1f)) {
                        s?.pumpSlots?.takeIf { it.isNotEmpty() }?.let { slots = it }
                        vm.refresh()
                    }
                }
                PrimaryButton(
                    if (vm.busy) "Wysyłanie…" else "Zapisz harmonogram pompki",
                    enabled = !vm.busy && vm.tokenReady
                ) {
                    val bad = slots.firstOrNull { !Logic.HHMM.matches(it.start) || !Logic.HHMM.matches(it.end) }
                    if (slots.isEmpty()) vm.notify("Dodaj co najmniej jedno okno")
                    else if (bad != null) vm.notify("Godziny muszą mieć format HH:MM (00:00–23:59)")
                    else vm.savePumpSlots(slots)
                }
            }
            InfoBanner(
                "Firmware przyjmuje maksymalnie 6 okien. Okno może przechodzić przez północ " +
                    "(np. 23:00 → 01:00)."
            )
        }
    }
}
