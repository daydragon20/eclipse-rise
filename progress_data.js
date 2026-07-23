// ECLIPSE voortgang — INSCHATTINGSDATA (handmatig, door de dev-sessie bij milestones bijgewerkt).
// De automatische feiten (commits, tests, verse screenshots) staan in progress_auto.js
// en worden door Tools/update_progress.ps1 gegenereerd — daar blijf je vanaf.
// Regels: percentages zijn eerlijk (liever te laag dan gejokt); geen HTML hier, alleen data.
window.PROGRESS_DATA = {
  bijgewerkt: "2026-07-23 15:32",
  hero: { label: "Hele game", pct: 8 },
  playtestChip: "ACTIVE_MILESTONE: Phase 2 (owner-instructie 23-07) · 13.2-playtest Phase 1: OPEN (standing owner-actie)",

  // Grove tijdsinschattingen — bewust ruw; door de dev-sessie per milestone bijgesteld o.b.v. tempo + resterende backlog.
  eta: {
    fase1: "~vrijwel af (97%) — resteert alleen jouw 30-min playtest (13.2)",
    slice: "Vertical Slice (Fase 2): grove schatting enkele weken bij dit tempo",
    totaal: "Hele game: grove schatting maanden — sterk afhankelijk van scope-keuzes en jouw playtests",
    toelichting: "Ruwe inschatting, geen belofte: gebaseerd op het huidige commit-tempo en de resterende backlog per fase. Wordt elke milestone bijgesteld.",
    bijgesteld: "2026-07-23"
  },

  // Wat de owner concreet moet doen — apart paneel bovenaan. Dev-sessie houdt kort + actueel; leeg [] = niks te doen.
  ownerActies: [
    { titel: "Overbodige downloads annuleren", prio: "nu", waarom: "bespaart ~30 GB schijf", stappen: [
      "Epic Games Launcher → Downloads",
      "Annuleer: alle Paragon-helden (Kwang, Narbash, Wukong, Riktor, Sevarog…), Old West VOL 4/6, Path of Adventure JRPG-muziek, NWIRO AI Pro",
      "Laat staan: Photogrammetry Snow, Plant Models Vol 60, Perfect Fire VFX"
    ] },
    { titel: "MetaHuman-gezichten binnenhalen", prio: "gauw", waarom: "nodig voor de named characters (Fase 2)", stappen: [
      "Unreal → Window → Fab → My Library",
      "Klik 1× Download bij elk MetaHuman-item",
      "Zeg de agent: 'MetaHumans staan erin'"
    ] },
    { titel: "1 civilian/worker-pack toevoegen", prio: "optioneel", waarom: "enige character-gat: Kessara-burgers close-up", stappen: [
      "Fab: zoek een gestileerd civilian/worker-pack (4–6 bodies, Mannequin-rig)",
      "Add to Project → Eclipse"
    ] },
    { titel: "13.2-playtest (~30 min)", prio: "als je tijd hebt", waarom: "sluit Fase 1 af + stuurt de eerste review", stappen: [
      "Speel de graybox-loop ~30 min in de editor",
      "Noteer kort wat goed/slecht voelt",
      "Geef het door aan de agent"
    ] }
  ],

  // Assets-overzicht — wat is binnengehaald, zelf gemaakt, en wat nog te doen. Dev-sessie houdt dit actueel.
  assets: {
    gepakt: [
      "8× RAISOR SciFi-soldier packs (Fab) — squad- én vijand-bodies via DT_BodyDefs",
      "Paragon: Lt. Belica (Fab) — speler-body",
      "Paragon: Minions (Fab) — extra bodies",
      "Quaternius CC0-characters (rigged, mét anim-sets) — eerste bewoners",
      "Poly Haven CC0-texturen: asfalt, beton, golfplaat, metaalplaat",
      "Poly Haven CC0-props: vat, wegbarrière, krat",
      "ambientCG CC0-materialen: Metal041B, Metal063, Concrete042A, CorrugatedSteel007A",
      "1082 gratis UE-assets klaar in Fab-library (reserve, nog niet geïmporteerd)"
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

  // Live takenlijst van de AI-dev-sessie — status: bezig | wachten | klaar | gepland.
  taken: [
    { taak: "Autonomy-loop actief", status: "bezig", pct: 100, detail: "3 element-builders parallel: (1) westgevel-B doorvoeren (owner-keuze), (2) P2-01 squad van 4 + classes, (3) audio-ronde 16.7 (ElevenLabs); planner houdt de backlog risk-first (phase0/EXECUTION_PLAN.md). Reviews vóór elke commit; wakeup 60 s; voltooide taken verhuizen naar de changelog." },
    { taak: "Westgevel-B doorvoeren (owner-keuze)", status: "bezig", pct: 60, detail: "Owner koos B in het A/B-paneel — B wordt district-breed doorgevoerd: BandHi terug naar 0.55 + per-gevel warmte-compensatie op élke westgevel (masters al her-authored en wees-vrij, C++-rollout geschreven). Wacht alleen op het sluiten van de owner-editor (Live Coding blokkeert de build); bouwt daarna automatisch → shots → reviews → commit → nieuw cam-1-shot in het A/B-paneel." },
    { taak: "P2-01: Squad van 4 + classes", status: "bezig", pct: 85, detail: "Code compileert groen, event-catalog 21/21, DT_ClassDefs gevuld en gekoppeld (setup_class_data.py). Enige rest: de headless testsuite (verwacht 38 tests) + EclipseValidateData — die draaien automatisch zodra de owner-editor sluit. Daarna review (checkpoint 14.3.6: ClassId-migratie + v0-fixture-test in zelfde commit) en commit." },
    { taak: "Stap 3: MetaHuman-pijplijn", status: "bezig", pct: 75, detail: "MIJLPAAL (monitor 18:45): /Game/MetaHumans is GELAND — 908 MB met Common-basis + eerste MetaHuman 'SentinelC'; ook Atira_LODSettings + Locodrome staan erin. De owner-editor draait nog (Live Coding): zodra die sluit volgt inventaris + curatie + koppeling MH → DT_NamedCharacters-slots (Kaya/Brick/Vale/Dex/Petra/Kaine) en de toon-restyle-check per 15.7. Resterende MH-items: per stuk 1 Download-klik zoals SentinelC." },
    { taak: "Audio-ronde 16.7 (ElevenLabs)", status: "bezig", pct: 5, detail: "Build-vrij element terwijl de editor bezet is: eerste SFX-set (wapenschot, impact, UI-tick, ambient-loop Kessara) + één muzieksting via de ElevenLabs-API (scopes open, HTTP 200) naar Saved/AudioStaging met manifest + kant-en-klaar importscript; alles gecachet zodat er nul herhaal-kosten zijn. Import in UE draait zodra de editor vrij is (16.7-endpoints)." },
    { taak: "Groene bar bewaken", status: "bezig", pct: 100, detail: "Elke iteratie: build (-NoUba) + 31/31+ tests + validatie 0 + catalog vóór elke commit/push." },
    { taak: "Wereld zonder einde (Borderlands-compositie)", status: "gepland", pct: 5, detail: "Staande owner-taak (23-07): elke review-cam toont een gevulde horizon — geen zichtbaar wereld-einde; horizon-check van de plaatsingsronde was schoon op alle 6 cams. Skyline-audit + gelaagde silhouetten/landmarks naar Borderlands-voorbeeld hervat zodra de art-fix-ronde geland is (zelfde 15.8-loop)." },
    { taak: "Pack-slim-ronde (migrate + drop)", status: "gepland", pct: 0, detail: "Vervolg op de opruimronde (owner-vraag 23-07): gebruikte assets uit ParagonMinions (well + rocks) en SciFi_Materials_10 (7 albedo's) met dependencies migreren naar /Game/Art/Imported, builder-paden omzetten, dan beide packs van schijf — ~5,9 GB extra vrij. Start zodra de look-builder EclipseGrayboxBuilder.cpp vrijgeeft." },
    { taak: "Wachtrij eigenaar", status: "wachten", pct: 0, detail: "1) MetaHuman-items: 1× Download-klik per item in Window → Fab → My Library (login werkt) · 2) MH_<Naam>-gezichten per phase0/metahuman_recipes.md · 3) env-pack-pulls: Factory Pack Vol.1, Industrial Building 49 PBR, UNIBLOCKS, Sci FI Hallway, Sci-Fi Light Pack, Auto Footsteps Utility, Niagara Footstep VFX, FPS Weapon Bundle, Free Muzzle Flash · 4) CHARACTER-GAT (enige!): 1 gestileerd civilian/worker-pack, 4–6 bodies, Mannequin-rig, mét arbeider-varianten — voor Hollow Point-crew/idlers (SPEC-P2-03) en Kessara-burgers close-up · 5) 13.2-playtest (~30 min, stuurt R1) · 6) westgevel-A/B nakijken zodra de shots in het paneel staan. KLAAR: Fab-login ✓, ElevenLabs-scopes ✓, Blender ✓, GameAnimationSample mag weg (0 refs)." }
  ],

  // A/B-keuzes die de owner in de viewer nakijkt — status: open | beslist; keuze vult de dev-sessie in na owner-antwoord.
  abTests: [
    { titel: "Westgevel-kleur (checkpoint-muur, cam 1)", status: "beslist",
      vraag: "De westgevel leest zalmroze doordat hij nét in de mid-band van de cel-shading valt (ndl +0.52, BandHi 0.55); de zuidgevel leest verzadigd oxide — één gebouw oogt als twee assets. Welke variant leest beter als één gebouw, zonder de andere cams te schaden?",
      opties: [
        { label: "A — banddrempel 0.50", img: "progress_media/ab_westgevel_A.png", uitleg: "westgevel schuift de lit-band in en wordt pixel-gelijk aan de zonzijde (gemeten 251,160,53 ≈ 252,158,55); werkt district-breed, exposure-delta ≤2 op alle zes cams" },
        { label: "B — per-gevel tint-compensatie", img: "progress_media/ab_westgevel_B.png", uitleg: "alleen de gecompenseerde gevel wordt goed; de compound-gevel verderop en elke andere westgevel blijven zalmroze — schaalt niet" },
        { label: "referentie (oude stand)", img: "progress_media/ab_westgevel_ref.png", uitleg: "gebankte ronde vóór de fix: westgevel zalmroze (236,126,88), zuidgevel oxide — één gebouw leest als twee assets" }
      ],
      aanbeveling: "A werd aanbevolen op de metingen; de keuze is aan de owner en die koos B.", keuze: "B — per-gevel warmte (owner-keuze 2026-07-23). Wordt nu district-breed doorgevoerd: de warme B-look consequent op élke westgevel (in de losse test was alleen één gevel gecompenseerd). BandHi terug naar 0.55; nieuw cam-1-shot volgt in dit paneel; builder draait." }
  ],

  secties: [
    {
      titel: "1 · Roadmap", scope: "Part 13",
      items: [
        { naam: "Fase 0 — Pre-productie", pct: 90, doel: "2026-07-24", groot: "Fundament: alle systeem-architectuur opgezet, de Borderlands-art-richting gelockt, en de tools klaar. De basis waar alles op bouwt.", notitie: "Open: CI-runner, concept-art, feel-clips.", nodig: { type: "wachten" } },
        { naam: "Fase 1 — Prototype \"The Loop\"", pct: 97, doel: "2026-07-25", groot: "Bewijzen dat de kern-loop (missie → squad → gevecht → basis) écht leuk speelt — op blokken (graybox). Bewust nog geen mooie graphics; die volgen pas als het spel leuk is.", notitie: "Vrijwel af; resteert de 13.2-playtest.", nodig: { type: "owner", stappen: ["Speel de graybox-loop ~30 min in de editor (PIE)", "Noteer kort wat goed/slecht voelt", "Geef het door aan de agent — dit sluit Fase 1 af"] } },
        { naam: "Fase 2 — Vertical Slice \"Thirteen Bullets\"", pct: 9, doel: "2026-08-12", groot: "HET grote moment: het eerste district (Kessara) in vólle Borderlands-kwaliteit — echte gebouwen, characters met MetaHuman-gezichten, gevechts-feel. Hier zie je voor het eerst hoe de game er echt uit gaat zien.", notitie: "ACTIEF: body-pipeline + P2-01 (squad van 4 + classes) lopen.", nodig: { type: "owner", stappen: ["MetaHuman: 1× Download per item in Window→Fab, dan agent laten inpluggen", "1 gestileerd civilian/worker-pack toevoegen (enige character-gat)", "Westgevel-kleur A/B kiezen zodra de shots in het paneel staan"] } },
        { naam: "Fase 3 — Early Build", pct: 0, doel: "2026-08-31", groot: "De wereld wordt groot: meer planeten + missies uitgerold, allemaal in de Borderlands-stijl. De vertical-slice-kwaliteit wordt over de hele game uitgesmeerd.", nodig: { type: "wachten" } },
        { naam: "Fase 4 — Alpha", pct: 0, doel: "2026-09-20", groot: "Hele game speelbaar van begin tot eind — alle content zit erin, nog niet gepolijst.", nodig: { type: "wachten" } },
        { naam: "Fase 5 — Beta", pct: 0, doel: "2026-10-10", groot: "Polijsten + optimaliseren: performance, bugfixes, alles naar AAA-afwerking.", nodig: { type: "wachten" } },
        { naam: "Fase 6 — Release", pct: 0, doel: "2026-10-31", groot: "Afwerken en uitbrengen op Steam / Epic Games Store.", nodig: { type: "wachten" } }
      ]
    },
    {
      titel: "2 · Systemen (C++)", scope: "t.o.v. Phase-1-scope",
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
      titel: "3 · Graphics", scope: "Part 15 — fidelity-pass loopt op de sterke PC (1080 Ti, software-Lumen-pad)",
      totaal: 14,
      items: [
        { naam: "Art-richting gelockt (Borderlands-stilering)", pct: 100, notitie: "Owner-revisie 22-07: scherpere fidelity bínnen de stijl (15.5)." },
        { naam: "Cel/toon-materiaal (banden + hatching, M_EclipseToon)", pct: 100, notitie: "Live geverifieerd; hatching leest nu als penseelstroken (25% duty, periode 120)." },
        { naam: "Ink-outline post-materiaal", pct: 85, notitie: "PP_EclipseInk LIVE bewezen: Sobel→Laplaciaan-fix (scherende vloer vloeide vol inkt); silhouet- + naadlijnen schoon." },
        { naam: "Belichting / mood", pct: 85, notitie: "SM6-pad live: volumetric smog + zon-schaduwen + skylight + film grain (15.5-revisie); lit-toon-migratie (echte Lumen-GI) is de volgende milestone." },
        { naam: "Kleurenpalet & blok-dressing", pct: 60, notitie: "Palet + skyline-ring + EERSTE ECHTE TEXTURES (CC0 Poly Haven, world-aligned door de toon-pijplijn; gain genormaliseerd op gemeten lineair gemiddelde)." },
        { naam: "Gebouwen (echte kits)", pct: 10 },
        { naam: "Straten / props / decals", pct: 30, notitie: "CC0-props (vaten/barrières/kratten) + Pillow-gegenereerde bezettings-decals (Dominion-posters, hazard-pads, verzets-stencils) — allemaal door de toon-pijplijn, no-collision." },
        { naam: "Bomen / vegetatie", pct: 0, notitie: "Eerste vegetatie Phase 3 (Sylvaris)." },
        { naam: "Characters / MetaHumans", pct: 35, notitie: "Body-pipeline LIVE: speler/squad/vijanden dragen echte meshes (Belica, RAISOR-soldiers) via DT_BodyDefs; 5 Dominion-archetypes met eigen stats + body. QC: belichting/restyle; daarna MetaHuman-slots (stap 3)." },
        { naam: "Weer & dag/nacht", pct: 0 },
        { naam: "VFX", pct: 0 },
        { naam: "Nanite / Lumen / VSM op target-hardware", pct: 5, notitie: "Sterke PC gemeten: GTX 1080 Ti (11 GB, SM6) — Nanite/VSM/software-Lumen kunnen, géén RT-cores; HWRT-validatie later op RTX-klasse." }
      ]
    },
    {
      titel: "4 · Audio", scope: "Part 16",
      totaal: 65,
      items: [
        { naam: "ElevenLabs TTS-client (HTTPS, key-hygiëne, PCM/WAV)", pct: 100, notitie: "Live geverifieerd 22-07: 8/8 regels gegenereerd." },
        { naam: "Cache + manifest (nooit 2× dezelfde regel)", pct: 100, notitie: "Cache reist mee in Content/Audio/Generated; herhaalrun = 8 hits, 0 API-calls (live bewezen)." },
        { naam: "Runtime-playback in game", pct: 95, notitie: "PlayLine af, assets staan er; eerste hoorbare check in PIE nog te doen." },
        { naam: "Auto-assign naar Dialogue DataAssets", pct: 100, notitie: "8/8 GeneratedAudio-refs gezet en opgeslagen." },
        { naam: "Editor-bulk-tool (-run=EclipseGenerateVoices)", pct: 100, notitie: "Incl. dialogue-seed (JSON → voice-assets, create-only)." },
        { naam: "Music-endpoint", pct: 0 },
        { naam: "SFX-endpoint", pct: 0 },
        { naam: "Adaptieve always-on muziek (16.7)", pct: 0 },
        { naam: "Credit-budgetplan (16.13)", pct: 100 }
      ]
    },
    {
      titel: "5 · Content & verhaal", scope: "Parts 2, 3, 11",
      items: [
        { naam: "Missie-templates Phase 1 (3 stuks)", pct: 100 },
        { naam: "Dialogen geschreven", pct: 2, notitie: "8 squad-barks in DialogueSeed.json (Phase-1 debugbarks)." },
        { naam: "VO gegenereerd", pct: 2, notitie: "8 regels live gegenereerd + gecachet (2 stemmen)." },
        { naam: "Planeten uitgewerkt in game", pct: 5, notitie: "1 graybox-district van Kessara." },
        { naam: "Lore-canon (bible)", pct: 100, notitie: "Docs 00–17 staan." }
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
  screenshotNoot: "Sterke PC (GTX 1080 Ti, SM6) — texture-ronde: CC0-albedo's (asfalt/beton/metaalplaat/golfplaat) door de toon-pijplijn, exposure-neutraal genormaliseerd; skyline + inktlijnen + schemer-mood intact.",

  changelog: [
    { datum: "Sessie 2026-07-23 (namiddag — look-ronde + owner-A/B + SPEC-P2-02 GELAND)", punten: [
      "Look-ronde gebankt (eccd6f2, code GO + art GO): westgevel-banding kwantitatief opgelost (hue-spreiding 17°→4°), witte kiosk van laatste stijl-overtreding naar worker-teal (nieuw KitRow-paletslot), drie gele waarde-treden (Cover > CoverB > DecoLine), signs/posters/stencils volgen de lit-toon-masterkeuze. Characters byte-identiek — nul regressies.",
      "OWNER-A/B via het nieuwe nakijkpaneel: owner koos variant B (per-gevel warmte) boven de AI-aanbeveling A — B wordt nu district-breed doorgevoerd (BandHi 0.55 + compensatie op élke westgevel, mét wees-export-purge in de material-authoring). Het paneel toont keuze + komt met een nieuw cam-1-shot.",
      "SPEC-P2-02 Command Mode gebankt (29cd549, review GO): falsificatie-eerst — Stage A debug-feel (hold + 0.30 dilatatie) met meetbare R3-criteria vóór polish; ack-lat wall-clock zodat dilatatie de meting niet vervalst; 5 nieuwe verbs op het bestaande order-contract; negatieve transities nooit stil (9.5).",
      "Monitor: eerste owner-content-landing (Atira_LODSettings, Locodrome) — curatie volgt bij editor-close." ] },
    { datum: "Sessie 2026-07-23 (middag — art-fix-ronde + asset-opruim GELAND)", punten: [
      "15.8 art-fix-ronde gebankt (8e06d23, code GO + art GO): DecoStain-vlekken nu organisch via nieuw M_EclipseToonDecal + T_stain_mask-falloff en per-instance rotatie (gebankte plaatsingen bit-identiek); alle 7 placards waarde-genormaliseerd (p99→245) + tint-lift, gains her-gemeten; well-ring amber accent + volgt de lit-toon-masterkeuze; apron één waardestap; hazard-generator-bug gefixt (effen gele quad → worn chevron). Shots 00022–00027 = nieuwe referentie; 38/38 tests, validatie 0.",
      "Asset-opruimronde (f1d0ad0, owner-opdracht): 5,2 GB machine-lokaal vrijgemaakt op bewijs — Twinblast (0 accepts) en FD-signs-pack (60 raws als bron bewaard) weg, SDI-template-filler weg met HUD-payload bewaard, zips/blends gededupliceerd. Binaire ref-scan 0 hits; tracked repo onaangetast; phase0/ASSET_CLEANUP.md is de bewijsvoering incl. KEEP-gepland-tabel per ronde.",
      "Owner verwijdert zelf GameAnimationSample (18 GB): repo-breed 0 referenties, niets gemigreerd — bevestigd veilig." ] },
    { datum: "Sessie 2026-07-23 (middag — curatie-plaatsing + SPEC-P2-03 GELAND)", punten: [
      "Grunge-vervanger zonder owner-klik (owner-besluit): ambientCG CC0 Metal041B (DecoStain, gain 3.44) + CorrugatedSteel007A (warehouse, 2.72) + Metal063 1K→2K; fetch_cc0.py-zoekfix (q=-parameter); herbruikbaar import_cc0_albedos.py.",
      "Curatie-plaatsingsronde gebankt (b70dcf9): 4 nieuwe placards (route/labor/blast/reactor) in het sign-patroon, SciFi10-deckplate-apron + tile-locked bassinring als plaza-middelpunt, Megascans-4K-asfalt als Floor mét repo-eigen fallback, MID-cache-bugfix (keyde op kleur, nu paletprefix). Code-review GO + §15.8-art-review GO; shots 00008–00013 zijn de nieuwe referentie; horizon schoon op alle 6 cams.",
      "SPEC-P2-03 Hollow Point gebankt (7194cf4) na review-ronde (3 blokkerende econ-fixes verwerkt): Intelligence Center vóór Medbay (pre-flip intel-schaarste), Slot B start als 5.2-bunkerkamp (squad-pick nooit gegate), missie-debrief committeert +1 dag (cross-spec gemarkeerd), econ-paden als nachtelijke soak-asserts.",
      "Groene bar: build ✓, 31/31 ✓, validatie 0 ✓; art-fix-ronde (stains/placards/well) + P2-01 draaien door." ] },
    { datum: "Sessie 2026-07-23 (stap 2 — character-body-pipeline GELAND)", punten: [
      "11 Fab-packs binnen (eigenaar): 8× RAISOR SciFi-soldiers + Paragon Belica/Twinblast/Minions (~50 skeletal meshes, ~950 animaties; 10,8 GB — machine-lokaal, gitignored).",
      "Sleutelvondst: de hele SciFi-familie deelt de UE4-Mannequin-skeletfamilie → anim-rijke packs voeden anim-arme, geen retargeting op dit tier.",
      "DT_BodyDefs (9 bodies, registry-geresolved) + FEclipseBodyDefRow + ApplyBodyDef op de gedeelde AEclipseCharacter; DT_EnemyArchetypes uitgebreid naar 5 Dominion-archetypes (09.3) mét eigen body en stats; vijand-spawn cyclet archetypes; squad uit de Rebel-pool; speler = Belica.",
      "MetaHumanCharacter + MetaHumanSDK + Python-plugins aan; shot-rig kreeg een body-showcase door het echte datapad; Fab-packs bewust buiten git.",
      "Groene bar: build ✓, 31/31 ✓ (incl. squad-suite), validatie 0 ✓, catalog 19/19 ✓."
    ]},
    { datum: "Sessie 2026-07-23 (loop-iteratie 6 — eerste bewoners)", punten: [
      "EERSTE CHARACTERS in het district: Quaternius CC0-pack (BlueSoldier M/V + 2 burgers, rigged mét animatiesets incl. Shoot/Death/RecieveHit — de squad-werkwoorden van later) via gdown gedownload en als skeletal meshes geïmporteerd.",
      "Geplaatst als Idle-figuren: Dominion-enforcers bij poort en checkpoint (goud cel), burgers bij het warehouse (grijs-teal) — visueel niveau, geen AI/collision (Part 9-werk komt later).",
      "Twee materiaallessen gebankt: skeletal-usage-flag ontbrak (figuren renderden zwart in -game) en bijna-neutrale ×10-paletten wassen naar grijs (enforcer nu verzadigd goud).",
      "Fab-library: eigenaar ingelogd (Launcher + fab.com); wacht op 'Add to Project'-kliks — monitor vuurt automatisch zodra content in Eclipse/Content landt.",
      "Groene bar: build ✓, 31/31 ✓, validatie 0 ✓, catalog 19/19 ✓."
    ]},
    { datum: "Sessie 2026-07-23 (loop-iteratie 5 — bezettings-decals via Pillow)", punten: [
      "PILLOW IN GEBRUIK: Tools/generate_decals.py genereert abstracte luminantie-decals (AEGIS-oog-poster, hazard-strepen, verzets-eclips-stencil) — kleur komt ALTIJD van het palet (Dominion-goud, rebel-rood, amber), dus palet-discipline is structureel geborgd.",
      "Decals geplaatst als no-collision planes: propaganda op de compound-gevels, hazard-pads bij de kruising, stencils bij Entry_Main en op het warehouse — het district vertelt nu bezetting én verzet (15.5/03.3).",
      "Zelfde meetdiscipline als textures: gains per decal gemeten (7.8/1.3/7.1), niet geschat.",
      "Groene bar: build ✓, 31/31 ✓, validatie 0 ✓, catalog 19/19 ✓."
    ]},
    { datum: "Sessie 2026-07-23 (loop-iteratie 4 — eerste echte props + tools)", punten: [
      "EERSTE ECHTE 3D-MESHES in het district: CC0-props van Poly Haven (Barrel_01, concrete_road_barrier, plastic_crate_03) — FBX + diffuse gedownload via API, geïmporteerd (Tools/import_polyhaven_props.py), geplaatst als vaten-clusters, checkpoint-barrières en krat-stapels (no-collision dressing, deterministisch).",
      "Toon-materiaal kreeg een mesh-UV-pad (UVMode) zodat geïmporteerde props hun eigen UVs gebruiken; alle material-slots krijgen de toon-MID; tonemapper-les: bijna-neutrale heldere paletten worden bleek — vat-palet naar donker verzadigd roest.",
      "Tools geïnstalleerd met owner-akkoord: Pillow 12.3 ✓ (texture/decal-generatie), ffmpeg ✓ (audio-pijplijn); Blender staat in de UAC-goedkeurings-wachtrij (geen blokker: UE heeft glTF-import en Quaternius levert FBX).",
      "Eigenaar heeft fab.com-login gezet; wachtrij voor eigenaar: Launcher-login + pack-shortlist aanklikken, ElevenLabs Music/SFX-scopes.",
      "Groene bar: build ✓, 31/31 ✓, validatie 0 ✓, catalog 19/19 ✓."
    ]},
    { datum: "Sessie 2026-07-23 (loop-iteratie 3 — lit-toon-experiment)", punten: [
      "Lit-toon-migratie gebouwd als veilig A/B-experiment: tweede master M_EclipseToonLit (DefaultLit — cel-banden als BaseColor, echt VSM/Lumen-licht erbovenop), alleen actief met -EclipseLitToon op SM6; Glow-strips blijven altijd unlit-emissive; default onveranderd.",
      "A/B-verdict (eerlijk): bij de schemerzon op command-afstand vrijwel niet te onderscheiden van unlit (auto-exposure normaliseert) — beslissing wacht op interieur/dag-scènes waar GI/schaduw er echt toe doen; unlit blijft de gelockte default.",
      "Groene bar: build ✓, 31/31 ✓, validatie 0 ✓, catalog 19/19 ✓.",
      "Eigenaar-vraag beantwoord: CC0-spoor volledig autonoom (bewezen); Fab-spoor wacht op éénmalige Epic-login + per-pack library-klik; ElevenLabs Music/SFX-scopes voor taak D."
    ]},
    { datum: "Sessie 2026-07-22 (nacht — loop-iteratie 2)", punten: [
      "Palet-discipline structureel: texture-variatie is nu luminantie-only in de shader — een roestige texture kan een Dominion-gevel nooit meer verkleuren; de post staat weer op het gelockte zalm-oxide.",
      "Straat-dressing als no-collision-deco: hoofdader + dwarsstraat met rijstrook-markering en 14 olie/roestvlekken; natrium-checkpointstrips (12) op de binnenmuren — de plaza vertelt nu een bezettingsverhaal (15.5).",
      "SPEC-P2-00 (Vertical Slice-overview) opgeleverd via subagent en gereviewd: build-volgorde P2-01…09, events per spec, testeisen, non-goals; ACTIVE_MILESTONE-omzetting blijft expliciet aan de eigenaar.",
      "Groene bar: build ✓, 31/31 ✓, validatie 0 ✓, catalog 19/19 ✓."
    ]},
    { datum: "Sessie 2026-07-22 (nacht, sterke PC — asset-pass gestart)", punten: [
      "EERSTE ECHTE TEXTURES in de game (taak C, owner-akkoord): 4×2K CC0-diffuse van Poly Haven (asfalt, betonblok, golfplaat, metaalplaat) — gedownload via API, geïmporteerd via nieuw script Tools/import_polyhaven_textures.py, herkomst in Content/Art/Textures/SOURCES.md.",
      "M_EclipseToon uitgebreid met world-aligned albedo-pad (dominante-as-projectie, default volledig neutraal); builder koppelt per palet-entry texture + schaal.",
      "Exposure-les geleerd en gefixt: textures her-meterden de auto-exposure (schemer werd dag) — per texture het lineaire gemiddelde gemeten en gain = 1/gemiddelde gezet (klem op 2.5 tegen hotspot-krassen); de gebankte dusk-grade is terug.",
      "Fab-route gedocumenteerd: wacht op éénmalige Epic-login van de eigenaar (Launcher nooit gestart op deze pc); CC0-spoor loopt ondertussen autonoom door.",
      "Groene bar: build ✓, 31/31 ✓, validatie 0 ✓, catalog 19/19 ✓."
    ]},
    { datum: "Sessie 2026-07-22 (avond, sterke PC — vervolg)", punten: [
      "Shot-rig gefixt: pawn in flying-mode tijdens de rig — overview-camera 4 kadreert nu het hele district (viel eerst 2 s naar de grond vóór elke capture).",
      "Hatch-tuning: 25% duty + periode 120 — schaduw-arcering leest als penseelstroken, niet meer als golfplaat.",
      "SM6-fidelitypad in de builder (feature-level-gated, laptop onaangetast): volumetric smog + zon-schaduwen (lichtschachten), realtime skylight, film grain 0.07 + bloom 0.45 per de 15.5-revisie.",
      "KESSARA-SKYLINE (03.3, code-built placeholder): deterministische ring (seed 503) buiten de perimeter — 56 fabriekshulks, 18 schoorstenen, 12 kraanportalen, natrium-raamstroken; het district staat nu in een stad. Buiten nav/missies, tests onaangetast.",
      "Groene bar na alles opnieuw: build ✓, 31/31 ✓, validatie 0 ✓, catalog 19/19 ✓."
    ]},
    { datum: "Sessie 2026-07-22 (avond, sterke PC)", punten: [
      "Eerste sessie op de sterke PC: groene bar onafhankelijk herbevestigd (build ✓, 31/31 tests ✓, validatie 0 fouten ✓, catalog 19/19 ✓).",
      "Machine gemeten: GTX 1080 Ti 11 GB — SM6 met Nanite/VSM/TSR/software-Lumen, géén RT-cores (HWRT-validatie blijft RTX-werk); charter 15.2 bijgewerkt.",
      "Owner-revisie 15.5 vastgelegd: Borderlands-leunend blijft gelockt, fidelity erbinnen omhoog (Nanite-dichtheid, software-Lumen-GI, SSAO/bloom/film grain, particles, TSR).",
      "Beide materiaal-scripts gedraaid; INKTLIJNEN LIVE: eerste-orde depth-Sobel overspoelde de scherende vloer met inkt (heel middenveld zwart) — herbouwd op Laplaciaan (2e afgeleide), silhouet- én naadlijnen schoon, vloer loopt weer door tot de horizon.",
      "Galerij ververst met de geverifieerde sterke-PC-shotronde (review-camera's 1-4)."
    ]},
    { datum: "Sessie 2026-07-22 (namiddag)", punten: [
      "GRAPHICS-DOORBRAAK: de 'paarse waas' van 7 passes bleek het oude outline-postmateriaal dat het hele beeld overschilderde; toon-materiaal M_EclipseToon (cel-banden + hatching, Unlit) bewezen werkend, inktlijnen herbouwd als PP_EclipseInk-script (nog te draaien + shotronde).",
      "Xbox-controller + muis samen speelbaar: stick-lopen/kijken, RT vuren, D-pad squad-orders, LB stance — feel-pass volgt in Phase 2.",
      "MIGRATION_TO_STRONG_PC.md herschreven als dag-één-draaiboek: consent-protocol (eerst uitleggen, dan akkoord, -NoUba), asset-beleid (algemeen downloaden / hero handgebouwd op gelijk niveau), bootstrap-prompt voor de nieuwe PC (§7).",
      "Groene bar herbevestigd na alle wijzigingen: build ✓, 31/31 ✓."
    ]},
    { datum: "Sessie 2026-07-22 (middag)", punten: [
      "Groene bar onafhankelijk herbevestigd: build ✓, 31/31 tests ✓, validatie 0 fouten ✓, catalog 19/19 ✓.",
      "Dialogue-seed gebouwd (16.12): Content/Audio/DialogueSeed.json → voice-assets via de commandlet (create-only), pure parser + 31e headless test.",
      "EERSTE LIVE ELEVENLABS-RUN GESLAAGD: 8/8 barks gegenereerd (PCM→WAV), geïmporteerd als USoundWave en auto-toegewezen; herhaalrun = 8 cache-hits, 0 API-calls.",
      "Voice-cache gecommit — geen enkele machine betaalt deze regels opnieuw.",
      "Let op: key werkt voor TTS maar /v1/voices geeft 401 (scope-beperkt); rotatiestatus bevestigen is aan de eigenaar."
    ]},
    { datum: "Sessie 2026-07-22", punten: [
      "Onafhankelijke her-review van alle Phase-1-code afgerond.",
      "4 defect-fixes gecommit: missions-served-teller, cover-scorer vriend/vijand, data-wiring speler/wapens/vijanden + speler-revive, UI-foutmeldingen.",
      "Ink-outline post-materiaal geauthord; district-dressing + -EclipseShot screenshot-rig gebouwd (19 review-passes, SM5-limieten gedocumenteerd).",
      "Audio-pipeline afgerond conform phase0/OWNER_MANDATE.md: PCM→WAV, runtime-PlayLine, auto-assign + bulk-commandlet, 2 nieuwe tests.",
      "Live dashboard omgebouwd naar auto-lezend systeem (progress_data.js + progress_auto.js + watcher)."
    ]}
  ]
};
