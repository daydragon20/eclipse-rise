// ECLIPSE voortgang — INSCHATTINGSDATA (handmatig, door de dev-sessie bij milestones bijgewerkt).
// De automatische feiten (commits, tests, verse screenshots) staan in progress_auto.js
// en worden door Tools/update_progress.ps1 gegenereerd — daar blijf je vanaf.
// Regels: percentages zijn eerlijk (liever te laag dan gejokt); geen HTML hier, alleen data.
window.PROGRESS_DATA = {
  bijgewerkt: "2026-07-22 21:35",
  hero: { label: "Hele game", pct: 8 },
  playtestChip: "PIE-playtest door eigenaar: OPEN (gate-vraag 13.2)",

  secties: [
    {
      titel: "1 · Roadmap", scope: "Part 13",
      items: [
        { naam: "Fase 0 — Pre-productie", pct: 90, notitie: "Open: CI-runner, concept-art 0/10, feel-clips 0/5." },
        { naam: "Fase 1 — Prototype \"The Loop\"", pct: 95, notitie: "Alle 8 specs + live loop af. Open: gate-vraag (eigenaar)." },
        { naam: "Fase 2 — Vertical Slice \"Thirteen Bullets\"", pct: 2, notitie: "Art-richting gelockt + outline-materiaal." },
        { naam: "Fase 3 — Early Build", pct: 0 },
        { naam: "Fase 4 — Alpha", pct: 0 },
        { naam: "Fase 5 — Beta", pct: 0 },
        { naam: "Fase 6 — Release", pct: 0 }
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
        { naam: "Cel/toon-materiaal (banden + hatching, M_EclipseToon)", pct: 95, notitie: "Geauthord + live geverifieerd op de sterke PC; hatch-schaal leest nog als golfplaat — tunen." },
        { naam: "Ink-outline post-materiaal", pct: 85, notitie: "PP_EclipseInk LIVE bewezen: Sobel→Laplaciaan-fix (scherende vloer vloeide vol inkt); silhouet- + naadlijnen schoon." },
        { naam: "Belichting / mood", pct: 80, notitie: "SM5-kalibratie gebankt; eerste SM6-shotronde (sterke PC) rendert de schemer-mood correct." },
        { naam: "Kleurenpalet & blok-dressing", pct: 35, notitie: "Lit/shade-paren met hue-shift per bloktype." },
        { naam: "Gebouwen (echte kits)", pct: 10 },
        { naam: "Straten / props / decals", pct: 5 },
        { naam: "Bomen / vegetatie", pct: 0, notitie: "Eerste vegetatie Phase 3 (Sylvaris)." },
        { naam: "Characters / MetaHumans", pct: 0, notitie: "Phase 2→3." },
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
  screenshotNoot: "Sterke PC (GTX 1080 Ti, SM6, Scalability 3) — eerste ronde mét werkende PP_EclipseInk-inktlijnen; camera 4 kadreert laag (pawn zakt vóór capture — rig-punt).",

  changelog: [
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
