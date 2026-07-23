# Imports the Quaternius "Ultimate Animated Character Pack" FBX characters
# (CC0, https://quaternius.com - downloaded via the pack's public Drive folder)
# as skeletal meshes + animation takes into /Game/Art/Characters.
# Provenance: Content/Art/Textures/SOURCES.md. Re-runnable (replace_existing).
#
# Run headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\import_quaternius_characters.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

import os
import unreal

STAGING = os.path.join(
    unreal.SystemLibrary.get_project_saved_directory(),
    "QuaterniusStaging", "Ultimate Animated Character Pack - Nov 2019", "FBX"
)
PICKS = ["BlueSoldier_Male", "BlueSoldier_Female", "Casual_Bald", "Casual2_Male"]

if not os.path.isdir(STAGING):
    raise RuntimeError(f"staging dir missing: {STAGING}")

tasks = []
for name in PICKS:
    path = os.path.join(STAGING, f"{name}.fbx")
    if not os.path.isfile(path):
        unreal.log_warning(f"skip {name}: file missing")
        continue
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("import_animations", True)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", path)
    task.set_editor_property("destination_path", f"/Game/Art/Characters/{name}")
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", ui)
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for task in tasks:
    for path in task.get_editor_property("imported_object_paths"):
        unreal.log(f"imported: {path}")
