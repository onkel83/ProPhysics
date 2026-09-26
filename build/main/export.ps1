#Requires -Version 5.0
<#
.SYNOPSIS
    ProPhysics Export -- Build + Kopieren in Zielverzeichnisse + Docs.

.DESCRIPTION
    Exportiert ProPhysics-Artefakte in eine saubere Ordnerstruktur.
    Drei Modi:

      exe   -- EXEs + DLLs + Runner + Test-Docs
      sdk   -- DLLs + LIBs + Header + Build-Docs + Test-Katalog
      src   -- Quellcode + tools + vollstaendige Docs-Mirror

    Jedes Paket enthaelt:
      - README.md         (generiert, beschreibt Paketinhalt)
      - BUILD_INFO.txt    (Kopie aus Projekt-Root)
      - docs\             (modus-abhaengige Auswahl)

    Der -Scope bestimmt, welche Komponenten einbezogen werden:
      prophysics  -- nur Kernel
      sdk         -- Kernel + SDK-Interface
      test        -- Kernel + SDK + Tests
      all         -- identisch mit test (Default)

.PARAMETER Mode
    exe | sdk | src  (Mandatory, Position 0)

.PARAMETER Scope
    prophysics | sdk | test | all  (Default: all)

.PARAMETER OutRoot
    Wurzelverzeichnis. Default: <repo>\out

.PARAMETER Name
    Nur fuer Mode=src: Unterordner-Name. Default: Version aus
    ProPhysics_Version.h (z.B. v3.0.0).

.PARAMETER NoBuild
    nmake-Schritt ueberspringen.

.PARAMETER Rebuild
    nmake clean + Build vor dem Export.

.PARAMETER Clean
    Zielordner vor dem Export rekursiv leeren.

.PARAMETER DryRun
    Nur auflisten, nichts kopieren.

.EXAMPLE
    .\export.ps1 exe
    .\export.ps1 sdk -Scope sdk
    .\export.ps1 src -Scope all -Name v3.1-etappe22
    .\export.ps1 sdk -Rebuild -Clean -Scope all

.NOTES
    Version: 3.1 (Etappe 22 + Refactoring)
    Refactoring 22: Test-Runner sind von bin\ nach tools\ umgezogen.
    - exe-Export kopiert Runner aus tools\.
    - src-Export nimmt tools\ als eigenen Ordner mit.
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory=$true, Position=0)]
    [ValidateSet('exe','sdk','src')]
    [string]$Mode,

    [ValidateSet('prophysics','sdk','test','all')]
    [string]$Scope = 'all',

    [string]$OutRoot,
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
# Pfad-Konfiguration
# ==========================================================================
$RepoRoot   = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$BuildMain  = $PSScriptRoot
$BuildPP    = Join-Path $RepoRoot 'build\prophysics'
$BuildSdk   = Join-Path $RepoRoot 'build\sdk'
$BuildTst   = Join-Path $RepoRoot 'build\test'

$BinDir     = Join-Path $RepoRoot 'bin'
$LibDir     = Join-Path $RepoRoot 'lib'
$ToolsDir   = Join-Path $RepoRoot 'tools'   # Refactoring 22: Runner-Ablage
$SrcPP      = Join-Path $RepoRoot 'src\prophysics'
$SrcSdk     = Join-Path $RepoRoot 'src\sdk'
$SrcTst     = Join-Path $RepoRoot 'src\test'
$DocsRoot   = Join-Path $RepoRoot 'docs'
$VhPath     = Join-Path $RepoRoot 'src\prophysics\header\ProPhysics_Version.h'
$InfoFile   = Join-Path $RepoRoot 'BUILD_INFO.txt'

