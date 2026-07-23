# ECLIPSE — EXECUTION PLAN (NU → RELEASE)
*Werkdocument game-planner | aangemaakt 2026-07-23 | risk-first per 13.1*
*Bronnen: 13_roadmap.md, SPEC-P2-00, 14_ai_dev_instructions.md (14.4/14.5), 12_technical_design.md, HANDOFF.md "LAATSTE STAND".*

**Status:** Phase 2 — Vertical Slice "Thirteen Bullets" is per expliciete owner-instructie (2026-07-23) de **actieve milestone**. De 13.2-gatevraag van Phase 1 ("speelt een tester vrijwillig een tweede loop?") blijft open als **staande owner-actie** (§4, T-1) — het antwoord kan de fidelity-investering nog bijsturen, maar blokkeert de Phase-2-systemenbouw niet.

**Leesregels:** bouwvolgorde per spec volgt altijd 14.5 (dataschema → pure-logic core + unit tests → subsystem-wrapper + events → debug-UI → echte UI/content laatst). Elke taak eindigt build-groen. Bouwer: **main** = hoofdagent (architectuur/systemen/specs, seams per 14.6), **eb** = element-builder(s), parallel spawnbaar, één element per agent.

---

## 1. Genummerde backlog — NU tot release

### Fase 2 — VERTICAL SLICE "Thirteen Bullets" (actief; volgorde = SPEC-P2-00 §Spec set)

| # | Item | Doel (één zin) | GDD-ref | Afhankelijk van | Bouwer |
|---|---|---|---|---|---|
| 1 | Milestone-administratie | ACTIVE_MILESTONE-blok → Phase 2 flippen en HANDOFF §4/§5 + `progress_data.js`-takenlijst op de Phase-2-backlog zetten, met de 13.2-playtest expliciet als open owner-actie. | 13 (ACTIVE_MILESTONE), dashboard-workflow | owner-instructie 2026-07-23 (binnen) | main |
| 2 | SPEC-P2-01 Squad of 4 & Classes | Squad naar 4 met de eerste 3 klassen (voorstel Assault/Medic/Scout, open vraag 1+2 in de spec beslissen) zodat een squad leest als *mensen met een vak*. | 4.2.2/4.2.3, 8.3 | P1-06, P1-07 | main |
| 3 | Body-swap Quaternius-meshes | Squad/vijand-lichamen in `AEclipseCharacter` van kapsel naar de binnengehaalde Quaternius-meshes met animaties, mét squad-scenario-suite groen. | 12.3 (character), 15.5 | assets binnen (klaar); parallel aan #2 | eb |
| 4 | SPEC-P2-02 Command Mode final feel | Command Mode van debug-orderpicker naar shipping feel: hold-to-enter, 30% dilatatie, 8.4-ordertabel, verbale refusals ("orders zijn beloftes"). | 8.4, 4.1.2, 9.5 | #2 | main |
| 5 | SPEC-P2-03 Hollow Point walkable base | Menu-basis vervangen door de beloopbare Act-1-vault met 4 faciliteiten op de strategische klok en zichtbare groei (facility-keuze = open vraag 3, econ-check). | 5.1–5.4, 12.3 | P1-08, P1-03 | main (systeem) + eb (vault-shells) |
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

## 2. Eerstvolgende 5 sprinttaken (elk ≤ 1 dag)

DoD-basis (14.4): *spec gerefereerd + code + data + tests + EventCatalog/docs bijgewerkt + CI/lokale groene bar (build -NoUba ✓, tests ✓, validatie ✓, catalog ✓) + door een mens/reviewende agent gezien.*

