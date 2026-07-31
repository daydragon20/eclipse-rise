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

# BUDGET-WAARSCHUWINGEN UIT GESLAAGDE TESTS.
#
# Gemeten op 27-07: 47 tests dragen Warning-regels en ALLE 47 hebben state
# 'Success'. De lus hierboven loopt alleen over tests die NIET Success zijn, dus
# geen enkele testwaarschuwing kwam ooit in beeld - inclusief de
# budgetwaarschuwing op de strategische tick die ik dezelfde nacht had gebouwd.
# Dat is dezelfde fout als het Error-only filter van een uur eerder: een
# waarschuwing die niemand ziet, is geen waarschuwing.
#
# NIET ALLE 47 afdrukken. Het overgrote deel is bewuste degradatie uit tests die
# expres zonder setup-asset draaien ("no DT_Facilities linked", "recruits start
# classless") plus engine-afbraakruis. Die horen daar en zijn geen signaal. Wat
# hier hoort te schreeuwen is de klasse "nog niets kapot, wel de kant op", en die
# is aan het woord BUDGET te herkennen.
$BudgetWarnings = $j.tests | ForEach-Object { $_.entries } |
    Where-Object { $_.event.type -eq 'Warning' -and $_.event.message -match 'budget' }
if ($BudgetWarnings) {
    Write-Host ""
    Write-Host "BOVEN EEN BUDGET (nog geen fout, wel de kant op):"
    $BudgetWarnings | ForEach-Object { Write-Host "  $($_.event.message)" }
}

