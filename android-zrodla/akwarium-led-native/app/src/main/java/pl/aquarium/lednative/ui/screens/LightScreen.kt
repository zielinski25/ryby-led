package pl.aquarium.lednative.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Brightness6
import androidx.compose.material.icons.filled.Tune
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import pl.aquarium.lednative.core.Logic
import pl.aquarium.lednative.ui.AppViewModel
import pl.aquarium.lednative.ui.Cyan
import pl.aquarium.lednative.ui.Warm
import pl.aquarium.lednative.ui.components.CardHeader
import pl.aquarium.lednative.ui.components.InfoBanner
import pl.aquarium.lednative.ui.components.Pill
import pl.aquarium.lednative.ui.components.PrimaryButton
import pl.aquarium.lednative.ui.components.PwmSlider
import pl.aquarium.lednative.ui.components.ScreenTitle
import pl.aquarium.lednative.ui.components.SectionCard
import pl.aquarium.lednative.ui.components.SettingRow
import androidx.compose.material3.Switch
import androidx.compose.material3.SwitchDefaults
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Text
import androidx.compose.foundation.clickable
import androidx.compose.ui.Alignment
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.sp
import kotlin.math.roundToInt
import pl.aquarium.lednative.ui.Border
import pl.aquarium.lednative.ui.Surface3
import pl.aquarium.lednative.ui.TextDim
import pl.aquarium.lednative.ui.TextMid
import pl.aquarium.lednative.ui.TextHi

@Composable
fun LightScreen(vm: AppViewModel) {
    val s = vm.status
    var pwm by remember { mutableStateOf(List(Logic.PWM_CHANNELS) { 0 }) }
    var loaded by remember { mutableStateOf(false) }
    var channelsExpanded by remember { mutableStateOf(false) }

    // Pierwszy odczyt wypelnia suwaki; pozniej nie nadpisujemy edycji uzytkownika.
    LaunchedEffect(s?.pwm) {
        if (!loaded && s != null) {
            pwm = s.pwm
            loaded = true
        }
    }

    Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
        ScreenTitle("Światło", "Pięć kanałów LED · PWM 0–1023") {
            Pill(if (s?.auto != false) "AUTO" else "MANUAL", if (s?.auto != false) Cyan else Warm)
        }

        if (s?.auto != false) {
            InfoBanner(
                "Sterownik jest w trybie AUTO — harmonogram nadpisuje ręczne PWM. " +
                    "Przełącz na MANUAL, aby suwaki działały na stałe.",
                warn = true
            )
        }

        SectionCard {
            CardHeader("Tryb pracy", Icons.Default.Tune)
            SettingRow(
                "Automatyczny harmonogram",
                if (s?.auto != false) "Godziny sterują jasnością" else "Ręczne PWM z aplikacji"
            ) {
                Switch(
                    checked = s?.auto != false,
                    onCheckedChange = { vm.toggleMode() },
                    enabled = !vm.busy,
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = Cyan, checkedTrackColor = Cyan.copy(alpha = 0.35f),
                        uncheckedThumbColor = TextDim, uncheckedTrackColor = Surface3,
                        uncheckedBorderColor = Border
                    )
                )
            }
            SettingRow(
                "Zasilanie LED",
                if (s?.power == true) "Włączone" else "Wyłączone"
            ) {
                Switch(
                    checked = s?.power == true,
                    onCheckedChange = { vm.togglePower() },
                    enabled = !vm.busy,
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = Warm, checkedTrackColor = Warm.copy(alpha = 0.35f),
                        uncheckedThumbColor = TextDim, uncheckedTrackColor = Surface3,
                        uncheckedBorderColor = Border
                    )
                )
            }
        }

        SectionCard {
            CardHeader("Sterowanie PWM", Icons.Default.Brightness6, Warm) {
                Pill("0–1023", Cyan)
            }
            Column(verticalArrangement = Arrangement.spacedBy(5.dp)) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text("★ Wszystkie kanały", fontSize = 8.sp, color = TextHi, fontWeight = FontWeight.Bold, modifier = Modifier.weight(1f))
                    Text("${pwm.average().roundToInt()} / 1023", fontSize = 8.sp, color = Cyan, fontFamily = FontFamily.Monospace)
                }
                Slider(
                    value = pwm.average().toFloat(),
                    onValueChange = { raw ->
                        val v = raw.roundToInt().coerceIn(0, Logic.PWM_MAX)
                        pwm = List(Logic.PWM_CHANNELS) { v }
                    },
                    onValueChangeFinished = { vm.applyPwm(pwm) },
                    valueRange = 0f..Logic.PWM_MAX.toFloat(),
                    enabled = vm.tokenReady,
                    colors = SliderDefaults.colors(
                        thumbColor = Cyan, activeTrackColor = Cyan,
                        inactiveTrackColor = Border, disabledThumbColor = TextDim,
                        disabledActiveTrackColor = TextDim, disabledInactiveTrackColor = Border
                    )
                )
            }
            Row(
                Modifier.fillMaxWidth().clickable { channelsExpanded = !channelsExpanded }.padding(vertical = 6.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text("Rozwiń osobne kanały", fontSize = 8.sp, color = TextMid, modifier = Modifier.weight(1f))
                Text("5 kanałów  ${if (channelsExpanded) "⌃" else "⌄"}", fontSize = 7.sp, color = TextDim, fontFamily = FontFamily.Monospace)
            }
            if (channelsExpanded) {
                (0 until Logic.PWM_CHANNELS).forEach { i ->
                    PwmSlider(
                        index = i,
                        value = pwm.getOrElse(i) { 0 },
                        enabled = vm.tokenReady,
                        onChange = { v -> pwm = pwm.toMutableList().also { it[i] = v } },
                        onChangeFinished = { vm.applyPwm(pwm) }
                    )
                }
            }
            PrimaryButton(
                if (vm.busy) "Wysyłanie…" else "Zapisz",
                enabled = !vm.busy && vm.tokenReady
            ) { vm.applyPwm(pwm) }
        }
    }
}
