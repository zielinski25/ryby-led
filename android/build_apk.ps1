# build_apk.ps1 — buduje APK aplikacji Ryby LED na Windows (PowerShell 5).
# Uruchom z folderu repo (albo z rozpakowanego ZIP-a):
#   powershell -ExecutionPolicy Bypass -File .\android\build_apk.ps1
# Wynik: plik ryby-led-4.7.2-debug.apk na pulpicie.

$ErrorActionPreference = "Stop"
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

# Narzędzia wiersza poleceń Androida (aktualny link: developer.android.com/studio#command-line-tools-only)
$CmdlineZipUrl = "https://dl.google.com/android/repository/commandlinetools-win-14742923_latest.zip"
$AndroidDir = $PSScriptRoot
$Sdk = Join-Path $env:LOCALAPPDATA "Android\Sdk"
$Desktop = [Environment]::GetFolderPath("Desktop")

function Step($text) {
    Write-Host ""
    Write-Host "== $text" -ForegroundColor Cyan
}

# 1) Java (JDK 17–21)
Step "1/5 Java (JDK 17-21)"
$java = Get-Command java -ErrorAction SilentlyContinue
if (-not $java) {
    Write-Host "Nie znaleziono Javy. Instaluję JDK 21 przez winget..."
    winget install --id Microsoft.OpenJDK.21 -e --accept-package-agreements --accept-source-agreements
    Write-Host ""
    Write-Host "Zamknij PowerShell, otwórz go ponownie i uruchom skrypt jeszcze raz." -ForegroundColor Yellow
    exit 1
}
$javaVer = cmd /c "java -version 2>&1"
Write-Host ($javaVer | Select-Object -First 1)

# 2) Android SDK + licencje + pakiety
Step "2/5 Android SDK"
$sdkmanager = Join-Path $Sdk "cmdline-tools\latest\bin\sdkmanager.bat"
if (-not (Test-Path $sdkmanager)) {
    Write-Host "Pobieram narzędzia Android SDK (ok. 150 MB)..."
    New-Item -ItemType Directory -Force -Path $Sdk | Out-Null
    $zip = Join-Path $env:TEMP "android-cmdline-tools.zip"
    Invoke-WebRequest -Uri $CmdlineZipUrl -OutFile $zip -UseBasicParsing
    $tmp = Join-Path $env:TEMP "android-cmdline-tools"
    if (Test-Path $tmp) { Remove-Item $tmp -Recurse -Force }
    Expand-Archive -Path $zip -DestinationPath $tmp -Force
    $latest = Join-Path $Sdk "cmdline-tools\latest"
    if (Test-Path $latest) { Remove-Item $latest -Recurse -Force }
    New-Item -ItemType Directory -Force -Path (Split-Path $latest -Parent) | Out-Null
    Move-Item -Path (Join-Path $tmp "cmdline-tools") -Destination $latest
}
if (-not (Test-Path $sdkmanager)) {
    throw "Nie znaleziono sdkmanager.bat. Sprawdź link w zmiennej CmdlineZipUrl (na górze skryptu)."
}
$env:ANDROID_HOME = $Sdk
$env:ANDROID_SDK_ROOT = $Sdk

Write-Host "Akceptuję licencje SDK..."
$yes = ("y" + [Environment]::NewLine) * 50
$yes | & $sdkmanager --licenses | Out-Null

Write-Host "Instaluję platform-tools, platform 35 i build-tools 35.0.0 (pierwszy raz: kilka minut)..."
& $sdkmanager "platform-tools" "platforms;android-35" "build-tools;35.0.0"
if ($LASTEXITCODE -ne 0) { throw "sdkmanager zakończył się błędem (kod $LASTEXITCODE)." }

# 3) Konfiguracja projektu (local.properties nie trafia do repo)
Step "3/5 Konfiguracja projektu"
$sdkEsc = $Sdk -replace '\\', '\\'
Set-Content -Path (Join-Path $AndroidDir "local.properties") -Value "sdk.dir=$sdkEsc" -Encoding ASCII
Write-Host "sdk.dir=$sdkEsc"

# 4) Build (Gradle pobiera się sam przez gradlew.bat)
Step "4/5 Budowanie APK (pierwszy raz: Gradle pobiera ok. 300 MB, kilka minut)"
Push-Location $AndroidDir
try {
    & .\gradlew.bat :app:assembleDebug
    if ($LASTEXITCODE -ne 0) {
        throw "Build nieudany (kod $LASTEXITCODE). Skopiuj pierwsze linie błędu i wklej je do czatu."
    }
} finally {
    Pop-Location
}

# 5) Kopia APK na pulpit
Step "5/5 Kopiuję APK na pulpit"
$apk = Join-Path $AndroidDir "app\build\outputs\apk\debug\app-debug.apk"
if (-not (Test-Path $apk)) { throw "Brak pliku APK: $apk" }
$dest = Join-Path $Desktop "ryby-led-4.7.2-debug.apk"
Copy-Item $apk $dest -Force
Write-Host ""
Write-Host "GOTOWE: $dest" -ForegroundColor Green
Write-Host "Przenieś ten plik na telefon i zainstaluj (zezwól na instalację z tego źródła)."
