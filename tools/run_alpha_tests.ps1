#Requires -Version 5.0
<#
.SYNOPSIS
    ProPhysics Alpha-Test-Runner.

.DESCRIPTION
    Fuehrt die Alpha-Test-Suite sequenziell aus, schreibt pro Test ein
    Log und liefert eine PASS/FAIL-Bilanz mit Exit-Code.

    Ablageort: <repo>\tools\
    Ziel-EXEs: <repo>\bin\          (Default, aus $PSScriptRoot\..\bin)
    Logs:      <repo>\bin\logs\     (Default)

    Prios:
      1  2D-Basis (Unitaer, Born, Gauge, CHSH)        12 Tests
      2  Emergenz (Born, Lorentz, Interferenz, CHSH)  10 Tests
      3  Langlauf (Invarianten, Transport, Soliton)   10 Tests
      4  3D-Torus (Smoke, Invariance, Dispersion)      3 Tests
      5  Hydrogen + Shared-Ref + Tournament            4 Tests
      6  Spin-1/2                                      1 Test
      7  Dirac                                         1 Test
      8  SU(2)-Eichfeld (Etappe 22)                    1 Test
      all                                             42 Tests

    Self-Locating: -ExeDir default = <repo>\bin (relativ zu $PSScriptRoot).

.PARAMETER Prio
    Auswahl der Prioritaetsstufe: 1..8 oder 'all'.

.PARAMETER ExeDir
    Verzeichnis der Test-EXEs. Default: <repo>\bin.

.PARAMETER LogDir
    Zielverzeichnis fuer Logs. Default: <ExeDir>\logs.

.EXAMPLE
    run_alpha_tests.cmd -Prio 7
    run_alpha_tests.cmd -Prio all
    run_alpha_tests.cmd -Prio 8 -LogDir H:\temp\logs

.NOTES
    Version: 3.1 (Etappe 22 + Refactoring)
    Refactoring 22: verschoben von bin\ nach tools\. ExeDir-Default
    zeigt jetzt auf <repo>\bin statt auf den Skript-Ablageort.
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory=$false, Position=0)]
    [ValidateSet('1','2','3','4','5','6','7','8','all')]
    [string]$Prio,

    [string]$ExeDir,
    [string]$LogDir
)

$ErrorActionPreference = 'Stop'

# ==========================================================================
# UTF-8 Konsole
# ==========================================================================
try {
    [Console]::OutputEncoding = [System.Text.Encoding]::UTF8
    [Console]::InputEncoding  = [System.Text.Encoding]::UTF8
    $OutputEncoding           = [System.Text.Encoding]::UTF8
} catch { }
try { & chcp.com 65001 > $null 2>&1 } catch { }

# ==========================================================================
# Default-Pfade (Refactoring 22: tools\ statt bin\)
#
# Skript liegt in <repo>\tools\.
# ExeDir-Default: <repo>\bin\  (eine Ebene hoch, dann bin\).
# LogDir-Default: <ExeDir>\logs\.
# ==========================================================================
$RepoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if (-not $ExeDir) { $ExeDir = Join-Path $RepoRoot 'bin' }
if (-not $LogDir) { $LogDir = Join-Path $ExeDir 'logs' }

# ==========================================================================
# Interaktiv: Prio erfragen, falls nicht gesetzt.
# ==========================================================================
if (-not $Prio) {
    Write-Host "Prio waehlen (1, 2, 3, 4, 5, 6, 7, 8, all): " -NoNewline
    $Prio = Read-Host
    if ($Prio -notin @('1','2','3','4','5','6','7','8','all')) {
        Write-Host "Ungueltige Auswahl: $Prio" -ForegroundColor Red
        exit 2
    }
}

