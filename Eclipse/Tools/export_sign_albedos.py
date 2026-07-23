# Exports ALL FD_WarningSigns_V1 albedos to Saved/WarningSignStaging/raw so the
# curation pass can judge the remaining 57 signs visually (contact sheet) and
# pick the next placard derivations for prepare_warning_signs.py. Idempotent;
# skips files that already exist. Pack stays machine-local (14.3.5).
#
# Run headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\export_sign_albedos.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

import os
import unreal

RAW = os.path.join(unreal.SystemLibrary.get_project_saved_directory(), "WarningSignStaging", "raw")
os.makedirs(RAW, exist_ok=True)

for index in range(1, 61):
    name = f"T_Albedo_WarningSigns_V1_{index:02d}"
    out = os.path.join(RAW, f"{name}.png")
    if os.path.isfile(out):
        continue
    tex = unreal.load_asset(f"/Game/FD_WarningSigns_V1/Textures/{name}.{name}")
    if tex is None:
        unreal.log_error(f"missing: {name}")
        continue
    task = unreal.AssetExportTask()
    task.object = tex
    task.filename = out
    task.exporter = unreal.TextureExporterPNG()
    task.automated = True
    task.replace_identical = True
    task.prompt = False
    unreal.Exporter.run_asset_export_task(task)
    unreal.log(f"exported: {out}")
