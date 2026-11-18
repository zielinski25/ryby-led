package pl.aquarium.lednative

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.viewModels
import androidx.core.view.WindowCompat
import pl.aquarium.lednative.ui.AppRoot
import pl.aquarium.lednative.ui.AppViewModel
import pl.aquarium.lednative.ui.AkwariumTheme

/**
 * Aplikacja w 100% natywna: Jetpack Compose.
 * Zero WebView, zero HTML, zero JavaScript, zero mostka JS.
 */
class MainActivity : ComponentActivity() {

    private val vm: AppViewModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        WindowCompat.setDecorFitsSystemWindows(window, false)
        window.statusBarColor = android.graphics.Color.rgb(6, 13, 24)
        window.navigationBarColor = android.graphics.Color.rgb(6, 13, 24)
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.Q) {
            window.isNavigationBarContrastEnforced = false
        }
        WindowCompat.getInsetsController(window, window.decorView).apply {
            isAppearanceLightStatusBars = false
            isAppearanceLightNavigationBars = false
        }
        setContent {
            AkwariumTheme {
                AppRoot(vm)
            }
        }
    }
}
