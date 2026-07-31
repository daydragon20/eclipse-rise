# gen_weapons.py — ECLIPSE wapenfamilies als LOSSE meshes (O-5 "volledig", GDD 15.5)
#
# Draaien (headless):
#   & "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup \
#       --python C:\Dev\ECLIPSE_GDD\Eclipse\Tools\blender\gen_weapons.py
#
# Levert vier FBX'en in Eclipse/Saved/BlenderWeapons/, een per rij in DT_Weapons:
#   SM_Weapon_AR_Foundry.fbx        allrounder aanvalsgeweer
#   SM_Weapon_SMG_Patch.fbx         korte, dikke SMG
#   SM_Weapon_DMR_Longsight.fbx     lange scherpschutter
#   SM_Weapon_Sidearm_Scrap.fbx     gedempt pistool
#
# ---------------------------------------------------------------- DE CONVENTIE
# Dit is het enige dat de C++-kant hoeft te weten, en het staat hier omdat het
# asset de autoriteit is over "welke kant wijst dit ding op":
#
#   OORSPRONG  = de GREEP. Precies waar de rechterhand omheen sluit. Niet het
#                midden van het wapen, niet de onderkant — de greep. Daardoor is
#                de aanhechting aan hand_r een vaste rig-correctie en niet een
#                getal per wapen.
#   +X         = de LOOP, richting waar de kogel heen gaat.
#   +Z         = BOVENkant (vizier/rail).
#   +Y         = links van de schutter.
#
# De FBX-export gebruikt dezelfde as-instelling als gen_street_props.py, zodat er
# maar EEN conventie in dit project bestaat. Welke as de loop na de import in UE
# is, wordt daar GEMETEN en niet aangenomen — de lengte-as van een geweer is
# ondubbelzinnig (lengte >> breedte en hoogte), dus die meting kan niet twee
# antwoorden geven.
#
# ------------------------------------------------------------------- DE STIJL
# Borderlands-leaning (15.5): overdreven dikke onderdelen, een magazijn dat te
# groot is voor het wapen, zware afkanting op elke harde rand, en een silhouet dat
# op 40 meter nog te herkennen is. Geen fotorealistische detaillering, geen
# low-poly hoekigheid — de afkanting doet het werk.
#
# De MAAT is niet gestileerd, alleen de vorm. Een aanvalsgeweer is 0,85-0,90 m in
# werkelijkheid en dat blijft het hier, want de maat bepaalt hoe het in de handen
# van een personage van 1,80 m staat. Overdrijven daar geeft een speelgoedgeweer
# of een kanon, en beide zijn zichtbaar fout naast een lichaam.

import bpy
import math
import os

OUT_DIR = r"C:\Dev\ECLIPSE_GDD\Eclipse\Saved\BlenderWeapons"
TRI_BUDGET = 4000

# ----------------------------------------------------------------------- helpers


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


def box(name, sx, sy, sz, loc=(0, 0, 0), rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=loc, rotation=rot)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = (sx, sy, sz)
    _activate(obj)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return obj


def cyl(name, radius, depth, loc=(0, 0, 0), rot=(0, 0, 0), verts=12):
    """Cilinder LANGS X als rot leeg blijft — de loop-as van dit project."""
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=radius, depth=depth,
                                        location=loc,
                                        rotation=(rot[0], rot[1] + math.pi / 2.0, rot[2]))
    obj = bpy.context.active_object
    obj.name = name
    return obj


def cyl_z(name, radius, depth, loc=(0, 0, 0), rot=(0, 0, 0), verts=12):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=radius, depth=depth,
                                        location=loc, rotation=rot)
    obj = bpy.context.active_object
    obj.name = name
    return obj


def cone_x(name, r1, r2, depth, loc=(0, 0, 0), verts=12):
    bpy.ops.mesh.primitive_cone_add(vertices=verts, radius1=r1, radius2=r2, depth=depth,
                                    location=loc, rotation=(0, math.pi / 2.0, 0))
    obj = bpy.context.active_object
    obj.name = name
    return obj


def bevel(obj, width=0.006, segments=2, angle=40.0):
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


def tri_count(obj):
    obj.data.calc_loop_triangles()
    return len(obj.data.loop_triangles)


