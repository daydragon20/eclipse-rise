# ECLIPSE — PROJECT HANDOFF & PROGRESS
*Single "start here" page for a new machine or a new Claude session. Last updated: 2026-07-22 (first strong-PC session).*

> **Live progress dashboard:** open [`PROGRESS.html`](PROGRESS.html) in a browser and leave it open — it reloads itself every 60 s. Facts (commits, tests, newest screenshots) refresh automatically every 10 min via `start_progress_watcher.bat` (→ `Tools/update_progress.ps1`). Dev sessions update the judgment percentages by editing **`progress_data.js`** at milestones — never edit PROGRESS.html itself, and never hand-edit `progress_auto.js`. Owner instructions for the audio pipeline + studio working method are recorded verbatim in [`phase0/OWNER_MANDATE.md`](phase0/OWNER_MANDATE.md).

> **Nathan's simpele pagina = [`START_HIER.html`](START_HIER.html)** (kindertaal, leest live uit `progress_data.js`). Dit is de pagina die de owner opent — PROGRESS.html is de detailversie. **Elke dev-iteratie MOET `progress_data.js` bijwerken**: het veld `bijgewerkt` (tijd), de `taken`-statussen, `ownerActies` (weghalen wat af is), en een geverifieerde owner-actie verhuizen naar `jijGedaan` met tijd. Bewerk NOOIT START_HIER.html direct. Verse-chat startprompt staat in [`NIEUWE_CHAT_PROMPT.txt`](NIEUWE_CHAT_PROMPT.txt). Géén `/loop`/ScheduleWakeup gebruiken (vuurt onbetrouwbaar).

> **Read order for whoever picks this up:** this file → `DOCUMENTATION_README.md` → `00_INDEX.md` → `13_roadmap.md` (ACTIVE_MILESTONE) → `14_ai_dev_instructions.md`. Then the phase specs in `phase0/specs/`.

---

## 1. What this project is

**ECLIPSE: Rise of the Resistance** — a single-player third-person Action-Strategy RPG built in **Unreal Engine 5.8 (C++)**. Full design lives in the bible (docs `00`–`15`). It is a ~4.5-year, 6-phase game (roadmap `13`). We are in **Phase 1 — Prototype "The Loop"** (deliberately ugly graybox; prove the risks, not the graphics).

This is **not** a web app and does **not** deploy to Vercel — it is a native UE game that runs in the Unreal Editor / a packaged build.

---

## 2. Where everything lives (one repo)

