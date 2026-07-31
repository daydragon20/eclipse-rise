# install_dashboard_autostart.ps1
#
# Zorgt dat het live dashboard automatisch start bij het aanmelden, zodat het
# ook draait als er GEEN chatsessie open is. Vereist GEEN beheerdersrechten:
# er wordt een taak op de huidige gebruiker gezet, geen systeemtaak.
#
# Draaien:   powershell -ExecutionPolicy Bypass -File Tools\install_dashboard_autostart.ps1
# Verwijderen: powershell -ExecutionPolicy Bypass -File Tools\install_dashboard_autostart.ps1 -Remove

param([switch]$Remove)

$ErrorActionPreference = "Stop"
$TaskName = "ECLIPSE-Dashboard"
$Repo     = Split-Path -Parent $PSScriptRoot
$Script   = Join-Path $Repo "Tools\eclipse_dashboard.py"

if ($Remove) {
    try {
        Unregister-ScheduledTask -TaskName $TaskName -Confirm:$false
        Write-Host "Autostart verwijderd." -ForegroundColor Yellow
    } catch {
        Write-Host "Er stond geen autostart-taak." -ForegroundColor Yellow
    }
    return
}

if (-not (Test-Path $Script)) { throw "Niet gevonden: $Script" }

# pythonw draait zonder consolevenster
$py = (Get-Command pythonw -ErrorAction SilentlyContinue).Source
if (-not $py) { $py = (Get-Command python -ErrorAction SilentlyContinue).Source }
if (-not $py) { throw "Python niet gevonden in PATH." }

$action  = New-ScheduledTaskAction -Execute $py -Argument "`"$Script`"" -WorkingDirectory $Repo
$trigger = New-ScheduledTaskTrigger -AtLogOn -User $env:USERNAME
$set     = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries `
                                        -StartWhenAvailable -ExecutionTimeLimit ([TimeSpan]::Zero)

try { Unregister-ScheduledTask -TaskName $TaskName -Confirm:$false } catch {}

Register-ScheduledTask -TaskName $TaskName -Action $action -Trigger $trigger `
                       -Settings $set -Description "ECLIPSE live dashboard op poort 8377" | Out-Null

Write-Host ""
Write-Host "  Autostart geinstalleerd." -ForegroundColor Green
Write-Host "  Het dashboard start voortaan vanzelf bij aanmelden."
Write-Host "  Nu meteen starten:  Start-ScheduledTask -TaskName $TaskName"
Write-Host "  Adres:              http://127.0.0.1:8377/"
Write-Host ""
