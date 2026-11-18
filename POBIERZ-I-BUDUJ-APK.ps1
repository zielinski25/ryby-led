# POBIERZ-I-BUDUJ-APK.ps1 - pobiera wlasciwy branch Androida i buduje APK.
# Uruchamiany przez one-liner z repo albo recznie. Nie zmienia istniejacych katalogow.
[CmdletBinding()]
param([string]$FirmwareSecrets = "", [switch]$Release, [switch]$RunTests)
$ErrorActionPreference = 'Stop'
$Repo = 'https://github.com/zielinski25/ryby-led.git'
$Branch = 'arena/426726eb-ryby-led'
$BuildRoot = Join-Path $env:USERPROFILE ('ryby-led-build-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))

Write-Host '=== Akwarium LED: pobieranie aktualnego branchu i budowanie APK ===' -ForegroundColor Cyan
Write-Host "Branch: $Branch" -ForegroundColor Cyan
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw 'Nie znaleziono git. Zainstaluj Git for Windows, uruchom ponownie PowerShell i sprobuj ponownie.'
}
& git clone --single-branch --branch $Branch $Repo $BuildRoot
if ($LASTEXITCODE -ne 0) { throw "Klonowanie branchu nie powiodlo sie (kod $LASTEXITCODE)." }
$AndroidDir = Join-Path $BuildRoot 'android-zrodla\akwarium-led-native'
$Gradle = Join-Path $AndroidDir 'gradlew.bat'
$Builder = Join-Path $BuildRoot 'BUDUJ-APK.ps1'
if (-not (Test-Path -LiteralPath $Gradle) -or -not (Test-Path -LiteralPath $Builder)) {
    throw "Repo pobrane, ale brakuje plikow budowania. Oczekiwano: $Gradle"
}
Write-Host "Zrodla pobrane do: $AndroidDir" -ForegroundColor Green
Push-Location $AndroidDir
try {
    $BuildArgs = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $Builder)
    if ($FirmwareSecrets) { $BuildArgs += @('-FirmwareSecrets', $FirmwareSecrets) }
    if ($Release) { $BuildArgs += '-Release' }
    if ($RunTests) { $BuildArgs += '-RunTests' }
    & powershell.exe @BuildArgs
    if ($LASTEXITCODE -ne 0) { throw "Budowanie APK nie powiodlo sie (kod $LASTEXITCODE)." }
} finally {
    Pop-Location
}
