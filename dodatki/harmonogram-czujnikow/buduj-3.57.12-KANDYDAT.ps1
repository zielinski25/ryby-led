# Kandydat do testow: buduje na Windows, nie wgrywa i nie publikuje OTA.
$ErrorActionPreference = 'Stop'
$project = Join-Path $env:USERPROFILE 'Documents\PlatformIO\sterownik-co-esp32'
$source = Join-Path $project 'src\main_centrala.cpp'
$patch = Join-Path $env:TEMP 'centrala-3.57.11-do-3.57.12-KANDYDAT.patch'
$patchUrl = 'https://github.com/zielinski25/ryby-led/raw/refs/heads/arena/318fd089-ryby-led/dodatki/harmonogram-czujnikow/centrala-3.57.11-do-3.57.12-KANDYDAT.patch'
$expectedPatch = 'DEB617AF47E01E1CCFDE242851B3E45208AA0F1889A8509E760112CD46BF7FEF'
$expectedBase = '6E86F349A12B9C1BE94EE18FC28E12991DCB07927FD4B2DC28B7F6084D8F503D'
$expectedModified = 'EA6256BEBE8BDF370388FC80BD61231F7297A09C4DBE02F52E77167F31D8D2F2'
function Get-NormalizedHash([string]$path) {
    $text = [IO.File]::ReadAllText($path, [Text.Encoding]::UTF8).Replace("`r`n", "`n")
    $hash = [Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($hash.ComputeHash([Text.Encoding]::UTF8.GetBytes($text)))).Replace('-', '') }
    finally { $hash.Dispose() }
}
if (-not (Test-Path -LiteralPath $source)) { throw "Nie znaleziono: $source" }
$ini = Join-Path $project 'platformio.ini'
if (-not (Test-Path -LiteralPath $ini)) { throw 'Brak platformio.ini.' }
if (-not ((Get-Content -LiteralPath $ini -Raw) -match '(?m)^\s*\[env:esp32-s3-n16r8\]\s*$')) { throw 'Brak srodowiska esp32-s3-n16r8. Nic nie zmieniono.' }
Get-Command git -ErrorAction Stop | Out-Null
$pioCmd = Get-Command pio -ErrorAction SilentlyContinue
if ($pioCmd) { $pio = $pioCmd.Source } else { $pio = Join-Path $env:USERPROFILE '.platformio\penv\Scripts\platformio.exe' }
if (-not (Test-Path -LiteralPath $pio)) { throw 'Nie znaleziono PlatformIO.' }
Invoke-WebRequest -Uri $patchUrl -OutFile $patch
if ((Get-FileHash -LiteralPath $patch -Algorithm SHA256).Hash -ne $expectedPatch) { throw 'Nieprawidlowa suma poprawki. Nic nie zmieniono.' }
Set-Location -LiteralPath $project
$sourceHash = Get-NormalizedHash $source
if ($sourceHash -eq $expectedBase) {
    git apply --check --ignore-space-change "$patch"
    if ($LASTEXITCODE -ne 0) { throw 'Poprawka nie pasuje. Nic nie zmieniono.' }
    $backup = Join-Path $project ("main_centrala.backup-" + (Get-Date -Format 'yyyyMMdd-HHmmss-fff') + '.cpp.txt')
    Copy-Item -LiteralPath $source -Destination $backup -ErrorAction Stop
    Write-Host "Kopia zrodla: $backup"
    git apply --ignore-space-change "$patch"
    if ($LASTEXITCODE -ne 0) { throw 'Nie udalo sie zastosowac poprawki; kopia jest w folderze projektu.' }
} elseif ($sourceHash -eq $expectedModified) {
    Write-Host 'Poprawka jest juz zastosowana; ponawiam budowanie.'
} else {
    throw 'Zrodlo nie jest identyczne z kandydatem 3.57.11. Nic nie zmieniono. Nie obchodzic kontroli.'
}
if ((Get-NormalizedHash $source) -ne $expectedModified) { throw 'Weryfikacja zrodla nieudana. Nie buduje; kopia jest w folderze projektu.' }
& $pio run -e esp32-s3-n16r8
if ($LASTEXITCODE -ne 0) { throw 'Budowanie nie powiodlo sie. Nie skopiowano pliku BIN.' }
$bin = Join-Path $project '.pio\build\esp32-s3-n16r8\firmware.bin'
if (-not (Test-Path -LiteralPath $bin)) { throw 'Brak firmware.bin po budowaniu.' }
$desktop = [Environment]::GetFolderPath('Desktop')
if ([string]::IsNullOrWhiteSpace($desktop)) { throw 'Nie znaleziono pulpitu.' }
$destination = Join-Path $desktop 'SterownikCO-3.57.12-KANDYDAT-NIE-WGRYWAC.bin'
Copy-Item -LiteralPath $bin -Destination $destination -Force -ErrorAction Stop
Write-Host "KANDYDAT ZBUDOWANY: $destination"
Write-Host 'Nie wgrano. Najpierw testy poza pracujacym piecem i kontrola powrotu.'
