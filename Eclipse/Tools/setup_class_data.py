# SPEC-P2-01 data fill: creates /Game/Data/DT_ClassDefs (FEclipseClassDefRow)
# with the three slice classes (Assault/Medic/Sniper — GDD 4.2.3), extends
# DT_Weapons with the class platforms (rows only — spec non-goal: nothing
# outside DT_Weapons), adds the "Stabilize" bark row to DT_SquadOrderDefs,
# sets DA_SquadTuning.MaxDeployed = 4 and links DA_CampaignSetup.ClassDefs.
# Numbers live here as data (GDD 14.2); code never hardcodes them. Re-runnable.
#
# Run headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\setup_class_data.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

import json

import unreal

DATA_PATH = "/Game/Data"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary


def get_or_create_table(asset_name, row_struct):
    path = f"{DATA_PATH}/{asset_name}"
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", row_struct)
    return asset_tools.create_asset(asset_name, DATA_PATH, unreal.DataTable, factory)


# --- DT_ClassDefs: the three slice classes (SPEC-P2-01 locked decision 1).
# Medic window 30 s (GDD 4.2.5); Sniper killzone 6000 cm; Assault push 300 cm
# (~2 cover pieces past the ordered point); Sniper lane bias 1.0 (prefers the
# longer covered lane); bodies reuse the step-2 Rebel pool so the kit is
# visible today without new assets.
classes = get_or_create_table("DT_ClassDefs", unreal.EclipseClassDefRow.static_struct())
classes_json = json.dumps([
    {"Name": "Assault", "DisplayName": "Assault",
     "WeaponRow": "AR_Foundry", "BodyDefOverride": "Rebel_A",
     "SignatureVerb": {"TagName": "Class.Verb.Momentum"},
     "StabilizeWindowSeconds": 0, "KillzoneRangeCm": 0,
     "BarkSet": "Barks_Assault",
     "OrderPushDistanceCm": 300, "CoverLaneBias": 0, "bAutoTriage": False},
    {"Name": "Medic", "DisplayName": "Medic",
     "WeaponRow": "SMG_Patch", "BodyDefOverride": "Rebel_B",
     "SignatureVerb": {"TagName": "Class.Verb.Stabilize"},
     "StabilizeWindowSeconds": 30, "KillzoneRangeCm": 0,
     "BarkSet": "Barks_Medic",
     "OrderPushDistanceCm": 0, "CoverLaneBias": 0, "bAutoTriage": True},
    {"Name": "Sniper", "DisplayName": "Sniper",
     "WeaponRow": "DMR_Longsight", "BodyDefOverride": "Rebel_C",
     "SignatureVerb": {"TagName": "Class.Verb.Killzone"},
     "StabilizeWindowSeconds": 0, "KillzoneRangeCm": 6000,
     "BarkSet": "Barks_Sniper",
     "OrderPushDistanceCm": 0, "CoverLaneBias": 1.0, "bAutoTriage": False},
])
if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(classes, classes_json):
    raise RuntimeError("DT_ClassDefs JSON fill failed")
eal.save_loaded_asset(classes)
unreal.log("DT_ClassDefs: 3 slice classes (Assault/Medic/Sniper)")

# --- DT_Weapons: full refill = the two Phase 1 platforms verbatim + the class
# platforms (fill_data_table replaces all rows, so originals repeat here).
# Feel intent: SMG shreds close/fast, low reach; DMR hits hard/slow at lane
# range matching the killzone data.
weapons = eal.load_asset(f"{DATA_PATH}/DT_Weapons")
weapons_json = json.dumps([
    {"Name": "AR_Foundry", "Damage": 22, "RangeCm": 5000, "FireInterval": 0.15, "HeadshotMultiplier": 2.5},
    {"Name": "Sidearm_Scrap", "Damage": 16, "RangeCm": 2500, "FireInterval": 0.25, "HeadshotMultiplier": 2.5},
    {"Name": "SMG_Patch", "Damage": 14, "RangeCm": 3000, "FireInterval": 0.09, "HeadshotMultiplier": 2.0},
    {"Name": "DMR_Longsight", "Damage": 45, "RangeCm": 9000, "FireInterval": 1.1, "HeadshotMultiplier": 3.0},
])
if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(weapons, weapons_json):
    raise RuntimeError("DT_Weapons JSON fill failed")
eal.save_loaded_asset(weapons)
unreal.log("DT_Weapons: 4 platforms (2 Phase 1 + SMG_Patch + DMR_Longsight)")

# --- DT_SquadOrderDefs: full refill = the four SPEC-P1-06 rows verbatim + the
# Stabilize bark row (the save is spoken — GDD 9.5; refusal lines cover the
# too-late arrival).
order_defs = eal.load_asset(f"{DATA_PATH}/DT_SquadOrderDefs")
order_defs_json = json.dumps([
    {"Name": "MoveTo",
     "AcknowledgeLines": ["On it.", "Moving.", "Copy - relocating."],
     "RefusalLines": ["No route, boss.", "Can't get there from here.", "That path's blocked."]},
    {"Name": "FocusTarget",
     "AcknowledgeLines": ["Target marked.", "On your mark.", "Engaging."],
     "RefusalLines": ["Can't see the target.", "No shot from here.", "Target's gone."]},
    {"Name": "Hold",
     "AcknowledgeLines": ["Holding.", "Anchored.", "Not moving."],
     "RefusalLines": ["Can't hold here."]},
    {"Name": "Regroup",
     "AcknowledgeLines": ["Falling back to you.", "Coming in.", "Regrouping."],
     "RefusalLines": ["No way back to you.", "Route's cut."]},
    {"Name": "Stabilize",
     "AcknowledgeLines": ["Stay with me - you're not done.", "Got you. Breathe.", "Stabilized. We carry them home."],
     "RefusalLines": ["I was too late...", "No pulse. Damn it."]},
])
if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(order_defs, order_defs_json):
    raise RuntimeError("DT_SquadOrderDefs JSON fill failed")
eal.save_loaded_asset(order_defs)
unreal.log("DT_SquadOrderDefs: 5 rows (4 orders + Stabilize barks)")

# --- DA_SquadTuning: the squad-of-4 number lives in data (SPEC-P2-01).
squad_tuning = eal.load_asset(f"{DATA_PATH}/DA_SquadTuning")
squad_tuning.set_editor_property("max_deployed", 4)
eal.save_loaded_asset(squad_tuning)
unreal.log("DA_SquadTuning.MaxDeployed = 4 (player + 3)")

# --- DA_CampaignSetup: link the class table (asset object, not a path string).
setup = eal.load_asset(f"{DATA_PATH}/DA_CampaignSetup")
setup.set_editor_property("class_defs", classes)
eal.save_loaded_asset(setup)
unreal.log("DA_CampaignSetup.ClassDefs gekoppeld")

unreal.log("SPEC-P2-01 class data complete.")
