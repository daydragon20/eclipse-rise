# Step-2 character pipeline data fill: creates /Game/Data/DT_BodyDefs
# (FEclipseBodyDefRow), fills the five Dominion archetypes in
# DT_EnemyArchetypes (GDD 09.3 flavor), and points DA_CampaignSetup.BodyDefs
# at the new table. Asset paths are RESOLVED from the asset registry per pack
# (never guessed); a pack without a usable mesh is skipped with a warning.
#
# 2026-07-25, minimale locomotie-laag: er is een RunAnim-kolom bij, de
# take-keuze filtert richting-/stance-varianten weg (de Player liep ACHTERUIT
# omdat "jog" als eerste Jog_Bwd matchte), en mesh + animaties komen nu
# gegarandeerd uit hetzelfde pack — zie de BODIES-tabel voor waarom dat geen
# detail is.
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


# Variants that are never the base locomotion take. Without this list the old
# "first name that contains the keyword" picker gave the Player a BACKWARD jog
# (Jog_Bwd matched "walk"->"run"->"jog") and gave Veil a crouched backward walk,
# which is why the bodies looked wrong long before the anim layer existed.
NOT_BASE_TAKE = (
    "bwd", "back", "_lt", "_rt", "left", "right",       # directions
    "circle", "pivot", "turn", "spin", "start", "stop", # transitions
    "crouch", "prone", "carry",                         # stances
    "additive", "zero", "_msa", "montage", "break",     # technical variants
    "downhill", "uphill", "jump", "aimoffset", "_ao_",
)


def pick_directional(path, wanted, direction_keys):
    """Zoek de richtingsvariant van een gangcyclus (26-07, punt 8).

    pick_anim() SLUIT richtingen expliciet uit - het zoekt de basis-take en
    bwd/_lt/_rt staan daarom in NOT_BASE_TAKE. Sinds het personage camera-relatief
    is hebben we die varianten juist nodig: achteruit lopen moet een
    achteruit-cyclus spelen en niet een omgekeerde vooruit-cyclus.

    Zelfde prioriteitsregel en dezelfde kortste-naam-tiebreak, alleen met de
    richting als extra eis en zonder de richtingsuitsluiting."""
    anims = sorted(find_assets(path, "AnimSequence"), key=lambda a: len(str(a.asset_name)))
    stance_or_technical = tuple(b for b in NOT_BASE_TAKE
                                if b not in ("bwd", "back", "_lt", "_rt", "left", "right"))
    for keyword in wanted:
        for asset in anims:
            name = str(asset.asset_name).lower()
            if keyword not in name:
                continue
            if not any(d in name for d in direction_keys):
                continue
            if any(bad in name for bad in stance_or_technical):
                continue
            return obj_path(asset)
    return ""


def pick_anim(path, wanted):
    """First take matching a keyword from `wanted` (priority order) that is not
    a direction/stance/technical variant. Ties break on the SHORTEST name, so
    `Idle` wins over `Idle_Relaxed` and `Jog_Fwd` over `Jog_Fwd_Downhill`."""
    anims = sorted(find_assets(path, "AnimSequence"), key=lambda a: len(str(a.asset_name)))
    for keyword in wanted:
        for asset in anims:
            name = str(asset.asset_name).lower()
            if keyword not in name:
                continue
            if any(bad in name for bad in NOT_BASE_TAKE):
                continue
            return obj_path(asset)
    return ""


def pick_mesh(path):
    meshes = find_assets(path, "SkeletalMesh")
    if not meshes:
        return ""
    return obj_path(sorted(meshes, key=lambda m: str(m.asset_name))[0])


# Keyword priorities per anim slot. Note that walk deliberately accepts the
# _Ironsights variants: in the SciFiSoldier/Warrior/Character packs those are
# the ONLY walk cycles shipped (the hip-fire set jumps straight to Jog).
IDLE_KEYWORDS = ("idle_rifle_hip", "thirdpersonidle", "idle")
# Richtingsstammen: het naamdeel ZONDER _Fwd, zodat dezelfde stam de vier
# varianten vindt (Jog_Bwd_Rifle, Jog_Lt_Rifle, ...).
WALK_STEMS = ("walk", "thirdpersonwalk")
RUN_STEMS = ("jog", "run", "sprint")
BACK_KEYS = ("bwd", "back")
LEFT_KEYS = ("_lt", "left")
RIGHT_KEYS = ("_rt", "right")

WALK_KEYWORDS = ("walk_fwd", "thirdpersonwalk", "walk")
RUN_KEYWORDS = ("jog_fwd", "thirdpersonrun", "sprint_fwd", "run", "jog")
HITREACT_KEYWORDS = ("hit_react", "hitreact", "impact", "flinch", "hit")
SHOOT_KEYWORDS = ("fire_rifle_hip", "fire_rifle", "primary_fire", "shoot", "fire", "attack")
DEATH_KEYWORDS = ("death", "die", "dead")