**T1 — Milestone-flip + planning-administratie** *(backlog #1, main, ~0,5 dag)*
ACTIVE_MILESTONE-blok in 13_roadmap.md → Phase 2 (met owner-datum en de 13.2-playtest als expliciet open gebleven Phase-1-actie); HANDOFF §4/§5 bijgewerkt; `taken` in `progress_data.js` op de Phase-2-backlog (alleen dat bestand, nooit progress_auto.js).
*DoD:* docs consistent (roadmap↔HANDOFF↔dashboard), geen code geraakt, groene bar onaangetast, owner ziet de nieuwe takenlijst op PROGRESS.html.

**T2 — SPEC-P2-01 schrijven (Squad of 4 & Classes)** *(backlog #2 stap 0, main, ~1 dag)*
One-page-plus spec per 14.3.4: klassekeuze vastleggen (voorstel Assault/Medic/Scout) + besluit "pre-classed recruits, Academy Phase 3" (open vragen 1+2), events consumed/emitted vóór code, datamodel-schets (`UEclipseClassData`), scenario-suite-uitbreiding benoemd.
*DoD:* spec in `phase0/specs/`, events-tabel compleet incl. nieuwe `Event.Squad.SoldierRevived`/`ClassAbilityUsed`, open vragen 1+2 beantwoord met één-zin-rationale, reviewende agent akkoord.

**T3 — P2-01 dataschema + pure-logic core** *(backlog #2 stappen 1–2 van 14.5, main, ~1 dag)*
Class-DataAssets/DataTable (kits, orders, ability-kosten — geen hardcoded constants) + headless kit/order-resolutielogica met deterministische unit tests; compileert zonder engine-actor-headers (14.3.2).
*DoD:* nieuwe Automation-specs groen naast de bestaande 31, `EclipseValidateData` 0 fouten op de nieuwe assets, geen wrapper/UI aangeraakt.

**T4 — Squad live naar 4 + scenario-suite op 4** *(backlog #2 stap 3, main, ~1 dag)*
Subsystem-wrapper + events (bestaande order-events op 4-sterkte; nieuwe tags in EventCatalog zelfde commit), vierde recruit in roster/prep-flow, squad-AI-scenario-suite uitgebreid (cover onder vuur, refusal-messaging op 4) — stille orderfout = blocker.
*DoD:* catalog-check groen (tags in sync), scenario-suite groen op 4, PIE-rooktest: 4 squadleden volgen orders of weigeren hoorbaar/leesbaar.

**T5 — SPEC-P2-03 schrijven (Hollow Point) mét facility-econ-check** *(backlog #5 stap 0, main of tweede agent parallel aan T3/T4, ~1 dag)*
Spec voor de walkable vault: slot-graph, strategische-klok-timers, zichtbare groei; open vraag 3 beslissen (Medbay vs. Intelligence Center) via een doorgerekende economie-loop op de P1-03-ledger; events vóór code.
*DoD:* spec in `phase0/specs/` incl. econ-tabel (kosten/dagen/yield per facility-keuze met 11.1-prep-effect), besluit gedocumenteerd, review akkoord.
*Status (2026-07-23):* spec geschreven — `phase0/specs/SPEC-P2-03_hollow_point_base.md` (besluit: Intelligence Center i.p.v. Medbay; Act-1-kostentabel ×0,8; missie-debrief = +1 dag). Wacht op review + commit.

*Daarna klaarstaand:* T6 = P2-01 klasse-abilities via GAS + debug-UI (14.5 stap 4); T7 = body-swap Quaternius (backlog #3, eb, parallel); T8 = SPEC-P2-02 schrijven.
*Status T8/backlog #4 (2026-07-23):* spec geschreven — `phase0/specs/SPEC-P2-02_command_mode.md` (falsificatie-eerst: Stage-A-debug-feel-versie + R3-feel-gauntlet mét meetbare verdict-criteria vóór polish; besluiten: order-antwoorden op real-time klok onder dilatatie, per-soldier targeting, 8.4-tabel gemapt op bestaande verbs + 5 nieuwe, 3 nieuwe refusal-reasons, dilatatie als wrapper-laag). Wacht op review + commit.

---

## 3. Risicolijst — met vroegst mogelijke falsificatietest

| R | Risico | Vroegst mogelijke falsificatie |
|---|---|---|
| R1 | **Phase-1-loop is niet leuk** (13.2-gate nooit beantwoord) → alle slice-investering staat op een onbewezen loop. | **Vandaag, nul bouwwerk:** owner speelt de loop in PIE (controller + barks) en beantwoordt "tweede loop: ja/nee?". Bij "nee": eerst loop-pivot, fidelity-werk (#10) bevriezen. |
| R2 | **Squad-AI schaalt niet naar 4+klassen** — orders voelen niet langer "obeyed" (existentieel risico 2, 13.1). | Direct na T4: scenario-suite op 4 + PIE-ordertest op graybox, vóór er ook maar één klasse-ability of UI bestaat. Stille orderfout = stop. |
| R3 | **Command Mode-feel (30% dilatatie) overleeft echte gevechten niet** (leesbaarheid/chaos). | 14.5-stap-4-debug-versie van P2-02 in het bestaande graybox-district testen vóór UI/camera-polish — feel-verdict op de lelijkste versie. |
| R4 | **15.12 "AAA-ready" onhaalbaar binnen 12.4-budgetten** op software-Lumen/1080 Ti. | Eén straatblok (niet het hele district) naar volle 15.5-revisie dressen en profileren tegen 12.4 op dev-preview-scalability — vóór de kit-pass districtbreed gaat. |
| R5 | **Fab-afhankelijkheid**: owner-kliks blijven uit → kit-pass (#10) stagneert. | Timebox: staan de shortlist-packs er over 7 dagen niet in, dan Quaternius/CC0+Blender-route als plan-of-record (al bewezen pipeline) en Fab als upgrade-pad. |
| R6 | **Save v1-migratie breekt v0-campagnes** (project-killer per 12.3). | Eerste schema-break in Phase 2: migratie-entry + v0-fixture-test in hetzélfde commit (14.3.6), per merge draaiend — niet wachten tot spec 06. |
| R7 | **Quest-runtime draagt geen authored missies** (StateTree-fases/consequence-commits te licht voor M1.x). | M1.1-skelet als Gauntlet spawn→complete-by-script→CampaignState-asserts bouwen vóór M1.2–M1.4 geautoreerd worden. |
| R8 | **Prologue-scope-explosie** (open vraag 5 maakt SPEC-P2-04 2 u groter). | Besluit bij spec-schrijven: slice opent bij M1.1 met briefing-recap; prologue alleen als de gate-review erom vraagt. Falsificatie = cold-reader begrijpt de recap zonder prologue. |
| R9 | **16.7-muziek blokkeert op ElevenLabs-scopes** (Music/SFX-endpoints dicht). | Eén live endpoint-call zodra owner scopes zet; tot dan verticale lagen bouwen op CC0-stems — systeem falsifieert zichzelf zonder API. |
| R10 | **12.4-budgetten stilletjes overschreden** naarmate Nanite-density groeit. | Per-merge budgetcheck op Underworks-referentiescène vanaf de éérste kit-pass-merge (niet pas bij spec 08-afronding); Insights-trace bij elke perf-verdachte PR (14.6 skill 7). |
| R11 | **Externe gate-reviewers niet geregeld** → Phase-2-gate wordt zelfbeoordeling. | Owner werft reviewers zodra M1.1+M1.2 speelbaar zijn (halverwege), niet aan het eind — eerste koude sessie is meteen een halfweg-falsificatie. |

---

## 4. Menselijke-touchpoints-wachtrij (owner-acties, gequeued — nooit blind omheen plannen)

| T | Actie | Waarvoor / blokkeert | Wanneer |
|---|---|---|---|
| T-1 | **Playtest 13.2**: loop in PIE spelen (Xbox-controller, squad-barks) en de gate-vraag beantwoorden. | Eerlijk Phase-1-verdict; stuurt R1 en de fidelity-prioriteit. | **Nu — staande actie**, ~30 min |
| T-2 | **Fab-kliks**: shortlist "Add to library" + "Add to Project" (Industry Props Pack, modulaire industriële kit, Lt. Belica, Quixel-decals) + "packs staan erin"-sein; Launcher/fab.com-login is al gedaan. | Kit-pass backlog #10; monitor vuurt daarna autonoom. | Nu, ~10 min (R5-timebox loopt) |
| T-3 | **ElevenLabs-scopes**: Music/SFX aanzetten op de vault-key; bevestigen of de geroteerde key actief is. | Live-runs 16.7 (backlog #11); TTS werkt al. | Vóór spec 09-implementatie |
| T-4 | **Blender-install** (UAC-prompt staat in de wachtrij). | Zelfgemaakte props via `Eclipse/Tools/blender/` (eb-route, R5-fallback). | Bij eerste eb-prop-taak |
| T-5 | **Mixamo-kliklijst** afwerken zodra de animatie-pass start (Quaternius-CC0 is het autonome alternatief). | Animatiekwaliteit body-swap (backlog #3). | Bij animatie-pass |
| T-6 | **CI self-hosted runner** goedkeuren/aanwijzen (Phase-0-carryover). | 14.4-lagen per merge i.p.v. lokaal (backlog #12). | Deze fase, niet-blokkerend |
| T-7 | **Externe reviewers werven** voor de Phase-2-gate (koud, geen teamleden). | Backlog #13 + R11-halfwegtest. | Zodra M1.1+M1.2 speelbaar |
| T-8 | **MetaHuman-gezichten**: Creator-sessie + downloads voor hero-companions (Mara, Brick) bij de Phase 2→3-evaluatie. | Backlog #21. | Gate #13 → Phase 3-start |
| T-9 | Verder weg: VO-opnamecontracten, lokalisatievendors, closed-beta-programma (Phase 5); storefront/gold-beslissingen (Phase 6). | Backlog #29–#32. | Phase 5+ |

*Consent-protocol blijft gelden: installs/downloads/security-prompts alleen na uitleg + expliciet akkoord; bouwen met `-NoUba`; PROGRESS.html nooit direct bewerken.*
