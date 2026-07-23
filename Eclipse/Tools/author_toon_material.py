# Authors the stylized cel/toon master materials (Part 15.5, Borderlands-leaning,
# locked direction):
#   /Game/Art/M_EclipseToon      - Unlit, bands + hatching + albedo as emissive.
#                                  The shipped district look on every hardware tier.
#   /Game/Art/M_EclipseToonLit   - DefaultLit variant: the SAME banded color as
#                                  BaseColor, so real VSM shadows + software Lumen
#                                  GI paint on top of the cel bands. The lit-toon
#                                  migration experiment (15.5 fidelity revision) -
#                                  selected by the builder only behind the
#                                  -EclipseLitToon flag on SM6; never the default
#                                  until an A/B review locks it.
#   /Game/Art/M_EclipseToonDecal - Unlit TRANSLUCENT variant of the unlit master
#                                  (15.8 art-fix round): same cel/albedo path,
#                                  plus a MaskTex whose red channel drives
#                                  opacity. Ground stains ride this so they fade
#                                  out organically instead of reading as hard
#                                  dark rectangles ("carpet tiles", review shots
#                                  00008-00013). A separate asset by design: the
#                                  opaque masters that every other slot rides
#                                  stay bit-identical.
# Run headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\author_toon_material.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash
#
# Why Unlit remains the floor: on the GTX 1050 dev box the D3D12 SM5 fallback
# never lights horizontal surfaces reliably (EclipseGrayboxBuilder.cpp, passes
# 5-16 forensics). Computing the light bands in the shader from a data-driven
# LightDir parameter makes the district's read deterministic on every hardware
# tier - the lit variant then ADDS real light on top for SM6 machines.
#
# Re-runnable by design: existing materials are cleared and rebuilt (the assets
# are wholly owned by this script; palette/tuning live in MID parameters).

import unreal

ART_PATH = "/Game/Art"

# Cel band thresholds — baked as the parameter DEFAULTS of all three masters;
# no MID overrides them, so this is the single authority for the district's
# banding (EclipseCharacter's pawn MIDs inherit them too, keeping pawns and
# district on the same sun story without touching that file).
# 15.8 look-ronde A/B (cam 1, checkpoint-muur): under the -25/55 dusk sun,
# west facades sit at ndl +0.52 and landed in the MID band (50% lerp toward
# the shade tone - BldgA_W read salmon-pink) while south facades (+0.74) went
# lit: one asset read as two color plates, district-wide on every west gevel
# (BldgB_W washed the same way). BAND_HI 0.50 moves both sun-facing verticals
# into the lit band - cel logic: faces toward the sun read lit - while the
# floor (ndl = sin 25 deg = 0.423) stays safely in the mid band.
# A/B evidence (2026-07-23): variant A (0.50, shot 00029) west face 251/160/53
# vs lit 252/158/55 - one asset, one color; exposure deltas <= 2 on every cam
# and the cam-5 sky corner is pixel-identical (not banding-related). Variant B
# (0.55 + per-gevel tint-compensatie, shot 00036) fixed only the named slab:
# the compound's other west faces stayed salmon (190/125/91). Sun-yaw was
# rejected outright - SunRotation is synced with EclipseCharacter.cpp's
# hard-coded SunTravel. Owner can overrule via the A/B panel.
BAND_HI = 0.50
BAND_LO = 0.10

