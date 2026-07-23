# Step-2 character pipeline data fill: creates /Game/Data/DT_BodyDefs
# (FEclipseBodyDefRow), fills the five Dominion archetypes in
# DT_EnemyArchetypes (GDD 09.3 flavor), and points DA_CampaignSetup.BodyDefs
# at the new table. Asset paths are RESOLVED from the asset registry per pack
# (never guessed); a pack without a usable mesh is skipped with a warning.
# Requires the compiled C++ with FEclipseBodyDefRow. Re-runnable.
#
# Run headless:
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\setup_character_data.py" \
#     -unattended -nopause -nosplash

import unreal

registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.wait_for_completion()
eal = unreal.EditorAssetLibrary


def find_assets(pack, cls):
    return [a for a in registry.get_assets_by_path(f"/Game/{pack}", recursive=True)
            if str(a.asset_class_path.asset_name) == cls]


def obj_path(asset):
    return f"{asset.package_name}.{asset.asset_name}"


def pick_anim(pack, *keywords):
    anims = find_assets(pack, "AnimSequence")
    for kw in keywords:
        for a in anims:
            if kw.lower() in str(a.asset_name).lower():
                return obj_path(a)
    return obj_path(anims[0]) if anims else ""


def pick_mesh(pack, prefer=None):
    meshes = find_assets(pack, "SkeletalMesh")
    if not meshes:
        return ""
    if prefer:
        for m in meshes:
            if prefer.lower() in str(m.asset_name).lower():
                return obj_path(m)
    # Largest asset name heuristic is meaningless; take the first sorted.
    return obj_path(sorted(meshes, key=lambda m: str(m.asset_name))[0])


# Row -> (mesh pack, anim pack) — anim pack may differ: the SciFi family shares
# the UE4-Mannequin skeleton, so anim-poor packs borrow from anim-rich ones.
BODIES = {
    "Player":       ("ParagonLtBelica", "ParagonLtBelica"),
    "Rebel_A":      ("SciFiSoldier", "SciFiCharacterPack"),
    "Rebel_B":      ("SciFiGirl", "SciFiGirl"),
    "Rebel_C":      ("SciFiCharacterPack", "SciFiCharacterPack"),
    "Enforcer":     ("SciFiSoldier03", "SciFiSoldier03"),
    "Trooper":      ("SciFiSoldier02", "SciFiSoldier02"),
    "Shock":        ("SciFiWarrior02", "SciFiWarrior02"),
    "Veil":         ("SciFiCharacter", "SciFiCharacter"),
    "RadiantGuard": ("SciFiWarrior", "SciFiCharacterPack"),
}

# Faction hue per body (15.5 palette discipline; saturated — near-neutral
# washes to gray at the x10 range): (TintLit, TintShade).
TINTS = {
    "Player":       ((0.30, 0.14, 0.06), (0.100, 0.050, 0.050)),  # Cinder ember
    "Rebel_A":      ((0.10, 0.16, 0.18), (0.040, 0.060, 0.080)),  # worker teal
    "Rebel_B":      ((0.12, 0.17, 0.16), (0.045, 0.065, 0.075)),
    "Rebel_C":      ((0.09, 0.14, 0.19), (0.035, 0.055, 0.085)),
    "Enforcer":     ((0.30, 0.235, 0.095), (0.090, 0.072, 0.055)),  # Dominion gold
    "Trooper":      ((0.26, 0.22, 0.11), (0.080, 0.070, 0.060)),
    "Shock":        ((0.28, 0.10, 0.06), (0.100, 0.040, 0.050)),   # assault red
    "Veil":         ((0.12, 0.09, 0.16), (0.045, 0.035, 0.070)),   # violet dark
    "RadiantGuard": ((0.32, 0.28, 0.16), (0.110, 0.100, 0.070)),   # radiant white-gold
}


def color(c):
    return f"\"(R={c[0]:.3f},G={c[1]:.3f},B={c[2]:.3f},A=1.0)\""


