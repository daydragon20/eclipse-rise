# Pack-slim migration — move every curation ACCEPT out of the machine-local
# Fab packs into repo-tracked /Game/Art/Imported, so ParagonMinions and
# SciFi_Materials_10 can be dropped from disk (owner order 2026-07-23,
# progress_data.js "Pack-slim-ronde"; evidence pattern: phase0/ASSET_CLEANUP.md).
#
# Per accept: duplicate -> consolidate (old into new). Consolidation re-points
# every on-disk referencer to the new asset AND leaves a UObjectRedirector at
# the old pack path — deterministic, unlike rename_asset's "only when needed"
# redirectors. The three redirectors the C++ builder still loads through
# (SM_Well_Center_FX + SciFi10 BaseColor 1/2, EclipseGrayboxBuilder.cpp
# 161/720/741) are preserved machine-local during the pack delete, so the
# runtime graybox keeps its plaza well until the builder strings move to
# /Game/Art/Imported in the 15.8 dressing round (Source/ is out of scope for
# the pack-slim slot). On machines without the redirectors the builder already
# degrades gracefully (flat cel + log line, GDD 14.3.5).
#
# Migrated meshes get their default material slots re-pointed to the toon
# master /Game/Art/M_EclipseToon: raw pack materials never render (15.5) —
# every placement overrides slots with M_EclipseToon MIDs anyway — and this
# cuts the last hard dependencies into the packs. A dependency audit per
# migrated asset proves self-containment (only /Game/Art plus engine-owned
# /Script/* and /Engine/* packages — machine-independent by definition).
#
# Idempotent: an accept whose destination already exists is skipped.
#
# Run headless (editor closed):
#   UnrealEditor-Cmd <project> -run=pythonscript -script="<repo>\Eclipse\Tools\migrate_curation_accepts.py" \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash -NoUba

import unreal

TOON_MASTER = "/Game/Art/M_EclipseToon"
DEST_MESHES = "/Game/Art/Imported/Meshes"
DEST_TEXTURES = "/Game/Art/Imported/Textures"

# (curation id, old package path, destination folder, expected class)
ACCEPTS = [
    ("A2", "/Game/ParagonMinions/FX/Meshes/Environment/Maps/Agora/SM_Well_Center_FX", DEST_MESHES, unreal.StaticMesh),
    ("A3", "/Game/ParagonMinions/FX/Meshes/Debris/Granite_Large_Grey_Mossy_Rough", DEST_MESHES, unreal.StaticMesh),
    ("C1", "/Game/ParagonMinions/FX/Meshes/Minion_Specific/Jungle/SM_Rock_Chunk_LowPoly", DEST_MESHES, unreal.StaticMesh),
    ("A1", "/Game/ParagonLtBelica/Characters/Maps/BackGroundAssets/SM_AssetPlatform", DEST_MESHES, unreal.StaticMesh),
] + [
    (f"SciFi10_{i}", f"/Game/SciFi_Materials_10/Textures/{i}/T_4k_SciFi10_{i}_BaseColor", DEST_TEXTURES, unreal.Texture2D)
    for i in (1, 2, 5, 6, 7, 9, 10)
]

# Dependency prefixes that may remain after migration; anything else fails the audit.
ALLOWED_DEP_PREFIXES = ("/Game/Art/", "/Script/", "/Engine/")

lib = unreal.EditorAssetLibrary
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.wait_for_completion()

dep_options = unreal.AssetRegistryDependencyOptions(
    include_soft_package_references=True,
    include_hard_package_references=True,
    include_searchable_names=False,
    include_soft_management_references=False,
    include_hard_management_references=False,
)

toon = lib.load_asset(TOON_MASTER)
if toon is None:
    raise RuntimeError(f"MIGRATE-FATAL: toon master missing at {TOON_MASTER} — nothing migrated (15.5).")

