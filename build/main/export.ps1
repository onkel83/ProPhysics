#Requires -Version 5.0
<#
.SYNOPSIS
    ProPhysics Export -- Build + Kopieren + ZIP.

.DESCRIPTION
    Exportiert ProPhysics-Artefakte in drei Pakettypen und packt jedes
    Paket als ZIP-Archiv.

    Export-Typen:
      exe  -- Test-EXEs + DLLs + Runner + Test-Docs
      sdk  -- DLLs + LIBs + Header + Build-Docs + Test-Katalog
      kit  -- SDK + SDK-Interface + Beispiel-Quellen + Doku-Auszug
      all  -- alle drei Pakete nacheinander

    Jedes Paket liegt als Ordner UND als ZIP vor:
      <OutDir>\<kind>\                       (entpackter Inhalt)
      <OutDir>\prophysics-<kind>-<version>.zip   (Archiv)

    Der -Scope bestimmt, welche Komponenten einbezogen werden:
      kernel / prophysics  -- nur Kernel
      sdk                  -- Kernel + SDK-Interface
      test                 -- Kernel + SDK + Tests
      all                  -- identisch mit test (Default)

.PARAMETER Export
    exe | sdk | kit | all   (Mandatory, Position 0)

.PARAMETER Scope
    kernel | prophysics | sdk | test | all   (Default: all)

.PARAMETER OutDir
    Wurzelverzeichnis. Default: <repo>\out
    Alias: -OutRoot (Legacy-Kompatibilitaet).

.PARAMETER Version
    Versions-Tag fuer ZIP-Namen. Default: aus ProPhysics_Version.h
    (MAJOR.MINOR.PATCH, ohne v-Praefix).

.PARAMETER Name
    Nur fuer -Export src (Legacy): Unterordner-Name.

.PARAMETER NoBuild
    nmake-Schritt ueberspringen.

.PARAMETER Rebuild
    nmake clean + Build vor dem Export.

.PARAMETER Clean
    Zielordner vor dem Export rekursiv leeren.

.PARAMETER DryRun
    Nur auflisten, nichts kopieren/packen.

.EXAMPLE
    .\export.ps1 exe
    .\export.ps1 sdk -Scope sdk
    .\export.ps1 kit -Scope all
    .\export.ps1 all -Rebuild -Clean -Version 1.23.0

.EXAMPLE
    .\export.ps1 sdk -NoBuild -DryRun

.NOTES
    Version: 3.2 (Etappe 23)
    -Export statt -Mode, kit neu, ZIP-Erzeugung, -Version Parameter.
    ZIP-Schema: prophysics-<kind>-<version>.zip (ohne v-Praefix).
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory=$true, Position=0)]
    [ValidateSet('exe','sdk','kit','all')]
    [string]$Export,

    [ValidateSet('kernel','prophysics','sdk','test','all')]
    [string]$Scope = 'all',

    [Alias('OutRoot')]
    [string]$OutDir,

    [string]$Version,
    [string]$Name,

    [switch]$NoBuild,
    [switch]$Rebuild,
    [switch]$Clean,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
$script:DryRun = $DryRun.IsPresent

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
# Pfad-Konfiguration (relativ zu $PSScriptRoot = <repo>\build\main)
# ==========================================================================
$RepoRoot   = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$BuildMain  = $PSScriptRoot
$BuildPP    = Join-Path $RepoRoot 'build\prophysics'
$BuildSdk   = Join-Path $RepoRoot 'build\sdk'
$BuildTst   = Join-Path $RepoRoot 'build\test'

$BinDir     = Join-Path $RepoRoot 'bin'
$LibDir     = Join-Path $RepoRoot 'lib'
$ToolsDir   = Join-Path $RepoRoot 'tools'
$SrcPP      = Join-Path $RepoRoot 'src\prophysics'
$SrcSdk     = Join-Path $RepoRoot 'src\sdk'
$SrcTst     = Join-Path $RepoRoot 'src\test'
$DocsRoot   = Join-Path $RepoRoot 'docs'
$VhPath     = Join-Path $RepoRoot 'src\prophysics\header\ProPhysics_Version.h'
$InfoFile   = Join-Path $RepoRoot 'BUILD_INFO.txt'

if (-not $OutDir) { $OutDir = Join-Path $RepoRoot 'out' }

# ==========================================================================
# Helper
# ==========================================================================
function Write-Step  ($m) { Write-Host "[*] $m"      -ForegroundColor Cyan }
function Write-Ok    ($m) { Write-Host "    OK  $m"  -ForegroundColor Green }
function Write-Warn2 ($m) { Write-Host "    --  $m"  -ForegroundColor Yellow }
function Write-Err   ($m) { Write-Host "    !!  $m"  -ForegroundColor Red }
function Write-Dry   ($m) { Write-Host "    [dry] $m" -ForegroundColor DarkGray }

