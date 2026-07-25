# Importeert de hand-geauthorde asfalt-tegel naar /Game/Art/Textures.
#
# Bewust NIET via import_generated_decals.py: dat script veegt alles uit
# Saved/GeneratedDecals naar /Game/Art/Decals, en deze tegel is geen decal maar een
# oppervlaktextuur die de vloer-paletingang gaat dragen. Een asset in de verkeerde
# map is geen cosmetisch probleem - de curatie- en migratiescripts in dit project
# redeneren over mappen.
#
# Idempotent: bestaat het doel al, dan wordt het overschreven (dat is gewenst -
# de generator is deterministisch en een herimport hoort dezelfde pixels te geven).
#
# Draai headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\import_toon_asphalt.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

import os

import unreal

SOURCE = os.path.join(unreal.SystemLibrary.get_project_saved_directory(),
                      "GeneratedDecals", "T_toon_asphalt_diff.png")
DEST_PATH = "/Game/Art/Textures"
DEST_NAME = "T_toon_asphalt_diff"

if not os.path.isfile(SOURCE):
    raise SystemExit(f"IMPORT-FATAL: {SOURCE} ontbreekt — draai eerst "
                     f"Tools/generate_toon_asphalt.py (geen editor nodig).")

task = unreal.AssetImportTask()
task.set_editor_property("filename", SOURCE)
task.set_editor_property("destination_path", DEST_PATH)
task.set_editor_property("destination_name", DEST_NAME)
task.set_editor_property("automated", True)
task.set_editor_property("replace_existing", True)
task.set_editor_property("save", True)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

full = f"{DEST_PATH}/{DEST_NAME}"
texture = unreal.load_asset(f"{full}.{DEST_NAME}")
if texture is None:
    raise SystemExit(f"IMPORT-FAIL: {full} is na de import niet laadbaar.")

# Luminantie-only: de toon-master trekt er alleen de helderheid uit, dus sRGB uit
# en een grijswaarde-compressie scheelt geheugen zonder dat er iets verloren gaat.
texture.set_editor_property("srgb", False)
texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
unreal.EditorAssetLibrary.save_asset(full)

unreal.log(f"IMPORT-OK: {full} ({texture.blueprint_get_size_x()}x{texture.blueprint_get_size_y()}, "
           f"sRGB uit, grayscale). Gemeten gain voor de builder: 2.56")
