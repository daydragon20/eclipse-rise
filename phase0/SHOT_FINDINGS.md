# BEVINDINGEN UIT SCREENSHOTS
*Eigenaar: `screenshot-inspector`. Vul aan, verwijder nooit — een afgehandelde bevinding krijgt `opgelost` in de laatste kolom.*
*Doel: Nathan hoeft niet zelf te melden wat er op zijn scherm mis is.*

**Beoordeeld tot en met:** `Schermafbeelding 2026-07-31 195356.png` (handmatig, door de hoofdsessie)

---

## Bevindingen

| Datum | Bestand | Ernst | Wat er te zien is | Waarschijnlijke oorzaak |
|---|---|---|---|---|
| 2026-07-31 19:11 | Schermafbeelding 191142 | blokkeert | Dialoog: map `C:/Users/natha/AppData/Local/Programs/Git/Game/Maps/GrayboxDistrict` niet gevonden | **Bevestigd:** Git Bash verminkt `/Game/...`-argumenten. `DEBUG_DISCIPLINE.md` §4.4. Start via PowerShell. |
| 2026-07-31 19:19 | Schermafbeelding 191919 | blokkeert | Zelfde dialoog, opnieuw | De agent gebruikte opnieuw de Bash-tool |
| 2026-07-31 19:20 | Schermafbeelding 192039 | blokkeert | "GPU Crashed or D3D Device Removed", mini-dump weggeschreven | Page fault in een compute-shader (Aftermath-dump). Zie owner-vraag O-7. |
| 2026-07-31 19:21 | Schermafbeelding 192113 | blokkeert | Crash Reporter, stack ×5 in `UnrealEditor_D3D12RHI` | Zelfde crash |
| 2026-07-31 19:53 | Schermafbeelding 195356 | blokkeert | "Missing Eclipse Modules — Eclipse" | `UnrealEditor-Eclipse.dll` ontbreekt in `Eclipse/Binaries/Win64`; alleen de editor-module is gebouwd. Bouwen met `-NoUba`. |
| 2026-07-31 avond | *(owner-waarneming)* | fout | Wapen wordt **omgekeerd** vastgehouden | Vermoedelijk socket-rotatie: forward-as van de mesh matcht niet met `hand_r`. Zie OBS-1. |
| 2026-07-31 avond | *(owner-waarneming)* | fout | Geen handen/armen zichtbaar in first-person | Lichaam wordt niet gerenderd in 1e persoon, dus wapen én handen verdwijnen mee. Ontwerpkeuze nodig — zie OBS-2. |
| 2026-07-31 avond | *(owner-waarneming)* | stijl | Schermlaag volgt de Borderlands-taal nog niet | Stijlronde staat gepland ná functionele compleetheid. Zie `REFERENTIE_HUD_BORDERLANDS.md` en O-8. |

---

## Hoe dit werkt

`screenshot-inspector` draait **aan het begin van elke werkcyclus**, kijkt naar de beelden die nog niet in de tabel staan, en vult aan. Twee mappen:

- `Eclipse/Saved/Screenshots/WindowsEditor` — wat de agents zelf maken
- `C:\Users\natha\Pictures\Screenshots` — wat Nathan met Win+PrtSc vastlegt, dus waar de **foutmeldingen** staan

De eerste acht regels hierboven zijn met de hand ingevuld door de hoofdsessie op 31-07, als startpunt. Vanaf nu gaat het vanzelf.
