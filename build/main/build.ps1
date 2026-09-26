#Requires -Version 5.0
<#
.SYNOPSIS
    ProPhysics Build-Orchestrierung (Master).

.DESCRIPTION
    Duenner PowerShell-Wrapper um build\main\Makefile.nmake. Baut die
    gewaehlte Komponente, optional mit Rebuild, und delegiert die
    BUILD_INFO.txt-Erzeugung an write_build_info.ps1.

    Modi (Auswahl der zu bauenden Komponente):
      prophysics  -- nur Kernel-DLL + Import-Lib
      sdk         -- Kernel + SDK-Interface-DLL + Import-Lib
      test        -- Kernel + SDK + Test-EXEs
      all         -- identisch mit test (alles)

    Erweiterungen:
      -Rebuild    nmake clean + Build in einem Schritt
      -Clean      nur clean, kein Build
      -NoBuild    nur BUILD_INFO.txt neu erzeugen
      -GitStamp   Git-Info in BUILD_INFO.txt (describe/branch/sha/status)
      -GitNote    Freitext-Kommentar fuer den Notiz-Block
      -Sign       DLLs in bin\ via signtool signieren
      -DryRun     nur auflisten, nichts schreiben

    Ausgabe:
      bin\              DLLs und EXEs
      lib\              Import-Libs
      BUILD_INFO.txt    im Root

    Voraussetzung:
      nmake + cl im PATH (VS-Developer-Prompt).
      git optional (nur fuer -GitStamp).

.EXAMPLE
    .\build.ps1

.EXAMPLE
    .\build.ps1 -Mode sdk -Rebuild -GitStamp -GitNote "SDK v3.0"

.EXAMPLE
    .\build.ps1 -Mode prophysics -Rebuild

.EXAMPLE
    .\build.ps1 -Clean

.EXAMPLE
    .\build.ps1 -Mode all -Sign -DryRun
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory=$false, Position=0)]
    [ValidateSet('prophysics','sdk','test','all')]
    [string]$Mode = 'all',

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
# ==========================================================================
function Get-NmakeTarget {
    if ($Clean -and -not $Rebuild) {
        switch ($Mode) {
            'prophysics' { return 'clean_prophysics' }
            'sdk'        { return 'clean_sdk' }
            'test'       { return 'clean_test' }
            'all'        { return 'clean' }
        }
    }
    if ($Rebuild) {
        switch ($Mode) {
            'prophysics' { return 'rebuild_prophysics' }
            'sdk'        { return 'rebuild_sdk' }
            'test'       { return 'rebuild_test' }
            'all'        { return 'rebuild' }
        }
    }
    switch ($Mode) {
        'prophysics' { return 'prophysics' }
        'sdk'        { return 'sdk' }
        'test'       { return 'test' }
        'all'        { return 'all' }
    }
}

# ==========================================================================
# nmake aufrufen
# ==========================================================================
function Invoke-Nmake {
    param([string]$Target)

    $nmake = Get-Command nmake -ErrorAction SilentlyContinue
    if (-not $nmake) {
        Write-Err 'nmake nicht im PATH. Bitte VS-Developer-Prompt oeffnen.'
        exit 2
    }

    if ($script:DryRun) {
        Write-Dry "cd `"$BuildMain`" && nmake /NOLOGO /f Makefile.nmake $Target"
        return
    }

    Write-Step "nmake $Target"
    Push-Location $BuildMain
    try {
        & nmake /NOLOGO /f Makefile.nmake $Target
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
        Write-Dry "powershell -File write_build_info.ps1 -RepoRoot `"$RepoRoot`" ..."
        return
    }

    Write-Step 'BUILD_INFO.txt schreiben...'
    $psArgs = @(
        '-NoProfile'
        '-ExecutionPolicy','Bypass'
        '-File', $InfoScript
        '-RepoRoot', $RepoRoot
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
Write-Host "  ProPhysics Build  |  Modus: $Mode" -ForegroundColor Cyan
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

# --- 1. Clean-only (kein Build) ---
$cleanOnly = $Clean -and -not $Rebuild
$skipBuild = $NoBuild -or $cleanOnly

if ($cleanOnly) {
    Write-Step "Nur Clean (kein Build)"
    Invoke-Nmake -Target (Get-NmakeTarget)
}
elseif ($NoBuild) {
    Write-Step 'Build uebersprungen (-NoBuild)'
}
else {
    Invoke-Nmake -Target (Get-NmakeTarget)
}

# --- 2. Sign (nur nach erfolgreichem Build) ---
if ($Sign -and -not $skipBuild) {
    $null = Invoke-Sign
}

# --- 3. BUILD_INFO (ausser bei reinem Clean) ---
if (-not $cleanOnly) {
    Invoke-BuildInfo
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