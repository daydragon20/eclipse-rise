# gen_building_kit.py — ECLIPSE Kessara modular building kit (GDD 15.5, Borderlands-lean)
# Run headless:
#   & "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup --python gen_building_kit.py
#
# Conventions:
#   - Units: meters (scene METRIC, scale 1.0). FBX export axis_forward='-Y', axis_up='Z',
#     global_scale=1.0, apply_unit_scale -> 1 m = 100 UE units.
#   - Origin: floor-level center of each module. +X = facade direction.
#   - One object per FBX, identity transform (transforms trivially applied).
#   - Style: chunky exaggerated trims, all edges faceted with bevel modifier
#     (segments=2, width 0.03-0.05; gantry bars use 0.015 to avoid degenerate 0.08 bars).
#   - No materials/UVs (world-aligned toon materials in-engine). Low/mid poly, < 5000 tris each.

import bpy
import bmesh
import math
import os
import sys
from mathutils import Vector

OUT_DIR = r"C:\Dev\ECLIPSE_GDD\Eclipse\Saved\BlenderKit"
MIN_BYTES = 5 * 1024
MAX_TRIS = 5000


# ---------------------------------------------------------------- geometry builder

class MB:
    """Accumulates box/bar/frustum primitives as one vert/face soup."""

    def __init__(self):
        self.verts = []
        self.faces = []

    def add_box(self, center, size, top_scale=(1.0, 1.0), bottom_scale=(1.0, 1.0)):
        cx, cy, cz = center
        hx, hy, hz = size[0] / 2.0, size[1] / 2.0, size[2] / 2.0
        tsx, tsy = top_scale
        bsx, bsy = bottom_scale
        i = len(self.verts)
        self.verts += [
            (cx - hx * bsx, cy - hy * bsy, cz - hz),
            (cx + hx * bsx, cy - hy * bsy, cz - hz),
            (cx + hx * bsx, cy + hy * bsy, cz - hz),
            (cx - hx * bsx, cy + hy * bsy, cz - hz),
            (cx - hx * tsx, cy - hy * tsy, cz + hz),
            (cx + hx * tsx, cy - hy * tsy, cz + hz),
            (cx + hx * tsx, cy + hy * tsy, cz + hz),
            (cx - hx * tsx, cy + hy * tsy, cz + hz),
        ]
        self._box_faces(i)

    def add_bar(self, p0, p1, thickness):
        """Square-section bar from p0 to p1 (for truss diagonals)."""
        p0 = Vector(p0)
        p1 = Vector(p1)
        axis = (p1 - p0).normalized()
        up = Vector((0.0, 0.0, 1.0))
        if abs(axis.dot(up)) > 0.99:
            up = Vector((1.0, 0.0, 0.0))
        side = axis.cross(up).normalized()
        up2 = side.cross(axis).normalized()
        h = thickness / 2.0
        i = len(self.verts)
        for end in (p0, p1):
            for a, b in ((-h, -h), (h, -h), (h, h), (-h, h)):
                v = end + side * a + up2 * b
                self.verts.append((v.x, v.y, v.z))
        self._box_faces(i)

    def add_frustum(self, center_xy, z0, z1, r0, r1, sides=12):
        cx, cy = center_xy
        i = len(self.verts)
        for r, z in ((r0, z0), (r1, z1)):
            for j in range(sides):
                a = 2.0 * math.pi * j / sides
                self.verts.append((cx + r * math.cos(a), cy + r * math.sin(a), z))
        for j in range(sides):
            k = (j + 1) % sides
            self.faces.append((i + j, i + k, i + sides + k, i + sides + j))
        self.faces.append(tuple(i + j for j in range(sides - 1, -1, -1)))      # bottom cap
        self.faces.append(tuple(i + sides + j for j in range(sides)))          # top cap

    def _box_faces(self, i):
        self.faces += [
            (i + 0, i + 3, i + 2, i + 1),
            (i + 4, i + 5, i + 6, i + 7),
            (i + 0, i + 1, i + 5, i + 4),
            (i + 1, i + 2, i + 6, i + 5),
            (i + 2, i + 3, i + 7, i + 6),
            (i + 3, i + 0, i + 4, i + 7),
        ]


# ---------------------------------------------------------------- scene helpers

def wipe_scene():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    for m in list(bpy.data.meshes):
        if m.users == 0:
            bpy.data.meshes.remove(m)
    sc = bpy.context.scene
    sc.unit_settings.system = 'METRIC'
    sc.unit_settings.scale_length = 1.0
    try:
        sc.unit_settings.length_unit = 'METERS'
    except Exception:
        pass


def _recalc(mesh):
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()


def make_object(name, builder):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(builder.verts, [], builder.faces)
    mesh.validate()
    _recalc(mesh)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def merge_into(obj, builder):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    new_verts = [bm.verts.new(v) for v in builder.verts]
    for f in builder.faces:
        bm.faces.new([new_verts[i] for i in f])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(obj.data)
    bm.free()


def remove_object(o):
    m = o.data
    bpy.data.objects.remove(o, do_unlink=True)
    if m is not None and m.users == 0:
        bpy.data.meshes.remove(m)


