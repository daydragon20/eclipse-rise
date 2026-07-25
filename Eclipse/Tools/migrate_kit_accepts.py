# Kit-pass P2-08: de 12 gemeten Factory-Pack-kandidaten naar repo-tracked
# /Game/Art/Imported (owner-beslissing 2026-07-25: "kit A" — de meshes mogen mét
# hun 4K-textures de repo in).
#
# Zelfde patroon als migrate_curation_accepts.py, en om dezelfde reden:
# duplicate -> consolidate. Consolidate her-richt elke referentie op schijf naar
# het nieuwe asset en laat een redirector achter op het oude pad, zodat het pack
# van schijf kan zonder dat er iets breekt. Rename doet dat alleen "wanneer
# nodig" en is dus niet deterministisch.
#
# Wat dit script BEWUST anders doet dan zijn voorganger:
#  * het migreert ook de TEXTURES waar de meshes van afhangen, want dat is wat
#    "kit A" betekent — zonder textures reist de geometrie kaal mee en valt het
#    district op een andere machine terug op flat cel;
#  * het RAPPORTEERT de bytes die het toevoegt, per asset en als totaal. De
#    owner besliste op de aanname "een veelvoud van 2,4 MB"; als het in de
#    praktijk veel meer is, hoort hij dat te zien in plaats van het te ontdekken
#    in een push.
#
# Materiaalslots gaan naar de toon-master: rauwe pack-materialen renderen nooit
# (15.5), elke plaatsing overschrijft de slots toch met palet-MIDs, en het knipt
# de laatste harde afhankelijkheid naar het pack door.
#
# Idempotent: een kandidaat waarvan de bestemming al bestaat wordt overgeslagen.
#
# Draai headless (editor dicht):
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\migrate_kit_accepts.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

import json
import os

import unreal

TOON_MASTER = "/Game/Art/M_EclipseToon"
DEST_MESHES = "/Game/Art/Imported/Meshes"
DEST_TEXTURES = "/Game/Art/Imported/Textures"
REPORT = os.path.join(unreal.SystemLibrary.get_project_saved_directory(),
                      "CurationStaging", "kit_migration.json")

# De 12 kandidaten uit KITPASS_P2-08 par. 1, gemeten in par. 2a. Namen, geen
# paden: het pack legt ze in verschillende submappen en een gegokt pad is
# precies het soort aanname dat deze pass vermijdt.
CANDIDATES = [
    "SM_Controller_1", "SM_Tank", "SM_pipe_5", "SM_Power_Pole",
    "SM_crane_2", "SM_crane_3", "SM_Tower_9_A", "SM_Tower_11_C",
    "SM_wall_2", "SM_wall_3", "SM_barrel_2", "SM_gas_bottle_packfbx",
]
PACK = "/Game/Factory_Pack_V1"

lib = unreal.EditorAssetLibrary
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.wait_for_completion()

dep_options = unreal.AssetRegistryDependencyOptions(
    include_soft_package_references=True,
    include_hard_package_references=True,
    include_searchable_names=False,
    include_soft_management_references=False,
    include_hard_management_references=False,
)

toon = lib.load_asset(TOON_MASTER)
if toon is None:
    raise RuntimeError(f"KIT-FATAL: toon-master ontbreekt op {TOON_MASTER} — niets gemigreerd (15.5).")

content_dir = unreal.SystemLibrary.get_project_content_directory()


def package_bytes(package_path):
    """Bytes op schijf van een /Game-package, of 0 als het bestand ontbreekt."""
    if not package_path.startswith("/Game/"):
        return 0
    relative = package_path[len("/Game/"):] + ".uasset"
    full = os.path.join(content_dir, relative.replace("/", os.sep))
    return os.path.getsize(full) if os.path.isfile(full) else 0


# Pack eenmalig indexeren op naam.
by_name = {}
for data in registry.get_assets_by_path(PACK, recursive=True):
    by_name.setdefault(str(data.asset_name), str(data.package_name))
unreal.log(f"kit-migratie: {len(by_name)} assets geindexeerd onder {PACK}")


