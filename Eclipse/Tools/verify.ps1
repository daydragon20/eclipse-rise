# DE HELE CONTROLE IN EEN COMMANDO: bouwen, de suite, en de opnameronde.
#
# Waarom dit bestaat (owner-opdracht 26-07): "deze ronde draait met ELKE landing".
# Tot vannacht waren dat vier losse stappen die ik met de hand aan elkaar knoopte,
# en zo wordt een stap stilletjes overgeslagen — precies de vorm van falen die de
# hele dag is opgeruimd. Een controle die je moet onthouden, is geen controle.
#
# De ronde levert PLAATJES op en geen oordeel. Het oordeel is mensenwerk: open de
# beelden en schrijf op wat je ziet. Deze twee vondsten stonden op geen enkele
# meter en alleen op een frame:
#   - aankleedfiguren van 328 cm naast een speler van 190
#   - een gele engine-waarschuwing die over het scherm liep
#
# Draaien:
#   powershell -File Eclipse\Tools\verify.ps1
#   powershell -File Eclipse\Tools\verify.ps1 -SkipShots     (alleen bouwen + suite)

param(
    [switch]$SkipShots,
    [string]$Engine = "C:\Program Files\Epic Games\UE_5.8\Engine"
)

$ErrorActionPreference = "Stop"
$Project = "C:\Dev\ECLIPSE_GDD\Eclipse\Eclipse.uproject"
$Root = "C:\Dev\ECLIPSE_GDD\Eclipse"
$Failures = @()
$script:Counts = "niet gedraaid"
$script:ShotCount = 0

function Write-Step($Text) { Write-Host ""; Write-Host "=== $Text ===" }

# ---------------------------------------------------------------- 1. bouwen
# -NoUba staat er omdat de owner geen adminrechten heeft; zonder die vlag valt de
# build terug op een pad dat hier niet beschikbaar is.
Write-Step "BOUWEN"
& "$Engine\Build\BatchFiles\Build.bat" EclipseEditor Win64 Development -project="$Project" -NoUba -WaitMutex |
    Select-Object -Last 3
if ($LASTEXITCODE -ne 0) {
    Write-Host "BUILD GEFAALD — de rest heeft geen zin op oude binaries."
    exit 1
}

