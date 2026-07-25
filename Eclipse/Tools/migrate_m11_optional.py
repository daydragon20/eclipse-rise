#!/usr/bin/env python3
"""Wave-2-migratie: voeg M1.1's zero-casualty-optional toe aan het BESTAANDE MT_M11.

Waarom dit een eigen script is en geen rerun van setup_story_missions.py:
dat script schrijft zijn objectives binnen `if created:` en MT_M11 bestaat sinds
d7f22e4, dus een rerun logt "exists - untouched" en verandert niets. Die
create-only-regel is bewust (authored content nooit klobberen), dus de juiste
vorm is een smalle, expliciete, idempotente migratie - deze.

Wat het toevoegt (SPEC-P2-04 M1.1 + het bRequiresNoCasualties-amendement dat met
commit 67f46a6 landde):
  Obj_M11_NoCasualties  bOptional=True  bRequiresNoCasualties=True  +20 Materials

Semantiek van de vlag is een LATCH over de hele run - "niemand ging ooit neer" -
en de as-built draagt die latch al, want stabiliseren haalt een soldaat niet uit
DownedSoldierIds. Een gestabiliseerde soldaat kost dus nog steeds de bonus. Dat is
M1.4's strengste lezing, en die geldt bewust ook voor M1.1.

Idempotent: draait hij twee keer, dan ziet hij zijn eigen objective al staan en
doet niets. Hij raakt de twee mandatory objectives niet aan.

Run headless (commandlet-slot, editor DICHT):
  UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\\Eclipse\\Tools\\migrate_m11_optional.py" \\
    -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

Verwacht daarna: EclipseValidateData 0 fouten, en de suite moet de bestaande
optional-tests groen houden (die draaien op eigen in-memory drill-assets, dus ze
raken deze data niet). De HUD/debrief-kant komt uit dezelfde pure compose die al
getest is; dit script levert alleen de authored data.
"""

import unreal

OBJECTIVE_ID = "Obj_M11_NoCasualties"
MISSION_PATH = "/Game/Data/Missions/MT_M11"
REWARD_MATERIALS = 20  # SPEC-P2-04 M1.1: +20 M

eal = unreal.EditorAssetLibrary

if not eal.does_asset_exist(MISSION_PATH):
    raise RuntimeError(
        f"{MISSION_PATH} bestaat niet - draai eerst Tools/setup_story_missions.py "
        "(die maakt MT_M11 aan; deze migratie breidt hem alleen uit)."
    )

mission = eal.load_asset(MISSION_PATH)
objectives = list(mission.get_editor_property("objectives"))

existing_ids = [str(o.get_editor_property("objective_id")) for o in objectives]
if OBJECTIVE_ID in existing_ids:
    unreal.log(f"{OBJECTIVE_ID} staat er al - niets te doen (idempotent).")
else:
    # Sanity: de twee mandatory objectives moeten er zijn. Ontbreken ze, dan is dit
    # niet het asset dat de spec beschrijft en stoppen we liever dan te gokken.
    expected_mandatory = {"Obj_M11_PatrolLeader", "Obj_M11_Exfil"}
    missing = expected_mandatory - set(existing_ids)
    if missing:
        raise RuntimeError(
            f"MT_M11 mist de verwachte mandatory objectives {sorted(missing)} - "
            f"gevonden: {existing_ids}. Migratie afgebroken: dit asset ziet niet uit "
            "zoals SPEC-P2-04 beschrijft, en blind een optional toevoegen zou de "
            "missie stiller kapotter maken dan hij al is."
        )

    optional = unreal.EclipseObjectiveDef()
    optional.set_editor_property("objective_id", OBJECTIVE_ID)
    # Type blijft de default: de vlag doet het werk, niet een nieuw verb (decision 5
    # verbiedt nieuwe objective-verbs; conditie-vlaggen zijn juist het precedent).
    optional.set_editor_property("description", unreal.Text("Bring everyone home standing"))
    # LET OP de naamgeving: UE strípt de b-prefix van een bool-UPROPERTY en
    # snake_cases de rest, dus bOptional -> "optional" (NIET "b_optional") en
    # bRequiresNoCasualties -> "requires_no_casualties". Zelfde patroon als
    # setup_story_missions.py, dat bProgressRegionOnSuccess zet als
    # "progress_region_on_success". Eerste run hierop gefaald; dit is de correctie.
    optional.set_editor_property("optional", True)
    optional.set_editor_property("requires_no_casualties", True)
    optional.set_editor_property("optional_reward_materials", REWARD_MATERIALS)

    objectives.append(optional)
    mission.set_editor_property("objectives", objectives)
    eal.save_loaded_asset(mission)
    unreal.log(
        f"{OBJECTIVE_ID} toegevoegd aan MT_M11: bOptional, bRequiresNoCasualties, "
        f"+{REWARD_MATERIALS} Materials. Mandatory objectives onaangeraakt."
    )

unreal.log("Wave-2 M1.1-optional migratie klaar.")
