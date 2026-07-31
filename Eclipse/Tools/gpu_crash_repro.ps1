# DE REPRODUCTIE VOOR HET GPU-CRASHDOSSIER (phase0/DEBUG_DISCIPLINE.md 4.5).
#
# Waarom dit bestaat. De crash is GRILLIG: 1 op 9 opnamerondes. Bij ~11% kans is
# elke uitspraak op basis van EEN ronde waardeloos -- 89% van de rondes gaat
# vanzelf goed. Er is dus geen enkele manier om een fix te bewijzen zolang er
# geen harnas is dat N rondes draait en de uitkomst PER RONDE opschrijft.
#
# Dit script doet precies dat en niets meer. Het repareert niets.
#
#   powershell -File Eclipse\Tools\gpu_crash_repro.ps1 -Runs 10 -Churn 0 -Tag nul
#   powershell -File Eclipse\Tools\gpu_crash_repro.ps1 -Runs 10 -Churn 8 -Tag churn8
#
# -Churn <N> geeft de ronde `-EclipseSkyChurn=N` mee: N keer per spelframe de
# SkyAtmosphere slopen en herbouwen via exact het pad van de bouwer. -Churn 0 is
# de nulmeting: dezelfde binary, dezelfde aanroep, alleen zonder die vlag. Dat is
# de enige eerlijke vergelijking -- de historische 1-op-9 komt van een ANDERE
# binary en telt hier hooguit als voorkennis.
#
# Elke ronde krijgt zijn eigen logbestand via -abslog. Zonder dat schrijft een
# tweede tegelijk draaiende instantie naar Eclipse_2.log en meet dit harnas het
# verkeerde bestand -- er draaien meer agents op deze werkboom.

param(
    [int]$Runs = 10,
    [int]$Churn = 0,
    [string]$Tag = "run",
    [string]$Engine = "C:\Program Files\Epic Games\UE_5.8\Engine",
    [int]$TimeoutSec = 330
)

$ErrorActionPreference = "Stop"
$Project = "C:\Dev\ECLIPSE_GDD\Eclipse\Eclipse.uproject"
$Root    = "C:\Dev\ECLIPSE_GDD\Eclipse"
$Exe     = "$Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$OutDir  = "$Root\Saved\CrashRepro\$Tag"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$Crashes = 0
$Rows = @()

Write-Host "=== $Runs rondes, churn=$Churn, tag=$Tag ==="

