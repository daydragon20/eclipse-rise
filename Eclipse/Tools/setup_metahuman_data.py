# Step-3 MetaHuman pipeline: inventory + probe + slot link (phase0/metahuman_recipes.md).
#
# The Fab click landed the first MetaHuman ("SentinelC") with every internal
# reference authored for the /Game/CharacterAssemblies/SentinelC/ root (232/232
# assets, binary-verified); the tree now lives there on disk. This script:
#   1. inventories /Game/CharacterAssemblies/SentinelC (classes, counts),
#   2. PROBES the body mesh: it must load AND resolve its skeleton — the link
#      is only written on proof (phase0/ASSET_CLEANUP.md evidence rule),
#   3. links the body mesh into the Kaya slot of DT_NamedCharacters as a
#      PROVISIONAL stand-in (evidence: metahuman_base_skel lives under
#      Common/Female/Medium/NormalWeight -> female, medium, normal weight;
#      groom Hair_S_AfroFade = short practical hair -> closest to the Kaya
#      recipe; owner delivery of MH_Kaya per the recipe replaces it).
#      FallbackBodyDef stays Rebel_B, so nothing regresses if the soft ref
#      ever fails to resolve (GDD 14.3.5).
#
# NOTE: FEclipseNamedCharacterRow.MetaHumanMesh has no runtime consumer yet
# (grep 2026-07-24: struct + DA_CampaignSetup pointer only) — this is data
# wiring ahead of the consumer tier; toon-restyle over MetaHuman materials is
# a 15.7 policy decision that stays with the art reviewer/owner.
#
# Run headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\setup_metahuman_data.py" \
#     -unattended -nopause -nosplash -nullrhi

import unreal

registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.wait_for_completion()
eal = unreal.EditorAssetLibrary

ROOT = "/Game/CharacterAssemblies/SentinelC"
BODY_MESH = f"{ROOT}/SentinelC/Body/SKM_MHC_Little_00_BodyMesh"
FACE_MESH = f"{ROOT}/SentinelC/Face/SKM_MHC_Little_00_FaceMesh"

# --- 1. Inventory -----------------------------------------------------------
assets = registry.get_assets_by_path(ROOT, recursive=True)
by_class = {}
for a in assets:
    by_class.setdefault(str(a.asset_class_path.asset_name), []).append(str(a.asset_name))
unreal.log(f"[MH-inventaris] {ROOT}: {len(assets)} assets")
for cls in sorted(by_class):
    names = by_class[cls]
    preview = ", ".join(sorted(names)[:4]) + ("..." if len(names) > 4 else "")
    unreal.log(f"[MH-inventaris]   {cls}: {len(names)} ({preview})")

# --- 2. Probe: proof before link -------------------------------------------
def probe_mesh(path, label):
    if not eal.does_asset_exist(path):
        unreal.log_error(f"[MH-probe] {label}: asset ontbreekt op {path}")
        return False
    mesh = eal.load_asset(path)
    if not mesh:
        unreal.log_error(f"[MH-probe] {label}: laadt niet ({path})")
        return False
    skel = mesh.get_editor_property("skeleton")
    if not skel:
        unreal.log_error(f"[MH-probe] {label}: skeleton onopgelost — referentieroot klopt niet")
        return False
    unreal.log(f"[MH-probe] {label}: OK — skeleton={skel.get_path_name()}, "
               f"LODs={mesh.get_lod_count() if hasattr(mesh, 'get_lod_count') else '?'}")
    return True

body_ok = probe_mesh(BODY_MESH, "BodyMesh")
face_ok = probe_mesh(FACE_MESH, "FaceMesh")

if not body_ok:
    unreal.log_error("[MH-link] Probe gefaald — DT_NamedCharacters blijft ongewijzigd (bewijsregel).")
    raise SystemExit(0)

# --- 3. Link Kaya slot (provisional) ----------------------------------------
# Same CSV shape as setup_character_data.py (single writer discipline for the
# named table); only Kaya.MetaHumanMesh changes vs. the step-2 fill.
mh_ref = f"{BODY_MESH}.SKM_MHC_Little_00_BodyMesh"
NAMED_CSV = f"""---,DisplayName,MetaHumanMesh,FallbackBodyDef,Faction
Kaya,"Kaya Renn","{mh_ref}",Rebel_B,Rebel
Brick,"Oram \"\"Brick\"\" Bex","",Rebel_A,Rebel
Vale,"Torren Vale","",Rebel_C,Rebel
Dex,"Dex Callum","",Rebel_C,Rebel
Petra,"Petra Voss","",Rebel_B,Rebel
Kaine,"Grand Marshal Sera Kaine","",RadiantGuard,Dominion"""

NC_PATH = "/Game/Data/DT_NamedCharacters"
nc = eal.load_asset(NC_PATH)
if not nc:
    unreal.log_error(f"[MH-link] {NC_PATH} ontbreekt — niets gewijzigd")
    raise SystemExit(0)
ok = unreal.DataTableFunctionLibrary.fill_data_table_from_csv_string(nc, NAMED_CSV)
if ok:
    eal.save_asset(NC_PATH)
    unreal.log(f"[MH-link] Kaya.MetaHumanMesh -> {mh_ref} (provisioneel; fallback Rebel_B intact)")
    if face_ok:
        unreal.log(f"[MH-link] FaceMesh + BP-assembly genoteerd voor de gezichts-tier: {FACE_MESH} | {ROOT}/SentinelC/BP_SentinelC")
else:
    unreal.log_error("[MH-link] CSV-fill MISLUKT — tabel niet opgeslagen")
