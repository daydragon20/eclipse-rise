# ANTWOORDEN VAN DE OWNER
*Nathan beantwoordt vragen met een knop op het dashboard; ze komen hier terecht.*
*Agents: LEES DIT ELKE SESSIE. Een antwoord hier is bindend en telt als owner-instructie.*
*Voeg nieuwe vragen toe aan `phase0/owner_questions.json` — dan verschijnen ze bij hem op het scherm.*

| Wanneer | Vraag | Antwoord | Toelichting |
|---|---|---|---|
| 2026-07-31 19:32 | O-7 | agent-kiest |  |
| 2026-07-31 19:33 | T-11 | true |  |
| 2026-07-31 19:34 | T-10 | gedaan |  |
| 2026-07-31 19:34 | T-2 | gedaan |  |
| 2026-07-31 19:48 | O-6 | A |  |
| 2026-07-31 19:48 | O-5 | volledig |  |
| 2026-07-31 19:48 | O-8 | vol |  |
| 2026-07-31 20:05 | O-6 | A | Via de chat, niet via de knop. De Borderlands-lock uit 15.5 blijft; geen her-lock, geen materiaal opnieuw, toon-master blijft. Deblokkeert de vormgeving van base en map (REFERENTIE_BASE_MAP 3). |
| 2026-07-31 20:10 | O-5 | volledig | Via de chat, niet via de knop. Het wapen gaat echt uit de karaktermesh: isoleren, los mesh per wapenfamilie, socket op hand_r, wisselogica eraan. Mijn advies was de goedkope tint-stap; de owner koos anders. |
| 2026-07-31 19:49 | T-7 | ja |  |
| 2026-07-31 20:12 | T-1 | nog-niet | Ik speel nog NIET. Eerst laat ik alle basisdingen bouwen waar je nu mee bezig bent. Fase 0 en 1 zijn hiermee afgesloten - wacht niet meer op mijn playtest, die staat niet langer in de weg. |
| 2026-07-31 20:12 | OBS-1 | wapen-omgekeerd | WAARNEMING: het wapen wordt OMGEKEERD vastgehouden. Klassieke socket-rotatiefout - de forward-as van de wapenmesh komt niet overeen met de orientatie van de socket op hand_r. Hoort bij O-5 volledig. Controleer de mesh-pivot en de socket-rotatie, niet de code. |
| 2026-07-31 20:12 | OBS-2 | geen-handen-1e-persoon | WAARNEMING: in first-person zie ik mijn handen niet. Klopt met wat al gemeten is - zodra het lichaam niet gerenderd wordt verdwijnt alles mee, inclusief het wapen. Een third-person game met een first-person modus heeft een APARTE armen-mesh nodig (first-person arms), of het lichaam moet zichtbaar blijven met de camera in het hoofd. Dat is een keuze, geen bug - leg hem voor als owner-vraag. |
| 2026-07-31 20:12 | OBS-3 | meer-wapens-beschikbaar | WAARNEMING: er is nu een tweede wapen maar het is weinig gedetailleerd, waarschijnlijk zelfgemaakt. Ik heb al VEEL wapens gedownload die je kunt gebruiken, en je mag ook zelf nieuwe assets ophalen. Bouw dus geen wapens meer met de hand als er een goed asset ligt. Let bij het kiezen op de fictiewet uit doc 20: geen aardse semiotiek, en alles door de toon-master. |
| 2026-07-31 20:12 | OBS-4 | hud-nog-niet-borderlands | WAARNEMING: de gegevens rond het scherm zijn nog niet Borderlands-achtig. Referentie met echte screenshots en element-voor-element vertaling staat nu in phase0/REFERENTIE_HUD_BORDERLANDS.md - inclusief Game UI Database en het officiele BL3-ontwerpproces. Waarschijnlijk was je hier al mee bezig. |
| 2026-07-31 21:01 | OBS-5 | schrijf-door | OWNER-VRAAG: wanneer wordt er doorgeschreven aan de scenes? ANTWOORD EN CORRECTIE: er zat een valse afhankelijkheid in het plan. Massaproductie wachtte op de ijkmissie-gate, die op stemgeneratie, die op de ElevenLabs-scopes. Een ontbrekend vinkje hield daardoor AL het schrijfwerk tegen, terwijl schrijven geen credit en geen API-aanroep kost. OPGEHEVEN: act 1 mag NU volledig geschreven en door de critic gehaald worden. dialogue-writer is 1x gespawnd voor M1.1 maar er is nog geen enkele regel geland (71 scenes, 0 regels). Spawn 4-6 dialogue-writers parallel op act 1 en laat dialogue-critic per s |
| 2026-07-31 21:09 | O-10 | 131000 | Verbatim: "het waren geen 310000 credits maar 131000, fout onthouden." Alle tiers opnieuw gesneden in ebc764e. Gevolg: ~17% van de gesproken tekst past deze maand in plaats van 40%; de Act-1-hub- en companiongesprekken vallen af. |
| 2026-07-31 21:37 | T-12 | gedaan |  |
| 2026-07-31 21:38 | O-11 | vervangen |  |
| 2026-07-31 21:39 | O-9 | B |  |
| 2026-07-31 21:39 | T-5 | mixamo |  |
| 2026-07-31 21:39 | T-6 | ja |  |
| 2026-07-31 21:50 | OBS-6 | nachtmodus | OWNER GAAT SLAPEN (31-07 laat). ER KOMT VANNACHT GEEN ENKELE REACTIE VAN HEM. Drie regels: (1) BLOKKEER NOOIT op een owner-actie. Loopt iets vast op zijn antwoord, zet het als kaart in owner_questions.json en ga meteen aan iets anders verder - er staat werk genoeg: HUD, wapen uit de mesh, de drie open dossiers, en de dialoog van act 1 die nergens meer op wacht. (2) VRAAG NIETS OVER DOWNLOADS. Zijn woorden: laat hem dat nu nog niet vragen, ik zal sturen wanneer hij het mag doen. De bestaande kaarten T-2 (Fab-packs) en T-8 (MetaHuman) blijven passief staan - hij klikt ze wanneer hij wil. Maak GE |
| 2026-07-31 21:55 | OBS-7 | script-is-engels | OWNER-VRAAG: in welke taal wordt het script geschreven? ANTWOORD: ENGELS, en dat is nu onmiskenbaar vastgelegd in 18_writing_standard.md 18.0. Geverifieerd op drie manieren: (1) 13_roadmap r45 noemt de lokalisatie als EN VO; FIGS+NL+PL+BR+RU+ZH text - Nederlands is dus een DOELtaal, niet de bron. (2) De casting filtert op language=en en auditeert op Engelse signature-regels. (3) Alle 71 act-1-scenes zijn al Engels. Nederlands is uitsluitend de taal waarin met Nathan gewerkt wordt: chat, dashboard, STATUS.md, JOUW_ACTIES.md, commit-berichten, note-velden. VUISTREGEL: kan de speler het zien of h |
| 2026-08-01 10:05 | O-16 | voorstelkolom | BESLIST 01-08. Bron: PROMPT_VOOR_AGENT.md regel 25 (Nathans eigen opdrachtdocument) plus phase0/O16_KEUZE.json met tien vastgelegde slots. Gemeten: check_voice_resolves.py exit 0 - elke spreker resolvet en geen twee sprekers delen een stem-ID. Afwijkingen per slot: eclipse_fighter:B Michael, dominion_conscript:B Miguel, veil_operative:B Madison Ray, C/D Beth en Arric, emissaris Marcus K. Deze regel is hier gezet omdat de beslissing wel in O16_KEUZE.json stond maar niet in dit bestand, waardoor het dashboard hem als open bleef tonen. |
| 2026-08-01 10:05 | O-14 | een-stem | BESLIST 01-08. Act 1 wordt gegenereerd met EEN Voss-stem; bespaart 20.570 credits. Bron: PROMPT_VOOR_AGENT.md regel 26. De tweede stem komt er alleen als phase0/VOSS_TWEEDE_STEM_AANVRAAG.json bestaat. NOG OPEN EN APART TE VRAGEN: of de vier persoonlijkheidsassen ook gehalveerd worden (varianten, ~8.900 credits) - daar heeft Nathan zich niet over uitgesproken, dus dat mag niet stilzwijgend meegenomen worden. |
| 2026-08-01 10:05 | O-17 | advies-volgen | BESLIST 01-08. De drie Kessara-burgers lenen een bestaande stem; de Dominion-officier krijgt een eigen. Maakt 12.833 credits en 8 scenes vrij. Bron: PROMPT_VOOR_AGENT.md regel 27. |
| 2026-08-01 10:05 | O-18 | optie-3 | BESLIST 01-08. Creditbedrag EN meetdatum in de statuskaart - een bedrag zonder datum veroudert stil. Bron: PROMPT_VOOR_AGENT.md regel 28. |
| 2026-08-01 10:05 | O-12 | 51 |  |
| 2026-08-01 10:06 | O-15 | sfeer |  |
| 2026-08-01 10:09 | O-20 | alle_tien_ok |  |