As of the consolidation commit, the **design bible and the UE game are in ONE git repo** rooted at `C:\Dev\ECLIPSE_GDD\`, so a single `git clone` gets everything (docs + code + full history).

| What | Path (relative to repo root) |
|---|---|
| **Design bible** (docs 00–15, `phase0/` specs, this file, SETUP.md) | repo root |
| **The UE game** (code + content) | `Eclipse/` |
| **Save & push safety net** | `push-all.bat` / `push-all.ps1` |

Repo is local-only until pushed (see §7). Save-and-push any time with **`push-all.bat`** (double-click) — commits everything and pushes so the other machine can `git pull`.

**Progress is readable in two places:**
1. `git log` inside `Eclipse\` — each commit is one spec landed (the real build record).
2. `13_roadmap.md` → `ACTIVE_MILESTONE` block — the single source of "where we are."
3. This file's §4 (status) and §5 (what's next).

---

## 3. Hardware note (why a stronger machine matters)

This dev box is a **2017 laptop: NVIDIA GTX 1050 (4 GB), i7-7700HQ, 32 GB RAM**. It is a fine **graybox / logic / CI** box (Phase 1 runs on it), but it is **not** the fidelity target: no hardware ray tracing (no RT cores), Lumen only in reduced software mode, 4 GB VRAM can't stream 4K/8K. The **visual quality work (Phase 2+) belongs on a stronger RTX-class machine** — see the new `15_visual_quality_charter.md` (§15.2) for the dev-box-vs-target split. So handing the high-fidelity phases to a stronger PC is exactly the intended plan.

---

## 4. Current status (2026-07-22)

**Phase 1 — all 8 feature specs landed, independently re-reviewed (Fable 5), and the tree is green.**

- Build: `EclipseEditor Win64 Development` **Succeeded** (UE 5.8).
- Automation tests: **31/31 pass** (`Automation RunTests Eclipse`, headless `-nullrhi`; incl. 3 audio tests).
- Data validation: **clean** (`EclipseValidateData` commandlet, 0 errors).
- Event catalog: **19/19 in sync** (`Eclipse/Tools/check_event_catalog.py`).
- Voice pipeline: **live-verified** against the ElevenLabs PCM endpoint (8/8 lines generated + imported + assigned; re-run = 8 cache hits, 0 API calls).

**Systems in place (headless-proven):** event bus · campaign state + transaction API + save v0 · deterministic economy ledger · 6-node strategy mini-map · mission runtime + debrief consequences · squad of 2 with ordered actions and *reasoned refusals* (never-silent) · roster + permadeath + memorial stub · menu base hub + preparation flow · playable graybox layer (character/GAS health, hitscan combat, enemy AI, code-built district).

The full loop is **proven headless** (test `Eclipse.Base.Prep.FullCircleSmoke`: advance day → select → intel spend → launch → win → consequences → second loop) and is now **wired live** — playable in PIE with no console commands (menu base → launch → graybox mission with squad orders → extraction → debrief → base).

---

## 5. What's next

The **live playable loop is done** (commit `[Loop] Wire the live playable loop`): boot → menu base → launch → graybox mission with squad orders → extraction/lose → debrief → base, no console commands. The remaining Phase-1 step is the **gate question** — a human plays it and answers *"do testers voluntarily play a second loop?"* (13.2). That's the owner's call, not an automated check.

**Done since 2026-07-21 (Fable 5 independent re-review, all green + pushed):**
- Stance stub + `DA_SquadTuning` wiring landed (previous session's last commit).
- Re-review fixes: `MarkMissionServed` mutation (roster service count was never incremented); squad cover scorer treated the id-less player as a threat + downed-callback lifetime hardened; ground-data wiring (DA_CharacterTuning/DT_Weapons/DT_EnemyArchetypes were dead data, the player pawn had **no weapon** and stayed **downed after a lost run** — all fixed, player revives per launch); prep/launch rejections now visible in the hub.
- First stylized dressing pass on the code-built district (owner-authorized ahead of Phase 2): palette MIDs, SkyAtmosphere, haze, punch grading, authored ink-outline post material (`/Game/Art/PP_EclipseOutline`), and a `-EclipseShot` screenshot review rig (15.9). Dev-box (SM5) lighting calibration in progress — see PROGRESS.html for honest status; the full stack (Lumen/Nanite/VSM) remains RTX-PC work.

**Done later on 2026-07-22 (owner-mandate session, all green + pushed):**
- **Audio pipeline per OWNER_MANDATE** (16.12): TTS now requests raw PCM and stores **WAV** in `Content/Audio/Generated/` (cache travels with the repo); runtime `PlayLine` (asset → cached wav → generate); `FEclipseDialogueLine.GeneratedAudio` + per-character `Lines`; bulk commandlet `-run=EclipseGenerateVoices` (AssetRegistry scan → generate → import USoundWave → auto-assign + save); 2 new headless tests. First live-API run still pending (no key on this box).
- **Live progress dashboard** `PROGRESS.html` + `progress_media/` (per-subtask %, screenshots) — update it every session.

**Done still later on 2026-07-22 (green-bar re-verify + first live voice run, all pushed):**
- Green bar independently re-verified (build ✓, tests ✓, validation ✓, catalog ✓).
- **Dialogue-database seed** (16.12): `Content/Audio/DialogueSeed.json` → the commandlet materialises `UEclipseCharacterVoiceData` assets under `/Game/Audio` (create-only, never overwrites); pure parser `EclipseDialogueSeed` + headless rules test (test #31).
- **First live ElevenLabs run succeeded** using the vault key (`ECLIPSE_SECRETS\UserSecrets.ini`, copied to `Eclipse/Config/` — gitignored): 8/8 squad barks generated (PCM→WAV), imported as `USoundWave`, auto-assigned, saved; the re-run proved 8 cached / 0 generated. Cache + assets committed.
- **Key scope note:** the vault key works for TTS but `/v1/voices` returns **401** — it appears scope-restricted to text-to-speech. Whether this is the already-rotated key is for the owner to confirm; if the plaintext-shared key is still active, rotate it in the ElevenLabs dashboard and update the vault + `Eclipse/Config/UserSecrets.ini`.

**Open (small, non-blocking) polish:**
- First audible PIE check of `PlayLine` (assets exist; nobody has *heard* them in-game yet).
- Music/SFX endpoints + adaptive always-on music (16.7) — Phase 2 forward infra.
- Graphics: the SM5 dev-box milestone is banked (dusk two-tone + ink outlines); the real fidelity pass (Lumen/Nanite/VSM, Part 15 full stack) runs on the RTX PC.

**Non-blocking carryover** (roadmap-allowed to lag): CI self-hosted runner, 10 concept-art pieces, 5 feel-reference clips. **Fidelity/art starts at Phase 2** (graybox rule); Parts 15–17 are the target for that.

---

## 6. How to build, test, and run (any Windows machine with the toolchain)

Prereqs: **UE 5.8** (Epic Launcher) + **Visual Studio 2022** with the "Game development with C++" workload. Set `UE_ROOT` to the engine, e.g. `C:\Program Files\Epic Games\UE_5.8`.

```powershell
# 1. Generate VS project files (right-click Eclipse.uproject → Generate, or:)
& "$env:UE_ROOT\Engine\Build\BatchFiles\Build.bat" EclipseEditor Win64 Development `
  -project="<repo>\Eclipse\Eclipse.uproject" -WaitMutex

# 2. Run the tests (same as CI)
& "$env:UE_ROOT\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "<repo>\Eclipse\Eclipse.uproject" -ExecCmds="Automation RunTests Eclipse; quit" `
  -unattended -nopause -nosplash -nullrhi -log -ReportExportPath="<repo>\Eclipse\Saved\Automation"

# 3. Data validation
& "$env:UE_ROOT\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "<repo>\Eclipse\Eclipse.uproject" -run=EclipseValidateData -unattended -nopause -nosplash

