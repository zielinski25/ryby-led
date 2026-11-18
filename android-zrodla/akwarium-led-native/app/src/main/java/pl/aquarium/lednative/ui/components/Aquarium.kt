package pl.aquarium.lednative.ui.components

import androidx.compose.animation.core.LinearEasing
import androidx.compose.animation.core.RepeatMode
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.infiniteRepeatable
import androidx.compose.animation.core.rememberInfiniteTransition
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import pl.aquarium.lednative.core.DeviceStatus
import pl.aquarium.lednative.core.Logic
import pl.aquarium.lednative.ui.Border
import pl.aquarium.lednative.ui.ChannelColors
import pl.aquarium.lednative.ui.Cyan
import pl.aquarium.lednative.ui.TextDim
import kotlin.math.PI
import kotlin.math.sin

/**
 * Wizualizacja akwarium rysowana w calosci przez Compose Canvas.
 * Belka LED swieci kolorem kanalowym, woda faluje, babelki plyna gdy pompka dziala.
 */
@Composable
fun AquariumView(status: DeviceStatus?, modifier: Modifier = Modifier) {
    val transition = rememberInfiniteTransition(label = "aquarium")
    val phase by transition.animateFloat(
        initialValue = 0f, targetValue = (2 * PI).toFloat(),
        animationSpec = infiniteRepeatable(
            animation = tween(5200, easing = LinearEasing),
            repeatMode = RepeatMode.Restart
        ),
        label = "phase"
    )
    val bubblePhase by transition.animateFloat(
        initialValue = 0f, targetValue = 1f,
        animationSpec = infiniteRepeatable(
            animation = tween(3600, easing = LinearEasing),
            repeatMode = RepeatMode.Restart
        ),
        label = "bubbles"
    )

    val power = status?.power ?: false
    val pump = status?.pumpOn ?: false
    val pwm = status?.pwm ?: List(Logic.PWM_CHANNELS) { 0 }
    val intensity = if (power) (pwm.maxOrNull() ?: 0) / Logic.PWM_MAX.toFloat() else 0f

    // Kolor belki = srednia wazona kanalow.
    val mix = blendChannels(pwm, power)

    Column(modifier.fillMaxWidth()) {
        Canvas(
            Modifier.fillMaxWidth().height(190.dp)
        ) {
            val w = size.width
            val h = size.height
            val pad = 10f
            val tankLeft = pad
            val tankTop = h * 0.22f
            val tankW = w - pad * 2
            val tankH = h * 0.70f
            val waterTop = tankTop + tankH * 0.16f

            // Belka LED nad zbiornikiem.
            val barH = h * 0.055f
            val barY = tankTop - barH - h * 0.07f
            drawRoundRect(
                color = Color(0xFF1B2C44),
                topLeft = Offset(tankLeft + tankW * 0.10f, barY),
                size = Size(tankW * 0.80f, barH),
                cornerRadius = CornerRadius(barH / 2, barH / 2)
            )
            // Diody.
            val leds = 14
            for (i in 0 until leds) {
                val cx = tankLeft + tankW * 0.13f + (tankW * 0.74f) * (i + 0.5f) / leds
                val cy = barY + barH / 2
                val diodeColor = if (power) mix else Color(0xFF33465E)
                drawCircle(
                    color = diodeColor.copy(alpha = if (power) 0.55f + 0.45f * intensity else 0.5f),
                    radius = barH * 0.30f, center = Offset(cx, cy)
                )
            }

            // Stozek swiatla.
            if (power && intensity > 0.01f) {
                val cone = Path().apply {
                    moveTo(tankLeft + tankW * 0.16f, barY + barH)
                    lineTo(tankLeft + tankW * 0.84f, barY + barH)
                    lineTo(tankLeft + tankW * 0.96f, tankTop + tankH)
                    lineTo(tankLeft + tankW * 0.04f, tankTop + tankH)
                    close()
                }
                drawPath(
                    cone,
                    brush = Brush.verticalGradient(
                        colors = listOf(
                            mix.copy(alpha = 0.30f * intensity),
                            mix.copy(alpha = 0.06f * intensity),
                            Color.Transparent
                        ),
                        startY = barY, endY = tankTop + tankH
                    )
                )
            }

            // Szklo zbiornika.
            drawRoundRect(
                color = Color(0xFF0A1626),
                topLeft = Offset(tankLeft, tankTop),
                size = Size(tankW, tankH),
                cornerRadius = CornerRadius(14f, 14f)
            )

            // Woda z falujaca powierzchnia.
            val water = Path().apply {
                moveTo(tankLeft + 3f, waterTop)
                val steps = 40
                for (i in 0..steps) {
                    val x = tankLeft + 3f + (tankW - 6f) * i / steps
                    val y = waterTop + sin(phase + i * 0.42f) * 3.2f
                    lineTo(x, y)
                }
                lineTo(tankLeft + tankW - 3f, tankTop + tankH - 3f)
                lineTo(tankLeft + 3f, tankTop + tankH - 3f)
                close()
            }
            drawPath(
                water,
                brush = Brush.verticalGradient(
                    colors = listOf(
                        Color(0xFF0E3A5C).copy(alpha = 0.95f),
                        Color(0xFF06253C)
                    ),
                    startY = waterTop, endY = tankTop + tankH
                )
            )
            // Rozswietlenie wody od LED.
            if (power && intensity > 0.01f) {
                drawPath(
                    water,
                    brush = Brush.verticalGradient(
                        colors = listOf(
                            mix.copy(alpha = 0.26f * intensity),
                            Color.Transparent
                        ),
                        startY = waterTop, endY = tankTop + tankH
                    )
                )
            }

            // Dwie listwy LED nad wodą (w makiecie zawierają osobne odczyty temperatury).
            val panelW = tankW * 0.29f
            val panelH = tankH * 0.15f
            val panelY = tankTop + tankH * 0.035f
            listOf(0.20f, 0.52f).forEachIndexed { index, fx ->
                val px = tankLeft + tankW * fx
                drawRoundRect(
                    color = Color(0xFF21354A),
                    topLeft = Offset(px, panelY), size = Size(panelW, panelH),
                    cornerRadius = CornerRadius(7f, 7f)
                )
                drawRoundRect(
                    color = Color(0xFF91AEBF),
                    topLeft = Offset(px, panelY), size = Size(panelW, panelH),
                    cornerRadius = CornerRadius(7f, 7f), style = Stroke(width = 1.4f)
                )
                drawRoundRect(
                    color = (if (index == 0) Color(0xFFD6F7FF) else Color(0xFF6ED8F4)).copy(alpha = 0.85f),
                    topLeft = Offset(px + panelW * 0.12f, panelY + panelH * 0.23f),
                    size = Size(panelW * 0.76f, panelH * 0.18f),
                    cornerRadius = CornerRadius(5f, 5f)
                )
            }

            // Termometr wody po lewej i obudowa filtra po prawej.
            val thermometerX = tankLeft + tankW * 0.075f
            val thermometerY = waterTop + tankH * 0.28f
            drawRoundRect(
                color = Color(0xFF9FEFFF),
                topLeft = Offset(thermometerX, thermometerY),
                size = Size(8f, tankH * 0.42f), cornerRadius = CornerRadius(5f, 5f)
            )
            drawRoundRect(
                color = Color(0xFFFF755E),
                topLeft = Offset(thermometerX + 2f, thermometerY + tankH * 0.20f),
                size = Size(4f, tankH * 0.18f), cornerRadius = CornerRadius(3f, 3f)
            )
            val filterX = tankLeft + tankW * 0.88f
            val filterY = waterTop + tankH * 0.17f
            drawRoundRect(
                color = Color(0xFF344B61),
                topLeft = Offset(filterX, filterY),
                size = Size(tankW * 0.07f, tankH * 0.40f), cornerRadius = CornerRadius(9f, 9f)
            )
            drawRoundRect(
                color = if (pump) Color(0xFF4ADE80) else Color(0xFF6B8299),
                topLeft = Offset(filterX + tankW * 0.023f, filterY + tankH * 0.11f),
                size = Size(4f, 4f), cornerRadius = CornerRadius(2f, 2f)
            )

            // Podloze.
            drawRoundRect(
                color = Color(0xFF123049),
                topLeft = Offset(tankLeft + 4f, tankTop + tankH - h * 0.075f),
                size = Size(tankW - 8f, h * 0.07f),
                cornerRadius = CornerRadius(8f, 8f)
            )
            // Rosliny.
            val plantBase = tankTop + tankH - h * 0.07f
            listOf(0.18f to 0.30f, 0.30f to 0.22f, 0.68f to 0.26f, 0.80f to 0.34f)
                .forEachIndexed { idx, (fx, fh) ->
                    val px = tankLeft + tankW * fx
                    val ph = tankH * fh
                    val sway = sin(phase * 0.8f + idx) * 5f
                    val plant = Path().apply {
                        moveTo(px, plantBase)
                        cubicTo(px + sway, plantBase - ph * 0.35f, px + sway, plantBase - ph * 0.75f, px + sway * 1.6f, plantBase - ph)
                    }
                    drawPath(
                        plant,
                        color = Color(0xFF1F7A55).copy(alpha = 0.9f),
                        style = Stroke(width = 4f, cap = androidx.compose.ui.graphics.StrokeCap.Round)
                    )
                    // Liście po obu stronach łodygi — w HTML są pełnymi, widocznymi liśćmi,
                    // nie samymi kreskami.
                    val leafColor = if (idx % 2 == 0) Color(0xFF24BC83) else Color(0xFF168F91)
                    val leafStroke = if (idx % 2 == 0) Color(0xFF75EFBD) else Color(0xFF62DAD3)
                    for (leafIndex in 0..2) {
                        val leafY = plantBase - ph * (0.28f + leafIndex * 0.22f)
                        val side = if (leafIndex % 2 == 0) -1f else 1f
                        drawPlantLeaf(
                            x = px + sway * (1f - leafIndex * 0.25f),
                            y = leafY,
                            direction = side,
                            leafSize = tankW * 0.045f,
                            fill = leafColor,
                            outline = leafStroke
                        )
                    }
                }

            // Sześć neonów w ławicy oraz dwie większe ryby jak w makiecie HTML.
            for (i in 0 until 6) {
                val x = tankLeft + tankW * (0.24f + i * 0.085f)
                val y = waterTop + tankH * (0.33f + if (i % 2 == 0) 0.04f else -0.015f)
                drawFish(Offset(x, y), 5.5f, Color(0xFF27DDF4), phase + i * 0.45f)
            }
            drawFish(Offset(tankLeft + tankW * 0.69f, waterTop + tankH * 0.59f), 13f, Color(0xFFE7A63F), phase + 0.8f)
            drawFish(Offset(tankLeft + tankW * 0.52f, waterTop + tankH * 0.78f), 10f, Color(0xFF9D8066), phase + 1.7f)

            // Babelki pompki.
            if (pump) {
                for (i in 0 until 7) {
                    val t = (bubblePhase + i / 7f) % 1f
                    val bx = tankLeft + tankW * 0.88f + sin(t * 8f + i) * 5f
                    val by = tankTop + tankH - h * 0.08f - t * (tankH * 0.72f)
                    drawCircle(
                        color = Cyan.copy(alpha = 0.45f * (1f - t)),
                        radius = 2.4f + i % 3,
                        center = Offset(bx, by)
                    )
                }
            }

            // Obrys szkla.
            drawRoundRect(
                color = Border,
                topLeft = Offset(tankLeft, tankTop),
                size = Size(tankW, tankH),
                cornerRadius = CornerRadius(14f, 14f),
                style = Stroke(width = 2f)
            )
            // Refleks na szkle.
            drawLine(
                color = Color.White.copy(alpha = 0.06f),
                start = Offset(tankLeft + tankW * 0.06f, tankTop + 6f),
                end = Offset(tankLeft + tankW * 0.06f, tankTop + tankH - 10f),
                strokeWidth = 6f
            )
        }

        Box(Modifier.fillMaxWidth().padding(top = 6.dp), contentAlignment = Alignment.Center) {
            Text(
                when {
                    status == null -> "Brak danych z akwarium"
                    !status.power -> "Światło wyłączone"
                    else -> "Świeci ${status.pwmPercent}% · ${if (status.pumpOn) "pompka pracuje" else "pompka wyłączona"}"
                },
                fontSize = 11.sp, color = TextDim, fontWeight = FontWeight.Medium
            )
        }
    }
}

