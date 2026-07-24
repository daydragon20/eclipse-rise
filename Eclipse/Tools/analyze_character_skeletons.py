# Prints bone count + first bones per character-pack skeleton to judge
# retarget compatibility (step 2 of the character pipeline). Read-only.
#
# Run headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\analyze_character_skeletons.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

import unreal

registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.wait_for_completion()

for pack in ["SciFiSoldier", "SciFiSoldier02", "SciFiSoldier03", "SciFiGirl",
             "SciFiCharacter", "SciFiCharacterPack", "SciFiWarrior", "SciFiWarrior02",
             "ParagonLtBelica"]:  # ParagonMinions: dropped in the pack-slim round (2026-07-24)
    for a in registry.get_assets_by_path(f"/Game/{pack}", recursive=True):
        if str(a.asset_class_path.asset_name) != "Skeleton":
            continue
        skel = a.get_asset()
        if skel is None:
            continue
        # Bone info via the reference skeleton through a helper: count only.
        try:
            bones = unreal.EclipseUnavailable  # placeholder, replaced below
        except AttributeError:
            bones = None
        # Python API: Skeleton has no direct bone list; use the editor subsystem.
        names = []
        try:
            names = [str(n) for n in unreal.SkeletonEditorLibrary.get_bone_names(skel)]
        except Exception:
            try:
                names = [str(n) for n in skel.get_editor_property("bone_tree")]
            except Exception:
                names = []
        unreal.log(f"SKEL {pack}/{a.asset_name}: bones={len(names)} first={names[:6]}")
