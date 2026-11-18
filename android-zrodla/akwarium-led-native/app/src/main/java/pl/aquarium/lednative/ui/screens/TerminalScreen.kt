package pl.aquarium.lednative.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Code
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.OutlinedTextFieldDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import pl.aquarium.lednative.ui.AppViewModel
import pl.aquarium.lednative.ui.Border
import pl.aquarium.lednative.ui.Cyan
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
import pl.aquarium.lednative.ui.components.Pill
import pl.aquarium.lednative.ui.components.PrimaryButton
import pl.aquarium.lednative.ui.components.ScreenTitle
import pl.aquarium.lednative.ui.components.SectionCard

private val QUICK = listOf(
    "status", "adaptive", "minlux", "logsize",
    "serial", "wifi", "both", "off", "save"
)

@Composable
fun TerminalScreen(vm: AppViewModel) {
    var input by remember { mutableStateOf("") }
    val listState = rememberLazyListState()

    LaunchedEffect(vm.terminalLines.size) {
        if (vm.terminalLines.isNotEmpty()) {
            listState.animateScrollToItem(vm.terminalLines.size - 1)
        }
    }

    Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
        ScreenTitle("Terminal", "Natywny WebSocket do ESP32 (/ws)") {
            Pill(
                if (vm.terminalConnected) "POŁĄCZONY" else "OFFLINE",
                if (vm.terminalConnected) Green else Warm
            )
        }

        if (vm.lanBase == null) {
            InfoBanner(
                "Terminal działa tylko w sieci lokalnej. Najpierw wykryj ESP w zakładce Więcej.",
                warn = true
            )
        }

        SectionCard {
            CardHeader("Połączenie", Icons.Default.Code) {
                Text(vm.terminalState, fontSize = 10.sp, color = TextMid)
            }
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                if (vm.terminalConnected) {
                    GhostButton("Rozłącz", Modifier.weight(1f), color = Red) { vm.disconnectTerminal() }
                } else {
                    PrimaryButton("Połącz z ESP", Modifier.weight(1f)) { vm.connectTerminal() }
                }
                GhostButton("Wyczyść", Modifier.weight(1f)) { vm.clearTerminal() }
            }
            vm.lanBase?.let {
                Text(it.replace("http://", "ws://") + "/ws", fontSize = 10.sp, color = TextDim)
            }
        }

        SectionCard {
            CardHeader("Konsola (${vm.terminalLines.size})", Icons.Default.Code, Cyan)
            if (vm.terminalLines.isEmpty()) {
                EmptyState(
                    "Konsola jest pusta",
                    "Połącz się i wyślij komendę, np. status. Firmware odpowiada tekstem."
                )
            } else {
                LazyColumn(
                    Modifier.fillMaxWidth().heightIn(min = 200.dp, max = 380.dp)
                        .clip(RoundedCornerShape(10.dp)).background(Color(0xFF040A12))
                        .padding(10.dp),
                    state = listState,
                    verticalArrangement = Arrangement.spacedBy(3.dp)
                ) {
                    items(vm.terminalLines) { line ->
                        Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                            Text(line.at, fontSize = 9.sp, color = TextDim, fontFamily = FontFamily.Monospace)
                            Text(
                                (if (line.outgoing) "> " else "") + line.text,
                                fontSize = 10.sp,
                                color = if (line.outgoing) Cyan else Green,
                                fontFamily = FontFamily.Monospace,
                                lineHeight = 14.sp,
                                modifier = Modifier.weight(1f)
                            )
                        }
                    }
                }
            }

            OutlinedTextField(
                value = input,
                onValueChange = { input = it },
                modifier = Modifier.fillMaxWidth(),
                singleLine = true,
                placeholder = { Text("komenda, np. status", fontSize = 12.sp, color = TextDim) },
                shape = RoundedCornerShape(12.dp),
                keyboardOptions = KeyboardOptions(imeAction = ImeAction.Send),
                keyboardActions = KeyboardActions(onSend = {
                    vm.sendTerminal(input)
                    input = ""
                }),
                colors = OutlinedTextFieldDefaults.colors(
                    focusedTextColor = TextHi, unfocusedTextColor = TextHi,
                    focusedBorderColor = Cyan, unfocusedBorderColor = Border,
                    cursorColor = Cyan, focusedContainerColor = Surface2,
                    unfocusedContainerColor = Surface2
                )
            )
            PrimaryButton("Wyślij komendę", enabled = vm.terminalConnected) {
                vm.sendTerminal(input)
                input = ""
            }
        }

        SectionCard {
            CardHeader("Szybkie komendy", Icons.Default.Code, Warm)
            QUICK.chunked(3).forEach { row ->
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    row.forEach { cmd ->
                        GhostButton(
                            cmd, Modifier.weight(1f), enabled = vm.terminalConnected
                        ) { vm.sendTerminal(cmd) }
                    }
                    repeat(3 - row.size) { Box(Modifier.weight(1f)) }
                }
            }
            InfoBanner(
                "Komendy firmware: status, adaptive, minlux [on|off|target <500-8000>], " +
                    "serial/usb, wifi/web, both/all, off, save, logon, logoff, logsize."
            )
        }
    }
}
