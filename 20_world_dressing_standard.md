# PART 20 — WORLD DRESSING STANDARD
**ECLIPSE: RISE OF THE RESISTANCE**
*Game Design Bible — Document 20 of 20 | Compositie, geloofwaardigheid, en het verbod op opvulling*

---

> **Aanleiding.** Owner-oordeel, 2026-07-31: *"De wereld ziet er nu helemaal niet episch mooi en indrukwekkend en realistisch geloofwaardig uit — met verkeersborden en gewoon wat random assets als opvulling."*
>
> **Verhouding tot Part 15.** `15_visual_quality_charter.md` gaat over **fidelity** — Nanite, Lumen, materialen, budgetten. Dit document gaat over **compositie en geloofwaardigheid**: waar dingen staan en waarom. Een scène kan elke technische eis halen en er nog steeds uitzien als een asset-dump. Dat is precies wat er nu gebeurt.

---

## 20.1 De diagnose

Er wordt **gestrooid in plaats van gecomponeerd**. Props worden geplaatst om leegte te vullen, niet om een ruimte te bouwen. Dat produceert drie herkenbare fouten tegelijk:

1. **Ruis zonder betekenis** — veel objecten, geen focuspunt. Het oog weet niet waar het moet kijken, dus kijkt het nergens naar. Meer props maken dit erger, niet beter.
2. **Fictiebreuk** — verkeersborden. Dit is het scherpste symptoom en het verdient een eigen regel (§20.2).
3. **Geen schaal** — zonder ankers van bekende grootte leest niets als groot. "Episch" is geen eigenschap van een mesh; het is een verhouding.

**De onderliggende oorzaak:** er wordt gewerkt met een assetlijst in plaats van met een ruimte-intentie. De vraag was *"welke props heb ik en waar passen ze?"*. De vraag moet zijn *"welke ruimte bouw ik, en wat hoort daar dan te staan?"*

---

## 20.2 De fictiewet — geen aardse semiotiek

**Er zijn geen verkeersborden in de Vantara Expanse.**

De mensheid verliet Origin zeshonderd jaar geleden. Er is geen Aarde, geen Weens Verdrag over verkeerstekens, geen westerse straatmeubilairtraditie. Een verkeersbord in Kessara is niet "een beetje generiek" — het vertelt de speler dat deze wereld niet echt is bedacht.

**Verboden, categorisch:**

- Verkeersborden, stoplichten, brandkranen, straatnaambordjes in modern-westerse vorm
- Herkenbare hedendaagse voertuigen, meubels, apparaten
- Merken, logo's of typografie uit de echte wereld
- Latijns alfabet in wereld-tekst zonder Dominion-typografie
- Elk asset dat "modern-industrieel Aarde" leest in plaats van "Vantara-koloniaal"

**In plaats daarvan** — elk stuk wereld-informatie draagt de iconografie uit `02_story_bible.md` §2.2:

| Functie | Vantara-vorm |
|---|---|
| Waarschuwing / gevaar | Dominion-hazardtaal, gebouwd rond de **Radiance**-zon |
| Wegwijzing | Lane-codes en sectornummers, niet straatnamen |
| Autoriteit | Het zon-embleem, de sun-mirrors, Veil-zegels |
| Rantsoenering / arbeid | Tithe-lotnummers, rantsoenkaart-iconografie, ploegcodes |
| Verzet | De Eclipse-sigil — gespoten, gekrast, weggeschuurd en terug |

**De test:** kan een speler dit object aanwijzen en zeggen "dat komt uit onze wereld"? Dan hoort het hier niet. Een asset dat stilistisch perfect is maar aards leest, breekt de wereld harder dan een lelijke mesh die wél van hier is.

**Praktisch:** gekochte packs bevatten bijna altijd aardse semiotiek. Curatie betekent niet alleen "past de stijl" maar ook **"past de fictie"**. Borden, decals en tekst uit een pack worden vervangen, niet geplaatst — en dat is goedkoop, want het is decal-werk (`Tools/generate_decals.py` bestaat al).

---

## 20.3 De opvullingswet

> **Geen enkel object wordt geplaatst om ruimte te vullen.**

Elk geplaatst object beantwoordt één vraag in één zin: **waarom staat dit hier?** Geldige antwoorden:

- **Functie** — "Dit is een laadperron; hier staan pallets omdat er geladen wordt."
- **Verhaal** — "Hier is iemand weggerend; de kar staat scheef en de lading ligt eruit."
- **Compositie** — "Dit kadert de doorkijk naar de spire."
- **Gameplay** — "Dit is dekking op de route die de speler zal nemen."
- **Schaal** — "Dit anker maakt de hal leesbaar groot."