# 4. Play: open Eclipse.uproject in the editor → Play In Editor.
```

CI definition (needs a self-hosted Windows+UE5 runner): `Eclipse\.github\workflows\ci.yml`.

---

## 7. How another (stronger) computer takes over

The clean mechanism is **git**. The bible + code are already **one repo** (§2), local-only until pushed.

**One-time setup — private GitHub repo:**
1. `gh auth login --hostname github.com --git-protocol https --web` → authorize in the browser (kimi can drive it).
2. From the repo root: `gh repo create eclipse-rise --private --source . --remote origin --push` (creates the private repo AND pushes everything in one go).

**From then on — keep it synced (this is the auto-push command):**
- Double-click **`push-all.bat`** (or run `push-all.ps1`). It commits all current work and `git push`es. Use it before switching machines or whenever Claude nears a session limit.

**On the strong PC:** install the toolchain (`SETUP.md`), `git clone https://github.com/<owner>/eclipse-rise.git C:\Dev\ECLIPSE_GDD`, build (§6), read this file + `ACTIVE_MILESTONE`, continue at §5. **Full day-one migration guide — with UML/BPMN diagrams, the Max/Fable login, the secrets vault, and a paste-ready Fable prompt — is in [`MIGRATION_TO_STRONG_PC.md`](MIGRATION_TO_STRONG_PC.md).**

**Alternatives if you don't want GitHub yet:** `git bundle create eclipse.bundle --all` (one file, full history) transferred by USB/drive/server; or push to the existing Rocadelo server the project already came from.

**What the strong PC's Claude should be told:** *"Read `C:\...\ECLIPSE_GDD\HANDOFF.md`, then continue Phase 1 at §5. Stay inside the ACTIVE_MILESTONE. Fidelity work starts at Phase 2 per `15_visual_quality_charter.md`."*

---

## 8. Handy pointers

- Governance / coding rules: `14_ai_dev_instructions.md` (naming `AEclipse*/UEclipse*/FEclipse*/EEclipse*`; commits `[System] Verb summary (GDD ref)`; event-bus-only cross-system; tunables in DataAssets; PLACEHOLDER tags).
- Event registry: `Eclipse\Docs\EventCatalog.md` (keep in sync with code, checked by `Tools/check_event_catalog.py`).
- Save contract: `12_technical_design.md` §12.3.
- Performance budgets: `12_technical_design.md` §12.4.
- Visual target + hardware reality: `15_visual_quality_charter.md`.

---

## LAATSTE STAND (2026-07-25 ~15:30 — CYCLUS 6 GELAND: vier bouwsporen + review-ronde)

**Bar bij de commits: build ✓ (-NoUba) · tests 85/85 (0 fail; 55 schoon + 30 met bekende fixture-warnings) · EclipseValidateData 4 validators/0 fouten · catalog 29/29.** Bar is bewust twee keer gedraaid: de tweede run dekt exact de gecommitte boom (een test-helper-guard landde ná de eerste build, dus die eerste run dekte hem niet).

1. **`e8bd5c8` [Quests]** — M1.1-Gauntlet op de geshipte keten (DA_CampaignSetup → DT_StoryMissions-pin → MT_M11.uasset), win- én verliespad. Rewards gemeten aan de commit-eigen ResourcesChanged-feiten i.p.v. wallet-delta's (de dagtick boekt legitiem extra inkomsten).
2. **`b068212` [Strategy]** — liberation-trigger: één writer, één transactie, state-derived idempotentie + vierde ValidateData-validator + `Tools/setup_liberation_data.py`.
3. **`67f46a6` [Quests]** — Taak-4-kern: Event.Mission.PhaseChanged, alarm als benoemde sub-fase, alarm/casualty-latch, optional-payouts atomair in de debrief-transactie. Catalog 29/29 + `bRequiresNoCasualties` als spec-amendement in SPEC-P2-04 §Data schema.
4. **`c6cf7fe` [Base]** — walkable vault (NIEUW `Base/EclipseVaultBuilder.*`): pure `PlanSlots` + parity-Gauntlet, her-render op de bestaande Event.Base.*-feiten, gecoalesceerd op plan-hash (1 rebuild per commit, geen tick).
5. **`966c800` [Art]** — dressing-iteratie 2: dusk-vloer, lamp-pools + blob-schaduwen als luminantie-decals (district is unlit), volledige nudge-lijst, `generate_decals.py`.

**Reviewronde:** 5 lenzen over de drie complete changesets, elke blocker/major langs een tegenlezer die hem moest wéérleggen. Uitkomst: 5× GO, 13 majors weerlegd, **1 bevestigd** — EB-2's wiring-tests bouwden de twee productiefeiten zélf in hun eigen orde, dus een omgedraaide commit/broadcast in `ResolveDebrief` zou de liberation stil doden terwijl alles groen bleef. Gefixt met `Eclipse.Strategy.Liberation.WiringDebriefSeamFlipsTrio` (draait het échte debrief-pad en pint commit-voor-broadcast + beide tabel-koppelingen) plus twee nieuwe validator-checks. Verder verwerkt: twee majors op de eigen Gauntlet (verlies-test stelde niet vast wélke missie liep; vier ongeguarde region-dereferences) en een liegende logregel bij een lege liberation-resolutie.

