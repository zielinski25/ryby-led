package pl.aquarium.lednative.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import pl.aquarium.lednative.core.Logic
import pl.aquarium.lednative.ui.Border
import pl.aquarium.lednative.ui.ChannelColors
import pl.aquarium.lednative.ui.ChannelNames
import pl.aquarium.lednative.ui.TextDim
import pl.aquarium.lednative.ui.TextMid
import kotlin.math.max

/** Wykres liniowy z wypelnieniem — rysowany Canvas, bez bibliotek zewnetrznych. */
@Composable
fun LineChart(
    values: List<Double>,
    color: Color,
    modifier: Modifier = Modifier,
    heightDp: Int = 140,
    unit: String = ""
) {
    if (values.size < 2) {
        Box(modifier.fillMaxWidth().height(heightDp.dp), contentAlignment = Alignment.Center) {
            Text("Za mało danych — poczekaj na kolejne odczyty", fontSize = 11.sp, color = TextDim)
        }
        return
    }
    val maxValue = (values.maxOrNull() ?: 0.0).let { if (it <= 0.0) 1.0 else it * 1.15 }
    val minValue = (values.minOrNull() ?: 0.0).let { if (it >= 0.0) 0.0 else it * 1.15 }
    Column(modifier.fillMaxWidth()) {
        Canvas(Modifier.fillMaxWidth().height(heightDp.dp)) {
            val w = size.width
            val h = size.height
            val left = 4f
            val right = w - 4f
            val top = 8f
            val bottom = h - 18f
            // Siatka.
            for (i in 0..3) {
                val y = top + (bottom - top) * i / 3f
                drawLine(
                    color = Border.copy(alpha = 0.6f),
                    start = Offset(left, y), end = Offset(right, y), strokeWidth = 1f
                )
            }
            fun px(i: Int) = left + (right - left) * i / (values.size - 1).coerceAtLeast(1)
            fun py(v: Double): Float {
                val span = (maxValue - minValue).takeIf { it > 0 } ?: 1.0
                return (bottom - (bottom - top) * ((v - minValue) / span)).toFloat()
            }
            val line = Path()
            values.forEachIndexed { i, v ->
                val x = px(i); val y = py(v)
                if (i == 0) line.moveTo(x, y) else line.lineTo(x, y)
            }
            val fill = Path().apply {
                addPath(line)
                lineTo(px(values.lastIndex), bottom)
                lineTo(px(0), bottom)
                close()
            }
            drawPath(
                fill,
                brush = Brush.verticalGradient(
                    listOf(color.copy(alpha = 0.28f), Color.Transparent),
                    startY = top, endY = bottom
                )
            )
            drawPath(line, color = color, style = Stroke(width = 2.5f, cap = StrokeCap.Round))
            // Ostatni punkt.
            drawCircle(
                color = color, radius = 3.5f,
                center = Offset(px(values.lastIndex), py(values.last()))
            )
        }
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
            Text("min ${formatNum(values.minOrNull())}$unit", fontSize = 9.sp, color = TextDim)
            Text("teraz ${formatNum(values.lastOrNull())}$unit", fontSize = 9.sp, color = color)
            Text("max ${formatNum(values.maxOrNull())}$unit", fontSize = 9.sp, color = TextDim)
        }
    }
}

private fun formatNum(v: Double?): String =
    if (v == null || !v.isFinite()) "—" else String.format(java.util.Locale.US, "%.1f", v)

/** Wykres slupkowy (historia energii / godzinowa). */
@Composable
fun BarChart(
    values: List<Double>,
    color: Color,
    labels: List<String>? = null,
    modifier: Modifier = Modifier,
    heightDp: Int = 140
) {
    if (values.isEmpty()) {
        Box(modifier.fillMaxWidth().height(heightDp.dp), contentAlignment = Alignment.Center) {
            Text("Brak danych historycznych w firmware", fontSize = 11.sp, color = TextDim)
        }
        return
    }
    val maxValue = (values.maxOrNull() ?: 0.0).let { if (it <= 0.0) 1.0 else it }
    Column(modifier.fillMaxWidth()) {
        Canvas(Modifier.fillMaxWidth().height(heightDp.dp)) {
            val w = size.width
            val h = size.height
            val bottom = h - 16f
            val top = 6f
            val gap = 4f
            val barW = ((w - gap * (values.size - 1)) / values.size).coerceAtLeast(2f)
            values.forEachIndexed { i, v ->
                val x = i * (barW + gap)
                val bh = ((bottom - top) * (v / maxValue)).toFloat().coerceAtLeast(1f)
                drawRoundRect(
                    brush = Brush.verticalGradient(
                        listOf(color, color.copy(alpha = 0.35f)),
                        startY = bottom - bh, endY = bottom
                    ),
                    topLeft = Offset(x, bottom - bh),
                    size = Size(barW, bh),
                    cornerRadius = androidx.compose.ui.geometry.CornerRadius(3f, 3f)
                )
            }
            drawLine(
                color = Border, start = Offset(0f, bottom),
                end = Offset(w, bottom), strokeWidth = 1f
            )
        }
        if (labels != null && labels.isNotEmpty()) {
            Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
                Text(labels.first(), fontSize = 9.sp, color = TextDim)
                if (labels.size > 1) Text(labels.last(), fontSize = 9.sp, color = TextDim)
            }
        }
    }
}

/** Rozklad mocy na 5 kanalow w formie poziomych paskow. */
@Composable
fun ChannelBars(values: List<Double>, modifier: Modifier = Modifier) {
    val maxValue = max(values.maxOrNull() ?: 0.0, 0.001)
    Column(modifier.fillMaxWidth(), verticalArrangement = Arrangement.spacedBy(8.dp)) {
        ChannelNames.forEachIndexed { i, name ->
            val v = values.getOrNull(i) ?: 0.0
            val c = ChannelColors[i]
            Column(verticalArrangement = Arrangement.spacedBy(3.dp)) {
                Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
                    Text(name, fontSize = 10.sp, color = TextMid)
                    Text("${Logic.fmtWatts(v)} W", fontSize = 10.sp, color = c, fontWeight = FontWeight.SemiBold)
                }
                Canvas(Modifier.fillMaxWidth().height(6.dp).clip(RoundedCornerShape(3.dp))) {
                    drawRect(color = Border.copy(alpha = 0.5f))
                    drawRect(
                        brush = Brush.horizontalGradient(listOf(c.copy(alpha = 0.6f), c)),
                        size = Size(size.width * (v / maxValue).toFloat().coerceIn(0f, 1f), size.height)
                    )
                }
            }
        }
    }
}

/** Legenda kanalow LED. */
@Composable
fun ChannelLegend(modifier: Modifier = Modifier) {
    Row(
        modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.spacedBy(10.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        ChannelNames.forEachIndexed { i, name ->
            Row(
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(4.dp)
            ) {
                Box(Modifier.size(7.dp).clip(RoundedCornerShape(999.dp)).padding(0.dp)) {
                    Canvas(Modifier.size(7.dp)) { drawCircle(color = ChannelColors[i]) }
                }
                Text(name.split(' ').first(), fontSize = 8.sp, color = TextDim)
            }
        }
    }
}
