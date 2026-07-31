# maak_bureaublad_snelkoppelingen.ps1
#
# Zet de vier knoppen die Nathan dagelijks nodig heeft op het bureaublad.
# Het zijn SNELKOPPELINGEN, geen kopieen: als de .bat in de repo verandert,
# verandert de knop mee. Opnieuw draaien is altijd veilig - bestaande
# snelkoppelingen worden gewoon overschreven.
#
# Draaien:     powershell -ExecutionPolicy Bypass -File Tools\maak_bureaublad_snelkoppelingen.ps1
# Verwijderen: powershell -ExecutionPolicy Bypass -File Tools\maak_bureaublad_snelkoppelingen.ps1 -Remove

param([switch]$Remove)

$ErrorActionPreference = "Stop"
$Repo    = Split-Path -Parent $PSScriptRoot
$Desktop = [Environment]::GetFolderPath("Desktop")

$Knoppen = @(
    @{ Naam = "1 - SPEEL ECLIPSE";      Doel = "SPEEL_ECLIPSE.bat";      Info = "Start de game" }
    @{ Naam = "2 - ECLIPSE Dashboard";  Doel = "START_DASHBOARD.bat";    Info = "Live dashboard op poort 8377" }
    @{ Naam = "3 - Loop STOPPEN";       Doel = "STOP-autonomie.bat";     Info = "Claude mag weer stoppen als hij klaar is" }
    @{ Naam = "4 - Loop HERVATTEN";     Doel = "HERVAT-autonomie.bat";   Info = "Claude blijft autonoom doorwerken" }
)

if ($Remove) {
    foreach ($k in $Knoppen) {
        $lnk = Join-Path $Desktop ($k.Naam + ".lnk")
        if (Test-Path $lnk) { Remove-Item $lnk -Force; Write-Host "  weg: $($k.Naam)" -ForegroundColor Yellow }
    }
    Write-Host "`n  Snelkoppelingen verwijderd." -ForegroundColor Yellow
    return
}

$shell = New-Object -ComObject WScript.Shell
$gemaakt = 0

Write-Host ""
Write-Host "  Bureaublad: $Desktop" -ForegroundColor Cyan
Write-Host ""

foreach ($k in $Knoppen) {
    $doel = Join-Path $Repo $k.Doel
    if (-not (Test-Path $doel)) {
        Write-Host "  OVERGESLAGEN: $($k.Doel) bestaat niet" -ForegroundColor Red
        continue
    }

    $lnk = Join-Path $Desktop ($k.Naam + ".lnk")
    $s = $shell.CreateShortcut($lnk)
    $s.TargetPath       = $doel
    $s.WorkingDirectory = $Repo
    $s.Description      = $k.Info
    $s.WindowStyle      = 1
    $s.Save()

    Write-Host "  [OK] $($k.Naam)" -NoNewline -ForegroundColor Green
    Write-Host "  ->  $($k.Doel)" -ForegroundColor DarkGray
    $gemaakt++
}

Write-Host ""
Write-Host "  $gemaakt snelkoppelingen op je bureaublad gezet." -ForegroundColor Green
Write-Host "  Het zijn snelkoppelingen, dus ze blijven meelopen met de repo."
Write-Host ""
