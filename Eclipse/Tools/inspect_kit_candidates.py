# Kit-pass P2-08, werkorde stap 3: per kandidaat-mesh de materiaalslots UITLEZEN
# en elke base-colour texture EXPORTEREN, zodat measure_albedo_gain.py er een
# gemeten gain van kan maken.
#
# Waarom dit een eigen stap is en niet "gewoon plaatsen": KITPASS_P2-08 par. 2.3
# eist een gemeten gain per albedo, omdat 254 van de 416 textures 4K+ zijn en een
# geschatte gain precies is waar de magenta-container en de vloer-omkering op
# stukliepen. En de slot-telling uit de curatie is een AGGREGAAT (78% van
# Uniblocks heeft een slot) - welke slots een concrete mesh heeft, en of die
# scheiding bruikbaar is voor wand/trim, staat daar niet in.
#
# Report-and-export only: dit script plaatst niets, wijzigt geen asset en
# migreert niets. De packs blijven machine-lokaal en untracked.
#
# Draai headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\inspect_kit_candidates.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

import json
import os

import unreal

# De kandidaten uit KITPASS_P2-08 par. 1, in de volgorde van de werkorde: de
# warehouse-yard eerst. De machine-bank-voorkant staat bovenaan omdat de
# art-review "machine-faces onleesbaar" als eigen bevinding noemde en
# SM_Controller_1 met 7 slots de enige mesh is die die scheiding kan dragen.
CANDIDATES = [
    ("machine-bank voorkant", "SM_Controller_1"),
    ("industriele massa", "SM_Tank"),
    ("industriele massa", "SM_pipe_5"),
    ("verticaliteit", "SM_Power_Pole"),
    ("verticaliteit", "SM_crane_2"),
    ("verticaliteit", "SM_crane_3"),
    ("skyline", "SM_Tower_9_A"),
    ("skyline", "SM_Tower_11_C"),
    ("modulair wand+trim", "SM_wall_2"),
    ("modulair wand+trim", "SM_wall_3"),
    ("hero-prop", "SM_barrel_2"),
    ("hero-prop", "SM_gas_bottle_packfbx"),
]

PACK = "/Game/Factory_Pack_V1"
RAW = os.path.join(unreal.SystemLibrary.get_project_saved_directory(), "CurationStaging", "raw")
REPORT = os.path.join(unreal.SystemLibrary.get_project_saved_directory(), "CurationStaging",
                      "kit_candidates.json")
os.makedirs(RAW, exist_ok=True)

registry = unreal.AssetRegistryHelpers.get_asset_registry()


def triangles_of(package, mesh_name):
    """Driehoeken uit de asset-registry-tag, net als de curatiepass.

    Niet via een editor-subsysteem: dat is None in een commandlet, en de tag is
    bovendien dezelfde bron waarop de curatie-verdicten al gebaseerd zijn - dus
    de getallen hier zijn per definitie vergelijkbaar met die in
    CURATIE_ENVPACKS_2026-07-25.md.
    """
    for data in registry.get_assets_by_package_name(unreal.Name(package)):
        if str(data.asset_name) != mesh_name:
            continue
        raw = data.get_tag_value("Triangles")
        try:
            return int(str(raw))
        except (TypeError, ValueError):
            return 0
    return 0

# Eenmalig het pack indexeren: de meshes zitten niet allemaal in dezelfde
# submap, en een pad raden is precies het soort aanname dat deze pass vermijdt.
by_name = {}
for data in registry.get_assets_by_path(PACK, recursive=True):
    by_name.setdefault(str(data.asset_name), str(data.package_name))

unreal.log(f"kit-inspectie: {len(by_name)} assets geindexeerd onder {PACK}")

# Base-colour heet in dit pack niet overal hetzelfde; deze lijst bepaalt wat als
# albedo telt. Alles wat hier niet op matcht wordt WEL gerapporteerd maar niet
# geexporteerd, zodat een gemiste naamconventie zichtbaar blijft in plaats van
# stil te verdwijnen.
COLOUR_HINTS = ("basecolor", "base_color", "albedo", "_color", "_diff")
# En een expliciete uitsluitlijst, want een losse "_d"-hint matchte hier
# "_Displacement" en exporteerde hoogtekaarten als albedo. Een gain gemeten op
# een displacement-map is een getal dat er geloofwaardig uitziet en nergens op
# slaat - precies de fout die deze hele pass moet voorkomen.
NON_COLOUR = ("displacement", "normal", "roughness", "metallic", "_ao",
              "ambientocclusion", "height", "opacity", "specular", "emissive")