**Owner-besluit geland:** 15.7 = **B (hybride MetaHuman-shading)**. Vastgelegd in `phase0/metahuman_recipes.md`; implementatie-spec `phase0/MH_FACE_TIER_B.md` met twee vondsten: de bestaande restyle-lus is slot-blind (vervangt élk slot op index = optie A), en de body-gain 3.2 is bewust ongemeten omdat "bodies klein op het scherm zijn" — bij een dialoog-close-up valt die rechtvaardiging weg.

**Eerstvolgende stappen:** (1) commandlet-slot: `generate_decals.py` → `import_generated_decals.py` → shotronde (eerste PNG = warm-up, overslaan) → art-review op de pool/blob-vraag; (2) `setup_liberation_data.py` NIET vóór M1.3 authored is — de nieuwe validator zou dan correct melden dat de rij nooit kan vuren; (3) wave 2: M1.1 zero-casualty-optional (LET OP: `setup_story_missions.py` schrijft objectives alleen in `if created:`, en MT_M11 bestaat al — er is een eigen migratiestap nodig, staat als comment in het script), recap-materialisatie → cold-reader (harde poort vóór M1.2), civilian-wiring; (4) menu-hub-retirement als eigen mini nu de parity groen is.

## VORIGE STAND (2026-07-25 ~13:15 — CYCLUS 5: de vijf wacht-changesets)

**Trigger:** Nathan sloot de editor om 13:06 (Live-Coding-buildlock weg, memory eclipse-editor-buildlock) — de voorbereide landing rolde daarna in één run. **Unie-bar vóór de commits: build ✓ (-NoUba, 69 s) · tests 68/68 (50 schoon + 18 met bekende fixture-warnings, 0 fail) · EclipseValidateData 3 validators/0 fouten · catalog 28/28.** Alle vijf changesets hadden vóóraf review-GO (2 verse reviews, 1 herkeur, art-review, planner ×2).

1. **`6ac96c1` [Strategy]** — P2-04 stap 2: ResolveOfferForRegion als het ene offer-pad, 5 native Story.Beat.*-tags, per-tabel-validatiepass (luid, 1× per tabel), map her-rendert op BeatReached. (De StaffAssigned-comment-hunk reed bewust hier mee.)
2. **`6b08bb2` [Art]** — 15.8-dressingronde 1 op de Imported-accepts; magenta-container → mix 0.45; iteratie-2-spec staat in `phase0/DRESSING_ITERATIE_2.md` (kerninzicht: district is unlit → licht via luminantie-decals).
3. **`173d4ec` [Quests]** — completion-beat atomair in de debrief-transactie (skip-if-set load-bearing); verlies committeert nooit story-voortgang.
4. **`d503651` [Base]** — P2-03 stap 4-5A: casualty-releases, muster-gate, mutatie-laag-cap + 3 tests; catalog-rij eerlijk.
5. **`9be3786` [Strategy]** — P2-05 pure core: monotone liberation-resolve + beat-gate + 6 tests (veldnamen spec-conform na review-B1).

**Direct hierna (zelfde sessie):** commandlet-rij in de vrije slot — `setup_story_missions.py` (MT_M11 + DT_StoryMissions + DA_CampaignSetup, daarna her-bar + [Quests]-datacommit) → `inventory_metahumans_wave2.py` (report-only MH-probe → aftekening Nathans MetaHuman-taak) → `import_modular_civilians.py` (11 CC0-civilians + SOURCES.md + [Art]-datacommit). **Cyclus N+1 staat gepland in `phase0/CYCLUS_N1_PLAN.md`** (main M1.1-Gauntlet eerst, dan 4 element-builders per fence-matrix; wave 2 = optional-schema, recap-materialisatie + cold-reader, civilian-wiring; M1.2 = N+2, gated op cold-reader 4/4). Taak-4-spec: `phase0/TAAK4_STORY_SURFACE.md`; recap-copy: `phase0/RECAP_CARDS_M1.md`.

## VORIGE STAND (2026-07-24 — P2-01 GELAND)

**Waar gestopt:** Phase 2 "Thirteen Bullets" is de actieve milestone (owner-instructie 23-07; de 13.2-playtest van Phase 1 blijft een staande owner-actie). **SPEC-P2-01 (Squad van 4 + eerste 3 classes: Assault/Medic/Sniper) is geland en gepusht** na een volle groene bar én een onafhankelijke code-review (verdict GO, 0 blockers).

