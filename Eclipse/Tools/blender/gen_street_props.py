# gen_street_props.py — ECLIPSE stylized street/checkpoint props (Borderlands-leaning, GDD 15.5)
# Run headless:
#   & "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup --python gen_street_props.py
#
# Generates 5 props, each exported as its own FBX to Eclipse/Saved/BlenderProps/:
#   SM_Prop_SodiumLamp.fbx   (+ separate "GlowPlane" object)
#   SM_Prop_VentUnit.fbx
#   SM_Prop_CableArc.fbx
#   SM_Prop_PropagandaBoard.fbx (+ separate "PosterPlane" object)
#   SM_Prop_Barricade.fbx
# Style rules: big readable silhouettes, exaggerated thicknesses, bevel (segments=2,
# width 0.02-0.04) on hard edges, < 4000 tris per prop, no materials/UVs, transforms applied.

import bpy
import math
import os
import sys

OUT_DIR = r"C:\Dev\ECLIPSE_GDD\Eclipse\Saved\BlenderProps"
TRI_BUDGET = 4000

# ----------------------------------------------------------------------------- helpers

def clear_scene():
    if bpy.context.mode != 'OBJECT' and bpy.context.view_layer.objects.active:
        bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.select_all(action='SELECT')
    if bpy.context.selected_objects:
        bpy.ops.object.delete(use_global=False)
    for coll in (bpy.data.meshes, bpy.data.curves):
        for block in list(coll):
            if block.users == 0:
                coll.remove(block)


def _activate(obj):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


def make_box(name, sx, sy, sz, loc=(0, 0, 0), rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=loc, rotation=rot)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = (sx, sy, sz)
    _activate(obj)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return obj


def make_cyl(name, radius, depth, loc=(0, 0, 0), rot=(0, 0, 0), verts=12):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=radius, depth=depth,
                                        location=loc, rotation=rot)
    obj = bpy.context.active_object
    obj.name = name
    return obj


def make_cone(name, r1, r2, depth, loc=(0, 0, 0), rot=(0, 0, 0), verts=12):
    bpy.ops.mesh.primitive_cone_add(vertices=verts, radius1=r1, radius2=r2, depth=depth,
                                    location=loc, rotation=rot)
    obj = bpy.context.active_object
    obj.name = name
    return obj


def make_plane(name, sx, sy, loc=(0, 0, 0), rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_plane_add(size=1.0, location=loc, rotation=rot)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = (sx, sy, 1.0)
    _activate(obj)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return obj


def add_bevel(obj, width=0.03, segments=2, angle=40.0):
    mod = obj.modifiers.new(name="EdgeBevel", type='BEVEL')
    mod.width = width
    mod.segments = segments
    mod.limit_method = 'ANGLE'
    mod.angle_limit = math.radians(angle)
    _activate(obj)
    bpy.ops.object.modifier_apply(modifier=mod.name)
    return obj


def join_objects(objs, name):
    _activate(objs[0])
    for o in objs:
        o.select_set(True)
    bpy.ops.object.join()
    obj = bpy.context.active_object
    obj.name = name
    obj.data.name = name
    return obj


def finalize(obj):
    """Apply all transforms so the pivot sits at the world origin (ground-center)."""
    _activate(obj)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    return obj


def tri_count(objs):
    total = 0
    for o in objs:
        if o.type == 'MESH':
            o.data.calc_loop_triangles()
            total += len(o.data.loop_triangles)
    return total


def make_cable(name, span, h_end, sag, yoff, n=17, radius=0.025):
    cu = bpy.data.curves.new(name + "_cu", 'CURVE')
    cu.dimensions = '3D'
    cu.fill_mode = 'FULL'
    cu.bevel_depth = radius
    cu.bevel_resolution = 1
    cu.use_fill_caps = True
    sp = cu.splines.new('POLY')
    sp.points.add(n - 1)
    half = span / 2.0
    for i in range(n):
        x = -half + span * i / (n - 1)
        z = h_end - sag * (1.0 - (x / half) ** 2)
        sp.points[i].co = (x, yoff, z, 1.0)
    obj = bpy.data.objects.new(name, cu)
    bpy.context.collection.objects.link(obj)
    _activate(obj)
    bpy.ops.object.convert(target='MESH')
    return bpy.context.active_object


def export_fbx(path, objs):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]

    attempts = [
        dict(filepath=path, use_selection=True, axis_forward='-Y', axis_up='Z',
             object_types={'MESH'}, mesh_smooth_type='FACE', add_leaf_bones=False,
             bake_anim=False, apply_unit_scale=True, use_mesh_modifiers=True),
        dict(filepath=path, use_selection=True, axis_forward='-Y', axis_up='Z'),
        dict(filepath=path, use_selection=True),
    ]
    if hasattr(bpy.ops.export_scene, "fbx"):
        for kw in attempts:
            try:
                bpy.ops.export_scene.fbx(**kw)
                return "export_scene.fbx"
            except TypeError as exc:  # unknown kwarg on this Blender version
                print("  [export retry] %s" % exc)
            except AttributeError:
                break
    # Fallback: native exporter naming style (newer Blender I/O operators)
    native_attempts = [
        dict(filepath=path, export_selected_objects=True,
             forward_axis='NEGATIVE_Y', up_axis='Z'),
        dict(filepath=path, export_selected_objects=True),
        dict(filepath=path),
    ]
    if hasattr(bpy.ops.wm, "fbx_export"):
        for kw in native_attempts:
            try:
                bpy.ops.wm.fbx_export(**kw)
                return "wm.fbx_export"
            except TypeError as exc:
                print("  [export retry] %s" % exc)
    raise RuntimeError("No usable FBX exporter found for " + path)