TOON_HLSL = """
float3 N = normalize(NormalWS);
float3 L = normalize(-LightDir.rgb);
float ndl = dot(N, L);
float litStep = step(BandHi, ndl);
float midStep = step(BandLo, ndl);
float3 midColor = lerp(ShadeColor.rgb, LitColor.rgb, 0.5);
float3 col = lerp(ShadeColor.rgb, midColor, midStep);
col = lerp(col, LitColor.rgb, litStep);
// Surface character from a world-aligned albedo (CC0 texture pass, 15.5): the
// dominant-axis projection keeps tiling uniform across the wildly non-uniform
// cube scales (a 200:1 slab and a 1:1 crate read the same grain). AlbedoMix
// defaults to 0 so untextured instances are bit-identical to the flat cel.
// Luminance-only variation: the palette keeps exclusive hue authority (15.5
// palette discipline - a rusty texture must never re-tint a Dominion facade),
// the texture contributes value/grain only. AlbedoGain is measured per
// texture as 1/average-linear-luminance (builder) so the mean multiplier is
// 1.0 and texturing cannot re-meter the scene's auto-exposure; the clamp
// kills specular-scratch outliers a 10-30x normalization would blow out.
float3 an = abs(N);
float2 wuv = ((an.z > 0.6) ? WorldPos.xy : ((an.x >= an.y) ? WorldPos.yz : WorldPos.xz)) / max(TexWorldScale, 1.0);
// UVMode 1 = authored mesh UVs (imported props); 0 = world-aligned (the
// scaled graybox cubes, which have no usable UVs of their own).
float2 tuv = lerp(wuv, MeshUV, saturate(UVMode));
float3 albedo = Texture2DSample(AlbedoTex, AlbedoTexSampler, tuv).rgb;
float varL = min(dot(albedo, float3(0.2126, 0.7152, 0.0722)) * AlbedoGain, 2.5);
col *= lerp(1.0, varL, saturate(AlbedoMix));
// Hatching reads as pen strokes, not corrugation: wide spacing with a thin
// dark stroke (25% duty). At 50% duty and a 42-unit period the shade band
// looked like corrugated sheet metal (first strong-PC round, camera 2).
float stripe = step(0.75, frac((WorldPos.x + WorldPos.y) / max(HatchScale, 1.0)));
col *= 1.0 - (stripe * (1.0 - midStep) * saturate(HatchStrength));
return col * EmissiveScale;
"""

# Decal variant: same cel body, but the return carries opacity in .a. The mask
# rides the MESH UVs (an engine-cube face spans 0..1) while the albedo keeps its
# own world-aligned/mesh choice via UVMode - rotating a stain quad rotates its
# silhouette but never the world-locked grain under it.
DECAL_RETURN = """float mask = Texture2DSample(MaskTex, MaskTexSampler, MeshUV).r;
return float4(col * EmissiveScale, saturate(mask * OpacityScale));
"""
DECAL_HLSL = TOON_HLSL.replace("return col * EmissiveScale;", DECAL_RETURN)
assert DECAL_HLSL != TOON_HLSL, "toon return line moved - update DECAL_RETURN splice"

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary


