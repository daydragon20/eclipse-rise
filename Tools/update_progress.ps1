# ECLIPSE — dashboard-watcher. Leest repo-feiten (git, testrapport, nieuwste screenshots)
# en schrijft ze naar progress_auto.js, zodat PROGRESS.html bij elke herlaad verse info toont
# zonder dat iemand HTML hoeft aan te passen. Bewust GEEN git push/pull hier: het dashboard
# wordt lokaal bekeken en auto-commits zouden de nette [Systeem]-historie vervuilen.
#
# Gebruik:  eenmalig:  powershell -File Tools\update_progress.ps1
#           watcher :  powershell -File Tools\update_progress.ps1 -Loop [-IntervalSec 600]
#           (of dubbelklik start_progress_watcher.bat in de repo-root)
param(
    [switch]$Loop,
    [int]$IntervalSec = 600,
    [string]$RepoRoot = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = "Continue"

function Get-RelativeAge([datetime]$When) {
    $mins = [int]((Get-Date) - $When).TotalMinutes
    if ($mins -lt 1) { return "zojuist" }
    if ($mins -lt 60) { return "$mins min geleden" }
    $hours = [math]::Floor($mins / 60)
    if ($hours -lt 24) { return "$hours uur geleden" }
    $dagen = [math]::Floor($hours / 24)
    if ($dagen -eq 1) { return "1 dag geleden" }
    return "$dagen dagen geleden"
}

function Update-Once([string]$Root) {
    $auto = [ordered]@{
        generatedAt = (Get-Date -Format "yyyy-MM-dd HH:mm")
        tests       = $null
        commits     = @()
        screenshots = @()
    }

    # --- git: laatste 8 commits (feit, geen inschatting) ---
    try {
        $log = git -C $Root log -8 --pretty=format:"%h|%ad|%s" --date=format:"%d-%m %H:%M" 2>$null
        foreach ($line in @($log)) {
            $parts = $line -split '\|', 3
            if ($parts.Count -eq 3) {
                $auto.commits += [ordered]@{ hash = $parts[0]; date = $parts[1]; msg = $parts[2] }
            }
        }
    } catch {}

    # --- testrapport: Saved/Automation/index.json van de laatste run ---
    try {
        $report = Join-Path $Root "Eclipse\Saved\Automation\index.json"
        if (Test-Path $report) {
            $r = Get-Content $report -Raw | ConvertFrom-Json
            $auto.tests = [ordered]@{
                ok        = [int]$r.succeeded + [int]$r.succeededWithWarnings
                failed    = [int]$r.failed
                total     = @($r.tests).Count
                reportAge = Get-RelativeAge (Get-Item $report).LastWriteTime
            }
        }
    } catch {}

    # --- screenshots: nieuwste 4 review-shots -> verkleinde live-thumbnails ---
    try {
        $shotDir = Join-Path $Root "Eclipse\Saved\Screenshots\WindowsEditor"
        $mediaDir = Join-Path $Root "progress_media"
        if (Test-Path $shotDir) {
            if (-not (Test-Path $mediaDir)) { New-Item -ItemType Directory -Path $mediaDir | Out-Null }
            Add-Type -AssemblyName System.Drawing
            $enc = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object { $_.MimeType -eq 'image/jpeg' }
            $prm = New-Object System.Drawing.Imaging.EncoderParameters(1)
            $prm.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter([System.Drawing.Imaging.Encoder]::Quality, [long]82)

            $src = Get-ChildItem $shotDir -Filter "HighresScreenshot*.png" -ErrorAction SilentlyContinue |
                   Sort-Object LastWriteTime -Descending | Select-Object -First 4
            $i = 0
            foreach ($png in $src) {
                $i++
                $thumb = Join-Path $mediaDir ("live_0{0}.jpg" -f $i)
                $stale = (-not (Test-Path $thumb)) -or ((Get-Item $thumb).LastWriteTime -lt $png.LastWriteTime)
                if ($stale) {
                    $img = [System.Drawing.Image]::FromFile($png.FullName)
                    $bmp = New-Object System.Drawing.Bitmap(640, 360)
                    $g = [System.Drawing.Graphics]::FromImage($bmp)
                    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                    $g.DrawImage($img, 0, 0, 640, 360)
                    $bmp.Save($thumb, $enc, $prm)
                    $g.Dispose(); $bmp.Dispose(); $img.Dispose()
                }
                $auto.screenshots += [ordered]@{
                    file    = "progress_media/live_0$i.jpg"
                    caption = "Laatste run - beeld $i ($(Get-RelativeAge $png.LastWriteTime))"
                }
            }
        }
    } catch {}

    # --- werkvoortgang: GETELD uit progress_data.js, niet geschat ---
    # De percentages in het dashboard zijn handmatige scope-inschattingen. Die kunnen
    # verouderen zonder dat iemand het ziet. Deze telling is een hard feit uit dezelfde
    # bron, zodat het dashboard kan tonen of de schatting nog met het werk meebeweegt.
    try {
        $dataFile = Join-Path $Root "progress_data.js"
        if (Test-Path $dataFile) {
            $raw = Get-Content $dataFile -Raw

            $taken = [regex]::Matches($raw, 'taak:\s*"(?:[^"\\]|\\.)*",\s*status:\s*"(\w+)",\s*pct:\s*(\d+)')
            $status = @{ klaar = 0; bezig = 0; gepland = 0; wachten = 0 }
            $som = 0
            foreach ($m in $taken) {
                $s = $m.Groups[1].Value
                if ($status.ContainsKey($s)) { $status[$s]++ }
                $som += [int]$m.Groups[2].Value
            }
            $aantal = @($taken).Count

            $fases = @()
            # Escape-tolerant: fase-namen bevatten aanhalingstekens (Prototype \"The Loop\"),
            # en een naief [^"]* kapt die eruit — precies de twee actieve fases.
            foreach ($m in [regex]::Matches($raw, 'naam:\s*"(Fase\s*\d+(?:[^"\\]|\\.)*)",\s*pct:\s*(\d+)')) {
                $fases += [ordered]@{ naam = ($m.Groups[1].Value -replace '\\"', '"'); pct = [int]$m.Groups[2].Value }
            }

            $heel = [regex]::Match($raw, 'hero:\s*\{[^}]*pct:\s*(\d+)')

            $auto.werk = [ordered]@{
                taken       = $aantal
                klaar       = $status.klaar
                bezig       = $status.bezig
                open        = $status.gepland + $status.wachten
                gemiddeld   = if ($aantal -gt 0) { [math]::Round($som / $aantal) } else { 0 }
                fases       = $fases
                heleGamePct = if ($heel.Success) { [int]$heel.Groups[1].Value } else { $null }
                dataAge     = Get-RelativeAge (Get-Item $dataFile).LastWriteTime
                dataStamp   = (Get-Item $dataFile).LastWriteTime.ToString("yyyy-MM-dd HH:mm")
            }
        }
    } catch {}

    # --- wat is er NU onderhanden: gewijzigd maar nog niet gecommit ---
    # Dit is "waar is hij mee bezig" in de letterlijke zin. Het handmatige
    # taken-veld in progress_data.js liep anderhalve dag achter; dit kan dat niet.
    try {
        $gewijzigd = @(git -C $Root status --porcelain 2>$null | Where-Object { $_ -notmatch '^\?\?' })
        $groepen = [ordered]@{}
        foreach ($g in $gewijzigd) {
            $pad = ($g.Substring(3)).Trim('"')
            $gebied = switch -Regex ($pad) {
                'Source/Eclipse/Combat'      { 'Gevecht'; break }
                'Source/Eclipse/Characters'  { 'Personage en camera'; break }
                'Source/Eclipse/UI'          { 'Interface'; break }
                'Source/Eclipse/Audio'       { 'Geluid'; break }
                'Source/Eclipse/Tests'       { 'Tests'; break }
                'Source/Eclipse/Core'        { 'Kern'; break }
                'Source/'                    { 'Overige code'; break }
                'Content/'                   { 'Assets'; break }
                '^(progress_|START_HIER|PROGRESS)' { 'Dashboard'; break }
                'progress_media/'            { 'Dashboard'; break }
                '\.(html|js)$'               { 'Dashboard'; break }
                '^Tools/|\.ps1$|\.py$'       { 'Gereedschap'; break }
                '\.(md|txt)$'                { 'Documentatie'; break }
                default                      { 'Overig' }
            }
            if (-not $groepen.Contains($gebied)) { $groepen[$gebied] = @() }
            $groepen[$gebied] += (Split-Path $pad -Leaf)
        }
        $lijst = @()
        foreach ($k in $groepen.Keys) {
            $lijst += [ordered]@{ gebied = $k; aantal = @($groepen[$k]).Count; bestanden = @($groepen[$k] | Select-Object -First 3) }
        }
        $auto.onderhanden = [ordered]@{ totaal = @($gewijzigd).Count; gebieden = $lijst }
    } catch {}

    # --- waar is hij de laatste uren mee bezig geweest ---
    # Uit git en niet uit een handmatig veld: dan klopt het altijd en blijft er
    # nooit iets staan dat al af is. Per uur gegroepeerd, want dat is hoe de
    # eigenaar het leest — "waar ging vanmiddag heen".
    try {
        $sinds = (Get-Date).AddHours(-9).ToString("yyyy-MM-dd HH:mm:ss")
        $regels = @(git -C $Root log --since="$sinds" --pretty=format:"%H|%ad|%s" --date=format:"%H:%M" 2>$null)

        $perUur = [ordered]@{}
        foreach ($r in $regels) {
            $d = $r -split '\|', 3
            if ($d.Count -lt 3) { continue }
            $uur = ($d[1] -split ':')[0] + ":00"
            if (-not $perUur.Contains($uur)) { $perUur[$uur] = @() }
            # Het bloklabel eraf: [Repo], [Squad] enz. zeggen weinig en zijn hier
            # bovendien onbetrouwbaar — er zit code onder [Repo]-commits.
            $perUur[$uur] += ($d[2] -replace '^\[[^\]]+\]\s*', '')
        }

        $tijdlijn = @()
        foreach ($uur in $perUur.Keys) {
            $tijdlijn += [ordered]@{
                uur    = $uur
                aantal = @($perUur[$uur]).Count
                titels = @($perUur[$uur] | Select-Object -First 4)
            }
        }

        $auto.recentWerk = [ordered]@{
            urenTerug = 9
            totaal    = @($regels).Count
            perUur    = $tijdlijn
        }
    } catch {}

    # --- tempo: commits per dag, zodat stilstand zichtbaar wordt ---
    try {
        $vandaag = @(git -C $Root log --since="midnight" --oneline 2>$null).Count
        $week    = @(git -C $Root log --since="7 days ago" --oneline 2>$null).Count
        # git gebruikt strftime, geen .NET-formaten — %Y niet yyyy.
        $laatste = git -C $Root log -1 --pretty=format:"%ad" --date=format:"%Y-%m-%d %H:%M" 2>$null
        $auto.tempo = [ordered]@{
            vandaag = $vandaag
            week    = $week
            laatsteCommit = "$laatste"
        }
        if ($laatste) { $auto.tempo.laatsteAge = Get-RelativeAge ([datetime]::ParseExact($laatste, "yyyy-MM-dd HH:mm", $null)) }
    } catch {}

    # --- wegschrijven als JS (ConvertTo-Json levert geldige JS-objectliteral) ---
    $json = ($auto | ConvertTo-Json -Depth 6)
    $body = "// AUTOMATISCH GEGENEREERD door Tools/update_progress.ps1 - NIET met de hand bewerken.`r`nwindow.PROGRESS_AUTO = $json;`r`n"
    $body | Out-File -FilePath (Join-Path $Root "progress_auto.js") -Encoding utf8
    Write-Host "[update_progress] $($auto.generatedAt) - $(@($auto.commits).Count) commits, tests: $(if ($auto.tests) { "$($auto.tests.ok)/$($auto.tests.total)" } else { 'geen rapport' }), $(@($auto.screenshots).Count) shots"
}

if ($Loop) {
    Write-Host "[update_progress] watcher gestart - elke $IntervalSec s (stoppen: Ctrl+C of venster sluiten)"
    while ($true) {
        Update-Once $RepoRoot
        Start-Sleep -Seconds $IntervalSec
    }
} else {
    Update-Once $RepoRoot
}
