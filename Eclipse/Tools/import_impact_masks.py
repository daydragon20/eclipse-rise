# Importeert de twee inslagmaskers uit Saved/GeneratedImpactMasks naar
# /Game/Art/Decals (generator: Tools/generate_impact_masks.py — herhaalbaar).
#
# EIGEN IMPORTEUR EN GEEN REGEL IN import_generated_decals.py, om precies één
# reden: die leest zijn HELE staging-map in en zet daarmee ook de poster,
# de borden en de vlekken opnieuw neer. Op dit moment werkt een tweede agent aan
# de poster. Een importeur die meer aanraakt dan zijn onderwerp is een botsing die
# wacht op een moment.
#
# Draaien headless:
#   UnrealEditor-Cmd <project> -run=pythonscript \
#     -script="C:\Dev\ECLIPSE_GDD\Eclipse\Tools\import_impact_masks.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash -NoUba

import os
import unreal

STAGING = os.path.join(
    unreal.SystemLibrary.get_project_saved_directory(), "GeneratedImpactMasks"
)

if not os.path.isdir(STAGING):
    raise RuntimeError(
        f"staging ontbreekt: {STAGING} (draai eerst Tools/generate_impact_masks.py)"
    )

tasks = []
for name in sorted(os.listdir(STAGING)):
    if not name.lower().endswith(".png"):
        continue
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(STAGING, name))
    task.set_editor_property("destination_path", "/Game/Art/Decals")
    task.set_editor_property("destination_name", os.path.splitext(name)[0])
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    tasks.append(task)

if not tasks:
    raise RuntimeError(f"geen maskers in {STAGING}")

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for task in tasks:
    for path in task.get_editor_property("imported_object_paths"):
        unreal.log(f"geimporteerd: {path}")
        # Opaciteitsmaskers zijn LINEAIRE data. Met sRGB aan wordt de falloff
        # opnieuw gevormd en is de harde rand die dit masker juist moet leveren
        # weer zacht — dan had de hele ronde geen zin.
        asset = unreal.load_asset(path)
        if asset is not None:
            asset.set_editor_property("srgb", False)
            asset.set_editor_property(
                "compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE
            )
            unreal.EditorAssetLibrary.save_loaded_asset(asset)
            unreal.log(f"masker: srgb uit + grijswaarden ({path})")
