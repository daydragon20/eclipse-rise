# ECLIPSE — EXECUTION PLAN (NU → RELEASE)
*Werkdocument game-planner | aangemaakt 2026-07-23 | laatst bijgewerkt 2026-07-24 ~17:35 (sync na cyclus 2: Stage A + P2-03 stap 1–2 + pack-slim geland; vijf sporen in flight) | risk-first per 13.1*
*Bronnen: 13_roadmap.md, SPEC-P2-00, 14_ai_dev_instructions.md (14.4/14.5), 12_technical_design.md, HANDOFF.md "LAATSTE STAND".*

**Status:** Phase 2 — Vertical Slice "Thirteen Bullets" is per expliciete owner-instructie (2026-07-23) de **actieve milestone**. De 13.2-gatevraag van Phase 1 ("speelt een tester vrijwillig een tweede loop?") blijft open als **staande owner-actie** (§4, T-1) — het antwoord kan de fidelity-investering nog bijsturen, maar blokkeert de Phase-2-systemenbouw niet.

**Leesregels:** bouwvolgorde per spec volgt altijd 14.5 (dataschema → pure-logic core + unit tests → subsystem-wrapper + events → debug-UI → echte UI/content laatst). Elke taak eindigt build-groen. Bouwer: **main** = hoofdagent (architectuur/systemen/specs, seams per 14.6), **eb** = element-builder(s), parallel spawnbaar, één element per agent.

---

## 1. Genummerde backlog — NU tot release

### Fase 2 — VERTICAL SLICE "Thirteen Bullets" (actief; volgorde = SPEC-P2-00 §Spec set)