# ==========================================================================
# Test-Katalog
# ==========================================================================
$TestCatalog = [ordered]@{

    # --- Prio 1: 2D-Basis ---
    'Amp-Smoke' = @{
        Prio = 1; Exe = 'example_alpha_test.exe'
        Args = @('--test-amp'); Timeout = 60
    }
    'Born-Regel' = @{
        Prio = 1; Exe = 'example_alpha_test.exe'
        Args = @('--test-born'); Timeout = 60
    }
    'Unitary-Tick' = @{
        Prio = 1; Exe = 'example_alpha_test.exe'
        Args = @('--test-unitary'); Timeout = 60
    }
    'Context-Perm' = @{
        Prio = 1; Exe = 'example_alpha_test.exe'
        Args = @('--test-context'); Timeout = 60
    }
    'Wilson-Loop' = @{
        Prio = 1; Exe = 'example_alpha_test.exe'
        Args = @('--test-wilson'); Timeout = 60
    }
    'Local-Gauge' = @{
        Prio = 1; Exe = 'example_alpha_test.exe'
        Args = @('--test-gauge'); Timeout = 60
    }
    'Triangle-Corr' = @{
        Prio = 1; Exe = 'example_alpha_test.exe'
        Args = @('--test-triangle'); Timeout = 120
    }
    'CHSH-Native' = @{
        Prio = 1; Exe = 'example_alpha_test.exe'
        Args = @('--test-chsh'); Timeout = 120
    }
    'CHSH-Collapse' = @{
        Prio = 1; Exe = 'example_alpha_test.exe'
        Args = @('--test-chsh-collapse'); Timeout = 120
    }
    'CHSH-Graph' = @{
        Prio = 1; Exe = 'example_alpha_test.exe'
        Args = @('--test-chsh-graph'); Timeout = 120
    }
    'Density-Regression' = @{
        Prio = 1; Exe = 'example_test_density.exe'
        Args = @(); Timeout = 180
    }
    'Tensor-Regression' = @{
        Prio = 1; Exe = 'example_test_tensor.exe'
        Args = @(); Timeout = 180
    }

    # --- Prio 2: Emergenz ---
    'Superdet' = @{
        Prio = 2; Exe = 'example_alpha_test.exe'
        Args = @('--test-superdet'); Timeout = 300
    }
    'Observer-CHSH' = @{
        Prio = 2; Exe = 'example_alpha_test.exe'
        Args = @('--test-observer-chsh'); Timeout = 300
    }
    'CHSH-Diffusion' = @{
        Prio = 2; Exe = 'example_alpha_test.exe'
        Args = @('--test-chsh-diffusion'); Timeout = 600
    }
    'CHSH-Wave' = @{
        Prio = 2; Exe = 'example_alpha_test.exe'
        Args = @('--test-chsh-wave'); Timeout = 600
    }
    'Born-Emergent' = @{
        Prio = 2; Exe = 'example_alpha_test.exe'
        Args = @('--test-born-emergent'); Timeout = 900
    }
    'Born-Local' = @{
        Prio = 2; Exe = 'example_alpha_test.exe'
        Args = @('--test-born-local'); Timeout = 900
    }
    'Born-Equiv' = @{
        Prio = 2; Exe = 'example_alpha_test.exe'
        Args = @('--test-born-equiv'); Timeout = 600
    }
    'QM-Basics' = @{
        Prio = 2; Exe = 'example_alpha_test.exe'
        Args = @('--test-qm-basics'); Timeout = 300
    }
    'QM-Advanced' = @{
        Prio = 2; Exe = 'example_alpha_test.exe'
        Args = @('--test-qm-advanced'); Timeout = 600
    }
    'QM-Emergent' = @{
        Prio = 2; Exe = 'example_alpha_test.exe'
        Args = @('--test-qm-emergent'); Timeout = 600
    }

    # --- Prio 3: Langlauf ---
    'Amp-Invariant' = @{
        Prio = 3; Exe = 'example_alpha_test.exe'
        Args = @('--test-amp-invariant'); Timeout = 300
    }
    'Amp-Inv-Colored' = @{
        Prio = 3; Exe = 'example_alpha_test.exe'
        Args = @('--test-amp-invariant-colored'); Timeout = 300
    }
    'Amp-Inv-Bisect' = @{
        Prio = 3; Exe = 'example_alpha_test.exe'
        Args = @('--test-amp-invariant-bisect'); Timeout = 600
    }
    'Edge-Transport' = @{
        Prio = 3; Exe = 'example_alpha_test.exe'
        Args = @('--test-edge-transport'); Timeout = 300
    }
    'Edge-Trans-Colored' = @{
        Prio = 3; Exe = 'example_alpha_test.exe'
        Args = @('--test-edge-transport-colored'); Timeout = 300
    }
    'Edge-Trans-Scaling' = @{
        Prio = 3; Exe = 'example_alpha_test.exe'
        Args = @('--test-edge-transport-scaling'); Timeout = 600
    }
    'Wave-Packet' = @{
        Prio = 3; Exe = 'example_alpha_test.exe'
        Args = @('--test-wave-packet'); Timeout = 300
    }
    'Soliton' = @{
        Prio = 3; Exe = 'example_alpha_test.exe'
        Args = @('--test-soliton'); Timeout = 1200
    }
    'Lorentz' = @{
        Prio = 3; Exe = 'example_alpha_test.exe'
        Args = @('--test-lorentz'); Timeout = 600
    }
    'No-Signaling' = @{
        Prio = 3; Exe = 'example_alpha_test.exe'
        Args = @('--test-no-signaling'); Timeout = 900
    }

    # --- Prio 4: 3D-Torus ---
    '3D-Smoke' = @{
        Prio = 4; Exe = 'example_alpha_test.exe'
        Args = @('--test-3d-smoke'); Timeout = 300
    }
    '3D-Invariance' = @{
        Prio = 4; Exe = 'example_alpha_test.exe'
        Args = @('--test-3d-invariance'); Timeout = 900
    }
    '3D-Dispersion' = @{
        Prio = 4; Exe = 'example_alpha_test.exe'
        Args = @('--test-3d-dispersion'); Timeout = 900
    }

    # --- Prio 5: Hydrogen + Shared-Ref + Tournament ---
    'Hydrogen' = @{
        Prio = 5; Exe = 'example_alpha_test.exe'
        Args = @('--test-hydrogen'); Timeout = 1800
    }
    'Hydrogen-48' = @{
        Prio = 5; Exe = 'example_alpha_test.exe'
        Args = @('--test-hydrogen-48'); Timeout = 7200
    }
    'Shared-Reference' = @{
        Prio = 5; Exe = 'example_alpha_test.exe'
        Args = @('--test-shared-reference'); Timeout = 600
    }
    'Shared-Formula-Tourn' = @{
        Prio = 5; Exe = 'example_alpha_test.exe'
        Args = @('--test-shared-formula-tournament'); Timeout = 300
    }

    # --- Prio 6: Spin-1/2 ---
    'Spin-Half' = @{
        Prio = 6; Exe = 'example_alpha_test.exe'
        Args = @('--test-spin-half'); Timeout = 120
    }

    # --- Prio 7: Dirac ---
    'Dirac' = @{
        Prio = 7; Exe = 'example_alpha_test.exe'
        Args = @('--test-dirac'); Timeout = 300
    }

    # --- Prio 8: SU(2)-Eichfeld (Etappe 22) ---
    'SU2-Wilson-Loop' = @{
        Prio = 8; Exe = 'example_alpha_test.exe'
        Args = @('--test-su2-wilson-loop'); Timeout = 300
    }
    'Running-Coupling' = @{
        Prio = 8; Exe = 'example_alpha_test.exe'
        Args = @('--test-running-coupling'); Timeout = 1800
    }
}

