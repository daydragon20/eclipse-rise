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
$script:ChangedShots = ""

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
        # EN DE METINGEN VAN DIE TEST ERBIJ.
        #
        # Op 27-07 viel de contacttest om en er stond alleen "Expected ... to be
        # true". Ik heb er een uur in gestoken: test apart gedraaid, harnastijd
        # nagetrokken, de aanlooplus gelezen. Daarna bleek het rapport van die
        # ene val 122 Info-regels te bevatten met alle GEMETEN-waarden erin -
        # dichtste nadering, schade, tijdstippen. Dit script gooide ze weg omdat
        # het alleen op type 'Error' filterde.
        #
        # Het bewijs lag er dus al en het gereedschap liet het niet zien. Alleen
        # de GEMETEN-regels, alleen bij een gevallen test: de rest van de 122 is
        # ruis, en een dump die niemand leest is precies waar dit tegen is.
        $_.entries | Where-Object { $_.event.type -ne 'Error' -and $_.event.message -like '*GEMETEN*' } |
            ForEach-Object { Write-Host "       $($_.event.message)" }
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
    $script:ValidationLine = ($Validation.Line -replace '^.*Display: ', '')
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

    # WAT IS ER VERANDERD SINDS DE VORIGE RONDE?
    #
    # De owner-regel is dat er bij elke landing iemand naar de beelden kijkt. Ik
    # heb dat vannacht een keer of tien niet gedaan: de bar zei "0 frame-fouten"
    # en ik ging door. Negen beelden per ronde, vijftien rondes, en het meeste is
    # identiek aan de vorige keer - dat houdt niemand vol.
    #
    # Dus niet "kijk altijd" maar "kijk als er iets veranderd is". Dit vervangt
    # het kijken niet; het maakt de vraag goedkoop genoeg om hem elke keer te
    # stellen.
    Write-Host ""
    Write-Host "VERANDERD SINDS DE VORIGE RONDE:"
    $DiffOut = & python "$Root\Tools\shot_diff.py"
    $DiffOut | ForEach-Object { Write-Host $_ }
    # DE UITSLAG MOET HET MOMENT OVERLEVEN.
    #
    # shot_diff werkt zijn ijkbeelden ALTIJD bij, ook als er iets veranderd is -
    # anders blijft hij eeuwig hetzelfde melden. Gevolg: een verandering die ik
    # één ronde negeer, is de ronde daarna onzichtbaar. Dat is precies het gat
    # dat het soak-logboek voor de suite dicht, dus gaat het hier net zo: staat
    # er iets in de regel, dan komt het in het logboek terecht, gelezen of niet.
    $script:ChangedShots = ($DiffOut | Where-Object { $_ -match 'VERANDERD ->' }) -join ''
    if ($script:ChangedShots) {
        $script:ChangedShots = ($script:ChangedShots -replace '^shot-diff:\s*', '')
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
if ($script:ChangedShots) { $Reason = ($Reason -replace '^-$', '') + " [beeld: $script:ChangedShots]" }
$Row = "| {0} | ``{1}`` | **{2}** | {3} | {4} | {5} |`r`n" -f `
    (Get-Date -Format "yyyy-MM-dd HH:mm"), $Commit, $Verdict, $script:Counts, $script:ShotCount, $Reason
[System.IO.File]::AppendAllText($LogPath, $Row, $Utf8NoBom)

Write-Step "STAND"
if ($Failures.Count -gt 0) {
    Write-Host "NIET GROEN — $($Failures.Count) punt(en):"
    $Failures | ForEach-Object { Write-Host "  $_" }
    exit 1
}
# EEN OVERZICHT AAN HET EIND, want er hangen inmiddels zeven controles onder
# deze bar en die stonden verspreid over honderden regels engine-uitvoer. Wie
# alleen "groen" leest, weet niet WAT er groen is - en dat is precies hoe je
# gaat vertrouwen op een vinkje in plaats van op een meting.
Write-Host "Suite groen."
Write-Host ""
Write-Host "  tests        $script:Counts"
if ($script:ValidationLine) { Write-Host "  validatie    $script:ValidationLine" }
Write-Host "  opnames      $script:ShotCount"
if (-not $SkipShots) {
    Write-Host "  beelden      zie 'VERANDERD SINDS DE VORIGE RONDE' hierboven"
}
Write-Host "  logboek      phase0\SOAK_LOG.md"
Write-Host ""
Write-Host "NU ZELF NAAR DE BEELDEN KIJKEN als er iets veranderd is — en anders"
Write-Host "steekproefsgewijs. Een groene bar bewijst niet dat er iets op het scherm staat."
