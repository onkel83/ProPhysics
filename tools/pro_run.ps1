#Requires -Version 5.0
<#
.SYNOPSIS
    ProPhysics -- zentraler Einstiegspunkt fuer Build, Test, Export und Web-Docs.

.DESCRIPTION
    Duenner Dispatcher um die bestehenden Skripte:

      build   -> build\main\build.ps1
      export  -> build\main\export.ps1
      test    -> tools\run_alpha_tests.ps1
      web     -> build\prowb\Makefile.nmake + bin\...\prowb.exe

    Der Aufruf 'all' fuehrt build -> test -> export in dieser
    Reihenfolge aus. Fehlschlag eines Schritts bricht ab.
    'web' ist bewusst NICHT Teil von 'all' -- der Web-Docs-Build
    laeuft unabhaengig vom Kernel und wird separat aufgerufen.

    Die Optionen werden an die jeweiligen Subskripte weitergereicht.
    Details siehe 'pro_run help <aktion>'.

.PARAMETER Aktion
    build | export | test | web | all | help   (Mandatory, Position 0)

.PARAMETER Mode
    build: all | kernel | sdk | test   (Default: all)

.PARAMETER Config
    build | web: release | debug       (Default: release)

.PARAMETER Rebuild
    build | web: clean + build

.PARAMETER Export
    export: exe | sdk | kit | all      (Default: sdk)

.PARAMETER OutDir
    export: Zielverzeichnis            (Default: <repo>\out)

.PARAMETER WebOutDir
    web: Zielverzeichnis fuer index.html   (Default: <repo>\out\web)

.PARAMETER Version
    export: Versions-Tag               (Default: aus ProPhysics_Version.h)

.PARAMETER Prio
    test: all | 1..8 | 1-4 | 1,3,5     (Default: all)

.PARAMETER Test
    test: einzelner Test nach Name (statt Prio)
    help: optionaler Hilfethema-Alias (help build, help web, ...)

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

.PARAMETER ShowVerbose
    Ausfuehrliche Ausgabe (wird an Subskripte durchgereicht)

.EXAMPLE
    pro_run all
    pro_run build -Config debug -Rebuild
    pro_run test -Prio 1-4
    pro_run test -Test Running-Coupling -LogDir C:\logs
    pro_run export -Export kit -OutDir D:\sdk-kit
    pro_run web
    pro_run web -Rebuild
    pro_run web -Config debug -WebOutDir out\web-local
    pro_run all -NoBuild -NoExport -Prio 1
    pro_run help
    pro_run help build
    pro_run help web

.NOTES
    Version: 3.3.3 (Etappe 23 + ProWB)
    Self-Locating: RepoRoot = $PSScriptRoot\..  (pro_run liegt in tools\).
    Alle Subskripte werden via 'powershell.exe -File' aufgerufen; die
    Exit-Codes werden sauber propagiert.

    WICHTIG fuer Wartung:
    ---------------------
    PowerShell-Funktionen, die einen Exit-Code zurueckgeben, duerfen
    NICHTS anderes in ihren Erfolgsstrom schreiben. Sonst wird der
    Rueckgabewert ein Array aus Konsolzeilen + Exit-Code.

    Falsch:  & nmake ... 2>&1 | ForEach-Object { Write-Host $_ }
             (in Windows PowerShell 5.x leakt Write-Host aus
              ForEach-Object in den Erfolgsstrom der Funktion)

    Richtig: $prev = $ErrorActionPreference
             $ErrorActionPreference = 'Continue'
             $out = & nmake ... 2>&1
             $ErrorActionPreference = $prev
             foreach ($line in $out) { Write-Host $line }
             return [int]$LASTEXITCODE

    $ErrorActionPreference = 'Stop' wuerde sonst den ersten Fehler
    aus dem stderr-Stream als Ausnahme werfen und den restlichen
    Output verschlucken.
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory=$true, Position=0)]
    [ValidateSet('build','export','test','web','all','help')]
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

    # --- web ---
    [string]$WebOutDir,

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
# ==========================================================================
$RepoRoot  = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$BuildMain = Join-Path $RepoRoot 'build\main'
$ToolsDir  = $PSScriptRoot

$ScriptBuild  = Join-Path $BuildMain 'build.ps1'
$ScriptExport = Join-Path $BuildMain 'export.ps1'
$ScriptTest   = Join-Path $ToolsDir  'run_alpha_tests.ps1'

# --- ProWB-spezifische Pfade --------------------------------------------
$BuildProwb    = Join-Path $RepoRoot 'build\prowb'
$ProwbSrcDir   = Join-Path $RepoRoot 'docs\web\src'
$ProwbManifest = Join-Path $RepoRoot 'docs\web\manifest.txt'

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

# Schreibt jedes Element einer Sammlung nach Write-Host, ohne dass
# es in den Erfolgsstrom der aufrufenden Funktion geraet.
# 'foreach' ist ein Sprachkonstrukt, kein Cmdlet -- sein Output wird
# NICHT Teil des Funktionsrueckgabewerts.
function Write-LinesToHost {
    param($Lines)
    if ($null -eq $Lines) { return }
    foreach ($line in $Lines) {
        Write-Host $line
    }
}

