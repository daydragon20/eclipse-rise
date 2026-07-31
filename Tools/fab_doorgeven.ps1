# ECLIPSE — Fab-doorgeefluik.
#
# Waarom dit bestaat: Fab weigert "Add to Project" als een pack 5.8 niet in zijn
# manifest noemt, ook als de assets prima laden. De omweg is: installeer het pack
# naar een ouder project dat je toch al hebt staan, en kopieer de map daarna hier
# naartoe. Dit script doet die tweede stap, zodat je hem niet met de hand hoeft
# te zoeken.
#
# Gebruik:  powershell -File Tools\fab_doorgeven.ps1              (toont wat er nieuw is)
#           powershell -File Tools\fab_doorgeven.ps1 -Kopieer     (kopieert het echt)
#           powershell -File Tools\fab_doorgeven.ps1 -UrenTerug 6 (breder zoeken)
param(
    [switch]$Kopieer,
    # Alleen deze mappen (namen, komma-gescheiden). Zonder dit: alles wat past.
    # Bewust nodig, want een doorgeefproject bevat ook zijn EIGEN content
    # (Movies, Splash, template-mappen) en die hoort niet in Eclipse thuis.
    [string[]]$Alleen,
    [int]$UrenTerug = 3,
    [string]$ProjectRoot = "C:\Users\natha\Documents\Unreal Projects",
    [string]$Doel = "C:\Dev\ECLIPSE_GDD\Eclipse\Content"
)

$ErrorActionPreference = "Stop"
$grens = (Get-Date).AddHours(-$UrenTerug)

# Bij aanroep met -File geeft PowerShell elk argument als EEN string door, dus
# "-Alleen A,B" komt binnen als @("A,B"). Zelf splitsen, anders matcht niets.
if ($Alleen) {
    $Alleen = @($Alleen -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ })
}

if (-not (Test-Path $ProjectRoot)) { Write-Host "Projectmap niet gevonden: $ProjectRoot"; exit 1 }
if (-not (Test-Path $Doel))        { Write-Host "Eclipse Content niet gevonden: $Doel";  exit 1 }

$bestaand = Get-ChildItem $Doel -Directory -ErrorAction SilentlyContinue | ForEach-Object { $_.Name }
$vondsten = @()

foreach ($proj in Get-ChildItem $ProjectRoot -Directory) {
    $content = Join-Path $proj.FullName "Content"
    if (-not (Test-Path $content)) { continue }

    foreach ($map in Get-ChildItem $content -Directory -ErrorAction SilentlyContinue) {
        # Met -Alleen telt de naam, niet de tijd: zo haal je ook iets op dat er
        # al langer staat zonder het venster te moeten oprekken.
        if ($Alleen) {
            if ($Alleen -notcontains $map.Name) { continue }
        }
        elseif ($map.LastWriteTime -lt $grens) { continue }
        # Al in Eclipse? Dan overslaan — nooit stil overschrijven.
        if ($bestaand -contains $map.Name) {
            Write-Host "  = $($map.Name) staat al in Eclipse (overgeslagen)" -ForegroundColor DarkGray
            continue
        }
        $mb = [math]::Round((Get-ChildItem $map.FullName -Recurse -File -ErrorAction SilentlyContinue |
                             Measure-Object -Property Length -Sum).Sum / 1MB, 1)

        # Kopieren werkt voor content (meshes, texturen, materialen, animaties, geluid):
        # die laden vooruit in een nieuwere engine. C++ en plugins niet — die moeten
        # tegen 5.8 gecompileerd worden, en dat is geen kopieerklus.
        $risico = ""
        $bronProj = Split-Path (Split-Path $map.FullName -Parent) -Parent
        if (Test-Path (Join-Path $bronProj "Source"))  { $risico = "project heeft C++" }
        if (Test-Path (Join-Path $bronProj "Plugins")) {
            $risico = if ($risico) { "$risico + plugins" } else { "project heeft plugins" }
        }

        $vondsten += [pscustomobject]@{
            Naam = $map.Name; Bron = $map.FullName; Project = $proj.Name
            MB = $mb; Tijd = $map.LastWriteTime.ToString("HH:mm"); Risico = $risico
        }
    }
}

if (-not $vondsten) {
    Write-Host ""
    Write-Host "Niets nieuws gevonden in de laatste $UrenTerug uur." -ForegroundColor Yellow
    Write-Host "Klopt dat niet? Probeer -UrenTerug 12, of controleer of de Launcher klaar is met downloaden."
    exit 0
}

Write-Host ""
Write-Host "Gevonden in een doorgeefproject:" -ForegroundColor Cyan
foreach ($v in $vondsten) {
    Write-Host ("  {0,-34} {1,7} MB   uit {2}  ({3})" -f $v.Naam, $v.MB, $v.Project, $v.Tijd)
}
$risicovol = @($vondsten | Where-Object { $_.Risico })
if ($risicovol.Count -gt 0) {
    Write-Host ""
    Write-Host "LET OP - kopieren dekt alleen content:" -ForegroundColor Yellow
    foreach ($v in $risicovol) {
        Write-Host "  $($v.Naam): $($v.Risico). Hangt dit pack aan code of een plugin," -ForegroundColor Yellow
        Write-Host "  dan komen de assets wel mee maar werkt het gedrag niet." -ForegroundColor Yellow
    }
}

if (-not $Kopieer) {
    Write-Host ""
    Write-Host "Dit was een proefronde. Draai opnieuw met -Kopieer om het echt te doen." -ForegroundColor Yellow
    exit 0
}

Write-Host ""
foreach ($v in $vondsten) {
    $naar = Join-Path $Doel $v.Naam
    Copy-Item $v.Bron $naar -Recurse -Force
    Write-Host "  -> $($v.Naam) gekopieerd naar Eclipse\Content" -ForegroundColor Green
}

Write-Host ""
Write-Host "Klaar. De editor ziet ze bij de volgende start; een draaiende editor moet je" -ForegroundColor Green
Write-Host "even laten herladen (rechtsklik op Content -> Refresh)." -ForegroundColor Green
