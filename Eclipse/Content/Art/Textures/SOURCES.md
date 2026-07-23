# Texture sources — /Game/Art/Textures

All textures in this folder are **CC0 (public domain)** from [Poly Haven](https://polyhaven.com),
downloaded 2026-07-22 via the public API with owner authorization (asset policy,
MIGRATION_TO_STRONG_PC.md §5). CC0 requires no attribution; recorded here anyway
for provenance and reproducibility (re-download: `Tools/import_polyhaven_textures.py`
header + the ids below against `https://api.polyhaven.com/files/<id>`).

| Asset | Poly Haven id | Map | Used on |
|---|---|---|---|
| `T_asphalt_03_diff` | `asphalt_03` | Diffuse 2K JPG | District floor (Floor) |
| `T_concrete_block_wall_diff` | `concrete_block_wall` | Diffuse 2K JPG | Perimeter walls (Wall_) |
| `T_corrugated_iron_02_diff` | `corrugated_iron_02` | Diffuse 2K JPG | Warehouse (BldgB) |
| `T_metal_plate_diff` | `metal_plate` | Diffuse 2K JPG | Dominion post (BldgA) |

**Prop meshes** (`/Game/Art/Props`, FBX 2K, same CC0/Poly Haven origin, downloaded
2026-07-23; import: `Tools/import_polyhaven_props.py`):

| Asset | Poly Haven id | Used as |
|---|---|---|
| `Barrel_01` + `T_Barrel_01_diff` (oxide_grey variant) | `Barrel_01` | Barrel clusters (yards, checkpoints) |
| `concrete_road_barrier` + `T_concrete_road_barrier_diff` | `concrete_road_barrier` | Artery checkpoint barriers |
| `plastic_crate_03` + `T_plastic_crate_03_diff` | `plastic_crate_03` | Warehouse-yard crate stacks |

Style contract (15.5): textures enter the picture **through the toon pipeline** —
multiplied into the cel bands via world-aligned projection in `M_EclipseToon`;
the palette, banding, and ink outlines stay in charge. These are placeholder-grade
inputs per the asset policy; the Fab/Quixel art pass upgrades them.
