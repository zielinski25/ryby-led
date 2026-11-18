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
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontWeight
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

@Composable
fun Pill(text: String, color: Color = Cyan) {
    Box(
        Modifier.clip(RoundedCornerShape(999.dp))
            .background(color.copy(alpha = 0.16f))
            .border(1.dp, color.copy(alpha = 0.35f), RoundedCornerShape(999.dp))
            .padding(horizontal = 9.dp, vertical = 3.dp)
    ) {
        Text(
            text, color = color, fontSize = 9.sp,
            fontWeight = FontWeight.Bold, letterSpacing = 0.6.sp
        )
    }
}

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

/** Kafelek statystyki: etykieta, duza wartosc, opis. */
@Composable
fun StatTile(
    label: String,
    value: String,
    hint: String,
    icon: ImageVector,
    accent: Color,
    modifier: Modifier = Modifier
) {
    Surface(
        modifier = modifier,
        color = Surface2,
        shape = RoundedCornerShape(14.dp),
        border = androidx.compose.foundation.BorderStroke(1.dp, Border)
    ) {
        Column(Modifier.padding(12.dp), verticalArrangement = Arrangement.spacedBy(4.dp)) {
            Row(
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(6.dp)
            ) {
                Icon(icon, null, tint = accent, modifier = Modifier.size(14.dp))
                Text(
                    label, fontSize = 9.sp, color = TextDim,
                    fontWeight = FontWeight.Bold, letterSpacing = 0.7.sp
                )
            }
            Text(
                value, fontSize = 19.sp, fontWeight = FontWeight.Bold,
                color = accent, maxLines = 1
            )
            Text(hint, fontSize = 10.sp, color = TextMid, maxLines = 2)
        }
    }
}

/** Duzy kafelek sterujacy (wl/wyl) z podswietleniem stanu. */
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
    onClick: () -> Unit
) {
    val tint = if (active) accent else TextDim
    Surface(
        modifier = modifier
            .clip(RoundedCornerShape(16.dp))
            .clickable(enabled = enabled, onClick = onClick),
        color = if (active) accent.copy(alpha = 0.12f) else Surface2,
        shape = RoundedCornerShape(16.dp),
        border = androidx.compose.foundation.BorderStroke(
            1.dp, if (active) accent.copy(alpha = 0.45f) else Border
        )
    ) {
        Row(
            Modifier.padding(14.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            Box(
                Modifier.size(40.dp).clip(RoundedCornerShape(12.dp))
                    .background(tint.copy(alpha = 0.16f)),
                contentAlignment = Alignment.Center
            ) {
                Icon(icon, null, tint = tint, modifier = Modifier.size(21.dp))
            }
            Column(Modifier.weight(1f)) {
                Text(
                    label, fontSize = 9.sp, color = TextDim,
                    fontWeight = FontWeight.Bold, letterSpacing = 0.8.sp
                )
                Text(value, fontSize = 15.sp, fontWeight = FontWeight.Bold, color = TextHi)
                if (hint.isNotBlank()) Text(hint, fontSize = 10.sp, color = TextMid)
            }
            Box(
                Modifier.size(10.dp).clip(RoundedCornerShape(999.dp))
                    .background(if (active) accent else Border)
            )
        }
    }
}

@Composable
fun QuickTile(
    label: String,
    icon: ImageVector,
    modifier: Modifier = Modifier,
    accent: Color = Cyan,
    onClick: () -> Unit
) {
    Surface(
        modifier = modifier.clip(RoundedCornerShape(14.dp)).clickable(onClick = onClick),
        color = Surface2,
        shape = RoundedCornerShape(14.dp),
        border = androidx.compose.foundation.BorderStroke(1.dp, Border)
    ) {
        Column(
            Modifier.padding(vertical = 14.dp, horizontal = 10.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(6.dp)
        ) {
            Icon(icon, null, tint = accent, modifier = Modifier.size(20.dp))
            Text(
                label, fontSize = 10.sp, color = TextMid,
                fontWeight = FontWeight.Medium, textAlign = androidx.compose.ui.text.style.TextAlign.Center
            )
        }
    }
}

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

@Composable
fun SegmentedTabs(
    options: List<Pair<String, String>>,
    selected: String,
    modifier: Modifier = Modifier,
    onSelect: (String) -> Unit
) {
    Row(
        modifier.fillMaxWidth().clip(RoundedCornerShape(12.dp))
            .background(Surface2).padding(3.dp),
        horizontalArrangement = Arrangement.spacedBy(3.dp)
    ) {
        options.forEach { (key, label) ->
            val active = key == selected
            Box(
                Modifier.weight(1f).clip(RoundedCornerShape(10.dp))
                    .background(if (active) Cyan.copy(alpha = 0.18f) else Color.Transparent)
                    .clickable { onSelect(key) }
                    .padding(vertical = 8.dp),
                contentAlignment = Alignment.Center
            ) {
                Text(
                    label, fontSize = 11.sp,
                    fontWeight = if (active) FontWeight.Bold else FontWeight.Medium,
                    color = if (active) Cyan else TextMid, maxLines = 1
                )
            }
        }
    }
}

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

@Composable
fun KeyValue(key: String, value: String, valueColor: Color = TextHi) {
    Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
        Text(key, fontSize = 11.sp, color = TextDim, modifier = Modifier.weight(1f))
        Text(value, fontSize = 12.sp, color = valueColor, fontWeight = FontWeight.SemiBold)
    }
}
