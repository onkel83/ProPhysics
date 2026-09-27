# ==========================================================================
# ProPhysics - BUILD_INFO.txt Generator
# File: build\main\write_build_info.ps1
# Version: 3.2 (Etappe 23)
#
# Erzeugt <RepoRoot>\BUILD_INFO.txt mit:
#   - Zeitstempel, Version + Etappe (aus ProPhysics_Version.h), Host/User
#   - Config (release/debug) bei -Config
#   - optional Git-Info (describe/branch/sha/status) bei -GitStamp
#   - optional Freitext-Notiz bei -GitNote
#   - Liste der Artefakte in bin\ und lib\
#
# Aufruf (direkt oder via Master-Makefile / build.ps1):
#   powershell -NoProfile -ExecutionPolicy Bypass `
#       -File write_build_info.ps1 `
#       -RepoRoot "H:\...\ProPhysics" `
#       -Config release `
#       -GitStamp `
#       -GitNote "Etappe 23 abgeschlossen"
#
# Self-Locating: -RepoRoot Default = $PSScriptRoot\..\..
#
# Ohne git oder ohne -GitStamp laeuft das Skript ohne Git-Block durch.
# ==========================================================================

[CmdletBinding()]
param(
    [string]$RepoRoot,

    [ValidateSet('release','debug')]
    [string]$Config = '',

    [string]$Version = '',
    [string]$OutFile = '',

    [switch]$GitStamp,
    [string]$GitNote = ''
)

$ErrorActionPreference = 'Stop'

# --------------------------------------------------------------------------
# Konsolenausgabe auf UTF-8
# --------------------------------------------------------------------------
try {
    [Console]::OutputEncoding = [System.Text.Encoding]::UTF8
    $OutputEncoding           = [System.Text.Encoding]::UTF8
} catch { }

# --------------------------------------------------------------------------
# RepoRoot: Default aus Skript-Ablageort
#
# Skript liegt in <repo>\build\main\. RepoRoot ist zwei Ebenen hoeher.
# --------------------------------------------------------------------------
if (-not $RepoRoot) {
    $RepoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
}
else {
    $RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
}

# --------------------------------------------------------------------------
# Pfade
# --------------------------------------------------------------------------
$binDir  = Join-Path $RepoRoot 'bin'
$libDir  = Join-Path $RepoRoot 'lib'
if (-not $OutFile) { $OutFile = Join-Path $RepoRoot 'BUILD_INFO.txt' }
$vhPath  = Join-Path $RepoRoot 'src\prophysics\header\ProPhysics_Version.h'

# --------------------------------------------------------------------------
# Version + Etappe aus ProPhysics_Version.h lesen
# --------------------------------------------------------------------------
function Get-ProVersion {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) {
        return @{ Version = 'unknown'; Etappe = 'unknown' }
    }
    $txt = Get-Content -LiteralPath $Path -Raw

    $maj = if ($txt -match 'VERSION_MAJOR\s+(\d+)') { $Matches[1] } else { '?' }
    $min = if ($txt -match 'VERSION_MINOR\s+(\d+)') { $Matches[1] } else { '?' }
    $pat = if ($txt -match 'VERSION_PATCH\s+(\d+)') { $Matches[1] } else { '?' }

    $etappe = if ($txt -match 'PROPHYSICS_ETAPPE\s+(\d+)') { $Matches[1] } else { '?' }

    return @{
        Version = "$maj.$min.$pat"
        Etappe  = $etappe
    }
}

# --------------------------------------------------------------------------
# Git-Info sammeln (nur bei -GitStamp)
# --------------------------------------------------------------------------
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