def textures_of(material):
    """Alle Texture2D's waar dit materiaal van afhangt, instance of niet."""
    found = []
    seen = set()
    package = material.get_outermost().get_path_name()
    try:
        deps = registry.get_dependencies(unreal.Name(package),
                                         unreal.AssetRegistryDependencyOptions())
    except Exception as exc:
        unreal.log_warning(f"dependencies van {package} onleesbaar ({exc})")
        return found
    for dep in deps or []:
        dep_name = str(dep)
        if dep_name in seen or not dep_name.startswith("/Game"):
            continue
        seen.add(dep_name)
        for data in registry.get_assets_by_package_name(unreal.Name(dep_name)):
            asset = data.get_asset()
            if isinstance(asset, unreal.Texture2D):
                found.append(asset)
    return found

report = []
exported = 0
for purpose, mesh_name in CANDIDATES:
    package = by_name.get(mesh_name)
    if package is None:
        unreal.log_warning(f"NIET GEVONDEN: {mesh_name} - staat niet in {PACK}")
        report.append({"mesh": mesh_name, "purpose": purpose, "found": False})
        continue

    mesh = unreal.load_asset(f"{package}.{mesh_name}")
    if mesh is None:
        unreal.log_warning(f"NIET LAADBAAR: {package}")
        report.append({"mesh": mesh_name, "purpose": purpose, "found": False})
        continue

    tris = triangles_of(package, mesh_name)

    slots = []
    for slot_index, slot in enumerate(mesh.static_materials):
        material = slot.material_interface
        slot_row = {
            "index": slot_index,
            "slot_name": str(slot.material_slot_name),
            "material": str(material.get_path_name()) if material else None,
            "textures": [],
        }
        if material is not None:
            # Via de dependency-graaf en niet via MaterialEditingLibrary: dat
            # laatste accepteert alleen een UMaterial, en dit pack levert
            # MaterialInstanceConstants. De graaf werkt voor allebei en pakt ook
            # de textures die alleen als instance-parameter zijn overschreven.
            for tex in textures_of(material):
                tex_name = str(tex.get_name())
                entry = {
                    "texture": tex_name,
                    "size": [int(tex.blueprint_get_size_x()), int(tex.blueprint_get_size_y())],
                    "exported": False,
                }
                lowered = tex_name.lower()
                is_colour = (any(h in lowered for h in COLOUR_HINTS)
                             and not any(n in lowered for n in NON_COLOUR))
                entry["is_colour"] = is_colour
                if is_colour:
                    out = os.path.join(RAW, f"{tex_name}.png")
                    if not os.path.isfile(out):
                        task = unreal.AssetExportTask()
                        task.object = tex
                        task.filename = out
                        task.exporter = unreal.TextureExporterPNG()
                        task.automated = True
                        task.replace_identical = True
                        task.prompt = False
                        unreal.Exporter.run_asset_export_task(task)
                        exported += 1
                    entry["exported"] = os.path.isfile(out)
                slot_row["textures"].append(entry)
        slots.append(slot_row)

    unreal.log(f"{mesh_name}: {tris} tris, {len(slots)} slots ({purpose})")
    report.append({
        "mesh": mesh_name,
        "purpose": purpose,
        "found": True,
        "package": package,
        "triangles": tris,
        "slot_count": len(slots),
        "slots": slots,
    })

with open(REPORT, "w", encoding="utf-8") as handle:
    json.dump(report, handle, indent=1)

found = sum(1 for row in report if row.get("found"))
unreal.log(f"kit-inspectie klaar: {found}/{len(CANDIDATES)} meshes gevonden, "
           f"{exported} textures nieuw geexporteerd naar {RAW}")
unreal.log(f"rapport: {REPORT}")
