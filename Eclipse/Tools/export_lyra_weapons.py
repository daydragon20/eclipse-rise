# Exporteert de wapenmeshes uit LyraStarterGame naar FBX, zodat ECLIPSE ze kan
# importeren zonder de Lyra-materiaalketen mee te slepen.
#
# Draaien TEGEN HET LYRA-PROJECT (niet tegen ECLIPSE):
#   UnrealEditor-Cmd "C:\Users\natha\Documents\Unreal Projects\LyraStarterGame\LyraStarterGame.uproject" \
#     -run=pythonscript -script="C:\Dev\ECLIPSE_GDD\Eclipse\Tools\export_lyra_weapons.py" \
#     -unattended -nopause -nosplash -NoLiveCoding
#
# ------------------------------------------------- WAAROM EXPORT EN GEEN MIGRATE
# "Migrate" in de editor sleept de hele afhankelijkheidsboom mee: materialen,
# material functions, texturen, en bij Lyra ook de gedeelde master-materials. Dat
# is echt werk, het vraagt een open editor (en dus een owner-klik), en het levert
# ons spullen op die we per definitie weggooien.
#
# Want dat is de crux: **ECLIPSE gebruikt die materialen niet.** De stijl-wet
# (15.5) zegt dat alles door de toon-master gaat, en AttachWeaponMesh vervangt
# ELK materiaalslot door een toon-MID. Van een geimporteerd wapen overleeft dus
# alleen de GEOMETRIE. Dan is de FBX-weg niet een omweg maar de rechte lijn: geen
# afhankelijkheden, geen redirectors, geen owner-klik.
#
# ---------------------------------------------------- EN DE FICTIEWET (20.2)
# Dit is ook precies waarom deze assets de fictiewet kunnen halen waar een
# doorsnee pack dat niet doet. §20.2 verbiedt aardse merktekens, logo's en Latijns
# schrift in wereld-tekst. Een Lyra-geweer draagt zulke tekens in zijn DIFFUSE
# (T_Rifle_D, T_Rifle_Masks) — en juist die texture komt hier nooit aan: de
# weapon-MID in AttachWeaponMesh zet LitColor, ShadeColor, LightDir, EmissiveScale
# en UVMode, en **geen AlbedoTex**. Wat er op het scherm komt is silhouet plus
# gunmetal-waarde. De aardse semiotiek wordt dus door CONSTRUCTIE weggelaten en
# niet door curatie — dat is het verschil met de zeven borden.
#
# Wat daarmee NIET is afgevangen en dus met het oog moet: de VORM zelf. Een
# geweer dat als hedendaags-aards leest (AR-15-silhouet met picatinny-rails)
# blijft fout, ook zonder textuur. Dat oordeel staat in het rapport, niet hier.

import os
import unreal

OUT = r"C:\Dev\ECLIPSE_GDD\Eclipse\Saved\LyraWeapons"

# Alleen de STATIC meshes: de wapencomponent in ECLIPSE is een
# UStaticMeshComponent. De SK_-varianten dragen een eigen skelet dat we niet
# gebruiken en dat alleen maar afhankelijkheden meebrengt.
ASSETS = [
    "/Game/Weapons/Rifle/Mesh/SM_Rifle",
    "/Game/Weapons/Pistol/Mesh/SM_Pistol",
    "/Game/Weapons/Shotgun/Mesh/SM_Shotgun",
]

os.makedirs(OUT, exist_ok=True)
print("=" * 96)
print("LYRA-WAPENS -> FBX  (alleen geometrie; materialen blijven bewust achter)")
print("=" * 96)

exported = []
for path in ASSETS:
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if asset is None:
        print("ONTBREEKT: %s" % path)
        continue

    name = asset.get_name()
    box = asset.get_bounding_box()
    size = (box.max.x - box.min.x, box.max.y - box.min.y, box.max.z - box.min.z)
    longest = max(size)
    axis = "XYZ"[size.index(longest)]
    slots = len(asset.static_materials)
    tris = asset.get_num_triangles(0)
    verts = asset.get_num_vertices(0)
    lods = asset.get_num_lods()

    # DE MAAT EN DE AS, hier al. Een asset uit een ander project brengt zijn eigen
    # as-conventie mee, en dat is precies waar de greeprotatie-diagnose van
    # vanavond over ging: sluit de mesh-as UIT voordat je aan de rotatie draait.
    print("%-14s %7.1f x %6.1f x %6.1f cm | langste as %s = %6.1f | %5d tris, %5d verts, %d LODs, %d slots"
          % (name, size[0], size[1], size[2], axis, longest, tris, verts, lods, slots))
    print("               pivot: X[%.1f..%.1f] Y[%.1f..%.1f] Z[%.1f..%.1f]"
          % (box.min.x, box.max.x, box.min.y, box.max.y, box.min.z, box.max.z))
    for i, m in enumerate(asset.static_materials):
        iface = m.material_interface
        print("               slot %d: %s" % (i, iface.get_name() if iface else "(leeg)"))

    task = unreal.AssetExportTask()
    task.set_editor_property("object", asset)
    task.set_editor_property("filename", os.path.join(OUT, name + ".fbx"))
    task.set_editor_property("automated", True)
    task.set_editor_property("prompt", False)
    task.set_editor_property("replace_identical", True)
    options = unreal.FbxExportOption()
    options.set_editor_property("collision", False)
    options.set_editor_property("level_of_detail", False)
    task.set_editor_property("options", options)
    if unreal.Exporter.run_asset_export_task(task):
        exported.append(name)
        print("               -> %s.fbx" % name)
    else:
        print("               EXPORT MISLUKT")

print("=" * 96)
print("geexporteerd: %d van %d -> %s" % (len(exported), len(ASSETS), OUT))
if not exported:
    raise RuntimeError("geen enkel wapen geexporteerd")
