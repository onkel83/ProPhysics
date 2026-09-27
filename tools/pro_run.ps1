#Requires -Version 5.0
<#
.SYNOPSIS
    ProPhysics -- zentraler Einstiegspunkt fuer Build, Test und Export.

.DESCRIPTION
    Duenner Dispatcher um die bestehenden Skripte:

      build   -> build\main\build.ps1
      export  -> build\main\export.ps1
      test    -> tools\run_alpha_tests.ps1

    Der Aufruf 'all' fuehrt build -> test -> export in dieser
    Reihenfolge aus. Fehlschlag eines Schritts bricht ab.

    Die Optionen werden an die jeweiligen Subskripte weitergereicht.
    Details siehe 'pro_run help <aktion>'.

.PARAMETER Aktion
    build | export | test | all | help   (Mandatory, Position 0)

.PARAMETER Mode
    build: all | kernel | sdk | test   (Default: all)

.PARAMETER Config
    build: release | debug            (Default: release)

.PARAMETER Rebuild
    build: clean + build

.PARAMETER Export
    export: exe | sdk | kit | all     (Default: sdk)

.PARAMETER OutDir
    export: Zielverzeichnis            (Default: <repo>\out)

.PARAMETER Version
    export: Versions-Tag               (Default: aus ProPhysics_Version.h)

.PARAMETER Prio
    test: all | 1..8 | 1-4 | 1,3,5     (Default: all)

.PARAMETER Test
    test: einzelner Test nach Name (statt Prio)

.PARAMETER ExeDir
    test: EXE-Verzeichnis              (Default: <repo>\bin)

.PARAMETER DllDir
    test: DLL-Verzeichnis              (Default: <repo>\bin)

.PARAMETER LogDir
    test: Log-Verzeichnis              (Default: <ExeDir>\logs)

.PARAMETER NoBuild
    all: build-Schritt ueberspringen

.PARAMETER NoTest
    all: test-Schritt ueberspringen

.PARAMETER NoExport
    all: export-Schritt ueberspringen

.PARAMETER DryRun
    Nur auflisten, nichts ausfuehren (alle Aktionen)

.PARAMETER Verbose_
    Ausfuehrliche Ausgabe (wird an Subskripte durchgereicht)

.EXAMPLE
    pro_run all
    pro_run build -Config debug -Rebuild
    pro_run test -Prio 1-4
    pro_run test -Test Running-Coupling -LogDir C:\logs
    pro_run export -Export kit -OutDir D:\sdk-kit
    pro_run all -NoBuild -NoExport -Prio 1
    pro_run help
    pro_run help build

.NOTES
    Version: 3.2 (Etappe 23)
    Self-Locating: RepoRoot = $PSScriptRoot\..  (pro_run liegt in tools\).
    Alle Subskripte werden via 'powershell.exe -File' aufgerufen; die
    Exit-Codes werden sauber propagiert.
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory=$true, Position=0)]
    [ValidateSet('build','export','test','all','help')]
    [string]$Aktion,

    # --- build ---
    [ValidateSet('all','kernel','sdk','test')]
    [string]$Mode = 'all',

    [ValidateSet('release','debug')]
    [string]$Config = 'release',

    [switch]$Rebuild,

    # --- export ---
    [ValidateSet('exe','sdk','kit','all')]
    [string]$Export = 'sdk',

    [string]$OutDir,
    [string]$Version,

    # --- test ---
    [string]$Prio = 'all',
    [string]$Test,
    [string]$ExeDir,
    [string]$DllDir,
    [string]$LogDir,

    # --- allgemein ---
    [switch]$NoBuild,
    [switch]$NoTest,
    [switch]$NoExport,
    [switch]$DryRun,

    # Hilfs-Alias, weil -Verbose von PowerShell reserviert ist.
    [Alias('Verbose_','v')]
    [switch]$ShowVerbose
)

$ErrorActionPreference = 'Stop'
$script:DryRun = $DryRun.IsPresent
$script:ShowVerbose = $ShowVerbose.IsPresent

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
#
# pro_run.ps1 liegt in <repo>\tools\.
# RepoRoot ist eine Ebene hoeher.
# ==========================================================================
$RepoRoot  = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$BuildMain = Join-Path $RepoRoot 'build\main'
$ToolsDir  = $PSScriptRoot

$ScriptBuild  = Join-Path $BuildMain 'build.ps1'
$ScriptExport = Join-Path $BuildMain 'export.ps1'
$ScriptTest   = Join-Path $ToolsDir  'run_alpha_tests.ps1'

# ==========================================================================
# Helper
# ==========================================================================
function Write-Head ($m) {
    Write-Host ''
    Write-Host '============================================================' -ForegroundColor Cyan
    Write-Host "  $m" -ForegroundColor Cyan
    Write-Host '============================================================' -ForegroundColor Cyan
}
function Write-Step  ($m) { Write-Host "[*] $m"      -ForegroundColor Cyan }
function Write-Ok    ($m) { Write-Host "    OK  $m"  -ForegroundColor Green }
function Write-Warn2 ($m) { Write-Host "    --  $m"  -ForegroundColor Yellow }
function Write-Err   ($m) { Write-Host "    !!  $m"  -ForegroundColor Red }
function Write-Dry   ($m) { Write-Host "    [dry] $m" -ForegroundColor DarkGray }

