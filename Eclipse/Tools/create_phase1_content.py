"""Phase 1 content bootstrap (SPEC-P1-02/03/04 data assets).

Run headless:
  UnrealEditor-Cmd.exe Eclipse.uproject -run=pythonscript
    -script="Tools/create_phase1_content.py" -unattended -nopause -nosplash

Creates (idempotent — existing assets are updated in place):
  /Game/Data/DA_KessaraDistrictGraph   (UEclipseRegionGraphAsset, 6 nodes)
  /Game/Data/DT_MissionOffers          (FEclipseMissionOfferRow per region type)
  /Game/Data/DT_ProductionItems        (FEclipseProductionItemRow, GDD 6.4.2)
  /Game/Data/DA_EconomyData            (UEclipseEconomyDataAsset)
  /Game/Data/DA_CampaignSetup          (UEclipseCampaignSetupAsset)

All numbers here are GDD-sourced data (14.2); code never hardcodes them.
"""
import json

import unreal

DATA_PATH = "/Game/Data"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
editor_asset = unreal.EditorAssetLibrary


def get_or_create(asset_name, asset_class, factory=None):
    path = f"{DATA_PATH}/{asset_name}"
    if editor_asset.does_asset_exist(path):
        return editor_asset.load_asset(path)
    return asset_tools.create_asset(asset_name, DATA_PATH, asset_class, factory)


def make_tag(tag_string):
    return unreal.EclipseContentLibrary.request_tag(unreal.Name(tag_string))


def region(region_id, name, rtype, edges, owner, unrest, garrison, yields):
    d = unreal.EclipseRegionDefinition()
    d.set_editor_property("region_id", unreal.Name(region_id))
    d.set_editor_property("display_name", unreal.Text(name))
    d.set_editor_property("region_type", rtype)
    d.set_editor_property("connected_region_ids", [unreal.Name(e) for e in edges])
    d.set_editor_property("starting_owner", owner)
    d.set_editor_property("starting_unrest", unrest)
    d.set_editor_property("starting_garrison", garrison)
    d.set_editor_property("base_yield_per_day", {make_tag(k): v for k, v in yields.items()})
    return d


CREDITS = "Resource.Credits"
MATERIALS = "Resource.Materials"
INTEL = "Resource.Intel"

RO = unreal.EclipseRegionOwner
RT = unreal.EclipseRegionType

# --- Region graph: the Kessara Foundry District, 6 nodes (GDD 3.3.1 flavor).
# Yields tuned to the GDD 6.5 Act 1 band (income 20-60 M/day, 50-150 C/day at
# partial control).
graph = get_or_create("DA_KessaraDistrictGraph", unreal.EclipseRegionGraphAsset)
graph.set_editor_property("regions", [
    region("Underworks", "The Underworks", RT.INDUSTRIAL,
           ["TransitCheckpoint", "WorkerHousing"], RO.PLAYER, 60, 0,
           {MATERIALS: 8, CREDITS: 20}),
    region("TransitCheckpoint", "Transit Checkpoint", RT.CHECKPOINT,
           ["Underworks", "FoundryRow", "SupplyDepot"], RO.DOMINION, 20, 4,
           {CREDITS: 30, INTEL: 2}),
    region("FoundryRow", "Foundry Row", RT.INDUSTRIAL,
           ["TransitCheckpoint", "CommsRelay"], RO.DOMINION, 35, 6,
           {MATERIALS: 30}),
    region("WorkerHousing", "Worker Housing", RT.RESIDENTIAL,
           ["Underworks", "SupplyDepot"], RO.CONTESTED, 55, 2,
           {CREDITS: 40}),
    region("SupplyDepot", "Supply Depot", RT.INDUSTRIAL,
           ["TransitCheckpoint", "WorkerHousing", "CommsRelay"], RO.DOMINION, 25, 5,
           {MATERIALS: 20, CREDITS: 20}),
    region("CommsRelay", "Comms Relay", RT.RESIDENTIAL,
           ["FoundryRow", "SupplyDepot"], RO.DOMINION, 15, 3,
           {INTEL: 4, CREDITS: 20}),
])

# --- Mission offers: one per region type (SPEC-P1-04), context lines follow
# GDD 11.3's causal rule.
def make_table_factory(row_struct):
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", row_struct)
    return factory


offers = get_or_create("DT_MissionOffers", unreal.DataTable,
                       make_table_factory(unreal.EclipseMissionOfferRow.static_struct()))