| # | Item | Doel (één zin) | GDD-ref | Afhankelijk van | Bouwer |
|---|---|---|---|---|---|
| 1 | Milestone-administratie | ACTIVE_MILESTONE-blok → Phase 2 flippen en HANDOFF §4/§5 + `progress_data.js`-takenlijst op de Phase-2-backlog zetten, met de 13.2-playtest expliciet als open owner-actie. | 13 (ACTIVE_MILESTONE), dashboard-workflow | owner-instructie 2026-07-23 (binnen) | main |
| 2 | SPEC-P2-01 Squad of 4 & Classes — **GELAND 2026-07-24** | Geland in `1a85b78` (review GO, 0 blockers): squad van 4, Assault/Medic/Sniper als púre data (`DT_ClassDefs`), save v2→v3 + fixtures per 14.3.6, derde validator; 38/38 ✓, validatie 0 ✓, catalog 21/21 ✓. Follow-ups m4/m6/m7 ✓ afgehandeld in `3c2f3a5`. | 4.2.2/4.2.3, 8.3 | — | main |
| 3 | Body-swap echte meshes — **materieel vervuld** | Vervuld via de DT_BodyDefs-body-pipeline (sessie 23-07): speler/squad/vijanden dragen echte meshes (Belica, RAISOR-soldiers), 5 Dominion-archetypes; Quaternius blijft reserve/bewoners. Rest-QC lift mee in de 15.8-rondes. | 12.3 (character), 15.5 | — | eb |
| 4 | SPEC-P2-02 Command Mode final feel | Command Mode van debug-orderpicker naar shipping feel: hold-to-enter, 30% dilatatie, 8.4-ordertabel, verbale refusals ("orders zijn beloftes"). *Spec gebankt `29cd549`; **Stage A GELAND** `b5aa157` (review GO) — de feel-gauntlet staat als owner-actie (§4 T-11, R3-verdict open); **Stage B pas ná verdict "true"**.* | 8.4, 4.1.2, 9.5 | #2 ✓ | main |
| 5 | SPEC-P2-03 Hollow Point walkable base | Menu-basis vervangen door de beloopbare Act-1-vault met 4 faciliteiten op de strategische klok en zichtbare groei (besluit: Intelligence Center vóór Medbay). *Spec gebankt `7194cf4`; **stap 1–2 GELAND** `deabc9f` (review GO; 5 bevindingen verplicht naar stap 3); **stap 3 in uitvoering** (§2-F3).* | 5.1–5.4, 12.3 | P1-08 ✓, P1-03 ✓ | main (systeem) + eb (vault-shells) |
| 6 | SPEC-P2-04 Missies M1.1–M1.4 | De vier openingsmissies autoren op de gedeelde quest-runtime met de volle 11.1-loop en stealth-viable routes. *Spec geschreven + main-review verwerkt (**ACCEPTED**; 4 besluiten in de spec: geen prologue/recap-opening, R7-Gauntlet als bouwstap 1, nul nieuwe primitieven, Brick = Assault); **bouwstap 1 (R7-skelet) in flight** (§2-F1).* | 2.9, 11.1, 11.4, 12.3 | #2, #4, #5, P1-05 | main (runtime/graphs) + eb (per missie-site) |
| 7 | SPEC-P2-05 Liberation-instance | Eén campagne-missietemplate instantiëren op Kessara's districtsgraaf zodat M1.3 zichtbaar regiostaat flipt en de strategische laag antwoordt. *Concept-spec **in uitvoering** (eb, §2-F5; open vraag 4 = schaal, voorstel Foothold/3 regio's); main-review = §2-N4.* | 11.2, 11.3-inputs | #6, P1-04 | main |
| 8 | SPEC-P2-06 Save v1 | Save v0 promoveren naar de versioned plugin met per-subsystem contracten, v0-migratie, autosaves en missie-checkpoints — vóór content-freeze zodat elke nieuwe struct serialiseerbaar landt. | 12.3 (save), 14.3.6 | #2, #5, #7 | main |
| 9 | SPEC-P2-07 UI Stack v1 | Debug-UI vervangen door de CommonUI-stack (HUD, command, basis, map, prep/debrief) met controller+M/KB-pariteit; UI is pure consumer van de bus. | 12.1, 8.8 | #4, #5, #6 | main (C++ bases) + eb (widgets/schermen) |
| 10 | SPEC-P2-08 Fidelity-district Kessara Underworks | Het slice-district aankleden tot de gelockte 15.5-revisie (Nanite-dense kits, software-Lumen, cel+ink) door de 15.8-loop tot 15.12 "AAA-ready" binnen 12.4-budgetten. | 15.5/15.8/15.12, 3.3 | #6 (spaces locked), Fab-kliks (owner, §4) | eb ×N parallel (kit-pass, verticaliteit, particles, interieurs) |
| 11 | SPEC-P2-09 Audio-infrastructuur | 16.7 adaptieve muziek (verticale lagen, Quartz, never-silent) + combat-audio-basics + squad-barks als primaire game-state-audio, TTS-placeholder. | 16.7, 16.9, 16.14, 16.4 | #4, #6; live-runs achter ElevenLabs-scopes (§4) | main (systeem) + eb (stems/barks) |
| 12 | Phase-2-testlagen compleet | Alle zes 14.4-lagen draaiend voor de slice: Gauntlet per missie, scenario-suite op 4+klassen, save-roundtrip, perf per merge, nightly soak M1.1→liberation. | 14.4, SPEC-P2-00 §Tests | groeit mee met #2–#11; CI-runner (§4) | main |
| 13 | Gate-review Phase 2 | Externe reviewers spelen de 3 uur koud; verdict "I want the rest of this game" ja/nee — bij "nice systems, but…" eerst de feel/fidelity-kloof dichten. | 13.2, SPEC-P2-00 §DoD | #1–#12 | owner + main |

### Fase 3 — EARLY BUILD "One Planet War" (na gate #13)

