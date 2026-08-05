<#
.SYNOPSIS
    Runs the ECLIPSE verification ladder (G1-G4) in order, stopping at the first failure.

.DESCRIPTION
    Gates run cheapest-and-most-decisive first. A failed build makes every downstream
    result meaningless, so the ladder halts rather than burning minutes producing noise.

    Emits a JSON verdict so an agent loop can read the outcome without scraping text.
    Per-gate logs land in Eclipse\Saved\GauntletLoop\.

    Windows PowerShell 5.1 dialect on purpose: no pwsh, no '&&', no ternary.
    Native executables are launched via Start-Process so exit codes stay trustworthy —
    piping a native exe's stderr inside PS 5.1 wraps output in ErrorRecords and can
    report failure on a clean exit.

.PARAMETER UeRoot
    Unreal Engine install. Defaults to $env:UE_ROOT, then C:\Program Files\Epic Games\UE_5.8.

.PARAMETER RepoRoot
    Repository root containing the Eclipse\ project folder.

.PARAMETER SkipBuild
    Skip G1. Only sound when nothing under Source\ changed since the last successful build.

.EXAMPLE
    powershell -NoProfile -File run_gates.ps1
    powershell -NoProfile -File run_gates.ps1 -SkipBuild -OutJson verdict.json
#>
[CmdletBinding()]
param(
    [string]$UeRoot,
    [string]$RepoRoot = "C:\Dev\ECLIPSE_GDD",
    [string]$OutJson,
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'

if (-not $UeRoot) {
    if ($env:UE_ROOT) { $UeRoot = $env:UE_ROOT }
    else { $UeRoot = "C:\Program Files\Epic Games\UE_5.8" }
}

$ProjectDir  = Join-Path $RepoRoot 'Eclipse'
$UProject    = Join-Path $ProjectDir 'Eclipse.uproject'
$BuildBat    = Join-Path $UeRoot 'Engine\Build\BatchFiles\Build.bat'
$EditorCmd   = Join-Path $UeRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$CatalogPy   = Join-Path $ProjectDir 'Tools\check_event_catalog.py'
$ReportDir   = Join-Path $ProjectDir 'Saved\Automation'
$LogDir      = Join-Path $ProjectDir 'Saved\GauntletLoop'

foreach ($required in @($UProject, $BuildBat, $EditorCmd)) {
    if (-not (Test-Path $required)) {
        Write-Error "Missing required path: $required"
        exit 2
    }
}
if (-not (Test-Path $LogDir)) { New-Item -ItemType Directory -Path $LogDir -Force | Out-Null }

$results = @()

function Invoke-Gate {
    <# Runs one gate. Returns a result object; never throws on gate failure. #>
    param(
        [string]$Id,
        [string]$Name,
        [string]$FilePath,
        [string[]]$Arguments
    )

    Write-Host ""
    Write-Host "== $Id  $Name ==" -ForegroundColor Cyan

    $stdout  = Join-Path $LogDir "$Id.out.log"
    $stderr  = Join-Path $LogDir "$Id.err.log"
    $started = Get-Date

    try {
        $proc = Start-Process -FilePath $FilePath -ArgumentList $Arguments `
            -NoNewWindow -Wait -PassThru `
            -RedirectStandardOutput $stdout -RedirectStandardError $stderr
        $code = $proc.ExitCode
    }
    catch {
        return [pscustomobject]@{
            id = $Id; name = $Name; passed = $false; exit_code = $null
            seconds = [math]::Round(((Get-Date) - $started).TotalSeconds, 1)
            log = $stdout; note = "Failed to launch: $($_.Exception.Message)"
        }
    }

    $seconds = [math]::Round(((Get-Date) - $started).TotalSeconds, 1)
    $passed  = ($code -eq 0)

    if ($passed) { Write-Host "   pass  ($seconds s)" -ForegroundColor Green }
    else { Write-Host "   FAIL  exit $code  ($seconds s)  ->  $stdout" -ForegroundColor Red }

    return [pscustomobject]@{
        id = $Id; name = $Name; passed = $passed; exit_code = $code
        seconds = $seconds; log = $stdout; note = $null
    }
}

function Read-AutomationReport {
    <#
        UE's automation exit code is not a dependable pass/fail signal, so prefer the
        exported report. When the report is absent we fall back to the exit code and
        say so loudly — a silent fallback would let a failing suite read as a pass.
    #>
    param([pscustomobject]$GateResult)

    $index = Join-Path $ReportDir 'index.json'
    if (-not (Test-Path $index)) {
        $GateResult.note = "No index.json at $ReportDir - verdict is from exit code only, treat as weak evidence."
        return $GateResult
    }

    try { $report = Get-Content $index -Raw | ConvertFrom-Json }
    catch {
        $GateResult.note = "index.json unreadable ($($_.Exception.Message)) - verdict is from exit code only."
        return $GateResult
    }

    $failed = @()
    if ($report.PSObject.Properties.Name -contains 'tests') {
        foreach ($t in $report.tests) {
            $state = $t.state
            if (-not $state) { $state = $t.State }
            if ($state -and $state -notmatch '^(Success|Passed)$') {
                $name = $t.fullTestPath
                if (-not $name) { $name = $t.testDisplayName }
                $failed += $name
            }
        }
    }

    if ($failed.Count -gt 0) {
        $GateResult.passed = $false
        $GateResult.note = "Failing tests: " + ($failed -join ', ')
        Write-Host "   report: $($failed.Count) failing test(s)" -ForegroundColor Red
    }
    else {
        $GateResult.note = "Automation report clean."
    }
    return $GateResult
}

# ---- G1 Build ---------------------------------------------------------------
if ($SkipBuild) {
    Write-Host "== G1  Build  (skipped by request) ==" -ForegroundColor DarkYellow
    $results += [pscustomobject]@{
        id='G1'; name='Build'; passed=$true; exit_code=$null; seconds=0
        log=$null; note='Skipped via -SkipBuild. Only valid if Source\ is unchanged.'
    }
}
else {
    $results += Invoke-Gate -Id 'G1' -Name 'Build EclipseEditor' -FilePath $BuildBat `
        -Arguments @('EclipseEditor','Win64','Development',"-Project=`"$UProject`"",'-WaitMutex','-FromMsBuild')
}

# ---- G2 Data validation -----------------------------------------------------
if ($results[-1].passed) {
    $results += Invoke-Gate -Id 'G2' -Name 'ValidateData commandlet' -FilePath $EditorCmd `
        -Arguments @("`"$UProject`"",'-run=EclipseValidateData','-unattended','-nopause','-nosplash')
}

# ---- G3 Event catalog -------------------------------------------------------
if ($results[-1].passed) {
    if (Test-Path $CatalogPy) {
        $results += Invoke-Gate -Id 'G3' -Name 'Event catalog coverage' -FilePath 'python' `
            -Arguments @("`"$CatalogPy`"")
    }
    else {
        Write-Host "== G3  Event catalog  (script missing) ==" -ForegroundColor DarkYellow
        $results += [pscustomobject]@{
            id='G3'; name='Event catalog coverage'; passed=$false; exit_code=$null; seconds=0
            log=$null; note="Not found: $CatalogPy"
        }
    }
}

# ---- G4 Automation tests ----------------------------------------------------
if ($results[-1].passed) {
    if (Test-Path $ReportDir) { Remove-Item (Join-Path $ReportDir '*') -Recurse -Force -ErrorAction SilentlyContinue }
    $g4 = Invoke-Gate -Id 'G4' -Name 'Automation suite (nullrhi)' -FilePath $EditorCmd `
        -Arguments @("`"$UProject`"",'-ExecCmds="Automation RunTests Eclipse; quit"',
                     '-unattended','-nopause','-nosplash','-nullrhi','-log',
                     "-ReportExportPath=`"$ReportDir`"")
    $results += Read-AutomationReport -GateResult $g4
}

# ---- Verdict ----------------------------------------------------------------
$ran     = $results.Count
$failedG = @($results | Where-Object { -not $_.passed })
$allPass = ($failedG.Count -eq 0)

$verdict = [pscustomobject]@{
    all_passed   = $allPass
    gates_run    = $ran
    gates_total  = 4
    first_failure= if ($failedG.Count -gt 0) { $failedG[0].id } else { $null }
    unverified   = @('G5 feel probes','G6 performance','G7 visual')
    results      = $results
}

Write-Host ""
if ($allPass) {
    Write-Host "G1-G4 pass. Note G5-G7 do not exist - feel, performance and visuals are UNVERIFIED." -ForegroundColor Green
}
else {
    Write-Host "Stopped at $($failedG[0].id): $($failedG[0].name)" -ForegroundColor Red
    if ($failedG[0].note) { Write-Host "  $($failedG[0].note)" -ForegroundColor Red }
    if ($failedG[0].log)  { Write-Host "  log: $($failedG[0].log)" -ForegroundColor Red }
}

if ($OutJson) {
    $verdict | ConvertTo-Json -Depth 6 | Out-File -FilePath $OutJson -Encoding utf8
    Write-Host "verdict -> $OutJson"
}
else {
    Write-Host ""
    $verdict | ConvertTo-Json -Depth 6
}

if ($allPass) { exit 0 } else { exit 1 }