**Deze sessie (samengevat):**
- *Groene bar P2-01:* build ✓ (-NoUba) · **tests 38/38** (6 nieuwe, incl. v0-fixture-migratietest én een echte v2-fixture-test) · **EclipseValidateData 3 validators / 0 fouten** · catalog **21/21**.
- *Checkpoint 14.3.6 (R6) vervuld:* save-schema v2→v3 (roster-ClassId-tail) mét migratie-entry + fixture-tests in hetzélfde commit; pre-v3 saves landen deterministisch op classless (NAME_None).
- *DoD-gat gedicht:* EclipseValidateData dekte DT_ClassDefs niet — nieuwe derde validator toegevoegd (verb-familie, verb↔tunable-consistentie, weapon/body-cross-refs per campaign-setup, negatieve-tunable-vangnet voor script-gevulde tabellen).
- *Review-fixes in dezelfde changeset:* M1 — medic-her-dispatch: na een afgeronde triage-run krijgt een tweede casualty alsnog zijn poging (peek `CanStabilizeSoldier` voorkomt shuttle-loops op geredde/verlopen patiënten); m2 stilte-log; m3 struct-checks; m5 asset-telling. Follow-ups genoteerd: m4 (spawn-fan-offsets → SquadTuning), m6 (cover-scorerconstanten → tuning), m7 (catalog-formulering SoldierDowned-consumer).
- *Hygiëne:* `__pycache__/` ge-gitignored. `Eclipse/Content/MetaHumans|Atira_LODSettings|Locodrome` staan bewust untracked tot de MetaHuman-curatie (aparte taak) besluit wat reist.
- *Sessies 23-07 (niet eerder in dit bestand bijgeschreven):* zie de changelog in `progress_data.js` — o.a. body-pipeline (RAISOR/Belica via DT_BodyDefs), westgevel-B district-breed (412f14f), eerste audio-ronde gebankt (348b7f0), SPEC-P2-02/-P2-03 geschreven + gereviewd, MetaHumans-basis geland (908 MB, curatie in de rij).

**Vervolg dezelfde dag (middag/avond — drie sporen parallel gebouwd):**
- **P2-02 Stage A geland** ([Command]-commit): hold Q/pad-LB → 0.30-dilatatie via pure state-machine (`EclipseCommandLogic`) + wrapper-component met sluitende fail-safe (release/death/mission-end/travel → exact 1.0; tick alleen tijdens de hold), per-soldier-selectie (cycle + reticle-pick, range uit DA_CommandModeTuning) door het bestaande IssueOrder-contract, `Event.Command.ModeEntered/Exited` (catalog 23/23), debug-HUD, `Eclipse.Command.Dump`, 3 headless tests. Review GO; de MAJOR (pick-range was dode data) + alle minors in dezelfde changeset gefixt. **Feel-gauntlet (R3-verdict, ~20 min) staat nu in Nathans kliklijst — draaiboek: `phase0/FEEL_GAUNTLET_P2-02.md`.** Stage B start pas ná verdict "true"; bij "false" geldt de fallback-ladder uit de spec.
- **P2-03 stap 1-2 geland** ([Base]-commit, via element-builder + eigen review): DT_Facilities-schema, slot-graph-asset, DA_BaseTuning, `FEclipseBaseState` mét save v3→v4-migratie + byte-getrouwe v3-fixture in hetzelfde commit (14.3.6/R6 tweede keer bewezen); spec-startstand = type-default (gratis L1 Command Center op Slot_A); hash-dekking uitgebreid. Stap 3 (wrapper + Event.Base.*) = volgende iteratie.
- **Pack-slim-ronde af** ([Art]-commit): alle 11 curatie-accepts naar `/Game/Art/Imported` (repo-tracked, provenance in SOURCES.md), builder-refs via 3 mini-stubs pixel-gelijk gehouden (string-swap = eerste edit 15.8-ronde), Minions+SciFi10 van schijf — **~5,9 GB vrij**, scan-bewijs 0 refs (ASSET_CLEANUP §7-10).
- **m4/m6/m7-follow-ups geland** ([Squad]-commit): spawn-fan + cover-scorer-constanten naar DA_SquadTuning (gedrag-neutraal), catalog-formulering eerlijk gemaakt.
- **SPEC-P2-04 geschreven + main-review verwerkt** (concept ACCEPTED): geen prologue (recap-cards + cold-reader-falsificatie), R7-Gauntlet als bouwstap 1, nul nieuwe objective-primitieven, M1.3 als enige world-state-change via de P2-05-seam, Brick = Assault-rosterrecord; de 4 open punten beslist in de spec zelf.
- **Groene bar (unie, alle sporen samen): 47/47 tests · validatie 3 validators/0 fouten · catalog 23/23.**

**Cyclus 3 (avond — R7 groen, MetaHumans gecureerd, P2-03 stap 3 + P2-04 stap 2):**
- **R7 = GROEN, geland** (`60014e6`): de missie-runtime draagt aantoonbaar authored missies (M1.1-skelet op het echte laadpad; mandatory-set, rewards, regio-onaangetast) — M1.2-M1.4-authoring vrijgegeven. In dezelfde changeset de **debrief-dag-regel** (P2-03 locked decision 4: elke missie kost mechanisch een dag, win óf verlies, mét dag-asserts op beide paden).
- **MetaHuman-curatie geland** (`15dfe61`, review GO): SentinelC (908 MB) verplaatst naar zijn geauthorde root (dode kruisrefs gerepareerd), Kaya provisioneel bedraad via setup_metahuman_data.py, single-writer-fix op setup_character_data.py, .gitignore dekt de machine-lokale character-content, 15.7-beslisdocument (aanbeveling B) bij owner/art-review. Builder-string-swap naar de Imported-paden (`7036849`).
- **P2-03 stap 3 geland** (`69b5f4b`, review GO): transactie-API + Event.Base.* ×4 (uitsluitend commit-emissie) + UEclipseBaseSubsystem-wrapper + alle 5 stap-1-2-bevindingen mét tests + gematerialiseerde Hollow Point-data; zes stap-4-5-follow-ups verankerd in de taak. **P2-04 stap 2 Quests-deel geland** (`16ce2bd`, review GO): DT_StoryMissions-schema + pure story-sequencing. **Audio 16.12 geïmporteerd** (`937f35f`): bed/cues/sting als engine-assets; bed-plaatsing + sting-koppeling lopen. **Eindbar unie: 54/54 tests · validatie 0 · catalog 27/27.**
- **SPEC-P2-05 geschreven + geaccepteerd** (Foothold-trio; geen nieuwe events — LiberationPhaseAdvanced gedropt als P2-00-amendement; bouw ná StoryFlags v5).

