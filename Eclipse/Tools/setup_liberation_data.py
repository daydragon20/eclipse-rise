# SPEC-P2-05 build step 1 content: materialises DT_LiberationInstances with the
# slice's ONE row — Foothold: the trio TransitCheckpoint + WorkerHousing +
# SupplyDepot flips to Player on M1.3 "Signal Fire" completion, gated on the
# Story.Beat.M13_SignalFire beat the debrief commits (locked decisions 1/2/5;
# the row stores the tag NAME, so this script runs before and after the native
# tag exists). No hardcoded trio anywhere in C++ — the instance is data.
# Re-runnable: DT_LiberationInstances full-fill (authoritative data,
# setup_story_missions pattern); DA_CampaignSetup link idempotent.
#
# Run headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\setup_liberation_data.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

import json

import unreal

DATA_PATH = "/Game/Data"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary

# --- DT_LiberationInstances: the Foothold row (SPEC-P2-05 locked decision 1 —
# the minimal set that touches all three resource columns the P2-03 calendar
# leans on: TC = C+I, WH = C, SD = M+C -> the 28 M / 110 C / 2 I band).
table_path = f"{DATA_PATH}/DT_LiberationInstances"
if eal.does_asset_exist(table_path):
    table = eal.load_asset(table_path)
else:
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", unreal.EclipseLiberationRow.static_struct())
    table = asset_tools.create_asset("DT_LiberationInstances", DATA_PATH, unreal.DataTable, factory)

rows = json.dumps([
    {"Name": "Foothold",
     "TriggerMissionId": "MT_M13",
     # Row order = commit order = event order (pure-core contract): the map
     # walks the pocket outward from the Underworks.
     "RegionIds": ["TransitCheckpoint", "WorkerHousing", "SupplyDepot"],
     # Audit gate against the committed story flags (spec review point 3: KEEP,
     # empty-means-ungated). The debrief sets this beat BEFORE Completed fires.
     "RequiredBeatTag": {"TagName": "Story.Beat.M13_SignalFire"},
     "NewOwner": "Player",
     # Step-4 debrief surface line (11.3's causal rule, hand-written).
     "ContextLine": "The district jammer is dead. Three blocks answer at once - the Foothold holds."},
])
if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, rows):
    raise RuntimeError("DT_LiberationInstances JSON fill failed")
eal.save_loaded_asset(table)
unreal.log("DT_LiberationInstances: Foothold row filled (trio -> Player, beat-gated).")

# Link the table into the campaign setup — without this the trigger degrades to
# "no liberations + one warning" by design (GDD 14.3.5) and M1.3 never flips.
setup = eal.load_asset(f"{DATA_PATH}/DA_CampaignSetup")
if setup is None:
    raise RuntimeError("DA_CampaignSetup missing - liberation table created but not linked")
setup.set_editor_property("liberation_instances", table)
eal.save_loaded_asset(setup)
unreal.log("DA_CampaignSetup.LiberationInstances gekoppeld.")

unreal.log("SPEC-P2-05 liberation data complete.")
