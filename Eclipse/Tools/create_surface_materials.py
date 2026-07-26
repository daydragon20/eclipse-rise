"""Maak de physical materials waar de voetstapcode het oppervlak aan afleest.

Waarom dit bestaat: op 26-07-2026 kwam Footsteps_Volume_02 binnen met 206 cues
per oppervlak — metaal, modder, zand, gras, hout — en er was geen enkel
oppervlaktetype in het project. Geen physical material van ons, geen regel code
die er een uitlas. Voetstappen klonken op één sample, overal.

De ladder heeft drie sporten en dit is de tweede:

  1. de types zelf staan in Config/DefaultEngine.ini (SurfaceType1..6)
  2. per type een physical material dat je op een vloer kunt leggen   <-- hier
  3. de voetstapcode traceert omlaag, leest het type, kiest de bank

Alleen de types die het district ECHT gebruikt krijgen een asset. Metal en
Concrete dragen Kessara; Mud, Grass, Sand en Wood staan in de ini en in de
geluidstabel klaar, maar een asset zonder vloer eronder is een dood asset — en
dood is van mij om op te ruimen, niet van de owner om te beslissen.

Draaien:

    UnrealEditor-Cmd.exe Eclipse.uproject -run=pythonscript ^
        -script="Eclipse\\Tools\\create_surface_materials.py" -unattended -nullrhi
"""

import unreal

TARGET_DIR = "/Game/Art/Physics"

# (assetnaam, oppervlaktetype, waar het op ligt)
SURFACES = [
    ("PM_Metal", unreal.PhysicalSurface.SURFACE_TYPE1, "loopvlakken, roosters, dekplaat"),
    ("PM_Concrete", unreal.PhysicalSurface.SURFACE_TYPE2, "het plein en de straat"),
]

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

if not eal.does_directory_exist(TARGET_DIR):
    eal.make_directory(TARGET_DIR)

made = 0
for name, surface, where in SURFACES:
    path = f"{TARGET_DIR}/{name}"
    if eal.does_asset_exist(path):
        material = eal.load_asset(path)
    else:
        material = tools.create_asset(
            asset_name=name,
            package_path=TARGET_DIR,
            asset_class=unreal.PhysicalMaterial,
            factory=unreal.PhysicalMaterialFactoryNew(),
        )
        made += 1
    if material is None:
        unreal.log_warning(f"SURFACE {name}: niet aan te maken")
        continue

    # Alleen het oppervlaktetype. Wrijving en dempingen bewust NIET aanraken:
    # die veranderen hoe het personage beweegt, en bewegen is gemeten en
    # afgesteld. Dit asset bestaat om te zeggen WAAR je op staat, niet hoe het
    # aanvoelt onder je voeten.
    material.set_editor_property("surface_type", surface)
    eal.save_asset(path)
    unreal.log(f"SURFACE {name} = {surface} ({where})")

unreal.log(f"SURFACE klaar: {made} nieuw, {len(SURFACES)} totaal in {TARGET_DIR}")
