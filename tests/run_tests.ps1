# Usage: .\tests\run_tests.ps1 [filtre]
# Ex:    .\tests\run_tests.ps1        -> tous les tests
#        .\tests\run_tests.ps1 01     -> seulement test 01

param([string]$Filter = "")

$Root     = Split-Path $PSScriptRoot -Parent
$Artisan  = "$Root\cmake-build-debug\tools\artisan\artisan.exe"
$VM       = "$Root\cmake-build-debug\varablizer.exe"
$TestsDir = "$Root\tests"

$Pass = 0
$Fail = 0

Write-Host "=== Varablizer Test Suite ===" -ForegroundColor Cyan
Write-Host ""

foreach ($vz in Get-ChildItem "$TestsDir\[0-9]*.vz" | Sort-Object Name) {
    $name = $vz.BaseName
    if ($Filter -and $name -notlike "*$Filter*") { continue }

    $vbin = $vz.FullName -replace '\.vz$', '.vbin'

    # Compile
    $compileOut = & $Artisan compile $vz.FullName -o $vbin 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  [COMPILE ERROR] $name" -ForegroundColor Red
        Write-Host "    $compileOut"
        $Fail++
        continue
    }

    # Expected output: lignes commençant par "//   "
    $expected = (Get-Content $vz.FullName |
        Where-Object { $_ -match '^//   ' } |
        ForEach-Object { $_ -replace '^//   ', '' }) -join "`n"

    # Execute
    $actual = (& $VM $vbin 2>$null) -join "`n"

    # Compare
    if ($actual -eq $expected) {
        Write-Host "  [PASS] $name" -ForegroundColor Green
        $Pass++
    } else {
        Write-Host "  [FAIL] $name" -ForegroundColor Red
        Write-Host "    Expected: $($expected -replace "`n", ' | ')"
        Write-Host "    Actual:   $($actual   -replace "`n", ' | ')"
        $Fail++
    }
}

Write-Host ""
Write-Host "=== Results: $Pass passed, $Fail failed ===" -ForegroundColor Cyan
