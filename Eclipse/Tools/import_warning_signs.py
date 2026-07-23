# Warning-sign placard pipeline, UE side (15.8). Two-phase, self-selecting:
#   - cleaned placards present in Saved/WarningSignStaging -> import them into
#     /Game/Art/Decals (T_sign_*_diff), the builder's AlbedoTex path;
#   - otherwise -> export the raw FD_WarningSigns_V1 albedos to
#     Saved/WarningSignStaging/raw for Tools/prepare_warning_signs.py (Pillow).
#
# Run headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\import_warning_signs.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash
#
# Provenance: FD_WarningSigns_V1 (Fab free pack, machine-local; the pack is not
# committed — a machine without it keeps the flat-cel fallback, GDD 14.3.5).

import os
import unreal

STAGING = os.path.join(unreal.SystemLibrary.get_project_saved_directory(), "WarningSignStaging")
RAW = os.path.join(STAGING, "raw")

SIGNS = {"09": "stop", "17": "radiation", "30": "toxic"}

cleaned = [f"T_sign_{name}_diff.png" for name in SIGNS.values()]
if all(os.path.isfile(os.path.join(STAGING, f)) for f in cleaned):
    tasks = []
    for name in cleaned:
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", os.path.join(STAGING, name))
        task.set_editor_property("destination_path", "/Game/Art/Decals")
        task.set_editor_property("destination_name", os.path.splitext(name)[0])
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("automated", True)
        task.set_editor_property("save", True)
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    for task in tasks:
        for path in task.get_editor_property("imported_object_paths"):
            unreal.log(f"imported: {path}")
else:
    os.makedirs(RAW, exist_ok=True)
    for index in sorted(SIGNS):
        name = f"T_Albedo_WarningSigns_V1_{index}"
        tex = unreal.load_asset(f"/Game/FD_WarningSigns_V1/Textures/{name}.{name}")
        if tex is None:
            unreal.log_error(f"pack texture missing: {name} (FD_WarningSigns_V1 not on this machine?)")
            continue
        task = unreal.AssetExportTask()
        task.object = tex
        task.filename = os.path.join(RAW, f"{name}.png")
        task.exporter = unreal.TextureExporterPNG()
        task.automated = True
        task.replace_identical = True
        task.prompt = False
        unreal.Exporter.run_asset_export_task(task)
        unreal.log(f"exported raw: {task.filename}")
    unreal.log("now run: python Eclipse/Tools/prepare_warning_signs.py, then this script again.")
