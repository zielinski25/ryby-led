# Builds only. Never uploads firmware to the controller.
$ErrorActionPreference = 'Stop'
$project = Join-Path $env:USERPROFILE 'Documents\PlatformIO\sterownik-co-esp32'
$source = Join-Path $project 'src\main_centrala.cpp'
$patch = Join-Path $env:TEMP 'centrala-3.57.10-zew-wykresy.patch'
$patchUrl = 'https://github.com/zielinski25/ryby-led/raw/refs/heads/arena/318fd089-ryby-led/dodatki/temperatury-zew/centrala-3.57.10-zew-wykresy.patch'
$expectedPatch = 'CE84FD1A3C7C0D982932ACE042164137917B0844A990C1F522C297CF63D13E3B'
$expectedBase = 'E263437B1FB850CB202C4696404A41B7777B0F12AEEF928C7E57788FB2FC158C'
$expectedModified = 'B8985F480DB251849E32EFC414BDD08C5ECE5F07899E98297EF1A9473D093361'

function Get-NormalizedHash([string]$path) {
    $text = [IO.File]::ReadAllText($path, [Text.Encoding]::UTF8).Replace("`r`n", "`n")
    $hash = [Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($hash.ComputeHash([Text.Encoding]::UTF8.GetBytes($text)))).Replace('-', '') }
    finally { $hash.Dispose() }
}

if (-not (Test-Path -LiteralPath $source)) { throw "Nie znaleziono: $source" }
$ini = Join-Path $project 'platformio.ini'
if (-not (Test-Path -LiteralPath $ini)) { throw 'Brak platformio.ini.' }
if (-not ((Get-Content -LiteralPath $ini -Raw) -match '(?m)^\s*\[env:esp32-s3-n16r8\]\s*$')) { throw 'Nie znaleziono srodowiska esp32-s3-n16r8. Przerwano.' }
Get-Command git -ErrorAction Stop | Out-Null
$pioCmd = Get-Command pio -ErrorAction SilentlyContinue
if ($pioCmd) { $pio = $pioCmd.Source } else { $pio = Join-Path $env:USERPROFILE '.platformio\penv\Scripts\platformio.exe' }
if (-not (Test-Path -LiteralPath $pio)) { throw 'Nie znaleziono PlatformIO (pio).' }
Invoke-WebRequest -Uri $patchUrl -OutFile $patch
if ((Get-FileHash -LiteralPath $patch -Algorithm SHA256).Hash -ne $expectedPatch) { throw 'Nieprawidlowa suma poprawki.' }

Set-Location -LiteralPath $project
$sourceHash = Get-NormalizedHash $source
if ($sourceHash -eq $expectedBase) {
    git apply --check --ignore-space-change "$patch"
    if ($LASTEXITCODE -ne 0) { throw 'Poprawka nie pasuje - plik pozostaje bez zmian.' }
    $backup = Join-Path $project ("main_centrala.backup-" + (Get-Date -Format 'yyyyMMdd-HHmmss-fff') + '.cpp.txt')
    Copy-Item -LiteralPath $source -Destination $backup -ErrorAction Stop
    Write-Host "Kopia zrodla: $backup"
    git apply --ignore-space-change "$patch"
    if ($LASTEXITCODE -ne 0) { throw 'Nie udalo sie zastosowac poprawki. Kopia zrodla jest w folderze projektu.' }
} elseif ($sourceHash -eq $expectedModified) {
    Write-Host 'Poprawka jest juz zastosowana - ponawiam budowanie.'
} else {
    throw 'Zrodlo nie jest zgodne z pobrana baza 3.57.9. Nic nie zmieniono. Przeslij informacje o bledzie.'
}
if ((Get-NormalizedHash $source) -ne $expectedModified) { throw 'Weryfikacja zmienionego zrodla nie powiodla sie. Nie buduje.' }

& $pio run -e esp32-s3-n16r8
if ($LASTEXITCODE -ne 0) { throw 'Budowanie PlatformIO nie powiodlo sie. Nie skopiowano pliku bin.' }
$bin = Join-Path $project '.pio\build\esp32-s3-n16r8\firmware.bin'
if (-not (Test-Path -LiteralPath $bin)) { throw 'Brak firmware.bin po budowaniu.' }
$desktop = [Environment]::GetFolderPath('Desktop')
if ([string]::IsNullOrWhiteSpace($desktop)) { throw 'Nie znaleziono pulpitu Windows.' }
$destination = Join-Path $desktop 'SterownikCO-3.57.10-zew-test.bin'
Copy-Item -LiteralPath $bin -Destination $destination -Force -ErrorAction Stop
Write-Host "ZBUDOWANO: $destination"
Write-Host 'To kandydat do testu - nie wgrano go na ESP.'
