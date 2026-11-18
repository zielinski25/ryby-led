package pl.aquarium.lednative.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.AutoMode
import androidx.compose.material.icons.filled.Bolt
import androidx.compose.material.icons.filled.DeviceThermostat
import androidx.compose.material.icons.filled.Lightbulb
import androidx.compose.material.icons.filled.PowerSettingsNew
import androidx.compose.material.icons.filled.RestartAlt
import androidx.compose.material.icons.filled.ShowChart
import androidx.compose.material.icons.filled.Thermostat
import androidx.compose.material.icons.filled.Waves
import androidx.compose.material.icons.filled.WbSunny
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import pl.aquarium.lednative.ui.Border
import pl.aquarium.lednative.ui.Cyan
import pl.aquarium.lednative.ui.Surface1
import pl.aquarium.lednative.ui.Surface2
import pl.aquarium.lednative.ui.TextDim
import pl.aquarium.lednative.ui.TextHi
import pl.aquarium.lednative.ui.TextMid
import pl.aquarium.lednative.ui.Warm

private val ControlValueOn = Color(0xFF57E6BB)   // HTML .primaryControl[data-state="on"] b
private val ControlValueOff = Color(0xFFFFAC84)  // HTML .primaryControl[data-state="off"] b / i

/** Karta sekcji — ciemne tło z obramowaniem jak w HTML .card. */
@Composable
fun SectionCard(
    modifier: Modifier = Modifier,
    content: @Composable () -> Unit
) {
    Surface(
        modifier = modifier.fillMaxWidth(),
        color = Surface1,
        shape = RoundedCornerShape(16.dp),
        border = androidx.compose.foundation.BorderStroke(1.dp, Border)
    ) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(10.dp)) {
            content()
        }
    }
}

/** Nagłówek karty — tytuł, ikona w kwadracie, opcjonalny element trailing. */
@Composable
fun CardHeader(
    title: String,
    icon: ImageVector? = null,
    iconTint: Color = Cyan,
    trailing: (@Composable () -> Unit)? = null
) {
    Row(
        Modifier.fillMaxWidth(),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        if (icon != null) {
            Box(
                Modifier.size(28.dp).clip(RoundedCornerShape(9.dp))
                    .background(iconTint.copy(alpha = 0.14f)),
                contentAlignment = Alignment.Center
            ) {
                Icon(icon, null, tint = iconTint, modifier = Modifier.size(16.dp))
            }
        }
        Text(title, style = MaterialTheme.typography.titleMedium, modifier = Modifier.weight(1f))
        trailing?.invoke()
    }
}

/** Pigułka statusu — jak w HTML .pill. */
@Composable
fun Pill(text: String, color: Color = Cyan) {
    Box(
        Modifier.clip(RoundedCornerShape(999.dp))
            .background(color.copy(alpha = 0.10f))
            .border(1.dp, color.copy(alpha = 0.33f), RoundedCornerShape(999.dp))
            .padding(horizontal = 6.dp, vertical = 4.dp)
    ) {
        Text(
            text, color = color, fontSize = 7.sp,
            fontWeight = FontWeight.Bold, letterSpacing = 0.4.sp,
            fontFamily = FontFamily.Monospace
        )
    }
}