def finalize_modifiers(obj):
    """Apply all modifiers via depsgraph evaluation (headless-safe, no ops)."""
    bpy.context.view_layer.update()
    deps = bpy.context.evaluated_depsgraph_get()
    new_mesh = bpy.data.meshes.new_from_object(obj.evaluated_get(deps))
    new_mesh.name = obj.name
    old = obj.data
    obj.modifiers.clear()
    obj.data = new_mesh
    if old.users == 0:
        bpy.data.meshes.remove(old)


def tri_count(obj):
    obj.data.calc_loop_triangles()
    return len(obj.data.loop_triangles)


def build_module(name, base, cutters=None, trims=None, bevel_width=0.04, bevel_angle=30.0):
    obj = make_object(name, base)
    if cutters:
        cutter_objs = []
        for idx, cb in enumerate(cutters):
            cut = make_object("%s_cut%d" % (name, idx), cb)
            mod = obj.modifiers.new("Cut%d" % idx, 'BOOLEAN')
            mod.operation = 'DIFFERENCE'
            mod.object = cut
            try:
                mod.solver = 'EXACT'
            except Exception:
                pass
            cutter_objs.append(cut)
        finalize_modifiers(obj)
        for c in cutter_objs:
            remove_object(c)
    if trims is not None and trims.verts:
        merge_into(obj, trims)
    bev = obj.modifiers.new("EdgeBevel", 'BEVEL')
    bev.width = bevel_width
    bev.segments = 2
    bev.limit_method = 'ANGLE'
    bev.angle_limit = math.radians(bevel_angle)
    bev.use_clamp_overlap = True
    finalize_modifiers(obj)
    for p in obj.data.polygons:
        p.use_smooth = False
    return obj


# ---------------------------------------------------------------- FBX export

def export_fbx(obj, path):
    for o in bpy.data.objects:
        o.select_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    attempts = [
        ("export_scene.fbx", dict(
            filepath=path, use_selection=True, object_types={'MESH'},
            axis_forward='-Y', axis_up='Z', global_scale=1.0,
            apply_unit_scale=True, apply_scale_options='FBX_SCALE_NONE',
            mesh_smooth_type='FACE', use_mesh_modifiers=True,
            add_leaf_bones=False, bake_anim=False)),
        ("export_scene.fbx", dict(
            filepath=path, use_selection=True,
            axis_forward='-Y', axis_up='Z', global_scale=1.0)),
        ("wm.fbx_export", dict(
            filepath=path, export_selected_objects=True,
            forward_axis='NEGATIVE_Y', up_axis='Z', global_scale=1.0)),
        ("wm.fbx_export", dict(filepath=path)),
    ]
    last_err = None
    for opname, kwargs in attempts:
        cat, opn = opname.split('.')
        op = getattr(getattr(bpy.ops, cat), opn)
        try:
            op(**kwargs)
            return opname
        except (TypeError, AttributeError, RuntimeError) as e:
            last_err = e
    raise RuntimeError("FBX export failed for %s: %s" % (path, last_err))


# ---------------------------------------------------------------- modules
# Wall coordinate frame: facade normal +X, wall spans Y (width) and Z (height),
# slab thickness x in [-0.15, 0.15], floor at z = 0.

def slab(builder):
    builder.add_box((0.0, 0.0, 1.75), (0.3, 4.0, 3.5))


def mod_wallpanel():
    base = MB()
    slab(base)
    c1 = MB(); c1.add_box((0.15, -0.95, 1.45), (0.12, 1.6, 2.1))   # inset 0.06 deep
    c2 = MB(); c2.add_box((0.15,  0.95, 1.45), (0.12, 1.6, 2.1))
    t = MB()
    t.add_box((0.20, 0.0, 2.80), (0.16, 4.0, 0.18))                # ledge @ 2.8 m
    t.add_box((0.19, 0.0, 0.11), (0.14, 4.0, 0.22))                # base skirt
    return build_module("SM_Kit_WallPanel", base, [c1, c2], t)


def mod_wallwindow():
    base = MB()
    slab(base)
    cut = MB(); cut.add_box((0.10, 0.0, 1.60), (0.30, 1.8, 1.4))   # recessed opening, 0.2 deep
    t = MB()
    t.add_box((0.085, -0.97, 1.60), (0.29, 0.18, 1.76))            # jamb L
    t.add_box((0.085,  0.97, 1.60), (0.29, 0.18, 1.76))            # jamb R
    t.add_box((0.085,  0.00, 2.37), (0.29, 2.12, 0.18))            # lintel
    t.add_box((0.100,  0.00, 0.835), (0.34, 2.20, 0.17))           # chunky sill
    return build_module("SM_Kit_WallWindow", base, [cut], t)