if (-not $OutRoot) { $OutRoot = Join-Path $RepoRoot 'out' }

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
    # Kopiert einen Quellbaum 1:1 in einen Zielbaum.
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
# Version aus ProPhysics_Version.h lesen
# ==========================================================================
function Get-ProVersion {
    if (-not (Test-Path -LiteralPath $VhPath)) { return 'unknown' }
    $txt = Get-Content -LiteralPath $VhPath -Raw
    $maj = if ($txt -match 'VERSION_MAJOR\s+(\d+)') { $Matches[1] } else { '?' }
    $min = if ($txt -match 'VERSION_MINOR\s+(\d+)') { $Matches[1] } else { '?' }
    $pat = if ($txt -match 'VERSION_PATCH\s+(\d+)') { $Matches[1] } else { '?' }
    return "v$maj.$min.$pat"
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
# Docs kopieren -- modus-abhaengig
# ==========================================================================
function Copy-Docs {
    param(
        [Parameter(Mandatory=$true)][ValidateSet('exe','sdk','src')][string]$ExportMode,
        [Parameter(Mandatory=$true)][string]$DestDir
    )

    if (-not (Test-Path -LiteralPath $DocsRoot)) {
        Write-Warn2 "docs\ nicht gefunden -- kein Dokumenten-Export"
        return 0
    }

    $docsDest = Join-Path $DestDir 'docs'
    Ensure-Dir $docsDest

    # --- src: volle Mirror ---
    if ($ExportMode -eq 'src') {
        $count = Copy-Tree -Source $DocsRoot -DestDir $docsDest
        Write-Ok ("Docs ({0} Dateien)" -f $count)
        return $count
    }

    # --- exe / sdk: kuratierte Liste ---
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
# README.md generieren -- modus-abhaengig
# ==========================================================================
function New-PackageReadme {
    param(
        [Parameter(Mandatory=$true)][ValidateSet('exe','sdk','src')][string]$ExportMode,
        [Parameter(Mandatory=$true)][string]$Scope,
        [Parameter(Mandatory=$true)][string]$DestDir,
        [string]$PackageName = ''
    )

    $version  = Get-ProVersion
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
            $lines.Add("| ``example_alpha_test.exe`` | Alpha-Test-Suite (42 Tests) |")
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
            $lines.Add("| 8 | SU(2)-Eichfeld | 1 | ~30 s |")
            $lines.Add("| ``all`` | alle | 42 | ~43 min |")
            $lines.Add("")
            $lines.Add("Fuer schnelle Regressionen: ``-Prio 1`` bis ``-Prio 4``,")
            $lines.Add("dann ``-Prio 6``, ``-Prio 7`` und ``-Prio 8``. Prio 5")
            $lines.Add("(Hydrogen-48) braucht ~36 min und laeuft nur im Nightly-CI.")
            $lines.Add("")
            $lines.Add("## Dokumentation")
            $lines.Add("")
            $lines.Add("| Datei | Inhalt |")
            $lines.Add("|---|---|")
            $lines.Add("| ``docs\run_alpha_tests.md`` | Test-Runner-Bedienung, Prios, Logs |")
            $lines.Add("| ``docs\ProPhysics_Testkatalog.md`` | alle 42 Tests mit Kriterien |")
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
            $lines.Add("")
            $lines.Add("## Build-Voraussetzungen")
            $lines.Add("")
            $lines.Add("- Visual Studio 2019/2022 Developer Prompt (nmake + cl)")
            $lines.Add("- Windows SDK (fuer signtool, falls gewuenscht)")
            $lines.Add("- Optional: git (fuer Release-Metadaten)")
            $lines.Add("")
            $lines.Add("## Build-Anleitung")
            $lines.Add("")
            $lines.Add("Die Makefiles und PowerShell-Wrapper liegen unter ``build\``.")
            $lines.Add("Bevorzugter Aufruf:")
            $lines.Add("")
            $lines.Add("``````cmd")
            $lines.Add("cd build\main")
            $lines.Add("build.cmd                  :: alles bauen")
            $lines.Add("build.cmd -Mode sdk        :: nur Kernel + SDK")
            $lines.Add("build.cmd -Rebuild         :: clean + alles")
            $lines.Add("``````")
            $lines.Add("")
            $lines.Add("Direkter nmake-Aufruf:")
            $lines.Add("")
            $lines.Add("``````cmd")
            $lines.Add("cd build\prophysics && nmake /f Makefile.nmake")
            $lines.Add("cd ..\sdk           && nmake /f Makefile.sdk.nmake")
            $lines.Add("cd ..\test          && nmake /f Makefile.nmake")
            $lines.Add("``````")
            $lines.Add("")
            $lines.Add("## Tests")
            $lines.Add("")
            $lines.Add("Nach dem Build:")
            $lines.Add("")
            $lines.Add("``````cmd")
            $lines.Add("cd tools")
            $lines.Add("run_alpha_tests.cmd -Prio all")
            $lines.Add("``````")
            $lines.Add("")
            $lines.Add("Erwartung: 42/42 PASS in ~43 min. Prios 1-4, 6, 7, 8 alleine")
            $lines.Add("in ~2,5 min.")
            $lines.Add("")
            $lines.Add("## Dokumentation")
            $lines.Add("")
            $lines.Add("Die komplette Dokumentation liegt unter ``docs\``:")
            $lines.Add("")
            $lines.Add("| Bereich | Datei(en) |")
            $lines.Add("|---|---|")
            $lines.Add("| Build | ``docs\build\BUILD_SCRIPT.md`` + ``build\<modul>\Makefile.md`` + ``build\helper\*.md`` |")
            $lines.Add("| Projekt | ``docs\project\Project.md`` |")
            $lines.Add("| Tests | ``docs\test\ProPhysics_Testkatalog.md`` + ``docs\test\run_alpha_tests.md`` |")
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
# Export: exe
#
# Refactoring 22: Runner-Skripte kommen jetzt aus tools\, nicht mehr aus
# bin\. Die Artefakte (DLLs + EXEs) bleiben in bin\.
# ==========================================================================
function Export-Exe {
    param([string]$OutDir)

    Write-Step "Export exe -> $OutDir"
    if ($Clean -and (Test-Path -LiteralPath $OutDir)) {
        Write-Warn2 'Leere Zielordner (-Clean)'
        if (-not $script:DryRun) { Remove-Item -LiteralPath $OutDir -Recurse -Force }
    }
    Ensure-Dir $OutDir

    $null = Copy-Item-Safe -Source $BinDir -DestDir $OutDir -Filter 'ProPhysics.dll' -Required

    if ($Scope -in @('sdk','test','all')) {
        $null = Copy-Item-Safe -Source $BinDir -DestDir $OutDir -Filter 'pro_sdk_interface.dll'
    }

    if ($Scope -in @('test','all')) {
        $null = Copy-Item-Safe -Source $BinDir -DestDir $OutDir -Filter 'example_*.exe' -Required

        # Runner aus tools\ (Refactoring 22).
        foreach ($f in @('run_alpha_tests.cmd','run_alpha_tests.ps1')) {
            $null = Copy-One-Safe -Path (Join-Path $ToolsDir $f) -DestDir $OutDir
        }
    }

    $null = Copy-BuildInfo -DestDir $OutDir
    $null = Copy-Docs -ExportMode 'exe' -DestDir $OutDir
    New-PackageReadme -ExportMode 'exe' -Scope $Scope -DestDir $OutDir

    Write-Ok "Fertig."
}

# ==========================================================================
# Export: sdk
# ==========================================================================
function Export-Sdk {
    param([string]$OutDir)

    Write-Step "Export sdk -> $OutDir"
    if ($Clean -and (Test-Path -LiteralPath $OutDir)) {
        Write-Warn2 'Leere Zielordner (-Clean)'
        if (-not $script:DryRun) { Remove-Item -LiteralPath $OutDir -Recurse -Force }
    }

    $libsDir  = Join-Path $OutDir 'libs'
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

    $null = Copy-BuildInfo -DestDir $OutDir
    $null = Copy-Docs -ExportMode 'sdk' -DestDir $libsDir
    New-PackageReadme -ExportMode 'sdk' -Scope $Scope -DestDir $OutDir

    Write-Ok "Fertig."
}

# ==========================================================================
# Export: src
#
# Refactoring 22: tools\ wird als eigener Ordner mitkopiert, damit der
# Test-Runner im Quellcode-Snapshot verfuegbar ist.
# ==========================================================================
function Export-Src {
    param([string]$OutDir)

    Write-Step "Export src -> $OutDir"
    if ($Clean -and (Test-Path -LiteralPath $OutDir)) {
        Write-Warn2 'Leere Zielordner (-Clean)'
        if (-not $script:DryRun) { Remove-Item -LiteralPath $OutDir -Recurse -Force }
    }
    Ensure-Dir $OutDir

    # prophysics (immer).
    $dstPP = Join-Path $OutDir 'prophysics'
    Ensure-Dir $dstPP
    $null = Copy-Tree -Source $SrcPP -DestDir $dstPP

    # sdk (bei Scope >= sdk).
    if ($Scope -in @('sdk','test','all')) {
        $dstSdk = Join-Path $OutDir 'sdk'
        Ensure-Dir $dstSdk
        $null = Copy-Tree -Source $SrcSdk -DestDir $dstSdk
    }

    # test (bei Scope >= test).
    if ($Scope -in @('test','all')) {
        $dstTst = Join-Path $OutDir 'test'
        Ensure-Dir $dstTst
        $null = Copy-Tree -Source $SrcTst -DestDir $dstTst
    }

    # tools\ (Refactoring 22): Runner-Skripte als eigener Ordner.
    if (Test-Path -LiteralPath $ToolsDir) {
        $dstTools = Join-Path $OutDir 'tools'
        Ensure-Dir $dstTools
        $null = Copy-Tree -Source $ToolsDir -DestDir $dstTools
    } else {
        Write-Warn2 "tools\ nicht gefunden -- kein Runner im src-Export"
    }

    $null = Copy-BuildInfo -DestDir $OutDir
    $null = Copy-Docs -ExportMode 'src' -DestDir $OutDir
    New-PackageReadme -ExportMode 'src' -Scope $Scope -DestDir $OutDir -PackageName $Name

    Write-Ok "Fertig."
}

# ==========================================================================
# Main
# ==========================================================================
if ($Mode -eq 'src' -and -not $Name) {
    $Name = Get-ProVersion
}

$targetRoot = switch ($Mode) {
    'exe' { Join-Path $OutRoot 'exe' }
    'sdk' { Join-Path $OutRoot 'sdk' }
    'src' { Join-Path $OutRoot "src\$Name" }
}

Write-Host ''
Write-Host '============================================================' -ForegroundColor Cyan
Write-Host "  ProPhysics Export  |  Mode: $Mode  Scope: $Scope" -ForegroundColor Cyan
Write-Host '============================================================' -ForegroundColor Cyan
Write-Host "  Repo:    $RepoRoot"
Write-Host "  Ziel:    $targetRoot"
if ($script:DryRun) {
    Write-Host '  Status:  DRY-RUN (nichts wird kopiert)' -ForegroundColor Yellow
}
Write-Host ''

if ($Mode -eq 'src') {
    Write-Step 'Modus src -- nur Kopieren, kein Build'
} else {
    Invoke-Build
}

switch ($Mode) {
    'exe' { Export-Exe -OutDir $targetRoot }
    'sdk' { Export-Sdk -OutDir $targetRoot }
    'src' { Export-Src -OutDir $targetRoot }
}

Write-Host ''
Write-Host '------------------------------------------------------------' -ForegroundColor Cyan
Write-Host '  Zusammenfassung' -ForegroundColor Cyan
Write-Host '------------------------------------------------------------' -ForegroundColor Cyan
if (Test-Path -LiteralPath $targetRoot) {
    $count = @(Get-ChildItem -LiteralPath $targetRoot -Recurse -File -ErrorAction SilentlyContinue).Count
    Write-Host ("  {0} Datei(en) in {1}" -f $count, $targetRoot)
} else {
    Write-Host '  (Zielordner nicht angelegt)'
}
Write-Host ''
exit 0