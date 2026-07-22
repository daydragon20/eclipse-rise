// ECLIPSE voortgang — INSCHATTINGSDATA (handmatig, door de dev-sessie bij milestones bijgewerkt).
// De automatische feiten (commits, tests, verse screenshots) staan in progress_auto.js
// en worden door Tools/update_progress.ps1 gegenereerd — daar blijf je vanaf.
// Regels: percentages zijn eerlijk (liever te laag dan gejokt); geen HTML hier, alleen data.
window.PROGRESS_DATA = {
  bijgewerkt: "2026-07-22 15:15",
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
      titel: "3 · Graphics", scope: "Part 15 — echt werk start op de RTX-PC",
      totaal: 14,
      items: [
        { naam: "Art-richting gelockt (Borderlands-stilering)", pct: 100 },
        { naam: "Ink-outline post-materiaal", pct: 80 },
        { naam: "Belichting / mood dev-box (SM5)", pct: 70, notitie: "Milestone gebankt: schemer-twee-tonen; herkalibratie op RTX." },
        { naam: "Kleurenpalet & blok-dressing", pct: 25 },
        { naam: "Gebouwen (echte kits)", pct: 10 },
        { naam: "Straten / props / decals", pct: 5 },
        { naam: "Bomen / vegetatie", pct: 0, notitie: "Eerste vegetatie Phase 3 (Sylvaris)." },
        { naam: "Characters / MetaHumans", pct: 0, notitie: "Phase 2→3." },
        { naam: "Weer & dag/nacht", pct: 0 },
        { naam: "VFX", pct: 0 },
        { naam: "Nanite / Lumen / VSM op target-hardware", pct: 0, notitie: "Wacht op RTX-PC (deze laptop: SM5-fallback)." }
      ]
    },
    {
      titel: "4 · Audio", scope: "Part 16",
      totaal: 60,
      items: [
        { naam: "ElevenLabs TTS-client (HTTPS, key-hygiëne, PCM/WAV)", pct: 100 },
        { naam: "Cache + manifest (nooit 2× dezelfde regel)", pct: 100, notitie: "Cache reist mee in Content/Audio/Generated (16.12)." },
        { naam: "Runtime-playback in game", pct: 85, notitie: "PlayLine af; eerste live-API-run nog te doen." },
        { naam: "Auto-assign naar Dialogue DataAssets", pct: 90 },
        { naam: "Editor-bulk-tool (-run=EclipseGenerateVoices)", pct: 90, notitie: "Zonder key: nette cache-only run." },
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
        { naam: "Dialogen geschreven", pct: 0 },
        { naam: "VO gegenereerd", pct: 0 },
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
  screenshotNoot: "Dev-laptop (GTX 1050, SM5) — gestileerde dressing-pass; echte fidelity-pass volgt op de RTX-PC.",

  changelog: [
    { datum: "Sessie 2026-07-22", punten: [
      "Onafhankelijke her-review van alle Phase-1-code afgerond.",
      "4 defect-fixes gecommit: missions-served-teller, cover-scorer vriend/vijand, data-wiring speler/wapens/vijanden + speler-revive, UI-foutmeldingen.",
      "Ink-outline post-materiaal geauthord; district-dressing + -EclipseShot screenshot-rig gebouwd (19 review-passes, SM5-limieten gedocumenteerd).",
      "Audio-pipeline afgerond conform phase0/OWNER_MANDATE.md: PCM→WAV, runtime-PlayLine, auto-assign + bulk-commandlet, 2 nieuwe tests.",
      "Live dashboard omgebouwd naar auto-lezend systeem (progress_data.js + progress_auto.js + watcher)."
    ]}
  ]
};
