# Report-only: welk skelet draagt elke DT_BodyDefs-rij, en welke animaties zijn
# voor DAT skelet geauthord?
#
# Aanleiding: de locomotie-laag landde groen en degradeerde vervolgens luid op
# elke body — "authored for skeleton /Game/SciFiCharacterPack/SciFiGirl/.../
# UE4_Mannequin_Skeleton but this body wears /Game/SciFiSoldier/.../
# UE4_Mannequin_Skeleton". Zelfde skeletNAAM, ander PAD: de UE4-Mannequin staat
# meerdere keren in het project omdat meerdere packs hun eigen kopie meebrachten,
# en een AnimSequence hoort bij precies één skelet-asset, niet bij een naam.
#
# Dit script raadt niets: het leest per body-rij het echte skelet van de mesh en
# telt hoeveel AnimSequences er per skelet bestaan, zodat de koppeling op feiten
# kan worden gelegd in plaats van op gelijkende namen.
#
# Draai headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\inspect_body_skeletons.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

import json
import os

import unreal

BODY_TABLE = "/Game/Data/DT_BodyDefs"
REPORT = os.path.join(unreal.SystemLibrary.get_project_saved_directory(),
                      "CurationStaging", "body_skeletons.json")

registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.wait_for_completion()

# Alle AnimSequences per skelet, uit de registry-tag (geen asset-load nodig).
anims_by_skeleton = {}
for data in registry.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "AnimSequence"), True):
    skeleton = str(data.get_tag_value("Skeleton") or "")
    anims_by_skeleton.setdefault(skeleton, []).append(str(data.asset_name))

unreal.log(f"skeletten met animaties: {len(anims_by_skeleton)}")
for skeleton, names in sorted(anims_by_skeleton.items(), key=lambda kv: -len(kv[1])):
    if not skeleton:
        continue
    unreal.log(f"  {len(names):4d} clips  {skeleton}")

# Wat draagt elke body-rij echt?
rows = []
table = unreal.load_asset(BODY_TABLE)
if table is None:
    unreal.log_error(f"{BODY_TABLE} niet gevonden — geen body-rapport.")
else:
    for row_name in unreal.DataTableFunctionLibrary.get_data_table_row_names(table):
        found, row = unreal.DataTableFunctionLibrary.get_data_table_row_from_name(table, row_name)
        if not found:
            continue
        mesh = row.mesh.load_synchronous() if hasattr(row, "mesh") else None
        skeleton_path = ""
        if mesh is not None:
            skeleton = mesh.skeleton
            if skeleton is not None:
                skeleton_path = str(skeleton.get_path_name())
        clip_count = len(anims_by_skeleton.get(skeleton_path, []))
        rows.append({"row": str(row_name), "skeleton": skeleton_path, "clips_for_that_skeleton": clip_count})
        unreal.log(f"BODY {row_name}: skelet {skeleton_path or 'GEEN MESH'} "
                   f"({clip_count} clips voor dat skelet)")

with open(REPORT, "w", encoding="utf-8") as handle:
    json.dump({"bodies": rows,
               "anims_by_skeleton": {k: len(v) for k, v in anims_by_skeleton.items()}},
              handle, indent=1)
unreal.log(f"rapport: {REPORT}")