/** Tytuł ekranu — jak w HTML .pageHead. */
@Composable
fun ScreenTitle(title: String, subtitle: String, badge: (@Composable () -> Unit)? = null) {
    Row(
        Modifier.fillMaxWidth().padding(bottom = 2.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(Modifier.weight(1f)) {
            Text(title, style = MaterialTheme.typography.headlineSmall)
            Text(subtitle, style = MaterialTheme.typography.bodySmall)
        }
        badge?.invoke()
    }
}

/** Kafelek statystyki — 1:1 ze wzoru HTML .tile. */
@Composable
fun StatTile(
    label: String,
    value: String,
    hint: String,
    icon: ImageVector,
    accent: Color,
    modifier: Modifier = Modifier
) {
    val gradient = Brush.linearGradient(
        colors = listOf(Color(0xFF0E1B2D), Color(0xFF0B1727)),
        start = Offset(0f, 0f),
        end = Offset(1f, 1f)
    )
    Surface(
        modifier = modifier,
        color = Surface2,
        shape = RoundedCornerShape(13.dp),
        border = androidx.compose.foundation.BorderStroke(1.dp, Border)
    ) {
        Box(
            Modifier
                .background(gradient)
                .clip(RoundedCornerShape(13.dp))
                .padding(11.dp)
        ) {
            Column(modifier = Modifier.fillMaxWidth(), verticalArrangement = Arrangement.spacedBy(7.dp)) {
                Row(
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.spacedBy(6.dp)
                ) {
                    Icon(icon, null, tint = accent, modifier = Modifier.size(12.dp))
                    Text(
                        label, fontSize = 7.sp, color = TextDim,
                        fontWeight = FontWeight.Bold, letterSpacing = 0.7.sp,
                        fontFamily = FontFamily.Monospace
                    )
                }
                Text(
                    value, fontSize = 19.sp, fontWeight = FontWeight.Bold,
                    color = accent, maxLines = 1, fontFamily = FontFamily.Monospace
                )
                Text(hint, fontSize = 8.sp, color = TextMid, maxLines = 2)
            }
        }
    }
}

/** Duzy kafelek sterujacy (wl/wyl) — 1:1 ze wzoru HTML .primaryControl. */
@Composable
fun ControlTile(
    label: String,
    value: String,
    hint: String,
    icon: ImageVector,
    active: Boolean,
    accent: Color,
    enabled: Boolean = true,
    modifier: Modifier = Modifier,
    trailing: (@Composable () -> Unit)? = null,
    onClick: () -> Unit
) {
    val tint = if (active) accent else TextDim
    val gradient = if (active) {
        Brush.linearGradient(
            colors = listOf(
                accent.copy(alpha = 0.16f),
                accent.copy(alpha = 0.06f)
            ),
            start = Offset(0f, 0f),
            end = Offset(1f, 1f)
        )
    } else {
        Brush.linearGradient(
            colors = listOf(Color(0xFF122237), Color(0xFF0B1727)),
            start = Offset(0f, 0f),
            end = Offset(1f, 1f)
        )
    }
    Surface(
        modifier = modifier
            .clip(RoundedCornerShape(11.dp))
            .clickable(enabled = enabled, onClick = onClick),
        color = Surface2,
        shape = RoundedCornerShape(11.dp),
        border = androidx.compose.foundation.BorderStroke(
            1.dp, if (active) accent.copy(alpha = 0.45f) else Border
        )
    ) {
        Box(
            Modifier
                .background(gradient)
                .clip(RoundedCornerShape(11.dp))
                .padding(10.dp, 9.dp)
        ) {
            Row(
                Modifier.fillMaxWidth(),
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Box(
                    Modifier.size(26.dp).clip(RoundedCornerShape(8.dp))
                        .background(tint.copy(alpha = 0.16f)),
                    contentAlignment = Alignment.Center
                ) {
                    Icon(icon, null, tint = tint, modifier = Modifier.size(14.dp))
                }
                Column(Modifier.weight(1f)) {
                    Text(
                        label, fontSize = 9.sp, color = TextDim,
                        fontWeight = FontWeight.Bold, letterSpacing = 0.6.sp,
                        fontFamily = FontFamily.Monospace
                    )
                    Text(value, fontSize = 15.sp, fontWeight = FontWeight.Bold, color = if (active) ControlValueOn else ControlValueOff)
                    if (hint.isNotBlank()) Text(hint, fontSize = 10.sp, color = TextMid)
                }
                Box(
                    Modifier.size(10.dp).clip(RoundedCornerShape(999.dp))
                        .background(if (active) accent else ControlValueOff)
                )
                trailing?.invoke()
            }
        }
    }
}

/** Szybki kafelek akcji — jak w HTML .quickTile. */
@Composable
fun QuickTile(
    label: String,
    icon: ImageVector,
    modifier: Modifier = Modifier,
    accent: Color = Cyan,
    onClick: () -> Unit
) {
    Surface(
        modifier = modifier.clip(RoundedCornerShape(10.dp)).clickable(onClick = onClick),
        color = Surface2,
        shape = RoundedCornerShape(10.dp),
        border = androidx.compose.foundation.BorderStroke(1.dp, Border)
    ) {
        Column(
            Modifier.padding(vertical = 10.dp, horizontal = 9.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(6.dp)
        ) {
            Icon(icon, null, tint = accent, modifier = Modifier.size(13.dp))
            Text(
                label, fontSize = 8.sp, color = TextMid,
                fontWeight = FontWeight.Medium, textAlign = TextAlign.Center,
                fontFamily = FontFamily.Monospace
            )
        }
    }
}

/** Wiersz ustawień — tytuł, opis, kontrolka po prawej. */
@Composable
fun SettingRow(
    title: String,
    subtitle: String,
    modifier: Modifier = Modifier,
    control: @Composable () -> Unit
) {
    Row(
        modifier.fillMaxWidth().padding(vertical = 4.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(10.dp)
    ) {
        Column(Modifier.weight(1f)) {
            Text(title, style = MaterialTheme.typography.titleSmall)
            if (subtitle.isNotBlank()) Text(subtitle, style = MaterialTheme.typography.bodySmall)
        }
        control()
    }
}

/** Zakładki segmentowane — jak w HTML .seg. */
@Composable
fun SegmentedTabs(
    options: List<Pair<String, String>>,
    selected: String,
    modifier: Modifier = Modifier,
    onSelect: (String) -> Unit
) {
    Row(
        modifier.fillMaxWidth().clip(RoundedCornerShape(8.dp))
            .background(Surface2).padding(1.dp),
        horizontalArrangement = Arrangement.spacedBy(1.dp)
    ) {
        options.forEachIndexed { index, (key, label) ->
            val active = key == selected
            Box(
                Modifier.weight(1f).clip(RoundedCornerShape(7.dp))
                    .background(if (active) Cyan.copy(alpha = 0.18f) else Color.Transparent)
                    .clickable { onSelect(key) }
                    .padding(vertical = 6.dp),
                contentAlignment = Alignment.Center
            ) {
                Text(
                    label, fontSize = 7.sp,
                    fontWeight = if (active) FontWeight.Bold else FontWeight.Medium,
                    color = if (active) Cyan else TextMid, maxLines = 1,
                    fontFamily = FontFamily.Monospace
                )
            }
            if (index < options.size - 1) {
                Box(
                    Modifier.width(0.5.dp).background(Border.copy(alpha = 0.3f))
                )
            }
        }
    }
}

/** Baner informacyjny — jak w HTML .info. */
@Composable
fun InfoBanner(text: String, warn: Boolean = false) {
    val c = if (warn) Warm else Cyan
    Row(
        Modifier.fillMaxWidth().clip(RoundedCornerShape(12.dp))
            .background(c.copy(alpha = 0.10f))
            .border(1.dp, c.copy(alpha = 0.3f), RoundedCornerShape(12.dp))
            .padding(12.dp),
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        Text(text, fontSize = 11.sp, color = TextMid, lineHeight = 16.sp)
    }
}

/** Stan pusty — jak w HTML .emptyState. */
@Composable
fun EmptyState(text: String, hint: String) {
    Column(
        Modifier.fillMaxWidth().padding(vertical = 28.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(6.dp)
    ) {
        Text(text, style = MaterialTheme.typography.titleMedium, color = TextMid)
        Text(
            hint, style = MaterialTheme.typography.bodySmall,
            textAlign = androidx.compose.ui.text.style.TextAlign.Center
        )
    }
}

/** Para klucz-wartość — jak w HTML .keyValue. */
@Composable
fun KeyValue(key: String, value: String, valueColor: Color = TextHi) {
    Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
        Text(key, fontSize = 11.sp, color = TextDim, modifier = Modifier.weight(1f))
        Text(value, fontSize = 12.sp, color = valueColor, fontWeight = FontWeight.SemiBold)
    }
}