**Cyclus 4 (avond — StoryFlags v5 + audio-runtime):** [Strategy] `fd38933` = StoryFlags geland als DERDE bewezen R6-migratie (v4→v5, byte-getrouwe fixtures, SetStoryFlag-mutatie met atomaire duplicate-reject, Event.Story.BeatReached, catalog 28/28); [Audio] `6b48ba4` = never-silent Kessara-bed (2D + attenuation-override + anti-stapel-cleanup) en de verzets-sting op Mission.Completed via UEclipseAudioSubsystem (pure bus-consumer, BusContract-test). Suite 57/57. M1.1-authoring-script (`Tools/setup_story_missions.py`) staat klaar voor de slot.

**Eerstvolgende stappen:** (1) owner: **feel-gauntlet** (R3, bovenaan de actielijst) + MH-kliks (Frey/Hannah/Mason) + 15.7-keuze; (2) code: Strategy-offer-precedence + native Story.Beat.*-tags + M1.1 authored (P2-04 bouwstap 2-afronding; script klaar) → daarna debrief-beat-koppeling en M1.2-authoring; Stage B wacht op het R3-verdict; (3) builder-rondes: 15.8-dressing (cold-DDC-check, stub-verwijdering, shotronde + 15.7-vergelijk) · P2-03 stap 4-5 (walkable vault, 6 verankerde follow-ups).

---

## VORIGE STAND (2026-07-22, ~21:40 CET — EERSTE SESSIE OP DE STERKE PC)

**Waar gestopt:** eerste Fable 5-sessie op de sterke PC (na de installateur-bootstrap). Werkboom schoon, alles gecommit en gepusht; **groene bar hier onafhankelijk herbevestigd: build ✓ (-NoUba), 31/31 tests ✓, validatie 0 fouten ✓, catalog 19/19 ✓.**

**Deze sessie (samengevat):**
- *Machine gemeten:* **GTX 1080 Ti, 11 GB VRAM** — volle SM6/DX12 (Nanite, VSM, TSR, software-Lumen), maar **géén RT-cores**: hardware ray tracing kan hier niet. Charter 15.2 heeft nu een aparte rij (C) voor deze machine; HWRT-validatie blijft RTX-klasse-werk.
- *Owner-revisie §15 verwerkt (bindend):* Borderlands-leunend blijft gelockt, **fidelity erbínnen omhoog** — Nanite-polygoondichtheid, opgeschroefde software-Lumen-GI, rijkere post (SSAO/bloom/film grain), meer particles, TSR hoog. Vastgelegd in 15.5 ("Fidelity revision").
- *Graphics-kalibratie (taak A) — inktlijnen LIVE:* beide materiaal-scripts gedraaid (`M_EclipseToon` banden 0.55/0.10; `PP_EclipseInk`). Eerste live-ronde onthulde een klassiek depth-Sobel-artefact: **de scherende vloer vloeide vol inkt** (heel middenveld zwart). Fix: edge-detectie omgebouwd naar **Laplaciaan (2e afgeleide)** in `author_outline_material.py` — vlak-interieurs schoon onder elke kijkhoek, silhouetten + gevel-vloer-naden krijgen wél lijn. Tweede shotronde geverifieerd: vloer loopt door tot de horizon, lijnen schoon, schemerpalet staat. Galerij (`progress_media/shot_01..04.jpg`) ververst.
- *Kleine carry-over:* rename `Entries`→`EntryPoints` van de installateur gecommit (gedragsneutraal, groen geverifieerd).

**Vervolg diezelfde avond (alles groen + gepusht):** (1) shot-rig gefixt — pawn vliegt tijdens de rig, camera 4 kadreert nu het hele district; (2) hatch-tuning — arcering leest als penseelstroken (25% duty, periode 120); (3) SM6-fidelitypad in de builder, feature-level-gated zodat de SM5-laptop identiek blijft: volumetric smog, zon-schaduwen (lichtschachten door de silhouetten), realtime skylight, film grain 0.07 + bloom 0.45 (15.5-revisie); (4) **Kessara-skyline** (03.3) als code-built placeholder: deterministische ring (seed 503) buiten de perimeter — 56 fabriekshulks, 18 schoorstenen, 12 kraanportalen, natrium-oranje raamstroken; het district staat nu visueel in een stad, zonder één nav/missie/test-raakvlak.

