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
                        color = Color(0xFF1F7A55).copy(alpha = 0.85f),
                        style = Stroke(width = 6f, cap = androidx.compose.ui.graphics.StrokeCap.Round)
                    )
                }

            // Ryby.
            drawFish(Offset(tankLeft + tankW * 0.42f, waterTop + tankH * 0.34f), 16f, Color(0xFFFF9F43), phase)
            drawFish(Offset(tankLeft + tankW * 0.62f, waterTop + tankH * 0.52f), 12f, Color(0xFF7AD4FF), phase + 1.7f)

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
