#Requires -Version 5.0
<#
.SYNOPSIS
    ProPhysics Build-Orchestrierung (Master).

.DESCRIPTION
    Duenner PowerShell-Wrapper um build\main\Makefile.nmake. Baut die
    gewaehlte Komponente, optional mit Rebuild, und delegiert die
    BUILD_INFO.txt-Erzeugung an write_build_info.ps1.

    Modi (Auswahl der zu bauenden Komponente):
      kernel / prophysics  -- nur Kernel-DLL + Import-Lib
      sdk                  -- Kernel + SDK-Interface-DLL + Import-Lib
      test                 -- Kernel + SDK + Test-EXEs
      all                  -- identisch mit test (alles)  [Default]
      info                 -- nur BUILD_INFO.txt neu erzeugen

    Erweiterungen:
      -Config <release|debug>  Build-Konfiguration (Default: release)
      -Rebuild                 nmake clean + Build
      -Clean                   nur clean, kein Build
      -NoBuild                 nur BUILD_INFO.txt
      -GitStamp                Git-Info in BUILD_INFO.txt
      -GitNote <text>          Freitext-Kommentar
      -Sign                    DLLs via signtool signieren
      -DryRun                  nur auflisten, nichts schreiben

    Ausgabe:
      bin\              DLLs und EXEs
      lib\              Import-Libs
      BUILD_INFO.txt    im Root

    Voraussetzung:
      nmake + cl im PATH (VS-Developer-Prompt).
      git optional (nur fuer -GitStamp).

.PARAMETER Mode
    Zu bauende Komponente. 'kernel' ist Alias fuer 'prophysics'.

.PARAMETER Config
    Build-Konfiguration. Wird als CONFIG=<Wert> an nmake weitergereicht.

.PARAMETER Rebuild
    nmake clean + Build in einem Schritt.

.PARAMETER Clean
    Nur clean, kein Build.

.PARAMETER NoBuild
    Build ueberspringen, nur BUILD_INFO.txt erzeugen.

.PARAMETER GitStamp
    Git-Info (describe/branch/sha/status) in BUILD_INFO.txt aufnehmen.

.PARAMETER GitNote
    Freitext-Kommentar fuer den Notiz-Block in BUILD_INFO.txt.

.PARAMETER Sign
    DLLs in bin\ via signtool signieren.

.PARAMETER DryRun
    Nur auflisten, nichts schreiben.

.EXAMPLE
    .\build.ps1

.EXAMPLE
    .\build.ps1 -Mode sdk -Config debug -Rebuild

.EXAMPLE
    .\build.ps1 -Mode kernel -Rebuild -GitStamp -GitNote "Etappe 23"

.EXAMPLE
    .\build.ps1 -Clean

.EXAMPLE
    .\build.ps1 -Mode all -Sign -DryRun

.EXAMPLE
    .\build.ps1 -Mode info -GitStamp

.NOTES
    Version: 3.2 (Etappe 23)
    -Config Parameter, 'kernel'-Alias, 'info'-Mode, CONFIG-Weitergabe
    an nmake, kein Doppelaufruf von write_build_info.ps1.
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory=$false, Position=0)]
    [ValidateSet('kernel','prophysics','sdk','test','all','info')]
    [string]$Mode = 'all',

    [ValidateSet('release','debug')]
    [string]$Config = 'release',

    [switch]$Rebuild,
    [switch]$Clean,
    [switch]$NoBuild,

    [switch]$GitStamp,
    [string]$GitNote = '',
    [switch]$Sign,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
$script:DryRun = $DryRun.IsPresent

# ==========================================================================
# UTF-8 Konsole (PowerShell + native Tools wie nmake/cl/git/signtool)
# ==========================================================================
try {
    [Console]::OutputEncoding = [System.Text.Encoding]::UTF8
    [Console]::InputEncoding  = [System.Text.Encoding]::UTF8
    $OutputEncoding           = [System.Text.Encoding]::UTF8
} catch { }
try { & chcp.com 65001 > $null 2>&1 } catch { }