# ==========================================================================
# Test-Filter
# ==========================================================================
$selected = if ($Prio -eq 'all') {
    $TestCatalog.Keys
} else {
    $prioInt = [int]$Prio
    $TestCatalog.Keys | Where-Object { $TestCatalog[$_].Prio -eq $prioInt }
}

if (-not $selected -or $selected.Count -eq 0) {
    Write-Host "Keine Tests fuer Prio '$Prio' gefunden." -ForegroundColor Yellow
    exit 2
}

# ==========================================================================
# Pre-Flight
# ==========================================================================
if (-not (Test-Path -LiteralPath $ExeDir)) {
    Write-Host "FEHLER: ExeDir nicht gefunden: $ExeDir" -ForegroundColor Red
    Write-Host "        (Erwartet: <repo>\\bin mit den Test-EXEs.)" -ForegroundColor Red
    exit 2
}
if (-not (Test-Path -LiteralPath $LogDir)) {
    New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
}

$stamp = Get-Date -Format 'yyyyMMdd_HHmmss'

Write-Host ''
Write-Host '============================================================' -ForegroundColor Cyan
Write-Host "  ProPhysics Alpha-Test-Runner -- Prio $Prio" -ForegroundColor Cyan
Write-Host '============================================================' -ForegroundColor Cyan
Write-Host "  ExeDir: $ExeDir"
Write-Host "  LogDir: $LogDir"
Write-Host "  Tests:  $($selected.Count)"
Write-Host ''