rows = []
for row_name, (mesh_pack, anim_pack) in BODIES.items():
    mesh = pick_mesh(mesh_pack)
    if not mesh:
        unreal.log_warning(f"{row_name}: geen skeletal mesh in {mesh_pack} — rij overgeslagen")
        continue
    idle = pick_anim(anim_pack, "idle")
    walk = pick_anim(anim_pack, "walk", "run", "jog")
    shoot = pick_anim(anim_pack, "shoot", "fire", "attack")
    death = pick_anim(anim_pack, "death", "die", "dead")
    lit, shade = TINTS.get(row_name, ((0.20, 0.20, 0.22), (0.07, 0.07, 0.09)))
    rows.append(f'{row_name},"{mesh}","{idle}","{walk}","{shoot}","{death}",true,{color(lit)},{color(shade)},1.0,-90.0,-90.0')

csv_body = "---,Mesh,IdleAnim,WalkAnim,ShootAnim,DeathAnim,bToonRestyle,TintLit,TintShade,MeshScale,MeshZOffset,MeshYaw\n" + "\n".join(rows)

DT_PATH = "/Game/Data/DT_BodyDefs"
if eal.does_asset_exist(DT_PATH):
    dt = eal.load_asset(DT_PATH)
else:
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", unreal.load_object(None, "/Script/Eclipse.EclipseBodyDefRow"))
    dt = unreal.AssetToolsHelpers.get_asset_tools().create_asset("DT_BodyDefs", "/Game/Data", unreal.DataTable, factory)

ok = unreal.DataTableFunctionLibrary.fill_data_table_from_csv_string(dt, csv_body)
unreal.log(f"DT_BodyDefs: fill={'OK' if ok else 'MISLUKT'} rows={len(rows)}")
eal.save_asset(DT_PATH)

# Dominion archetypes (GDD 09.3): readable-tell flavor in data, melee 200 overal.
ARCH_CSV = """---,BodyDef,Health,Damage,PerceptionRadius,FireInterval,EngageRange,MeleeRange
Enforcer,Enforcer,60,10,2500,0.8,600,200
Trooper,Trooper,80,12,2800,0.7,700,200
Shock,Shock,70,16,2200,0.9,350,200
Veil,Veil,50,14,3200,1.1,900,200
RadiantGuard,RadiantGuard,140,18,3000,0.6,800,200"""
arch = eal.load_asset("/Game/Data/DT_EnemyArchetypes")
ok = unreal.DataTableFunctionLibrary.fill_data_table_from_csv_string(arch, ARCH_CSV)
unreal.log(f"DT_EnemyArchetypes: fill={'OK' if ok else 'MISLUKT'} (5 archetypes)")
eal.save_asset("/Game/Data/DT_EnemyArchetypes")

# Named story-character slots (step 3): MetaHumanMesh blijft leeg tot de
# eigenaar MH_<Naam> maakt (phase0/metahuman_recipes.md); fallback dresses.
NAMED_CSV = """---,DisplayName,MetaHumanMesh,FallbackBodyDef,Faction
Kaya,"Kaya Renn","",Rebel_B,Rebel
Brick,"Oram \"\"Brick\"\" Bex","",Rebel_A,Rebel
Vale,"Torren Vale","",Rebel_C,Rebel
Dex,"Dex Callum","",Rebel_C,Rebel
Petra,"Petra Voss","",Rebel_B,Rebel
Kaine,"Grand Marshal Sera Kaine","",RadiantGuard,Dominion"""
NC_PATH = "/Game/Data/DT_NamedCharacters"
if eal.does_asset_exist(NC_PATH):
    nc = eal.load_asset(NC_PATH)
else:
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", unreal.load_object(None, "/Script/Eclipse.EclipseNamedCharacterRow"))
    nc = unreal.AssetToolsHelpers.get_asset_tools().create_asset("DT_NamedCharacters", "/Game/Data", unreal.DataTable, factory)
ok = unreal.DataTableFunctionLibrary.fill_data_table_from_csv_string(nc, NAMED_CSV)
unreal.log(f"DT_NamedCharacters: fill={'OK' if ok else 'MISLUKT'} (6 slots)")
eal.save_asset(NC_PATH)

setup = eal.load_asset("/Game/Data/DA_CampaignSetup")
setup.set_editor_property("body_defs", dt)
setup.set_editor_property("named_characters", nc)
eal.save_asset("/Game/Data/DA_CampaignSetup")
unreal.log("DA_CampaignSetup.BodyDefs + NamedCharacters gekoppeld")