# ==========================================================================
# Pfad-Konfiguration
#
# build.ps1 liegt in <repo>\build\main\.
# Repo-Root ist zwei Ebenen hoeher.
# ==========================================================================
$RepoRoot   = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$BuildMain  = $PSScriptRoot
$MakeFile   = Join-Path $BuildMain  'Makefile.nmake'
$InfoScript = Join-Path $BuildMain  'write_build_info.ps1'

$BinDir     = Join-Path $RepoRoot   'bin'
$LibDir     = Join-Path $RepoRoot   'lib'
$InfoFile   = Join-Path $RepoRoot   'BUILD_INFO.txt'

# ==========================================================================
# Helper
# ==========================================================================
function Write-Step  ($m) { Write-Host "[*] $m"      -ForegroundColor Cyan }
function Write-Ok    ($m) { Write-Host "    OK  $m"  -ForegroundColor Green }
function Write-Warn2 ($m) { Write-Host "    --  $m"  -ForegroundColor Yellow }
function Write-Err   ($m) { Write-Host "    !!  $m"  -ForegroundColor Red }
function Write-Dry   ($m) { Write-Host "    [dry] $m" -ForegroundColor DarkGray }

# ==========================================================================
# nmake-Target aus Mode + Flags ableiten
#
# 'kernel' ist Alias fuer 'prophysics'.
# 'info' ist ein reiner Metadaten-Target ohne Build.
# ==========================================================================
function Get-NmakeTarget {
    if ($Mode -eq 'info') { return 'info' }

    if ($Clean -and -not $Rebuild) {
        switch ($Mode) {
            'kernel'     { return 'clean_prophysics' }
            'prophysics' { return 'clean_prophysics' }
            'sdk'        { return 'clean_sdk' }
            'test'       { return 'clean_test' }
            'all'        { return 'clean' }
        }
    }

    if ($Rebuild) {
        switch ($Mode) {
            'kernel'     { return 'rebuild_prophysics' }
            'prophysics' { return 'rebuild_prophysics' }
            'sdk'        { return 'rebuild_sdk' }
            'test'       { return 'rebuild_test' }
            'all'        { return 'rebuild' }
        }
    }

    switch ($Mode) {
        'kernel'     { return 'prophysics' }
        'prophysics' { return 'prophysics' }
        'sdk'        { return 'sdk' }
        'test'       { return 'test' }
        'all'        { return 'all' }
    }
}

# ==========================================================================
# Liste der nmake-Targets, die den info-Target bereits selbst ausfuehren.
#
# Master-Makefile:
#   all                 -> ... info
#   rebuild             -> clean all          (all enthaelt info)
#   rebuild_prophysics  -> clean_prophysics prophysics info
#   rebuild_sdk         -> clean_sdk sdk info
#   rebuild_test        -> clean_test test info
#
# Fuer diese Targets darf build.ps1 NICHT nochmal write_build_info.ps1
# aufrufen, sonst wird die Datei doppelt erzeugt.
# ==========================================================================
function Test-NmakeHandlesInfo {
    param([string]$Target)
    return @(
        'all',
        'rebuild',
        'rebuild_prophysics',
        'rebuild_sdk',
        'rebuild_test'
    ) -contains $Target
}