# ==========================================================================
# Hilfe
# ==========================================================================
function Show-Help {
    param([string]$Topic)

    if (-not $Topic) {
        Write-Head 'pro_run -- zentraler Einstiegspunkt'
        Write-Host @"
  Aufruf:  pro_run <aktion> [optionen]

  AKTIONEN
    build     Baut Kernel, SDK und/oder Tests.
    export    Packt Artefakte als EXE-, SDK- oder KIT-Archiv.
    test      Fuehrt die Alpha-Test-Suite aus.
    all       Fuehrt build -> test -> export aus.
    help      Diese Uebersicht (oder 'pro_run help <aktion>').

  BEISPIELE
    pro_run all
    pro_run build -Config debug -Rebuild
    pro_run test -Prio 1-4
    pro_run test -Test Running-Coupling -LogDir C:\logs
    pro_run export -Export kit -OutDir D:\sdk-kit
    pro_run all -NoBuild -NoExport -Prio 1

  Weiter: 'pro_run help build', 'pro_run help export', 'pro_run help test'
"@
        return
    }

    switch ($Topic) {
        'build' {
            Write-Head 'pro_run build'
            Write-Host @"
  Aufruf:  pro_run build [optionen]

  OPTIONEN
    -Mode <all|kernel|sdk|test>   (Default: all)
    -Config <release|debug>       (Default: release)
    -Rebuild                      clean + build
    -DryRun                       nur auflisten

  BEISPIELE
    pro_run build
    pro_run build -Mode kernel -Rebuild
    pro_run build -Config debug
"@
        }
        'export' {
            Write-Head 'pro_run export'
            Write-Host @"
  Aufruf:  pro_run export [optionen]

  OPTIONEN
    -Export <exe|sdk|kit|all>     (Default: sdk)
    -OutDir <pfad>                (Default: <repo>\out)
    -Version <tag>                (Default: aus ProPhysics_Version.h)
    -DryRun                       nur auflisten

  BEISPIELE
    pro_run export
    pro_run export -Export kit
    pro_run export -Export all -OutDir D:\release
    pro_run export -Version 1.23.1
"@
        }
        'test' {
            Write-Head 'pro_run test'
            Write-Host @"
  Aufruf:  pro_run test [optionen]

  OPTIONEN
    -Prio <all|1..8|1-4|1,3,5>    (Default: all)
    -Test <name>                  statt Prio
    -ExeDir <pfad>                (Default: <repo>\bin)
    -DllDir <pfad>                (Default: <repo>\bin)
    -LogDir <pfad>                (Default: <ExeDir>\logs)
    -DryRun                       nur auflisten

  BEISPIELE
    pro_run test
    pro_run test -Prio 1-4
    pro_run test -Prio 8
    pro_run test -Test Running-Coupling -LogDir C:\logs
"@
        }
        'all' {
            Write-Head 'pro_run all'
            Write-Host @"
  Aufruf:  pro_run all [optionen]

  Fuehrt build -> test -> export in dieser Reihenfolge aus.
  Fehlschlag eines Schritts bricht ab.

  OPTIONEN (Auswahl, alle Subskript-Optionen moeglich)
    -NoBuild        build-Schritt ueberspringen
    -NoTest         test-Schritt ueberspringen
    -NoExport       export-Schritt ueberspringen
    -Config <...>   an build weiterreichen
    -Prio <...>     an test weiterreichen
    -Export <...>   an export weiterreichen

  BEISPIELE
    pro_run all
    pro_run all -Config debug
    pro_run all -NoBuild
    pro_run all -NoExport -Prio 1-4
"@
        }
        default {
            Write-Err "Unbekanntes Hilfethema: $Topic"
            Write-Host "  Moeglich: build, export, test, all"
        }
    }
}

