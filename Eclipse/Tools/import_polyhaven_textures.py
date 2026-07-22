# Imports the Poly Haven CC0 diffuse maps from Saved/PolyHavenStaging into
# /Game/Art/Textures (see Content/Art/Textures/SOURCES.md for origin/license).
# Downloaded by the dev session via the public API (https://api.polyhaven.com);
# re-running is idempotent: existing assets are reimported in place.
#
# Run headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\import_polyhaven_textures.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

import os
import unreal

STAGING = os.path.join(
    unreal.SystemLibrary.get_project_saved_directory(), "PolyHavenStaging"
)
DEST = "/Game/Art/Textures"

if not os.path.isdir(STAGING):
    raise RuntimeError(f"staging dir missing: {STAGING} (download step not run)")

tasks = []
for name in sorted(os.listdir(STAGING)):
    if not name.lower().endswith((".jpg", ".png")):
        continue
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(STAGING, name))
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", os.path.splitext(name)[0])
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    tasks.append(task)

if not tasks:
    raise RuntimeError(f"no textures found in {STAGING}")

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for task in tasks:
    for path in task.get_editor_property("imported_object_paths"):
        unreal.log(f"imported: {path}")
