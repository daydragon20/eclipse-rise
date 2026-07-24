# SPEC-P2-03 data fill (step 3 materialisation): creates /Game/Data/DT_Facilities
# (FEclipseFacilityRow, the x0.8 Act-1 cost table - spec lines 88-98),
# DA_BaseTuning (rush 60 C/day, crew -1, analyst +1 Intel keyed by resource tag)
# and DA_BaseLayout_HollowPoint (slot-graph A-D on the EclipseBaseDefaults ids,
# Slot B's empty state = the 5.2 bunk camp), then links all three into
# DA_CampaignSetup. Numbers live here as data (GDD 14.2); code never hardcodes
# them - the 5.3.1 full-campaign values return with Phase 3 rebalancing, in this
# file, not in code. Re-runnable (idempotent fills).
#
# Run headless (queued in the commandlet slot - do not run alongside an editor):
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\setup_base_data.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

import json

import unreal

DATA_PATH = "/Game/Data"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary


def get_or_create(asset_name, asset_class, factory=None):
    path = f"{DATA_PATH}/{asset_name}"
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    return asset_tools.create_asset(asset_name, DATA_PATH, asset_class, factory)


def make_table_factory(row_struct):
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", row_struct)
    return factory


def make_tag(tag_string):
    return unreal.EclipseContentLibrary.request_tag(unreal.Name(tag_string))


INTEL = "Resource.Intel"

# --- DT_Facilities: the slice cost table (SPEC-P2-03 locked decision 3 - x0.8
# of the GDD 5.3.1 values so the 6.5 Act 1 band carries three builds + one
# upgrade; ratios Barracks : Workshop : IC = 1 : 1.33 : 2 preserved). Row names
# must match EclipseBaseDefaults / the layout's AllowedFacilityRows. "Only the
# Workshop has an L2" is the Levels array length - data, never code. Workshop
# L2's 200 M is slice-native (not x0.8 of 5.3.1's L2). UnlockTags stay empty in
# this step: the Workshop-L2 loadout-row wiring is SPEC-P2-07 integration.
facilities = get_or_create("DT_Facilities", unreal.DataTable,
                           make_table_factory(unreal.EclipseFacilityRow.static_struct()))
facilities_json = json.dumps([
    {"Name": "CommandCenter", "DisplayName": "Command Center",
     # Free and pre-built (locked decision 2: the stolen map table of 5.2);
     # the row exists for display/data completeness - slot A never rebuilds.
     "Levels": [{"CostMaterials": 0, "CostCredits": 0, "BuildDays": 0}]},
    {"Name": "Barracks", "DisplayName": "Barracks",
     # Formalizes the day-1 bunk camp: roster cap 11 -> 12, proper muster venue.
     "Levels": [{"CostMaterials": 120, "CostCredits": 0, "BuildDays": 2}]},
    {"Name": "Workshop", "DisplayName": "Workshop",
     # L1 unlocks the P1-03 production queue; L2 = manufacture tier (the only
     # slice upgrade).
     "Levels": [{"CostMaterials": 160, "CostCredits": 0, "BuildDays": 3},
                {"CostMaterials": 200, "CostCredits": 0, "BuildDays": 3}]},
    {"Name": "IntelligenceCenter", "DisplayName": "Intelligence Center",
     # +2 Intel/day (+1 with analyst via DA_BaseTuning): funds the 5 I prep
     # reveal ~every 2 days - the 11.1 fairness loop (locked decision 1).
     "Levels": [{"CostMaterials": 240, "CostCredits": 0, "BuildDays": 4,
                 "YieldPerDay": {INTEL: 2}}]},
])
if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(facilities, facilities_json):
    raise RuntimeError("DT_Facilities JSON fill failed")
eal.save_loaded_asset(facilities)
unreal.log("DT_Facilities: 4 slice facilities (CC free / Barracks 120M / Workshop 160+200M / IC 240M)")