**Nog later die nacht — asset-pass gestart (taak C, owner gaf blanket-akkoord):** eerste echte textures in de game via het autonome CC0-spoor: 4×2K diffuse van Poly Haven (asfalt/betonblok/golfplaat/metaalplaat; herkomst + licentie in `Eclipse/Content/Art/Textures/SOURCES.md`, import via `Tools/import_polyhaven_textures.py`). `M_EclipseToon` heeft nu een world-aligned albedo-pad (default neutraal); de builder koppelt per palet-entry texture/schaal/gain/mix. **Kalibratieles:** texturen her-meterden de auto-exposure (schemer→dag); opgelost door per texture het lineaire gemiddelde te meten en gain = 1/gemiddelde te zetten (shader klemt op 2.5) — de dusk-grade is exposure-invariant onder texturing. Geverifieerd met shotronde; groene bar opnieuw volledig groen.

**FAB-WACHTRIJ (voor de eigenaar, ~10 min):** Epic Games Launcher is op deze pc nog nooit gestart/ingelogd. Stappen: (1) Launcher starten → inloggen met het Epic-account (2FA bij de hand); (2) in de browser op fab.com met hetzelfde account inloggen; (3) melden — dan volgt een concrete shortlist (industriële modulaire kit, straatprops, Paragon-characters) waarop alleen "Add to library" + licentie-akkoord geklikt hoeft te worden; import + toon-restyle gebeurt daarna autonoom. Ook nuttig: ElevenLabs-key scopes Music/SFX aanzetten voor taak D-liveruns (TTS werkt al).

**Loop-iteratie 2 (zelfde nacht, alles groen + gepusht):** texture-variatie luminantie-only gemaakt (palet = enige kleur-autoriteit; Dominion-post weer zalm-oxide) · straat-dressing gelegd (hoofdader + dwarsstraat + markering + vlekken, no-collision) · 12 natrium-checkpointstrips op de binnenmuren · **SPEC-P2-00** (Vertical Slice-overview) via subagent opgeleverd + gereviewd in `phase0/specs/` — de ACTIVE_MILESTONE-omzetting naar Phase 2 blijft de call van de eigenaar (na de 13.2-playtest).

**Loop-iteratie 3:** lit-toon-migratie gebouwd als vlag-gated A/B (`M_EclipseToonLit`, DefaultLit, alleen via `-EclipseLitToon` op SM6; Glow blijft unlit). A/B-verdict: bij schemer op command-afstand ~niet te onderscheiden — unlit blijft default; het lit-pad wint pas bij interieurs/dag/characters. Alles groen.

**Loop-iteratie 4:** eerste echte 3D-meshes geland — CC0-props (vat/wegbarrière/krat, Poly Haven-API) door de toon-pijplijn via een nieuw mesh-UV-pad (UVMode-param); alle slots MID; les: bijna-neutrale ×10-paletten tonemappen naar bleek → vat op donker roest. Tools: Pillow ✓, ffmpeg ✓, Blender in UAC-wachtrij (geen blokker). Eigenaar: fab.com-login ✓; nog open: Launcher-login + shortlist-kliks, ElevenLabs-scopes. Character-route gekozen per Bible: **CC0-stylized (Quaternius) nu** voor squad/vijand-placeholders (past bij gelockte 15.5 + volledig autonoom), **MetaHuman-evaluatie bij Phase 2→3** voor hero-companions; Mixamo-animaties = klik-lijst voor de eigenaar zodra de animatie-pass start (geen browser-besturing in deze omgeving), met Quaternius' CC0-animatiebibliotheek als autonoom alternatief.

**Loop-iteratie 5:** Pillow-decals live — generator `Tools/generate_decals.py` (abstract AEGIS-oog, hazard, verzets-eclips; luminantie-only zodat het palet de kleur bepaalt), import + plaatsing als no-collision planes (compound-posters, kruisings-pads, stencils bij Entry_Main/warehouse); gains gemeten (7.8/1.3/7.1). Eigenaar heeft fab.com klaargezet + shortlist ontvangen (Industry Props Pack, modulaire industriële kit, Paragon Lt. Belica, Quixel-decals) — wacht op "Add to library"-kliks + "packs staan erin"-sein.

**Loop-iteratie 6:** Quaternius-characters binnen (gdown; BlueSoldier M/V + 2 burgers, volledige animatiesets) en als Idle-figuren geplaatst (enforcers goud bij poort/checkpoint, burgers teal bij warehouse). Lessen: `used_with_skeletal_mesh`-flag verplicht (anders zwart in -game) en verzadigde paletten vereist op de ×10-range. Fab-monitor draait; `_fab_inbox` bestaat; eigenaar is ingelogd op Launcher + fab.com en heeft de klik-route voor "Add to Project".

**Bekende volgende zwakste punten (15.8-lijst):** (1) zodra Fab-packs landen: de kit-pass (graybox-gebouwen vervangen — monitor vuurt vanzelf); (2) squad/vijand-BODY-swap naar Quaternius-meshes in AEclipseCharacter (gameplay-pass mét squad-scenario-suite); (3) meer figuren + patrol-loop (Walk-anim langs de hoofdader); (4) particles (vonken/as); (5) district-verticaliteit (03.3); (6) lit-toon bij interieur/dag.