def bbox_of(obj):
    """Wereld-bbox als (min, max) — de meting die dit script zelf afdrukt."""
    xs, ys, zs = [], [], []
    for corner in obj.bound_box:
        world = obj.matrix_world @ __import__("mathutils").Vector(corner)
        xs.append(world.x)
        ys.append(world.y)
        zs.append(world.z)
    return (min(xs), min(ys), min(zs)), (max(xs), max(ys), max(zs))


def export_fbx(path, obj):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

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
            except TypeError as exc:
                print("  [export retry] %s" % exc)
            except AttributeError:
                break
    native = [
        dict(filepath=path, export_selected_objects=True,
             forward_axis='NEGATIVE_Y', up_axis='Z'),
        dict(filepath=path, export_selected_objects=True),
        dict(filepath=path),
    ]
    if hasattr(bpy.ops.wm, "fbx_export"):
        for kw in native:
            try:
                bpy.ops.wm.fbx_export(**kw)
                return "wm.fbx_export"
            except TypeError as exc:
                print("  [export retry] %s" % exc)
    raise RuntimeError("geen bruikbare FBX-exporteur voor " + path)


# --------------------------------------------------------------- gedeelde delen


def pistol_grip(prefix, x=0.0, top_z=-0.015, length=0.135, tilt_deg=16.0,
                width=0.038, depth=0.062):
    """De greep waar de OORSPRONG doorheen loopt.

    De hand sluit rond het bovenste derde deel, dus de greep hangt onder z=0 en
    de oorsprong ligt er net bovenin. Dat is de hele reden dat de aanhechting aan
    hand_r straks een vaste rig-correctie is: elk wapen presenteert zijn greep op
    dezelfde plek.
    """
    tilt = math.radians(tilt_deg)
    cx = x - math.sin(tilt) * length * 0.5
    cz = top_z - math.cos(tilt) * length * 0.5
    return box(prefix + "Grip", depth, width, length, loc=(cx, 0, cz), rot=(0, tilt, 0))


def trigger_guard(prefix, x=0.055, z=-0.028, r=0.030):
    ring = cyl_z(prefix + "Guard", radius=r, depth=0.026, loc=(x, 0, z), verts=10)
    ring.rotation_euler = (math.pi / 2.0, 0, 0)
    _activate(ring)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    return ring


def rail_sight(prefix, x, z, length=0.10, tall=0.030):
    """Een BLOKKIG vizier. Borderlands leest een wapen aan zijn bovenkant af, dus
    dit onderdeel is met opzet te groot voor zijn functie."""
    parts = [box(prefix + "Rail", length, 0.030, 0.012, loc=(x, 0, z))]
    parts.append(box(prefix + "SightBlock", 0.045, 0.042, tall, loc=(x + length * 0.28, 0, z + tall * 0.5)))
    parts.append(box(prefix + "SightHood", 0.014, 0.050, 0.014,
                     loc=(x + length * 0.28 + 0.022, 0, z + tall * 0.72)))
    return parts


# ------------------------------------------------------------------- de wapens


def build_ar_foundry():
    """AR_Foundry — de allrounder. 0,88 m, zware kast, magazijn te groot."""
    p = []
    # Kast: het herkenningspunt. Dik, recht, met een schuine bovenkant.
    p.append(box("Receiver", 0.30, 0.072, 0.105, loc=(0.115, 0, 0.038)))
    p.append(box("ReceiverTop", 0.26, 0.060, 0.030, loc=(0.115, 0, 0.100)))
    # Handbescherming: octagonaal aandoend blok, overdreven dik.
    p.append(box("Handguard", 0.215, 0.062, 0.070, loc=(0.365, 0, 0.030)))
    p.append(box("HandguardVent", 0.150, 0.070, 0.022, loc=(0.365, 0, 0.030)))
    # Loop + remmer.
    p.append(cyl("Barrel", radius=0.0135, depth=0.20, loc=(0.545, 0, 0.030), verts=10))
    p.append(box("MuzzleBrake", 0.058, 0.042, 0.042, loc=(0.660, 0, 0.030)))
    p.append(cone_x("MuzzleLip", r1=0.021, r2=0.014, depth=0.020, loc=(0.696, 0, 0.030)))
    # Magazijn: gebogen, en met opzet te lang — dit is het silhouetdetail.
    p.append(box("Magazine", 0.058, 0.040, 0.185, loc=(0.150, 0, -0.098), rot=(0, math.radians(-9), 0)))
    p.append(box("MagBase", 0.070, 0.050, 0.024, loc=(0.180, 0, -0.190)))
    # Greep + trekkerbeugel rond de oorsprong.
    p.append(pistol_grip(""))
    p.append(trigger_guard("", x=0.062, z=-0.026, r=0.030))
    # Kolf: zware buis met een dikke schouderplaat.
    p.append(box("StockTube", 0.185, 0.048, 0.048, loc=(-0.115, 0, 0.030)))
    p.append(box("StockPlate", 0.036, 0.070, 0.115, loc=(-0.225, 0, 0.024)))
    p.append(box("Cheek", 0.120, 0.040, 0.026, loc=(-0.150, 0, 0.068)))
    p += rail_sight("", x=0.150, z=0.122, length=0.170, tall=0.030)
    for o in p:
        bevel(o, width=0.005)
    return join_objects(p, "SM_Weapon_AR_Foundry")