for ($i = 1; $i -le $Runs; $i++) {
    $LogPath = "$OutDir\$Tag`_$('{0:d2}' -f $i).log"
    if (Test-Path $LogPath) { Remove-Item $LogPath -Force }

    # NIET $Args: dat is de automatische argumentenlijst van het script zelf.
    $RunArgs = @(
        "`"$Project`"", '/Game/Maps/GrayboxDistrict', '-game', '-windowed',
        '-resx=1280', '-resy=720', '-nosplash', '-NoLiveCoding', '-EclipseShotPlay',
        '-EclipseStartMission=TransitCheckpoint', "-abslog=`"$LogPath`""
    )
    if ($Churn -gt 0) { $RunArgs += "-EclipseSkyChurn=$Churn" }

    $Start = Get-Date
    $p = Start-Process -FilePath $Exe -PassThru -ArgumentList $RunArgs
    $Exited = $p.WaitForExit($TimeoutSec * 1000)
    if (-not $Exited) { try { $p.Kill() } catch {} ; Start-Sleep -Milliseconds 500 }
    $Secs = [math]::Round(((Get-Date) - $Start).TotalSeconds, 1)

    # De crashmelder houdt het proces anders vast en de VOLGENDE ronde start dan
    # naast een dialoogvenster. Een harnas dat na de eerste crash vastloopt meet
    # per constructie hoogstens een crash.
    #
    # ALLEEN CrashReportClient, nadrukkelijk NIET elke UnrealEditor-Cmd: er draaien
    # meer agents op deze werkboom en hun testsuite draait in precies zo'n proces.
    # Een opruimregel die andermans werk omlegt is erger dan de hang die hij voorkomt.
    Get-Process CrashReportClient* -ErrorAction SilentlyContinue | ForEach-Object {
        try { if ($_.StartTime -gt $Start) { $_.Kill() } } catch {}
    }

    Start-Sleep -Milliseconds 800

    $Crash = "-"; $Frames = 0; $Rebuilds = 0; $ChurnFrames = 0; $Shots = 0; $Pass = "?"
    if (Test-Path $LogPath) {
        $Text = Get-Content $LogPath -Raw
        # Twee onafhankelijke handtekeningen: de RHI-melding en de reden van DXGI.
        # Een van de twee kan ontbreken als het log halverwege afkapt.
        if ($Text -match 'GPU crash detected' -or $Text -match 'DXGI_ERROR_DEVICE_(HUNG|REMOVED)') {
            $Crash = "GPU"; $Crashes++
        }
        elseif ($Text -match '(?m)^\[.*\]\[.*\]LogWindows: Error: (Fatal error|begin: stack for UAT)') {
            $Crash = "FATAL"; $Crashes++
        }
        # Het hoogste framenummer in de logstempels: de NOEMER. Een ronde die vroeg
        # sterft heeft minder frames blootgesteld, en dan is "kans per ronde"
        # misleidend zonder "kans per frame" ernaast.
        $Fm = [regex]::Matches($Text, '(?m)^\[[\d\.\-\:]+\]\[\s*(\d+)\]')
        if ($Fm.Count -gt 0) { $Frames = ($Fm | ForEach-Object { [int]$_.Groups[1].Value } | Measure-Object -Maximum).Maximum }
        $Shots = ([regex]::Matches($Text, '\[PLAYSHOT \d+')).Count
        $Cm = [regex]::Matches($Text, '\[SKYCHURN\] (\d+) spelframes, (\d+) herbouwen')
        if ($Cm.Count -gt 0) {
            $ChurnFrames = [int]$Cm[$Cm.Count-1].Groups[1].Value
            $Rebuilds    = [int]$Cm[$Cm.Count-1].Groups[2].Value
        }
        $Em = [regex]::Match($Text, '\[SKYCHURN EINDE\] (\d+) spelframes gechurnd, (\d+) herbouwen')
        if ($Em.Success) {
            $ChurnFrames = [int]$Em.Groups[1].Value
            $Rebuilds    = [int]$Em.Groups[2].Value
            $Pass = "eind"
        }
        elseif ($Text -match 'LogExit: Exiting\.') { $Pass = "eind" }
        else { $Pass = "afgebroken" }
    }
    else { $Crash = "GEEN LOG" }

    # EEN RONDE DIE NOOIT GERENDERD HEEFT TELT NIET ALS SCHONE RONDE.
    #
    # Gevonden bij de allereerste proefronde van dit harnas: de module-DLL was
    # verouderd, de game sloot na 18 s met "Incompatible or missing module:
    # Eclipse", en het harnas schreef er doodleuk "crash=-, eind" bij. Zo bouw je
    # een teller die 0 crashes op 10 rondes meldt zonder dat er ooit een frame
    # getekend is -- precies de vorm van bewijs die dit hele dossier moet
    # vermijden. De noemer is FRAMES, niet processtarts.
    if ($Crash -eq "-" -and ($Frames -lt 30 -or $Shots -lt 1)) {
        $Crash = "ONGELDIG"
        Write-Host "    ^ geen beeld: frames=$Frames shots=$Shots -- deze ronde meet niets, zie $LogPath"
    }

    $Row = [pscustomobject]@{
        Ronde = $i; Crash = $Crash; Sec = $Secs; Frames = $Frames
        Shots = $Shots; ChurnFrames = $ChurnFrames; Herbouwen = $Rebuilds; Einde = $Pass
    }
    $Rows += $Row
    Write-Host ("  ronde {0,2}: crash={1,-6} {2,6}s frames={3,-5} shots={4,-4} churnframes={5,-5} herbouwen={6,-6} {7}" -f `
        $i, $Crash, $Secs, $Frames, $Shots, $ChurnFrames, $Rebuilds, $Pass)
}

$Invalid     = ($Rows | Where-Object { $_.Crash -eq "ONGELDIG" -or $_.Crash -eq "GEEN LOG" }).Count
$Valid       = $Runs - $Invalid
$TotalFrames = ($Rows | Measure-Object -Property Frames -Sum).Sum
$TotalReb    = ($Rows | Measure-Object -Property Herbouwen -Sum).Sum
Write-Host ""
$Pct = if ($Valid -gt 0) { [math]::Round(100.0 * $Crashes / $Valid, 1) } else { "n.v.t." }
Write-Host ("UITSLAG $Tag (churn=$Churn): {0} crashes op {1} GELDIGE rondes = {2}%   ({3} ongeldig van {4} gestart)" -f `
    $Crashes, $Valid, $Pct, $Invalid, $Runs)
Write-Host ("  frames totaal: $TotalFrames   sky-herbouwen totaal: $TotalReb")
if ($Invalid -gt 0) { Write-Host "  LET OP: ongeldige rondes tellen NIET mee als schone ronde." }
$Rows | Export-Csv -Path "$OutDir\uitslag.csv" -NoTypeInformation -Encoding UTF8
Write-Host "  per ronde: $OutDir\uitslag.csv"