def author_toon(mat_name, lit):
    full_path = f"{ART_PATH}/{mat_name}"
    if eal.does_asset_exist(full_path):
        mat = eal.load_asset(full_path)
        mel.delete_all_material_expressions(mat)
        unreal.log(f"{mat_name}: rebuilding existing asset")
    else:
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        mat = tools.create_asset(mat_name, ART_PATH, unreal.Material, unreal.MaterialFactoryNew())
        unreal.log(f"{mat_name}: created new asset")

    if not lit:
        # Unlit: every scene-light bug on the SM5 fallback becomes irrelevant.
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    else:
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    # Skeletal usage: without this flag the material renders BLACK on skeletal
    # meshes in -game builds (first figure round: the silhouette guard). The
    # editor auto-adds usages on the fly; cooked/game paths never do.
    mat.set_editor_property("used_with_skeletal_mesh", True)
    mat.set_editor_property("used_with_morph_targets", True)

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
    # normal, so PixelNormalWS reads garbage there and every surface fell into
    # the shade band (pass-23 forensics). The mesh normal is what cel bands want.
    normal_ws = mel.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -900, -520)
    lit_color = vec_param("LitColor", (0.50, 0.50, 0.52, 1.0), -900, -380)
    shade_color = vec_param("ShadeColor", (0.14, 0.13, 0.18, 1.0), -900, -180)
    # Direction the light TRAVELS (world space); builder syncs with its sun.
    light_dir = vec_param("LightDir", (-0.44, -0.62, -0.66, 0.0), -900, 20)
    # Bands tuned for the -25 deg dusk sun: floor lands in the mid tone (asphalt
    # must never be the brightest surface), sun-facing facades go lit.
    band_hi = scalar_param("BandHi", BAND_HI, -900, 220)
    band_lo = scalar_param("BandLo", BAND_LO, -900, 300)
    hatch_scale = scalar_param("HatchScale", 120.0, -900, 380)
    hatch_strength = scalar_param("HatchStrength", 0.22, -900, 460)
    # Unlit: authored color -> final pixel via the emissive range shared with
    # the SkyAtmosphere (builder sets 10). Lit: MUST stay 1 - BaseColor is an
    # albedo, the real lights supply the energy (builder skips the override).
    emissive_scale = scalar_param("EmissiveScale", 1.0, -900, 540)
    world_pos = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -900, 640)
    # Albedo path (CC0 texture pass): white texture + mix 0 = the untextured
    # cel look, exactly. The builder opts palette entries in per texture.
    albedo_tex = mel.create_material_expression(mat, unreal.MaterialExpressionTextureObjectParameter, -900, 700)
    albedo_tex.set_editor_property("parameter_name", "AlbedoTex")
    white_tex = unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture")
    if white_tex:
        albedo_tex.set_editor_property("texture", white_tex)
    tex_world_scale = scalar_param("TexWorldScale", 400.0, -900, 800)
    albedo_mix = scalar_param("AlbedoMix", 0.0, -900, 880)
    albedo_gain = scalar_param("AlbedoGain", 1.8, -900, 960)
    uv_mode = scalar_param("UVMode", 0.0, -900, 1040)
    mesh_uv = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -900, 1120)

    toon = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -420, 0)
    toon.set_editor_property("code", TOON_HLSL)
    toon.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    toon.set_editor_property("description", "EclipseToonBands")

    inputs = []
    for input_name in ("NormalWS", "LitColor", "ShadeColor", "LightDir", "BandHi", "BandLo",
                       "HatchScale", "HatchStrength", "EmissiveScale", "WorldPos",
                       "AlbedoTex", "TexWorldScale", "AlbedoMix", "AlbedoGain",
                       "UVMode", "MeshUV"):
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
    mel.connect_material_expressions(albedo_tex, "", toon, "AlbedoTex")
    mel.connect_material_expressions(tex_world_scale, "", toon, "TexWorldScale")
    mel.connect_material_expressions(albedo_mix, "", toon, "AlbedoMix")
    mel.connect_material_expressions(albedo_gain, "", toon, "AlbedoGain")
    mel.connect_material_expressions(uv_mode, "", toon, "UVMode")
    mel.connect_material_expressions(mesh_uv, "", toon, "MeshUV")

    if not lit:
        mel.connect_material_property(toon, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        mel.connect_material_property(toon, "", unreal.MaterialProperty.MP_BASE_COLOR)
        # Matte surfaces: the cel read must come from the bands, not speculars.
        rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -420, 300)
        rough.set_editor_property("r", 0.92)
        mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    mel.recompile_material(mat)
    if not eal.save_asset(full_path):
        raise RuntimeError(f"{mat_name}: save failed")
    unreal.log(f"{mat_name}: authored and saved ({full_path})")