| # | Item | Doel | GDD-ref | Afhankelijk van | Bouwer |
|---|---|---|---|---|---|
| 14 | Kessara + Tarsis compleet | Twee volledige planeten met alle districten, occupation-states en missie-sites. | 3.3/3.4, 13.2 | #13 | eb ×N + main (World Partition/Data Layers) |
| 15 | Act 1 volledig + Act 2-skelet | M1.5–M1.8 plus de Act-2-ruggegraat op dezelfde runtime; companions voorbij Mara+Brick. | 2.9, 11 | #6, #14 | main + eb |
| 16 | Alle 9 klassen + Training Academy | Klassenroster compleet met Academy-toewijzing (het Phase-2-besluit "pre-classed" vervalt hier). | 4.2.3, 5.3 | #2, #5 | main |
| 17 | Basis-tiers 1–2 + officer/company-laag v1 | Basisgroei voorbij de 4-facility-vault en de eerste officerslaag. | 5.4, 7.x | #5, #8 | main |
| 18 | Dominion Response Tiers 0–3 + economie-vol-loop | Strategische tegenspeler (utility-planner op strategische tijd) en de volledige economieband per Part 6. | 9.x, 6.x, 12.3 | #7, #8 | main |
| 19 | Mission Generator v1 | Generator componeert dezelfde objective-primitieven als de authored missies, op beide planeten. | 11.3, 12.3 | #6, #7, #14 | main |
| 20 | Hybrid battle v1 — "Siege of Hollow Point" | Derde existentiële risico bewijzen: grond+simulator in één gevecht, front-swap ≤10 s. | 8.6, 12.4, 13.1 | #17, #18 | main |
| 21 | MetaHuman-evaluatie hero-companions | Beslissen (en zo ja pipelinen) of Mara/Brick/companions MetaHuman-gezichten krijgen. | 12.1, HANDOFF it.4 | owner-sessie (§4) | main + owner |
| 22 | Gate: 10-uurs externe playtests | Retentie meten op een ~25 u-campagne. | 13.2 Phase 3 | #14–#20 | owner |

### Fase 4 — ALPHA "The Whole War, Ugly"

| # | Item | Doel | GDD-ref | Afhankelijk van | Bouwer |
|---|---|---|---|---|---|
| 23 | Alle 10 planeten graybox→dressed | Volledige wereldruimte speelbaar (scope-ladder: 10→8 bij nood, besluit vooraf gedocumenteerd). | 3.x, 13.1 | #22 | eb ×N + main |
| 24 | Alle acts + twists/branches + tech tree compleet | Verhaal start-tot-eind speelbaar met volledige onderzoeksboom (graaf zonder orphans/cycles, unit-getest). | 2.x, 10.x, 14.4 | #15, #19 | main + eb |
| 25 | Fleet-laag | Vlootlaag erin (cut line vooraf: 2D-tactische kaart, simulator ongewijzigd; schepen nooit player-piloted). | 7.x, 12.3, 13.1 | #20 | main |
| 26 | Content-complete + maandelijkse bot-playthrough | Elke missie/template/systeem bestaat; maandelijkse volledige bot-campagne + save-integriteits-soaks draaien. | 13.2 Phase 4, 14.4 | #23–#25 | main |
| 27 | Gate: campagne uitspeelbaar zonder dev-ingreep | Volledige campagne haalbaar door de bot én een mens. | 13.2 | #26 | owner |

### Fase 5 — BETA · Fase 6 — RELEASE+POST

