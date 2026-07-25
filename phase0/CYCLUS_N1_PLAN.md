# Cyclus N+1 — orkestratieplan (game-planner, 2026-07-25 ~12:10)

*Geldt ná de landing van de vijf changesets + commandlet-rij (setup_story_missions →
inventory_metahumans_wave2 → import_modular_civilians) + her-gemeten unie-bar.
Kalibratie: HEAD telt 57 testmacro's, de werkboom 68 → **landings-bar verwacht 68/68**
(niet 58). Eind N+1 verwacht: ~80-87 tests (middenschatting 83).*

## Main-agent (serieel)
1. **M1.1-functionele Gauntlet** (eerste actie; EclipseMissionM1Tests.cpp): rewards == rij
   (50 C/25 M), dag +1, beat gezet, regio onaangetast, fail-pad zonder beat. Bij groen:
   "spaces locked M1.1-site" vastleggen (P2-08-startsignaal).
2. Land-orkestratie + **exclusief eigenaarschap EventCatalog.md** (builders leveren
   catalog-rijen als handoff-tekst; main commit ze samen met de code).
3. Wave-2-changesets (hieronder).

## Vier parallelle element-builders (bestand-fences exclusief)
- **EB-1 [Quests] Taak 4-kern** (TAAK4_STORY_SURFACE.md stap 1): MissionTypes/MissionLogic/
  MissionSubsystem/EventPayloads/GameplayTags/MissionTests/StoryTests. Catalog 28→29 zelfde
  commit (via main); géén schema-bump. NIET: CampaignSetupAsset.h (recap = wave 2).
- **EB-2 [Strategy] P2-05 stap-1-restant + wiring**: StrategySubsystem, Liberation-bestanden,
  CampaignSetupAsset.h (deze wave exclusief EB-2), ValidateData-commandlet, nieuw
  Tools/setup_liberation_data.py. Verplicht: header-doc-verplichtingen EclipseLiberationLogic.h,
  F1-re-entrancy-Gauntlet, idempotentie-Gauntlet, regressie "M1.1-completion flipt niets".
  Géén nieuwe tags; 2 catalog-consumer-rijen via main.
- **EB-3 [Base] P2-03 stap 4-5B**: walkable vault in NIEUW Base/EclipseVaultBuilder.* (nooit
  EclipseGrayboxBuilder.cpp), BaseSubsystem, BaseTests. Parity-Gauntlet groen VÓÓR
  menu-hub-retirement (aparte mini-changeset).
- **EB-4 [Art] Dressing-iteratie 2** per DRESSING_ITERATIE_2.md (vloer eerst, dan pools/
  blobs, dan nudges); exclusief GrayboxBuilder.cpp + generate_decals.py. Civilian-wiring
  is wave 2, NIET hier.

**Slot-rij (main beheert):** liberation-data-script → dressing-shotrondes (eerste PNG =
warm-up, overslaan). **Landingsvolgorde build-slot:** main-Gauntlet → EB-2 → EB-1 → EB-3 → EB-4.

## Wave 2 (serieel, binnen N+1)
1. setup_story_missions.py-extensie: M1.1 zero-casualty-optional (+20 M) — ná EB-1.
2. Recap-materialisatie (copy: RECAP_CARDS_M1.md) — ná EB-2 (header-fence vrij);
   daarna **cold-reader op de owner-kliklijst** (harde poort vóór M1.2).
3. Civilian-builder-wiring (eigen [Art]-mini + shotronde) — ná EB-4 + civilian-import.
4. **M1.2-authoring is N+2** — gegate op cold-reader 4/4.

## Besluit open spec-punt (TAAK4 §2)
**`bRequiresNoCasualties` AANGENOMEN als SPEC-P2-04-amendement** (zelfde commit als de code,
één alinea in §Data schema): decision 5 verbiedt nieuwe verbs, geen conditie-vlaggen —
bRequiresNoAlarm is al precedent; deblokkeert M1.1 (+20 M) én M1.4 ("no soldier downed",
+15 M). **Semantiek = latch**: "niemand ging óóit neer" (strengste lezing, M1.4-formulering);
als DownedSoldierIds gestabiliseerden verwijdert → aparte casualty-latch naast bAlarmRaised
(reset bij StartMission), truth-table-test mét downed→gestabiliseerd-geval zelfde commit.

## Owner-kliklijst-mutaties
- Eraf bij landing: editor-close (is de trigger); MetaHumans zodra de probe groen rapporteert.
- R3 'true' → Stage B plánbaar voor N+2 (niets in N+1 hangt eraan); 'false' → fallback-ladder
  SPEC-P2-02, capaciteit naar feel-tuning in N+2. Verdict loggen in EXECUTION_PLAN §2-S7.
- Erbij (wave 2): cold-reader-moment (~10 min, protocol in RECAP_CARDS_M1.md).
- Geen nieuwe install-consents nodig in heel N+1.