def textures_of(package_path):
    """Texture2D-packages waar dit package van afhangt, één niveau diep."""
    found = []
    try:
        deps = registry.get_dependencies(unreal.Name(package_path), dep_options)
    except Exception as exc:
        unreal.log_warning(f"dependencies van {package_path} onleesbaar ({exc})")
        return found
    for dep in deps or []:
        dep_name = str(dep)
        if not dep_name.startswith(PACK):
            continue
        for data in registry.get_assets_by_package_name(unreal.Name(dep_name)):
            if str(data.asset_class_path.asset_name) == "Texture2D":
                found.append(dep_name)
    return found


def migrate(old_path, dest_folder, label):
    """Duplicate + consolidate. Geeft (nieuw_pad, toegevoegde_bytes) of (None, 0)."""
    name = old_path.rsplit("/", 1)[-1]
    new_path = f"{dest_folder}/{name}"
    if lib.does_asset_exist(new_path):
        old_data = lib.find_asset_data(old_path)
        settled = (not old_data.is_valid()) or str(old_data.asset_class_path.asset_name) == "ObjectRedirector"
        if settled:
            unreal.log(f"KIT-SKIP {label}: {new_path} bestaat al (idempotente herstart).")
            return new_path, 0
        unreal.log_error(f"KIT-HALFDONE {label}: {new_path} bestaat maar {old_path} is nog een levend asset — "
                         f"consolidate heeft nooit gedraaid; met de hand oplossen.")
        return None, 0
    if not lib.does_asset_exist(old_path):
        unreal.log_error(f"KIT-MISS {label}: bron niet gevonden: {old_path}")
        return None, 0

    src = lib.load_asset(old_path)
    dup = lib.duplicate_asset(old_path, new_path)
    if dup is None:
        unreal.log_error(f"KIT-FAIL {label}: duplicate naar {new_path} mislukt — bron ongemoeid.")
        return None, 0
    if not lib.consolidate_assets(dup, [src]):
        unreal.log_error(f"KIT-FAIL {label}: consolidate {old_path} -> {new_path} mislukt; "
                         f"duplicaat blijft staan, origineel ongemoeid — met de hand oplossen.")
        return None, 0
    lib.save_asset(new_path)
    return new_path, package_bytes(new_path)


report = []
total_bytes = 0
failures = []

for mesh_name in CANDIDATES:
    package = by_name.get(mesh_name)
    if package is None:
        unreal.log_error(f"KIT-MISS {mesh_name}: staat niet in {PACK}")
        failures.append(mesh_name)
        continue

    texture_packages = textures_of(package)
    new_mesh, mesh_bytes = migrate(package, DEST_MESHES, mesh_name)
    if new_mesh is None:
        failures.append(mesh_name)
        continue

    moved_textures = []
    for tex_package in texture_packages:
        new_tex, tex_bytes = migrate(tex_package, DEST_TEXTURES, tex_package.rsplit("/", 1)[-1])
        if new_tex is not None:
            moved_textures.append(new_tex)
            total_bytes += tex_bytes

    # Slots naar de toon-master: het pack-materiaal is nooit wat rendert.
    mesh = lib.load_asset(new_mesh)
    if isinstance(mesh, unreal.StaticMesh):
        materials = mesh.static_materials
        for slot in materials:
            slot.material_interface = toon
        mesh.set_editor_property("static_materials", materials)
        lib.save_asset(new_mesh)

    total_bytes += mesh_bytes
    report.append({
        "mesh": mesh_name,
        "new_path": new_mesh,
        "bytes": mesh_bytes,
        "textures": moved_textures,
    })
    unreal.log(f"KIT-OK {mesh_name}: {new_mesh} ({mesh_bytes/1024/1024:.1f} MB, "
               f"{len(moved_textures)} textures mee)")

with open(REPORT, "w", encoding="utf-8") as handle:
    json.dump({"assets": report, "total_bytes": total_bytes, "failures": failures},
              handle, indent=1)

unreal.log(f"kit-migratie klaar: {len(report)}/{len(CANDIDATES)} meshes, "
           f"{total_bytes/1024/1024:.1f} MB toegevoegd aan de repo.")
if failures:
    unreal.log_error(f"kit-migratie: {len(failures)} mislukt: {', '.join(failures)}")
unreal.log(f"rapport: {REPORT}")
