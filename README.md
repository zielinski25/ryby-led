# Akwarium LED — aplikacja w 100% natywna (Android / Jetpack Compose)

Sterowanie sterownikiem oświetlenia akwarium ESP32-S3 (`ryby-led-s3-01`).
Interfejs kafelkowy odtworzony **natywnymi komponentami Compose** — bez WebView,
bez HTML, bez JavaScriptu i bez mostka JS.

- pakiet: `pl.aquarium.lednative`
- wersja: `1.0.0-native`
- minSdk 26 (Android 8.0), target/compileSdk 35
- Kotlin 2.0.21, AGP 8.7.3, Gradle 8.9, JDK 17
- Compose BOM 2024.10.01, Material 3, OkHttp 4.12.0 (WebSocket)
- artefakty UI: `ui-graphics`, `ui-tooling-preview`, `ui-tooling` (przedrostek `ui-`)

## Dlaczego „natywna”, a nie hybrydowa

| Warstwa | Implementacja |
|---|---|
| UI | Jetpack Compose (`androidx.compose.*`), Material 3 |
| Nawigacja | własny stan + `Crossfade`, natywny dolny pasek 8 zakładek |
| Wizualizacja akwarium | `Canvas` (belka LED, stożek światła, fala, ryby, bąbelki) |
| Wykresy | `Canvas` (liniowe z gradientem, słupkowe, paski kanałów) |
| Suwaki PWM | `Slider` (Material 3) |
| Terminal | `OkHttpClient.newWebSocket` → `ws://ESP:8080/ws` |
| HTTP | `HttpURLConnection` z limitem rozmiaru i bez przekierowań |
| Logi/pliki | REST ESP + `MediaStore.Downloads` |
| Skarbiec | AES/GCM w Android Keystore |

W projekcie nie ma katalogu `assets/`, żadnego pliku `.html`, `.js`, `.css`,
żadnego wystąpienia `WebView`, `loadUrl`, `JavascriptInterface` ani
`evaluateJavascript`.

## Budowanie

Najszybciej (Windows, z wstrzyknięciem `CMD_TOKEN` z `secrets.h`):

```powershell
.\BUDUJ-APK.bat
# lub
powershell -ExecutionPolicy Bypass -File .\BUDUJ-APK.ps1
```

Skrypt wykrywa JDK 17–22 i Android SDK, wstrzykuje token przez `BuildConfig`
i kopiuje APK na Pulpit. Szczegóły: `INSTRUKCJA-PL.txt`.

Build ręczny:

```powershell
$env:RYBY_CMD_TOKEN = "<token z secrets.h>"
.\gradlew.bat :app:assembleDebug
Remove-Item Env:RYBY_CMD_TOKEN
```

> **Bez `RYBY_CMD_TOKEN` aplikacja się zbuduje, ale każda komenda zostanie
> odrzucona przez firmware jako `reason=invalid`.** Zakładka *Więcej* pokazuje,
> czy token jest obecny (nigdy jego wartości).

Testy jednostkowe logiki i parserów (JVM, bez emulatora):

```powershell
.\gradlew.bat :app:testDebugUnitTest
```

## Architektura

```
core/
  Logic.kt            stałe, walidacja, PWM, ts, świeżość danych, formatowanie
  Models.kt           DeviceStatus (parser chmura + LAN), harmonogramy, logi, FS
  Http.kt             HTTP bez redirectów, limity 16 KB / 256 KB / 1 MB
  Rtdb.kt             Firebase Auth (signInWithPassword + refresh) i RTDB REST
  LanClient.kt        HTTP do ESP, wykrywanie (mDNS → skan podsieci /32 wątki)
  TerminalClient.kt   natywny WebSocket do /ws
  DeviceRepository.kt orkiestracja: cloud-first, LAN jako fallback
  AppPrefs.kt         preferencje; CMD_TOKEN tylko z BuildConfig
  SecretBox.kt        AES/GCM Keystore dla refresh tokenu
ui/
  Theme.kt            paleta i typografia (ciemny motyw)
  AppViewModel.kt     stan, polling 15 s, komendy, logi, terminal, eksport
  AppRoot.kt          Scaffold + dolna nawigacja (8 zakładek)
  components/         kafelki, kontrolki, Canvas akwarium, wykresy
  screens/            Pulpit, Światło, Plan, Czujniki, Wykresy, Logi, Terminal, Więcej
```