function Ensure-Dir ($p) {
    if (Test-Path -LiteralPath $p) { return }
    if ($script:DryRun) { Write-Dry "mkdir $p" ; return }
    New-Item -ItemType Directory -Path $p -Force | Out-Null
}

function Copy-Item-Safe {
    param(
        [Parameter(Mandatory=$true)][string]$Source,
        [Parameter(Mandatory=$true)][string]$DestDir,
        [string]$Filter = '*',
        [switch]$Required
    )
    if (-not (Test-Path -LiteralPath $Source)) {
        if ($Required) { Write-Err "Quelle fehlt: $Source" }
        else           { Write-Warn2 "skip (nicht vorhanden): $Source" }
        return 0
    }
    $items = @(Get-ChildItem -LiteralPath $Source -Filter $Filter -File -ErrorAction SilentlyContinue)
    if ($items.Count -eq 0) {
        if ($Required) { Write-Err "Keine Treffer fuer $Filter in $Source" }
        return 0
    }
    foreach ($f in $items) {
        if ($script:DryRun) { Write-Dry $f.Name }
        else                { Copy-Item -LiteralPath $f.FullName -Destination $DestDir -Force }
    }
    return $items.Count
}

function Copy-One-Safe {
    param([string]$Path, [string]$DestDir, [switch]$Required)
    if (-not (Test-Path -LiteralPath $Path)) {
        if ($Required) { Write-Err "Fehlt: $Path" }
        else           { Write-Warn2 "skip: $Path" }
        return 0
    }
    if ($script:DryRun) { Write-Dry (Split-Path $Path -Leaf); return 1 }
    Copy-Item -LiteralPath $Path -Destination $DestDir -Force
    return 1
}