failures = []
for accept_id, old_path, dest_folder, expected_cls in ACCEPTS:
    name = old_path.rsplit("/", 1)[-1]
    new_path = f"{dest_folder}/{name}"

    if lib.does_asset_exist(new_path):
        # Review fix: destination existing only counts as "done" when the old
        # path is gone or a redirector — a crash between duplicate and
        # consolidate would otherwise pass silently with live referencers on
        # the old asset, and the pack drop would then strand them.
        old_data = lib.find_asset_data(old_path)
        old_is_settled = (not old_data.is_valid()) or str(old_data.asset_class_path.asset_name) == "ObjectRedirector"
        if old_is_settled:
            unreal.log(f"MIGRATE-SKIP {accept_id}: {new_path} already exists (idempotent re-run).")
        else:
            unreal.log_error(f"MIGRATE-HALFDONE {accept_id}: {new_path} exists but {old_path} is still a live asset — "
                             f"consolidate never ran; resolve by hand before dropping packs.")
            failures.append(accept_id)
        continue
    if not lib.does_asset_exist(old_path):
        unreal.log_error(f"MIGRATE-MISS {accept_id}: source not found: {old_path} — skipped, report to owner.")
        failures.append(accept_id)
        continue

    src = lib.load_asset(old_path)
    if src is None or not isinstance(src, expected_cls):
        unreal.log_error(
            f"MIGRATE-MISS {accept_id}: {old_path} loaded as "
            f"{type(src).__name__ if src else 'None'}, expected {expected_cls.__name__} — skipped."
        )
        failures.append(accept_id)
        continue

    dup = lib.duplicate_asset(old_path, new_path)
    if dup is None:
        unreal.log_error(f"MIGRATE-FAIL {accept_id}: duplicate to {new_path} failed — source untouched.")
        failures.append(accept_id)
        continue

    # Consolidate old into new: referencers re-pointed, redirector left at old path.
    if not lib.consolidate_assets(dup, [src]):
        unreal.log_error(f"MIGRATE-FAIL {accept_id}: consolidate {old_path} -> {new_path} failed; "
                         f"duplicate stays, original untouched — resolve by hand.")
        failures.append(accept_id)
        continue

    # Meshes: default slots to the toon master (raw pack materials never render, 15.5).
    if expected_cls is unreal.StaticMesh:
        mesh = lib.load_asset(new_path)
        slots = list(mesh.get_editor_property("static_materials"))
        for idx in range(len(slots)):
            mesh.set_material(idx, toon)
        unreal.log(f"MIGRATE-SLOTS {accept_id}: {len(slots)} slot(s) -> {TOON_MASTER}")

    lib.save_asset(new_path, only_if_is_dirty=False)

    redirector = "none"
    old_data = lib.find_asset_data(old_path)
    if old_data.is_valid():
        redirector = str(old_data.asset_class_path.asset_name)
    unreal.log(f"MIGRATE-OK {accept_id}: {old_path} -> {new_path} (old path now: {redirector})")

# Persist everything the consolidation dirtied (redirectors, re-pointed referencers).
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(save_map_packages=False, save_content_packages=True)

# ------------------------------------------------------------ dependency audit
unreal.log("MIGRATE-AUDIT: hard+soft package dependencies of every migrated asset:")
audit_fail = False
for accept_id, old_path, dest_folder, _cls in ACCEPTS:
    name = old_path.rsplit("/", 1)[-1]
    new_pkg = f"{dest_folder}/{name}"
    if not lib.does_asset_exist(new_pkg):
        continue
    deps = registry.get_dependencies(new_pkg, dep_options) or []
    bad = [str(d) for d in deps if not str(d).startswith(ALLOWED_DEP_PREFIXES)]
    unreal.log(f"MIGRATE-AUDIT {accept_id} {new_pkg}: deps={[str(d) for d in deps]}")
    if bad:
        unreal.log_error(f"MIGRATE-AUDIT-FAIL {accept_id}: pack-external deps remain: {bad}")
        audit_fail = True

if failures or audit_fail:
    # Hard failure so wrapper scripts and the commandlet exit non-zero (review fix).
    raise RuntimeError(f"MIGRATE-RESULT: INCOMPLETE (failures={failures}, audit_fail={audit_fail})")
unreal.log("MIGRATE-RESULT: ALL ACCEPTS MIGRATED CLEAN")
