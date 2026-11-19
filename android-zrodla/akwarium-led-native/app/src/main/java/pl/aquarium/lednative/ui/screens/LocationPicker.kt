package pl.aquarium.lednative.ui.screens

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.location.LocationManager
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.core.content.ContextCompat
import androidx.compose.ui.window.Dialog
import androidx.compose.ui.window.DialogProperties
import org.osmdroid.config.Configuration
import org.osmdroid.events.MapEventsReceiver
import org.osmdroid.tileprovider.tilesource.TileSourceFactory
import org.osmdroid.util.GeoPoint
import org.osmdroid.views.MapView
import org.osmdroid.views.overlay.MapEventsOverlay
import org.osmdroid.views.overlay.Marker
import pl.aquarium.lednative.ui.AppViewModel

@Composable
fun LocationPicker(vm: AppViewModel, onDismiss: () -> Unit) {
    val context = LocalContext.current
    var selected by remember(vm.espLocation) {
        mutableStateOf(vm.espLocation?.let { GeoPoint(it.first, it.second) })
    }
    var mapRef by remember { mutableStateOf<MapView?>(null) }
    var marker by remember { mutableStateOf<Marker?>(null) }
    var info by remember { mutableStateOf("Dotknij mapy, aby wskazać lokalizację akwarium.") }

    val locationPermission = rememberLauncherForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { grants ->
        if (grants.values.any { it }) {
            val lm = context.getSystemService(Context.LOCATION_SERVICE) as LocationManager
            val last = listOf(LocationManager.GPS_PROVIDER, LocationManager.NETWORK_PROVIDER)
                .firstNotNullOfOrNull { provider ->
                    try { if (lm.isProviderEnabled(provider)) lm.getLastKnownLocation(provider) else null }
                    catch (_: SecurityException) { null }
                }
            if (last != null) {
                val point = GeoPoint(last.latitude, last.longitude)
                selected = point
                mapRef?.controller?.animateTo(point)
                marker?.position = point
                marker?.let { mapRef?.invalidate() }
                info = "Ustawiono bieżącą pozycję telefonu. Sprawdź pinezkę i zatwierdź wysłanie do ESP."
            } else info = "Brak ostatniej pozycji. Włącz lokalizację telefonu i spróbuj ponownie."
        } else info = "Brak zgody na lokalizację. Możesz nadal wskazać punkt, dotykając mapy."
    }

    DisposableEffect(Unit) {
        Configuration.getInstance().userAgentValue = context.packageName
        onDispose { mapRef?.onPause(); mapRef?.onDetach(); mapRef = null }
    }

    AlertDialog(
            onDismissRequest = onDismiss,
            title = { Text("Wybierz lokalizację akwarium") },
            text = {
                Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    Text(info)
                    AndroidView(
                        modifier = Modifier.fillMaxWidth().height(390.dp),
                        factory = { ctx ->
                            MapView(ctx).apply {
                                onResume()
                                setTileSource(TileSourceFactory.MAPNIK)
                                setMultiTouchControls(true)
                                val start = selected ?: GeoPoint(52.0, 19.0)
                                controller.setZoom(if (vm.espLocation != null) 10.0 else 6.0)
                                controller.setCenter(start)
                                val pin = Marker(this).apply {
                                    position = start
                                    title = "Lokalizacja akwarium"
                                }
                                marker = pin
                                if (selected != null) overlays.add(pin)
                                overlays.add(MapEventsOverlay(object : MapEventsReceiver {
                                    override fun singleTapConfirmedHelper(point: GeoPoint): Boolean {
                                        selected = point
                                        pin.position = point
                                        if (!overlays.contains(pin)) overlays.add(pin)
                                        invalidate()
                                        return true
                                    }
                                    override fun longPressHelper(point: GeoPoint): Boolean = false
                                }))
                                mapRef = this
                            }
                        }
                    )
                    selected?.let { Text("Wybrane: %.5f°, %.5f°".format(it.latitude, it.longitude)) }
                    Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                        OutlinedButton(onClick = {
                            val fine = ContextCompat.checkSelfPermission(context, Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED
                            val coarse = ContextCompat.checkSelfPermission(context, Manifest.permission.ACCESS_COARSE_LOCATION) == PackageManager.PERMISSION_GRANTED
                            if (fine || coarse) {
                                val lm = context.getSystemService(Context.LOCATION_SERVICE) as LocationManager
                                val last = listOf(LocationManager.GPS_PROVIDER, LocationManager.NETWORK_PROVIDER)
                                    .firstNotNullOfOrNull { provider -> try { if (lm.isProviderEnabled(provider)) lm.getLastKnownLocation(provider) else null } catch (_: SecurityException) { null } }
                                if (last != null) {
                                    val point = GeoPoint(last.latitude, last.longitude)
                                    selected = point
                                    marker?.position = point
                                    mapRef?.controller?.animateTo(point)
                                    mapRef?.invalidate()
                                    info = "Ustawiono bieżącą pozycję telefonu. Zatwierdź, aby wysłać do ESP."
                                } else info = "Nie znaleziono ostatniej pozycji telefonu."
                            } else locationPermission.launch(arrayOf(Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION))
                        }) { Text("Moja lokalizacja") }
                    }
                }
            },
            confirmButton = {
                Button(onClick = { selected?.let { vm.saveEspLocation(it.latitude, it.longitude); onDismiss() } }, enabled = selected != null && !vm.busy) {
                    Text(if (vm.busy) "Wysyłanie…" else "Zapisz w ESP")
                }
            },
            dismissButton = { OutlinedButton(onClick = onDismiss) { Text("Anuluj") } },
            properties = DialogProperties(usePlatformDefaultWidth = false)
        )
}