## Kontrakt z urządzeniem

**Chmura (priorytet).** Baza
`https://akwarium-367be-default-rtdb.europe-west1.firebasedatabase.app`,
ścieżka `/aquarium/ryby-led-s3-01`.

- odczyt: `GET /status.json?auth=<idToken>`
- komendy: `POST /commands.json` → `{cmd, token, ts}`; `ts` monotonicznie rosnący
- konfiguracja: `PATCH /config.json` → `{token, cfgTs, configType, …}`
  plus komenda `config_refresh`
- `configType`: `schedule` / `adapt` / `minlux` / `params` / `pump`
- `pumpSlots` zapisywane jako klucze `"0"`..`"5"` (wymóg reguł RTDB);
  firmware odczytuje je z GET jako tablicę `[{s,e}, …]`
- kolejka komend jest odpytywana przez ESP mniej więcej co 14 s

**LAN (fallback + funkcje lokalne).** `http://<esp>:8080`

- `GET /api/status`, `/set-power?v=`, `/set-tryb?v=`, `/set-pwm?pwm0..4=`
- `/set-schedule?mw&me&mo&eb`, `/set-fade?f=`, `/set-pump-schedule` (POST)
- `/set-ema-filter?v=`, `/set-sens-int?v=`, `/set-ramp-sec?v=`
  (uwaga: lokalnie ESP przyjmuje 5–120, węziej niż chmura)
- `/api/adaptive/save`, `/api/minlux/save`, `/api/quick/{pump_on,pump_off,led_test,led_off,restart}`
- logi: `/api/log-lines?lines=`, `/api/log-download`, `/api/log-clear`
- pliki: `/api/fs-list`, `/api/fs-view?file=`
- terminal: `ws://<esp>:8080/ws` — jedna komenda = jedna pełna ramka tekstowa
  (firmware ignoruje ramki fragmentowane)

Status ma dwa kształty i jeden parser: chmura zwraca `mode` jako tekst oraz
`temps[]`, LAN zwraca `tryb` jako boolean oraz `temp1/temp2/water`.

## Świeżość danych

`updatedAt` to `millis()` sterownika, więc pojedynczy odczyt nie dowodzi, że ESP
żyje. `Logic.trackUpdate` wymaga **obserwacji zmiany** tej wartości:

- `UNVERIFIED` — pierwszy odczyt, brak potwierdzenia zmiany
- `ONLINE` — zaobserwowano zmianę `updatedAt`
- `STALE` — brak zmiany przez 150 s

Odczyt LAN oznaczany jest jako `ONLINE`, bo odpowiedź przyszła wprost z ESP.

## Bezpieczeństwo

- `CMD_TOKEN` wyłącznie z build-time `BuildConfig`; brak pola w UI
- hasło Firebase niezapisywane; refresh token w AES/GCM (Keystore)
- walidacja adresu bazy: HTTPS, tylko `*.firebasedatabase.app` /
  `*.firebaseio.com`, bez query i userinfo
- HTTP bez podążania za przekierowaniami, z twardym limitem odpowiedzi
- `allowBackup=false`
- aplikacja nie modyfikuje reguł Firebase ani firmware

## Czego ten projekt nie robi

- nie zmienia reguł RTDB ani firmware ESP,
- nie zawiera gotowego APK (budujesz lokalnie),
- nie obsługuje konfiguracji Telegrama (reguły odrzucają `tgBotToken`),
- nie używa endpointu lokalizacji (stary firmware go nie ma).