# ==========================================================================
# nmake aufrufen -- mit CONFIG-Weitergabe
# ==========================================================================
function Invoke-Nmake {
    param([string]$Target)

    $nmake = Get-Command nmake -ErrorAction SilentlyContinue
    if (-not $nmake) {
        Write-Err 'nmake nicht im PATH. Bitte VS-Developer-Prompt oeffnen.'
        exit 2
    }

    if ($script:DryRun) {
        Write-Dry "cd `"$BuildMain`" && nmake /NOLOGO /f Makefile.nmake $Target CONFIG=$Config"
        return
    }

    Write-Step "nmake $Target CONFIG=$Config"
    Push-Location $BuildMain
    try {
        & nmake /NOLOGO /f Makefile.nmake $Target "CONFIG=$Config"
        if ($LASTEXITCODE -ne 0) {
            Write-Err "nmake fehlgeschlagen (Exit $LASTEXITCODE)"
            exit $LASTEXITCODE
        }
    } finally { Pop-Location }
    Write-Ok "nmake $Target abgeschlossen."
}

# ==========================================================================
# Git-Info (nur wenn -GitStamp gesetzt)
# ==========================================================================
function Get-GitInfo {
    $git = Get-Command git -ErrorAction SilentlyContinue
    if (-not $git) { return $null }

    Push-Location $RepoRoot
    try {
        $null = & git rev-parse --is-inside-work-tree 2>$null
        if ($LASTEXITCODE -ne 0) { return $null }

        $desc   = (& git describe --tags --dirty --always --long 2>$null)
        $branch = (& git rev-parse --abbrev-ref HEAD 2>$null)
        $sha    = (& git rev-parse --short HEAD 2>$null)
        $porc   = (& git status --porcelain 2>$null)

        return [pscustomobject]@{
            Describe = if ($desc)   { $desc.Trim() }   else { 'n/a' }
            Branch   = if ($branch) { $branch.Trim() } else { 'n/a' }
            Sha      = if ($sha)    { $sha.Trim() }    else { 'n/a' }
            Dirty    = if ($porc)   { 'dirty' }        else { 'clean' }
        }
    } catch { return $null }
    finally { Pop-Location }
}

# ==========================================================================
# Signieren (Ziel: bin\*.dll)
# ==========================================================================
function Invoke-Sign {
    if ($script:DryRun) { Write-Step 'Sign uebersprungen (-DryRun)'; return 0 }

    $signtool = Get-Command signtool.exe -ErrorAction SilentlyContinue
    if (-not $signtool) {
        Write-Warn2 'signtool.exe nicht gefunden - Signierung uebersprungen'
        Write-Warn2 '  (Install: Windows SDK / VS -> "Windows SDK Signing Tools")'
        return 0
    }
    if (-not (Test-Path -LiteralPath $BinDir)) {
        Write-Warn2 "bin\ nicht gefunden - nichts zu signieren"
        return 0
    }

    Write-Step 'Signiere DLLs in bin\ ...'
    $signed = 0; $failed = 0
    $dlls = @(Get-ChildItem -LiteralPath $BinDir -Filter '*.dll' -File)
    foreach ($d in $dlls) {
        Write-Host "    sign $($d.Name)" -ForegroundColor DarkGray
        & signtool sign /a /fd SHA256 `
            /tr http://timestamp.digicert.com /td SHA256 `
            $d.FullName 2>&1 | Out-Null
        if ($LASTEXITCODE -eq 0) { $signed++ }
        else { $failed++; Write-Warn2 "  Fehler: $($d.Name)" }
    }
    Write-Ok "Signiert: $signed / $($dlls.Count) (Fehler: $failed)"
    return $signed
}