# ----------------------------------------------------------------------------- props

def build_sodium_lamp():
    parts = []
    # Heavy base, 0.5 m footprint (chunky, slightly tapered)
    parts.append(make_cyl("Base", radius=0.27, depth=0.30, loc=(0, 0, 0.15), verts=12))
    parts.append(make_cone("BaseCap", r1=0.20, r2=0.12, depth=0.14, loc=(0, 0, 0.37), verts=12))
    # Slanted mast, 4.5 m, exaggerated thickness (r=0.09)
    tilt = math.radians(8.0)
    d = (math.sin(tilt), 0.0, math.cos(tilt))
    base_top = (0.0, 0.0, 0.30)
    center = (base_top[0] + 2.25 * d[0], 0.0, base_top[2] + 2.25 * d[2])
    parts.append(make_cyl("Mast", radius=0.09, depth=4.5, loc=center,
                          rot=(0, tilt, 0), verts=10))
    top = (base_top[0] + 4.5 * d[0], 0.0, base_top[2] + 4.5 * d[2])
    # Overhanging arm + neck
    parts.append(make_box("Arm", 0.70, 0.14, 0.14, loc=(top[0] + 0.30, 0, top[2])))
    head_x = top[0] + 0.60
    parts.append(make_cyl("Neck", radius=0.07, depth=0.18,
                          loc=(head_x, 0, top[2] - 0.10), verts=10))
    # Wide hood, 0.8 m diameter
    cap_z = top[2] - 0.26
    parts.append(make_cone("Hood", r1=0.40, r2=0.14, depth=0.22,
                           loc=(head_x, 0, cap_z), verts=14))
    for p in parts:
        add_bevel(p, width=0.025)
    body = join_objects(parts, "SM_Prop_SodiumLamp")
    finalize(body)
    # Separate emissive plane under the hood, facing down (0.6 x 0.3)
    glow = make_plane("GlowPlane", 0.60, 0.30,
                      loc=(head_x, 0, cap_z - 0.13), rot=(math.pi, 0, 0))
    finalize(glow)
    return [body, glow]


def build_vent_unit():
    W, D, H = 1.2, 0.8, 0.9
    t = 0.14          # exaggerated frame thickness
    front_y = -D / 2.0
    parts = []
    # Main body (slightly recessed behind the front frame)
    parts.append(make_box("Body", W, D - 0.10, H, loc=(0, 0.05, H / 2.0)))
    # Thick front rim
    ry = front_y + 0.04
    parts.append(make_box("RimTop", W, t, t, loc=(0, ry, H - t / 2.0)))
    parts.append(make_box("RimBot", W, t, t, loc=(0, ry, t / 2.0)))
    parts.append(make_box("RimL", t, t, H - 2 * t, loc=(-W / 2.0 + t / 2.0, ry, H / 2.0)))
    parts.append(make_box("RimR", t, t, H - 2 * t, loc=(W / 2.0 - t / 2.0, ry, H / 2.0)))
    # 5 slanted louvers
    louver_w = W - 2 * t - 0.04
    z0, z1 = t + 0.10, H - t - 0.10
    for i in range(5):
        z = z0 + (z1 - z0) * i / 4.0
        parts.append(make_box("Louver%d" % i, louver_w, 0.16, 0.05,
                              loc=(0, front_y + 0.05, z),
                              rot=(math.radians(35), 0, 0)))
    # Small feet pads
    parts.append(make_box("FootL", 0.20, D - 0.20, 0.06, loc=(-W / 2.0 + 0.14, 0, 0.03)))
    parts.append(make_box("FootR", 0.20, D - 0.20, 0.06, loc=(W / 2.0 - 0.14, 0, 0.03)))
    for p in parts:
        add_bevel(p, width=0.02)
    body = join_objects(parts, "SM_Prop_VentUnit")
    finalize(body)
    return [body]