# Fuehrt einen externen Befehl aus und liefert NUR den Exit-Code zurueck.
# Der komplette Output (stdout + stderr) wird VORHER auf den Host
# geschrieben. $ErrorActionPreference wird temporaer auf 'Continue'
# gesetzt, damit native stderr-Zeilen nicht als Ausnahmen behandelt
# werden und dadurch nachfolgende Zeilen verschluckt werden.
function Invoke-NativeCommand {
    param(
        [Parameter(Mandatory=$true)][string]$FilePath,
        [string[]]$Arguments = @(),
        [string]$WorkingDirectory = $null
    )

    $prevEA = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'

    $exitCode = 0
    $captured = $null

    try {
        if ($WorkingDirectory) {
            Push-Location -LiteralPath $WorkingDirectory
        }
        try {
            $captured = & $FilePath @Arguments 2>&1
            $exitCode = $LASTEXITCODE
        }
        finally {
            if ($WorkingDirectory) { Pop-Location }
        }
    }
    catch {
        Write-Err "Ausnahme bei '$FilePath': $_"
        $exitCode = 99
    }
    finally {
        $ErrorActionPreference = $prevEA
    }

    Write-LinesToHost -Lines $captured
    return [int]$exitCode
}

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
    web       Generiert die Web-Docs (ProWB, manifest-getrieben).
    all       Fuehrt build -> test -> export aus (ohne web).
    help      Diese Uebersicht (oder 'pro_run help <aktion>').

  BEISPIELE
    pro_run all
    pro_run build -Config debug -Rebuild
    pro_run test -Prio 1-4
    pro_run test -Test Running-Coupling -LogDir C:\logs
    pro_run export -Export kit -OutDir D:\sdk-kit
    pro_run web -Rebuild
    pro_run all -NoBuild -NoExport -Prio 1

  Weiter: 'pro_run help build', 'help export', 'help test', 'help web'
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
        'web' {
            Write-Head 'pro_run web'
            Write-Host @"
  Aufruf:  pro_run web [optionen]

  Baut (falls noetig) den ProWB-Builder und generiert aus
  docs\web\manifest.txt die Web-Dokumentation.

  ABLAUF
    1. prowb.exe bauen (nmake, build\prowb)
    2. prowb.exe --manifest <src> <manifest> <out> aufrufen
    3. out\web\index.html validieren (existiert, > 0 Bytes)

  OPTIONEN
    -Config <release|debug>       (Default: release)
                                  debug -> bin\prowb_debug\prowb.exe
                                  release -> bin\prowb\prowb.exe
    -Rebuild                      Builder neu bauen (clean + all)
    -WebOutDir <pfad>             (Default: <repo>\out\web)
    -DryRun                       nur auflisten

  QUELLEN (fix, projektweit)
    docs\web\src\           Templates, CSS, JS, Views
    docs\web\manifest.txt   Quellpfad|Sektion|Doc-ID
    docs\web\config\        emoji.txt, html_whitelist.txt, crosslinks.txt

  BEISPIELE
    pro_run web
    pro_run web -Rebuild
    pro_run web -Config debug
    pro_run web -WebOutDir out\web-local
"@
        }
        'all' {
            Write-Head 'pro_run all'
            Write-Host @"
  Aufruf:  pro_run all [optionen]

  Fuehrt build -> test -> export in dieser Reihenfolge aus.
  Fehlschlag eines Schritts bricht ab.
  Hinweis: 'web' ist NICHT Teil von 'all'. Separat aufrufen:
           pro_run web -Rebuild

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
            Write-Host "  Moeglich: build, export, test, web, all"
        }
    }
}

# ==========================================================================
# Subskript-Aufruf
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

    $exitCode = Invoke-NativeCommand -FilePath 'powershell.exe' -Arguments $psArgs

    if ($exitCode -ne 0) {
        Write-Err "$Label fehlgeschlagen (Exit $exitCode)"
    } else {
        Write-Ok "$Label abgeschlossen."
    }
    return [int]$exitCode
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
# ProWB-spezifische Helfer
# ==========================================================================

function Get-ProwbExePath {
    $sub = if ($Config -eq 'debug') { 'prowb_debug' } else { 'prowb' }
    return (Join-Path $RepoRoot "bin\$sub\prowb.exe")
}

