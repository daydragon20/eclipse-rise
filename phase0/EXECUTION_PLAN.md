# ECLIPSE — EXECUTION PLAN (NU → RELEASE)
*Werkdocument game-planner | aangemaakt 2026-07-23 | laatst bijgewerkt 2026-07-24 ~16:40 (na P2-01-landing `1a85b78`) | risk-first per 13.1*
*Bronnen: 13_roadmap.md, SPEC-P2-00, 14_ai_dev_instructions.md (14.4/14.5), 12_technical_design.md, HANDOFF.md "LAATSTE STAND".*

**Status:** Phase 2 — Vertical Slice "Thirteen Bullets" is per expliciete owner-instructie (2026-07-23) de **actieve milestone**. De 13.2-gatevraag van Phase 1 ("speelt een tester vrijwillig een tweede loop?") blijft open als **staande owner-actie** (§4, T-1) — het antwoord kan de fidelity-investering nog bijsturen, maar blokkeert de Phase-2-systemenbouw niet.

**Leesregels:** bouwvolgorde per spec volgt altijd 14.5 (dataschema → pure-logic core + unit tests → subsystem-wrapper + events → debug-UI → echte UI/content laatst). Elke taak eindigt build-groen. Bouwer: **main** = hoofdagent (architectuur/systemen/specs, seams per 14.6), **eb** = element-builder(s), parallel spawnbaar, één element per agent.

---

## 1. Genummerde backlog — NU tot release

### Fase 2 — VERTICAL SLICE "Thirteen Bullets" (actief; volgorde = SPEC-P2-00 §Spec set)

| # | Item | Doel (één zin) | GDD-ref | Afhankelijk van | Bouwer |
|---|---|---|---|---|---|
| 1 | Milestone-administratie | ACTIVE_MILESTONE-blok → Phase 2 flippen en HANDOFF §4/§5 + `progress_data.js`-takenlijst op de Phase-2-backlog zetten, met de 13.2-playtest expliciet als open owner-actie. | 13 (ACTIVE_MILESTONE), dashboard-workflow | owner-instructie 2026-07-23 (binnen) | main |
| 2 | SPEC-P2-01 Squad of 4 & Classes — **GELAND 2026-07-24** | Geland in `1a85b78` (review GO, 0 blockers): squad van 4, Assault/Medic/Sniper als púre data (`DT_ClassDefs`), save v2→v3 + fixtures per 14.3.6, derde validator; 38/38 ✓, validatie 0 ✓, catalog 21/21 ✓. Follow-ups m4/m6/m7 → §2-S4. | 4.2.2/4.2.3, 8.3 | — | main |
| 3 | Body-swap echte meshes — **materieel vervuld** | Vervuld via de DT_BodyDefs-body-pipeline (sessie 23-07): speler/squad/vijanden dragen echte meshes (Belica, RAISOR-soldiers), 5 Dominion-archetypes; Quaternius blijft reserve/bewoners. Rest-QC lift mee in de 15.8-rondes. | 12.3 (character), 15.5 | — | eb |
| 4 | SPEC-P2-02 Command Mode final feel | Command Mode van debug-orderpicker naar shipping feel: hold-to-enter, 30% dilatatie, 8.4-ordertabel, verbale refusals ("orders zijn beloftes"). *Spec gebankt `29cd549`; **Stage A in uitvoering** (§2-S1); Stage B pas ná het R3-verdict.* | 8.4, 4.1.2, 9.5 | #2 ✓ | main |
| 5 | SPEC-P2-03 Hollow Point walkable base | Menu-basis vervangen door de beloopbare Act-1-vault met 4 faciliteiten op de strategische klok en zichtbare groei (besluit: Intelligence Center vóór Medbay). *Spec gebankt `7194cf4`; **stap 1–2 in uitvoering** (§2-S2).* | 5.1–5.4, 12.3 | P1-08 ✓, P1-03 ✓ | main (systeem) + eb (vault-shells) |
| 6 | SPEC-P2-04 Missies M1.1–M1.4 | De vier openingsmissies autoren op de gedeelde quest-runtime met de volle 11.1-loop en stealth-viable routes (prologue-besluit = open vraag 5). | 2.9, 11.1, 11.4, 12.3 | #2, #4, #5, P1-05 | main (runtime/graphs) + eb (per missie-site) |
| 7 | SPEC-P2-05 Liberation-instance | Eén campagne-missietemplate instantiëren op Kessara's districtsgraaf zodat M1.3 zichtbaar regiostaat flipt en de strategische laag antwoordt (schaal = open vraag 4, voorstel Foothold/3 regio's). | 11.2, 11.3-inputs | #6, P1-04 | main |
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

