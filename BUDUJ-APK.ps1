# BUDUJ-APK.ps1 - buduje APK "Akwarium LED NATIVE" (100% Jetpack Compose) na Windows.
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\BUDUJ-APK.ps1
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\BUDUJ-APK.ps1 -FirmwareSecrets "C:\...\secrets.h"
#
# Albo po prostu dwuklik na BUDUJ-APK.bat.
#
# Skrypt czyta CMD_TOKEN z firmware secrets.h i wstrzykuje go do APK przez
# BuildConfig. Wartosc tokena NIE jest wyswietlana ani zapisywana w zrodlach.
# Bez tego kroku aplikacja zbuduje sie, ale ESP odrzuci kazda komende (reason=invalid).
#
# KODOWANIE: ten plik jest celowo w czystym ASCII (bez polskich znakow i bez
# myslnikow typograficznych) i ma BOM UTF-8. Windows PowerShell 5.1 czyta pliki
# .ps1 bez BOM jako ANSI/CP1250 - znak U+2014 zamienialby sie wtedy na sekwencje
# konczaca sie curly quote, co rozwalalo stringi i cale parsowanie skryptu.

[CmdletBinding()]
param(
    [string]$FirmwareSecrets = "",
    [switch]$Release,
    [switch]$RunTests
)

$ErrorActionPreference = "Stop"
try {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
} catch {
    # Starsze systemy moga nie znac Tls12 - nie przerywamy z tego powodu.
}

# Projekt jest w TYM katalogu (gradlew.bat obok skryptu).
$Proj = $PSScriptRoot
$Proj = $null
$curr = Get-Location
for ($i = 0; $i -lt 5; $i++) {
    if (Test-Path -LiteralPath (Join-Path $curr 'gradlew.bat')) {
        $Proj = $curr
        break
    }
    $curr = Split-Path -Parent $curr
}
if (-not $Proj) {
    throw "Nie znaleziono gradlew.bat w biezacym katalogu ani w katalogach nadrzednych. Uruchom skrypt z folderu zawierajacego zrodla Androida."
}
Write-Host "Zrodla Androida: $Proj" -ForegroundColor Green
$MainSrc = Join-Path $Proj 'app\src\main\java\pl\aquarium\lednative\MainActivity.kt'
if (-not (Test-Path -LiteralPath $MainSrc)) {
    throw "Niekompletny pakiet: brak zrodel aplikacji. Rozpakuj archiwum ponownie."
}
$Desktop = [Environment]::GetFolderPath("Desktop")

Write-Host ""
Write-Host "=== Akwarium LED - aplikacja NATYWNA (Compose, zero WebView) ===" -ForegroundColor Cyan
Write-Host ""

# --- 1. CMD_TOKEN z firmware secrets.h ---
Write-Host "== 1/5 CMD_TOKEN z secrets.h..." -ForegroundColor Cyan
if ([string]::IsNullOrWhiteSpace($FirmwareSecrets)) {
    $defaultSecrets = 'C:\Users\Rafcio\Documents\PlatformIO\260526-184242-esp32-s3-devkitc-1\src\secrets.h'
    if (Test-Path -LiteralPath $defaultSecrets) {
        $FirmwareSecrets = $defaultSecrets
        Write-Host "Uzywam domyslnego secrets.h z projektu PlatformIO." -ForegroundColor Green
    } else {
        Write-Host 'Podaj sciezke do firmware secrets.h (plik z definicja CMD_TOKEN).' -ForegroundColor Cyan
        try {
            Add-Type -AssemblyName System.Windows.Forms
            $dlg = New-Object System.Windows.Forms.OpenFileDialog
            $dlg.Title = "Wybierz firmware secrets.h"
            $dlg.Filter = "secrets.h (*.h)|*.h|Wszystkie pliki (*.*)|*.*"
            $dlg.CheckFileExists = $true
            if ($dlg.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
                $FirmwareSecrets = $dlg.FileName
            }
        } catch {
            $FirmwareSecrets = Read-Host "Sciezka do secrets.h"
        }
    }
}
if ([string]::IsNullOrWhiteSpace($FirmwareSecrets) -or -not (Test-Path -LiteralPath $FirmwareSecrets -PathType Leaf)) {
    throw "Nie znaleziono pliku secrets.h. Build przerwany."
}
$SourceText = [System.IO.File]::ReadAllText($FirmwareSecrets, [System.Text.Encoding]::UTF8)
$TokenPattern = '(?m)^\s*#\s*define\s+CMD_TOKEN\s+"([^"\r\n\\]*)"\s*(?://.*)?$'
$TokenMatch = [regex]::Match($SourceText, $TokenPattern)
if (-not $TokenMatch.Success) {
    throw 'Nie znaleziono prostego CMD_TOKEN w cudzyslowie w secrets.h. Build przerwany.'
}
$CmdToken = $TokenMatch.Groups[1].Value
if ([string]::IsNullOrWhiteSpace($CmdToken) -or $CmdToken -eq 'ZMIEN_MNIE' -or $CmdToken.Length -gt 128) {
    throw 'CMD_TOKEN pusty, przykladowy lub dluzszy niz 128 znakow. Build przerwany.'
}
$env:RYBY_CMD_TOKEN = $CmdToken
Write-Host ("CMD_TOKEN odczytany lokalnie ({0} znakow); wartosc nie bedzie wyswietlona." -f $CmdToken.Length) -ForegroundColor Green

