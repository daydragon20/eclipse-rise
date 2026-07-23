# Imports the Poly Haven CC0 prop meshes (FBX) + their diffuse maps from
# Saved/PolyHavenStaging/Props into /Game/Art/Props and /Game/Art/Textures.
# Provenance/license: Content/Art/Textures/SOURCES.md. Materials are NOT
# imported - the builder assigns toon-pipeline MIDs (mesh-UV albedo path).
# Re-runnable: existing assets are reimported in place.
#
# Run headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\import_polyhaven_props.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

import os
import unreal

STAGING = os.path.join(
    unreal.SystemLibrary.get_project_saved_directory(), "PolyHavenStaging", "Props"
)

if not os.path.isdir(STAGING):
    raise RuntimeError(f"staging dir missing: {STAGING} (download step not run)")

tasks = []
for name in sorted(os.listdir(STAGING)):
    path = os.path.join(STAGING, name)
    lower = name.lower()
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", path)
    task.set_editor_property("destination_name", os.path.splitext(name)[0])
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    if lower.endswith(".fbx"):
        ui = unreal.FbxImportUI()
        ui.set_editor_property("import_mesh", True)
        ui.set_editor_property("import_as_skeletal", False)
        ui.set_editor_property("import_animations", False)
        ui.set_editor_property("import_materials", False)
        ui.set_editor_property("import_textures", False)
        smid = ui.get_editor_property("static_mesh_import_data")
        smid.set_editor_property("combine_meshes", True)
        smid.set_editor_property("auto_generate_collision", True)
        task.set_editor_property("options", ui)
        task.set_editor_property("destination_path", "/Game/Art/Props")
    elif lower.endswith((".png", ".jpg")):
        task.set_editor_property("destination_path", "/Game/Art/Textures")
    else:
        continue
    tasks.append(task)

if not tasks:
    raise RuntimeError(f"nothing to import in {STAGING}")

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for task in tasks:
    for path in task.get_editor_property("imported_object_paths"):
        unreal.log(f"imported: {path}")
