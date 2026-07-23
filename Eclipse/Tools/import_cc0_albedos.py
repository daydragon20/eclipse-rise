# Reusable ambientCG CC0 albedo import — the standing follow-up to a
# Tools/fetch_cc0.py staging run (section-B pattern of inventory_curation_pass.py).
# Imports ONLY the Color maps into /Game/Art/Textures as T_<id>_diff: the toon
# pipeline (15.5 — palette owns hue, textures contribute luminance) has no use
# for PBR normal/roughness/metalness sets, so those never enter the project.
#
# replace_existing is deliberate: a re-run — or a resolution upgrade such as
# Metal063 1K -> 2K (2026-07-23) — replaces the asset in place and every
# existing MID reference keeps working.
#
# Gains are NOT set here: measure with Tools/measure_albedo_gain.py (Pillow,
# 1/mean-linear-luminance) and record them in phase0/ASSET_CURATION.md +
# Content/Art/Textures/SOURCES.md before wiring a texture into the builder.
#
# Run headless (editor closed):
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\import_cc0_albedos.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

import os
import unreal

SAVED = unreal.SystemLibrary.get_project_saved_directory()
CC0 = os.path.join(SAVED, "CC0Staging", "ambientcg")

# ambientCG id -> (staged Color map, /Game destination name). Names follow the
# T_<source>_diff convention of Content/Art/Textures/SOURCES.md.
CC0_MAPS = {
    # Grunge replacement (owner decision 2026-07-23): the Fab "Grungy Surface"
    # re-download is scrapped; Metal041B (heavy rust) is the CC0 equivalent.
    "Metal041B": ("Metal041B_2K-JPG_Color.jpg", "T_Metal041B_diff"),
    # Rusty corrugated sheet for the warehouse (BldgB).
    "CorrugatedSteel007A": ("CorrugatedSteel007A_2K-JPG_Color.jpg", "T_CorrugatedSteel007A_diff"),
    # Resolution upgrade: first import was the 1K set; this 2K re-import
    # replaces it under the same asset name.
    "Metal063": ("Metal063_2K-JPG_Color.jpg", "T_Metal063_diff"),
}

# Parked, deliberately NOT imported (recorded in phase0/ASSET_CURATION.md):
#   MetalWalkway014     - grating that ships an Opacity map; not a tileable
#                         albedo, would need a masked-material path first.
#   CorrugatedSteel009  - fetch by-catch; 007A is the accepted rusty sheet.

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
            unreal.log(f"CC0 imported: {path}")
else:
    unreal.log_error("CC0 import: nothing staged — run Tools/fetch_cc0.py first.")

unreal.log("CC0 albedo import complete.")
