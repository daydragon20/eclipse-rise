# Imports the Pillow-generated decal luminance maps from Saved/GeneratedDecals
# into /Game/Art/Decals (generator: Tools/generate_decals.py - re-runnable).
#
# Run headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\import_generated_decals.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

import os
import unreal

STAGING = os.path.join(
    unreal.SystemLibrary.get_project_saved_directory(), "GeneratedDecals"
)

if not os.path.isdir(STAGING):
    raise RuntimeError(f"staging dir missing: {STAGING} (run Tools/generate_decals.py)")

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
    raise RuntimeError(f"no decals in {STAGING}")

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for task in tasks:
    for path in task.get_editor_property("imported_object_paths"):
        unreal.log(f"imported: {path}")