def author_toon_decal(mat_name):
    """Unlit translucent stain/decal variant: cel body + MaskTex-driven opacity."""
    full_path = f"{ART_PATH}/{mat_name}"
    if eal.does_asset_exist(full_path):
        mat = eal.load_asset(full_path)
        mel.delete_all_material_expressions(mat)
        unreal.log(f"{mat_name}: rebuilding existing asset")
    else:
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        mat = tools.create_asset(mat_name, ART_PATH, unreal.Material, unreal.MaterialFactoryNew())
        unreal.log(f"{mat_name}: created new asset")

    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

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

    def tex_param(name, x, y):
        e = mel.create_material_expression(mat, unreal.MaterialExpressionTextureObjectParameter, x, y)
        e.set_editor_property("parameter_name", name)
        white = unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture")
        if white:
            e.set_editor_property("texture", white)
        return e

    normal_ws = mel.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -900, -520)
    lit_color = vec_param("LitColor", (0.50, 0.50, 0.52, 1.0), -900, -380)
    shade_color = vec_param("ShadeColor", (0.14, 0.13, 0.18, 1.0), -900, -180)
    light_dir = vec_param("LightDir", (-0.44, -0.62, -0.66, 0.0), -900, 20)
    band_hi = scalar_param("BandHi", BAND_HI, -900, 220)
    band_lo = scalar_param("BandLo", BAND_LO, -900, 300)
    hatch_scale = scalar_param("HatchScale", 120.0, -900, 380)
    hatch_strength = scalar_param("HatchStrength", 0.22, -900, 460)
    emissive_scale = scalar_param("EmissiveScale", 1.0, -900, 540)
    world_pos = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -900, 640)
    albedo_tex = tex_param("AlbedoTex", -900, 700)
    tex_world_scale = scalar_param("TexWorldScale", 400.0, -900, 800)
    albedo_mix = scalar_param("AlbedoMix", 0.0, -900, 880)
    albedo_gain = scalar_param("AlbedoGain", 1.8, -900, 960)
    uv_mode = scalar_param("UVMode", 0.0, -900, 1040)
    mesh_uv = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -900, 1120)
    # White default mask = fully opaque = the opaque master's look, exactly.
    mask_tex = tex_param("MaskTex", -900, 1200)
    opacity_scale = scalar_param("OpacityScale", 1.0, -900, 1280)

    toon = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -420, 0)
    toon.set_editor_property("code", DECAL_HLSL)
    toon.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    toon.set_editor_property("description", "EclipseToonDecal")

    inputs = []
    for input_name in ("NormalWS", "LitColor", "ShadeColor", "LightDir", "BandHi", "BandLo",
                       "HatchScale", "HatchStrength", "EmissiveScale", "WorldPos",
                       "AlbedoTex", "TexWorldScale", "AlbedoMix", "AlbedoGain",
                       "UVMode", "MeshUV", "MaskTex", "OpacityScale"):
        custom_input = unreal.CustomInput()
        custom_input.set_editor_property("input_name", input_name)
        inputs.append(custom_input)
    toon.set_editor_property("inputs", inputs)

    for expr, pin in ((normal_ws, "NormalWS"), (lit_color, "LitColor"), (shade_color, "ShadeColor"),
                      (light_dir, "LightDir"), (band_hi, "BandHi"), (band_lo, "BandLo"),
                      (hatch_scale, "HatchScale"), (hatch_strength, "HatchStrength"),
                      (emissive_scale, "EmissiveScale"), (world_pos, "WorldPos"),
                      (albedo_tex, "AlbedoTex"), (tex_world_scale, "TexWorldScale"),
                      (albedo_mix, "AlbedoMix"), (albedo_gain, "AlbedoGain"),
                      (uv_mode, "UVMode"), (mesh_uv, "MeshUV"),
                      (mask_tex, "MaskTex"), (opacity_scale, "OpacityScale")):
        mel.connect_material_expressions(expr, "", toon, pin)

    # float4 out: rgb -> emissive, a -> opacity.
    rgb_mask = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -180, -60)
    rgb_mask.set_editor_property("r", True)
    rgb_mask.set_editor_property("g", True)
    rgb_mask.set_editor_property("b", True)
    rgb_mask.set_editor_property("a", False)
    mel.connect_material_expressions(toon, "", rgb_mask, "")
    mel.connect_material_property(rgb_mask, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    alpha_mask = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -180, 160)
    alpha_mask.set_editor_property("r", False)
    alpha_mask.set_editor_property("g", False)
    alpha_mask.set_editor_property("b", False)
    alpha_mask.set_editor_property("a", True)
    mel.connect_material_expressions(toon, "", alpha_mask, "")
    mel.connect_material_property(alpha_mask, "", unreal.MaterialProperty.MP_OPACITY)

    mel.recompile_material(mat)
    if not eal.save_asset(full_path):
        raise RuntimeError(f"{mat_name}: save failed")
    unreal.log(f"{mat_name}: authored and saved ({full_path})")


author_toon("M_EclipseToon", lit=False)
author_toon("M_EclipseToonLit", lit=True)
author_toon_decal("M_EclipseToonDecal")