def build_cable_arc():
    span, h_end = 4.0, 3.0
    cables = []
    for i, (yoff, sag) in enumerate(((-0.07, 0.55), (0.0, 0.62), (0.07, 0.58))):
        cables.append(make_cable("Cable%d" % i, span, h_end, sag, yoff))
    parts = list(cables)
    # Mounting blocks at both ends (chunky)
    for sx, nm in ((-1.0, "MountL"), (1.0, "MountR")):
        blk = make_box(nm, 0.22, 0.30, 0.30, loc=(sx * span / 2.0, 0, h_end))
        add_bevel(blk, width=0.025)
        parts.append(blk)
    body = join_objects(parts, "SM_Prop_CableArc")
    finalize(body)
    return [body]


def build_propaganda_board():
    W, H, z0 = 2.2, 3.0, 0.5      # frame outer size, frame bottom height
    t, d = 0.18, 0.14             # exaggerated beam cross-section
    parts = []
    # 2 heavy legs
    for sx in (-1.0, 1.0):
        parts.append(make_box("Leg", 0.22, 0.22, 0.90, loc=(sx * 0.85, 0.02, 0.45)))
    # Industrial frame 2.2 x 3.0
    zc = z0 + H / 2.0
    parts.append(make_box("FrameL", t, d, H, loc=(-W / 2.0 + t / 2.0, 0, zc)))
    parts.append(make_box("FrameR", t, d, H, loc=(W / 2.0 - t / 2.0, 0, zc)))
    parts.append(make_box("FrameTop", W - 2 * t, d, t, loc=(0, 0, z0 + H - t / 2.0)))
    parts.append(make_box("FrameBot", W - 2 * t, d, t, loc=(0, 0, z0 + t / 2.0)))
    # Backing panel behind the poster
    parts.append(make_box("BackPanel", W - 0.24, 0.06, H - 0.24, loc=(0, 0.06, zc)))
    # 2 rear diagonal struts
    strut_rot = math.radians(-32)
    for sx in (-1.0, 1.0):
        parts.append(make_box("Strut", 0.12, 0.12, 1.7,
                              loc=(sx * 0.85, 0.48, 0.78), rot=(strut_rot, 0, 0)))
    for p in parts:
        add_bevel(p, width=0.025)
    body = join_objects(parts, "SM_Prop_PropagandaBoard")
    finalize(body)
    # Poster plane 2 x 2.8, just in front of the frame, facing -Y
    poster = make_plane("PosterPlane", 2.0, 2.8,
                        loc=(0, -d / 2.0 - 0.03, zc), rot=(math.pi / 2.0, 0, 0))
    finalize(poster)
    return [body, poster]


def build_barricade():
    parts = []
    # 3 criss-cross stacked beams, exaggerated 0.28 cross-section
    beams = (
        (math.radians(15), 0.14, 0.00),
        (math.radians(-12), 0.42, 0.05),
        (math.radians(8), 0.70, -0.04),
    )
    for i, (rz, z, yoff) in enumerate(beams):
        parts.append(make_box("Beam%d" % i, 1.95, 0.28, 0.28,
                              loc=(0, yoff, z), rot=(0, 0, rz)))
    for p in parts:
        add_bevel(p, width=0.03)
    # 2 sandbag-like rounded blocks (big bevel for the rounded read)
    bag1 = make_box("Sandbag0", 0.75, 0.50, 0.35, loc=(-0.62, -0.28, 0.175),
                    rot=(0, 0, math.radians(-18)))
    add_bevel(bag1, width=0.11, segments=3, angle=10)
    bag2 = make_box("Sandbag1", 0.70, 0.48, 0.34, loc=(0.52, 0.06, 0.94),
                    rot=(math.radians(4), 0, math.radians(22)))
    add_bevel(bag2, width=0.11, segments=3, angle=10)
    parts += [bag1, bag2]
    body = join_objects(parts, "SM_Prop_Barricade")
    finalize(body)
    return [body]

# ----------------------------------------------------------------------------- main

def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    builders = [
        ("SodiumLamp", build_sodium_lamp),
        ("VentUnit", build_vent_unit),
        ("CableArc", build_cable_arc),
        ("PropagandaBoard", build_propaganda_board),
        ("Barricade", build_barricade),
    ]
    report = []
    for name, fn in builders:
        clear_scene()
        objs = fn()
        tris = tri_count(objs)
        if tris >= TRI_BUDGET:
            raise RuntimeError("%s exceeds tri budget: %d" % (name, tris))
        path = os.path.join(OUT_DIR, "SM_Prop_%s.fbx" % name)
        used = export_fbx(path, objs)
        size = os.path.getsize(path)
        report.append((name, tris, size, used))
        print("[OK] SM_Prop_%s.fbx  tris=%d  bytes=%d  via=%s" % (name, tris, size, used))
    print("PROPS_DONE " + ", ".join("%s:%d tris/%d B" % (n, t, s) for n, t, s, _ in report))


if __name__ == "__main__":
    try:
        main()
    except Exception:
        import traceback
        traceback.print_exc()
        sys.exit(1)