def mod_doorway():
    base = MB()
    slab(base)
    cut = MB(); cut.add_box((0.0, 0.0, 1.15), (0.5, 1.2, 2.5))     # through opening 1.2 x 2.4
    t = MB()
    t.add_box((0.095, -0.675, 1.31), (0.31, 0.25, 2.62))           # heavy jamb L
    t.add_box((0.095,  0.675, 1.31), (0.31, 0.25, 2.62))           # heavy jamb R
    t.add_box((0.095,  0.000, 2.51), (0.31, 1.90, 0.26))           # heavy lintel
    t.add_box((0.450,  0.000, 2.80), (0.74, 1.90, 0.12))           # canopy plate
    t.add_box((0.420, -0.820, 2.60), (0.62, 0.07, 0.32), bottom_scale=(0.25, 1.0))  # gusset L
    t.add_box((0.420,  0.820, 2.60), (0.62, 0.07, 0.32), bottom_scale=(0.25, 1.0))  # gusset R
    return build_module("SM_Kit_Doorway", base, [cut], t)


def mod_cornerpillar():
    t = MB()
    t.add_box((0.0, 0.0, 0.175), (0.80, 0.80, 0.35))               # plinth
    t.add_box((0.0, 0.0, 1.850), (0.60, 0.60, 3.10))               # shaft
    t.add_box((0.0, 0.0, 3.420), (0.70, 0.70, 0.12))               # neck ring
    t.add_box((0.0, 0.0, 3.590), (0.72, 0.72, 0.22), top_scale=(0.6, 0.6))  # chamfered head
    return build_module("SM_Kit_CornerPillar", MB(), None, t)


def mod_rooftrim():
    t = MB()
    t.add_box((0.00, 0.0, 0.20), (0.50, 4.0, 0.40))                # main beam
    t.add_box((0.27, 0.0, 0.34), (0.10, 4.0, 0.12))                # top nose strip
    for i in range(8):                                             # dentils every 0.5 m
        y = -1.75 + i * 0.5
        t.add_box((0.28, y, 0.10), (0.14, 0.22, 0.20))
    return build_module("SM_Kit_RoofTrim", MB(), None, t)


def mod_chimney():
    def r_at(z):
        return 0.60 + (0.34 - 0.60) * (z - 0.3) / 5.4
    t = MB()
    t.add_box((0.0, 0.0, 0.225), (1.45, 1.45, 0.45))               # plinth
    t.add_frustum((0.0, 0.0), 0.30, 5.70, 0.60, 0.34, 12)          # tapered stack, 1.2 m foot
    t.add_frustum((0.0, 0.0), 2.05, 2.35, r_at(2.2) + 0.12, r_at(2.2) + 0.12, 12)  # ring 1
    t.add_frustum((0.0, 0.0), 3.85, 4.15, r_at(4.0) + 0.12, r_at(4.0) + 0.12, 12)  # ring 2
    t.add_frustum((0.0, 0.0), 5.68, 6.00, 0.45, 0.45, 12)          # crown lip -> 6 m total
    return build_module("SM_Kit_Chimney", MB(), None, t, bevel_width=0.035, bevel_angle=20.0)


def mod_gantrybeam():
    t = MB()
    for sx in (-1.0, 1.0):                                         # 4 chords, 6 m long
        for cz in (0.04, 0.46):
            t.add_box((0.21 * sx, 0.0, cz), (0.08, 6.0, 0.08))
    for y in (-2.96, -2.0, 0.0, 2.0, 2.96):                        # verticals
        for sx in (-1.0, 1.0):
            t.add_box((0.21 * sx, y, 0.25), (0.08, 0.08, 0.50))
    for sx in (-1.0, 1.0):                                         # X-pattern per 1 m bay
        x = 0.21 * sx
        for i in range(6):
            y0 = -3.0 + i * 1.0 + 0.04
            y1 = y0 + 0.92
            t.add_bar((x, y0, 0.04), (x, y1, 0.46), 0.08)
            t.add_bar((x, y0, 0.46), (x, y1, 0.04), 0.08)
    return build_module("SM_Kit_GantryBeam", MB(), None, t, bevel_width=0.015)


# ---------------------------------------------------------------- main

def main():
    print("Blender:", bpy.app.version_string)
    os.makedirs(OUT_DIR, exist_ok=True)
    wipe_scene()

    builders = [
        mod_wallpanel,
        mod_wallwindow,
        mod_doorway,
        mod_cornerpillar,
        mod_rooftrim,
        mod_chimney,
        mod_gantrybeam,
    ]

    failures = []
    for fn in builders:
        obj = fn()
        tris = tri_count(obj)
        path = os.path.join(OUT_DIR, obj.name + ".fbx")
        used_op = export_fbx(obj, path)
        size = os.path.getsize(path) if os.path.isfile(path) else 0
        ok = size >= MIN_BYTES and tris < MAX_TRIS
        print("[%s] %s  tris=%d  bytes=%d  op=%s" %
              ("OK " if ok else "BAD", os.path.basename(path), tris, size, used_op))
        if not ok:
            failures.append(obj.name)

    if failures:
        print("KIT_RESULT: FAILED ->", ", ".join(failures))
        sys.exit(1)
    print("KIT_RESULT: SUCCESS (7 modules)")


if __name__ == "__main__":
    main()