# ------------------------------------------------- 3. validatie + catalogus
# De owner-regel voor de groene bar is breder dan build+suite: "EclipseValidateData
# 0 fouten + catalog gedocumenteerd = geimplementeerd". Die twee stonden hier
# eerst NIET in, en ik heb ze een hele nacht met de hand bij elkaar gezocht - de
# exacte fout die dit script moest wegnemen. Een controle die je moet onthouden,
# is geen controle.
Write-Step "VALIDATIE"
#
# EIGEN LOGBESTAND, en dat is geen netheid maar een reparatie.
#
# Hier stond geen -abslog, dus de commandlet schreef naar het GEDEELDE
# Saved\Logs\Eclipse.log en we lazen zijn uitslag daar weer uit. Zodra een
# tweede agent gelijktijdig een commandlet draait, overschrijft die het log en
# is de validatieregel weg -- waarna dit script "validatie gaf geen uitslag"
# meldt en de bar ONTERECHT rood kleurt. Dat is 31-07 gemeten: een run die
# 20:19:42 zijn regel schreef, was om 20:20:05 gewist door een andere agent.
#
# Een valse rode bar is duurder dan geen bar. Hij kost een zoektocht naar een
# fout die er niet is, en erger: hij went, en dan wordt een echte rode bar ook
# weggewuifd. Elke run leest nu zijn eigen bestand en kan dus niet meer door
# iemand anders worden gewist.
$ValidationLog = "$Root\Saved\Logs\verify_validate.log"
$p = Start-Process -FilePath $exe -PassThru -NoNewWindow -ArgumentList `
    "`"$Project`"", '-run=EclipseValidateData', '-unattended', '-nullrhi', '-NoLiveCoding', `
    "-abslog=`"$ValidationLog`""
if (-not $p.WaitForExit(600000)) { $p.Kill(); throw "validatie liep vast" }
$Validation = Select-String -Path $ValidationLog -Pattern "ValidateData: .* errors\." -ErrorAction SilentlyContinue |
    Select-Object -Last 1
if ($Validation) {
    Write-Host ($Validation.Line -replace '^.*Display: ', '')
    if ($Validation.Line -notmatch ', 0 errors\.') { $Failures += "validatie meldt fouten" }
    $script:ValidationLine = ($Validation.Line -replace '^.*Display: ', '')
} else {
    Write-Host "GEEN VALIDATIEREGEL in $ValidationLog - draaide de commandlet wel?"
    $Failures += "validatie gaf geen uitslag"
}

Write-Step "CATALOGUS"
python "$Root\Tools\check_event_catalog.py"
if ($LASTEXITCODE -ne 0) { $Failures += "event-catalogus klopt niet" }

# De creditmeter: generatie mag NOOIT kunnen slagen zonder gemeten spend.
# Stond een tijd lang stil kapot (401 op user_read werd geslikt), en de ledger
# las dan als waarheid terwijl er niets gemeten was.
python "$Root\Tools\test_credit_meter.py"
if ($LASTEXITCODE -ne 0) { $Failures += "creditmeter kan blind genereren" }

# Elke `voice:` in Content/Script moet ergens op uitkomen. Een spreker die
# nergens op uitkomt crasht NIET en valt ook niet terug op een standaardstem:
# de commandlet slaat hem over op Display-niveau en de samenvatting ziet er
# gezond uit. Dat merk je pas na de sprint, als de credits op zijn.
python "$Root\Tools\check_voice_resolves.py"
if ($LASTEXITCODE -ne 0) { $Failures += "scriptstem komt nergens op uit" }

# DE SCRIPTVALIDATOR, EN EERST HET BEWIJS DAT HIJ ROOD KAN WORDEN.
#
# Deze volgorde is niet netheid. Op 31-07 meldde check_voice_resolves.py een
# spreker als "CAST AND READY" terwijl er geen stem aan zijn slot hing: de
# controle stond groen, het groen was niets waard, en niemand kon dat zien
# omdat die controle nog nooit rood was geweest. Een validator die alleen
# groen is geweest op het huidige script meet niets - hij is niet te
# onderscheiden van `exit 0`.
#
# De zelftest draait dus VOOR de validator. Zakt het bewijs, dan is het
# oordeel over het script waardeloos en hoort de bar rood, ook al zou de
# validator zelf niets vinden.
python "$Root\Tools\tests\test_validate_script.py"
if ($LASTEXITCODE -ne 0) { $Failures += "scriptvalidator kan niet meer rood worden" }

# En dan het oordeel zelf (SCRIPT_FORMAT 6). Hij vangt de fouten van de
# architect net zo goed als die van de schrijvers, en dat is het punt: een
# vlag die van vorm verandert breekt bij de LEZER, en de lezer staat per
# definitie in een andere missie dan de zetter (L1-R12). Dat is onzichtbaar
# van binnenuit een missie en overduidelijk van bovenaf.
#
# --no-voice omdat check_voice_resolves.py hierboven al zijn eigen poort is.
# De validator kan hem zelf aanroepen (dat doet hij losstaand ook), maar twee
# keer dezelfde uitslag afdrukken maakt een bar langer en niet strenger. Hij
# meldt de stemcontrole dan als OVERGESLAGEN en niet als schoon - een
# controle die niet gedraaid heeft mag nooit in het rijtje geslaagde staan.
python "$Root\Tools\validate_script.py" --no-voice
if ($LASTEXITCODE -ne 0) { $Failures += "scriptvalidatie meldt bevindingen" }

# En dan de kaarten die de OWNER ziet. Dit is geen code-controle en hij hoort
# hier toch, want een kapotte owner-kaart is duurder dan een rode test: Nathan
# opent het dashboard, kan niets doen, en de fout ligt bij hem tot hij hem
# uitzoekt. Op 01-08 stonden er drie tegelijk - "kies een stem voor Petra" (zij
# had geen shortlist), "beluister de ijkmissie" (er was geen audio) en een
# audio_map die naar een niet-bestaande map wees. Alle drie door mij geschreven
# zonder te kijken of het ding aan de andere kant bestond.
#
# Zelftest eerst, om dezelfde reden als hierboven.
python "$Root\Tools\check_owner_questions.py" --zelftest
if ($LASTEXITCODE -ne 0) { $Failures += "kaartcontrole kan niet meer rood worden" }
python "$Root\Tools\check_owner_questions.py"
if ($LASTEXITCODE -ne 0) { $Failures += "een owner-kaart vraagt iets dat niet kan" }

# En de owner-DOCUMENTEN. Op 01-08 stonden er vier verlopen, alle vier op dezelfde
# manier: ze HERHAALDEN de stand in plaats van ernaar te verwijzen. Het project
# heeft daar al een principe voor en dat hield het vier keer niet tegen.
#
# Deze controle dekt bewust maar EEN claim -- het testaantal in STATUS.md tegen de
# laatste bar-regel in SOAK_LOG.md, want die schrijft dit script zelf. "De
# ijkmissie is NO-GO" is niet mechanisch te toetsen, en een controle die doet
# alsof hij meer dekt dan hij kan is precies de fout die hij moet vangen.
python "$Root\Tools\check_owner_docs.py" --zelftest
if ($LASTEXITCODE -ne 0) { $Failures += "documentcontrole kan niet meer rood worden" }
python "$Root\Tools\check_owner_docs.py"
if ($LASTEXITCODE -ne 0) { $Failures += "STATUS.md claimt een getal dat niet meer klopt" }

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

    # STAAT ER OOK IETS OP? Deze bar meldde "0 frame-fouten" terwijl 22 van de 50
    # editor-frames van diezelfde nacht hun onderste kwart op luminantie NUL hadden.
    # De controle hieronder grept het LOGBOEK op fouten die de game zelf meldt --
    # een frame waar de grond niet op staat meldt niets, dus die kon het per
    # constructie niet zien. Zelfde vorm als de vault die nooit getekend werd.
    #
    # Alleen de frames van DEZE ronde, want de historische 22 zijn geen regressie.
    # Zelftest eerst: hij bevat de negatieve controle dat DONKER niet LEEG is, en
    # zonder die controle valt de vault om terwijl hij klopt.
    if ($Shots.Count -gt 0) {
        python "$Root\Tools\check_shot_sanity.py" --zelftest
        if ($LASTEXITCODE -ne 0) { $Failures += "opnamecontrole kan niet meer rood worden" }
        python "$Root\Tools\check_shot_sanity.py" @($Shots | ForEach-Object { $_.FullName })
        if ($LASTEXITCODE -ne 0) { $Failures += "opnameronde: een frame bevat geen gerenderde scene" }
    }
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
    # BEKENDE, GEPARKEERDE MELDINGEN TELLEN NIET ALS FOUT. Ze worden wel getoond,
    # want ze horen zichtbaar te blijven - maar een bar die permanent rood staat om
    # iets wat de owner heeft geparkeerd, verbergt de volgende echte fout.
    $Known = Select-String -Path "$Root\Saved\Logs\Eclipse.log" -Pattern "PLAYSHOT \d+ BEKEND\]"
    if ($Known) {
        Write-Host "BEKEND EN GEPARKEERD (telt niet als fout):"
        $Known | ForEach-Object { Write-Host "  $($_.Line -replace '^.*LogEclipse: Warning: ', '')" }
    }
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