offers_json = json.dumps([
    {"Name": "Offer_Industrial", "RegionType": "Industrial", "TemplateId": "MT_Sabotage",
     "ContextLine": "Foundry shipments feed the checkpoint garrisons - cut the line and the district breathes.",
     "RewardCredits": 40, "RewardMaterials": 25, "RewardIntel": 0},
    {"Name": "Offer_Residential", "RegionType": "Residential", "TemplateId": "MT_Rescue",
     "ContextLine": "The Veil swept the block after curfew - our people are still in the holding pens.",
     "RewardCredits": 20, "RewardMaterials": 0, "RewardIntel": 6},
    {"Name": "Offer_Checkpoint", "RegionType": "Checkpoint", "TemplateId": "MT_Assault",
     "ContextLine": "Every patrol in the sector rotates through this checkpoint - take it and the map opens.",
     "RewardCredits": 60, "RewardMaterials": 10, "RewardIntel": 4},
])
if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(offers, offers_json):
    raise RuntimeError("DT_MissionOffers JSON fill failed")
graph.set_editor_property("mission_offers", offers)

# --- Production items (GDD 6.4.2 rows verbatim).
items = get_or_create("DT_ProductionItems", unreal.DataTable,
                      make_table_factory(unreal.EclipseProductionItemRow.static_struct()))

items_json = json.dumps([
    {"Name": "Item_RiflePlatform", "DisplayName": "Rifle platform",
     "CostMaterials": 40, "CostCredits": 0, "TimeDays": 1,
     "ResultLoadoutTag": {"TagName": "Loadout.Rifle"}},
    {"Name": "Item_ArmorSet", "DisplayName": "Armor set (Medium)",
     "CostMaterials": 60, "CostCredits": 0, "TimeDays": 2,
     "ResultLoadoutTag": {"TagName": "Loadout.MediumArmor"}},
])
if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(items, items_json):
    raise RuntimeError("DT_ProductionItems JSON fill failed")

# --- Roster data (SPEC-P1-07): name pools per origin, trait stubs, tuning.
name_pools = get_or_create("DT_NamePools", unreal.DataTable,
                           make_table_factory(unreal.EclipseNamePoolRow.static_struct()))
name_pools_json = json.dumps([
    {"Name": "Kessara",
     "FirstNames": ["Vara", "Oscar", "Ilya", "Mirin", "Sef", "Anke", "Dario", "Lupe",
                    "Tessa", "Jorun", "Calla", "Bren", "Nadia", "Piotr", "Yara", "Emeric"],
     "LastNames": ["Chen", "Line", "Kessler", "Voss", "Ashdown", "Ferro", "Okonkwo",
                   "Reyes", "Stahl", "Marek", "Odum", "Calder"]},
])
if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(name_pools, name_pools_json):
    raise RuntimeError("DT_NamePools JSON fill failed")

traits = get_or_create("DT_TraitStubs", unreal.DataTable,
                       make_table_factory(unreal.EclipseTraitStubRow.static_struct()))
traits_json = json.dumps([
    {"Name": "Trait_SteadyHands", "Line": "Steady hands - never wastes the second shot."},
    {"Name": "Trait_Claustrophobic", "Line": "Hates the tunnels. Took the job anyway."},
    {"Name": "Trait_Lucky", "Line": "Walked out of the Foundry Collapse. Twice."},
    {"Name": "Trait_Quiet", "Line": "Speaks in hand-signs even off shift."},
    {"Name": "Trait_Grudge", "Line": "Keeps a list of Veil badge numbers. It is long."},
    {"Name": "Trait_Mender", "Line": "Patches gear, people, and morale in that order."},
])
if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(traits, traits_json):
    raise RuntimeError("DT_TraitStubs JSON fill failed")

roster_tuning = get_or_create("DA_RosterTuning", unreal.EclipseRosterTuningAsset)
roster_tuning.set_editor_property("wounded_days_out", 5)
roster_tuning.set_editor_property("name_pools", name_pools)
roster_tuning.set_editor_property("trait_stubs", traits)

# --- Economy tunables (GDD 6.2/6.3.2/6.4.1).
economy = get_or_create("DA_EconomyData", unreal.EclipseEconomyDataAsset)
economy.set_editor_property("wage_per_soldier_per_day", 15)
economy.set_editor_property("intel_decay_per_week", 0.05)
economy.set_editor_property("intel_decay_interval_days", 7)
economy.set_editor_property("contested_yield_factor", 0.4)
economy.set_editor_property("production_items", items)

# --- Campaign setup (SPEC-P1-02: no hardcoded starting values).
setup = get_or_create("DA_CampaignSetup", unreal.EclipseCampaignSetupAsset)
setup.set_editor_property("starting_day", 1)
setup.set_editor_property("starting_resources",
                          {make_tag(CREDITS): 150, make_tag(MATERIALS): 80, make_tag(INTEL): 10})