Ongeldig: *"het was leeg daar"*, *"dit pack had het"*, *"er stond niks in die hoek"*.

> **Dit is geen zuinigheidsregel.** `21_quality_mandate.md` vraagt om méér — meer dichtheid, meer detail, meer graphic points. Deze wet zegt alleen dat het méér zich moet verantwoorden. Twintig props die samen een ruimte bouwen: goed. Twintig props omdat de hoek leeg was: dat is het probleem waar dit document over gaat. Meer, niet willekeurig.

**Leegte is compositie, geen gebrek.** Een lege straat met één omgevallen kar vertelt meer dan een straat vol rommel. De reflex om leegte te vullen is precies de reflex die dit probleem veroorzaakt heeft.

---

## 20.4 Wat een beeld épisch maakt

"Episch" is geen kwaliteit van assets. Het zijn vijf meetbare verhoudingen. Elke player-facing view moet er minstens **drie** hebben; elke signature view alle vijf.

1. **Verticaal bereik met een menselijk anker.** Iets groots leest pas als groot wanneer er iets van bekende grootte naast staat. Een deur, een reling, een figuur, een trap. Zonder anker is een kolos van tweehonderd meter niet te onderscheiden van een maquette van twee.

2. **Drie dieptelagen.** Voorgrond die kadert (een pijler, een balustrade, een silhouet aan de rand), middengrond waar de actie is, achtergrond met een landmark. Eén laag is een plaatje; drie lagen is een ruimte.

3. **Atmosferische scheiding.** Elke laag zit in zijn eigen luchtdichtheid. Dit is de goedkoopste epische winst die er bestaat — mist- en dieptescheiding kost bijna niets en doet meer dan honderd extra props.

4. **Eén dominant silhouet.** Elk district heeft één ding waarop je navigeert en dat je onthoudt. De rest is ondergeschikt — en dat is een letterlijke regel: *de meeste assets in een scène horen visueel ondergeschikt te zijn aan het hero-asset.*

5. **Een onthulling.** Episch is een *gebeurtenis*, geen toestand. De speler loopt door iets kraps en het opent. Dat contrast is het effect; een permanent weids beeld went binnen tien seconden.

**Licht doet het werk, niet props.** Als een ruimte niet werkt, is het antwoord bijna nooit "meer objecten". Het is licht, schaal of leegte.

**En licht is nooit één licht** (`21_quality_mandate.md` §21.2). Key, fill, rim, praktische lampen die in de scène zelf zichtbaar staan, en bounce. Meer hoeken en meer aanleidingen voor licht zijn expliciete owner-prioriteit. Elk district hoort meerdere **graphic points** te hebben: plekken waar licht, silhouet en compositie samenvallen tot een beeld dat je zou willen vastleggen. Eén goed belichte hoek per district is te weinig.

---

## 20.5 Geloofwaardigheid — slijtage en gebruik

Niets is nieuw en niets is gelijkmatig vies. Beide lezen als nep.

- **Slijtage waar handen komen** — deurranden, leuningen, knoppen, hoeken op heuphoogte.
- **Slijtage waar voeten komen** — looppaden slijten, de rest niet. Waar mensen echt lopen, is af te lezen aan de vloer.
- **Slijtage waar water loopt** — strepen onder randen, roest onder bevestigingen, zout waar het opdroogt.
- **Herstel is zichtbaar** — een bezette wereld repareert goedkoop. Lasnaden, mismatchende panelen, geïmproviseerde steunen. Dit vertelt het politieke verhaal zonder één regel dialoog.
- **Bewoning** — mensen laten sporen na: waslijnen, afgeschermde ramen, aanbouwsels, dingen die verboden zijn maar er toch staan. Een district zonder bewoningssporen is een set, geen plek.

**Patroonbreuk:** dezelfde mesh drie keer in één beeld is zichtbaar. Roteren volstaat niet — breek met een hero-prop, een uniek silhouet, of gerichte asymmetrie.

---

## 20.6 De werkwijze — ruimte-eerst, niet asset-eerst

Dit is de procedurele fix voor §20.1.