# ==========================================================================
# Subskript-Aufruf
#
# Aufruf via 'powershell.exe -File' statt '& $script': robuster gegen
# Execution-Policy-Restriktionen und garantiert saubere Exit-Codes.
#
# Rueckgabe: Exit-Code (0 = OK).
# ==========================================================================
function Invoke-SubScript {
    param(
        [Parameter(Mandatory=$true)][string]$ScriptPath,
        [Parameter(Mandatory=$true)][string]$Label,
        [string[]]$Args = @()
    )

    if (-not (Test-Path -LiteralPath $ScriptPath)) {
        Write-Err "$Label-Skript fehlt: $ScriptPath"
        return 2
    }

    if ($script:DryRun) {
        $argStr = ($Args | ForEach-Object { "`"$_`"" }) -join ' '
        Write-Dry "powershell -File `"$ScriptPath`" $argStr"
        return 0
    }

    $psArgs = @(
        '-NoProfile'
        '-ExecutionPolicy','Bypass'
        '-File', $ScriptPath
    ) + $Args
    if ($script:ShowVerbose) { $psArgs += '-Verbose' }

    Write-Host ''
    Write-Step "$Label  ($([System.IO.Path]::GetFileName($ScriptPath)))"
    & powershell.exe @psArgs
    $exit = $LASTEXITCODE
    if ($exit -ne 0) {
        Write-Err "$Label fehlgeschlagen (Exit $exit)"
    } else {
        Write-Ok "$Label abgeschlossen."
    }
    return $exit
}

# ==========================================================================
# Argument-Listen pro Aktion aufbauen
# ==========================================================================
function Get-BuildArgs {
    $a = @()
    if ($Mode)     { $a += @('-Mode',   $Mode) }
    if ($Config)   { $a += @('-Config', $Config) }
    if ($Rebuild)  { $a += '-Rebuild' }
    return $a
}

function Get-ExportArgs {
    $a = @()
    if ($Export)  { $a += @('-Export', $Export) }
    if ($OutDir)  { $a += @('-OutDir', $OutDir) }
    if ($Version) { $a += @('-Version', $Version) }
    return $a
}

function Get-TestArgs {
    $a = @()
    if ($Test)    { $a += @('-Test',   $Test) }
    else          { $a += @('-Prio',   $Prio) }
    if ($ExeDir)  { $a += @('-ExeDir', $ExeDir) }
    if ($DllDir)  { $a += @('-DllDir', $DllDir) }
    if ($LogDir)  { $a += @('-LogDir', $LogDir) }
    return $a
}

# ==========================================================================
# Aktionen
# ==========================================================================
function Do-Build {
    $args = Get-BuildArgs
    if ($script:DryRun) { $args += '-DryRun' }
    return Invoke-SubScript -ScriptPath $ScriptBuild -Label 'Build' -Args $args
}

function Do-Export {
    $args = Get-ExportArgs
    if ($script:DryRun) { $args += '-DryRun' }
    return Invoke-SubScript -ScriptPath $ScriptExport -Label 'Export' -Args $args
}

function Do-Test {
    $args = Get-TestArgs
    if ($script:DryRun) { $args += '-DryRun' }
    return Invoke-SubScript -ScriptPath $ScriptTest -Label 'Test' -Args $args
}

function Do-All {
    $step = 0
    $failures = @()

    # --- 1. Build ---
    if ($NoBuild) {
        Write-Warn2 'Build uebersprungen (-NoBuild)'
    } else {
        $step++
        $rc = Do-Build
        if ($rc -ne 0) {
            $failures += "build (exit $rc)"
            Write-Err 'Abbruch: build fehlgeschlagen.'
            return 1
        }
    }

    # --- 2. Test ---
    if ($NoTest) {
        Write-Warn2 'Test uebersprungen (-NoTest)'
    } else {
        $step++
        $rc = Do-Test
        if ($rc -ne 0) {
            $failures += "test (exit $rc)"
            Write-Err 'Abbruch: test fehlgeschlagen.'
            return $rc
        }
    }

    # --- 3. Export ---
    if ($NoExport) {
        Write-Warn2 'Export uebersprungen (-NoExport)'
    } else {
        $step++
        $rc = Do-Export
        if ($rc -ne 0) {
            $failures += "export (exit $rc)"
            Write-Err 'Abbruch: export fehlgeschlagen.'
            return $rc
        }
    }

    if ($step -eq 0) {
        Write-Warn2 'Alle Schritte uebersprungen (NoBuild/NoTest/NoExport).'
    }
    if ($failures.Count -gt 0) { return 1 }
    return 0
}

# ==========================================================================
# Main
# ==========================================================================
if ($Aktion -eq 'help') {
    Show-Help -Topic $Test     # -Test wird als Alias fuer Hilfethema genutzt.
    exit 0
}

Write-Head "pro_run  |  $Aktion"
Write-Host "  Repo:   $RepoRoot"
Write-Host "  Config: $Config"
if ($script:DryRun) {
    Write-Host '  Status: DRY-RUN (nichts wird ausgefuehrt)' -ForegroundColor Yellow
}

$start = Get-Date
$rc = 0

switch ($Aktion) {
    'build'  { $rc = Do-Build  }
    'export' { $rc = Do-Export }
    'test'   { $rc = Do-Test   }
    'all'    { $rc = Do-All    }
}

$elapsed = ((Get-Date) - $start).TotalSeconds

Write-Host ''
Write-Host '------------------------------------------------------------' -ForegroundColor Cyan
if ($rc -eq 0) {
    Write-Host ("  pro_run {0}: OK  ({1:N1}s)" -f $Aktion, $elapsed) -ForegroundColor Green
} else {
    Write-Host ("  pro_run {0}: FAILED (exit {1}, {2:N1}s)" -f $Aktion, $rc, $elapsed) -ForegroundColor Red
}
Write-Host '------------------------------------------------------------' -ForegroundColor Cyan
Write-Host ''

exit $rc