setup.set_editor_property("starting_roster_size", 8)
setup.set_editor_property("region_graph", graph)
setup.set_editor_property("economy_data", economy)
setup.set_editor_property("roster_tuning", roster_tuning)

# --- Preparation data (SPEC-P1-08): loadout options + tuning.
loadouts = get_or_create("DT_LoadoutOptions", unreal.DataTable,
                         make_table_factory(unreal.EclipseLoadoutOptionRow.static_struct()))
loadouts_json = json.dumps([
    {"Name": "Loadout_Scavenged", "DisplayName": "Scavenged arms",
     "LoadoutTag": {"TagName": "Loadout.Scavenged"}, "RequiredUnlockTag": {"TagName": "None"}},
    {"Name": "Loadout_Rifle", "DisplayName": "Rifle platform",
     "LoadoutTag": {"TagName": "Loadout.Rifle"}, "RequiredUnlockTag": {"TagName": "Loadout.Rifle"}},
    {"Name": "Loadout_MediumArmor", "DisplayName": "Medium armor",
     "LoadoutTag": {"TagName": "Loadout.MediumArmor"}, "RequiredUnlockTag": {"TagName": "Loadout.MediumArmor"}},
])
if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(loadouts, loadouts_json):
    raise RuntimeError("DT_LoadoutOptions JSON fill failed")

prep_tuning = get_or_create("DA_PrepTuning", unreal.EclipsePrepTuningAsset)
prep_tuning.set_editor_property("intel_reveal_cost", 5)
prep_tuning.set_editor_property("squad_size", 2)
prep_tuning.set_editor_property("loadout_options", loadouts)
setup.set_editor_property("prep_tuning", prep_tuning)

# --- Squad data (SPEC-P1-06): bark pools + tuning.
order_defs = get_or_create("DT_SquadOrderDefs", unreal.DataTable,
                           make_table_factory(unreal.EclipseSquadOrderDefRow.static_struct()))
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
    # Een RIJ en geen veld (owner-beslissing 26-07, optie 1). Een neergeschoten
    # soldaat weigerde tot vandaag met de zin van het ORDERTYPE: vroeg je hem te
    # verplaatsen, dan zei hij "No route, boss." en wees hij je op een
    # routeprobleem dat er niet was. De reden Downed geldt identiek voor alle vier
    # de orders, dus een gedeelde pool is genoeg — en een extra rij vraagt geen
    # schemawijziging, wat na een nacht dode velden opruimen de bedoeling is.
    # AcknowledgeLines blijft leeg: wie neer ligt bevestigt niets.
    {"Name": "Downed",
     "AcknowledgeLines": [],
     "RefusalLines": ["I'm hit - can't move.", "I'm down, boss.", "Need a medic, not an order."]},
])
if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(order_defs, order_defs_json):
    raise RuntimeError("DT_SquadOrderDefs JSON fill failed")

squad_tuning = get_or_create("DA_SquadTuning", unreal.EclipseSquadTuningAsset)
squad_tuning.set_editor_property("follow_distance", 400.0)
squad_tuning.set_editor_property("cover_search_radius", 800.0)
squad_tuning.set_editor_property("response_timeout_seconds", 1.0)
squad_tuning.set_editor_property("order_defs", order_defs)
setup.set_editor_property("squad_tuning", squad_tuning)

# --- Combat data (SPEC-P1-05): archetypes + weapons + character tuning.
# Numbers follow the locked feel targets (player TTK ~0.6 s well-aimed: 60 HP /
# 22 dmg / 0.15 s interval; enemy vs exposed player ~2.5 s: 100 HP / 10 dmg /
# 0.8 s x 3 enemies in practice).
archetypes = get_or_create("DT_EnemyArchetypes", unreal.DataTable,
                           make_table_factory(unreal.EclipseEnemyArchetypeRow.static_struct()))
archetypes_json = json.dumps([
    {"Name": "Enforcer", "Health": 60, "Damage": 10, "PerceptionRadius": 2500, "FireInterval": 0.8},
    {"Name": "Trooper", "Health": 80, "Damage": 12, "PerceptionRadius": 3000, "FireInterval": 0.7},
])
if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(archetypes, archetypes_json):
    raise RuntimeError("DT_EnemyArchetypes JSON fill failed")

weapons = get_or_create("DT_Weapons", unreal.DataTable,
                        make_table_factory(unreal.EclipseWeaponRow.static_struct()))
