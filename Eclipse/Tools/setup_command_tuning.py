# SPEC-P2-02 data fill: creates /Game/Data/DA_CommandModeTuning
# (UEclipseCommandModeTuningAsset) so the Command Mode numbers are owner-
# editable data (GDD 14.2). CREATE-ONLY: an existing asset is never touched —
# the owner may have tuned it (dialogue-seed principle), and the C++ defaults
# already mirror the locked SPEC-P2-02 numbers (tested by
# Eclipse.Command.TuningDefaultsMatchLockedSpec), so a fresh asset needs no
# property writes. Queue this in the commandlet slot; the component degrades
# gracefully until it runs (GDD 14.3.5).
#
# Run headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\setup_command_tuning.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

import unreal

DATA_PATH = "/Game/Data"
ASSET_NAME = "DA_CommandModeTuning"

eal = unreal.EditorAssetLibrary
path = f"{DATA_PATH}/{ASSET_NAME}"

if eal.does_asset_exist(path):
    unreal.log(f"{ASSET_NAME} exists - leaving owner-tunable values untouched (create-only).")
else:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.EclipseCommandModeTuningAsset)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        ASSET_NAME, DATA_PATH, None, factory)
    if asset is None:
        raise RuntimeError(f"{ASSET_NAME} creation failed")
    eal.save_loaded_asset(asset)
    unreal.log(f"{ASSET_NAME} created with the locked SPEC-P2-02 defaults (0.30 dilation, pause-ready, 15% pullback).")

unreal.log("SPEC-P2-02 command tuning data complete.")
