package com.rybyled.panel.ui

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/*
 * System wizualny Ryby LED.
 * Paleta, promienie i rytm typografii pochodzą z Pieca (Sterownik Pieca C.O., Color.kt / Type.kt /
 * Dimens w android/app/src/main/java/com/sterownikco/pro/ui/theme). Opis i uzasadnienie: docs/08_DESIGN_APLIKACJI.md.
 * Jedna zmiana: etykiety pomocnicze (TextDim2) są jaśniejsze niż w Piecu, żeby spełniały WCAG AA (4,5:1).
 */
object Pal {
    // Tła i powierzchnie (od najciemniejszego)
    val Bg = Color(0xFF060D18)
    val BgTop = Color(0xFF0B1B30)
    val Top = Color(0xFF071421)
    val Top2 = Color(0xFF0A1928)
    val Nav = Color(0xFF081421)
    val Nav2 = Color(0xFF0B1725)
    val Surface = Color(0xFF0C1829)
    val Surface2 = Color(0xFF111F35)
    val Surface3 = Color(0xFF162942)
    val HeroA = Color(0xFF0F2438)
    val HeroB = Color(0xFF07131F)
    val TileA = Color(0xFF102437)
    val TileB = Color(0xFF07131F)

    // Obramowania
    val Border = Color(0xFF506D84).copy(alpha = 0.22f)
    val BorderStrong = Color(0xFF5E8294).copy(alpha = 0.35f)

    // Tekst
    val Text = Color(0xFFE6F0F7)
    val TextDim = Color(0xFF8EA6BA)
    val TextDim2 = Color(0xFF7D98AD)
    val TileTitle = Color(0xFF92A9BC)

    // Akcenty i stany
    val Cyan = Color(0xFF00D4F5)
    val Accent = Color(0xFFFF9F43)
    val Live = Color(0xFF4ADE80)
    val Warn = Color(0xFFFBBF24)
    val Err = Color(0xFFFF5F78)
    val Stale = Color(0xFF9DB0BE)
    val OnCyan = Color(0xFF060D18)
}

object Dimens {
    val radiusTile = 18.dp
    val radiusCard = 16.dp
    val radiusPill = 999.dp
    val radiusBanner = 13.dp
    val pagePad = 14.dp
    val gap = 12.dp
    val tileH = 104.dp
    val controlH = 52.dp
}

object Txt {
    private val sans = FontFamily.SansSerif

    val brand = TextStyle(fontFamily = sans, fontWeight = FontWeight.ExtraBold, fontSize = 12.sp, letterSpacing = 1.4.sp, color = Pal.Cyan)
    val title = TextStyle(fontFamily = sans, fontWeight = FontWeight.ExtraBold, fontSize = 24.sp, letterSpacing = 0.2.sp, color = Pal.Text)
    val sub = TextStyle(fontFamily = sans, fontWeight = FontWeight.Medium, fontSize = 13.sp, color = Pal.TextDim)
    val chip = TextStyle(fontFamily = sans, fontWeight = FontWeight.ExtraBold, fontSize = 12.sp, letterSpacing = 0.4.sp)
    val heroTemp = TextStyle(fontFamily = sans, fontWeight = FontWeight.ExtraBold, fontSize = 46.sp, letterSpacing = (-0.8).sp, color = Pal.Text)
    val heroLabel = TextStyle(fontFamily = sans, fontWeight = FontWeight.ExtraBold, fontSize = 12.sp, letterSpacing = 0.8.sp, color = Pal.Cyan)
    val section = TextStyle(fontFamily = sans, fontWeight = FontWeight.ExtraBold, fontSize = 12.sp, letterSpacing = 1.0.sp, color = Pal.TextDim)
    val tileTitle = TextStyle(fontFamily = sans, fontWeight = FontWeight.Bold, fontSize = 13.sp, letterSpacing = 0.3.sp, color = Pal.TileTitle)
    val tileValue = TextStyle(fontFamily = sans, fontWeight = FontWeight.ExtraBold, fontSize = 22.sp, letterSpacing = (-0.2).sp, color = Pal.Text)
    val body = TextStyle(fontFamily = sans, fontWeight = FontWeight.Medium, fontSize = 15.sp, color = Pal.Text)
    val bodyBold = TextStyle(fontFamily = sans, fontWeight = FontWeight.Bold, fontSize = 15.sp, color = Pal.Text)
    val note = TextStyle(fontFamily = sans, fontWeight = FontWeight.Medium, fontSize = 13.sp, color = Pal.TextDim, lineHeight = 18.sp)
    val btn = TextStyle(fontFamily = sans, fontWeight = FontWeight.Bold, fontSize = 15.sp)
    val nav = TextStyle(fontFamily = sans, fontWeight = FontWeight.Bold, fontSize = 12.sp)
}

private val RybyColors = darkColorScheme(
    primary = Pal.Cyan,
    onPrimary = Pal.OnCyan,
    primaryContainer = Color(0xFF0B3A4C),
    onPrimaryContainer = Color(0xFFBDF1FF),
    secondary = Pal.Accent,
    onSecondary = Pal.OnCyan,
    background = Pal.Bg,
    onBackground = Pal.Text,
    surface = Pal.Surface,
    onSurface = Pal.Text,
    surfaceVariant = Pal.Surface2,
    onSurfaceVariant = Pal.TextDim,
    secondaryContainer = Pal.Surface3,
    outline = Pal.BorderStrong,
    outlineVariant = Pal.Border,
    error = Pal.Err,
    onError = Pal.OnCyan
)

@Composable
fun RybyTheme(content: @Composable () -> Unit) {
    MaterialTheme(colorScheme = RybyColors, content = content)
}