| # | Item | Doel | GDD-ref | Afhankelijk van | Bouwer |
|---|---|---|---|---|---|
| 28 | Content-lock + balanspassen per act | Telemetrie-gedreven balans (casualty-rates, economie-curves binnen Part-6-banden). | 13.2 Phase 5, 6.x | #27 | main |
| 29 | VO-opname + implementatie | TTS-placeholders vervangen door opgenomen VO (companions/story), barks-accentensets. | 16.4, 12.3 | #28, contract (§4) | owner (contract) + main |
| 30 | Performance min-spec + accessibility + lokalisatie | 60 fps-doelen op min-spec, accessibility compleet, EN-VO + FIGS+NL+PL+BR+RU+ZH-tekst. | 12.4, 13.2 | #28 | main + eb |
| 31 | Closed beta per act → gate crash-free ≥99,5% | Beta-programma draaien en de funnel gezond krijgen. | 13.2 Phase 5 | #28–#30 | owner + main |
| 32 | Gold + day-one patch + hotfix-discipline | Release en de launch-window bemand. | 13.2 Phase 6 | #31 | owner + main |
| 33 | Post-launch: NG+ → QoL → "Ashfall"-drop → expansie-evaluatie | Vaste post-launch-volgorde afwerken; NG+ nooit launch-blocking. | 13.1/13.2 | #32 | main + eb |

---

## 2. Sprintbord — lopend + eerstvolgend (bijgewerkt 2026-07-24 ~17:35, sync na cyclus 2)

DoD-basis (14.4): *spec gerefereerd + code + data + tests + EventCatalog/docs bijgewerkt + CI/lokale groene bar (build -NoUba ✓, tests ✓, validatie ✓, catalog ✓) + door een mens/reviewende agent gezien.*

**T-ledger (afgerond):** T1 ✓ milestone-flip · T2 ✓ SPEC-P2-01 · **T3+T4 ✓ P2-01 GELAND** `1a85b78` (review GO, 0 blockers; save v2→v3 per 14.3.6; 38/38) · T5 ✓ SPEC-P2-03 gebankt `7194cf4` · T8 ✓ SPEC-P2-02 gebankt `29cd549` · T6 vervallen (geen los GAS-ability-systeem nodig) · T7 materieel vervuld (DT_BodyDefs-body-pipeline).
**Cyclus 2 (24-07, geland én gepusht):** P2-02 **Stage A** `b5aa157` (review GO — de R3-falsificatiebuild staat; feel-gauntlet = owner-actie T-11, verdict open) · P2-03 **stap 1–2** `deabc9f` (review GO "R6-discipline voorbeeldig"; 5 bevindingen verplicht doorgeschoven naar stap 3) · **m4/m6/m7** `3c2f3a5` (spawn-fan + cover-scorer → `DA_SquadTuning`, catalog-formulering eerlijk; gedragsneutraal) · **pack-slim** `a0a39b9` (11 accepts → `/Game/Art/Imported`, Minions+SciFi10 van schijf, ~5,9 GB vrij; licentie-check OK — **repo moet privé blijven**, vastgelegd in SOURCES.md) · `DA_CommandModeTuning` gematerialiseerd `6fe5081` · **SPEC-P2-04 geschreven + main-review verwerkt (ACCEPTED)** — 4 besluiten in de spec zelf. Eindbar op de unie: build ✓, **47/47** tests ✓, validatie 3 validators/0 fouten ✓, catalog **23/23** ✓.

### Nu in flight — vijf sporen (stand ~17:30; nog niet gecommit; file-ownership botst niet)

**F1 — [Quests] R7-falsificatie-skelet + debrief-dag-regel** *(main; SPEC-P2-04 bouwstap 1)*
Headless `Eclipse.Missions.M11SkeletonCarriesAuthoredAsset`: authored `UEclipseMissionAsset` op het échte ResolveMissionSpec-laadpad — asserts: authored objectives actief (geen synthesized fallback), mandatory-set afgedwongen, rewards gecommit, regio ONaangetast (decision 6), klok +1 dag. In dezelfde changeset de **debrief-dag-regel** (SPEC-P2-03 locked decision 4): `ComposeConsequences` append AdvanceDay + dag-assert in FullCircleSmoke.
*Stand:* build groen, scoped review loopt; **testrun wacht op het commandlet-slot** (bij F4). Commit zodra review GO + bar groen → dan is R7 pending-green → bewezen.