function Copy-Tree {
    param([string]$Source, [string]$DestDir)
    if (-not (Test-Path -LiteralPath $Source)) {
        Write-Warn2 "skip (nicht vorhanden): $Source"
        return 0
    }
    $files = @(Get-ChildItem -LiteralPath $Source -Recurse -File -ErrorAction SilentlyContinue)
    foreach ($f in $files) {
        $rel = $f.FullName.Substring($Source.Length).TrimStart('\')
        $target = Join-Path $DestDir $rel
        $targetDir = Split-Path $target -Parent
        if ($script:DryRun) {
            Write-Dry "$rel"
            continue
        }
        if (-not (Test-Path -LiteralPath $targetDir)) {
            New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
        }
        Copy-Item -LiteralPath $f.FullName -Destination $target -Force
    }
    return $files.Count
}

# ==========================================================================
# Version aus ProPhysics_Version.h lesen (ohne v-Praefix).
# ==========================================================================
function Get-ProVersion {
    if (-not (Test-Path -LiteralPath $VhPath)) { return '0.0.0' }
    $txt = Get-Content -LiteralPath $VhPath -Raw
    $maj = if ($txt -match 'VERSION_MAJOR\s+(\d+)') { $Matches[1] } else { '0' }
    $min = if ($txt -match 'VERSION_MINOR\s+(\d+)') { $Matches[1] } else { '0' }
    $pat = if ($txt -match 'VERSION_PATCH\s+(\d+)') { $Matches[1] } else { '0' }
    return "$maj.$min.$pat"
}

# ==========================================================================
# BUILD_INFO.txt Kopie ins Paket
# ==========================================================================
function Copy-BuildInfo {
    param([string]$DestDir)
    if (-not (Test-Path -LiteralPath $InfoFile)) {
        Write-Warn2 "BUILD_INFO.txt fehlt -- Build zuerst ausfuehren"
        return 0
    }
    if ($script:DryRun) { Write-Dry "BUILD_INFO.txt" ; return 1 }
    Copy-Item -LiteralPath $InfoFile -Destination $DestDir -Force
    return 1
}

# ==========================================================================
# Docs kopieren -- export-abhaengig
# ==========================================================================
function Copy-Docs {
    param(
        [Parameter(Mandatory=$true)][ValidateSet('exe','sdk','kit','src')][string]$ExportMode,
        [Parameter(Mandatory=$true)][string]$DestDir
    )

    if (-not (Test-Path -LiteralPath $DocsRoot)) {
        Write-Warn2 "docs\ nicht gefunden -- kein Dokumenten-Export"
        return 0
    }

    $docsDest = Join-Path $DestDir 'docs'
    Ensure-Dir $docsDest

    # --- src/kit: volle bzw. erweiterte Liste ---
    if ($ExportMode -eq 'src') {
        $count = Copy-Tree -Source $DocsRoot -DestDir $docsDest
        Write-Ok ("Docs ({0} Dateien)" -f $count)
        return $count
    }

    # --- exe / sdk / kit: kuratierte Liste ---
    $map = @()
    if ($ExportMode -eq 'exe') {
        $map = @(
            @{ src = 'project\Project.md';                  dst = 'Project.md' },
            @{ src = 'test\ProPhysics_Testkatalog.md';      dst = 'ProPhysics_Testkatalog.md' },
            @{ src = 'test\run_alpha_tests.md';             dst = 'run_alpha_tests.md' }
        )
    }
    elseif ($ExportMode -eq 'sdk') {
        $map = @(
            @{ src = 'project\Project.md';                  dst = 'Project.md' },
            @{ src = 'test\ProPhysics_Testkatalog.md';      dst = 'ProPhysics_Testkatalog.md' },
            @{ src = 'build\BUILD_SCRIPT.md';               dst = 'BUILD_SCRIPT.md' },
            @{ src = 'build\prophysics\Makefile.md';        dst = 'prophysics-Makefile.md' },
            @{ src = 'build\sdk\Makefile.md';               dst = 'sdk-Makefile.md' }
        )
    }
    elseif ($ExportMode -eq 'kit') {
        $map = @(
            @{ src = 'project\Project.md';                  dst = 'Project.md' },
            @{ src = 'project\CONFIG.md';                   dst = 'CONFIG.md' },
            @{ src = 'test\ProPhysics_Testkatalog.md';      dst = 'ProPhysics_Testkatalog.md' },
            @{ src = 'test\run_alpha_tests.md';             dst = 'run_alpha_tests.md' },
            @{ src = 'build\BUILD_SCRIPT.md';               dst = 'BUILD_SCRIPT.md' },
            @{ src = 'build\prophysics\Makefile.md';        dst = 'prophysics-Makefile.md' },
            @{ src = 'build\sdk\Makefile.md';               dst = 'sdk-Makefile.md' }
        )
    }

    $count = 0
    foreach ($entry in $map) {
        $srcPath = Join-Path $DocsRoot $entry.src
        $dstPath = Join-Path $docsDest $entry.dst
        if (-not (Test-Path -LiteralPath $srcPath)) {
            Write-Warn2 ("skip: {0}" -f $entry.src)
            continue
        }
        if ($script:DryRun) { Write-Dry $entry.dst ; $count++ ; continue }
        Copy-Item -LiteralPath $srcPath -Destination $dstPath -Force
        $count++
    }
    Write-Ok ("Docs ({0} Dateien)" -f $count)
    return $count
}

# ==========================================================================
# README.md generieren -- export-abhaengig
# ==========================================================================
function New-PackageReadme {
    param(
        [Parameter(Mandatory=$true)][ValidateSet('exe','sdk','kit','src')][string]$ExportMode,
        [Parameter(Mandatory=$true)][string]$Scope,
        [Parameter(Mandatory=$true)][string]$DestDir,
        [string]$PackageName = '',
        [string]$PackageVersion = ''
    )

    $version  = if ($PackageVersion) { $PackageVersion } else { Get-ProVersion }
    $now      = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'
    $hostName = $env:COMPUTERNAME

    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add("# ProPhysics Package -- $($ExportMode.ToUpper())")
    $lines.Add("")
    $lines.Add("**Version:** $version")
    $lines.Add("**Erzeugt:** $now auf $hostName")
    $lines.Add("**Scope:** $Scope")
    if ($PackageName) {
        $lines.Add("**Paket-Name:** $PackageName")
    }
    $lines.Add("")

    switch ($ExportMode) {

        'exe' {
            $lines.Add("## Was ist in diesem Paket?")
            $lines.Add("")
            $lines.Add("Ein lauffaehiges Runtime-Paket der ProPhysics-Alpha-Tests.")
            $lines.Add("Alle DLLs, EXEs und der Test-Runner liegen **flach")
            $lines.Add("nebeneinander**, so dass Windows die DLLs beim Start")
            $lines.Add("der EXEs automatisch findet.")
            $lines.Add("")
            $lines.Add("| Datei | Rolle |")
            $lines.Add("|---|---|")
            $lines.Add("| ``ProPhysics.dll`` | Kernel-Bibliothek |")
            $lines.Add("| ``pro_sdk_interface.dll`` | SDK-Interface |")
            $lines.Add("| ``example_alpha_test.exe`` | Alpha-Test-Suite (43 Tests) |")
            $lines.Add("| ``example_test_density.exe`` | Dichte-Regression (86 Checks) |")
            $lines.Add("| ``example_test_tensor.exe`` | Tensor-/Fock-Regression (61 Checks) |")
            $lines.Add("| ``run_alpha_tests.cmd`` / ``.ps1`` | Test-Runner |")
            $lines.Add("")
            $lines.Add("## Schnellstart")
            $lines.Add("")
            $lines.Add("``````cmd")
            $lines.Add("run_alpha_tests.cmd -Prio all")
            $lines.Add("``````")
            $lines.Add("")
            $lines.Add("Einzelner Test (Beispiel SU(2)-Wilson-Loop):")
            $lines.Add("")
            $lines.Add("``````cmd")
            $lines.Add("example_alpha_test.exe --test-su2-wilson-loop")
            $lines.Add("``````")
            $lines.Add("")
            $lines.Add("## Prioritaeten")
            $lines.Add("")
            $lines.Add("| Prio | Thema | Tests | Typische Dauer |")
            $lines.Add("|:-:|---|---:|---:|")
            $lines.Add("| 1 | 2D-Basis | 12 | ~2 s |")
            $lines.Add("| 2 | Emergenz | 10 | ~4,5 min |")
            $lines.Add("| 3 | Langlauf | 10 | ~45 s |")
            $lines.Add("| 4 | 3D-Torus | 3 | ~45 s |")
            $lines.Add("| 5 | Hydrogen + Shared-Ref | 4 | ~36 min |")
            $lines.Add("| 6 | Spin-1/2 | 1 | < 1 s |")
            $lines.Add("| 7 | Dirac | 1 | ~15 s |")
            $lines.Add("| 8 | SU(2) + Running-Coupling | 2 | ~30 min |")
            $lines.Add("| ``all`` | alle | 43 | ~65 min |")
            $lines.Add("")
            $lines.Add("Fuer schnelle Regressionen: ``-Prio 1-4``.")
            $lines.Add("Prio 5 (Hydrogen-48) braucht ~36 min und laeuft nur im Nightly-CI.")
            $lines.Add("")
            $lines.Add("## Dokumentation")
            $lines.Add("")
            $lines.Add("| Datei | Inhalt |")
            $lines.Add("|---|---|")
            $lines.Add("| ``docs\run_alpha_tests.md`` | Test-Runner-Bedienung, Prios, Logs |")
            $lines.Add("| ``docs\ProPhysics_Testkatalog.md`` | alle 43 Tests mit Kriterien |")
            $lines.Add("| ``docs\Project.md`` | Ontologie und Roadmap |")
            $lines.Add("| ``BUILD_INFO.txt`` | Version, Artefakt-Liste, Zeitstempel |")
        }

        'sdk' {
            $lines.Add("## Was ist in diesem Paket?")
            $lines.Add("")
            $lines.Add("Ein SDK-Paket zur Entwicklung gegen den ProPhysics-Kernel.")
            $lines.Add("Header, Import-Libs und DLLs sind so organisiert, dass")
            $lines.Add("sie direkt in ein externes Projekt eingebunden werden koennen.")
            $lines.Add("")
            $lines.Add("``````")
            $lines.Add("libs\")
            $lines.Add("+-- ProPhysics.dll / .lib")
            $lines.Add("+-- pro_sdk_interface.dll / .lib")
            $lines.Add("+-- src\header\")
            $lines.Add("|   +-- prophysics\   (Kernel-Header)")
            $lines.Add("|   +-- sdk\          (SDK-Interface-Header)")
            $lines.Add("``````")
            $lines.Add("")
            $lines.Add("## Einbindung")
            $lines.Add("")
            $lines.Add("**Include-Pfad:** ``libs\src\header\``")
            $lines.Add("**Library-Pfad:** ``libs\``")
            $lines.Add("")
            $lines.Add("``````c")
            $lines.Add("#include ""prophysics/ProPhysics.h""")
            $lines.Add("#include ""sdk/pro_sdk_interface.h""")
            $lines.Add("``````")
            $lines.Add("")
            $lines.Add("**Link-Reihenfolge:** ``pro_sdk_interface.lib`` **vor**")
            $lines.Add("``ProPhysics.lib``. Reihenfolge umgekehrt gibt")
            $lines.Add("``unresolved external symbol``.")
            $lines.Add("")
            $lines.Add("MSVC-Beispiel:")
            $lines.Add("")
            $lines.Add("``````cmd")
            $lines.Add("cl /I libs\src\header my_app.c ^")
            $lines.Add("   /link libs\pro_sdk_interface.lib libs\ProPhysics.lib")
            $lines.Add("``````")
            $lines.Add("")
            $lines.Add("## DLL-Weitergabe")
            $lines.Add("")
            $lines.Add("Beim Ausliefern einer EXE, die gegen dieses SDK gelinkt wurde,")
            $lines.Add("muessen beide DLLs neben der EXE liegen:")
            $lines.Add("")
            $lines.Add("- ``ProPhysics.dll``")
            $lines.Add("- ``pro_sdk_interface.dll``")
            $lines.Add("")
            $lines.Add("## Dokumentation")
            $lines.Add("")
            $lines.Add("| Datei | Inhalt |")
            $lines.Add("|---|---|")
            $lines.Add("| ``docs\Project.md`` | Projekt-Roadmap und Ontologie |")
            $lines.Add("| ``docs\ProPhysics_Testkatalog.md`` | Kernel-Faehigkeiten im Detail |")
            $lines.Add("| ``docs\BUILD_SCRIPT.md`` | Build-System-Uebersicht |")
            $lines.Add("| ``docs\prophysics-Makefile.md`` | Kernel-Build-Details |")
            $lines.Add("| ``docs\sdk-Makefile.md`` | SDK-Build-Details |")
            $lines.Add("| ``BUILD_INFO.txt`` | Version, Artefakt-Liste, Zeitstempel |")
        }

        'kit' {
            $lines.Add("## Was ist in diesem Paket?")
            $lines.Add("")
            $lines.Add("Ein **komplettes Entwicklungspaket** -- SDK plus Beispiel-")
            $lines.Add("Quellen plus erweiterter Doku-Auszug. Gedacht fuer")
            $lines.Add("Entwickler, die den Kernel direkt in eigenen Anwendungen")
            $lines.Add("nutzen und die vorhandenen Beispielprogramme als")
            $lines.Add("Startpunkt verwenden wollen.")
            $lines.Add("")
            $lines.Add("``````")
            $lines.Add("libs\")
            $lines.Add("+-- ProPhysics.dll / .lib")
            $lines.Add("+-- pro_sdk_interface.dll / .lib")
            $lines.Add("+-- src\header\")
            $lines.Add("|   +-- prophysics\   (Kernel-Header)")
            $lines.Add("|   +-- sdk\          (SDK-Interface-Header)")
            $lines.Add("examples\")
            $lines.Add("+-- example_test_density.c   (Dichte-API, Lindblad)")
            $lines.Add("+-- example_test_tensor.c    (Tensor-API, Fermionen, Fock)")
            $lines.Add("docs\")
            $lines.Add("+-- (erweiterter Doku-Auszug)")
            $lines.Add("``````")
            $lines.Add("")
            $lines.Add("## Einbindung")
            $lines.Add("")
            $lines.Add("Identisch zum SDK-Paket. Zusaetzlich koennen die")
            $lines.Add("Beispiele unter ``examples\`` direkt uebersetzt werden:")
            $lines.Add("")
            $lines.Add("``````cmd")
            $lines.Add("cl /I libs\src\header examples\example_test_density.c ^")
            $lines.Add("   /link libs\pro_sdk_interface.lib libs\ProPhysics.lib")
            $lines.Add("``````")
            $lines.Add("")
            $lines.Add("## Dokumentation")
            $lines.Add("")
            $lines.Add("| Datei | Inhalt |")
            $lines.Add("|---|---|")
            $lines.Add("| ``docs\Project.md`` | Projekt-Roadmap und Ontologie |")
            $lines.Add("| ``docs\CONFIG.md`` | Alle Config-Konstanten |")
            $lines.Add("| ``docs\ProPhysics_Testkatalog.md`` | Kernel-Faehigkeiten im Detail |")
            $lines.Add("| ``docs\run_alpha_tests.md`` | Test-Runner-Bedienung |")
            $lines.Add("| ``docs\BUILD_SCRIPT.md`` | Build-System-Uebersicht |")
            $lines.Add("| ``docs\prophysics-Makefile.md`` | Kernel-Build-Details |")
            $lines.Add("| ``docs\sdk-Makefile.md`` | SDK-Build-Details |")
            $lines.Add("| ``BUILD_INFO.txt`` | Version, Artefakt-Liste, Zeitstempel |")
        }

        'src' {
            $lines.Add("## Was ist in diesem Paket?")
            $lines.Add("")
            $lines.Add("Ein vollstaendiger Quellcode-Snapshot des ProPhysics-Projekts.")
            $lines.Add("Enthaelt alle Kernel-, SDK- und Test-Quellen, den")
            $lines.Add("Test-Runner unter ``tools\`` sowie die komplette Doku.")
            $lines.Add("")
            $lines.Add("``````")
            if ($PackageName) {
                $lines.Add("$PackageName\")
            } else {
                $lines.Add("<name>\")
            }
            $lines.Add("+-- prophysics\      (Kernel-Quellen, 12 Module)")
            $lines.Add("+-- sdk\             (SDK-Interface)")
            $lines.Add("+-- test\            (Alpha-Test + 2 Example-Tests)")
            $lines.Add("+-- tools\           (Test-Runner PS1 + CMD)")
            $lines.Add("+-- docs\            (vollstaendige Doku)")
            $lines.Add("``````")
        }
    }

    $lines.Add("")
    $lines.Add("---")
    $lines.Add("")
    $lines.Add("ProPhysics Kernel -- topologischer Graph-basierter QM-Simulator")
    $lines.Add("(C99, Q31, signed-permutation-Mechanik).")

    $readmePath = Join-Path $DestDir 'README.md'
    if ($script:DryRun) { Write-Dry "README.md" ; return }
    ($lines -join [Environment]::NewLine) |
        Out-File -LiteralPath $readmePath -Encoding utf8
    Write-Ok "README.md"
}

# ==========================================================================
# nmake-Target aus Scope ableiten
# ==========================================================================
function Get-BuildTarget {
    switch ($Scope) {
        'kernel'     { return 'prophysics' }
        'prophysics' { return 'prophysics' }
        'sdk'        { return 'sdk' }
        'test'       { return 'test' }
        'all'        { return 'all' }
    }
}

# ==========================================================================
# Build ausfuehren
# ==========================================================================
function Invoke-Build {
    if ($NoBuild) { Write-Step 'Build uebersprungen (-NoBuild)'; return }

    $nmake = Get-Command nmake -ErrorAction SilentlyContinue
    if (-not $nmake) {
        Write-Err 'nmake nicht im PATH. Bitte VS-Developer-Prompt oeffnen.'
        exit 2
    }

    $target = Get-BuildTarget
    if ($Rebuild) {
        switch ($Scope) {
            'kernel'     { $target = 'rebuild_prophysics' }
            'prophysics' { $target = 'rebuild_prophysics' }
            'sdk'        { $target = 'rebuild_sdk' }
            'test'       { $target = 'rebuild_test' }
            'all'        { $target = 'rebuild' }
        }
    }

    if ($script:DryRun) {
        Write-Dry "cd `"$BuildMain`" && nmake /NOLOGO /f Makefile.nmake $target"
        return
    }

    Write-Step "Baue ($target)..."
    Push-Location $BuildMain
    try {
        & nmake /NOLOGO /f Makefile.nmake $target
        if ($LASTEXITCODE -ne 0) {
            Write-Err "nmake fehlgeschlagen (Exit $LASTEXITCODE)"
            exit $LASTEXITCODE
        }
    } finally { Pop-Location }
    Write-Ok "Build abgeschlossen."
}

# ==========================================================================
# ZIP erzeugen
# ==========================================================================
function New-PackageZip {
    param(
        [Parameter(Mandatory=$true)][string]$SourceDir,
        [Parameter(Mandatory=$true)][string]$ZipPath
    )

    if ($script:DryRun) {
        Write-Dry "zip: $ZipPath"
        return
    }

    if (-not (Test-Path -LiteralPath $SourceDir)) {
        Write-Warn2 "ZIP-Quelle fehlt: $SourceDir"
        return
    }

    if (Test-Path -LiteralPath $ZipPath) {
        Remove-Item -LiteralPath $ZipPath -Force
    }

    Write-Step "ZIP: $(Split-Path $ZipPath -Leaf)"
    try {
        Compress-Archive -Path (Join-Path $SourceDir '*') `
                         -DestinationPath $ZipPath `
                         -CompressionLevel Optimal `
                         -Force
        $size = (Get-Item -LiteralPath $ZipPath).Length / 1KB
        Write-Ok ("ZIP erzeugt ({0:N1} KB)" -f $size)
    } catch {
        Write-Err "Compress-Archive fehlgeschlagen: $_"
    }
}

# ==========================================================================
# Export: exe
# ==========================================================================
function Export-Exe {
    param([string]$OutRoot, [string]$VersionTag)

    $outDir = Join-Path $OutRoot 'exe'
    Write-Step "Export exe -> $outDir"
    if ($Clean -and (Test-Path -LiteralPath $outDir)) {
        Write-Warn2 'Leere Zielordner (-Clean)'
        if (-not $script:DryRun) { Remove-Item -LiteralPath $outDir -Recurse -Force }
    }
    Ensure-Dir $outDir

    $null = Copy-Item-Safe -Source $BinDir -DestDir $outDir -Filter 'ProPhysics.dll' -Required

    if ($Scope -in @('sdk','test','all')) {
        $null = Copy-Item-Safe -Source $BinDir -DestDir $outDir -Filter 'pro_sdk_interface.dll'
    }

    if ($Scope -in @('test','all')) {
        $null = Copy-Item-Safe -Source $BinDir -DestDir $outDir -Filter 'example_*.exe' -Required

        # Runner aus tools\.
        foreach ($f in @('run_alpha_tests.cmd','run_alpha_tests.ps1')) {
            $null = Copy-One-Safe -Path (Join-Path $ToolsDir $f) -DestDir $outDir
        }
    }

    $null = Copy-BuildInfo -DestDir $outDir
    $null = Copy-Docs -ExportMode 'exe' -DestDir $outDir
    New-PackageReadme -ExportMode 'exe' -Scope $Scope -DestDir $outDir -PackageVersion $VersionTag

    $zip = Join-Path $OutRoot "prophysics-exe-$VersionTag.zip"
    New-PackageZip -SourceDir $outDir -ZipPath $zip

    Write-Ok "Fertig: $outDir"
}

# ==========================================================================
# Export: sdk
# ==========================================================================
function Export-Sdk {
    param([string]$OutRoot, [string]$VersionTag)

    $outDir = Join-Path $OutRoot 'sdk'
    Write-Step "Export sdk -> $outDir"
    if ($Clean -and (Test-Path -LiteralPath $outDir)) {
        Write-Warn2 'Leere Zielordner (-Clean)'
        if (-not $script:DryRun) { Remove-Item -LiteralPath $outDir -Recurse -Force }
    }

    $libsDir  = Join-Path $outDir 'libs'
    $hdrRoot  = Join-Path $libsDir 'src\header'
    $hdrPP    = Join-Path $hdrRoot 'prophysics'
    $hdrSdk   = Join-Path $hdrRoot 'sdk'

    Ensure-Dir $libsDir
    Ensure-Dir $hdrRoot

    $null = Copy-Item-Safe -Source $BinDir -DestDir $libsDir -Filter 'ProPhysics.dll' -Required
    $null = Copy-Item-Safe -Source $LibDir -DestDir $libsDir -Filter 'ProPhysics.lib' -Required

    if ($Scope -in @('sdk','test','all')) {
        $null = Copy-Item-Safe -Source $BinDir -DestDir $libsDir -Filter 'pro_sdk_interface.dll'
        $null = Copy-Item-Safe -Source $LibDir -DestDir $libsDir -Filter 'pro_sdk_interface.lib'
    }

    Ensure-Dir $hdrPP
    $null = Copy-Item-Safe -Source (Join-Path $SrcPP 'header') -DestDir $hdrPP -Filter '*.h'

    if ($Scope -in @('sdk','test','all')) {
        Ensure-Dir $hdrSdk
        $null = Copy-Item-Safe -Source (Join-Path $SrcSdk 'header') -DestDir $hdrSdk -Filter '*.h'
    }

    $null = Copy-BuildInfo -DestDir $outDir
    $null = Copy-Docs -ExportMode 'sdk' -DestDir $outDir
    New-PackageReadme -ExportMode 'sdk' -Scope $Scope -DestDir $outDir -PackageVersion $VersionTag

    $zip = Join-Path $OutRoot "prophysics-sdk-$VersionTag.zip"
    New-PackageZip -SourceDir $outDir -ZipPath $zip

    Write-Ok "Fertig: $outDir"
}

# ==========================================================================
# Export: kit
#
# SDK + SDK-Interface + Beispiel-Quellen + erweiterter Doku-Auszug.
# ==========================================================================
function Export-Kit {
    param([string]$OutRoot, [string]$VersionTag)

    $outDir = Join-Path $OutRoot 'kit'
    Write-Step "Export kit -> $outDir"
    if ($Clean -and (Test-Path -LiteralPath $outDir)) {
        Write-Warn2 'Leere Zielordner (-Clean)'
        if (-not $script:DryRun) { Remove-Item -LiteralPath $outDir -Recurse -Force }
    }

    $libsDir  = Join-Path $outDir 'libs'
    $hdrRoot  = Join-Path $libsDir 'src\header'
    $hdrPP    = Join-Path $hdrRoot 'prophysics'
    $hdrSdk   = Join-Path $hdrRoot 'sdk'
    $exaDir   = Join-Path $outDir 'examples'

    Ensure-Dir $libsDir
    Ensure-Dir $hdrRoot
    Ensure-Dir $exaDir

    # --- SDK-Teil ---
    $null = Copy-Item-Safe -Source $BinDir -DestDir $libsDir -Filter 'ProPhysics.dll' -Required
    $null = Copy-Item-Safe -Source $LibDir -DestDir $libsDir -Filter 'ProPhysics.lib' -Required
    $null = Copy-Item-Safe -Source $BinDir -DestDir $libsDir -Filter 'pro_sdk_interface.dll'
    $null = Copy-Item-Safe -Source $LibDir -DestDir $libsDir -Filter 'pro_sdk_interface.lib'

    Ensure-Dir $hdrPP
    $null = Copy-Item-Safe -Source (Join-Path $SrcPP 'header') -DestDir $hdrPP -Filter '*.h'
    Ensure-Dir $hdrSdk
    $null = Copy-Item-Safe -Source (Join-Path $SrcSdk 'header') -DestDir $hdrSdk -Filter '*.h'

    # --- Beispiele ---
    foreach ($f in @('example_test_density.c','example_test_tensor.c')) {
        $null = Copy-One-Safe -Path (Join-Path $SrcTst $f) -DestDir $exaDir
    }

    # --- Docs + BUILD_INFO + README ---
    $null = Copy-BuildInfo -DestDir $outDir
    $null = Copy-Docs -ExportMode 'kit' -DestDir $outDir
    New-PackageReadme -ExportMode 'kit' -Scope $Scope -DestDir $outDir -PackageVersion $VersionTag

    $zip = Join-Path $OutRoot "prophysics-kit-$VersionTag.zip"
    New-PackageZip -SourceDir $outDir -ZipPath $zip

    Write-Ok "Fertig: $outDir"
}

# ==========================================================================
# Main
# ==========================================================================
$versionTag = if ($Version) { $Version } else { Get-ProVersion }

Write-Host ''
Write-Host '============================================================' -ForegroundColor Cyan
Write-Host "  ProPhysics Export  |  Export: $Export  Scope: $Scope" -ForegroundColor Cyan
Write-Host '============================================================' -ForegroundColor Cyan
Write-Host "  Repo:    $RepoRoot"
Write-Host "  Ausgabe: $OutDir"
Write-Host "  Version: $versionTag"
if ($script:DryRun) {
    Write-Host '  Status:  DRY-RUN (nichts wird kopiert/gepackt)' -ForegroundColor Yellow
}
Write-Host ''

# Build (nicht fuer reine Kopier-Exporte noetig)
Invoke-Build

Ensure-Dir $OutDir

$targets = if ($Export -eq 'all') { @('exe','sdk','kit') } else { @($Export) }

foreach ($t in $targets) {
    switch ($t) {
        'exe' { Export-Exe -OutRoot $OutDir -VersionTag $versionTag }
        'sdk' { Export-Sdk -OutRoot $OutDir -VersionTag $versionTag }
        'kit' { Export-Kit -OutRoot $OutDir -VersionTag $versionTag }
    }
}

# ==========================================================================
# Zusammenfassung
# ==========================================================================
Write-Host ''
Write-Host '------------------------------------------------------------' -ForegroundColor Cyan
Write-Host '  Zusammenfassung' -ForegroundColor Cyan
Write-Host '------------------------------------------------------------' -ForegroundColor Cyan

foreach ($t in $targets) {
    $zip = Join-Path $OutDir "prophysics-$t-$versionTag.zip"
    $dir = Join-Path $OutDir $t
    $haveDir = Test-Path -LiteralPath $dir
    $haveZip = Test-Path -LiteralPath $zip
    $dirCount = if ($haveDir) {
        @(Get-ChildItem -LiteralPath $dir -Recurse -File -ErrorAction SilentlyContinue).Count
    } else { 0 }

    if ($haveZip) {
        $kb = (Get-Item -LiteralPath $zip).Length / 1KB
        Write-Host ("  {0,-4}  {1,5} Datei(en)  ->  {2} ({3:N1} KB)" -f `
            $t, $dirCount, (Split-Path $zip -Leaf), $kb)
    } elseif ($haveDir) {
        Write-Host ("  {0,-4}  {1,5} Datei(en)  ->  (kein ZIP)" -f $t, $dirCount) -ForegroundColor Yellow
    } else {
        Write-Host ("  {0,-4}  (nicht erzeugt)" -f $t) -ForegroundColor Yellow
    }
}

Write-Host ''
exit 0