## 2. Sprintbord — lopend + eerstvolgend (bijgewerkt 2026-07-24 ~16:40, na de P2-01-landing)

DoD-basis (14.4): *spec gerefereerd + code + data + tests + EventCatalog/docs bijgewerkt + CI/lokale groene bar (build -NoUba ✓, tests ✓, validatie ✓, catalog ✓) + door een mens/reviewende agent gezien.*

**T-ledger (afgerond):** T1 ✓ milestone-flip (ACTIVE_MILESTONE = Phase 2) · T2 ✓ SPEC-P2-01 geschreven + gereviewd · **T3+T4 ✓ — P2-01 GELAND** in `1a85b78` (review GO, 0 blockers: squad van 4; Assault/Medic/Sniper als púre data in `DT_ClassDefs`; save **v2→v3** mét migratie-entry + v0- én v2-fixture-tests in hetzélfde commit per 14.3.6; derde `EclipseValidateData`-validator; groene bar: build -NoUba ✓, 38/38 tests ✓, 3 validators/0 fouten ✓, catalog 21/21 ✓) · T5 ✓ SPEC-P2-03 gebankt (`7194cf4`, ná econ-reviewronde) · T8 ✓ SPEC-P2-02 gebankt (`29cd549`).
**T6 vervalt** als aparte taak: SPEC-P2-01 vraagt geen los GAS-ability-systeem — de signature-verbs zijn als data-gedreven orders in de P2-01-landing meegekomen. **T7 is materieel vervuld** door de DT_BodyDefs-body-pipeline (RAISOR/Belica, sessie 23-07); Quaternius blijft reserve.

### Nu lopend — drie parallelle sporen (gestart 2026-07-24 ~16:35; file-ownership botst niet)