# ==========================================================================
# Auswertungsfunktion
# ==========================================================================
# ==========================================================================
# Auswertungsfunktion: PASS/FAIL-Marker aus dem Log-Text ableiten.
#
# Hinweis: PowerShell 5.0 unterstuetzt keinen Ternary-Operator (? :).
# Wir nutzen explizite if/else-Rueckgaben.
# ==========================================================================
function Get-TestResult {
    param([string]$Output, [int]$ExitCode)

    # 1) Sammeltests: "Ergebnis: N PASS, M FAIL"
    if ($Output -match 'Ergebnis:\s*(\d+)\s*PASS,\s*(\d+)\s*FAIL') {
        $failCount = [int]$Matches[2]
        if ($failCount -eq 0) { return 'PASS' } else { return 'FAIL' }
    }

    # 2) No-Signaling-Sonderfall.
    if ($Output -match 'Ergebnis:\s*HELD')   { return 'PASS' }
    if ($Output -match 'Ergebnis:\s*BROKEN') { return 'FAIL' }

    # 3) Einzel-Check-Marker.
    $hasFail = $Output -match '\[FAIL\]'
    $hasPass = $Output -match '\[PASS\]'
    if ($hasFail) { return 'FAIL' }
    if ($hasPass) { return 'PASS' }

    # 4) Fuehrende Ergebniszeile.
    if ($Output -match 'Ergebnis:\s*PASSED') { return 'PASS' }
    if ($Output -match 'Ergebnis:\s*FAILED') { return 'FAIL' }

    # 5) Direkt-Marker "-> PASSED" / "-> FAILED".
    if ($Output -match '->\s*PASSED') { return 'PASS' }
    if ($Output -match '->\s*FAILED') { return 'FAIL' }

    # 6) Sondermarker ohne PASS/FAIL.
    if ($Output -match 'Klassische Schranke respektiert') { return 'PASS' }
    if ($Output -match 'KEIN Bell-Bruch')                 { return 'PASS' }
    if ($Output -match 'Signalverlust')                   { return 'PASS' }
    if ($Output -match '\[Bisect\]\s*Fertig')             { return 'PASS' }
    if ($Output -match '\[Amp-Test\]\s*OK')               { return 'PASS' }
    if ($Output -match '#\s*Gesamt\s*:\s*PASS')           { return 'PASS' }
    if ($Output -match '#\s*Gesamt\s*:\s*FAIL')           { return 'FAIL' }

    # 7) Superdet-Sonderfall (getrennte/geteilte Quelle).
    $mSep = [regex]::Match($Output, 'getrennte Quelle:\s*S\s*=\s*([0-9.+-]+)')
    $mShr = [regex]::Match($Output, 'geteilte Quelle:\s*S\s*=\s*([0-9.+-]+)')
    if ($mSep.Success -and $mShr.Success) {
        $sep = [double]$mSep.Groups[1].Value
        $shr = [double]$mShr.Groups[1].Value
        $okSep = [math]::Abs($sep - 2.0) -lt 0.20
        $okShr = [math]::Abs($shr - 4.0) -lt 0.20
        if ($okSep -and $okShr) { return 'PASS' } else { return 'FAIL' }
    }

    # 8) Fallback.
    if ($ExitCode -ne 0) { return 'FAIL' }
    return 'UNKNOWN'
}

# ==========================================================================
# Hauptschleife
# ==========================================================================
$results = @()
$counter = 0
$total = $selected.Count
$totalStart = Get-Date

