# Imports the Quaternius Ultimate Modular Men/Women civilian picks (CC0,
# https://quaternius.com - Men via Drive/gdown, Women via de poly.pizza-
# FBX-bundle, 2026-07-25) as skeletal meshes + animation takes into
# /Game/Art/Characters. Zusje van import_quaternius_characters.py
# (2019-pack); zelfde conventies, re-runnable.
#
# NIET draaien terwijl de owner-editor open is (interactieve sessie 2026-07-25).
# Landing: kopieer dit script naar Eclipse/Tools/ en voeg de SOURCES.md-regel toe.
#
# Run headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\import_modular_civilians.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

import os
import unreal

SAVED = unreal.SystemLibrary.get_project_saved_directory()
MEN = os.path.join(SAVED, "QuaterniusStaging", "UltimateModularMen",
                   "Individual Characters", "FBX")
WOMEN = os.path.join(SAVED, "QuaterniusStaging", "UltimateModularWomen")

# Civilian/worker-selectie (owner-doel: 4-6 close-up-waardige Kessara-burgers;
# dit levert 6 M + 5 V). Vrouwen krijgen _F-suffix (naamboting Worker/Suit/
# Punk). Off-direction blijft achter: King/Spacesuit/Swat/Adventurer/Beach (M),
# Medieval/Witch/SciFi/Soldier/Adventurer (V).
PICKS = [
    ("Worker_M",       os.path.join(MEN, "Worker.fbx")),
    ("Farmer_M",       os.path.join(MEN, "Farmer.fbx")),
    ("Casual_Hoodie",  os.path.join(MEN, "Casual_Hoodie.fbx")),
    ("Casual_2",       os.path.join(MEN, "Casual_2.fbx")),
    ("Suit_M",         os.path.join(MEN, "Suit.fbx")),
    ("Punk_M",         os.path.join(MEN, "Punk.fbx")),
    ("Worker_F",       os.path.join(WOMEN, "Worker", "Worker.fbx")),
    ("Casual_F",       os.path.join(WOMEN, "Animated Woman", "Casual.fbx")),
    ("Formal_F",       os.path.join(WOMEN, "Animated Woman-nIItLV9nxS", "Formal.fbx")),
    ("Suit_F",         os.path.join(WOMEN, "Suit", "Suit.fbx")),
    ("Punk_F",         os.path.join(WOMEN, "Punk", "Punk.fbx")),
]

tasks = []
for name, path in PICKS:
    if not os.path.isfile(path):
        unreal.log_warning(f"skip {name}: file missing ({path})")
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
