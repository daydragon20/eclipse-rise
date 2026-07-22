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
    return "$([math]::Floor($hours / 24)) dagen geleden"
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

    # --- wegschrijven als JS (ConvertTo-Json levert geldige JS-objectliteral) ---
    $json = ($auto | ConvertTo-Json -Depth 5)
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
