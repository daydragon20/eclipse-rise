# MetaHuman wave 2 (Frey/Hannah/Mason + overige Fab-arrivals): inventory + probe
# REPORT ONLY — schrijft niets. De slot-link volgt het SentinelC-precedent
# (setup_metahuman_data.py): eerst bewijs uit de asset zelf (skeleton-pad =
# gender/height/weight, groom-namen = kapsel), dan pas een aparte link-run die
# DT_NamedCharacters herschrijft (single-writer, fallback-bodies intact,
# GDD 14.3.5). Mapping-kandidaten per phase0/metahuman_recipes.md; de match
# beslist de main-agent op deze output — nooit dit script.
#
# Run headless (commandlet-slot, editor DICHT):
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\inventory_metahumans_wave2.py" \
#     -unattended -nopause -nosplash -nullrhi

import unreal

registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.wait_for_completion()
eal = unreal.EditorAssetLibrary

# Fab-plugin landt instances onder /Game/Fab/MetaHuman; de assembly-output van
# de MetaHuman-plugin kan daarnaast onder /Game/MetaHumans/<Naam> staan.
ROOTS = ["/Game/Fab/MetaHuman", "/Game/MetaHumans"]

for root in ROOTS:
    assets = registry.get_assets_by_path(root, recursive=True)
    unreal.log(f"[MH2-inventaris] {root}: {len(assets)} assets")
    by_class = {}
    for a in assets:
        by_class.setdefault(str(a.asset_class_path.asset_name), []).append(str(a.package_name))
    for cls in sorted(by_class):
        names = by_class[cls]
        preview = ", ".join(sorted(names)[:4]) + ("..." if len(names) > 4 else "")
        unreal.log(f"[MH2-inventaris]   {cls}: {len(names)} ({preview})")

    # Probe elke skeletal mesh: laadt hij, en welk skeleton draagt hij? Het
    # skeleton-pad is het gender/bouw-bewijs (Common/<Gender>/<Height>/<Weight>).
    for path in sorted(by_class.get("SkeletalMesh", [])):
        obj_path = f"{path}.{path.rsplit('/', 1)[-1]}"
        mesh = eal.load_asset(obj_path) if eal.does_asset_exist(obj_path) else None
        if mesh is None:
            unreal.log_warning(f"[MH2-probe] {obj_path}: bestaat/laadt niet")
            continue
        skel = mesh.get_editor_property("skeleton")
        unreal.log(f"[MH2-probe] {obj_path}: skeleton={skel.get_path_name() if skel else 'ONOPGELOST'}")

    # Grooms = kapsel-bewijs voor de recept-match (vgl. Hair_S_AfroFade->Kaya).
    for path in sorted(by_class.get("GroomAsset", []) + by_class.get("GroomBindingAsset", [])):
        unreal.log(f"[MH2-groom] {path}")

unreal.log("[MH2] Klaar — rapport hierboven; link-run volgt apart na de match-beslissing.")
