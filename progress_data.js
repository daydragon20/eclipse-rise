// ECLIPSE voortgang â€” INSCHATTINGSDATA (handmatig, door de dev-sessie bij milestones bijgewerkt).
// De automatische feiten (commits, tests, verse screenshots) staan in progress_auto.js
// en worden door Tools/update_progress.ps1 gegenereerd â€” daar blijf je vanaf.
// Regels: percentages zijn eerlijk (liever te laag dan gejokt); geen HTML hier, alleen data.
window.PROGRESS_DATA = {
  bijgewerkt: "2026-07-25 17:30",
  hero: { label: "Hele game", pct: 8 },
  playtestChip: "ACTIVE_MILESTONE: Phase 2 (owner-instructie 23-07) Â· 13.2-playtest Phase 1: OPEN (standing owner-actie)",

  // Grove tijdsinschattingen â€” bewust ruw; door de dev-sessie per milestone bijgesteld o.b.v. tempo + resterende backlog.
  eta: {
    fase1: "~vrijwel af (97%) â€” resteert alleen jouw 30-min playtest (13.2)",
    slice: "Vertical Slice (Fase 2): grove schatting 2-3 weken â€” vandaag landden P2-01, P2-02 Stage A en P2-03 stap 1-2 in Ã©Ã©n dag; de grote onbekenden zijn jouw R3-verdict (stuurt Stage B) en de fidelity-ronde (P2-08)",
    totaal: "Hele game: grove schatting maanden â€” sterk afhankelijk van scope-keuzes en jouw playtests",
    toelichting: "Ruwe inschatting, geen belofte: gebaseerd op het huidige commit-tempo en de resterende backlog per fase. Wordt elke milestone bijgesteld.",
    bijgesteld: "2026-07-24"
  },

  // Wat de AI al van Nathan heeft ontvangen Ã©n verwerkt/nagekeken â€” zodat hij ziet dat zijn acties zijn aangekomen.
  // Dev-sessie: zodra je een owner-actie verifieert, verplaats hem van ownerActies naar hier met de tijd. Nieuwste bovenaan.
  jijGedaan: [
    { wat: "Editor gesloten â€” geverifieerd: nul UnrealEditor-processen, build-slot vrij. Meteen benut: de openstaande codewijziging is geland, het wave-2-migratiescript is gedraaid (M1.1 heeft nu zijn zero-casualty-bonus), en er is een echte bug gevonden die je eerste schone build zou hebben gebroken.", tijd: "2026-07-25 17:15" },
    { wat: "Old West uit de cache verwijderd â€” geverifieerd: geen Old West-map meer te vinden en de VaultCache bestaat niet meer. Je hebt mÃ©Ã©r opgeruimd dan gevraagd: vrije ruimte op C: ging van ~22 GB naar 172,6 GB. Nagekeken of er niets nodig is meegegaan â€” de asfalt-albedo die de districtsvloer gebruikt, de 904 MetaHuman-assets, de 11 civilians, de curatie-accepts en de generated meshes staan er allemaal nog.", tijd: "2026-07-25 16:45" },
    { wat: "Character-pack beoordeeld en AFGEWEZEN op jouw verzoek â€” je twijfel was juist. LPCharacters_FREE is niet alleen stilistisch mis (low-poly is expliciet verworpen in het stijlkader; Borderlands is hoog detail mÃ©t inkomtrek), maar heeft ook vijf LOSSE skeletons voor vijf figuren, dus geen gedeelde animaties, en maar zes takes waarvan Dance en Victory. De 11 CC0-civilians die al binnen zijn delen Ã©Ã©n armature per gender-lijn met 24 takes inclusief Interact en Wave. Het pack staat in Eclipse/Saved/RejectedAssets met de reden erbij â€” niets verwijderd, terugzetten is Ã©Ã©n handeling.", tijd: "2026-07-25 16:45" },
    { wat: "Drie environment-packs uit je wachtrij binnengehaald: Factory Pack Vol.1 (2,1 GB), Industrial Building 49 (66 MB) en UNIBLOCKS (3,8 GB). Die moeten nog door de curatie vÃ³Ã³r gebruik â€” dat is agent-werk, geen actie voor jou.", tijd: "2026-07-25 16:45" },
    { wat: "Beslissing 15.7 gemaakt: B (hybride MetaHuman-shading) â€” huid/body/teeth gaan door de toon-master, ogen + lacrimal + grooms houden hun eigen shader mÃ©t exposure-compensatie, zodat de koppen in close-up leven Ã©n op dezelfde belichtings-tier lezen als het district. Vastgelegd in phase0/metahuman_recipes.md; de tegenprestatie is Ã©Ã©n 15.8-shotronde op de vaste camera's als harde poort (naad huidâ†”oog). Faalt die, dan is A de terugval â€” niet C.", tijd: "2026-07-25 14:45" },
    { wat: "MetaHuman-gezichten binnengehaald â€” probe-bewijs (25-07 13:16): Frey (11:25), Hannah + Mason (al 23-07) Ã©n bonus Advika (12:59) staan erin, plus volledige garderobe (hoodie/cargopants/boots/5 shirts/tanktop) en 6 grooms; 904 assets totaal. De 'missing optional item'-meldingen bleken het optionele Creator-contentpack van de plugin zelf â€” niets van jouw downloads mist. Assemblies (/Game/MetaHumans) genereren + slot-koppeling per de recepten is agent-vervolgwerk (SentinelC-precedent).", tijd: "2026-07-25 13:16" },
    { wat: "Editor gesloten â€” build-slot vrij; de landing-pijplijn (build â†’ tests â†’ 6 commits â†’ data-scripts) is automatisch gestart", tijd: "2026-07-25 13:06" },
    { wat: "Overbodige Launcher-downloads geannuleerd â€” geverifieerd: geen enkel cancel-lijst-item (Paragon-helden/JRPG-muziek/NWIRO) in de download-cache, keep-lijst wÃ©l aanwezig", tijd: "2026-07-25 11:25" },
    { wat: "Westgevel-kleur A/B gekozen (jij koos B)", tijd: "2026-07-23" },
    { wat: "Blender geÃ¯nstalleerd / goedgekeurd", tijd: "2026-07-23" },
    { wat: "ElevenLabs Music/SFX-scopes aangezet", tijd: "2026-07-23" },
    { wat: "Ingelogd op Fab + Epic Launcher", tijd: "2026-07-23" }
  ],

  // Wat de owner concreet moet doen â€” apart paneel bovenaan. Dev-sessie houdt kort + actueel; leeg [] = niks te doen.
  ownerActies: [
    { titel: "TIP tijdens je test: er is nu een testroute voor de missies", prio: "optioneel", waarom: "je hoeft de campagne niet te spelen om objectives na te lopen. phase0/TESTROUTE_OBJECTIVES.md heeft alle 16 debug-commando's (uit de bron geverifieerd) en M1.1 uitgeschreven van start tot debrief, met per stap wat je hoort te zien. Console open je met de tilde-toets.", stappen: [
      "Interessantste test: laat iemand neergaan (of Eclipse.Roster.Kill) en win dan alsnog — de +20 materiaal-bonus moet dan NIET komen",
      "Werkt iets niet? Eclipse.Events.Dump laat zien of het event wel gevallen is; dat scheelt raden",
      "Ziet iets er anders uit dan in het document staat: dat is een echte bug, geen documentatiefout — meld het gewoon"
    ] },
    { titel: "Vier packs: klik de tweede stap ('Add to Project'), niet alleen Download", prio: "gauw", afgekeurd: "Nog niet gelukt, en ik denk dat ik weet waarom. Ik heb goed gezocht: niet in Eclipse/Content (nieuwste map is Sci-Fi Hallway van 16:42), niet in de engine-plugins (nieuwste zijn van 23-07), niet in de Plugins-map van het project. De Launcher was wÃ©l actief om 17:12, dus je hebt iets geklikt. Fab heeft twee stappen: 'Download' zet een pack in je bibliotheek, en pas 'Add to Project' kopieert het naar Eclipse. Vermoedelijk is die tweede stap niet gebeurd (of nog niet klaar).", waarom: "Sci-Fi Light Pack is de belangrijkste van de vier: de art-review vraagt om zichtbare lichtbronnen, en op een onverlicht district zijn dat emissieve fixtures. Footsteps en muzzle flash zijn gameplay-feedback en blokkeren de kit-pass niet.", stappen: [
      "Fab â†’ My Library â†’ zoek Sci-Fi Light Pack",
      "Klik 'Add to Project' (NIET alleen Download) â†’ kies Eclipse",
      "Als er geen 'Add to Project' staat: zeg het, dan zoek ik een andere route",
      "Zeg daarna: 'lightpack staat erin'"
    ] },
    { titel: "Feel-gauntlet Command Mode (~20 min) â€” het R3-verdict", prio: "nu", waarom: "beslist of Stage B (nieuwe orders/camera/UI) mag starten; expres op de lelijke build. Je mag de editor hiervoor gewoon weer openen â€” de landing draait nu eerst; alleen als de agent later opnieuw moet bouwen vraagt hij je nog eens te sluiten", stappen: [
      "Open phase0/FEEL_GAUNTLET_P2-02.md (het draaiboek met besturing + meetcriteria)",
      "Speel in PIE: houd Q (of LB op de pad) â€” wereld vertraagt naar 30%; geef orders met 1-4/D-pad; kies soldaten met Tab/RB of E/X",
      "Score de 5 criteria uit het draaiboek en zeg: 'R3-verdict: true' of 'false' + wat je zag"
    ] },
    // Civilian/worker-pack: overgenomen door de agent â€” 11 CC0-bodies binnen (zie takenlijst); alleen deze optionele close-up-kliks resten:
    { titel: "13.2-playtest (~30 min)", prio: "als je tijd hebt", waarom: "sluit Fase 1 af + stuurt de eerste review", stappen: [
      "Speel de graybox-loop ~30 min in de editor",
      "Noteer kort wat goed/slecht voelt",
      "Geef het door aan de agent"
    ] }
  ],

  // Assets-overzicht â€” wat is binnengehaald, zelf gemaakt, en wat nog te doen. Dev-sessie houdt dit actueel.
  assets: {
    gepakt: [
      "8Ã— RAISOR SciFi-soldier packs (Fab) â€” squad- Ã©n vijand-bodies via DT_BodyDefs",
      "Paragon: Lt. Belica (Fab) â€” speler-body",
      "Paragon: Minions (Fab) â€” extra bodies",
      "Quaternius CC0-characters (rigged, mÃ©t anim-sets) â€” eerste bewoners",
      "Poly Haven CC0-texturen: asfalt, beton, golfplaat, metaalplaat",
      "Poly Haven CC0-props: vat, wegbarriÃ¨re, krat",
      "ambientCG CC0-materialen: Metal041B, Metal063, Concrete042A, CorrugatedSteel007A",
      "1082 gratis UE-assets klaar in Fab-library (reserve, nog niet geÃ¯mporteerd)"
    ],
    gemaakt: [
      "Toon-materiaal M_EclipseToon (cel-banden + penseel-hatching)",
      "Lit-variant M_EclipseToonLit + decal-variant M_EclipseToonDecal",
      "Ink-outline post-materiaal PP_EclipseInk (Laplaciaan, schone silhouet-/naadlijnen)",
      "Pillow-decals: AEGIS-propaganda, hazard-strepen, verzets-stencils, 7 waarschuwingsplacards",
      "Kessara-skyline in code: 56 fabriekshulks + 18 schoorstenen + 12 kraanportalen (seed 503)",
      "Eerste gebouw-kits + vegetatie geplaatst in het district",
      "Zelf-gegenereerde texturen: hazard-chevron (worn), stain-mask-falloff",
      "8 stem-regels via ElevenLabs (gegenereerd + gecachet, 0 herhaal-kosten)",
      "Straat-dressing: rijstrook-markering, 14 olie/roestvlekken, natrium-checkpointstrips"
    ],
    teDoen: [
      "MetaHuman-gezichten: Kaya, Brick, Vale, Dex, Petra, Kaine (recepten klaar)",
      "env-packs importeren: Factory Pack Vol.1, Industrial Building 49, UNIBLOCKS, Sci-Fi Hallway, Sci-Fi Light Pack",
      "Wapen-meshes: FPS Weapon Bundle, Free Muzzle Flash",
      "Meer gebouw-variatie + interieurs, meer vegetatie-soorten",
      "Music- + SFX-endpoints (ElevenLabs) aanzetten",
      "VFX, weer & dag/nacht"
    ]
  },

  // Live takenlijst van de AI-dev-sessie â€” status: bezig | wachten | klaar | gepland.
  taken: [
    { taak: "Autonomy-loop actief", status: "bezig", pct: 100, detail: "CYCLUS 5 GELAND (25-07 13:15, direct na jouw editor-close om 13:06): unie-bar eerst â€” build âœ“, 68/68 tests (0 fail), validatie 3 validators/0 fouten, catalog 28/28 â€” daarna de vijf review-groene changesets gecommit: 6ac96c1 [Strategy] story-resolver+validatiepass, 6b08bb2 [Art] dressing-1, 173d4ec [Quests] debrief-beat, d503651 [Base] stap 4-5A, 9be3786 [Strategy] liberation-core. Commandlet-rij daarna KLAAR: M1.1-data d7f22e4, MetaHuman-probe (jouw taak afgetekend), civilians b4d3510 â€” alles gepusht. CYCLUS 6 GELAND (15:30): vijf commits op een 85/85-bar (validatie 4 validators/0 fouten, catalog 29/29) â€” e8bd5c8 M1.1-Gauntlet op de Ã©chte geshipte data, b068212 liberation-trigger + validator + datascript, 67f46a6 Taak-4-kern (alarm als benoemde sub-fase + optional-payouts atomair in de debrief), c6cf7fe walkable vault mÃ©t parity-Gauntlet, 966c800 dressing-iteratie 2 (licht via luminantie-decals). Reviewronde: 5 lenzen over de changesets, en elke zware bevinding ging langs een tegenlezer die hem moest wÃ©Ã©rleggen â€” 13 sneuvelden, 1 bleef staan en is gefixt: een test die groen bleef terwijl de liberation stil dood kon zijn. VOLGENDE: decal-generatie + shotronde in de slot, dan wave 2 (optional, recap + cold-reader, civilian-wiring)." },
    { taak: "P2-01: Squad van 4 + classes", status: "klaar", pct: 100, detail: "GELAND (1a85b78, review GO): squad van 4 met Assault/Medic/Sniper als data (DT_ClassDefs â€” geen subclasses), ClassId op het roster mÃ©t save v2â†’v3-migratie + v0/v2-fixture-tests in hetzelfde commit (14.3.6/R6), medic-auto-triage incl. her-dispatch naar een tweede casualty (review-M1-fix, anti-shuttle-peek), nieuwe ClassDefs-validator in EclipseValidateData. Eindstand: build âœ“, 38/38 tests âœ“, validatie 3 validators/0 fouten âœ“, catalog 21/21 âœ“. Follow-ups (klein): m4 spawn-fan â†’ tuning, m6 cover-scorerconstanten â†’ tuning, m7 catalog-formulering." },
    { taak: "Stap 3: MetaHuman-pijplijn", status: "bezig", pct: 85, detail: "Curatieronde KLAAR (24-07): SentinelC (232 assets) bleek intern geauthored voor /Game/CharacterAssemblies/SentinelC â€” map verplaatst (omkeerbaar, niets verwijderd), waarna probe groen: body- Ã©n face-skeleton resolven. Kaya-slot PROVISIONEEL gekoppeld aan SKM_MHC_Little_00_BodyMesh via Tools/setup_metahuman_data.py (bewijs: female/medium/normalweight-baseskel + kort praktisch haar; fallback Rebel_B intact; MH_Kaya per recept vervangt dit). GELAND (15dfe61, review GO): incl. single-writer-fix (setup_character_data.py is nu create-only op de named-tabel â€” kan de Kaya-link nooit meer stil wissen) en de .gitignore-dekking. De 47/48-fail bleek de dag-regel uit de [Quests]-lane en is dÃ¡Ã¡r gefixt (unie daarna 48/48). Follow-ups genoteerd: named-table-validator in EclipseValidateData (volgende iteratie), SystemExit/try-except-cosmetica in het script. Atira_LODSettings + Locodrome: KEEP. 15.7 BESLIST (owner, 25-07 14:45): B = hybride â€” huid/body/teeth door de toon-master, ogen/lacrimal/grooms eigen shader met exposure-compensatie; harde poort = Ã©Ã©n 15.8-shotronde op de naad huidâ†”oog (faalt die â†’ A, niet C). Daarmee is de gezichts-tier bouwbaar geworden en staat de implementatie-spec al klaar (phase0/MH_FACE_TIER_B.md). Twee vondsten daaruit: (1) de bestaande restyle-lus is slot-blind â€” hij vervangt Ã©lk materiaalslot op index en zou dus ook de ogen overschrijven (dat Ã­s optie A); B vraagt slot-classificatie op naam, met de oog-familie als eerste test. (2) De body-gain 3.2 is bewust ongemeten met als reden 'bodies zijn klein op het scherm' â€” bij een dialoog-close-up valt die reden weg, dus het gezicht krijgt zijn eigen gemeten gain. Zelfde soort fout als de magenta-container, Ã©Ã©n tier hoger. WAVE-2-PROBE KLAAR (25-07 13:16, rapport-only): Frey/Hannah/Mason/Advika + garderobe + 6 grooms binnen (904 assets, skeleton-bewijs gelogd; Nathans download-taak afgetekend); /Game/MetaHumans nog leeg â€” VOLGENDE: assembly-run + match-beslissing per phase0/metahuman_recipes.md en dan de aparte link-run op DT_NamedCharacters (single-writer, SentinelC-precedent)." },
    { taak: "Audio-import + koppeling (16.12)", status: "klaar", pct: 100, detail: "IMPORT GELAND (937f35f) + RUNTIME-KOPPELING GEBOUWD (element-builder, build âœ“ -NoUba, nog niet gecommit): (1) Layer-1-bed als Ã‰Ã‰N 2D-AmbientSound in de graybox-mood-pass â€” bewuste keuze boven attenuated randbronnen: never-silent (16.7) wordt structureel i.p.v. falloff-radii tunen; de cue draagt ATT_Ambient_Bed baked-in, dus de component override't attenuation expliciet uit (anders stille hoeken); ontbrekende cue = logregel + stil district (14.3.5). Volume 0.35 als named constant (mix-DA komt met het 16.7-lagensysteem). (2) UEclipseAudioSubsystem (GameInstance, Source/Eclipse/Audio/) subscribet type-checked op Event.Mission.Completed en speelt de verzets-sting via PlaySound2D (vol. 0.9, named constant); audio = pure bus-consumer, geen nieuwe events; catalog-rij Mission.Completed heeft nu 'audio sting (16.12)' in de consumer-kolom. (3) Nieuwe headless test Eclipse.Audio.Subsystem.BusContract (bind/consume/unbind, re-bind lekt niet, Failed/Started triggeren geen sting). GELAND (6b48ba4, review GO; suite 57/57): incl. review-fix tag-scoped stale-bed-cleanup (mid-play-rebuild kan het bed nooit stapelen). Follow-ups: 16.x-citatiefixes in de scripts, /Game/Audio-structuur vs 16.14, mix naar 16.7-DataAsset + Layer-2-spots." },
    { taak: "Groene bar bewaken", status: "bezig", pct: 100, detail: "Elke iteratie: build (-NoUba) + 31/31+ tests + validatie 0 + catalog vÃ³Ã³r elke commit/push." },
    { taak: "Env-packs gecureerd (Factory Pack is de winnaar)", status: "klaar", pct: 100, detail: "GEMETEN in de editor (25-07 17:17, rapport-only script): de vier packs die je binnenhaalde zijn beoordeeld op de criteria die écht beslissen — driehoek-aantallen en materiaalslots, die alleen in de editor leesbaar zijn. UITKOMST DIE MIJN EIGEN EERSTE GOK WEERLEGT: ik had Uniblocks 'de kit-kandidaat' genoemd op basis van het aantal meshes (3200), maar 2506 daarvan hebben één materiaalslot. Eén slot kan geen kleurscheiding per zone dragen — voor een vat is dat prima, voor een wandpaneel dat wand, trim en lijst in verschillende tinten nodig heeft is het fataal, en dat is precies het 'stickers'-defect. FACTORY PACK VOL.1 is de echte kandidaat: vier keer betere slot-verdeling en geen enkele mesh boven het budget. Uniblocks blijft bruikbaar als massa en kleine props, per stuk te controleren. Sci-Fi Hallway (18 van 19 één slot) geparkeerd tot de vault een art-pass krijgt. IBuilding_49 afgewezen: één mesh van 26.385 driehoeken met één slot, te zwaar voor een SM5-laptop. Detail + werklijst in phase0/CURATIE_ENVPACKS_2026-07-25.md." },
    { taak: "Wereld zonder einde (Borderlands-compositie)", status: "gepland", pct: 5, detail: "Staande owner-taak (23-07): elke review-cam toont een gevulde horizon â€” geen zichtbaar wereld-einde; horizon-check van de plaatsingsronde was schoon op alle 6 cams. Skyline-audit + gelaagde silhouetten/landmarks liften mee in de 15.8-dressing-ronde (na pack-slim, zelfde builder-lock)." },
    { taak: "Pack-slim-ronde (migrate + drop)", status: "klaar", pct: 100, detail: "KLAAR (24-07 16:50): alle 11 curatie-accepts (A1/A2/A3/C1 + SciFi10-albedo's 1/2/5/6/7/9/10) via Tools/migrate_curation_accepts.py naar repo-tracked /Game/Art/Imported (~2,4 MB), tri/4K-geverifieerd, dep-audit 0 pack-refs; Minions+SciFi10 van schijf = 5.881 MB vrij (bewijs: ASSET_CLEANUP.md Â§7-10, scan 631 binaries 0 hits). 3 redirector-stubs (4 KB) houden de runtime-graybox pixel-gelijk. Groene bar in de commandlet-slot: tests 47/47 (0 failed, 11 met bekende fixture-warnings), validatie 3 validators/0 fouten, catalog 23/23. ENIGE follow-up: 15.8-dressing-ronde swapt EclipseGrayboxBuilder.cpp:161/720/741 naar de Imported-paden (Source-lane), daarna mogen de stubs weg. Commandlet-slot is VRIJ." },
    { taak: "P2-02 Stage A: Command Mode debug-feel (R3)", status: "klaar", pct: 100, detail: "GELAND (b5aa157, review GO): hold Q/pad-LB â†’ 0.30-dilatatie via pure state-machine + fail-safe-wrapper (elke exit â†’ exact 1.0; tick alleen tijdens hold), per-soldier-selectie (cycle Tab/RB/scroll/LT + reticle-pick E/X, range uit DA_CommandModeTuning â€” review-MAJOR gefixt) door bestaand IssueOrder, Event.Command.ModeEntered/Exited + catalog 23/23, debug-HUD, Eclipse.Command.Dump, 4 headless tests. Eindbar: 47/47 âœ“, validatie 0 âœ“. NU AAN NATHAN: de feel-gauntlet (~20 min, R3-verdict) â€” draaiboek phase0/FEEL_GAUNTLET_P2-02.md. Stage B pas nÃ¡ verdict 'true'." },
    { taak: "Civilians/workers: gestileerd gratis pack zoeken of zelf bouwen", status: "bezig", pct: 85, detail: "GEÃMPORTEERD (25-07 13:19, headless in de vrije slot): alle 11 CC0-bodies staan als engine-assets in /Game/Art/Characters (Worker/Farmer/Hoodie/Casual/Suit/Punk/Formal M+V â€” elk 26 assets: mesh + skelet + 24 anim-takes incl. Interact/Wave), bewust zÃ³nder pack-materialen (15.5: alleen palet-MID's straks). Provenance in Content/Art/Characters/SOURCES.md (CC0, Quaternius Ultimate Modular). Fab-shortlist close-ups blijft optioneel klikje (Bubba + Casual FREE). RESTEERT: builder-wiring met palet-MID's + Â§15.8-shotronde â€” eigen [Art]-mini in cyclus N+1 wave 2 (nÃ¡ EB-4-dressing, spec DRESSING_ITERATIE_2.md)." },
    { taak: "15.8-dressing-iteratie 2 â€” geland, art-review vond de Ã©chte oorzaak", status: "bezig", pct: 55, detail: "GELAND (966c800 + maskers 0b405ae) en meteen beoordeeld op 7 nieuwe shots. WINST: het licht-recept werkt â€” de lamp-pools lezen op afstand als warme lichtpunten in de straat, zonder zichtbare decal-rand, en de gemeten helderheidsverhouding klopt exact op het ontwerp. Shot 00069 is gebankt als recept-referentie. MAAR de reviewer mat de frames terug en vond iets dat de hele iteratie herschrijft: het beeld rendert alle kleuren 1,8x helderder dan ze zijn ontworpen, omdat de automatische belichting zich vastzet op de vloer (die 60-80% van elk frame vult). Daardoor werd de 'schemer-vloer' in werkelijkheid 16% LICHTER in plaats van donkerder â€” de kleurcorrectie was een no-op. Zolang die belichting vrij loopt, is de vloer niet te sturen. Alle zes frames: nog niet AAA-ready. Ook gevonden: de lampkoppen zijn 3,9x DONKERDER dan hun eigen lichtplek (bron onzichtbaar), de magenta container zit er nog Ã©n de eerdere diagnose was fout (de oorzaak is de donkere oxide-tint zelf, niet de textuurmix), en er staat een marketplace-showroom-draaischijf als dock-sokkel in beeld. Volledige fixladder met gemeten getallen staat in phase0/DRESSING_ITERATIE_3.md â€” belichting pinnen is stap 1 en de voorwaarde voor al het andere. STAP 1+2 AL GELAND (e97ea1d, 16:15) en nagemeten: de vloer is 3,9x donkerder en staat nu op 1,52x de lucht in plaats van 3,41x erboven â€” hij reageert dus weer op wat er ontworpen is, en niets is dichtgeknepen. Er is ook een meetgereedschap gebankt (Tools/measure_frame_values.py) zodat 'te licht' voortaan een getal is; dat ving meteen een verkeerde conclusie van mijzelf: ik dacht dat de grond naar de horizon oplichtte en heb daarvoor de mist verhoogd, maar een schone meting liet zien dat de vloer vlak is (~7% variatie) en dat de mist-wijziging niets deed â€” die is teruggedraaid en de les staat in de spec. ITERATIE 3 LOOPT EN LEVERT (17:05): de lamp is nu zijn eigen helderste punt (sodium-oranje, geklipt rood) in plaats van de witte barriÃ¨re â€” de bron is zichtbaar. Het waardeplafond is gezakt: de barriÃ¨re ging van 0,65 naar 0,29 en staat daarmee niet langer boven zijn eigen licht, en de perimeterband staat nu op 1,50x de vloer in plaats van 2,6x. En de magenta container is eindelijk Ã©cht weg: van 31.170 magenta pixels naar 0, doordat de derde poging de echte oorzaak raakte (de donkere oxide-tint had blauw boven groen; de regel 'B â‰¤ G' geldt nu ook voor de midtinten, niet alleen de shades). De marketplace-showroom-draaischijf die als laaddok in beeld stond is vervangen door gewone blokken â€” die verraadde zijn bron met leesbare maatstreepjes. Onderweg heb ik drie keer een eigen verkeerde conclusie moeten terugdraaien (mist, kruis-billboard, en een te donkere muur); alle drie staan met de meting in de spec zodat niemand ze herhaalt. RESTEERT: barriÃ¨re Ã©Ã©n stap verder omlaag, de lichtplek zelf herderiveren, halo/bloom, en contactschaduwen." },
    { taak: "15.8-dressing-ronde 1 (Imported-accepts) â€” afgerond", status: "klaar", pct: 100, detail: "GELAND (6b08bb2, 25-07 13:10; code-review GO, 0 blockers); shotronde 00056-00062 gedraaid + art-review binnen (25-07 11:30). Winst: gantry-truss-plating, hazard-decals, sandbags lezen als Ã©Ã©n toon-stijl; cams 6/7 bankbaar. VÃ“Ã“R commit gefixt: log-opsomming, 3Ã— warning-hoist, magenta-container â†’ echt oxide. VOLGENDE ITERATIE (prioriteitsladder art-review): (1) licht+vloer-pass (schemer-consistentie, lamp-pools, contactschaduwen â€” tilt alle cams), (2) toon-master over de plaza-vloer + graybox-restanten in gedresste frames, (3) dock-naad/boulder-clip/boulder-lichter/machine-faces 2-3Ã—/buttresses dieper+donkerder; cam-'dupe' verklaard: shot 1 is het bewuste warm-up-offerframe van de rig (15.9-comment) â€” reviewinstructie: eerste PNG overslaan. Iteratie-2 is executie-klaar gespect in phase0/DRESSING_ITERATIE_2.md â€” kerninzicht: het district is unlit, dus lamp-pools/grounding gaan via luminantie-decals + blob-shadows en de vloer-fix via de toon-master, niet via echte lichten." },
    { taak: "P2-04 stap 2: story-laag + offer-precedence", status: "bezig", pct: 90, detail: "BEDRADING + DEBRIEF-BEAT GELAND (25-07 13:10): 6ac96c1 = ResolveOfferForRegion als het ene offer-pad (pin wint), 5 native Story.Beat.*-tags, per-tabel-validatiepass (wrong-struct/lege beat/orphan-pin/double-pin, luid en 1Ã— per tabel), map her-rendert op BeatReached; 173d4ec = completion-beat atomair in de debrief-transactie (skip-if-set load-bearing; verlies committeert nooit story-voortgang). Eerder: StoryFlags v5 (fd38933) + DT_StoryMissions-schema (16ce2bd). NU in de commandlet-rij: setup_story_missions.py (MT_M11 met 2 mandatory objectives â€” de zero-casualty-optional wacht eerlijk op de OptionalReward-schemastap, Taak 4). VOLGENDE: Taak 4 (PhaseChanged + alarm-latch + optional-rewards + recap-materialisatie, spec phase0/TAAK4_STORY_SURFACE.md); M1.2-authoring pas na cold-reader 4/4." },
    { taak: "SPEC-P2-04 stap 1: R7-falsificatie (M1.1-skelet)", status: "klaar", pct: 100, detail: "R7 = GROEN, GELAND (60014e6 + [Art]-swap 7036849, gepusht): de missie-runtime draagt aantoonbaar authored missies â€” authored objectives actief (geen synthesized fallback), mandatory-set afgedwongen, rewards gecommit, regio onaangetast, klok +1 dag. Review-blocker (wond-assert off-by-one) + GC/shadow-hygiÃ«ne (uniek MT_M11Skeleton-id, package-flags) in dezelfde changeset gefixt; de debrief-dag-regel (P2-03 locked decision 4) geldt nu ook mechanisch mÃ©t verlies-pad-dag-assert. Unie-bar bij landing: 48/48, validatie 0, catalog 27/27. M1.2-M1.4-authoring is hiermee vrijgegeven (bouwstap 2 = DT_StoryMissions + M1.1 op het bewezen laadpad)." },
    { taak: "P2-03: Hollow Point (stap 4-5A GELAND; vault volgt)", status: "bezig", pct: 93, detail: "STAP 4-5A GELAND (d503651, 25-07 13:10, review GO): alle 6 verankerde stap-3-bevindingen â€” Kill/Wound strippen AssignedSoldierIds in dezelfde apply (ghost-analist dicht, StaffAssigned(none)-facts uitsluitend uit de commit), muster weigert assigned soldiers (persoon+post in de melding), MaxStaffPerSite gestamped Ã©n afgedwongen in de mutatie-laag (no-op-reject + hash-gelijk getest), dag-N-completion-yieldt-dag-N gepind, StampBaseTuning-early-out, negatieve-yield-filter gedocumenteerd; 3 nieuwe base-tests. Eerder: stap 3 (69b5f4b) = transactie-API + Event.Base.* + Hollow Point-data. RESTEERT stap 4-5B (cyclus N+1, EB-3): walkable vault in NIEUW Base/EclipseVaultBuilder.*, parity-Gauntlet groen VÃ“Ã“R menu-hub-retirement. Follow-ups: Barracks-rostercap 11â†’12 (data-driven), muster-UI-greying (P2-07)." },
    { taak: "P2-05 liberation: pure core GELAND, wiring volgt", status: "bezig", pct: 55, detail: "PURE CORE GELAND (9be3786, 25-07 13:10, review GO na B1/M1/M2-fixes): FEclipseLiberationRow spec-conform (TriggerMissionId/NewOwner/ContextLine), ResolveLiberationTransaction (monotoon, row-volgorde, unknown/duplicate ids gedropt+gerapporteerd â€” een typo mag de atomaire commit nooit wholesale laten klappen), IsLiberationTriggered (beat-gate op echte StoryFlags), IsLiberationComplete (state-derived); 6 tests incl. het verplichte no-op-reject-contract. Spec zelf was al ACCEPTED (24-07, Foothold-trio, geen nieuwe events). RESTEERT (cyclus N+1, EB-2): stap-1-restant (setup-slot + validatie + Foothold-row via setup_liberation_data.py) + stap-3-wiring (subsystem-koppeling; missing table/row = warn never throw, nooit een lege transactie committen) + F1-re-entrancy-Gauntlet + regressie 'M1.1-completion flipt niets'." },
    { taak: "15.8 dressing-ronde: rubble/warehouse + watch-items", status: "gepland", pct: 0, detail: "NA pack-slim (zelfde builder-lock, serieel): A3-slagkei + C1-brokstukken (3-5Ã— geÃ¯nstanced Ã³nder A3) als Contested-rubble tegen de perimeterwal, A1-plinth als loading-dock in de warehouse-yard; SciFi10-slots 5/6/7/9/10 (warehouse-binnenwand, machine-bank, BldgA-trim, treadplate-ramps, gantry-grid) en de 3 ongeplaatste ambientCG-albedo's (Metal046B 19.76, Concrete042A 8.94, Metal063 6.42) op hun curatie-bestemmingen â€” gains al gemeten, niet schatten. In dezelfde ronde de watch-items: teal-fin cam 6, perimeterband cam 4, crosshatch-tiling + horizon-audit, Ã©n (pack-slim-review NTH) Ã©Ã©n cold-DDC-open van de 7 gemigreerde SciFi10-albedo's als fresh-machine-check. De 3-string-swap in EclipseGrayboxBuilder.cpp:161/720/741 loopt intussen al als aparte [Art]-mini bij main (in flight, landt samen met de R7-changeset); de stubs blijven staan tot dÃ©ze dressing-shotronde bewijst dat niets meer via de oude paden loopt â€” dan pas weg. Shotronde + art-review per 15.8 vÃ³Ã³r commit." },
    { taak: "Wachtrij eigenaar", status: "wachten", pct: 0, detail: "1) MetaHuman-items: 1Ã— Download-klik per item in Window â†’ Fab â†’ My Library (login werkt) Â· 2) MH_<Naam>-gezichten per phase0/metahuman_recipes.md Â· 3) env-pack-pulls: Factory Pack Vol.1, Industrial Building 49 PBR, UNIBLOCKS, Sci FI Hallway, Sci-Fi Light Pack, Auto Footsteps Utility, Niagara Footstep VFX, FPS Weapon Bundle, Free Muzzle Flash Â· 4) CHARACTER-GAT (enige!): 1 gestileerd civilian/worker-pack, 4â€“6 bodies, Mannequin-rig, mÃ©t arbeider-varianten â€” voor Hollow Point-crew/idlers (SPEC-P2-03) en Kessara-burgers close-up Â· 5) 13.2-playtest (~30 min, stuurt R1) Â· 6) NU KLAAR OM TE SPELEN: P2-02 Stage A feel-gauntlet (~20 min, het R3-verdict â€” draaiboek phase0/FEEL_GAUNTLET_P2-02.md, staat bovenaan je actielijst). KLAAR: Fab-login âœ“, ElevenLabs-scopes âœ“, Blender âœ“, westgevel-A/B beslist (B, doorgevoerd 412f14f) âœ“, GameAnimationSample mag weg (0 refs)." }
  ],

  // A/B-keuzes die de owner in de viewer nakijkt â€” status: open | beslist; keuze vult de dev-sessie in na owner-antwoord.
  abTests: [
    { titel: "Westgevel-kleur (checkpoint-muur, cam 1)", status: "beslist",
      vraag: "De westgevel leest zalmroze doordat hij nÃ©t in de mid-band van de cel-shading valt (ndl +0.52, BandHi 0.55); de zuidgevel leest verzadigd oxide â€” Ã©Ã©n gebouw oogt als twee assets. Welke variant leest beter als Ã©Ã©n gebouw, zonder de andere cams te schaden?",
      opties: [
        { label: "A â€” banddrempel 0.50", img: "progress_media/ab_westgevel_A.png", uitleg: "westgevel schuift de lit-band in en wordt pixel-gelijk aan de zonzijde (gemeten 251,160,53 â‰ˆ 252,158,55); werkt district-breed, exposure-delta â‰¤2 op alle zes cams" },
        { label: "B â€” per-gevel tint-compensatie", img: "progress_media/ab_westgevel_B.png", uitleg: "alleen de gecompenseerde gevel wordt goed; de compound-gevel verderop en elke andere westgevel blijven zalmroze â€” schaalt niet" },
        { label: "referentie (oude stand)", img: "progress_media/ab_westgevel_ref.png", uitleg: "gebankte ronde vÃ³Ã³r de fix: westgevel zalmroze (236,126,88), zuidgevel oxide â€” Ã©Ã©n gebouw leest als twee assets" },
        { label: "B â€” DOORGEVOERD (eindstand)", img: "progress_media/ab_westgevel_B_eindstand.png", uitleg: "jouw keuze district-breed: ook de compound-gevel door de gap (was zalm 236/120/88, nu warm 251/155/53), BldgB-west en de skyline-familie; 38/38 tests, validatie 0" }
      ],
      aanbeveling: "A werd aanbevolen op de metingen; de keuze is aan de owner en die koos B.", keuze: "B â€” per-gevel warmte (owner-keuze 2026-07-23), district-breed DOORGEVOERD (BandHi 0.55, compensatie op alle westgevels â€” zie eindstand-shot). Gecommit: 412f14f (code GO + art GO)." }
  ],

  secties: [
    {
      titel: "1 Â· Roadmap", scope: "Part 13",
      items: [
        { naam: "Fase 0 â€” Pre-productie", pct: 90, doel: "2026-07-24", groot: "Fundament: alle systeem-architectuur opgezet, de Borderlands-art-richting gelockt, en de tools klaar. De basis waar alles op bouwt.", notitie: "Open: CI-runner, concept-art, feel-clips.", nodig: { type: "wachten" } },
        { naam: "Fase 1 â€” Prototype \"The Loop\"", pct: 97, doel: "2026-07-25", groot: "Bewijzen dat de kern-loop (missie â†’ squad â†’ gevecht â†’ basis) Ã©cht leuk speelt â€” op blokken (graybox). Bewust nog geen mooie graphics; die volgen pas als het spel leuk is.", notitie: "Vrijwel af; resteert de 13.2-playtest.", nodig: { type: "owner", stappen: ["Speel de graybox-loop ~30 min in de editor (PIE)", "Noteer kort wat goed/slecht voelt", "Geef het door aan de agent â€” dit sluit Fase 1 af"] } },
        { naam: "Fase 2 â€” Vertical Slice \"Thirteen Bullets\"", pct: 18, doel: "2026-08-12", groot: "HET grote moment: het eerste district (Kessara) in vÃ³lle Borderlands-kwaliteit â€” echte gebouwen, characters met MetaHuman-gezichten, gevechts-feel. Hier zie je voor het eerst hoe de game er echt uit gaat zien.", notitie: "P2-01 (squad van 4 + classes) GELAND; nu P2-02 Stage A + P2-03 parallel.", nodig: { type: "owner", stappen: ["MetaHuman: 1Ã— Download per item in Windowâ†’Fab, dan agent laten inpluggen", "1 gestileerd civilian/worker-pack toevoegen (enige character-gat)", "Westgevel-kleur A/B kiezen zodra de shots in het paneel staan"] } },
        { naam: "Fase 3 â€” Early Build", pct: 0, doel: "2026-08-31", groot: "De wereld wordt groot: meer planeten + missies uitgerold, allemaal in de Borderlands-stijl. De vertical-slice-kwaliteit wordt over de hele game uitgesmeerd.", nodig: { type: "wachten" } },
        { naam: "Fase 4 â€” Alpha", pct: 0, doel: "2026-09-20", groot: "Hele game speelbaar van begin tot eind â€” alle content zit erin, nog niet gepolijst.", nodig: { type: "wachten" } },
        { naam: "Fase 5 â€” Beta", pct: 0, doel: "2026-10-10", groot: "Polijsten + optimaliseren: performance, bugfixes, alles naar AAA-afwerking.", nodig: { type: "wachten" } },
        { naam: "Fase 6 â€” Release", pct: 0, doel: "2026-10-31", groot: "Afwerken en uitbrengen op Steam / Epic Games Store.", nodig: { type: "wachten" } }
      ]
    },
    {
      titel: "2 Â· Systemen (C++)", scope: "t.o.v. Phase-1-scope",
      items: [
        { naam: "Event bus (12.2)", pct: 100 },
        { naam: "Campaign state + transacties + save v0", pct: 100 },
        { naam: "Economie-ledger", pct: 100 },
        { naam: "Strategie-minimap", pct: 100 },
        { naam: "Mission runtime + debrief", pct: 100 },
        { naam: "Squad orders", pct: 95, notitie: "Stance-stub gedrag volgt (Phase 2)." },
        { naam: "Roster / permadeath / memorial", pct: 100 },
        { naam: "Base / preparation", pct: 100 },
        { naam: "Characters / GAS health", pct: 90, notitie: "Damage via GameplayEffects volgt." },
        { naam: "Combat hitscan", pct: 85, notitie: "Feel-pass Phase 2." },
        { naam: "Vijand-AI", pct: 70, notitie: "Perception-stub; patrouilles/variatie later." },
        { naam: "Debug-UI hub/HUD", pct: 80, notitie: "Bewust debug-grade (14.5)." },
        { naam: "Save-plugin", pct: 100 }
      ]
    },
    {
      titel: "3 Â· Graphics", scope: "Part 15 â€” fidelity-pass loopt op de sterke PC (1080 Ti, software-Lumen-pad)",
      totaal: 14,
      items: [
        { naam: "Art-richting gelockt (Borderlands-stilering)", pct: 100, notitie: "Owner-revisie 22-07: scherpere fidelity bÃ­nnen de stijl (15.5)." },
        { naam: "Cel/toon-materiaal (banden + hatching, M_EclipseToon)", pct: 100, notitie: "Live geverifieerd; hatching leest nu als penseelstroken (25% duty, periode 120)." },
        { naam: "Ink-outline post-materiaal", pct: 85, notitie: "PP_EclipseInk LIVE bewezen: Sobelâ†’Laplaciaan-fix (scherende vloer vloeide vol inkt); silhouet- + naadlijnen schoon." },
        { naam: "Belichting / mood", pct: 85, notitie: "SM6-pad live: volumetric smog + zon-schaduwen + skylight + film grain (15.5-revisie); lit-toon-migratie (echte Lumen-GI) is de volgende milestone." },
        { naam: "Kleurenpalet & blok-dressing", pct: 60, notitie: "Palet + skyline-ring + EERSTE ECHTE TEXTURES (CC0 Poly Haven, world-aligned door de toon-pijplijn; gain genormaliseerd op gemeten lineair gemiddelde)." },
        { naam: "Gebouwen (echte kits)", pct: 10 },
        { naam: "Straten / props / decals", pct: 30, notitie: "CC0-props (vaten/barriÃ¨res/kratten) + Pillow-gegenereerde bezettings-decals (Dominion-posters, hazard-pads, verzets-stencils) â€” allemaal door de toon-pijplijn, no-collision." },
        { naam: "Bomen / vegetatie", pct: 0, notitie: "Eerste vegetatie Phase 3 (Sylvaris)." },
        { naam: "Characters / MetaHumans", pct: 35, notitie: "Body-pipeline LIVE: speler/squad/vijanden dragen echte meshes (Belica, RAISOR-soldiers) via DT_BodyDefs; 5 Dominion-archetypes met eigen stats + body. QC: belichting/restyle; daarna MetaHuman-slots (stap 3)." },
        { naam: "Weer & dag/nacht", pct: 0 },
        { naam: "VFX", pct: 0 },
        { naam: "Nanite / Lumen / VSM op target-hardware", pct: 5, notitie: "Sterke PC gemeten: GTX 1080 Ti (11 GB, SM6) â€” Nanite/VSM/software-Lumen kunnen, gÃ©Ã©n RT-cores; HWRT-validatie later op RTX-klasse." }
      ]
    },
    {
      titel: "4 Â· Audio", scope: "Part 16",
      totaal: 65,
      items: [
        { naam: "ElevenLabs TTS-client (HTTPS, key-hygiÃ«ne, PCM/WAV)", pct: 100, notitie: "Live geverifieerd 22-07: 8/8 regels gegenereerd." },
        { naam: "Cache + manifest (nooit 2Ã— dezelfde regel)", pct: 100, notitie: "Cache reist mee in Content/Audio/Generated; herhaalrun = 8 hits, 0 API-calls (live bewezen)." },
        { naam: "Runtime-playback in game", pct: 95, notitie: "PlayLine af, assets staan er; eerste hoorbare check in PIE nog te doen." },
        { naam: "Auto-assign naar Dialogue DataAssets", pct: 100, notitie: "8/8 GeneratedAudio-refs gezet en opgeslagen." },
        { naam: "Editor-bulk-tool (-run=EclipseGenerateVoices)", pct: 100, notitie: "Incl. dialogue-seed (JSON â†’ voice-assets, create-only)." },
        { naam: "Music-endpoint", pct: 0 },
        { naam: "SFX-endpoint", pct: 0 },
        { naam: "Adaptieve always-on muziek (16.7)", pct: 0 },
        { naam: "Credit-budgetplan (16.13)", pct: 100 }
      ]
    },
    {
      titel: "5 Â· Content & verhaal", scope: "Parts 2, 3, 11",
      items: [
        { naam: "Missie-templates Phase 1 (3 stuks)", pct: 100 },
        { naam: "Dialogen geschreven", pct: 2, notitie: "8 squad-barks in DialogueSeed.json (Phase-1 debugbarks)." },
        { naam: "VO gegenereerd", pct: 2, notitie: "8 regels live gegenereerd + gecachet (2 stemmen)." },
        { naam: "Planeten uitgewerkt in game", pct: 5, notitie: "1 graybox-district van Kessara." },
        { naam: "Lore-canon (bible)", pct: 100, notitie: "Docs 00â€“17 staan." }
      ]
    }
  ],

  // Vaste galerij (milestone-beelden). Als progress_auto.js versere shots heeft, wint die.
  screenshots: [
    { file: "progress_media/shot_01.jpg", caption: "Controlepost-compound (review-camera 1)" },
    { file: "progress_media/shot_02.jpg", caption: "Warehouse-straat (review-camera 2)" },
    { file: "progress_media/shot_03.jpg", caption: "Cover-veld (review-camera 3)" },
    { file: "progress_media/shot_04.jpg", caption: "Overzicht district (review-camera 4)" }
  ],
  screenshotNoot: "Sterke PC (GTX 1080 Ti, SM6) â€” texture-ronde: CC0-albedo's (asfalt/beton/metaalplaat/golfplaat) door de toon-pijplijn, exposure-neutraal genormaliseerd; skyline + inktlijnen + schemer-mood intact.",

  changelog: [
    { datum: "Sessie 2026-07-24 (namiddag â€” VIJF SPOREN GELAND in Ã©Ã©n cyclus)", punten: [
      "P2-02 Stage A gebankt (b5aa157, review GO): Command Mode als R3-falsificatiebuild â€” hold Q/LB â†’ 0.30-dilatatie (pure state-machine, fail-safe naar exact 1.0 op elke exit, 20Ã—-ladder getest), per-soldier-selectie door het bestaande order-contract, Event.Command.ModeEntered/Exited, debug-HUD. De feel-gauntlet staat NU bovenaan Nathans actielijst (draaiboek phase0/FEEL_GAUNTLET_P2-02.md); Stage B wacht op het verdict.",
      "P2-03 stap 1-2 gebankt (deabc9f, review GO 'R6-discipline voorbeeldig'): Hollow Point-dataschema + pure BaseLogic; save v3â†’v4 met byte-getrouwe fixtures; spec-startstand als type-default. Stap 3 draagt 5 review-bevindingen verplicht mee (vastgelegd in de taak).",
      "P2-01-review-follow-ups m4/m6/m7 gebankt (3c2f3a5): spawn-fan + cover-scorer-constanten naar DA_SquadTuning (gedrag-neutraal), catalog-formulering eerlijk.",
      "Pack-slim-ronde af: alle 11 curatie-accepts repo-tracked naar /Game/Art/Imported, Minions+SciFi10 van schijf (~5,9 GB vrij, scan-bewijs 0 refs, ASSET_CLEANUP Â§7-10); [Art]-commit na de lopende review.",
      "SPEC-P2-04 (M1.1-M1.4) geschreven + main-review verwerkt: geen prologue (recap + cold-reader), R7-Gauntlet als bouwstap 1, nul nieuwe primitieven, Brick = Assault; 4 open punten beslist in de spec.",
      "Eindbar op de unie van alle sporen: 47/47 tests âœ“, validatie 3 validators/0 fouten âœ“, catalog 23/23 âœ“." ] },
    { datum: "Sessie 2026-07-24 (middag â€” P2-01 GELAND)", punten: [
      "SPEC-P2-01 gebankt (1a85b78, review GO 0 blockers): squad van 4 met de eerste 3 klassen (Assault/Medic/Sniper) als pÃºre data â€” DT_ClassDefs bepaalt kit, signature-verb (Class.Verb.*) en order-modulatie; ontbrekende row degradeert naar classless (14.3.5), nergens een klasse-branch in code.",
      "Checkpoint 14.3.6/R6 aantoonbaar vervuld: save-schema v2â†’v3 (roster-ClassId) mÃ©t migratie-entry, v0-fixture-test (hele keten 0â†’3) Ã©n echte v2-fixture-test in hetzÃ©lfde commit; pre-v3 saves landen deterministisch op classless recruits.",
      "Review-vondst direct gedicht: EclipseValidateData dekte DT_ClassDefs niet â€” derde validator toegevoegd (verb-familie, Stabilizeâ†’window/Killzoneâ†’range-consistentie, weapon/body-cross-refs per campaign-setup); eindstand 3 validators, 0 fouten.",
      "Review-M1-fix: medic her-dispatcht na een afgeronde triage-run naar een casualty die mid-run neerging â€” met CanStabilizeSoldier-peek zodat geredde/verlopen patiÃ«nten nooit een shuttle-loop veroorzaken.",
      "Groene bar eindstand: build âœ“ (-NoUba), 38/38 tests âœ“ (6 nieuw), validatie 0 âœ“, catalog 21/21 âœ“." ] },
    { datum: "Sessie 2026-07-23 (avond â€” owner-keuze B + audio GELAND)", punten: [
      "Westgevel-B district-breed doorgevoerd (412f14f, code GO + art GO): owner overrulede A via het paneel â€” BandHi 0.55 + per-gevel WestComp-compensatie (mid-lerp landt exact op palette-lit) op checkpoint-west, compound-gevel, warehouse-west en de skyline-familie; eindstand matcht de gekozen B op delta<2 en de zalm-gap is dicht. Material-authoring nu delete+recreate: wees-exports onmogelijk. Shots 00050â€“00055 = nieuwe referentie; 38/38, validatie 0.",
      "Eerste audio-ronde gebankt (348b7f0, review GO): 8 ElevenLabs-assets (rifle, impact, UI-tick/confirm, Kessara-Layer-1-loopbed, 2 voetstappen, mission-complete-sting) met hash-cache (her-run = 0 credits) Ã©n reisbare kopie in Eclipse/AudioCache â€” geen machine betaalt dubbel. Import-script staat klaar voor de vrije editor.",
      "MetaHumans geland: SentinelC + Common-basis (908 MB) via owner-kliks; curatie + koppeling aan DT_NamedCharacters staat in de rij achter de P2-01-tests." ] },
    { datum: "Sessie 2026-07-23 (namiddag â€” look-ronde + owner-A/B + SPEC-P2-02 GELAND)", punten: [
      "Look-ronde gebankt (eccd6f2, code GO + art GO): westgevel-banding kwantitatief opgelost (hue-spreiding 17Â°â†’4Â°), witte kiosk van laatste stijl-overtreding naar worker-teal (nieuw KitRow-paletslot), drie gele waarde-treden (Cover > CoverB > DecoLine), signs/posters/stencils volgen de lit-toon-masterkeuze. Characters byte-identiek â€” nul regressies.",
      "OWNER-A/B via het nieuwe nakijkpaneel: owner koos variant B (per-gevel warmte) boven de AI-aanbeveling A â€” B wordt nu district-breed doorgevoerd (BandHi 0.55 + compensatie op Ã©lke westgevel, mÃ©t wees-export-purge in de material-authoring). Het paneel toont keuze + komt met een nieuw cam-1-shot.",
      "SPEC-P2-02 Command Mode gebankt (29cd549, review GO): falsificatie-eerst â€” Stage A debug-feel (hold + 0.30 dilatatie) met meetbare R3-criteria vÃ³Ã³r polish; ack-lat wall-clock zodat dilatatie de meting niet vervalst; 5 nieuwe verbs op het bestaande order-contract; negatieve transities nooit stil (9.5).",
      "Monitor: eerste owner-content-landing (Atira_LODSettings, Locodrome) â€” curatie volgt bij editor-close." ] },
    { datum: "Sessie 2026-07-23 (middag â€” art-fix-ronde + asset-opruim GELAND)", punten: [
      "15.8 art-fix-ronde gebankt (8e06d23, code GO + art GO): DecoStain-vlekken nu organisch via nieuw M_EclipseToonDecal + T_stain_mask-falloff en per-instance rotatie (gebankte plaatsingen bit-identiek); alle 7 placards waarde-genormaliseerd (p99â†’245) + tint-lift, gains her-gemeten; well-ring amber accent + volgt de lit-toon-masterkeuze; apron Ã©Ã©n waardestap; hazard-generator-bug gefixt (effen gele quad â†’ worn chevron). Shots 00022â€“00027 = nieuwe referentie; 38/38 tests, validatie 0.",
      "Asset-opruimronde (f1d0ad0, owner-opdracht): 5,2 GB machine-lokaal vrijgemaakt op bewijs â€” Twinblast (0 accepts) en FD-signs-pack (60 raws als bron bewaard) weg, SDI-template-filler weg met HUD-payload bewaard, zips/blends gededupliceerd. Binaire ref-scan 0 hits; tracked repo onaangetast; phase0/ASSET_CLEANUP.md is de bewijsvoering incl. KEEP-gepland-tabel per ronde.",
      "Owner verwijdert zelf GameAnimationSample (18 GB): repo-breed 0 referenties, niets gemigreerd â€” bevestigd veilig." ] },
    { datum: "Sessie 2026-07-23 (middag â€” curatie-plaatsing + SPEC-P2-03 GELAND)", punten: [
      "Grunge-vervanger zonder owner-klik (owner-besluit): ambientCG CC0 Metal041B (DecoStain, gain 3.44) + CorrugatedSteel007A (warehouse, 2.72) + Metal063 1Kâ†’2K; fetch_cc0.py-zoekfix (q=-parameter); herbruikbaar import_cc0_albedos.py.",
      "Curatie-plaatsingsronde gebankt (b70dcf9): 4 nieuwe placards (route/labor/blast/reactor) in het sign-patroon, SciFi10-deckplate-apron + tile-locked bassinring als plaza-middelpunt, Megascans-4K-asfalt als Floor mÃ©t repo-eigen fallback, MID-cache-bugfix (keyde op kleur, nu paletprefix). Code-review GO + Â§15.8-art-review GO; shots 00008â€“00013 zijn de nieuwe referentie; horizon schoon op alle 6 cams.",
      "SPEC-P2-03 Hollow Point gebankt (7194cf4) na review-ronde (3 blokkerende econ-fixes verwerkt): Intelligence Center vÃ³Ã³r Medbay (pre-flip intel-schaarste), Slot B start als 5.2-bunkerkamp (squad-pick nooit gegate), missie-debrief committeert +1 dag (cross-spec gemarkeerd), econ-paden als nachtelijke soak-asserts.",
      "Groene bar: build âœ“, 31/31 âœ“, validatie 0 âœ“; art-fix-ronde (stains/placards/well) + P2-01 draaien door." ] },
    { datum: "Sessie 2026-07-23 (stap 2 â€” character-body-pipeline GELAND)", punten: [
      "11 Fab-packs binnen (eigenaar): 8Ã— RAISOR SciFi-soldiers + Paragon Belica/Twinblast/Minions (~50 skeletal meshes, ~950 animaties; 10,8 GB â€” machine-lokaal, gitignored).",
      "Sleutelvondst: de hele SciFi-familie deelt de UE4-Mannequin-skeletfamilie â†’ anim-rijke packs voeden anim-arme, geen retargeting op dit tier.",
      "DT_BodyDefs (9 bodies, registry-geresolved) + FEclipseBodyDefRow + ApplyBodyDef op de gedeelde AEclipseCharacter; DT_EnemyArchetypes uitgebreid naar 5 Dominion-archetypes (09.3) mÃ©t eigen body en stats; vijand-spawn cyclet archetypes; squad uit de Rebel-pool; speler = Belica.",
      "MetaHumanCharacter + MetaHumanSDK + Python-plugins aan; shot-rig kreeg een body-showcase door het echte datapad; Fab-packs bewust buiten git.",
      "Groene bar: build âœ“, 31/31 âœ“ (incl. squad-suite), validatie 0 âœ“, catalog 19/19 âœ“."
    ]},
    { datum: "Sessie 2026-07-23 (loop-iteratie 6 â€” eerste bewoners)", punten: [
      "EERSTE CHARACTERS in het district: Quaternius CC0-pack (BlueSoldier M/V + 2 burgers, rigged mÃ©t animatiesets incl. Shoot/Death/RecieveHit â€” de squad-werkwoorden van later) via gdown gedownload en als skeletal meshes geÃ¯mporteerd.",
      "Geplaatst als Idle-figuren: Dominion-enforcers bij poort en checkpoint (goud cel), burgers bij het warehouse (grijs-teal) â€” visueel niveau, geen AI/collision (Part 9-werk komt later).",
      "Twee materiaallessen gebankt: skeletal-usage-flag ontbrak (figuren renderden zwart in -game) en bijna-neutrale Ã—10-paletten wassen naar grijs (enforcer nu verzadigd goud).",
      "Fab-library: eigenaar ingelogd (Launcher + fab.com); wacht op 'Add to Project'-kliks â€” monitor vuurt automatisch zodra content in Eclipse/Content landt.",
      "Groene bar: build âœ“, 31/31 âœ“, validatie 0 âœ“, catalog 19/19 âœ“."
    ]},
    { datum: "Sessie 2026-07-23 (loop-iteratie 5 â€” bezettings-decals via Pillow)", punten: [
      "PILLOW IN GEBRUIK: Tools/generate_decals.py genereert abstracte luminantie-decals (AEGIS-oog-poster, hazard-strepen, verzets-eclips-stencil) â€” kleur komt ALTIJD van het palet (Dominion-goud, rebel-rood, amber), dus palet-discipline is structureel geborgd.",
      "Decals geplaatst als no-collision planes: propaganda op de compound-gevels, hazard-pads bij de kruising, stencils bij Entry_Main en op het warehouse â€” het district vertelt nu bezetting Ã©n verzet (15.5/03.3).",
      "Zelfde meetdiscipline als textures: gains per decal gemeten (7.8/1.3/7.1), niet geschat.",
      "Groene bar: build âœ“, 31/31 âœ“, validatie 0 âœ“, catalog 19/19 âœ“."
    ]},
    { datum: "Sessie 2026-07-23 (loop-iteratie 4 â€” eerste echte props + tools)", punten: [
      "EERSTE ECHTE 3D-MESHES in het district: CC0-props van Poly Haven (Barrel_01, concrete_road_barrier, plastic_crate_03) â€” FBX + diffuse gedownload via API, geÃ¯mporteerd (Tools/import_polyhaven_props.py), geplaatst als vaten-clusters, checkpoint-barriÃ¨res en krat-stapels (no-collision dressing, deterministisch).",
      "Toon-materiaal kreeg een mesh-UV-pad (UVMode) zodat geÃ¯mporteerde props hun eigen UVs gebruiken; alle material-slots krijgen de toon-MID; tonemapper-les: bijna-neutrale heldere paletten worden bleek â€” vat-palet naar donker verzadigd roest.",
      "Tools geÃ¯nstalleerd met owner-akkoord: Pillow 12.3 âœ“ (texture/decal-generatie), ffmpeg âœ“ (audio-pijplijn); Blender staat in de UAC-goedkeurings-wachtrij (geen blokker: UE heeft glTF-import en Quaternius levert FBX).",
      "Eigenaar heeft fab.com-login gezet; wachtrij voor eigenaar: Launcher-login + pack-shortlist aanklikken, ElevenLabs Music/SFX-scopes.",
      "Groene bar: build âœ“, 31/31 âœ“, validatie 0 âœ“, catalog 19/19 âœ“."
    ]},
    { datum: "Sessie 2026-07-23 (loop-iteratie 3 â€” lit-toon-experiment)", punten: [
      "Lit-toon-migratie gebouwd als veilig A/B-experiment: tweede master M_EclipseToonLit (DefaultLit â€” cel-banden als BaseColor, echt VSM/Lumen-licht erbovenop), alleen actief met -EclipseLitToon op SM6; Glow-strips blijven altijd unlit-emissive; default onveranderd.",
      "A/B-verdict (eerlijk): bij de schemerzon op command-afstand vrijwel niet te onderscheiden van unlit (auto-exposure normaliseert) â€” beslissing wacht op interieur/dag-scÃ¨nes waar GI/schaduw er echt toe doen; unlit blijft de gelockte default.",
      "Groene bar: build âœ“, 31/31 âœ“, validatie 0 âœ“, catalog 19/19 âœ“.",
      "Eigenaar-vraag beantwoord: CC0-spoor volledig autonoom (bewezen); Fab-spoor wacht op Ã©Ã©nmalige Epic-login + per-pack library-klik; ElevenLabs Music/SFX-scopes voor taak D."
    ]},
    { datum: "Sessie 2026-07-22 (nacht â€” loop-iteratie 2)", punten: [
      "Palet-discipline structureel: texture-variatie is nu luminantie-only in de shader â€” een roestige texture kan een Dominion-gevel nooit meer verkleuren; de post staat weer op het gelockte zalm-oxide.",
      "Straat-dressing als no-collision-deco: hoofdader + dwarsstraat met rijstrook-markering en 14 olie/roestvlekken; natrium-checkpointstrips (12) op de binnenmuren â€” de plaza vertelt nu een bezettingsverhaal (15.5).",
      "SPEC-P2-00 (Vertical Slice-overview) opgeleverd via subagent en gereviewd: build-volgorde P2-01â€¦09, events per spec, testeisen, non-goals; ACTIVE_MILESTONE-omzetting blijft expliciet aan de eigenaar.",
      "Groene bar: build âœ“, 31/31 âœ“, validatie 0 âœ“, catalog 19/19 âœ“."
    ]},
    { datum: "Sessie 2026-07-22 (nacht, sterke PC â€” asset-pass gestart)", punten: [
      "EERSTE ECHTE TEXTURES in de game (taak C, owner-akkoord): 4Ã—2K CC0-diffuse van Poly Haven (asfalt, betonblok, golfplaat, metaalplaat) â€” gedownload via API, geÃ¯mporteerd via nieuw script Tools/import_polyhaven_textures.py, herkomst in Content/Art/Textures/SOURCES.md.",
      "M_EclipseToon uitgebreid met world-aligned albedo-pad (dominante-as-projectie, default volledig neutraal); builder koppelt per palet-entry texture + schaal.",
      "Exposure-les geleerd en gefixt: textures her-meterden de auto-exposure (schemer werd dag) â€” per texture het lineaire gemiddelde gemeten en gain = 1/gemiddelde gezet (klem op 2.5 tegen hotspot-krassen); de gebankte dusk-grade is terug.",
      "Fab-route gedocumenteerd: wacht op Ã©Ã©nmalige Epic-login van de eigenaar (Launcher nooit gestart op deze pc); CC0-spoor loopt ondertussen autonoom door.",
      "Groene bar: build âœ“, 31/31 âœ“, validatie 0 âœ“, catalog 19/19 âœ“."
    ]},
    { datum: "Sessie 2026-07-22 (avond, sterke PC â€” vervolg)", punten: [
      "Shot-rig gefixt: pawn in flying-mode tijdens de rig â€” overview-camera 4 kadreert nu het hele district (viel eerst 2 s naar de grond vÃ³Ã³r elke capture).",
      "Hatch-tuning: 25% duty + periode 120 â€” schaduw-arcering leest als penseelstroken, niet meer als golfplaat.",
      "SM6-fidelitypad in de builder (feature-level-gated, laptop onaangetast): volumetric smog + zon-schaduwen (lichtschachten), realtime skylight, film grain 0.07 + bloom 0.45 per de 15.5-revisie.",
      "KESSARA-SKYLINE (03.3, code-built placeholder): deterministische ring (seed 503) buiten de perimeter â€” 56 fabriekshulks, 18 schoorstenen, 12 kraanportalen, natrium-raamstroken; het district staat nu in een stad. Buiten nav/missies, tests onaangetast.",
      "Groene bar na alles opnieuw: build âœ“, 31/31 âœ“, validatie 0 âœ“, catalog 19/19 âœ“."
    ]},
    { datum: "Sessie 2026-07-22 (avond, sterke PC)", punten: [
      "Eerste sessie op de sterke PC: groene bar onafhankelijk herbevestigd (build âœ“, 31/31 tests âœ“, validatie 0 fouten âœ“, catalog 19/19 âœ“).",
      "Machine gemeten: GTX 1080 Ti 11 GB â€” SM6 met Nanite/VSM/TSR/software-Lumen, gÃ©Ã©n RT-cores (HWRT-validatie blijft RTX-werk); charter 15.2 bijgewerkt.",
      "Owner-revisie 15.5 vastgelegd: Borderlands-leunend blijft gelockt, fidelity erbinnen omhoog (Nanite-dichtheid, software-Lumen-GI, SSAO/bloom/film grain, particles, TSR).",
      "Beide materiaal-scripts gedraaid; INKTLIJNEN LIVE: eerste-orde depth-Sobel overspoelde de scherende vloer met inkt (heel middenveld zwart) â€” herbouwd op Laplaciaan (2e afgeleide), silhouet- Ã©n naadlijnen schoon, vloer loopt weer door tot de horizon.",
      "Galerij ververst met de geverifieerde sterke-PC-shotronde (review-camera's 1-4)."
    ]},
    { datum: "Sessie 2026-07-22 (namiddag)", punten: [
      "GRAPHICS-DOORBRAAK: de 'paarse waas' van 7 passes bleek het oude outline-postmateriaal dat het hele beeld overschilderde; toon-materiaal M_EclipseToon (cel-banden + hatching, Unlit) bewezen werkend, inktlijnen herbouwd als PP_EclipseInk-script (nog te draaien + shotronde).",
      "Xbox-controller + muis samen speelbaar: stick-lopen/kijken, RT vuren, D-pad squad-orders, LB stance â€” feel-pass volgt in Phase 2.",
      "MIGRATION_TO_STRONG_PC.md herschreven als dag-Ã©Ã©n-draaiboek: consent-protocol (eerst uitleggen, dan akkoord, -NoUba), asset-beleid (algemeen downloaden / hero handgebouwd op gelijk niveau), bootstrap-prompt voor de nieuwe PC (Â§7).",
      "Groene bar herbevestigd na alle wijzigingen: build âœ“, 31/31 âœ“."
    ]},
    { datum: "Sessie 2026-07-22 (middag)", punten: [
      "Groene bar onafhankelijk herbevestigd: build âœ“, 31/31 tests âœ“, validatie 0 fouten âœ“, catalog 19/19 âœ“.",
      "Dialogue-seed gebouwd (16.12): Content/Audio/DialogueSeed.json â†’ voice-assets via de commandlet (create-only), pure parser + 31e headless test.",
      "EERSTE LIVE ELEVENLABS-RUN GESLAAGD: 8/8 barks gegenereerd (PCMâ†’WAV), geÃ¯mporteerd als USoundWave en auto-toegewezen; herhaalrun = 8 cache-hits, 0 API-calls.",
      "Voice-cache gecommit â€” geen enkele machine betaalt deze regels opnieuw.",
      "Let op: key werkt voor TTS maar /v1/voices geeft 401 (scope-beperkt); rotatiestatus bevestigen is aan de eigenaar."
    ]},
    { datum: "Sessie 2026-07-22", punten: [
      "Onafhankelijke her-review van alle Phase-1-code afgerond.",
      "4 defect-fixes gecommit: missions-served-teller, cover-scorer vriend/vijand, data-wiring speler/wapens/vijanden + speler-revive, UI-foutmeldingen.",
      "Ink-outline post-materiaal geauthord; district-dressing + -EclipseShot screenshot-rig gebouwd (19 review-passes, SM5-limieten gedocumenteerd).",
      "Audio-pipeline afgerond conform phase0/OWNER_MANDATE.md: PCMâ†’WAV, runtime-PlayLine, auto-assign + bulk-commandlet, 2 nieuwe tests.",
      "Live dashboard omgebouwd naar auto-lezend systeem (progress_data.js + progress_auto.js + watcher)."
    ]}
  ]
};
