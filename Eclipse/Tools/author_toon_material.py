# Authors /Game/Art/M_EclipseToon — the stylized cel/toon master material (Part 15.5,
# Borderlands-leaning, locked direction). Run headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\author_toon_material.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash
#
# Why Unlit + material-space banding: on the GTX 1050 dev box the D3D12 SM5
# fallback never lights horizontal surfaces reliably (EclipseGrayboxBuilder.cpp,
# passes 5-16 forensics). Computing the light bands in the shader from a
# data-driven LightDir parameter makes the district's read deterministic on every
# hardware tier — the RTX target then *adds* Lumen/VSM on top for characters and
# authored kits, it never has to fight this material.
#
# Re-runnable by design: an existing material is cleared and rebuilt (the asset is
# wholly owned by this script; palette/tuning live in MID parameters, not here).

import unreal

ART_PATH = "/Game/Art"
MAT_NAME = "M_EclipseToon"
FULL_PATH = f"{ART_PATH}/{MAT_NAME}"

TOON_HLSL = """
float3 N = normalize(NormalWS);
float3 L = normalize(-LightDir.rgb);
float ndl = dot(N, L);
float litStep = step(BandHi, ndl);
float midStep = step(BandLo, ndl);
float3 midColor = lerp(ShadeColor.rgb, LitColor.rgb, 0.5);
float3 col = lerp(ShadeColor.rgb, midColor, midStep);
col = lerp(col, LitColor.rgb, litStep);
float stripe = step(0.5, frac((WorldPos.x + WorldPos.y) / max(HatchScale, 1.0)));
col *= 1.0 - (stripe * (1.0 - midStep) * saturate(HatchStrength));
return col * EmissiveScale;
"""

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

if eal.does_asset_exist(FULL_PATH):
    mat = eal.load_asset(FULL_PATH)
    mel.delete_all_material_expressions(mat)
    unreal.log(f"{MAT_NAME}: rebuilding existing asset")
else:
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = tools.create_asset(MAT_NAME, ART_PATH, unreal.Material, unreal.MaterialFactoryNew())
    unreal.log(f"{MAT_NAME}: created new asset")

# Unlit: every scene-light bug on the SM5 fallback becomes irrelevant to the district's read.
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)


def vec_param(name, default, x, y):
    e = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", unreal.LinearColor(*default))
    return e


def scalar_param(name, default, x, y):
    e = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", default)
    return e


# VertexNormalWS, not PixelNormalWS: unlit materials never write a GBuffer
# normal, so PixelNormalWS reads garbage there and every surface fell into the
# shade band (pass-23 forensics). The mesh normal is exactly what cel bands want.
normal_ws = mel.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -900, -520)
lit_color = vec_param("LitColor", (0.50, 0.50, 0.52, 1.0), -900, -380)
shade_color = vec_param("ShadeColor", (0.14, 0.13, 0.18, 1.0), -900, -180)
# Direction the light TRAVELS (world space); the builder syncs this with its sun rotator.
light_dir = vec_param("LightDir", (-0.44, -0.62, -0.66, 0.0), -900, 20)
# Bands tuned for the -25 deg dusk sun: floor lands in the mid tone (asphalt must
# never be the brightest surface), sun-facing facades go lit, away faces shade+hatch.
band_hi = scalar_param("BandHi", 0.55, -900, 220)
band_lo = scalar_param("BandLo", 0.10, -900, 300)
hatch_scale = scalar_param("HatchScale", 42.0, -900, 380)
hatch_strength = scalar_param("HatchStrength", 0.30, -900, 460)
# Authored color -> final pixel needs the emissive to live in the same physical
# range as the SkyAtmosphere; the builder pins exposure at EV100=12 and sets this
# to 1.2 * 2^12 so a LitColor of (0.5, ...) lands on screen as exactly that value.
emissive_scale = scalar_param("EmissiveScale", 1.0, -900, 540)
world_pos = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -900, 640)

toon = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -420, 0)
toon.set_editor_property("code", TOON_HLSL)
toon.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
toon.set_editor_property("description", "EclipseToonBands")

inputs = []
for input_name in ("NormalWS", "LitColor", "ShadeColor", "LightDir", "BandHi", "BandLo", "HatchScale", "HatchStrength", "EmissiveScale", "WorldPos"):
    custom_input = unreal.CustomInput()
    custom_input.set_editor_property("input_name", input_name)
    inputs.append(custom_input)
toon.set_editor_property("inputs", inputs)

mel.connect_material_expressions(normal_ws, "", toon, "NormalWS")
mel.connect_material_expressions(lit_color, "", toon, "LitColor")
mel.connect_material_expressions(shade_color, "", toon, "ShadeColor")
mel.connect_material_expressions(light_dir, "", toon, "LightDir")
mel.connect_material_expressions(band_hi, "", toon, "BandHi")
mel.connect_material_expressions(band_lo, "", toon, "BandLo")
mel.connect_material_expressions(hatch_scale, "", toon, "HatchScale")
mel.connect_material_expressions(hatch_strength, "", toon, "HatchStrength")
mel.connect_material_expressions(emissive_scale, "", toon, "EmissiveScale")
mel.connect_material_expressions(world_pos, "", toon, "WorldPos")

mel.connect_material_property(toon, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

mel.recompile_material(mat)
if not eal.save_asset(FULL_PATH):
    raise RuntimeError(f"{MAT_NAME}: save failed")
unreal.log(f"{MAT_NAME}: authored and saved ({FULL_PATH})")