# ------------------------------------------------------------------ 2. suite
Write-Step "SUITE"
$Report = "$Root\Saved\TestReport"
$exe = "$Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$p = Start-Process -FilePath $exe -PassThru -NoNewWindow -ArgumentList `
    "`"$Project`"", '-ExecCmds="Automation RunTests Eclipse; Quit"', '-unattended', `
    '-nopause', '-nullrhi', '-NoLiveCoding', '-testexit="Automation Test Queue Empty"', `
    '-log', "-ReportExportPath=$Report"
if (-not $p.WaitForExit(1800000)) { $p.Kill(); throw "suite liep vast na 30 min" }

$j = Get-Content "$Report\index.json" -Raw | ConvertFrom-Json
# succeeded telt alleen de SCHONE tests; wie die optelt bij failed komt structureel
# te laag uit en denkt dat de suite gekrompen is. Het totaal is tests.Count.
$Total = $j.tests.Count
$script:Counts = "$Total tests / $($j.failed) gefaald / $($j.notRun) niet gedraaid"
Write-Host "$Total tests — $($j.succeeded) schoon, $($j.succeededWithWarnings) met waarschuwing, $($j.failed) GEFAALD, $($j.notRun) niet gedraaid"
if ($j.failed -gt 0 -or $j.notRun -gt 0) {
    $j.tests | Where-Object { $_.state -ne 'Success' } | ForEach-Object {
        Write-Host "  FAAL: $($_.fullTestPath)"
        $Failures += $_.fullTestPath
        $_.entries | Where-Object { $_.event.type -eq 'Error' } | ForEach-Object { Write-Host "     $($_.event.message)" }
    }
}

# ------------------------------------------------- 3. validatie + catalogus
# De owner-regel voor de groene bar is breder dan build+suite: "EclipseValidateData
# 0 fouten + catalog gedocumenteerd = geimplementeerd". Die twee stonden hier
# eerst NIET in, en ik heb ze een hele nacht met de hand bij elkaar gezocht - de
# exacte fout die dit script moest wegnemen. Een controle die je moet onthouden,
# is geen controle.
Write-Step "VALIDATIE"
$p = Start-Process -FilePath $exe -PassThru -NoNewWindow -ArgumentList `
    "`"$Project`"", '-run=EclipseValidateData', '-unattended', '-nullrhi', '-NoLiveCoding'
if (-not $p.WaitForExit(600000)) { $p.Kill(); throw "validatie liep vast" }
$Validation = Select-String -Path "$Root\Saved\Logs\Eclipse.log" -Pattern "ValidateData: .* errors\." |
    Select-Object -Last 1
if ($Validation) {
    Write-Host ($Validation.Line -replace '^.*Display: ', '')
    if ($Validation.Line -notmatch ', 0 errors\.') { $Failures += "validatie meldt fouten" }
} else {
    Write-Host "GEEN VALIDATIEREGEL gevonden - draaide de commandlet wel?"
    $Failures += "validatie gaf geen uitslag"
}

Write-Step "CATALOGUS"
python "$Root\Tools\check_event_catalog.py"
if ($LASTEXITCODE -ne 0) { $Failures += "event-catalogus klopt niet" }

# ---------------------------------------------------------- 4. de opnameronde
if (-not $SkipShots) {
    Write-Step "OPNAMERONDE"
    # De mapnaam voluit, want die kostte een halve avond: een verkeerde naam laat
    # de run gewoon starten en stil afsluiten, en het bewijs staat in het LOG en
    # niet in de screenshotmap.
    $Start = Get-Date
    $p = Start-Process -FilePath $exe -PassThru -ArgumentList `
        "`"$Project`"", '/Game/Maps/GrayboxDistrict', '-game', '-windowed', `
        '-resx=1280', '-resy=720', '-nosplash', '-NoLiveCoding', '-EclipseShotPlay', `
        '-EclipseStartMission=TransitCheckpoint'
    if (-not $p.WaitForExit(300000)) { $p.Kill(); Write-Host "opnameronde liep vast" }

    $Shots = Get-ChildItem -Recurse "$Root\Saved\Screenshots" -Filter *.png -ErrorAction SilentlyContinue |
        Where-Object { $_.LastWriteTime -gt $Start } | Sort-Object LastWriteTime
    $script:ShotCount = $Shots.Count
    Write-Host "$($Shots.Count) opnames:"
    $Shots | ForEach-Object { Write-Host "  $($_.FullName)" }
    if ($Shots.Count -eq 0) {
        Write-Host "GEEN BEELD — kijk in Saved\Logs\Eclipse.log, niet in de screenshotmap."
        $Failures += "opnameronde leverde niets op"
    }

    # Alleen de SPELERregel per moment. De volledige meting is acht regels per
    # opname en dan leest niemand hem meer — en een rapport dat niemand leest is
    # precies wat deze hele laag moest vervangen. De rest staat in het log.
    # De ronde oordeelt nu ook zelf waar dat kan: wordt de speler getekend, vult
    # hij een echt stuk beeld, staat hij in het frame. Die controles meten in
    # SCHERMruimte, en dat is precies het verschil dat de suite niet kan zien.
    # Ook de UI-dump kan fouten melden. Die staan niet in het PLAYSHOT-formaat
    # omdat ze uit de HUD zelf komen, en een scan die alleen het ene patroon kent
    # zou ze stil laten passeren.
    $Wrong = Select-String -Path "$Root\Saved\Logs\Eclipse.log" -Pattern "PLAYSHOT \d+ FOUT\]|UI: FOUT"
    if ($Wrong) {
        Write-Host ""
        Write-Host "DE OPNAMERONDE VOND FOUTEN:"
        $Wrong | ForEach-Object { Write-Host "  $(($_.Line -replace '^.*Error: ', ''))" }
        $Failures += "opnameronde: $($Wrong.Count) fout(en) in het frame"
    }

    # DE BUDGETBAND. GDD 12.4 zegt 16,7 ms; de harde fout staat pas op 33,3.
    # Daartussen zat niets, en een waarschuwing die niemand leest is precies zo
    # nuttig als geen waarschuwing. Hij maakt de bar NIET rood - dit is een
    # graybox in een editor-build, dus hard falen op het shipping-budget zou rood
    # gaan op werk dat klopt - maar hij komt wel bovenaan te staan.
    $Budget = Select-String -Path "$Root\Saved\Logs\Eclipse.log" -Pattern "BUDGET OVERSCHREDEN"
    if ($Budget) {
        Write-Host ""
        Write-Host "BOVEN HET 12.4-BUDGET (nog geen fout, wel de kant op):"
        $Budget | ForEach-Object { Write-Host "  $(($_.Line -replace '^.*Warning: ', ''))" }
    }

    Write-Host ""
    Select-String -Path "$Root\Saved\Logs\Eclipse.log" -Pattern "SPELER|PLAYSHOT \d+ (WAPEN|DRAAI|SILHOUET|BEWEGING)\]" |
        ForEach-Object { ($_.Line -replace '^.*Display: ', '') }
}

# --------------------------------------------------------- 5. het soak-logboek
# SPEC-P2-05 vraagt een soak die DRIE NACHTEN groen blijft, en SPEC-P2-03 vraagt
# hetzelfde van zijn econ-paden. Dat is niet te bewijzen zonder geheugen: er is
# geen CI-runner (Fase 0-restje), dus tot nu bestond "drie nachten" alleen in het
# hoofd van wie het draaide.
#
# HIJ SCHRIJFT OOK BIJ ROOD, en dat is de hele truc. Een logboek dat alleen
# successen bewaart kan "drie nachten achtereen" per definitie niet aantonen -
# dan zie je drie groene regels en niet de rode nacht ertussen. Wie de reeks
# leest moet de onderbrekingen kunnen zien.
#
# Geen BOM in het logbestand: dit is een repo-document dat door mensen en door
# git gelezen wordt, en een BOM belandt anders in de eerste tabelcel.
$LogPath = "C:\Dev\ECLIPSE_GDD\phase0\SOAK_LOG.md"
$Utf8NoBom = New-Object System.Text.UTF8Encoding $false
if (-not (Test-Path $LogPath)) {
    $Header = @(
        "# Soak-logboek",
        "",
        "Elke draai van ``Eclipse\Tools\verify.ps1`` schrijft hier een regel — GROEN en ROOD.",
        "Zonder de rode nachten is ``drie nachten achtereen`` niet te bewijzen.",
        "",
        "| Wanneer | Commit | Uitslag | Suite | Opnames | Waarop het viel |",
        "|---|---|---|---|---|---|",
        ""
    ) -join "`r`n"
    [System.IO.File]::WriteAllText($LogPath, $Header, $Utf8NoBom)
}
$Commit = (& git -C "C:\Dev\ECLIPSE_GDD" rev-parse --short HEAD 2>$null)
if (-not $Commit) { $Commit = "onbekend" }
$Verdict = if ($Failures.Count -gt 0) { "ROOD" } else { "GROEN" }
$Reason = if ($Failures.Count -gt 0) { ($Failures -join "; ") } else { "-" }
$Row = "| {0} | ``{1}`` | **{2}** | {3} | {4} | {5} |`r`n" -f `
    (Get-Date -Format "yyyy-MM-dd HH:mm"), $Commit, $Verdict, $script:Counts, $script:ShotCount, $Reason
[System.IO.File]::AppendAllText($LogPath, $Row, $Utf8NoBom)

Write-Step "STAND"
if ($Failures.Count -gt 0) {
    Write-Host "NIET GROEN — $($Failures.Count) punt(en):"
    $Failures | ForEach-Object { Write-Host "  $_" }
    exit 1
}
Write-Host "Suite groen. NU ZELF NAAR DE BEELDEN KIJKEN en opschrijven wat je ziet —"
Write-Host "een groene bar bewijst niet dat er iets op het scherm staat."