weapons_json = json.dumps([
    {"Name": "AR_Foundry", "Damage": 22, "RangeCm": 5000, "FireInterval": 0.15, "HeadshotMultiplier": 2.5},
    {"Name": "Sidearm_Scrap", "Damage": 16, "RangeCm": 2500, "FireInterval": 0.25, "HeadshotMultiplier": 2.5},
])
if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(weapons, weapons_json):
    raise RuntimeError("DT_Weapons JSON fill failed")

char_tuning = get_or_create("DA_CharacterTuning", unreal.EclipseCharacterTuningAsset)
char_tuning.set_editor_property("walk_speed", 180.0)
char_tuning.set_editor_property("run_speed", 420.0)
char_tuning.set_editor_property("sprint_speed", 650.0)
char_tuning.set_editor_property("crouch_speed", 150.0)
char_tuning.set_editor_property("camera_fov", 90.0)
char_tuning.set_editor_property("max_health", 100.0)

# --- Mission templates (SPEC-P1-05) under /Game/Data/Missions/<TemplateId>.
def make_objective(obj_id, obj_type, description, optional=False, target=""):
    objective = unreal.EclipseObjectiveDef()
    objective.set_editor_property("objective_id", unreal.Name(obj_id))
    objective.set_editor_property("type", obj_type)
    objective.set_editor_property("description", unreal.Text(description))
    objective.set_editor_property("optional", optional)
    objective.set_editor_property("target_id", unreal.Name(target))
    return objective


def make_spawn(archetype, count, site):
    spawn = unreal.EclipseEnemySpawnSet()
    spawn.set_editor_property("archetype_id", unreal.Name(archetype))
    spawn.set_editor_property("count", count)
    spawn.set_editor_property("spawn_site_id", unreal.Name(site))
    return spawn


MISSIONS_PATH = "/Game/Data/Missions"
OT = unreal.EclipseObjectiveType


def make_mission(template_id, name, objectives, spawns):
    path = f"{MISSIONS_PATH}/{template_id}"
    if editor_asset.does_asset_exist(path):
        mission = editor_asset.load_asset(path)
    else:
        mission = asset_tools.create_asset(template_id, MISSIONS_PATH, unreal.EclipseMissionAsset, None)
    mission.set_editor_property("template_id", unreal.Name(template_id))
    mission.set_editor_property("display_name", unreal.Text(name))
    mission.set_editor_property("objectives", objectives)
    mission.set_editor_property("insertion_point_ids",
                                [unreal.Name("Entry_Main"), unreal.Name("Entry_Sewer"), unreal.Name("Entry_Roof")])
    mission.set_editor_property("enemy_spawns", spawns)
    mission.set_editor_property("progress_region_on_success", True)
    return mission


mission_assault = make_mission("MT_Assault", "Checkpoint Assault", [
    make_objective("Obj_Primary", OT.DESTROY_TARGET, "Break the checkpoint's control post", target="Site_ControlPost"),
    make_objective("Obj_NoAlarms", OT.REACH_LOCATION, "Optional: cut the alarm relay first", optional=True, target="Site_AlarmRelay"),
    make_objective("Obj_Exfil", OT.EXTRACT_SQUAD, "Extract the squad", target="Site_Extraction"),
], [make_spawn("Enforcer", 4, "Spawn_Checkpoint"), make_spawn("Trooper", 2, "Spawn_Reserve")])

mission_sabotage = make_mission("MT_Sabotage", "Foundry Line Sabotage", [
    make_objective("Obj_Primary", OT.DESTROY_TARGET, "Wreck the shipment crane", target="Site_Crane"),
    make_objective("Obj_Exfil", OT.EXTRACT_SQUAD, "Extract the squad", target="Site_Extraction"),
], [make_spawn("Enforcer", 4, "Spawn_Yard")])

mission_rescue = make_mission("MT_Rescue", "Holding Pen Rescue", [
    make_objective("Obj_Primary", OT.COLLECT_ITEM, "Open the holding pens", target="Site_Pens"),
    make_objective("Obj_Exfil", OT.EXTRACT_SQUAD, "Get everyone out", target="Site_Extraction"),
], [make_spawn("Enforcer", 3, "Spawn_Pens"), make_spawn("Trooper", 2, "Spawn_Patrol")])

for asset in (graph, offers, items, economy, name_pools, traits, roster_tuning, loadouts,
              prep_tuning, order_defs, squad_tuning, archetypes, weapons, char_tuning,
              mission_assault, mission_sabotage, mission_rescue, setup):
    editor_asset.save_loaded_asset(asset)

unreal.log("Phase 1 content bootstrap complete: 18 assets in /Game/Data.")