```
1. INTENTIE      Eén alinea: wat is deze ruimte, wie gebruikt hem, wat is hier gebeurd?
2. BLOCKOUT      Vorm, schaal, circulatie, sightlines. Grijze dozen. Loop het.
3. COMPOSITIE    Waar staat de camera? Focuspunt, drie lagen, silhouet vastleggen.
4. LANDMARK      Het dominante silhouet plaatsen en de rest eraan ondergeschikt maken.
5. FUNCTIE       Objecten die de ruimte nodig heeft om te werken.
6. VERHAAL       Objecten die vertellen wat hier gebeurd is.
7. SLIJTAGE      Gebruik, herstel, bewoning.
8. LICHT         Nu pas. Licht op een gecomponeerde ruimte, niet op een assetdump.
9. SHOTRONDE     Vaste camera's, art-reviewer, §20.7-poort.
```

**Stap 1 is niet optioneel en duurt vijf minuten.** Wie een ruimte niet in één alinea kan beschrijven, gaat hem met props vullen — dat is precies wat er misging.

**Objecten worden nooit geplaatst vóór stap 5.** Alles daarvoor is vorm, schaal en licht.

---

## 20.7 De poort — art-reviewer keurt hierop

Per player-facing view, GO/NO-GO:

| # | Check | NO-GO wanneer |
|---|---|---|
| 1 | **Fictie** | Eén aards object of aardse tekst in beeld |
| 2 | **Verantwoording** | Eén object zonder antwoord op "waarom staat dit hier?" |
| 3 | **Focuspunt** | Geen dominant silhouet; het oog dwaalt |
| 4 | **Lagen** | Minder dan drie dieptelagen in een wijde view |
| 5 | **Schaal** | Geen menselijk anker in een view die groot wil lezen |
| 6 | **Slijtage** | Alles nieuw, of alles gelijkmatig vies |
| 7 | **Herhaling** | Dezelfde mesh ≥3× herkenbaar in één beeld |
| 8 | **Opvulling** | Objecten die alleen leegte dichten |
| 9 | **Episch** | Minder dan drie van de vijf §20.4-verhoudingen |

**De strengste steekproef:** zet een screenshot naast een screenshot uit een game die de eigenaar mooi vindt. Niet om te evenaren — om te zién wat het verschil is. Dat verschil is bijna nooit polycount. Het is compositie, licht en schaal.

---

## 20.8 Open beslissing voor de eigenaar — de stijlvraag

**Dit moet Nathan beslissen en het is groot.**

De gelockte richting is **Borderlands-leunend gestileerd**: cel/toon-shading, inktlijnen, hoge detaildichtheid (`15_visual_quality_charter.md` §15.5, commit "lock Borderlands art direction"). Deze vraag is al één keer eerder opgekomen: het `OWNER_MANDATE` vroeg om "realistische materialen, natuurlijke kleuren", en dat conflict werd toen opgelost **in het voordeel van de stilering**.

De formulering *"realistisch geloofwaardig"* brengt hem terug. Twee mogelijke betekenissen, met heel verschillende gevolgen:

| Lezing | Betekenis | Gevolg |
|---|---|---|
| **A — geloofwaardig binnen de stijl** *(aanname)* | De toon-stijl blijft; het probleem is compositie en fictiebreuk. Dit document lost het op. | Geen herwerk. Alles hierboven geldt, stijl blijft gelockt. |
| **B — echt fotorealisme** | De Borderlands-lock vervalt. | Her-lock van §15.5, elk materiaal opnieuw, de toon-master vervalt, en de gestileerde assets die al binnen zijn moeten opnieuw beoordeeld. Weken werk. |

**Dit document neemt lezing A aan** — omdat de klacht ging over verkeersborden en opvulling, en dat zijn compositieproblemen die in elke stijl bestaan. Een gestileerde wereld kan volstrekt geloofwaardig zijn; Borderlands zelf is dat.

**Wil de eigenaar lezing B, dan is dat een expliciete her-lock-beslissing** en geen stille koerswijziging. Zeg het, dan wordt §15.5 herzien vóór er nog één asset landt.

---

## 20.9 Bronnen

- [Environment Art — The Level Design Book](https://book.leveldesignbook.com/process/env-art)
- [Environment Artist Playbook: From Blockout to Final Pass — RMCAD](https://www.rmcad.edu/blog/environment-artist-playbook-from-blockout-to-final-pass/)
- [Environment Design Principles for Immersive World Building](https://garagefarm.net/blog/environment-design-principles-for-building-immersive-worlds)
- [Environmental Storytelling in Video Games](https://gamedesignskills.com/game-design/environmental-storytelling/)
- [The Stages of Environment Art in Gamedev — 80.lv](https://80.lv/articles/the-stages-of-environment-art-in-gamedev)