foreach ($name in $selected) {
    $counter++
    $entry = $TestCatalog[$name]
    $exePath = Join-Path $ExeDir $entry.Exe
    $logName = '{0}_{1}.log' -f $stamp, ($name -replace '[^\w\-]', '_')
    $logPath = Join-Path $LogDir $logName

    if (-not (Test-Path -LiteralPath $exePath)) {
        Write-Host ('[{0,3}/{1}] {2,-22} ... MISSING' -f $counter, $total, $name) -ForegroundColor Magenta
        $results += [pscustomobject]@{
            Name = $name; Result = 'MISSING'; Seconds = 0.0; Log = $logName
        }
        continue
    }

    Write-Host ('[{0,3}/{1}] {2,-22} ... ' -f $counter, $total, $name) -NoNewline

    $startTime = Get-Date
    $output = ''
    $exitCode = 0
    $timedOut = $false

        try {
        # PowerShell 5.1: -ArgumentList @() ist buggy und bricht den
        # Start-Process-Aufruf stillschweigend ab (kein Log, kein
        # Exit-Code). Bei leerer Liste den Parameter ganz weglassen.
        $hasArgs = ($entry.Args -and $entry.Args.Count -gt 0)

        if ($hasArgs) {
            $proc = Start-Process -FilePath $exePath -ArgumentList $entry.Args `
                                  -WorkingDirectory $ExeDir -NoNewWindow -PassThru `
                                  -RedirectStandardOutput $logPath `
                                  -RedirectStandardError "$logPath.stderr"
        } else {
            $proc = Start-Process -FilePath $exePath `
                                  -WorkingDirectory $ExeDir -NoNewWindow -PassThru `
                                  -RedirectStandardOutput $logPath `
                                  -RedirectStandardError "$logPath.stderr"
        }

        $finished = $proc.WaitForExit($entry.Timeout * 1000)
        if (-not $finished) {
            try { $proc.Kill() } catch { }
            $timedOut = $true
        } else {
            try { $proc.WaitForExit() } catch { }
            $exitCode = $proc.ExitCode
        }
    } catch {
        # Exception sichtbar machen: in die Log-Datei schreiben, damit
        # der Runner nicht mit stillem 0,0-s-FAIL endet.
        $output = "EXCEPTION beim Start: $_"
        if (-not (Test-Path -LiteralPath $logPath)) {
            "EXCEPTION beim Start von $exePath`n$_" |
                Out-File -LiteralPath $logPath -Encoding utf8
        }
        $exitCode = -1
    }

    if (Test-Path -LiteralPath $logPath) {
        $output = Get-Content -LiteralPath $logPath -Raw -ErrorAction SilentlyContinue
    }
    $stderrPath = "$logPath.stderr"
    if (Test-Path -LiteralPath $stderrPath) {
        $errText = Get-Content -LiteralPath $stderrPath -Raw -ErrorAction SilentlyContinue
        if ($errText) {
            $output += "`n--- STDERR ---`n$errText"
        }
        Remove-Item -LiteralPath $stderrPath -Force -ErrorAction SilentlyContinue
    }
    if ($timedOut) {
        $output += "`n--- TIMEOUT nach $($entry.Timeout)s ---`n"
        Add-Content -LiteralPath $logPath -Value "`n--- TIMEOUT nach $($entry.Timeout)s ---" -ErrorAction SilentlyContinue
    }

    $elapsed = ((Get-Date) - $startTime).TotalSeconds

    if ($timedOut) {
        $result = 'TIMEOUT'
    } else {
        $result = Get-TestResult -Output $output -ExitCode $exitCode
    }

    $color = switch ($result) {
        'PASS'    { 'Green' }
        'FAIL'    { 'Red' }
        'TIMEOUT' { 'Yellow' }
        'MISSING' { 'Magenta' }
        default   { 'Gray' }
    }
    Write-Host ('{0,-8} {1,7:N1}s' -f $result, $elapsed) -ForegroundColor $color

    $results += [pscustomobject]@{
        Name = $name; Result = $result; Seconds = $elapsed; Log = $logName
    }
}

$totalElapsed = ((Get-Date) - $totalStart).TotalSeconds

# ==========================================================================
# Zusammenfassung
# ==========================================================================
Write-Host ''
Write-Host '============================================================' -ForegroundColor Cyan
Write-Host '  Zusammenfassung' -ForegroundColor Cyan
Write-Host '============================================================' -ForegroundColor Cyan
Write-Host ''

Write-Host ('{0,-24} {1,-10} {2,7} {3}' -f 'Test', 'Ergebnis', 'Sek.', 'Log')
Write-Host ('{0,-24} {1,-10} {2,7} {3}' -f '----', '--------', '----', '---')
foreach ($r in $results) {
    $color = switch ($r.Result) {
        'PASS'    { 'Green' }
        'FAIL'    { 'Red' }
        'TIMEOUT' { 'Yellow' }
        'MISSING' { 'Magenta' }
        default   { 'Gray' }
    }
    Write-Host ('{0,-24} {1,-10} {2,7:N1} {3}' -f $r.Name, $r.Result, $r.Seconds, $r.Log) -ForegroundColor $color
}

$nPass    = @($results | Where-Object { $_.Result -eq 'PASS' }).Count
$nFail    = @($results | Where-Object { $_.Result -eq 'FAIL' }).Count
$nTimeout = @($results | Where-Object { $_.Result -eq 'TIMEOUT' }).Count
$nMissing = @($results | Where-Object { $_.Result -eq 'MISSING' }).Count
$nUnknown = @($results | Where-Object { $_.Result -eq 'UNKNOWN' }).Count

Write-Host ''
Write-Host ("PASS: {0}   FAIL: {1}   TIMEOUT: {2}   MISSING: {3}   UNKNOWN: {4}   | Gesamt: {5}" `
    -f $nPass, $nFail, $nTimeout, $nMissing, $nUnknown, $results.Count)
Write-Host ("Dauer gesamt: {0:N1}s" -f $totalElapsed)
Write-Host ''

$failing = @($results | Where-Object { $_.Result -notin @('PASS') })
if ($failing.Count -gt 0) {
    Write-Host 'Nicht-bestandene Tests:' -ForegroundColor Red
    foreach ($r in $failing) {
        $logPath = Join-Path $LogDir $r.Log
        Write-Host ('  - {0,-22} {1,-8} {2}' -f $r.Name, $r.Result, $logPath) -ForegroundColor Red
    }
    exit 1
}

Write-Host 'Alle Tests erfolgreich.' -ForegroundColor Green
exit 0