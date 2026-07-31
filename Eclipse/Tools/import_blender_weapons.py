# Importeert de zelfgeauthorde wapenmeshes (Tools/blender/gen_weapons.py) naar
# /Game/Art/Weapons, en MEET wat er binnenkwam.
#
# Draaien:
#   UnrealEditor-Cmd <project> -run=pythonscript \
#     -script="C:\Dev\ECLIPSE_GDD\Eclipse\Tools\import_blender_weapons.py" \
#     -unattended -nopause -nosplash -NoLiveCoding
#
# ------------------------------------------------------------------ DE METING
# Dit script doet er iets bij dat import_blender_props.py niet doet, en dat is
# geen netheid maar de falsificatie van stap 2 uit REFERENTIE_TPS.md hoofdstuk 4:
#
#   "Falsificatie: het asset bestaat, laadt, en heeft een gemeten bounding box in
#    de orde van een geweer — geen leeg asset dat 'bestaat'."
#
# Een geslaagde import zegt niets over de MAAT. Een FBX met een verkeerde
# eenheid-instelling importeert net zo groen en levert een geweer van 95 meter of
# van 9,5 millimeter op. Daarom drukt dit script per asset de bounding box af in
# CENTIMETERS (UE-eenheden) en zegt het er expliciet bij of die in de orde van een
# wapen ligt. Blender meldde 0,949 / 0,641 / 1,229 / 0,339 m; hier hoort dus
# 94,9 / 64,1 / 122,9 / 33,9 cm te staan.
#
# EN WELKE AS DE LOOP IS. De Blender-kant bouwt de loop langs +X, maar de
# FBX-asconventie tussen Blender en UE is precies het soort ding dat je moet
# NAKIJKEN in plaats van aannemen. De lengte-as van een geweer is ondubbelzinnig
# (lengte >> breedte en hoogte), dus deze meting kan niet twee antwoorden geven —
# en de C++-kant kan zijn greeprotatie erop baseren in plaats van erop gokken.

import os
import unreal

STAGING = os.path.join(unreal.SystemLibrary.get_project_saved_directory(), "BlenderWeapons")
DEST = "/Game/Art/Weapons"

if not os.path.isdir(STAGING):
    raise RuntimeError(
        "geen %s — draai eerst Tools/blender/gen_weapons.py" % STAGING)

tasks = []
for name in sorted(os.listdir(STAGING)):
    if not name.lower().endswith(".fbx"):
        continue
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", False)
    ui.set_editor_property("import_animations", False)
    # GEEN materialen en GEEN texturen uit de FBX. Het wapen gaat door de
    # toon-master (15.5) met de gunmetal-waarde die het lichaam afleidt; een
    # meegeimporteerd standaardmateriaal zou daar alleen overheen liggen en in het
    # unlit district te fel staan.
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    smid = ui.get_editor_property("static_mesh_import_data")
    # EEN asset per FBX: elk bestand is al precies een wapen.
    smid.set_editor_property("combine_meshes", True)
    # GEEN collision. Het wapen hangt aan een hand en mag nergens tegenaan botsen;
    # een auto-collision zou de speler laten haken achter deurposten.
    smid.set_editor_property("auto_generate_collision", False)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(STAGING, name))
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", ui)
    tasks.append(task)

if not tasks:
    raise RuntimeError("geen FBX in %s" % STAGING)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

# ------------------------------------------------------------------ nameten
# Een geweer is 60-130 cm, een pistool 20-40. Buiten die orde is het een
# importschaal-fout en geen wapen — en dan hoort dit script ROOD te worden in
# plaats van "4 assets geimporteerd" te melden.
LIMITS = {
    "SM_Weapon_AR_Foundry": (60.0, 130.0),
    "SM_Weapon_SMG_Patch": (40.0, 100.0),
    "SM_Weapon_DMR_Longsight": (80.0, 160.0),
    "SM_Weapon_Sidearm_Scrap": (18.0, 50.0),
}

failures = []
print("=" * 78)
for task in tasks:
    for path in task.get_editor_property("imported_object_paths"):
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if not isinstance(asset, unreal.StaticMesh):
            continue
        name = asset.get_name()
        lo, hi = asset.get_bounding_box().min, asset.get_bounding_box().max
        size = (hi.x - lo.x, hi.y - lo.y, hi.z - lo.z)
        longest = max(size)
        axis = "XYZ"[size.index(longest)]
        tris = asset.get_num_triangles(0)
        low, high = LIMITS.get(name, (10.0, 200.0))
        ok = low <= longest <= high
        print("%-28s %6.1f x %5.1f x %5.1f cm | langste as %s = %6.1f cm | %5d tris | %s"
              % (name, size[0], size[1], size[2], axis, longest, tris,
                 "OK" if ok else "BUITEN DE ORDE (%.0f-%.0f)" % (low, high)))
        # De GREEP hoort op de oorsprong te liggen: dat is de conventie waar de
        # C++-aanhechting op staat. Ligt de bbox volledig aan een kant, dan is de
        # pivot verschoven en hangt het wapen straks naast de hand.
        print("    pivot: bbox loopt X[%.1f..%.1f] Y[%.1f..%.1f] Z[%.1f..%.1f] cm — "
              "de oorsprong hoort IN de greep te liggen"
              % (lo.x, hi.x, lo.y, hi.y, lo.z, hi.z))
        if not ok:
            failures.append(name)
print("=" * 78)

if failures:
    raise RuntimeError("wapens buiten de orde van een wapen: %s" % ", ".join(failures))
print("wapens geimporteerd in %s" % DEST)