def build_smg_patch():
    """SMG_Patch — kort en dik. 0,53 m, magazijn van 40 dus zichtbaar lang."""
    p = []
    p.append(box("Receiver", 0.235, 0.078, 0.108, loc=(0.095, 0, 0.040)))
    p.append(box("ReceiverTop", 0.190, 0.062, 0.026, loc=(0.095, 0, 0.104)))
    p.append(box("Shroud", 0.130, 0.070, 0.070, loc=(0.270, 0, 0.034)))
    p.append(cyl("Barrel", radius=0.0125, depth=0.085, loc=(0.375, 0, 0.034), verts=10))
    p.append(box("MuzzleBlock", 0.040, 0.044, 0.044, loc=(0.435, 0, 0.034)))
    # Het magazijn van 40: recht, lang, en dat is hier de leesbare eigenschap.
    p.append(box("Magazine", 0.048, 0.038, 0.235, loc=(0.110, 0, -0.120)))
    p.append(box("MagBase", 0.062, 0.048, 0.022, loc=(0.110, 0, -0.245)))
    p.append(pistol_grip("", length=0.128, tilt_deg=13.0))
    p.append(trigger_guard("", x=0.056, z=-0.026, r=0.028))
    # Inklapbare kolf als DRAADFRAME: twee stangen plus plaat. Silhouetgat, en dat
    # is precies wat een SMG van een geweer onderscheidt op afstand.
    for side in (-0.028, 0.028):
        p.append(box("StockRod%d" % int(side * 1000), 0.150, 0.014, 0.014, loc=(-0.090, side, 0.036)))
    p.append(box("StockPlate", 0.028, 0.074, 0.086, loc=(-0.172, 0, 0.030)))
    p += rail_sight("", x=0.100, z=0.124, length=0.130, tall=0.026)
    for o in p:
        bevel(o, width=0.005)
    return join_objects(p, "SM_Weapon_SMG_Patch")


def build_dmr_longsight():
    """DMR_Longsight — 1,14 m. Lange dunne loop, kijker die te groot is."""
    p = []
    p.append(box("Receiver", 0.330, 0.070, 0.110, loc=(0.135, 0, 0.040)))
    p.append(box("ReceiverTop", 0.290, 0.058, 0.026, loc=(0.135, 0, 0.106)))
    p.append(box("Handguard", 0.300, 0.056, 0.062, loc=(0.450, 0, 0.032)))
    # Slanke, lange loop — het tegendeel van de SMG.
    p.append(cyl("Barrel", radius=0.0115, depth=0.300, loc=(0.740, 0, 0.032), verts=10))
    p.append(box("Bipod", 0.024, 0.020, 0.090, loc=(0.585, 0, -0.030), rot=(0, math.radians(20), 0)))
    p.append(box("BipodB", 0.024, 0.020, 0.090, loc=(0.585, 0, -0.030), rot=(0, math.radians(-20), 0)))
    p.append(box("MuzzleBrake", 0.075, 0.046, 0.046, loc=(0.925, 0, 0.032)))
    p.append(cone_x("MuzzleLip", r1=0.023, r2=0.015, depth=0.024, loc=(0.972, 0, 0.032)))
    # Kijker: dik, met twee hoge beugels. Dit IS de leesbare eigenschap.
    p.append(cyl("Scope", radius=0.034, depth=0.260, loc=(0.190, 0, 0.170), verts=14))
    p.append(cone_x("ScopeBell", r1=0.046, r2=0.034, depth=0.055, loc=(0.348, 0, 0.170)))
    p.append(cyl("ScopeEye", radius=0.030, depth=0.045, loc=(0.038, 0, 0.170), verts=12))
    for mx in (0.085, 0.275):
        p.append(box("ScopeRing%d" % int(mx * 1000), 0.026, 0.046, 0.062, loc=(mx, 0, 0.135)))
    # Magazijn van 10: kort, en dat is meteen zichtbaar tegen de SMG.
    p.append(box("Magazine", 0.056, 0.040, 0.108, loc=(0.160, 0, -0.062), rot=(0, math.radians(-6), 0)))
    p.append(box("MagBase", 0.068, 0.050, 0.020, loc=(0.172, 0, -0.118)))
    p.append(pistol_grip("", length=0.140, tilt_deg=18.0))
    p.append(trigger_guard("", x=0.066, z=-0.026, r=0.030))
    p.append(box("StockTube", 0.190, 0.052, 0.070, loc=(-0.110, 0, 0.032)))
    p.append(box("StockPlate", 0.040, 0.076, 0.140, loc=(-0.225, 0, 0.014)))
    p.append(box("Cheek", 0.150, 0.046, 0.036, loc=(-0.130, 0, 0.082)))
    for o in p:
        bevel(o, width=0.005)
    return join_objects(p, "SM_Weapon_DMR_Longsight")


