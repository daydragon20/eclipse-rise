"""Maak M_EclipseToonFixture: de toon-master plus een emissive-masker.

Waarom dit bestaat (owner-besluit 26-07 avond, lichtplan punt 1): het Sci-Fi
Light Pack levert 17 armaturen die elk ÉÉN materiaal hebben met een aparte
emissive-TEXTUUR — zwart waar het metaal zit, fel waar de lamp zit. De
toon-master kan daar niets mee: hij heeft alleen `AlbedoTex`. Het `MaskTex` dat
ik eerst aanwees zit op het DECAL-materiaal, niet op de master.

De owner: *"Als MaskTex de Glow-band niet kan aansturen: bouw die
materiaal-ingang."*

**Op een KOPIE en niet in de master.** M_EclipseToonLit schildert het hele
district; een fout in die graaf raakt elk oppervlak tegelijk, en ik kan vanavond
geen visuele ronde draaien om dat te zien. Een armatuur is bovendien een ander
soort oppervlak dan een muur — het is het enige dat licht MAAKT — dus een eigen
materiaal is ook inhoudelijk juist. De één-stijl-wet blijft staan: dezelfde
toon-shading, dezelfde palet-autoriteit, alleen een ingang erbij.

Wat de ingang doet:

    EmissiveColor = EmissiveMaskTex.r * GlowColor * GlowGain

De emissive-map van het pack bepaalt WAAR het licht zit; het palet bepaalt welke
kleur (15.5: het palet is de enige kleur-autoriteit), en de gain zet hem in de
Glow-band — die ligt ruwweg een factor 22 boven de albedoband.

Draaien:

    UnrealEditor-Cmd.exe Eclipse.uproject -run=pythonscript ^
        -script="Eclipse\\Tools\\create_fixture_material.py" -unattended -nullrhi
"""

import unreal

SOURCE = "/Game/Art/M_EclipseToonLit"
TARGET_DIR = "/Game/Art"
TARGET_NAME = "M_EclipseToonFixture"
TARGET = f"{TARGET_DIR}/{TARGET_NAME}"

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary

if eal.does_asset_exist(TARGET):
    eal.delete_asset(TARGET)

if not eal.duplicate_asset(SOURCE, TARGET):
    raise RuntimeError(f"FIXTURE kon {SOURCE} niet dupliceren naar {TARGET}")

material = eal.load_asset(TARGET)
if material is None:
    raise RuntimeError("FIXTURE: kopie niet te laden")

# --- de drie nieuwe ingangen ------------------------------------------------
# Posities zijn cosmetisch (waar de node in de graaf staat); links van de output
# zodat de graaf leesbaar blijft voor wie hem opent.
mask = mel.create_material_expression(
    material, unreal.MaterialExpressionTextureSampleParameter2D, -900, 600)
mask.set_editor_property("parameter_name", "EmissiveMaskTex")

glow = mel.create_material_expression(
    material, unreal.MaterialExpressionVectorParameter, -900, 800)
glow.set_editor_property("parameter_name", "GlowColor")
# Sodium-oranje uit de Glow-ingang van het district-palet (2.2 / 1.0 / 0.3).
glow.set_editor_property("default_value", unreal.LinearColor(2.2, 1.0, 0.3, 1.0))

gain = mel.create_material_expression(
    material, unreal.MaterialExpressionScalarParameter, -900, 950)
gain.set_editor_property("parameter_name", "GlowGain")
# 10, gelijk aan ToonEmissiveScale in de graybox-builder: dat is de schaal waarop
# de Glow-familie daar al staat, dus een armatuur landt in dezelfde band als de
# raamstrips die er nu hangen. Geen nieuw getal, hetzelfde getal.
gain.set_editor_property("default_value", 10.0)

mask_times_glow = mel.create_material_expression(
    material, unreal.MaterialExpressionMultiply, -600, 700)
mel.connect_material_expressions(mask, "R", mask_times_glow, "A")
mel.connect_material_expressions(glow, "", mask_times_glow, "B")

scaled = mel.create_material_expression(
    material, unreal.MaterialExpressionMultiply, -400, 700)
mel.connect_material_expressions(mask_times_glow, "", scaled, "A")
mel.connect_material_expressions(gain, "", scaled, "B")

# Op de EmissiveColor-ingang. De lit-master zet daar niets op (EmissiveScale 1
# betekent daar: albedo, echte lichten leveren de energie), dus er wordt niets
# overschreven — alleen een lege ingang gevuld.
mel.connect_material_property(scaled, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

mel.recompile_material(material)
eal.save_asset(TARGET)

unreal.log(f"FIXTURE {TARGET_NAME}: EmissiveMaskTex + GlowColor + GlowGain toegevoegd")
unreal.log(f"FIXTURE scalars={[str(x) for x in mel.get_scalar_parameter_names(material)]}")
unreal.log(f"FIXTURE textures={[str(x) for x in mel.get_texture_parameter_names(material)]}")
