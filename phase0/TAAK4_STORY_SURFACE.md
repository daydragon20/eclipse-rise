# P2-04 Taak 4 — objective-extensie, alarm-fold, PhaseChanged, recap-cards

*Implementatie-spec (25-07, main-agent) voor de eerstvolgende story-cyclus ná de landing van
changesets 1-5. Bron: SPEC-P2-04 resolved points 2/3 + decision 3/5/8 + testsectie; planner-Taak 4.
Deblokkeert: M1.1's zero-casualty-optional (bewust weggelaten uit setup_story_missions.py) en
M1.2-authoring (ghost-optional heeft bRequiresNoAlarm nodig; cold-reader vóór M1.2-start).*

## 1. Eén nieuw event: Event.Mission.PhaseChanged (catalog 28→29, zelfde commit)

- Tag in EclipseGameplayTags (na de landing niet meer in-flight) + catalog-rij.
- Payload: bestaand FEclipseMissionEventPayload uitbreiden met `FName PhaseName` +
  `bool bAuthoredSubPhase` (resolved point 2) — outer-fases (Insertion/Objectives/Extraction/
  Debrief) broadcasten met bAuthoredSubPhase=false vanuit de bestaande fase-overgangen in
  UEclipseMissionSubsystem; authored sub-fases (StateTree-tasks, later) met true.
- **Alarm = benoemde sub-fase, géén nieuw event**: het feit "Alarm" reist als
  PhaseChanged(PhaseName="Alarm", bAuthoredSubPhase=true). Bron in de slice: een
  `NotifyAlarmRaised()`-API op het mission-subsystem (aanroepbaar door enemy-AI/alert-code en
  straks StateTree); idempotent (tweede alarm broadcast niet opnieuw).

## 2. FEclipseObjectiveDef-extensie (EclipseMissionTypes.h ~r41-56; geen nieuwe primitieven)

- `bool bRequiresNoAlarm = false` — ghost-optional: bij debrief-compose faalt de optional als
  de alarm-latch stond (11.4: alarm ≠ missiefalen; het kost de optional).
- `int32 OptionalRewardCredits/Materials/Intel = 0` — alleen gelezen bij bOptional; complete
  optionals componeren AdjustResource-mutaties (Reason "OptionalObjective") in dezélfde
  debrief-transactie (14.3.3; zelfde patroon als de beat).
- **BESLOTEN (planner, 25-07 — zie CYCLUS_N1_PLAN.md): `bool bRequiresNoCasualties = false`
  AANGENOMEN als formeel SPEC-P2-04-amendement** (één alinea in §Data schema, zelfde commit
  als de code; decision 5 verbiedt verbs, geen conditie-vlaggen — bRequiresNoAlarm is het
  precedent; deblokkeert M1.1 +20 M én M1.4 "no soldier downed" +15 M). **Semantiek = latch**:
  "niemand ging óóit neer" (M1.4's strengste lezing geldt ook voor M1.1); als de as-built
  DownedSoldierIds gestabiliseerde soldaten weer verwijdert → aparte casualty-latch naast
  bAlarmRaised (reset bij StartMission, zelfde parameter-pad naar de pure compose).
  Truth-table-test mét het downed→gestabiliseerd-geval in dezelfde commit.

## 3. Alarm-latch (runtime, geen schemabreak)

- `bool bAlarmRaised` op het mission-subsystem, reset bij StartMission; gezet door
  NotifyAlarmRaised → broadcast PhaseChanged("Alarm"). Debrief-compose krijgt de latch als
  parameter (pure evaluatie in EclipseMissionLogic — testbaar zonder engine).
- Ghost-evaluatie bij compose: optional met bRequiresNoAlarm && latch → NIET in
  CompletedObjectiveIds' optionele beloningen; wél gewoon zichtbaar als "gemist" in debrief.

## 4. Recap-cards (copy staat klaar: phase0/RECAP_CARDS_M1.md)

- `FEclipseRecapCard { FName StillId; FText Lines; }` + `TArray<FEclipseRecapCard> RecapCards`
  op UEclipseCampaignSetupAsset (lege array = geen recap, 14.3.5).
- Toon-conditie (state-derived): geselecteerde missie == MT_M11 && Story.Beat.M11 niet gezet.
- Surface: bestaande briefing-debugscherm + `Eclipse.Story.Report`-console-uitbreiding.
- Owner-actie ná materialisatie: cold-reader (4 vragen, 4/4) — protocol in RECAP_CARDS_M1.md;
  harde regel: vóór M1.2-authoring-start.

## 5. Volgorde + tests

1. Schema + event + latch + compose-uitbreiding mét pure tests (ghost-truth-table:
   alarm×optional×win/verlies; optional-rewards atomair; PhaseChanged-emissiepad).
2. setup_story_missions.py: M1.1-optional alsnog authored (zero-casualty, +20 M via
   OptionalRewardMaterials) — de eerlijkheids-comment in het script verwijst hiernaar.
3. Recap-materialisatie + Story.Report + cold-reader-klikje op de owner-lijst.
4. Dan pas M1.2-authoring (DestroyTarget+ExtractSquad, ghost-optional, pin achter
   Story.Beat.M11 — DT_StoryMissions rij 2).
Checkpoint-eisen: catalog-rij zelfde commit (29/29); geen SchemaVersion-bump (alles
runtime/data-asset); nieuwe tests zelfde commit; review vóór commit.
