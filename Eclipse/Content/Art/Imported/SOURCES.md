# Imported curation accepts — /Game/Art/Imported

Every asset in this folder is a **curation accept** (phase0/ASSET_CURATION.md)
migrated out of the machine-local Fab-library packs in the **pack-slim round
2026-07-24** (`Tools/migrate_curation_accepts.py`; evidence:
phase0/ASSET_CLEANUP.md), so the multi-GB source packs could leave disk without
the 15.8 dressing round losing its sources. Repo-tracked on purpose: these are
the project's working copies. License: Fab/Epic marketplace content (Paragon
assets are free for Unreal Engine projects; SciFi Materials 10 was a free Fab
pull) — internal project repo use, not for redistribution outside the project.
**The repo is and must remain private**: Fab/Epic-licensed content lives in its
history, so flipping it public would be a license violation (review 2026-07-24).

**Restyle contract (15.5):** nothing here renders raw. Meshes carry
`/Game/Art/M_EclipseToon` in their default slots (placeholder only — the pack
materials were deliberately left behind); every placement overrides slots with
a toon MID (tint pair + AlbedoTex/AlbedoGain per phase0/ASSET_CURATION.md).

## Meshes (`/Game/Art/Imported/Meshes`)

| Asset | Curation id | Origin pack (old path) | Planned/placed |
|---|---|---|---|
| `SM_Well_Center_FX` | A2 | ParagonMinions `FX/Meshes/Environment/Maps/Agora` | **placed**: plaza centerpiece ring (amber accent, apron tile-locked) |
| `Granite_Large_Grey_Mossy_Rough` | A3 | ParagonMinions `FX/Meshes/Debris` | rubble-dressing round: slag heaps, Contested pockets |
| `SM_Rock_Chunk_LowPoly` | C1 | ParagonMinions `FX/Meshes/Minion_Specific/Jungle` | conditional: 3–5× instanced *under* A3 only (348-tri chunk; solo reads low-poly, 15.5) |
| `SM_AssetPlatform` | A1 | ParagonLtBelica `Characters/Maps/BackGroundAssets` | warehouse-yard round: loading-dock step / vendor-stand base |

Note: C1's saved registry tag in the pack said 348 tris; a fresh DDC build
reports 350 (same source geometry, 247 verts — duplicate is byte-derived from
the pack original; the 2-tri delta is a build-count artifact).

## Textures (`/Game/Art/Imported/Textures`)

The 7 accepted SciFi Materials 10 BaseColor sheets, 4096², luminance-only
albedos for `M_EclipseToon` (`AlbedoGain` = 1/mean-linear, measured
2026-07-23 — see phase0/ASSET_CURATION.md §5 for the full slot table):

| Asset | Gain | Slot |
|---|---|---|
| `T_4k_SciFi10_1_BaseColor` | 4.96 | **placed**: DecoPlaza deck-plate apron; still open: A1 plinth, landing pad |
| `T_4k_SciFi10_2_BaseColor` | 1.35 | **placed**: circular pad on the A2 ring; still open: machine bases |
| `T_4k_SciFi10_5_BaseColor` | 1.11 | warehouse (BldgB) interior walls |
| `T_4k_SciFi10_6_BaseColor` | 1.39 | machine-bank control faces |
| `T_4k_SciFi10_7_BaseColor` | 1.78 | Dominion post (BldgA) trim |
| `T_4k_SciFi10_9_BaseColor` | 2.70 | tread-plate: ramps, catwalks, cover tops |
| `T_4k_SciFi10_10_BaseColor` | 2.35 | grid plating: gantry/crane surfaces |

## Builder note (until the 15.8 dressing round lands)

`EclipseGrayboxBuilder.cpp` (161/720/741) still loads A2 + SciFi10 1/2 through
the **old pack paths**; three ~1.4 KB redirector stubs remain machine-local at
those paths so this machine's runtime graybox is unchanged. The dressing round
(which owns Source/) swaps the three strings to `/Game/Art/Imported/...`, after
which the stubs may be deleted. Machines without the stubs degrade gracefully
(flat cel + log line, GDD 14.3.5).