# Ruft nmake im build\prowb-Verzeichnis auf.
# Rueckgabe: NUR der Exit-Code (int).
function Invoke-ProwbNMake {
    param([string[]]$Targets = @('all'))

    if (-not (Test-Path -LiteralPath $BuildProwb)) {
        Write-Err "Makefile-Verzeichnis fehlt: $BuildProwb"
        return 2
    }

    $nmakeArgs = @('/nologo', '/f', 'Makefile.nmake') + $Targets + @("CONFIG=$Config")

    if ($script:DryRun) {
        Write-Dry "nmake $($nmakeArgs -join ' ')   (in $BuildProwb)"
        return 0
    }

    $exitCode = Invoke-NativeCommand `
        -FilePath 'nmake.exe' `
        -Arguments $nmakeArgs `
        -WorkingDirectory $BuildProwb

    return [int]$exitCode
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

function Do-Web {
    # --- Zielverzeichnis bestimmen -------------------------------------
    $outDir = if ($WebOutDir) {
        if ([System.IO.Path]::IsPathRooted($WebOutDir)) { $WebOutDir }
        else { Join-Path $RepoRoot $WebOutDir }
    } else {
        Join-Path $RepoRoot 'out\web'
    }

    # --- 1. Vorbedingungen pruefen ------------------------------------
    if (-not (Test-Path -LiteralPath $ProwbManifest)) {
        Write-Err "Manifest fehlt: $ProwbManifest"
        return 3
    }
    if (-not (Test-Path -LiteralPath $ProwbSrcDir)) {
        Write-Err "Web-Src fehlt: $ProwbSrcDir"
        return 3
    }
    if (-not (Test-Path -LiteralPath $BuildProwb)) {
        Write-Err "Build-Verzeichnis fehlt: $BuildProwb"
        return 3
    }

    # --- 2. Builder bauen (falls noetig) ------------------------------
    $prowbExe = Get-ProwbExePath

    if ($Rebuild) {
        Write-Step "ProWB-Builder: clean + rebuild (CONFIG=$Config)"

        $rc = Invoke-ProwbNMake -Targets @('clean')
        if ($rc -ne 0) {
            Write-Err "nmake clean fehlgeschlagen (Exit $rc)"
            return [int]$rc
        }

        $rc = Invoke-ProwbNMake -Targets @('all')
        if ($rc -ne 0) {
            Write-Err "nmake all fehlgeschlagen (Exit $rc)"
            return [int]$rc
        }
        Write-Ok "Builder neu gebaut: $prowbExe"
    }
    elseif (-not (Test-Path -LiteralPath $prowbExe)) {
        Write-Step "ProWB-Builder fehlt -- wird gebaut (CONFIG=$Config)"
        $rc = Invoke-ProwbNMake -Targets @('all')
        if ($rc -ne 0) {
            Write-Err "nmake all fehlgeschlagen (Exit $rc)"
            return [int]$rc
        }
        Write-Ok "Builder gebaut: $prowbExe"
    }
    else {
        Write-Step "ProWB-Builder vorhanden: $prowbExe"
    }

    if ($script:DryRun) {
        Write-Dry "& `"$prowbExe`" --manifest `"$ProwbSrcDir`" `"$ProwbManifest`" `"$outDir`""
        return 0
    }

    if (-not (Test-Path -LiteralPath $prowbExe)) {
        Write-Err "prowb.exe nicht gefunden nach Build: $prowbExe"
        return 4
    }

    # --- 3. Ausgabeverzeichnis vorbereiten ----------------------------
    if (-not (Test-Path -LiteralPath $outDir)) {
        New-Item -ItemType Directory -Path $outDir -Force | Out-Null
    }

    # --- 4. Builder aufrufen ------------------------------------------
    Write-Step "ProWB-Build"
    Write-Host "        src:      $ProwbSrcDir"   -ForegroundColor DarkGray
    Write-Host "        manifest: $ProwbManifest" -ForegroundColor DarkGray
    Write-Host "        out:      $outDir"        -ForegroundColor DarkGray

    $rc = Invoke-NativeCommand `
        -FilePath $prowbExe `
        -Arguments @('--manifest', $ProwbSrcDir, $ProwbManifest, $outDir) `
        -WorkingDirectory $RepoRoot

    if ($rc -ne 0) {
        Write-Err "ProWB-Build fehlgeschlagen (Exit $rc)"
        return [int]$rc
    }

    # --- 5. Validierung ----------------------------------------------
    $index = Join-Path $outDir 'index.html'
    if (-not (Test-Path -LiteralPath $index)) {
        Write-Err "index.html wurde nicht erzeugt: $index"
        return 5
    }
    $size = (Get-Item -LiteralPath $index).Length
    if ($size -le 0) {
        Write-Err "index.html ist leer: $index"
        return 5
    }

    Write-Ok ("Web-Docs erzeugt: {0}  ({1:N0} Bytes)" -f $index, $size)
    Write-Host "        Oeffnen: start `"$index`"" -ForegroundColor DarkGray
    return 0
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
            return [int]$rc
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
            return [int]$rc
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
    'web'    { $rc = Do-Web    }
    'all'    { $rc = Do-All    }
}

# rc defensiv auf int normalisieren -- falls irgendwo doch ein Array
# durchkommt, verwenden wir das LETZTE Element (das ist der Exit-Code).
if ($rc -is [array]) {
    $rc = [int]($rc[-1])
} else {
    $rc = [int]$rc
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