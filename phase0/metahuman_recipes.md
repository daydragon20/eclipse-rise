# MetaHuman-recepten — named story characters (stap 3)
*Werkdocument voor de eigenaar. Bron: `02_story_bible.md` (companions/villains), stijlkader `15_visual_quality_charter.md` §15.5/15.7. De agent heeft de code/data-slots klaar (`DT_NamedCharacters`); jij maakt per character alleen het gezicht in MetaHuman Creator en exporteert met de exacte naam hieronder — de rest (import, toon-restyle, body/outfit uit onze packs, inpluggen in missies) doet de agent.*

## Zo werk je (eenmalig per character, ±10 min)
1. Unreal-editor → **Window → MetaHuman** (plugin staat aan) → **Create New** (of MetaHuman Creator in de browser via Quixel Bridge — beide goed).
2. Kies het **preset-startpunt** hieronder, pas de genoemde punten aan (niet meer dan dat — de toon-shader doet de stilering, dus géén extreme detailsculpt nodig).
3. Naamgeving bij opslaan/exporteren: **exact** de assetnaam hieronder (`MH_<Naam>`), export naar het Eclipse-project.
4. Zeg kort "MH_<Naam> staat erin" — de agent koppelt hem dan aan het slot en zet de toon-restyle + fallback-body-keten erop.

> **Stijlbewaking:** MetaHumans zijn fotorealistisch; in-game krijgen ze de cel-shading-laag (zoals alle bodies). Kies dus voor *leesbare, uitgesproken* gezichtsvormen — Borderlands leeft van silhouet en karakterkoppen, niet van poriën.

## De zes recepten

### 1. MH_Kaya — Kaya Renn (27, smokkelpiloot, The Shroud)
- **Preset-start:** jonge vrouw, smal-atletisch, hoekige kaaklijn.
- **Gezicht:** scherpe jukbeenderen, alerte iets toegeknepen ogen (donker­bruin), smalle wenkbrauwen met hoge boog (permanent "wat denk je zelf"-blik), kleine snelle mond.
- **Huid:** licht getaand, ruimtevaarder-bleek aan de onderkant; een dun litteken door de rechterwenkbrauw.
- **Haar:** kort en praktisch (undercut of strakke bob), gitzwart.
- **Uitstraling:** snel, spottend, energiek — de fast-talker.

### 2. MH_Brick — Oram "Brick" Bex (34, heavy, ex-mijnwerker Krad-9)
- **Preset-start:** zwaarste mannelijke bouw die Creator biedt, brede nek.
- **Gezicht:** blokvormig, zware kaak, gebroken-en-geheelde neus, kleine rustige ogen (grijs), dikke lage wenkbrauwen; oorlelletje-kerf (mijnongeval).
- **Huid:** donker, verweerd, stof-in-de-poriën-look; baardschaduw.
- **Haar:** geschoren of stoppelkort.
- **Uitstraling:** stil verdriet, zachtaardige reus — de moraal-anker die elke naam op de muur kent.

### 3. MH_Vale — Torren Vale (45, ex-Dominion-kolonel)
- **Preset-start:** man middenveertig, rechte militaire houding, mager-pezig.
- **Gezicht:** lang en gegroefd, diepe nasolabiale lijnen, staalblauwe ogen met zware oogleden (vermoeide waakzaamheid), strakke dunne mond.
- **Huid:** licht, koel ondertoon; scheerlijn-precisie.
- **Haar:** kort zilvergrijs, militaire coupe; eventueel dunne grijze snor.
- **Uitstraling:** discipline met een barst erin — is hij leraar of bouwt hij het ding dat hij ontvluchtte opnieuw?

### 4. MH_Dex — Dex Callum (31, engineer/scavenger)
- **Preset-start:** man begin dertig, smal, licht voorovergebogen nek (bureau/werkbank-houding).
- **Gezicht:** asymmetrisch en expressief: één wenkbrauw permanent hoger, sceptische mondhoek, donkere kringen (slaapt niet), levendige lichtbruine ogen.
- **Huid:** olijf, brandvlekje op de linkerkaak (soldeerspat).
- **Haar:** warrig halflang donkerbruin, alsof er net een koptelefoon af is.
- **Uitstraling:** cynisch genie — bouwt in plaats van hoopt.

### 5. MH_Petra — Petra Voss (tante van Voss, ±55, het stille hart van de schuilplaats)
- **Preset-start:** vrouw midden vijftig, zachte maar niet fragiele bouw.
- **Gezicht:** warm en breed, lachrimpels naast de ogen ÉN een waakzame trek erboven (AEGIS-detentie laat sporen), heldere grijsgroene ogen.
- **Huid:** licht verweerd, arbeiders-handenvolk-teint.
- **Haar:** opgestoken grijzend kastanje, losse plukken.
- **Uitstraling:** de reden dat mensen blijven — warmte met staal eronder.

### 6. MH_Kaine — Grand Marshal Sera Kaine (49, villain, de degen)
- **Preset-start:** vrouw eind veertig, lang, rechte scherpe houding.
- **Gezicht:** symmetrisch en gebeeldhouwd — bijna te perfect; hoge jukbeenderen, kille lichtgrijze ogen, wenkbrauwen laag en recht (geen verrassing, alleen beoordeling), strakke mondlijn.
- **Huid:** bleek, vlekkeloos (het gezicht van de propaganda).
- **Haar:** streng zilverblond, strak achterover of scherpe korte snit.
- **Uitstraling:** eervol-door-eigen-code, dodelijk competent — de vijand die je bijna respecteert.

## Wat de agent daarna doet (geen actie van jou)
- Import koppelen aan `DT_NamedCharacters` (slot per character), toon-restyle over de MetaHuman-materialen, outfit/body-combinatie met onze pack-kleding, idle/gesprekshouding, en de missie/dialoog-hooks per SPEC-P2-04.
- Zolang een `MH_*` ontbreekt, draait het slot automatisch op de fallback-body uit `DT_BodyDefs` — niets blokkeert.
