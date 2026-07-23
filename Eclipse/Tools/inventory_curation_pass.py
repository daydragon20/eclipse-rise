# Curation pass over the already-downloaded packs (15.5 style law: everything
# re-enters through the toon master, never raw). Three phases, one launch:
#   A. Inventory the static side (StaticMesh/Material/MI/Texture2D) of the
#      landed packs -> Saved/CurationStaging/inventory.json (registry tags:
#      triangles, approx size, texture dimensions) for the curation doc.
#   B. Import the ambientCG CC0 Color maps staged by Tools/fetch_cc0.py
#      (Saved/CC0Staging/ambientcg) into /Game/Art/Textures — albedo only,
#      the toon pipeline has no use for PBR normal/roughness sets.
#   C. Export the tileable albedo candidates (SciFi_Materials_10 BaseColor
#      1..10 + the Fab Megascans asphalt B map) to Saved/CurationStaging/raw
#      so the Pillow side (Tools/measure_albedo_gain.py) can measure the
#      linear average -> AlbedoGain = 1/mean (same discipline as the
#      System.Drawing pass of 2026-07-22, EclipseGrayboxBuilder palette).
#
# Read-only towards the packs; writes only /Game/Art/Textures + Saved/. Run:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\inventory_curation_pass.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

import json
import os
import unreal

SAVED = unreal.SystemLibrary.get_project_saved_directory()
OUT_DIR = os.path.join(SAVED, "CurationStaging")
RAW_DIR = os.path.join(OUT_DIR, "raw")
os.makedirs(RAW_DIR, exist_ok=True)

# ---------------------------------------------------------------- A. inventory
ROOTS = [
    "/Game/ParagonLtBelica",
    "/Game/ParagonTwinblast",
    "/Game/ParagonMinions",
    "/Game/SciFi_Materials_10",
    "/Game/Fab",
    "/Game/Screen_Damage_Indicator",
    "/Game/FD_WarningSigns_V1",
    "/Game/RustyCarsFree",
]
WANTED = ("StaticMesh", "Material", "MaterialInstanceConstant", "Texture2D")

registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.wait_for_completion()

inventory = {}
for root in ROOTS:
    assets = registry.get_assets_by_path(root, recursive=True)
    rows = []
    counts = {}
    for a in assets:
        cls = str(a.asset_class_path.asset_name)
        counts[cls] = counts.get(cls, 0) + 1
        if cls not in WANTED:
            continue
        row = {
            "class": cls,
            "path": str(a.package_name) + "." + str(a.asset_name),
        }
        if cls == "StaticMesh":
            for tag in ("Triangles", "Vertices", "ApproxSize", "Materials", "LODs"):
                val = a.get_tag_value(tag)
                if val:
                    row[tag.lower()] = str(val)
        elif cls == "Texture2D":
            for tag in ("Dimensions", "Format", "SRGB"):
                val = a.get_tag_value(tag)
                if val:
                    row[tag.lower()] = str(val)
        rows.append(row)
    inventory[root] = {"counts": counts, "assets": rows}
    sm = counts.get("StaticMesh", 0)
    unreal.log(f"CURATION {root}: SM={sm} " + ", ".join(
        f"{k}={v}" for k, v in sorted(counts.items()) if k in WANTED))

inv_path = os.path.join(OUT_DIR, "inventory.json")
with open(inv_path, "w", encoding="utf-8") as f:
    json.dump(inventory, f, indent=1)
unreal.log(f"CURATION inventory written: {inv_path}")

# ------------------------------------------------- B. CC0 albedo import (2K/1K)
CC0 = os.path.join(SAVED, "CC0Staging", "ambientcg")
CC0_MAPS = {
    # ambientCG id -> (staged Color map, /Game destination name). Names follow
    # the T_<source>_diff convention of Content/Art/Textures/SOURCES.md.
    "Metal046B": ("Metal046B_2K-JPG_Color.jpg", "T_Metal046B_diff"),
    "Concrete042A": ("Concrete042A_2K-JPG_Color.jpg", "T_Concrete042A_diff"),
    "Metal063": ("Metal063_1K-JPG_Color.jpg", "T_Metal063_diff"),  # 1K on ambientCG free tier
}
tasks = []
for cc0_id, (fname, dest) in sorted(CC0_MAPS.items()):
    src = os.path.join(CC0, cc0_id, fname)
    if not os.path.isfile(src):
        unreal.log_error(f"CC0 staging missing: {src} (fetch_cc0.py not run?)")
        continue
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", src)
    task.set_editor_property("destination_path", "/Game/Art/Textures")
    task.set_editor_property("destination_name", dest)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    tasks.append(task)
if tasks:
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    for task in tasks:
        for path in task.get_editor_property("imported_object_paths"):
            unreal.log(f"CURATION imported: {path}")

# -------------------------------------- C. export albedo candidates (gain pass)
EXPORT = [
    f"/Game/SciFi_Materials_10/Textures/{i}/T_4k_SciFi10_{i}_BaseColor" for i in range(1, 11)
] + [
    "/Game/Fab/Megascans/Surfaces/Asphalt_Surface_rmqlqkp0/High/rmqlqkp0_tier_1/Textures/T_rmqlqkp0_4K_B",
]
for obj_path in EXPORT:
    name = obj_path.rsplit("/", 1)[-1]
    tex = unreal.load_asset(f"{obj_path}.{name}")
    if tex is None:
        unreal.log_error(f"CURATION export candidate missing: {obj_path}")
        continue
    task = unreal.AssetExportTask()
    task.object = tex
    task.filename = os.path.join(RAW_DIR, f"{name}.png")
    task.exporter = unreal.TextureExporterPNG()
    task.automated = True
    task.replace_identical = True
    task.prompt = False
    unreal.Exporter.run_asset_export_task(task)
    unreal.log(f"CURATION exported raw: {task.filename}")

unreal.log("CURATION pass complete.")
