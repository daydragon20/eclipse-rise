# Importeert de uit Lyra geexporteerde wapenmeshes in ECLIPSE, en zet ze daarbij
# om naar DE conventie van dit project.
#
# Draaien TEGEN ECLIPSE (na Tools/export_lyra_weapons.py tegen Lyra):
#   UnrealEditor-Cmd <Eclipse.uproject> -run=pythonscript \
#     -script="C:\Dev\ECLIPSE_GDD\Eclipse\Tools\import_lyra_weapons.py" \
#     -unattended -nopause -nosplash -NoLiveCoding
#
# ----------------------------------------------------- WAAROM EEN DRAAIING NODIG IS
# GEMETEN bij de export: Lyra legt zijn wapens langs **+Y** (geweer 97,0 cm op Y,
# pistool 24,3 cm op Y). ECLIPSE legt ze langs **+X** — dat is de conventie die
# gen_weapons.py hanteert en waar de greeprotatie in DT_BodyDefs op staat.
#
# Dat is precies het punt uit de greeprotatie-diagnose van 31-07, nu van de andere
# kant: elk ingekocht asset brengt zijn EIGEN as-conventie mee. Er zijn twee
# plekken waar je dat kunt rechtzetten, en maar een daarvan schaalt:
#
#   - per wapen een correctie in de DATA  -> dan draagt elk wapen zijn eigen
#     uitzondering mee, en de volgende inkoop voegt er weer een toe;
#   - EEN KEER bij de IMPORT             -> dan heeft het project precies een
#     conventie, en de test bewaakt hem voor alles wat er ooit binnenkomt.
#
# Het tweede. `Eclipse.Combat.WapenMeshPerFamilie` eist dat de langste as X is;
# die test is dus niet alleen een controle op mijn eigen Blender-meshes maar de
# poort waar elk toekomstig ingekocht wapen doorheen moet.
#
# Yaw -90 beeldt +Y af op +X. De loop van het Lyra-geweer ligt aan de LANGE kant
# (Y van -44,8 tot +52,3), dus na de draaiing wijst hij naar +X — voorwaarts.
#
# ------------------------------------------------------------- DE FICTIEWET (20.2)
# De Lyra-materialen komen NIET mee (import_materials=False). Dat is geen
# bezuiniging maar de reden dat deze assets de fictiewet halen: de aardse
# semiotiek van zo'n pack zit in de diffuse (T_Rifle_D, T_Rifle_Masks), en de
# weapon-MID in AttachWeaponMesh zet bewust GEEN AlbedoTex. Wat er op het scherm
# komt is silhouet plus gunmetal-waarde, door de toon-master.
#
# Wat dit NIET afvangt is de VORM. Dat oordeel hoort bij een mens en bij een
# frame, niet bij dit script.

import os
import shutil
import unreal

STAGING = r"C:\Dev\ECLIPSE_GDD\Eclipse\Saved\LyraWeapons"
DEST = "/Game/Art/Weapons"

# Welk Lyra-asset welke ECLIPSE-wapenfamilie wordt. De rijnaam bepaalt de
# assetnaam, want RefreshWeaponVisual lost op via /Game/Art/Weapons/SM_Weapon_<rij>.
#
# SMG_Patch en DMR_Longsight staan hier NIET: Lyra heeft geen SMG en geen
# scherpschutter. Die twee houden hun zelfgeauthorde mesh, en dat is precies de
# rol die de Blender-pijplijn houdt — terugval voor families waarvoor niets ligt.
MAPPING = {
    "SM_Rifle":   "SM_Weapon_AR_Foundry",
    "SM_Pistol":  "SM_Weapon_Sidearm_Scrap",
    # SM_Shotgun wordt WEL binnengehaald maar aan geen enkele familie gekoppeld:
    # er is nog geen hagelwapen in DT_Weapons. PelletsPerShot staat er al als
    # ongelezen veld, dus de dag dat die familie komt, ligt de mesh er.
    "SM_Shotgun": "SM_Weapon_Shotgun_Unassigned",
}

LIMITS = {
    "SM_Weapon_AR_Foundry": (60.0, 130.0),
    "SM_Weapon_Sidearm_Scrap": (18.0, 50.0),
    "SM_Weapon_Shotgun_Unassigned": (50.0, 120.0),
}

if not os.path.isdir(STAGING):
    raise RuntimeError("geen %s — draai eerst export_lyra_weapons.py tegen Lyra" % STAGING)

# Hernoemen VOOR de import: de bestandsnaam bepaalt de assetnaam.
renamed = []
for src_name, dest_name in MAPPING.items():
    src = os.path.join(STAGING, src_name + ".fbx")
    if not os.path.isfile(src):
        unreal.log_warning("ontbreekt in staging: %s" % src)
        continue
    dst = os.path.join(STAGING, dest_name + ".fbx")
    shutil.copyfile(src, dst)
    renamed.append((dst, dest_name))

if not renamed:
    raise RuntimeError("niets te importeren")

tasks = []
for path, name in renamed:
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", False)
    ui.set_editor_property("import_animations", False)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    smid = ui.get_editor_property("static_mesh_import_data")
    smid.set_editor_property("combine_meshes", True)
    smid.set_editor_property("auto_generate_collision", False)
    # DE OMZETTING NAAR DE PROJECTCONVENTIE: +Y (Lyra) -> +X (ECLIPSE).
    smid.set_editor_property("import_rotation", unreal.Rotator(0.0, 0.0, -90.0))
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", path)
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", ui)
    tasks.append((task, name))

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t for t, _ in tasks])

print("=" * 100)
failures = []
for task, name in tasks:
    for path in task.get_editor_property("imported_object_paths"):
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if not isinstance(asset, unreal.StaticMesh):
            continue
        box = asset.get_bounding_box()
        size = (box.max.x - box.min.x, box.max.y - box.min.y, box.max.z - box.min.z)
        longest = max(size)
        axis = "XYZ"[size.index(longest)]
        tris = asset.get_num_triangles(0)
        low, high = LIMITS.get(asset.get_name(), (10.0, 200.0))
        maat_ok = low <= longest <= high
        as_ok = axis == "X"
        print("%-30s %6.1f x %5.1f x %5.1f cm | langste as %s = %6.1f | %5d tris | maat %s | as %s"
              % (asset.get_name(), size[0], size[1], size[2], axis, longest, tris,
                 "OK" if maat_ok else "BUITEN DE ORDE", "OK" if as_ok else "FOUT (moet X zijn)"))
        print("    pivot: X[%.1f..%.1f] Y[%.1f..%.1f] Z[%.1f..%.1f] — de oorsprong hoort IN de greep te liggen"
              % (box.min.x, box.max.x, box.min.y, box.max.y, box.min.z, box.max.z))
        if not (maat_ok and as_ok):
            failures.append(asset.get_name())
print("=" * 100)
if failures:
    raise RuntimeError("wapens halen de conventie niet: %s" % ", ".join(failures))
print("Lyra-wapens geimporteerd en op de ECLIPSE-conventie gezet.")
