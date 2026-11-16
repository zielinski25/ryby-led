$ErrorActionPreference = 'Stop'

$project = Join-Path $env:USERPROFILE 'Downloads\co-0.31.89-build\SterownikCO-0.31.83'
$patch = Join-Path $env:TEMP 'sterownikco-0.31.100-widget-zew.patch'
$patchUrl = 'https://github.com/zielinski25/ryby-led/raw/refs/heads/arena/318fd089-ryby-led/dodatki/temperatury-zew/android-0.31.99-do-0.31.100-widget-zew.patch'
$sha256 = 'A262BD021394B7A199E470F3DD28A6554D82B8F1219C5C3914F309892B326884'

if (-not (Test-Path (Join-Path $project 'app\build.gradle.kts'))) { throw "Nie znaleziono projektu: $project" }
Get-Command git -ErrorAction Stop | Out-Null
Invoke-WebRequest -Uri $patchUrl -OutFile $patch
if ((Get-FileHash -LiteralPath $patch -Algorithm SHA256).Hash -ne $sha256) { throw 'Nieprawidlowa suma pobranej poprawki. Przerwano.' }
Set-Location -LiteralPath $project

$gradle = Get-Content -LiteralPath 'app\build.gradle.kts' -Raw
if ($gradle -match 'versionName\s*=\s*"0\.31\.99"' -and $gradle -match 'versionCode\s*=\s*31099') {
    git apply --check --ignore-space-change "$patch"
    if ($LASTEXITCODE -ne 0) { throw 'Poprawka nie pasuje do projektu. Nie zmieniono plikow.' }
    git apply --ignore-space-change "$patch"
    if ($LASTEXITCODE -ne 0) { throw 'Nie udalo sie zastosowac poprawki.' }
} elseif ($gradle -match 'versionName\s*=\s*"0\.31\.100"' -and $gradle -match 'versionCode\s*=\s*31100') {
    git apply --reverse --check --ignore-space-change "$patch"
    if ($LASTEXITCODE -ne 0) { throw 'Projekt ma numer 0.31.100, ale nie odpowiada tej poprawce.' }
    Write-Host 'Poprawka jest juz w projekcie - ponawiam testy i budowanie.'
} else {
    throw 'Nie rozpoznano projektu 0.31.99. Przerwano bez zmiany plikow.'
}

& '.\gradlew.bat' ':app:testDebugUnitTest' ':app:assembleDebug'
if ($LASTEXITCODE -ne 0) { throw 'Testy lub budowanie nie powiodly sie. Nie skopiowano APK.' }
$apk = Join-Path $project 'app\build\outputs\apk\debug\app-debug.apk'
if (-not (Test-Path -LiteralPath $apk)) { throw 'Gradle zakonczyl sie bez bledu, ale brak APK.' }
$desktop = [Environment]::GetFolderPath('Desktop')
if ([string]::IsNullOrWhiteSpace($desktop)) { throw 'Nie znaleziono katalogu pulpitu Windows.' }
$destination = Join-Path $desktop 'SterownikCO-0.31.100-widget-zew-debug.apk'
Copy-Item -LiteralPath $apk -Destination $destination -Force -ErrorAction Stop
Write-Host "GOTOWE: $destination"