private fun DrawScope.drawPlantLeaf(
    x: Float,
    y: Float,
    direction: Float,
    leafSize: Float,
    fill: Color,
    outline: Color
) {
    val endX = x + direction * leafSize * 1.7f
    val endY = y - leafSize * 1.25f
    val leaf = Path().apply {
        moveTo(x, y)
        cubicTo(x + direction * leafSize * 0.15f, y - leafSize * 0.95f,
            endX - direction * leafSize * 0.25f, endY - leafSize * 0.30f, endX, endY)
        cubicTo(endX - direction * leafSize * 0.15f, endY + leafSize * 0.95f,
            x + direction * leafSize * 0.35f, y + leafSize * 0.32f, x, y)
        close()
    }
    drawPath(leaf, color = fill.copy(alpha = 0.95f))
    drawPath(leaf, color = outline.copy(alpha = 0.9f), style = Stroke(width = 1.2f))
}

private fun DrawScope.drawFish(center: Offset, size: Float, color: Color, phase: Float) {
    val bob = sin(phase) * 3f
    val c = Offset(center.x, center.y + bob)
    drawOval(
        color = color.copy(alpha = 0.9f),
        topLeft = Offset(c.x - size, c.y - size * 0.5f),
        size = Size(size * 2, size)
    )
    val tail = Path().apply {
        moveTo(c.x + size * 0.85f, c.y)
        lineTo(c.x + size * 1.7f, c.y - size * 0.55f)
        lineTo(c.x + size * 1.7f, c.y + size * 0.55f)
        close()
    }
    drawPath(tail, color = color.copy(alpha = 0.7f))
    drawCircle(
        color = Color(0xFF06121F), radius = size * 0.14f,
        center = Offset(c.x - size * 0.55f, c.y - size * 0.08f)
    )
}

private fun blendChannels(pwm: List<Int>, power: Boolean): Color {
    if (!power) return Color(0xFF33465E)
    var r = 0f; var g = 0f; var b = 0f; var sum = 0f
    for (i in ChannelColors.indices) {
        val v = (pwm.getOrNull(i) ?: 0).coerceIn(0, Logic.PWM_MAX).toFloat()
        if (v <= 0f) continue
        val c = ChannelColors[i]
        r += c.red * v; g += c.green * v; b += c.blue * v; sum += v
    }
    if (sum <= 0f) return Color(0xFF33465E)
    return Color(r / sum, g / sum, b / sum, 1f)
}