# ==========================================================================
# BUILD_INFO.txt an write_build_info.ps1 delegieren
# ==========================================================================
function Invoke-BuildInfo {
    if (-not (Test-Path -LiteralPath $InfoScript)) {
        Write-Warn2 "write_build_info.ps1 nicht gefunden: $InfoScript"
        return
    }

    if ($script:DryRun) {
        Write-Step 'BUILD_INFO.txt wuerde geschrieben werden'
        Write-Dry "powershell -File write_build_info.ps1 -RepoRoot `"$RepoRoot`" -Config $Config ..."
        return
    }

    Write-Step 'BUILD_INFO.txt schreiben...'
    $psArgs = @(
        '-NoProfile'
        '-ExecutionPolicy','Bypass'
        '-File', $InfoScript
        '-RepoRoot', $RepoRoot
        '-Config', $Config
    )
    if ($GitStamp) { $psArgs += '-GitStamp' }
    if ($GitNote -and $GitNote.Trim().Length -gt 0) {
        $psArgs += '-GitNote'
        $psArgs += $GitNote
    }

    & powershell.exe @psArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Warn2 "write_build_info.ps1 exit $LASTEXITCODE"
    }
}

# ==========================================================================
# Main
# ==========================================================================
Write-Host ''
Write-Host '============================================================' -ForegroundColor Cyan
Write-Host "  ProPhysics Build  |  Modus: $Mode  Config: $Config" -ForegroundColor Cyan
Write-Host '============================================================' -ForegroundColor Cyan
Write-Host "  Repo:   $RepoRoot"
Write-Host "  Build:  $BuildMain"

if ($script:DryRun) {
    Write-Host '  Status: DRY-RUN (nichts wird geschrieben)' -ForegroundColor Yellow
}

# Flags-Konflikt-Check
if ($Clean -and $NoBuild -and -not $Rebuild) {
    Write-Warn2 '-Clean + -NoBuild: nur Clean wird ausgefuehrt (kein Build, keine Info)'
}
if ($Mode -eq 'info' -and ($Rebuild -or $Clean)) {
    Write-Warn2 "-Mode info ignoriert -Rebuild und -Clean"
}

# Optionale Git-Vorschau
if ($GitStamp) {
    $g = Get-GitInfo
    if ($g) {
        Write-Host "  Git:    $($g.Describe)  [$($g.Branch) @ $($g.Sha) / $($g.Dirty)]" -ForegroundColor DarkCyan
    } else {
        Write-Host '  Git:    nicht verfuegbar' -ForegroundColor DarkYellow
    }
}
if ($GitNote -and $GitNote.Trim().Length -gt 0) {
    $firstLine = ($GitNote -split "`r?`n")[0]
    Write-Host "  Notiz:  $firstLine" -ForegroundColor DarkCyan
}
Write-Host ''

# --- 1. Clean-only / Info-only / Build ---
$cleanOnly = $Clean -and -not $Rebuild
$skipBuild = $NoBuild -or $cleanOnly -or ($Mode -eq 'info')

if ($Mode -eq 'info') {
    Write-Step 'Nur BUILD_INFO (kein Build)'
}
elseif ($cleanOnly) {
    Write-Step 'Nur Clean (kein Build)'
    Invoke-Nmake -Target (Get-NmakeTarget)
}
elseif ($NoBuild) {
    Write-Step 'Build uebersprungen (-NoBuild)'
}
else {
    $target = Get-NmakeTarget
    Invoke-Nmake -Target $target
}

# --- 2. Sign (nur nach erfolgreichem Build) ---
if ($Sign -and -not $skipBuild) {
    $null = Invoke-Sign
}

# --- 3. BUILD_INFO ---
#
# Aufrufregel:
#   - clean-only:  keine Info
#   - Mode=info:   nur Info
#   - -NoBuild:    nur Info (Build lief nicht, Info soll trotzdem aktuell sein)
#   - Target all/rebuild*:  Makefile hat info schon ausgefuehrt -> nichts
#   - Target prophysics/sdk/test:  Makefile hat info NICHT ausgefuehrt -> Info
#
if (-not $cleanOnly) {
    if ($Mode -eq 'info' -or $NoBuild) {
        Invoke-BuildInfo
    }
    else {
        $target = Get-NmakeTarget
        if (Test-NmakeHandlesInfo -Target $target) {
            Write-Step "BUILD_INFO.txt bereits durch Makefile-Target '$target' erzeugt"
        }
        else {
            Invoke-BuildInfo
        }
    }
}

# --- 4. Zusammenfassung ---
Write-Host ''
Write-Host '------------------------------------------------------------' -ForegroundColor Cyan
Write-Host '  Zusammenfassung' -ForegroundColor Cyan
Write-Host '------------------------------------------------------------' -ForegroundColor Cyan

if (Test-Path -LiteralPath $BinDir) {
    $exes = @(Get-ChildItem -LiteralPath $BinDir -Filter '*.exe' -File -EA SilentlyContinue).Count
    $dlls = @(Get-ChildItem -LiteralPath $BinDir -Filter '*.dll' -File -EA SilentlyContinue).Count
    Write-Host ("  bin\:   {0} DLL(s), {1} EXE(s)" -f $dlls, $exes)
} else {
    Write-Host '  bin\:   (fehlt)'
}

if (Test-Path -LiteralPath $LibDir) {
    $libs = @(Get-ChildItem -LiteralPath $LibDir -Filter '*.lib' -File -EA SilentlyContinue).Count
    Write-Host ("  lib\:   {0} LIB(s)" -f $libs)
} else {
    Write-Host '  lib\:   (fehlt)'
}

if (Test-Path -LiteralPath $InfoFile) {
    Write-Host  '  Root:   BUILD_INFO.txt'
} else {
    Write-Host  '  Root:   (BUILD_INFO.txt fehlt)'
}

Write-Host ''
exit 0