# ============================================================================
# build_gate.ps1 — Ryby LED (wzór: run_v33120_build_gate.ps1 Centrali Pieca)
# ============================================================================
# Bramka przed każdym release: kompilacja + kontrola spójności wersji +
# kontrola rozmiaru bin względem partycji app (0x300000 = 3 MB).
# Uruchamianie:  cd firmware; .\scripts\build_gate.ps1
# Wymagania: PlatformIO CLI w PATH.
# ============================================================================

$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")   # -> firmware/

$repoRoot  = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$cppFile   = "src\Ryby_LED_fi_S3.cpp"
$envName   = "esp32-s3-n16r8"

Write-Host "=== BUILD GATE: Ryby LED ===" -ForegroundColor Cyan

# --- 1. Spójność wersji -------------------------------------------------
$fwLine = Select-String -Path $cppFile -Pattern '#define RYBY_FW_VERSION "([^"]+)"' | Select-Object -First 1
if (-not $fwLine) { throw "Nie znaleziono RYBY_FW_VERSION w $cppFile" }
$fwVersion = $fwLine.Matches[0].Groups[1].Value            # np. v4.0.0+build.261

$repoVersion = (Get-Content (Join-Path $repoRoot "version.txt") -First 1).Trim()
$fwSemver = $fwVersion -replace '^v','' -replace '\+.*$',''   # v4.0.0+build.261 -> 4.0.0
if ($fwSemver -ne $repoVersion) {
    throw "Rozjazd wersji: RYBY_FW_VERSION='$fwVersion' (semver '$fwSemver') vs version.txt='$repoVersion'"
}
Write-Host "[OK] wersje spojne: $fwSemver" -ForegroundColor Green

$chgTop = Select-String -Path (Join-Path $repoRoot "CHANGELOG.md") -Pattern '^## \[' | Select-Object -First 1
if ($chgTop.Line -notmatch [regex]::Escape($fwSemver)) {
    throw "Najnowszy wpis CHANGELOG.md nie odpowiada wersji $fwSemver : $($chgTop.Line)"
}
Write-Host "[OK] CHANGELOG.md ma wpis dla $fwSemver" -ForegroundColor Green

# --- 2. Kompilacja ------------------------------------------------------
Write-Host "--- pio run -e $envName ---" -ForegroundColor Cyan
pio run -e $envName
if ($LASTEXITCODE -ne 0) { throw "Kompilacja nie powiodla sie (exit $LASTEXITCODE)" }

# --- 3. Rozmiar bin vs partycja app (3 MB) ------------------------------
$bin = Get-ChildItem ".pio\build\$envName\*.bin" | Select-Object -First 1
if (-not $bin) { throw "Nie znaleziono firmware.bin po kompilacji" }
$partLimit = 0x300000
if ($bin.Length -ge $partLimit) {
    throw "firmware.bin ($($bin.Length) B) >= limit partycji app ($partLimit B)"
}
Write-Host ("[OK] bin {0} B / limit {1} B ({2:P1} zapasu)" -f $bin.Length, $partLimit, (1 - $bin.Length/$partLimit)) -ForegroundColor Green

# --- 4. Artefakty release (bin + elf do addr2line) ----------------------
$art = Join-Path $PSScriptRoot "..\release_artifacts"
New-Item -ItemType Directory -Force -Path $art | Out-Null
Copy-Item $bin.FullName (Join-Path $art "firmware.bin") -Force
$elf = Get-ChildItem ".pio\build\$envName\*.elf" | Select-Object -First 1
if ($elf) { Copy-Item $elf.FullName (Join-Path $art "firmware.elf") -Force }
Write-Host "[OK] artefakty (bin+elf) w firmware\release_artifacts\ — dodac do GitHub Release" -ForegroundColor Green

Write-Host "=== BUILD GATE: ZALICZONY (wersja $fwSemver) ===" -ForegroundColor Green