# --------------------------------------------------------------------------
# Artefakte auflisten
# --------------------------------------------------------------------------
function Get-ArtifactLines {
    param(
        [string]$Dir,
        [string]$Label,
        [string[]]$Filters
    )
    $lines = New-Object System.Collections.Generic.List[string]
    if (-not (Test-Path -LiteralPath $Dir)) {
        $lines.Add("  $Label  (Verzeichnis fehlt)")
        return $lines
    }
    $files = @()
    foreach ($f in $Filters) {
        $files += Get-ChildItem -LiteralPath $Dir -Filter $f -File -ErrorAction SilentlyContinue
    }
    $files = $files | Sort-Object Name -Unique

    if ($files.Count -eq 0) {
        $lines.Add("  $Label  (leer)")
        return $lines
    }
    $lines.Add("  $Label")
    foreach ($f in $files) {
        $kb = [math]::Round($f.Length / 1KB, 1)
        $lines.Add(("    {0,-32} {1,10} KB" -f $f.Name, $kb))
    }
    return $lines
}

# --------------------------------------------------------------------------
# Inhalt aufbauen
# --------------------------------------------------------------------------
$vinfo   = Get-ProVersion -Path $vhPath
$version = if ($Version) { $Version } else { $vinfo.Version }
$etappe  = $vinfo.Etappe
$now     = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'

$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("ProPhysics Build Information")
$lines.Add("============================")
$lines.Add("")
$lines.Add("Erzeugt:  $now")
$lines.Add("Host:     $env:COMPUTERNAME")
$lines.Add("User:     $env:USERNAME")
$lines.Add("Version:  $version")
$lines.Add("Etappe:   $etappe")
if ($Config) {
    $lines.Add("Config:   $Config")
}
$lines.Add("")

# --- Git-Block (optional) ---
if ($GitStamp) {
    $git = Get-GitInfo
    if ($git) {
        $lines.Add("Git:")
        $lines.Add("  describe: $($git.Describe)")
        $lines.Add("  branch:   $($git.Branch)")
        $lines.Add("  sha:      $($git.Sha)")
        $lines.Add("  status:   $($git.Dirty)")
    } else {
        $lines.Add("Git:      nicht verfuegbar (kein Repo oder git fehlt)")
    }
    $lines.Add("")
}

# --- Notiz-Block (optional, unabhaengig von -GitStamp) ---
if ($GitNote -and $GitNote.Trim().Length -gt 0) {
    $lines.Add("Notiz:")
    foreach ($nline in ($GitNote -split "`r?`n")) {
        $lines.Add("  $nline")
    }
    $lines.Add("")
}

# --- Artefakte ---
$lines.Add("Artefakte:")
$lines.Add("")

foreach ($l in (Get-ArtifactLines -Dir $binDir -Label "bin\" -Filters @('*.dll','*.exe','*.cmd','*.ps1'))) {
    $lines.Add($l)
}
$lines.Add("")
foreach ($l in (Get-ArtifactLines -Dir $libDir -Label "lib\" -Filters @('*.lib'))) {
    $lines.Add($l)
}

# --- Quellen-Info ---
$lines.Add("")
$lines.Add("Quellen:")
$lines.Add("  src\prophysics\   Kernel (12 Module)")
$lines.Add("  src\sdk\          SDK Interface (1 Modul)")
$lines.Add("  src\test\         Alpha-Test (17 Module) + 2 Example-Tests")
$lines.Add("")
$lines.Add("Hinweis:")
$lines.Add("  Diese Datei wird bei jedem erfolgreichen Build neu erzeugt.")
$lines.Add("  Inhalt und Format sind dokumentiert in docs\build\BUILD_SCRIPT.md.")

# --------------------------------------------------------------------------
# Schreiben
# --------------------------------------------------------------------------
$outDir = Split-Path $OutFile -Parent
if ($outDir -and -not (Test-Path -LiteralPath $outDir)) {
    New-Item -ItemType Directory -Path $outDir -Force | Out-Null
}
if (Test-Path -LiteralPath $OutFile) {
    Remove-Item -LiteralPath $OutFile -Force
}
$lines -join [Environment]::NewLine |
    Out-File -LiteralPath $OutFile -Encoding utf8

Write-Host "    OK  $OutFile"