**S1 — P2-02 Stage A: Command Mode debug-feel** *(main; backlog #4; dit is de R3-falsificatie)*
Hold-to-enter + 0.30-dilatatie (wrapper-component met fail-safe naar 1.0) + per-soldier-selectie op de bestaande 4 verbs, debug-HUD only — bewust de lelijkste build (SPEC-P2-02 locked decisions 1–3). `DA_CommandModeTuning` (geen hardcoded getallen), ack-meting op wall-clock, `Event.Command.ModeEntered/ModeExited` + catalog in hetzelfde commit. *Ownership:* `Squad/`, `Characters/EclipsePlayerController`, `Core/` tags+payloads, nieuw component.
*DoD:* Gauntlet 20× enter/exit eindigt op dilatatie exact 1.0; scenario "order under fire at 0.30" ack ≤1 s; groene bar; review vóór commit.

**S2 — P2-03 stap 1–2: DT_Facilities + pure EclipseBaseLogic** *(eb; backlog #5, 14.5-stappen 1–2)*
`FEclipseFacilityRow`/`DT_Facilities`, `UEclipseBaseLayoutAsset` (slot-graph A–D), `DA_BaseTuning`, `FEclipseBaseState` in `FCampaignState` **mét v3→v4-migratie-entry + fixture-test in dezelfde changeset** (14.3.6, R6-discipline), pure `EclipseBaseLogic` (build-validatie, day-tick, crew −1, rush) met deterministische unit tests, zonder engine-actor-headers (14.3.2). *Ownership:* `Base/` + `Strategy/` + `Tests/EclipseCampaignTests.cpp` + save-plugin-header.
*DoD:* nieuwe specs groen naast de 38; wrapper/UI/vault niet aangeraakt; asset-materialisatie (setup-script) schuift naar de commandlet-rij.

**S3 — Pack-slim-ronde** *(eb; bezit de commandlet-slot)*
Álle curatie-accepts migreren naar `/Game/Art/Imported` — ook de accepted-unplaced (A1-plinth, A3-slagkei, C1-brokstuk, SciFi10-albedo's 5/6/7/9/10) — builder-paden omzetten, dan Minions+SciFi10 van schijf (~5,9 GB vrij).
*DoD:* 0 verwijzingen naar de gedropte packs, shotronde zonder regressie, groene bar.

### Daarna klaarstaand (in volgorde)

**S4 — Stage A landen + feel-gauntlet in de owner-rij + m4/m6/m7-mini-changeset** *(main, direct na S1 code-complete)*
Reviewronde + groene bar + commit/push; de **feel-gauntlet** (~20 min, het R3-verdict; telemetry via de `ModeExited`-payload HeldSeconds/OrdersIssuedWhileHeld) als owner-touchpoint in `ownerActies` + §4 T-11 zetten. Overbruggingswerk terwijl het verdict openstaat (nooit wachten): de P2-01-review-follow-ups als één kleine, gedragsneutrale changeset — **m4** spawn-fan-offsets (150/130, `EclipseGameMode.cpp:409`) → `UEclipseSquadTuningAsset`; **m6** cover-scorerconstanten (`EclipseSquadOrderLogic.cpp:772`) → tuning; **m7** EventCatalog-formulering SoldierDowned-consumer ("Medic auto-triage" loopt via de OnDowned-delegate, niet via bus-subscriptie).
*DoD:* gedrag ongewijzigd (tests groen zonder aanpassing), catalogtekst klopt met de code, geen tunables meer hardcoded.

**S5 — P2-03 stap 3: subsystem-wrapper + events** *(main, zodra S2 gecommit is)*
`Event.Base.*`-events + catalog in hetzelfde commit, wrapper op de pure core, debug-hub-bediening; vault-shells (eb) pas daarna.

**S6 — Commandlet-rij ná S3 (één tegelijk):** (a) **MetaHuman-curatie** — inventaris `/Game/MetaHumans` (SentinelC + Common binnen, 908 MB), koppeling MH → `DT_NamedCharacters`-slots (Kaya/Brick/Vale/Dex/Petra/Kaine) + toon-restyle-check per 15.7; wacht mede op de owner-download-kliks (§4 T-8); (b) **audio-import 16.12** — de 8 gebankte assets via `Tools/import_generated_audio.py` naar `/Game/Audio`, Kessara-Layer-1-bed (never-silent floor) + sting op de mission-complete-beat; (c) daarna de **15.8-dressing-ronde** (rubble/warehouse + watch-items) op de gemigreerde bronnen — zelfde builder-lock.

**S7 — P2-02 Stage B** *(main, pas ná R3-verdict "true")*
Volledige 8.4-ordertabel (5 nieuwe verbs op het bestaande order-contract), 3 nieuwe refusal-reasons, stealth-stance; bij verdict "false": eerst spec-amendment (fallbacks per SPEC-P2-02) vóór enige polish. Parallel klaar te zetten zodra main vrij is: **SPEC-P2-04 schrijven** (M1.1–M1.4; open vraag 5 beslissen = R8; M1.1-skelet-Gauntlet = R7-falsificatie).

---

## 3. Risicolijst — met vroegst mogelijke falsificatietest

| R | Risico | Vroegst mogelijke falsificatie |
|---|---|---|
| R1 | **Phase-1-loop is niet leuk** (13.2-gate nooit beantwoord) → alle slice-investering staat op een onbewezen loop. | **Vandaag, nul bouwwerk:** owner speelt de loop in PIE (controller + barks) en beantwoordt "tweede loop: ja/nee?". Bij "nee": eerst loop-pivot, fidelity-werk (#10) bevriezen. |
| R2 | **Squad-AI schaalt niet naar 4+klassen** — orders voelen niet langer "obeyed" (existentieel risico 2, 13.1). | Direct na T4: scenario-suite op 4 + PIE-ordertest op graybox, vóór er ook maar één klasse-ability of UI bestaat. Stille orderfout = stop. **Status 2026-07-24: falsificatie doorstaan** — scenario-suite op 4 mét klassen groen in de P2-01-landing (`1a85b78`, 38/38; medic-her-dispatch als review-fix erin). Bewaking blijft per merge via de suite. |
| R3 | **Command Mode-feel (30% dilatatie) overleeft echte gevechten niet** (leesbaarheid/chaos). | 14.5-stap-4-debug-versie van P2-02 in het bestaande graybox-district testen vóór UI/camera-polish — feel-verdict op de lelijkste versie. **Status: ACTIEF** — Stage A (§2-S1) is de falsificatiebuild; het verdict valt in de owner-feel-gauntlet (§4 T-11) op de meetcriteria uit SPEC-P2-02. |
| R4 | **15.12 "AAA-ready" onhaalbaar binnen 12.4-budgetten** op software-Lumen/1080 Ti. | Eén straatblok (niet het hele district) naar volle 15.5-revisie dressen en profileren tegen 12.4 op dev-preview-scalability — vóór de kit-pass districtbreed gaat. |
| R5 | **Fab-afhankelijkheid**: owner-kliks blijven uit → kit-pass (#10) stagneert. | Timebox: staan de shortlist-packs er over 7 dagen niet in, dan Quaternius/CC0+Blender-route als plan-of-record (al bewezen pipeline) en Fab als upgrade-pad. **Status: deels ontladen** — character-packs zijn binnen (11 Fab-packs, 23-07); de env-pack-kliks (§4 T-2) blijven de resterende afhankelijkheid voor de kit-pass. |
| R6 | **Save v1-migratie breekt v0-campagnes** (project-killer per 12.3). | Eerste schema-break in Phase 2: migratie-entry + v0-fixture-test in hetzélfde commit (14.3.6), per merge draaiend — niet wachten tot spec 06. **Status 2026-07-24: falsificatie bewezen werkend** — de eerste echte schema-break (v2→v3, roster-ClassId) landde mét migratie-entry + v0- én v2-fixture-tests in hetzélfde commit (`1a85b78`); pre-v3 saves landen deterministisch op classless. De discipline herhaalt per break; eerstvolgende toets: P2-03 v3→v4 (§2-S2). |
| R7 | **Quest-runtime draagt geen authored missies** (StateTree-fases/consequence-commits te licht voor M1.x). | M1.1-skelet als Gauntlet spawn→complete-by-script→CampaignState-asserts bouwen vóór M1.2–M1.4 geautoreerd worden. |
| R8 | **Prologue-scope-explosie** (open vraag 5 maakt SPEC-P2-04 2 u groter). | Besluit bij spec-schrijven: slice opent bij M1.1 met briefing-recap; prologue alleen als de gate-review erom vraagt. Falsificatie = cold-reader begrijpt de recap zonder prologue. |
| R9 | **16.7-muziek blokkeert op ElevenLabs-scopes** (Music/SFX-endpoints dicht). | Eén live endpoint-call zodra owner scopes zet; tot dan verticale lagen bouwen op CC0-stems — systeem falsifieert zichzelf zonder API. |
| R10 | **12.4-budgetten stilletjes overschreden** naarmate Nanite-density groeit. | Per-merge budgetcheck op Underworks-referentiescène vanaf de éérste kit-pass-merge (niet pas bij spec 08-afronding); Insights-trace bij elke perf-verdachte PR (14.6 skill 7). |
| R11 | **Externe gate-reviewers niet geregeld** → Phase-2-gate wordt zelfbeoordeling. | Owner werft reviewers zodra M1.1+M1.2 speelbaar zijn (halverwege), niet aan het eind — eerste koude sessie is meteen een halfweg-falsificatie. |

---

## 4. Menselijke-touchpoints-wachtrij (owner-acties, gequeued — nooit blind omheen plannen)

*(Gesynchroniseerd met `ownerActies`/`jijGedaan` in `progress_data.js`, 2026-07-24.)*

| T | Actie | Waarvoor / blokkeert | Wanneer |
|---|---|---|---|
| T-1 | **Playtest 13.2**: loop in PIE spelen (Xbox-controller, squad-barks) en de gate-vraag beantwoorden. | Eerlijk Phase-1-verdict; stuurt R1 en de fidelity-prioriteit. | **Nu — staande actie**, ~30 min |
| T-2 | **Fab-kliks (rest)**: login ✓ en de character-packs zijn binnen; resterend zijn de env-pack-pulls (Factory Pack Vol.1, Industrial Building 49 PBR, UNIBLOCKS, Sci-Fi Hallway, Sci-Fi Light Pack, Auto Footsteps Utility, Niagara Footstep VFX, FPS Weapon Bundle, Free Muzzle Flash) + **1 gestileerd civilian/worker-pack** (4–6 bodies, Mannequin-rig — het enige character-gat: Hollow Point-crew/idlers en Kessara-burgers close-up). | Kit-pass backlog #10 + SPEC-P2-03-bevolking; monitor vuurt daarna autonoom. | Gauw, ~10 min (R5-timebox loopt op de env-packs) |
| T-3 | ~~ElevenLabs-scopes~~ — **✓ gedaan 2026-07-23** (Music/SFX aangezet, zie `jijGedaan`). | — | Afgerond |
| T-4 | ~~Blender-install~~ — **✓ gedaan 2026-07-23** (zie `jijGedaan`). | — | Afgerond |
| T-5 | **Mixamo-kliklijst** afwerken zodra de animatie-pass start (Quaternius-CC0 is het autonome alternatief). | Animatiekwaliteit body-pass (backlog #3-QC). | Bij animatie-pass |
| T-6 | **CI self-hosted runner** goedkeuren/aanwijzen (Phase-0-carryover). | 14.4-lagen per merge i.p.v. lokaal (backlog #12). | Deze fase, niet-blokkerend |
| T-7 | **Externe reviewers werven** voor de Phase-2-gate (koud, geen teamleden). | Backlog #13 + R11-halfwegtest. | Zodra M1.1+M1.2 speelbaar |
| T-8 | **MetaHuman-download-kliks**: basis is binnen (SentinelC + Common, 908 MB); resterend 1× Download-klik per overig MH-item in Window → Fab → My Library + sein "MetaHumans staan erin". De Creator-sessie voor hero-companions blijft de Phase 2→3-evaluatie (backlog #21). | MetaHuman-curatie §2-S6a (named-character-slots). | Gauw, ~5 min |
| T-9 | Verder weg: VO-opnamecontracten, lokalisatievendors, closed-beta-programma (Phase 5); storefront/gold-beslissingen (Phase 6). | Backlog #29–#32. | Phase 5+ |
| T-10 | **Overbodige downloads annuleren** (Launcher → Downloads): alle Paragon-helden (Kwang, Narbash, Wukong, Riktor, Sevarog…), Old West VOL 4/6, Path of Adventure JRPG-muziek, NWIRO AI Pro; láát staan: Photogrammetry Snow, Plant Models Vol 60, Perfect Fire VFX. | Bespaart ~30 GB schijf. | Nu, ~2 min |
| T-11 | **P2-02 Stage A feel-gauntlet** (~20 min): de lelijkste Stage-A-build spelen en het R3-verdict geven per de meetcriteria in SPEC-P2-02 (telemetry HeldSeconds/OrdersIssuedWhileHeld leest de agent uit). | Beslist Stage B vs. spec-amendment (§2-S7). | Zodra Stage A speelbaar — komt dan in `ownerActies` |

*Consent-protocol blijft gelden: installs/downloads/security-prompts alleen na uitleg + expliciet akkoord; bouwen met `-NoUba`; PROGRESS.html nooit direct bewerken.*
