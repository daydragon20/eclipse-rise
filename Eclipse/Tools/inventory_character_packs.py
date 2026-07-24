# Inventories the Fab character packs that just landed in /Game: per pack the
# skeletal meshes, skeletons, animation sequences/blueprints, and static meshes.
# Read-only; prints a compact report for the character-pipeline plan (step 2).
#
# Run headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\inventory_character_packs.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

import unreal

PACKS = [
    "SciFiSoldier", "SciFiSoldier02", "SciFiSoldier03", "SciFiGirl",
    "SciFiCharacter", "SciFiCharacterPack", "SciFiWarrior", "SciFiWarrior02",
    # ParagonMinions left disk in the pack-slim round (2026-07-24) — its
    # curation accepts live on under /Game/Art/Imported (no skeletal payload).
    "ParagonLtBelica", "ParagonTwinblast",
]

registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.wait_for_completion()

for pack in PACKS:
    assets = registry.get_assets_by_path(f"/Game/{pack}", recursive=True)
    if not assets:
        unreal.log(f"PACK {pack}: NIETS GEVONDEN")
        continue
    by_class = {}
    skeletons = set()
    skel_meshes = []
    anim_bps = []
    for a in assets:
        cls = str(a.asset_class_path.asset_name)
        by_class[cls] = by_class.get(cls, 0) + 1
        if cls == "Skeleton":
            skeletons.add(str(a.asset_name))
        elif cls == "SkeletalMesh":
            skel_meshes.append(str(a.asset_name))
        elif cls == "AnimBlueprint":
            anim_bps.append(str(a.asset_name))
    counts = ", ".join(f"{k}={v}" for k, v in sorted(by_class.items()) if v > 0 and k in (
        "SkeletalMesh", "Skeleton", "AnimSequence", "AnimBlueprint", "AnimMontage",
        "BlendSpace", "StaticMesh", "Material", "MaterialInstanceConstant", "Texture2D", "PhysicsAsset"))
    unreal.log(f"PACK {pack}: {counts}")
    unreal.log(f"  skeletons: {sorted(skeletons)}")
    unreal.log(f"  skelmeshes({len(skel_meshes)}): {sorted(skel_meshes)[:10]}")
    if anim_bps:
        unreal.log(f"  animBPs: {sorted(anim_bps)[:6]}")