# Row -> (mesh path, anim path), both under /Game.
#
# MESH AND ANIMS MUST COME FROM THE SAME PATH. The comment that used to sit here
# said the SciFi family "shares the UE4-Mannequin skeleton" and let anim-poor
# packs borrow from anim-rich ones. That is half true and wholly wrong: every
# pack ships its OWN COPY of UE4_Mannequin_Skeleton under its own folder
# (/Game/SciFiSoldier/Meshes/UE4_Mannequin_Skeleton is a different USkeleton
# asset from /Game/SciFiCharacterPack/SciFiSoldier/Meshes/UE4_Mannequin_Skeleton),
# so "borrowing" produced clips authored for a skeleton the mesh does not wear.
# UEclipseAnimInstance rejects that outright (a foreign skeleton evaluates to a
# mangled pose, it does not error), so a borrowed take costs the body its
# locomotion.
#
# The fix is free: /Game/SciFiCharacterPack is the BUNDLE edition of the
# Soldier/Warrior/Girl packs and carries meshes AND animations per subfolder,
# while /Game/SciFiSoldier and /Game/SciFiWarrior are the same meshes shipped
# standalone with no animations at all. Point both columns at the bundle.
BODIES = {
    "Player":       ("ParagonLtBelica", "ParagonLtBelica"),
    "Rebel_A":      ("SciFiCharacterPack/SciFiSoldier", "SciFiCharacterPack/SciFiSoldier"),
    "Rebel_B":      ("SciFiGirl", "SciFiGirl"),
    "Rebel_C":      ("SciFiCharacterPack/SciFiGirl", "SciFiCharacterPack/SciFiGirl"),
    "Enforcer":     ("SciFiSoldier03", "SciFiSoldier03"),
    "Trooper":      ("SciFiSoldier02", "SciFiSoldier02"),
    "Shock":        ("SciFiWarrior02", "SciFiWarrior02"),
    "Veil":         ("SciFiCharacter", "SciFiCharacter"),
    "RadiantGuard": ("SciFiCharacterPack/SciFiWarrior", "SciFiCharacterPack/SciFiWarrior"),
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
    idle = pick_anim(anim_pack, IDLE_KEYWORDS)
    walk = pick_anim(anim_pack, WALK_KEYWORDS)
    run = pick_anim(anim_pack, RUN_KEYWORDS)
    shoot = pick_anim(anim_pack, SHOOT_KEYWORDS)
    death = pick_anim(anim_pack, DEATH_KEYWORDS)
    hit_react = pick_anim(anim_pack, HITREACT_KEYWORDS)
    walk_back = pick_directional(anim_pack, WALK_STEMS, BACK_KEYS)
    walk_left = pick_directional(anim_pack, WALK_STEMS, LEFT_KEYS)
    walk_right = pick_directional(anim_pack, WALK_STEMS, RIGHT_KEYS)
    run_back = pick_directional(anim_pack, RUN_STEMS, BACK_KEYS)
    run_left = pick_directional(anim_pack, RUN_STEMS, LEFT_KEYS)
    run_right = pick_directional(anim_pack, RUN_STEMS, RIGHT_KEYS)
    directions = sum(1 for a in (walk_back, walk_left, walk_right, run_back, run_left, run_right) if a)
    # Locomotie-rapport per rij: welke rung van de EEclipseLocomotionTier-ladder
    # deze body haalt is precies wat de owner in het spel ziet, dus zeg het hier
    # hardop in plaats van het in de tabel te laten verdwijnen.
    tier = ("geen idle -> ref pose" if not idle else
            "idle+walk+run" if (walk and run) else
            "idle+1 gait" if (walk or run) else
            "alleen idle -> body glijdt")
    unreal.log(f"{row_name}: {tier}, {directions}/6 richtingsvarianten")
    if directions == 0 and (walk or run):
        unreal.log_warning(
            f"{row_name}: geen enkele zij-/achteruitcyclus in {anim_pack} - deze body "
            "moonwalkt bij achteruit en strafen (14.3.5)")
    if not idle:
        unreal.log_warning(f"{row_name}: geen idle-take in {anim_pack} — deze body krijgt geen pose")
    lit, shade = TINTS.get(row_name, ((0.20, 0.20, 0.22), (0.07, 0.07, 0.09)))
    rows.append(f'{row_name},"{mesh}","{idle}","{walk}","{run}","{shoot}","{death}","{hit_react}",'
                f'"{walk_back}","{walk_left}","{walk_right}","{run_back}","{run_left}","{run_right}",true,'
                f'{color(lit)},{color(shade)},1.0,-90.0,-90.0')

csv_body = ("---,Mesh,IdleAnim,WalkAnim,RunAnim,ShootAnim,DeathAnim,HitReactAnim,"
            "WalkBackAnim,WalkLeftAnim,WalkRightAnim,RunBackAnim,RunLeftAnim,RunRightAnim,bToonRestyle,"
            "TintLit,TintShade,MeshScale,MeshZOffset,MeshYaw\n" + "\n".join(rows))

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

# Named story-character slots (step 3). SINGLE-WRITER-REGEL (MH-review 24-07):
# de MetaHumanMesh-kolom is eigendom van setup_metahuman_data.py — dit script
# vult de tabel alleen CREATE-ONLY. Een re-run op een bestaande tabel laat de
# named-tabel volledig ongemoeid, anders zou het de Kaya-koppeling stil wissen.
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
    unreal.log("DT_NamedCharacters bestaat - create-only, niet aangeraakt (MetaHumanMesh-kolom is van setup_metahuman_data.py).")
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