**F2 — [Art]-mini: builder-string-swap** *(main)*
`EclipseGrayboxBuilder.cpp:161/720/741` naar de `/Game/Art/Imported`-paden; de 3 redirector-stubs (4 KB) blijven staan tot de 15.8-shotronde (N3c) bewijst dat niets meer via de oude paden loopt — dan pas verwijderen.

**F3 — P2-03 stap 3: wrapper + events** *(eb; backlog #5, 14.5-stappen 3–4)*
Transactie-API voor base-mutaties + `Event.Base.*` ×4 (catalog in hetzélfde commit) + subsystem-wrapper op de pure core + `setup_base_data.py`. Draagt de **5 reviewbevindingen verplicht** mee: (1) bemande-facility-round-trip-fixture + hash-assert zodra StaffAssign bestaat; (2) upgrade-completion NewLevel==2-test + yield-retentie/analist-suspensie; (3) crew-timing-semantiek expliciet beslissen; (4) ReleasedSoldierIds aan de completion-struct; (5) ApplyRush-guard.

**F4 — MetaHuman-curatie** *(eb; bezit het commandlet-slot)*
Inventaris `/Game/MetaHumans` (SentinelC + Common, 908 MB) + koppeling MH → `DT_NamedCharacters`-slots + toon-restyle-check per 15.7. *Stand (dashboard 17:35): curatieronde KLAAR* — SentinelC-map verplaatst naar zijn geauthorde pad (omkeerbaar), Kaya-slot provisioneel gekoppeld via `Tools/setup_metahuman_data.py`, validatie 0; **testrun 47/48 — de ene fail (MedicStabilizes, wound-days 7≠6) is per A/B pre-existing uit de [Quests]-lane** (zie N1). Afronding/commit maakt het commandlet-slot vrij. Resterende MH-items blijven owner-download-kliks (§4 T-8); het 15.7-toon-restyle-beslispunt ligt bij art-reviewer/owner.

**F5 — SPEC-P2-05-concept: liberation-instance** *(eb)*
Concept-spec voor backlog #7; open vraag 4 = schaal, met **Foothold/3 regio's** als voorstel. Main-review = N4.

### Daarna klaarstaand (volgorde)

**N1 — [Quests]+[Art]-commits landen** *(main)* — zodra F4 het commandlet-slot vrijgeeft: R7-testrun draaien; bij groen + review-GO beide changesets (F1, F2) committen/pushen. **Let op:** de 47/48-fail uit de F4-run (MedicStabilizes, wound-days 7≠6) is vrijwel zeker de debrief-dag-regel die de dag-boekhouding een dag opschuift — die test-verwachting **bewust en beargumenteerd** in de F1-changeset meenemen (niet stil laten passeren), zodat de bar op 48/48 landt.

**N2 — P2-03 stap 3 afronden** *(eb code-complete → main-review)* — groene bar op de nieuwe specs naast de 47, review, commit; vault-shells (eb) pas daarna.

**N3 — Commandlet-rij (serieel, één tegelijk, ná F4):** (a) **`setup_base_data.py`-materialisatie** (DT_Facilities/DA_BaseTuning/layout-asset — hoort logisch bij de N2-landing); (b) **audio-import 16.12** — de 8 gebankte assets via `Tools/import_generated_audio.py` naar `/Game/Audio`, Kessara-Layer-1-bed (never-silent floor) + sting op de mission-complete-beat; (c) **15.8-dressing-ronde** op de gemigreerde Imported-bronnen — rubble/warehouse-plaatsingen + watch-items (teal-fin cam 6, perimeterband cam 4, crosshatch-tiling, horizon-audit) + **cold-DDC-open-check** van de 7 SciFi10-albedo's (pack-slim-review NTH) + **stub-verwijdering** (na F2) + shotronde + art-review vóór commit.

**N4 — SPEC-P2-05 main-review** *(main)* — verdict op open vraag 4 (voorstel Foothold/3 regio's) + bank-commit; daarna is backlog #7 bouwklaar achter #6.

**N5 — P2-04 bouwstap 2: StoryFlags + DT_StoryMissions** *(main)* — schema-eerst per 14.5: `DT_StoryMissions` + StoryFlags in `FCampaignState` = save **v4→v5** mét migratie-entry + fixtures in hetzélfde commit (**derde R6-toets**). *Seam-note:* het `FCampaignState`/save-header-deel pas ná de N2-commit (die files zijn tot dan eb-ownership); het `Quests/`-deel (asset-schema, authored graphs) kan direct na N1.

**Wacht expliciet — nooit blind omheen plannen:**
- **Op het R3-verdict (owner-feel-gauntlet, §4 T-11):** P2-02 **Stage B** — volledige 8.4-ordertabel (5 nieuwe verbs op het bestaande order-contract), 3 nieuwe refusal-reasons, stealth-stance. Bij verdict "false": eerst spec-amendment (fallbacks per SPEC-P2-02) vóór enige polish.
- **Op owner-kliks:** MetaHuman-downloads (§4 T-8; F4 bedraadt intussen wat binnen is), env-pack-pulls + civilian/worker-pack (§4 T-2; blokkeert de kit-pass #10 en de SPEC-P2-03-bevolking). De downloads-annuleerlijst (T-10) staat in `ownerActies`.

---

## 3. Risicolijst — met vroegst mogelijke falsificatietest

| R | Risico | Vroegst mogelijke falsificatie |
|---|---|---|
| R1 | **Phase-1-loop is niet leuk** (13.2-gate nooit beantwoord) → alle slice-investering staat op een onbewezen loop. | **Vandaag, nul bouwwerk:** owner speelt de loop in PIE (controller + barks) en beantwoordt "tweede loop: ja/nee?". Bij "nee": eerst loop-pivot, fidelity-werk (#10) bevriezen. |
| R2 | **Squad-AI schaalt niet naar 4+klassen** — orders voelen niet langer "obeyed" (existentieel risico 2, 13.1). | Direct na T4: scenario-suite op 4 + PIE-ordertest op graybox, vóór er ook maar één klasse-ability of UI bestaat. Stille orderfout = stop. **Status 2026-07-24: falsificatie doorstaan** — scenario-suite op 4 mét klassen groen in de P2-01-landing (`1a85b78`, 38/38; medic-her-dispatch als review-fix erin). Bewaking blijft per merge via de suite. |
| R3 | **Command Mode-feel (30% dilatatie) overleeft echte gevechten niet** (leesbaarheid/chaos). | 14.5-stap-4-debug-versie van P2-02 in het bestaande graybox-district testen vóór UI/camera-polish — feel-verdict op de lelijkste versie. **Status 2026-07-24: falsificatiebuild GELAND** (`b5aa157`, review GO; 20×-exit-ladder + ack-scenario groen) — het verdict ligt nu bij de owner-feel-gauntlet (§4 T-11, draaiboek `phase0/FEEL_GAUNTLET_P2-02.md`; telemetry HeldSeconds/OrdersIssuedWhileHeld leest de agent uit). Stage B blijft dicht tot het verdict. |
| R4 | **15.12 "AAA-ready" onhaalbaar binnen 12.4-budgetten** op software-Lumen/1080 Ti. | Eén straatblok (niet het hele district) naar volle 15.5-revisie dressen en profileren tegen 12.4 op dev-preview-scalability — vóór de kit-pass districtbreed gaat. |
| R5 | **Fab-afhankelijkheid**: owner-kliks blijven uit → kit-pass (#10) stagneert. | Timebox: staan de shortlist-packs er over 7 dagen niet in, dan Quaternius/CC0+Blender-route als plan-of-record (al bewezen pipeline) en Fab als upgrade-pad. **Status: deels ontladen** — character-packs zijn binnen (11 Fab-packs, 23-07); de env-pack-kliks (§4 T-2) blijven de resterende afhankelijkheid voor de kit-pass. |
| R6 | **Save v1-migratie breekt v0-campagnes** (project-killer per 12.3). | Eerste schema-break in Phase 2: migratie-entry + v0-fixture-test in hetzélfde commit (14.3.6), per merge draaiend — niet wachten tot spec 06. **Status 2026-07-24: falsificatie bewezen werkend** — de eerste echte schema-break (v2→v3, roster-ClassId) landde mét migratie-entry + v0- én v2-fixture-tests in hetzélfde commit (`1a85b78`); pre-v3 saves landen deterministisch op classless. **Tweede bewijs geleverd 24-07:** de v3→v4-break (FEclipseBaseState) landde mét byte-getrouwe v3-fixture in hetzélfde commit (`deabc9f`, review "R6-discipline voorbeeldig"). De discipline herhaalt per break; eerstvolgende toets: P2-04 v4→v5 (StoryFlags, §2-N5). |
| R7 | **Quest-runtime draagt geen authored missies** (StateTree-fases/consequence-commits te licht voor M1.x). | M1.1-skelet als Gauntlet spawn→complete-by-script→CampaignState-asserts bouwen vóór M1.2–M1.4 geautoreerd worden. **Status 2026-07-24: skelet GEBOUWD, verdict pending-green** — `Eclipse.Missions.M11SkeletonCarriesAuthoredAsset` in flight (§2-F1): authored asset op het echte laadpad, mandatory-set, rewards, regio-onaangetast; de testrun wacht op het commandlet-slot. *Implicatie voor de economie:* de debrief-dag-regel in dezelfde changeset maakt "missie kost een dag" mechanisch waar — de P2-03-kalender (facility-ETA's, crew-dagen) klopt nu, en de econ-soak-asserts uit SPEC-P2-03 moeten met deze dag-kosten rekenen. |
| R8 | **Prologue-scope-explosie** (open vraag 5 maakt SPEC-P2-04 2 u groter). | Besluit bij spec-schrijven: slice opent bij M1.1 met briefing-recap; prologue alleen als de gate-review erom vraagt. Falsificatie = cold-reader begrijpt de recap zonder prologue. **Status 2026-07-24: BESLOTEN** in SPEC-P2-04 (ACCEPTED): geen prologue, recap-opening; de cold-reader-check staat als DoD-item in de spec. |
| R9 | **16.7-muziek blokkeert op ElevenLabs-scopes** (Music/SFX-endpoints dicht). | Eén live endpoint-call zodra owner scopes zet; tot dan verticale lagen bouwen op CC0-stems — systeem falsifieert zichzelf zonder API. |
| R10 | **12.4-budgetten stilletjes overschreden** naarmate Nanite-density groeit. | Per-merge budgetcheck op Underworks-referentiescène vanaf de éérste kit-pass-merge (niet pas bij spec 08-afronding); Insights-trace bij elke perf-verdachte PR (14.6 skill 7). |
| R11 | **Externe gate-reviewers niet geregeld** → Phase-2-gate wordt zelfbeoordeling. | Owner werft reviewers zodra M1.1+M1.2 speelbaar zijn (halverwege), niet aan het eind — eerste koude sessie is meteen een halfweg-falsificatie. |

---

## 4. Menselijke-touchpoints-wachtrij (owner-acties, gequeued — nooit blind omheen plannen)

*(Gesynchroniseerd met `ownerActies`/`jijGedaan` in `progress_data.js`, 2026-07-24 ~17:35 — de feel-gauntlet staat daar nu bovenaan, met T-10 (downloads annuleren) erachter.)*

| T | Actie | Waarvoor / blokkeert | Wanneer |
|---|---|---|---|
| T-1 | **Playtest 13.2**: loop in PIE spelen (Xbox-controller, squad-barks) en de gate-vraag beantwoorden. | Eerlijk Phase-1-verdict; stuurt R1 en de fidelity-prioriteit. | **Nu — staande actie**, ~30 min |
| T-2 | **Fab-kliks (rest)**: login ✓ en de character-packs zijn binnen; resterend zijn de env-pack-pulls (Factory Pack Vol.1, Industrial Building 49 PBR, UNIBLOCKS, Sci-Fi Hallway, Sci-Fi Light Pack, Auto Footsteps Utility, Niagara Footstep VFX, FPS Weapon Bundle, Free Muzzle Flash) + **1 gestileerd civilian/worker-pack** (4–6 bodies, Mannequin-rig — het enige character-gat: Hollow Point-crew/idlers en Kessara-burgers close-up). | Kit-pass backlog #10 + SPEC-P2-03-bevolking; monitor vuurt daarna autonoom. | Gauw, ~10 min (R5-timebox loopt op de env-packs) |
| T-3 | ~~ElevenLabs-scopes~~ — **✓ gedaan 2026-07-23** (Music/SFX aangezet, zie `jijGedaan`). | — | Afgerond |
| T-4 | ~~Blender-install~~ — **✓ gedaan 2026-07-23** (zie `jijGedaan`). | — | Afgerond |
| T-5 | **Mixamo-kliklijst** afwerken zodra de animatie-pass start (Quaternius-CC0 is het autonome alternatief). | Animatiekwaliteit body-pass (backlog #3-QC). | Bij animatie-pass |
| T-6 | **CI self-hosted runner** goedkeuren/aanwijzen (Phase-0-carryover). | 14.4-lagen per merge i.p.v. lokaal (backlog #12). | Deze fase, niet-blokkerend |
| T-7 | **Externe reviewers werven** voor de Phase-2-gate (koud, geen teamleden). | Backlog #13 + R11-halfwegtest. | Zodra M1.1+M1.2 speelbaar |
| T-8 | **MetaHuman-download-kliks**: basis is binnen (SentinelC + Common, 908 MB); resterend 1× Download-klik per overig MH-item in Window → Fab → My Library + sein "MetaHumans staan erin". De curatie loopt intussen al op wat binnen is (§2-F4). De Creator-sessie voor hero-companions blijft de Phase 2→3-evaluatie (backlog #21). | Rest van de MetaHuman-curatie (named-character-slots). | Gauw, ~5 min |
| T-9 | Verder weg: VO-opnamecontracten, lokalisatievendors, closed-beta-programma (Phase 5); storefront/gold-beslissingen (Phase 6). | Backlog #29–#32. | Phase 5+ |
| T-10 | **Overbodige downloads annuleren** (Launcher → Downloads): alle Paragon-helden (Kwang, Narbash, Wukong, Riktor, Sevarog…), Old West VOL 4/6, Path of Adventure JRPG-muziek, NWIRO AI Pro; láát staan: Photogrammetry Snow, Plant Models Vol 60, Perfect Fire VFX. | Bespaart ~30 GB schijf. | Nu, ~2 min |
| T-11 | **P2-02 Stage A feel-gauntlet** (~20 min): de gelande Stage-A-build (`b5aa157`) spelen per draaiboek `phase0/FEEL_GAUNTLET_P2-02.md` (hold Q/LB → 30%, orders 1-4/D-pad, selectie Tab/RB of E/X), de 5 criteria scoren en "R3-verdict: true/false" + observaties doorgeven; telemetry (HeldSeconds/OrdersIssuedWhileHeld via de ModeExited-payload) leest de agent uit. | Beslist Stage B vs. spec-amendment (§2 "Wacht expliciet"). | **NU — staat bovenaan `ownerActies`** |

*Consent-protocol blijft gelden: installs/downloads/security-prompts alleen na uitleg + expliciet akkoord; bouwen met `-NoUba`; PROGRESS.html nooit direct bewerken.*
