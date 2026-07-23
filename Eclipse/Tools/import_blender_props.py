# Imports the self-authored Blender props/kit (Tools/blender/gen_*.py output)
# into /Game/Art/Generated. combine_meshes=False on purpose: the GlowPlane and
# PosterPlane sub-objects must stay separate assets so the builder can give
# them emissive/decal materials while the body gets the metal cel MID.
# Re-runnable (replace_existing).
#
# Run headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\import_blender_props.py" \
#     -unattended -nopause -nosplash

import os
import unreal

SOURCES = [
    os.path.join(unreal.SystemLibrary.get_project_saved_directory(), "BlenderProps"),
    os.path.join(unreal.SystemLibrary.get_project_saved_directory(), "BlenderKit"),
]

tasks = []
for staging in SOURCES:
    if not os.path.isdir(staging):
        unreal.log_warning(f"staging ontbreekt (agent nog bezig?): {staging}")
        continue
    for name in sorted(os.listdir(staging)):
        if not name.lower().endswith(".fbx"):
            continue
        ui = unreal.FbxImportUI()
        ui.set_editor_property("import_mesh", True)
        ui.set_editor_property("import_as_skeletal", False)
        ui.set_editor_property("import_animations", False)
        ui.set_editor_property("import_materials", False)
        ui.set_editor_property("import_textures", False)
        smid = ui.get_editor_property("static_mesh_import_data")
        smid.set_editor_property("combine_meshes", False)
        smid.set_editor_property("auto_generate_collision", True)
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", os.path.join(staging, name))
        task.set_editor_property("destination_path", "/Game/Art/Generated")
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("automated", True)
        task.set_editor_property("save", True)
        task.set_editor_property("options", ui)
        tasks.append(task)

if not tasks:
    raise RuntimeError("geen FBX gevonden in BlenderProps/BlenderKit")

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for task in tasks:
    for path in task.get_editor_property("imported_object_paths"):
        unreal.log(f"imported: {path}")