def build_sidearm_scrap():
    """Sidearm_Scrap — 0,32 m MET demper. Gedempt is een DATA-feit (bSuppressed),
    dus het hoort ook zichtbaar te zijn: de demper is het silhouet."""
    p = []
    p.append(box("Slide", 0.150, 0.046, 0.058, loc=(0.070, 0, 0.030)))
    p.append(box("SlideTop", 0.120, 0.034, 0.014, loc=(0.070, 0, 0.064)))
    p.append(box("Frame", 0.115, 0.040, 0.030, loc=(0.052, 0, -0.006)))
    # De demper: dik, lang, ontegenzeggelijk aanwezig.
    p.append(cyl("Suppressor", radius=0.026, depth=0.130, loc=(0.208, 0, 0.030), verts=12))
    p.append(cyl("SuppressorCap", radius=0.029, depth=0.016, loc=(0.268, 0, 0.030), verts=12))
    p.append(cyl("SuppressorRing", radius=0.029, depth=0.014, loc=(0.152, 0, 0.030), verts=12))
    p.append(pistol_grip("", length=0.118, tilt_deg=20.0, width=0.034, depth=0.052))
    p.append(trigger_guard("", x=0.048, z=-0.028, r=0.026))
    p.append(box("MagBase", 0.052, 0.042, 0.014, loc=(-0.030, 0, -0.118)))
    p.append(box("RearSight", 0.016, 0.036, 0.016, loc=(0.012, 0, 0.072)))
    p.append(box("FrontSight", 0.012, 0.014, 0.016, loc=(0.128, 0, 0.072)))
    for o in p:
        bevel(o, width=0.004)
    return join_objects(p, "SM_Weapon_Sidearm_Scrap")


# -------------------------------------------------------------------- uitvoeren

BUILDERS = [
    ("SM_Weapon_AR_Foundry", build_ar_foundry),
    ("SM_Weapon_SMG_Patch", build_smg_patch),
    ("SM_Weapon_DMR_Longsight", build_dmr_longsight),
    ("SM_Weapon_Sidearm_Scrap", build_sidearm_scrap),
]

os.makedirs(OUT_DIR, exist_ok=True)
print("=" * 72)
print("ECLIPSE wapenmeshes — greep op de oorsprong, loop langs +X")
print("=" * 72)
for name, builder in BUILDERS:
    clear_scene()
    obj = builder()
    _activate(obj)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    tris = tri_count(obj)
    lo, hi = bbox_of(obj)
    length_m = hi[0] - lo[0]
    path = os.path.join(OUT_DIR, name + ".fbx")
    used = export_fbx(path, obj)
    status = "OK" if tris <= TRI_BUDGET else "BOVEN BUDGET"
    print("%-30s %5d tris (%s)  lengte=%.3f m  bbox X[%.3f..%.3f] Y[%.3f..%.3f] Z[%.3f..%.3f]  via %s"
          % (name, tris, status, length_m, lo[0], hi[0], lo[1], hi[1], lo[2], hi[2], used))
    if tris > TRI_BUDGET:
        raise RuntimeError("%s boven het driehoeksbudget (%d > %d)" % (name, tris, TRI_BUDGET))
print("=" * 72)
print("klaar — %s" % OUT_DIR)
