# Curatie-inventaris van de vier env-packs die op 2026-07-25 binnenkwamen.
# REPORT-ONLY: raakt geen enkel asset aan, schrijft alleen naar Saved/.
#
# Waarom dit script bestaat: de curatiecriteria die écht beslissen (tri-count,
# materiaal-slots, textuur-resolutie) staan in de asset-registry-tags en zijn dus
# alleen in de editor leesbaar. Bestandsgrootte is géén proxy voor tri-count.
# Zelfde patroon als Tools/inventory_curation_pass.py fase A, maar toegespitst op
# de nieuwe packs en met een expliciete GESCHIKTHEIDS-oordeel per mesh, zodat de
# accept-lijst uit data volgt en niet uit een gevoel.
#
# Beslisregels (12.4-budget + 15.5 toon-pijplijn):
#   - tri-count: <= 6000 = prima dressing; 6000-20000 = alleen als hero-prop;
#     > 20000 = afwijzen op deze tier (het district draait op een SM5-laptop).
#   - materiaal-slots: >= 2 is gewenst, want de toon-master tint PER ZONE. Een
#     mesh met 1 slot voor alles kan geen palet-scheiding dragen.
#   - textuur >= 4K wordt gemarkeerd: bruikbaar als luminantie-albedo, maar de
#     gain MOET gemeten worden (Tools/measure_albedo_gain.py), nooit geschat.
#
# Run headless (editor DICHT):
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\inventory_envpacks_2026_07_25.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

import json
import os

import unreal

SAVED = unreal.SystemLibrary.get_project_saved_directory()
OUT_DIR = os.path.join(SAVED, "CurationStaging")
os.makedirs(OUT_DIR, exist_ok=True)

ROOTS = [
    "/Game/Uniblocks",
    "/Game/Factory_Pack_V1",
    "/Game/Sci_fi_hallway",
    "/Game/IBuilding_49",
]

TRI_DRESSING_MAX = 6000
TRI_HERO_MAX = 20000

registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.wait_for_completion()


def as_int(value, default=0):
    try:
        return int(str(value).strip())
    except (TypeError, ValueError):
        return default


report = {}
for root in ROOTS:
    assets = registry.get_assets_by_path(root, recursive=True)
    counts = {}
    meshes = []
    textures = []
    for a in assets:
        cls = str(a.asset_class_path.asset_name)
        counts[cls] = counts.get(cls, 0) + 1
        if cls == "StaticMesh":
            tris = as_int(a.get_tag_value("Triangles"))
            slots = as_int(a.get_tag_value("Materials"))
            verdict = (
                "dressing" if 0 < tris <= TRI_DRESSING_MAX
                else "hero-only" if tris <= TRI_HERO_MAX
                else "te zwaar" if tris > TRI_HERO_MAX
                else "onbekend"
            )
            if slots == 1 and verdict != "te zwaar":
                verdict += " / 1-slot (geen palet-scheiding)"
            meshes.append({
                "path": str(a.package_name),
                "tris": tris,
                "slots": slots,
                "lods": as_int(a.get_tag_value("LODs")),
                "verdict": verdict,
            })
        elif cls == "Texture2D":
            dims = str(a.get_tag_value("Dimensions") or "")
            textures.append({"path": str(a.package_name), "dims": dims})

    meshes.sort(key=lambda m: -m["tris"])
    buckets = {"dressing": 0, "hero-only": 0, "te zwaar": 0, "onbekend": 0}
    one_slot = 0
    for m in meshes:
        for key in buckets:
            if m["verdict"].startswith(key):
                buckets[key] += 1
                break
        if m["slots"] == 1:
            one_slot += 1

    big_tex = [t for t in textures if "4096" in t["dims"] or "8192" in t["dims"]]

    report[root] = {
        "counts": counts,
        "mesh_total": len(meshes),
        "buckets": buckets,
        "one_slot_meshes": one_slot,
        "textures_total": len(textures),
        "textures_4k_plus": len(big_tex),
        "heaviest": meshes[:12],
        "lightest_usable": [m for m in meshes if m["verdict"].startswith("dressing")][-12:],
    }

    unreal.log(
        f"CURATIE {root}: {len(meshes)} meshes | dressing={buckets['dressing']} "
        f"hero={buckets['hero-only']} te-zwaar={buckets['te zwaar']} "
        f"| 1-slot={one_slot} | textures={len(textures)} (4K+: {len(big_tex)})"
    )
    if meshes:
        unreal.log(f"CURATIE   zwaarste: {meshes[0]['path']} = {meshes[0]['tris']} tris, {meshes[0]['slots']} slots")

out = os.path.join(OUT_DIR, "envpacks_2026_07_25.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump(report, f, indent=1)
unreal.log(f"CURATIE rapport geschreven: {out}")
unreal.log("CURATIE klaar - report-only, geen asset aangeraakt.")
