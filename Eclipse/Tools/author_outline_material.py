# Re-authors /Game/Art/PP_EclipseOutline â€” ink outlines as a VERIFIED scene
# passthrough (Part 15.5). The previous authored version replaced the whole frame
# with its own tint (pass-26 forensics: every "purple wash" since pass 20 was that
# material, not the scene). This build's contract: output = scene color with dark
# lines mixed in ONLY where a depth edge exists; with zero edges the frame is
# bit-identical to the scene.
#
# Run headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\author_outline_material.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash

import unreal

ART_PATH = "/Game/Art"
MAT_NAME = "PP_EclipseInk"
FULL_PATH = f"{ART_PATH}/{MAT_NAME}"

# Depth Sobel in a custom node. SceneTextureLookup ids: 1 = SceneDepth,
# 14 = PostProcessInput0 (the frame being post-processed). The wired SceneTexture
# expressions (SceneColorIn/DepthIn) exist to register those usages with the
# compiler; the actual taps happen here with per-pixel UV offsets.
OUTLINE_HLSL = """
float2 uv = UV.xy;
float2 px = View.ViewSizeAndInvSize.zw * LineWidth;
float dC = SceneTextureLookup(uv, 1, false).r;
float dL = SceneTextureLookup(uv + float2(-px.x, 0), 1, false).r;
float dR = SceneTextureLookup(uv + float2(px.x, 0), 1, false).r;
float dU = SceneTextureLookup(uv + float2(0, -px.y), 1, false).r;
float dD = SceneTextureLookup(uv + float2(0, px.y), 1, false).r;
// Second-derivative (Laplacian) edge test: a first-derivative Sobel floods any
// plane seen at a grazing angle -- the far floor's per-pixel depth delta dwarfs
// every workable threshold (first strong-PC round, 2026-07-22, shots 1-4: the
// whole midground painted solid ink). The Laplacian is ~zero on plane
// interiors at any view angle and spikes at true silhouettes and creases
// (wall-floor junctions), which is exactly the ink contract.
float grad = abs(dL + dR - 2.0 * dC) + abs(dU + dD - 2.0 * dC);
// Depth-proportional threshold: distant geometry needs a bigger delta to count
// as a silhouette, or the whole horizon becomes one smear of line.
float edge = saturate((grad - DepthThreshold * (1.0 + dC * DistanceFade)) * Hardness);
float3 scene = SceneTextureLookup(uv, 14, false).rgb;
return lerp(scene, LineColor.rgb, edge * saturate(LineOpacity));
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

mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
# After tonemapping: lines land on the graded frame, so bloom/exposure can never
# eat or tint them.
try:
    mat.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)
except Exception:
    mat.set_editor_property("blendable_location", unreal.BlendableLocation.BL_AFTER_TONEMAPPING)


def scalar_param(name, default, x, y):
    e = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", default)
    return e


def vec_param(name, default, x, y):
    e = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", unreal.LinearColor(*default))
    return e


scene_color_in = mel.create_material_expression(mat, unreal.MaterialExpressionSceneTexture, -950, -450)
scene_color_in.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
depth_in = mel.create_material_expression(mat, unreal.MaterialExpressionSceneTexture, -950, -250)
depth_in.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_SCENE_DEPTH)
screen_uv = mel.create_material_expression(mat, unreal.MaterialExpressionScreenPosition, -950, -50)

line_color = vec_param("LineColor", (0.013, 0.008, 0.012, 1.0), -950, 120)  # warm near-black ink
line_width = scalar_param("LineWidth", 1.6, -950, 320)
depth_threshold = scalar_param("DepthThreshold", 4.0, -950, 400)  # scene depth is in cm
distance_fade = scalar_param("DistanceFade", 0.00012, -950, 480)
hardness = scalar_param("Hardness", 0.02, -950, 560)
line_opacity = scalar_param("LineOpacity", 0.85, -950, 640)

outline = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -420, 0)
outline.set_editor_property("code", OUTLINE_HLSL)
outline.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
outline.set_editor_property("description", "EclipseInkOutline")

inputs = []
for input_name in ("UV", "SceneColorIn", "DepthIn", "LineColor", "LineWidth", "DepthThreshold", "DistanceFade", "Hardness", "LineOpacity"):
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", input_name)
    inputs.append(ci)
outline.set_editor_property("inputs", inputs)

mel.connect_material_expressions(screen_uv, "", outline, "UV")
mel.connect_material_expressions(scene_color_in, "Color", outline, "SceneColorIn")
mel.connect_material_expressions(depth_in, "Color", outline, "DepthIn")
mel.connect_material_expressions(line_color, "", outline, "LineColor")
mel.connect_material_expressions(line_width, "", outline, "LineWidth")
mel.connect_material_expressions(depth_threshold, "", outline, "DepthThreshold")
mel.connect_material_expressions(distance_fade, "", outline, "DistanceFade")
mel.connect_material_expressions(hardness, "", outline, "Hardness")
mel.connect_material_expressions(line_opacity, "", outline, "LineOpacity")

mel.connect_material_property(outline, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

mel.recompile_material(mat)
if not eal.save_asset(FULL_PATH):
    raise RuntimeError(f"{MAT_NAME}: save failed")
unreal.log(f"{MAT_NAME}: authored and saved ({FULL_PATH})")