**Owner-afspraken die blijven gelden (elke sessie):** geen installaties/downloads/acceptatie-prompts zonder expliciet akkoord vooraf (eerst uitleggen waarvoor); bouwen met `-NoUba`; PROGRESS.html nooit bewerken (data in `progress_data.js`; `progress_auto.js` is van de watcher). `gh` is hier NIET geïnstalleerd — push werkt via de vault-token in Windows Credential Manager (username gepind in repo-config).

**Eerstvolgende stap:** (1) hatch-tuning + shot-rig-fix, nieuwe shotronde ter review; (2) eigenaar speelt de loop in PIE — gate-vraag 13.2 — met squad-barks en Xbox-controller; (3) taak B: fidelity-pass per herziene 15.5 op deze machine; (4) taak C (art-pass, Fab/Quixel) — **wacht op per-download akkoord**; Epic Games Launcher staat op deze PC (Fab-toegang loopt via het ingelogde Epic-account); (5) taak D: Music/SFX-endpoints + adaptieve muziek (16.7).

---

## VORIGE STAND (2026-07-22, ~17:00 CET — laptop)

**Waar gestopt:** einde van de derde Fable 5-sessie van vandaag (voice live + graphics-doorbraak + controller + migratiedraaiboek). Werkboom schoon, alles gecommit en gepusht; **build groen, 31/31 tests groen, validatie 0 fouten, catalog 19/19**.

**Deze sessie (samengevat):**
- *Voice-pipeline LIVE bewezen:* dialogue-seed (JSON → voice-assets, create-only) + eerste echte ElevenLabs-run: 8/8 barks gegenereerd, geïmporteerd, auto-toegewezen; herhaalrun = 8 cache-hits / 0 API-calls. Cache gecommit (`9e33eec`, `2a67a09`). Key werkt volledig — de eigenaar heeft de ontbrekende scope aangezet; 23 stemmen zichtbaar via `/v1/voices`.
- *Graphics-doorbraak (pass 20–27 forensics):* het oude `PP_EclipseOutline`-postmateriaal bleek het HELE beeld te overschilderen — elke "paarse waas" was dat materiaal, niet de scène. Nieuw cel/toon-materiaal `M_EclipseToon` (Unlit, licht-banden + hatching in-shader; `Tools/author_toon_material.py`) bewezen werkend (rood/groen-diagnose). Builder: toon-MID's per paletkleur met hue-shifted schaduwen, zon –25°, exposure-bias –1.0, atmosfeer weer zon-gekoppeld. Inktlijnen her-authored als **`PP_EclipseInk`** (`Tools/author_outline_material.py`, verified scene-passthrough) — **beide scripts nog één keer draaien** (band-defaults 0.55/0.10 + nieuw ink-asset) en dan één `-EclipseShot`-ronde ter verificatie; tot dan rendert het district zonder lijnen (netjes afgevangen).
- *Xbox-controller + muis (owner request):* zelfde acties, gamepad-keys erbij (linkerstick lopen, rechterstick kijken, RT vuren, LB agressieve stance, D-pad squad-orders 1–4, B hurken, stick-klik sprint). Feel-pass (curves/deadzones/deltatime) bewust Phase 2 — PLACEHOLDER getagd.
- *Migratiedraaiboek:* `MIGRATION_TO_STRONG_PC.md` volledig herschreven als dag-één-draaiboek: **consent-protocol** (uitleg → akkoord vóór elke installatie/download; bouwen met `-NoUba` tegen firewallprompts), installatietabel mét redenen, secrets-flow, het bindende **asset-beleid** (algemene assets downloaden via Fab/Quixel ná akkoord; hero-assets pas handgebouwd op gelijk kwaliteitsniveau), de eerlijke laptop↔RTX-werkverdeling en in **§7 de bootstrap-prompt** voor de nieuwe PC (eindstaat: Visual Studio open met de solution, Fable 5/max ingelogd, groene bar, screenshots getoond — dan pas ontwikkelwerk).

**Owner-afspraken die blijven gelden (elke sessie):** geen installaties/downloads/acceptatie-prompts zonder expliciet akkoord vooraf (eerst uitleggen waarvoor); bouwen met `-NoUba`; PROGRESS.html nooit bewerken (data in `progress_data.js`; `progress_auto.js` is van de watcher).

**Bekende SM5-devbox-limiet (gedocumenteerd in code):** directionele lichten verlichten op het D3D12-SM5-fallbackpad van de GTX 1050 geen horizontale vlakken betrouwbaar; het Unlit-toonmateriaal omzeilt dit structureel (banden in de shader).

**Eerstvolgende stap:** (1) de twee materiaal-scripts draaien + één screenshotronde (kalibratie-verificatie — kan op deze laptop, geeft alleen even het reviewvenster); (2) eigenaar speelt de loop in PIE — gate-vraag 13.2 — hoort daarbij de eerste squad-barks en kan de Xbox-controller meteen testen; (3) migratie: `C:\Dev\ECLIPSE_SECRETS` per USB meenemen en op de nieuwe PC de **bootstrap-prompt uit `MIGRATION_TO_STRONG_PC.md` §7** plakken; (4) Phase-2 forward-infra wanneer gewenst: Music/SFX-endpoints + adaptieve muziek (16.7).
