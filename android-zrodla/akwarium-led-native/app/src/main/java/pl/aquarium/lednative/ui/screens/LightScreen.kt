package pl.aquarium.lednative.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Brightness6
import androidx.compose.material.icons.filled.Lightbulb
import androidx.compose.material.icons.filled.Tune
import androidx.compose.material.icons.filled.WbSunny
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
import pl.aquarium.lednative.ui.components.ChannelBars
import pl.aquarium.lednative.ui.components.GhostButton
import pl.aquarium.lednative.ui.components.InfoBanner
import pl.aquarium.lednative.ui.components.Pill
import pl.aquarium.lednative.ui.components.PrimaryButton
import pl.aquarium.lednative.ui.components.PwmSlider
import pl.aquarium.lednative.ui.components.ScreenTitle
import pl.aquarium.lednative.ui.components.SectionCard
import pl.aquarium.lednative.ui.components.SettingRow
import androidx.compose.material3.Switch
import androidx.compose.material3.SwitchDefaults
import pl.aquarium.lednative.ui.Border
import pl.aquarium.lednative.ui.Surface3
import pl.aquarium.lednative.ui.TextDim

@Composable
fun LightScreen(vm: AppViewModel) {
    val s = vm.status
    var pwm by remember { mutableStateOf(List(Logic.PWM_CHANNELS) { 0 }) }
    var loaded by remember { mutableStateOf(false) }

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
            CardHeader("Kanały LED", Icons.Default.Brightness6, Warm) {
                Pill("${(pwm.sum() / pwm.size * 100.0 / Logic.PWM_MAX).toInt()}% śr.")
            }
            (0 until Logic.PWM_CHANNELS).forEach { i ->
                PwmSlider(
                    index = i,
                    value = pwm.getOrElse(i) { 0 },
                    enabled = !vm.busy,
                    onChange = { v -> pwm = pwm.toMutableList().also { it[i] = v } }
                )
            }
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                GhostButton("Zeruj", Modifier.weight(1f)) { pwm = List(Logic.PWM_CHANNELS) { 0 } }
                GhostButton("50%", Modifier.weight(1f)) { pwm = List(Logic.PWM_CHANNELS) { 512 } }
                GhostButton("100%", Modifier.weight(1f)) { pwm = List(Logic.PWM_CHANNELS) { Logic.PWM_MAX } }
                GhostButton("Z ESP", Modifier.weight(1f)) {
                    s?.let { pwm = it.pwm }
                    vm.refresh()
                }
            }
            PrimaryButton(
                if (vm.busy) "Wysyłanie…" else "Wyślij PWM do akwarium",
                enabled = !vm.busy && vm.tokenReady
            ) { vm.applyPwm(pwm) }
        }

        SectionCard {
            CardHeader("Podgląd z ESP", Icons.Default.Lightbulb)
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                GhostButton("LED 100%", Modifier.weight(1f), color = Warm) { vm.ledFull() }
                GhostButton("LED OFF", Modifier.weight(1f)) { vm.ledOff() }
                GhostButton("Odśwież", Modifier.weight(1f)) { vm.refresh() }
            }
            s?.let {
                if (it.powerNowCh.isNotEmpty()) {
                    CardHeader("Moc na kanałach", Icons.Default.WbSunny, Warm)
                    ChannelBars(it.powerNowCh)
                }
            }
        }
    }
}