# --- DA_BaseTuning: rush available-never-comfortable (5.4), staffing v1
# numbers (locked decision 6). The analyst bonus is keyed by resource tag so
# the IC rule lives in data, not in an IC branch.
tuning = get_or_create("DA_BaseTuning", unreal.EclipseBaseTuningAsset)
tuning.set_editor_property("rush_cost_credits_per_day", 60)
tuning.set_editor_property("crew_day_reduction", 1)
tuning.set_editor_property("analyst_intel_bonus_per_day", 1)
tuning.set_editor_property("max_crew_per_site", 1)
tuning.set_editor_property("analyst_bonus_resource", make_tag(INTEL))
eal.save_loaded_asset(tuning)
unreal.log("DA_BaseTuning: rush 60 C/day, crew -1, analyst +1 Resource.Intel, cap 1")


# --- DA_BaseLayout_HollowPoint: the 4-slot Act 1 vault (SPEC-P2-03 slot-graph).
# Every slot hangs off the Spine; C adds the generator room, B the memorial
# alcove (both non-slot ambient). Streaming ids name the level-instance/Data
# Layer swaps of the 5.4 visible-growth table; a missing layer is a logged
# warning + placeholder blockout at the walkable step (4-5), never a crash.
def slot(slot_id, name, allowed, adjacent, empty_id, construction_id, level_ids):
    d = unreal.EclipseBaseSlotDef()
    d.set_editor_property("slot_id", unreal.Name(slot_id))
    d.set_editor_property("display_name", unreal.Text(name))
    d.set_editor_property("allowed_facility_rows", [unreal.Name(a) for a in allowed])
    d.set_editor_property("adjacent_slot_ids", [unreal.Name(a) for a in adjacent])
    d.set_editor_property("empty_state_streaming_id", unreal.Name(empty_id))
    d.set_editor_property("construction_streaming_id", unreal.Name(construction_id))
    d.set_editor_property("level_streaming_ids", [unreal.Name(l) for l in level_ids])
    return d


layout = get_or_create("DA_BaseLayout_HollowPoint", unreal.EclipseBaseLayoutAsset)
layout.set_editor_property("slots", [
    # Slot A: the mandatory core, pre-built at L1 (EclipseBaseDefaults ids).
    slot("Slot_A", "Command Center", ["CommandCenter"], ["Spine"],
         "HP_SlotA_Empty", "HP_SlotA_Construction", ["HP_SlotA_L1"]),
    # Slot B's EMPTY state is the 5.2 bunk camp (bedrolls, crate table, muster
    # board) - working from day 1, never raw rock; squad pick never gates.
    slot("Slot_B", "Barracks", ["Barracks"], ["Spine", "MemorialAlcove"],
         "HP_SlotB_BunkCamp", "HP_SlotB_Construction", ["HP_SlotB_L1"]),
    slot("Slot_C", "Workshop", ["Workshop"], ["Spine", "GeneratorRoom"],
         "HP_SlotC_Empty", "HP_SlotC_Construction", ["HP_SlotC_L1", "HP_SlotC_L2"]),
    slot("Slot_D", "Intelligence Center", ["IntelligenceCenter"], ["Spine"],
         "HP_SlotD_Empty", "HP_SlotD_Construction", ["HP_SlotD_L1"]),
])
eal.save_loaded_asset(layout)
unreal.log("DA_BaseLayout_HollowPoint: slots A-D on the Spine (B empty = bunk camp)")

# --- DA_CampaignSetup: link the base data (asset objects, not path strings).
setup = eal.load_asset(f"{DATA_PATH}/DA_CampaignSetup")
setup.set_editor_property("base_layout", layout)
setup.set_editor_property("base_tuning", tuning)
setup.set_editor_property("facilities", facilities)
eal.save_loaded_asset(setup)
unreal.log("DA_CampaignSetup: BaseLayout + BaseTuning + Facilities gekoppeld")

unreal.log("SPEC-P2-03 base data complete.")