function Test-Jdk17To22([string]$HomeDir) {
    if ([string]::IsNullOrWhiteSpace($HomeDir)) { return $false }
    $javaExe = Join-Path $HomeDir 'bin\java.exe'
    if (-not (Test-Path -LiteralPath $javaExe)) { return $false }
    $prev = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $out = (& $javaExe -version 2>&1 | Out-String)
    } catch {
        $out = ""
    } finally {
        $ErrorActionPreference = $prev
    }
    return ($out -match '"(17|18|19|20|21|22)\.')
}

try {
    Write-Host "== 2/5 Szukam JDK 17-22 (Gradle 8.9 nie dziala z Java 23+)..." -ForegroundColor Cyan
    $foundJdk = $null
    $roots = @(
        "$env:USERPROFILE\.jdks",
        "C:\Program Files\Eclipse Adoptium",
        "C:\Program Files\Microsoft",
        "C:\Program Files\Java",
        "C:\Program Files\Amazon Corretto",
        "C:\Program Files\Zulu",
        "C:\Program Files\BellSoft"
    )
    $cands = New-Object System.Collections.ArrayList
    if ($env:JAVA_HOME) { [void]$cands.Add($env:JAVA_HOME) }
    $onPath = Get-Command java -ErrorAction SilentlyContinue
    if ($onPath) { [void]$cands.Add((Split-Path (Split-Path $onPath.Source -Parent) -Parent)) }
    # Android Studio JBR jest zwykle JDK 21 - sprawdzamy go wczesnie.
    [void]$cands.Add("C:\Program Files\Android\Android Studio\jbr")
    foreach ($r in $roots) {
        if (Test-Path -LiteralPath $r) {
            Get-ChildItem -LiteralPath $r -Directory -ErrorAction SilentlyContinue |
                Sort-Object Name -Descending |
                ForEach-Object { [void]$cands.Add($_.FullName) }
        }
    }
    foreach ($c in $cands) {
        if (Test-Jdk17To22 $c) { $foundJdk = $c; break }
    }

    if (-not $foundJdk) {
        Write-Host "Nie znaleziono JDK 17-22. Instaluje Microsoft OpenJDK 21 przez winget..." -ForegroundColor Yellow
        $winget = Get-Command winget -ErrorAction SilentlyContinue
        if ($winget) {
            & winget install --id Microsoft.OpenJDK.21 -e --accept-package-agreements --accept-source-agreements
        } else {
            Write-Host "Brak winget. Zainstaluj JDK 21 recznie (np. Microsoft OpenJDK 21)." -ForegroundColor Yellow
        }
        foreach ($r in @("C:\Program Files\Microsoft", "C:\Program Files\Eclipse Adoptium", "C:\Program Files\Java")) {
            if (Test-Path -LiteralPath $r) {
                Get-ChildItem -LiteralPath $r -Directory -ErrorAction SilentlyContinue |
                    ForEach-Object {
                        if ((-not $foundJdk) -and (Test-Jdk17To22 $_.FullName)) { $foundJdk = $_.FullName }
                    }
            }
        }
    }
    if (-not $foundJdk) {
        throw "Nie udalo sie znalezc JDK 17-22. Zainstaluj JDK 21 i uruchom ponownie."
    }
    $env:JAVA_HOME = $foundJdk
    $env:Path = "$foundJdk\bin;$env:Path"
    Write-Host "JDK: $foundJdk" -ForegroundColor Green

    Write-Host "== 3/5 Szukam Android SDK..." -ForegroundColor Cyan
    $sdkCands = @(
        $env:ANDROID_HOME,
        $env:ANDROID_SDK_ROOT,
        (Join-Path $env:LOCALAPPDATA 'Android\Sdk'),
        "C:\Android\Sdk",
        "D:\Android\Sdk",
        "C:\Program Files\Android\Sdk"
    )
    $Sdk = $sdkCands |
        Where-Object { $_ -and (Test-Path -LiteralPath (Join-Path $_ 'platforms')) } |
        Select-Object -First 1
    if (-not $Sdk) {
        throw "Nie znaleziono Android SDK. Otworz Android Studio -> Tools -> SDK Manager i zainstaluj 'Android 15 (API 35)' oraz 'Android SDK Build-Tools'."
    }
    $env:ANDROID_HOME = $Sdk
    $env:ANDROID_SDK_ROOT = $Sdk
    $sdkLine = 'sdk.dir=' + ($Sdk -replace '\\', '/')
    Set-Content -LiteralPath (Join-Path $Proj 'local.properties') -Value $sdkLine -Encoding ASCII
    Write-Host "Android SDK: $Sdk" -ForegroundColor Green

    if ($Release) { $task = ':app:assembleRelease' } else { $task = ':app:assembleDebug' }
    Write-Host "== 4/5 Budowanie APK (Gradle 8.9, $task)..." -ForegroundColor Cyan
    Write-Host "Pierwsze budowanie pobiera Compose z sieci - moze potrwac kilka minut." -ForegroundColor Yellow
    Push-Location -LiteralPath $Proj
    # Gradle pisze postep na stderr. Przy ErrorActionPreference=Stop PS 5.1
    # potrafi z tego zrobic wyjatek, dlatego na czas builda przelaczamy na Continue
    # i sami sprawdzamy $LASTEXITCODE.
    $prevEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        & .\gradlew.bat --stop 2>&1 | Out-Null
        if ($RunTests) {
            & .\gradlew.bat :app:testDebugUnitTest --console=plain
            if ($LASTEXITCODE -ne 0) { throw "Testy jednostkowe nie przeszly (kod $LASTEXITCODE)." }
        }
        & .\gradlew.bat $task --console=plain
        $buildCode = $LASTEXITCODE
        if ($buildCode -ne 0) {
            throw "Build nieudany (kod $buildCode). Skopiuj linie bledu powyzej."
        }
    } finally {
        & .\gradlew.bat --stop 2>&1 | Out-Null
        $ErrorActionPreference = $prevEap
        Pop-Location
    }

    Write-Host "== 5/5 Kopiuje APK na Pulpit..." -ForegroundColor Cyan
    if ($Release) {
        $apk = Join-Path $Proj 'app\build\outputs\apk\release\app-release.apk'
    } else {
        $apk = Join-Path $Proj 'app\build\outputs\apk\debug\app-debug.apk'
    }
    if (-not (Test-Path -LiteralPath $apk)) { throw "Brak pliku APK: $apk" }
    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $dest = Join-Path $Desktop ("akwarium-led-native-$stamp.apk")
    Copy-Item -LiteralPath $apk -Destination $dest
    $sizeMb = [math]::Round((Get-Item -LiteralPath $dest).Length / 1MB, 2)
    Write-Host ""
    Write-Host ("GOTOWE: {0} ({1} MB)" -f $dest, $sizeMb) -ForegroundColor Green
    Write-Host ""
    Write-Host "Co dalej:" -ForegroundColor Cyan
    Write-Host "  1. Przenies APK na telefon i zainstaluj (zezwol na instalacje z nieznanych zrodel)."
    Write-Host "  2. Otworz aplikacje -> zakladka Wiecej -> zaloguj sie do Firebase (e-mail + haslo)."
    Write-Host "  3. Bedac w tej samej sieci WiFi: Wiecej -> Wykryj ESP (logi, pliki, terminal)."
    Write-Host ""
    Write-Host "CMD_TOKEN jest juz w APK (z secrets.h). Nie wpisuje sie go w aplikacji." -ForegroundColor Green
    Write-Host "Aplikacja jest w 100% natywna: Kotlin + Jetpack Compose, zero WebView i zero HTML." -ForegroundColor Green
} finally {
    Remove-Item Env:RYBY_CMD_TOKEN -ErrorAction SilentlyContinue
}
