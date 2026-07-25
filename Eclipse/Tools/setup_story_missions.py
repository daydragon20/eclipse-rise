# SPEC-P2-04 build step 2 completion: materialises the authored M1.1 mission
# asset on the exact ResolveMissionSpec load path (/Game/Data/Missions/MT_M11 —
# the runtime's carrying capacity for authored assets is R7-proven by
# Eclipse.Missions.M11SkeletonCarriesAuthoredAsset) and DT_StoryMissions with
# the M1.1 row (unlock empty = available from campaign start; completion beat
# Story.Beat.M11_ThirteenBullets lands as a native tag with the wiring commit —
# the row stores the tag NAME, so this script stays runnable before and after).
# Re-runnable: mission asset create-only (authored content is never clobbered);
# DT_StoryMissions full-fill (authoritative data, setup_class_data pattern).
#
# Run headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\setup_story_missions.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

import json

import unreal

MISSIONS_PATH = "/Game/Data/Missions"
DATA_PATH = "/Game/Data"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary


def get_or_create_mission(name):
    path = f"{MISSIONS_PATH}/{name}"
    if eal.does_asset_exist(path):
        unreal.log(f"{name}: exists - create-only, untouched (authored content).")
        return eal.load_asset(path), False
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.EclipseMissionAsset)
    return asset_tools.create_asset(name, MISSIONS_PATH, None, factory), True


# --- MT_M11 "Thirteen Bullets" (SPEC-P2-04 mission table: DestroyTarget patrol
# leader + ExtractSquad; no region flip — decision 6; ambush arena sites).
m11, created = get_or_create_mission("MT_M11")
if created:
    m11.set_editor_property("template_id", "MT_M11")
    m11.set_editor_property("display_name", unreal.Text("Thirteen Bullets"))
    m11.set_editor_property("progress_region_on_success", False)

    ambush = unreal.EclipseObjectiveDef()
    ambush.set_editor_property("objective_id", "Obj_M11_PatrolLeader")
    ambush.set_editor_property("type", unreal.EclipseObjectiveType.DESTROY_TARGET)
    ambush.set_editor_property("description", unreal.Text("Spring the ambush: take out the patrol leader"))
    ambush.set_editor_property("target_id", "Site_ControlPost")  # existing graybox site (SPEC-P1-05)

    exfil = unreal.EclipseObjectiveDef()
    exfil.set_editor_property("objective_id", "Obj_M11_Exfil")
    exfil.set_editor_property("type", unreal.EclipseObjectiveType.EXTRACT_SQUAD)
    exfil.set_editor_property("description", unreal.Text("Extract the squad"))
    exfil.set_editor_property("target_id", "Site_Extraction")

    # The zero-casualty optional (spec M1.1, +20 M) waits for the
    # FEclipseObjectiveDef schema extension (OptionalReward*/bRequiresNoAlarm +
    # debrief evaluation). A stand-in typed ExtractSquad would read "achieved"
    # on every extraction — a lying HUD line ships never (14.3.6 honesty).
    m11.set_editor_property("objectives", [ambush, exfil])
    m11.set_editor_property("insertion_point_ids", ["Entry_Main", "Entry_Sewer"])
    eal.save_loaded_asset(m11)
    unreal.log("MT_M11 authored: 2 mandatory objectives, no region flip.")

# --- DT_StoryMissions: the M1.1 pin (M1.2-M1.4 rows land as their missions are
# authored, each behind its predecessor's completion beat).
table_path = f"{DATA_PATH}/DT_StoryMissions"
if eal.does_asset_exist(table_path):
    table = eal.load_asset(table_path)
else:
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", unreal.EclipseStoryMissionRow.static_struct())
    table = asset_tools.create_asset("DT_StoryMissions", DATA_PATH, unreal.DataTable, factory)

rows = json.dumps([
    {"Name": "MT_M11", "MissionId": "MT_M11",
     "MissionAsset": f"{MISSIONS_PATH}/MT_M11.MT_M11",
     "PinnedRegionId": "TransitCheckpoint",  # the Phase-1 graph's checkpoint node — the ambush chokepoint
     "UnlockBeatTag": "",
     "CompletionBeatTag": {"TagName": "Story.Beat.M11_ThirteenBullets"},
     "RewardCredits": 50, "RewardMaterials": 25, "RewardIntel": 0,
     "BriefingText": "Thirteen bullets between us and the first real strike. Make every one of them a promise.",
     "BriefingSpeaker": "Mara"},
])
if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, rows):
    raise RuntimeError("DT_StoryMissions JSON fill failed")
eal.save_loaded_asset(table)
unreal.log("DT_StoryMissions: M1.1 row pinned (rewards on the decision-10 band).")

# Link the table into the campaign setup — without this the resolver degrades
# to region offers by design and the pin never surfaces (review blocker 2).
setup = eal.load_asset(f"{DATA_PATH}/DA_CampaignSetup")
if setup is None:
    raise RuntimeError("DA_CampaignSetup missing - story table created but not linked")
setup.set_editor_property("story_missions", table)
eal.save_loaded_asset(setup)
unreal.log("DA_CampaignSetup.StoryMissions gekoppeld.")

unreal.log("SPEC-P2-04 story-mission data complete.")
