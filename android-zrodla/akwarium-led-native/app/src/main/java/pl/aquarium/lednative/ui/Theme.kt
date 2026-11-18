package pl.aquarium.lednative.ui

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Typography
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.sp

// Paleta przeniesiona 1:1 z projektu kafelkowego.
val Bg = Color(0xFF060D18)
val Surface1 = Color(0xFF0C1829)
val Surface2 = Color(0xFF111F35)
val Surface3 = Color(0xFF16283F)
val Border = Color(0xFF1E3350)
val Cyan = Color(0xFF00D4F5)
val Blue = Color(0xFF0070FF)
val Warm = Color(0xFFFF9F43)
val Red = Color(0xFFFF4D6D)
val Green = Color(0xFF4ADE80)
val FsBlue = Color(0xFF7AD4FF)
val TextHi = Color(0xFFE6F7FC)
val TextMid = Color(0xFF9FB6CC)
val TextDim = Color(0xFF6B8299)

val ChannelColors = listOf(
    Color(0xFFE8F4FF), // Biale
    Color(0xFF7AD4FF), // Full Spectrum
    Color(0xFFB0E8FF), // FS Biale
    Color(0xFF4060FF), // Niebieskie
    Color(0xFFFF4D6D)  // Czerwone
)

val ChannelNames = listOf("Białe", "Full Spectrum", "FS Białe", "Niebieskie", "Czerwone")

private val Scheme = darkColorScheme(
    primary = Cyan,
    onPrimary = Color(0xFF04121C),
    primaryContainer = Surface3,
    onPrimaryContainer = TextHi,
    secondary = Blue,
    onSecondary = Color.White,
    tertiary = Warm,
    background = Bg,
    onBackground = TextHi,
    surface = Surface1,
    onSurface = TextHi,
    surfaceVariant = Surface2,
    onSurfaceVariant = TextMid,
    outline = Border,
    error = Red,
    onError = Color.White
)

private val AppTypography = Typography(
    headlineSmall = TextStyle(
        fontFamily = FontFamily.SansSerif, fontWeight = FontWeight.Bold,
        fontSize = 22.sp, color = TextHi
    ),
    titleLarge = TextStyle(
        fontFamily = FontFamily.SansSerif, fontWeight = FontWeight.Bold,
        fontSize = 18.sp, color = TextHi
    ),
    titleMedium = TextStyle(
        fontFamily = FontFamily.SansSerif, fontWeight = FontWeight.SemiBold,
        fontSize = 15.sp, color = TextHi
    ),
    titleSmall = TextStyle(
        fontFamily = FontFamily.SansSerif, fontWeight = FontWeight.SemiBold,
        fontSize = 13.sp, color = TextHi
    ),
    bodyLarge = TextStyle(
        fontFamily = FontFamily.SansSerif, fontSize = 15.sp, color = TextHi
    ),
    bodyMedium = TextStyle(
        fontFamily = FontFamily.SansSerif, fontSize = 13.sp, color = TextMid
    ),
    bodySmall = TextStyle(
        fontFamily = FontFamily.SansSerif, fontSize = 11.sp, color = TextDim
    ),
    labelLarge = TextStyle(
        fontFamily = FontFamily.SansSerif, fontWeight = FontWeight.SemiBold,
        fontSize = 13.sp, color = TextHi
    ),
    labelMedium = TextStyle(
        fontFamily = FontFamily.SansSerif, fontWeight = FontWeight.Medium,
        fontSize = 11.sp, color = TextMid
    ),
    labelSmall = TextStyle(
        fontFamily = FontFamily.SansSerif, fontWeight = FontWeight.Medium,
        fontSize = 10.sp, color = TextDim
    )
)

@Composable
fun AkwariumTheme(content: @Composable () -> Unit) {
    // Aplikacja ma jeden, ciemny motyw — zgodny z projektem wizualnym.
    MaterialTheme(
        colorScheme = Scheme,
        typography = AppTypography,
        content = content
    )
}
