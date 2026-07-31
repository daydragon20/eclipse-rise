# Asset Curation — downloaded packs, Kessara district pass
*2026-07-23 | Style law: GDD 15.5 (Borderlands-leaning cel/toon, ink outlines — nothing enters raw; palette owns hue, textures contribute luminance only) | Scope: STATIC meshes + materials + textures of the landed packs. Skeletal/character assets are owned by the character pipeline and are out of scope here.*

**Method.** Headless inventory (`Eclipse/Tools/inventory_curation_pass.py` → `Saved/CurationStaging/inventory.json`), visual contact sheets of every candidate albedo (SciFi set, Megascans, ambientCG, all 60 FD warning signs), and measured linear means (`Eclipse/Tools/measure_albedo_gain.py`, Pillow, exact sRGB EOTF, Rec.709 luminance). **AlbedoGain = 1/mean-linear-luminance** — same discipline as the System.Drawing pass of 2026-07-22, so texturing never re-meters the dusk auto-exposure.

**Restyle contract for every accept:** dynamic instance of `/Game/Art/M_EclipseToon` with the tint pair below, `AlbedoTex` + `AlbedoGain` as listed, `AlbedoMix` 0.4–0.9, `LightDir` synced to the builder sun. Fab packs are machine-local (gitignored): reference with graceful fallback (missing pack = flat cel, log line, never a crash — GDD 14.3.5).

Tint pairs referenced below (from the builder palette, hue-shifted-cool shade rule):

| Tint name | Lit | Shade |
|---|---|---|
| graphite | 0.230, 0.250, 0.290 | 0.075, 0.082, 0.130 |
| rust | 0.190, 0.115, 0.070 | 0.070, 0.045, 0.055 |
| oxide-red (Dominion) | 0.560, 0.160, 0.085 | 0.200, 0.045, 0.085 |
| worker-teal | 0.060, 0.300, 0.310 | 0.020, 0.100, 0.150 |
| stain (dark) | 0.070, 0.062, 0.075 | 0.028, 0.026, 0.038 |

---

## 1. /Game/ParagonLtBelica — 80 static meshes → **1 accepted, 79 rejected**

| # | Accept (object path) | Use | Restyle |
|---|---|---|---|
| A1 | `/Game/ParagonLtBelica/Characters/Maps/BackGroundAssets/SM_AssetPlatform` | 2×2 m industrial plinth: loading-dock step / vendor-stand base in the warehouse yard (4096 tri, clean quad disc) | graphite; AlbedoTex `T_4k_SciFi10_1_BaseColor`, gain **4.96**, world-aligned (UVMode 0), TexWorldScale ~200 |

**Rejected (79):** everything under `FX/Meshes` and `FX/Materials` — ability/muzzle/shell/aura/light-cone geometry (`SM_Belica_Aoe_*`, `SM_PlasmaShot_Shell`, `SM_FX_LightCone`, thrower-shatter fragments, …). Reads as VFX or as Belica gear debris, never as architecture; `SM_Turret_Spiral_Flames` is a flame-FX shell, not a turret. `SM_WitchHat`/`SM_Belica_ArmHolo` are character extras (fantasy/gear — off-direction). Belica's 371 textures/143 materials are character-domain (skeletal pipeline owns them).

## 2. /Game/ParagonTwinblast — 34 static meshes → **0 accepted, 34 rejected**

All FX helpers (muzzle flashes, water-impact cylinders, smoke planes, spline ribbons, jets) plus joke props (`SM_Cool_Sunglasses*`) and `TwinBlast_Hair`/`TwinBlast_Grenade` (character extras). `SM_AssetPlatform` here is a duplicate of A1 — use the Belica copy. Nothing reads industrial.
**Pack removed from disk 2026-07-23** (cleanup pass, `ASSET_CLEANUP.md` D1) — re-download from the Fab library if ever needed.

## 3. /Game/ParagonMinions — 162 static meshes → **2 accepted, 1 conditional, 159 rejected**

| # | Accept | Use | Restyle |
|---|---|---|---|
| A2 | `/Game/ParagonMinions/FX/Meshes/Environment/Maps/Agora/SM_Well_Center_FX` | 11.8 m circular basin ring (2048 tri): plaza centerpiece — **PLACED** (2026-07-23) at (600, 1800) on a 20 m DecoPlaza deck-plate apron, tile-locked so the pad graphic centers on the ring | **amber accent** (15.8 art-fix: graphite read grey-on-grey in the aerial) — Lit (0.52, 0.31, 0.06)/Shade (0.21, 0.115, 0.04), EmissiveScale ×1.3 on the unlit path, apron tint ×1.3 off Wall_; AlbedoTex `T_4k_SciFi10_2_BaseColor` (circular pad graphic), gain **1.35**, world-aligned, TexWorldScale 1200, mix 0.8; master follows the palette blocks' `-EclipseLitToon` choice |
| A3 | `/Game/ParagonMinions/FX/Meshes/Debris/Granite_Large_Grey_Mossy_Rough` | 5.1 m boulder (8738 tri): slag/rubble heap in Contested pockets against the perimeter wall | stain; **no AlbedoTex** (the mossy hue must never enter — luminance discipline; flat cel + ink outline makes it read stylized slag) |
| C1 | `/Game/ParagonMinions/FX/Meshes/Minion_Specific/Jungle/SM_Rock_Chunk_LowPoly` | CONDITIONAL: broken slab filler *inside* rubble piles only (348 tri — solo it reads low-poly, which 15.5 forbids; instanced 3–5× overlapping under A3 the facets read as Borderlands chunking) | stain; no AlbedoTex |

**Rejected (159):** minion gib sets (`Minion_Specific/*/SM_Chunks_*`, `SM_JungleChunks_*` — armor/body fragments, read as dismembered corpse parts, off the grounded tone), fauna (`SM_Bat`, `SM_Centipede`, `SM_Grub`, `SM_Roach`, `SM_Buff_Blue_Baby`), buff/targeting/spline/beam/lens/icon FX shapes, `SM_RedBuff_RocksGround` (near-duplicate footprint of A3, weaker silhouette), `SM_AssetPlatform` (duplicate of A1), `LP_Blade`/`SM_Tomahawk_*` (fantasy weapons).

## 4. /Game/RustyCarsFree — 17 static meshes → **5 accepted (in use), 12 rejected**

| # | Accept | Use | Restyle |
|---|---|---|---|
| A4–A8 | `/Game/RustyCarsFree/Geometries/SM_asset_00` … `SM_asset_04` | The five artery wrecks — already dressed by the builder (checkpoint choke dressing) | rust; each slot's own base texture as AlbedoTex, gain 3.2, mix 0.9, UVMode 1 (authored UVs) — banked, keep |

**Rejected (12):** `SM_ivy_00`–`SM_ivy_11` — organic overgrowth, off-biome for Kessara (rust/amber/graphite smog city; the art bible gives Kessara no green) and up to 198k tri for pure dressing. Parked: prime candidates for **Sylvaris** later. The pack's `Mi_*`/`M_00x` materials stay unused (raw PBR — everything renders through the toon MIDs).

## 5. /Game/SciFi_Materials_10 — 10 material sets → **7 albedos accepted, 3 rejected; master + all 10 MIs rejected**

The BaseColors are flat-graphic trim/panel sheets (cream/orange/grey) — the most Borderlands-native surfaces in the library. They enter **only** as `AlbedoTex` luminance into `M_EclipseToon`; the pack's `M_Master` + `MI_SciFi_1..10` are raw PBR and are rejected as materials (15.5: nothing renders raw). All 4096², machine-local.

| Albedo (`/Game/SciFi_Materials_10/Textures/<n>/…`) | mean-lin | gain | Use in the district |
|---|---|---|---|
| `1/T_4k_SciFi10_1_BaseColor` | 0.202 | **4.96** | dark X-braced deck plate — **PLACED** as the DecoPlaza apron under the A2 ring (TexWorldScale 200, mix 0.7); still open: A1 plinth, landing pad |
| `2/T_4k_SciFi10_2_BaseColor` | 0.742 | **1.35** | circular pad centerpiece — **PLACED** on the A2 ring; still open: machine bases |
| `5/T_4k_SciFi10_5_BaseColor` | 0.903 | **1.11** | perforated sheet panel: warehouse (BldgB) interior walls |
| `6/T_4k_SciFi10_6_BaseColor` | 0.720 | **1.39** | machine-bank graphic: control-wall faces on machine blocks |
| `7/T_4k_SciFi10_7_BaseColor` | 0.562 | **1.78** | cross-braced plating with corner castings: Dominion post (BldgA) trim |
| `9/T_4k_SciFi10_9_BaseColor` | 0.371 | **2.70** | diamond tread-plate: ramps, catwalks, cover tops |
| `10/T_4k_SciFi10_10_BaseColor` | 0.426 | **2.35** | dense grid plating: gantry/crane surfaces (Kit_GantryBeam) |

**Rejected (3):** `_3` (near-blank cream — no read at any distance), `_4` (candy-colored honeycomb — reads as fabric/toy, not industrial), `_8` (sparse wire squiggles — reads as scribble beyond 3 m).

## 6. /Game/Fab — **1 accepted, 1 incomplete (owner click), rest is infrastructure**

| # | Accept | Use | Gain |
|---|---|---|---|
| A9 | `/Game/Fab/Megascans/Surfaces/Asphalt_Surface_rmqlqkp0/High/rmqlqkp0_tier_1/Textures/T_rmqlqkp0_4K_B` | 4K asphalt albedo — **PLACED** (2026-07-23) as the Floor slot primary; the repo-tracked `T_asphalt_03_diff` (gain 16.8) is the declared fallback on machines without the pack | mean 0.081 → gain **12.41**; TexWorldScale 700, TexMix 0.5 |

**Scrapped (owner decision 2026-07-23):** `Grungy_Surface_slnnecvc` shipped only
its MR map — the Fab re-download is **cancelled**; the equivalent grungy surface
is ambientCG CC0 `Metal041B` (§9), which landed in the DecoStain slot. No owner
click remains. The `Grungy_Surface_slnnecvc` directory was physically removed
2026-07-23 (cleanup pass, `ASSET_CLEANUP.md` D6). **Not curation targets:** `MaterialFunctions/QMF_*`, default textures, MetaHuman base normals — Quixel/MetaHuman infrastructure, keep as-is, never restyle.

## 7. /Game/Screen_Damage_Indicator — 5 static meshes → **0 accepted; 2 assets flagged for the HUD lane**

The 5 `LevelPrototyping` meshes are UE-template primitives (the graybox already owns that job) — rejected, as are the mannequin materials (162 of 167 assets are template filler). The pack's actual payload — `/Game/Screen_Damage_Indicator/UI/WBP_DamageIndicator` + `/Game/Screen_Damage_Indicator/Texture/T_BloodOverlay` — is combat-feedback HUD (GDD 08), flagged for the HUD/gameplay agent; not district dressing, no toon restyle.
**Template filler (`UE/`, `L_DemoMap/`, `BluePrint/`) removed from disk 2026-07-23** (cleanup pass, `ASSET_CLEANUP.md` D3); the payload `UI/` + `Texture/` is kept for the HUD lane.

## 8. /Game/FD_WarningSigns_V1 (rest) — 60 sign albedos → **ALL 60 REJECTED (2026-07-31 fiction pass)**

> ### ⛔ RETIRED 2026-07-31 — all seven survivors failed `20_world_dressing_standard.md` §20.2
>
> **Nothing from this pack is in the build.** The seven placards below were dressed by the builder until the §20.2 fiction pass, and every one of them was Earth signage: STOP in Latin capitals, the ISO-361 radiation trefoil, skull-and-crossbones over the word TOXIC, a European mandatory-direction arrow, the men-at-work roadworks pictogram, a cartoon dynamite stick, and a second trefoil. Three were confirmed in frame on the 31-07 recording round.
>
> **Why this pass missed them.** The rejections below were judged on **tone** — "joke/meme", "reads suburban, not industrial oppression". They were never judged on **fiction**. §20.2 says curation is both: *"Curatie betekent niet alleen 'past de stijl' maar ook 'past de fictie'."* A photographed Earth sign passes a tone filter perfectly and still breaks the world.
>
> **Replacement:** an authored Dominion signage system, `Eclipse/Tools/generate_dominion_signs.py` → `Saved/GeneratedDecals` → `Tools/import_generated_decals.py`, as `T_sign_dom_*_diff`. Replaced, not removed — same seven placements, same scales, same red/amber split. The grammar is documented in the generator.
>
> `prepare_warning_signs.py`, `import_warning_signs.py` and `export_sign_albedos.py` are now **dead paths**; `Saved/WarningSignStaging` is a dead source. Do not derive new placards from this pack — it cannot produce an in-fiction sign.

*Historical record of the 2026-07-23 pass, kept so the failure mode is readable:*

All 60 albedos exported and judged on a contact sheet (`Tools/export_sign_albedos.py` → `Saved/WarningSignStaging/raw`). The pack's own decal MIs stay rejected (green-screen backing renders green cards — 2026-07-23 audit); accepted signs travel as derived placards via `prepare_warning_signs.py` (crop + dark-steel plate) into `/Game/Art/Decals`.
**Pack removed from disk 2026-07-23** (cleanup pass, `ASSET_CLEANUP.md` D2): the placard source is now `Saved/WarningSignStaging/raw` (all 60 albedos exported) — re-derivation runs without the pack.

**Value normalization (15.8 art-fix round, 2026-07-23):** the raw pack photos sit at wildly different exposures — half the placards rendered near-black (review shot 00012). `prepare_warning_signs.py` now scales every cutout so its p99 max-channel hits 245 (`NORM_TARGET`), and the builder's sign tints stepped up (lit ×1.4, shade to ~0.6×lit — value-only, hue unchanged). Means/gains below re-measured on the normalized placards.

| Placard (`/Game/Art/Decals/…`) | Pack index | norm scale | mean-lin | gain | Use |
|---|---|---|---|---|---|
| Placard (retired) | Pack index | What it actually was | §20.2 verdict |
|---|---|---|---|
| `T_sign_stop_diff` | 09 | red triangle, **"STOP"** in Latin capitals | Earth traffic sign — bullets 1 + 4 |
| `T_sign_radiation_diff` | 17 | the **ISO-361 trefoil** (UC Berkeley, 1946) | datable Earth institution — bullet 5 |
| `T_sign_toxic_diff` | 30 | skull-and-crossbones over **"TOXIC"** | Earth semiotics + Latin text — bullets 4 + 5 |
| `T_sign_route_diff` | 36 | **white arrow in a circle** on a triangle | European mandatory-direction road sign — bullet 1 |
| `T_sign_labor_diff` | 50 | the **men-at-work roadworks pictogram** | a literal road sign — bullet 1 (the "probably fine" that was not) |
| `T_sign_blast_diff` | 51 | burst + **cartoon dynamite stick with a fuse** | Western cartoon semiotics — bullet 5 |
| `T_sign_reactor_diff` | 53 | the trefoil again, in a triangle | Earth ISO **and** a repeat read (§20.7 check 7) |

Replaced 1:1 by `T_sign_dom_seal / flare / scrubber / lane / shift / charge / core` at the same seven placements. **Rejected (53):** joke/meme signs (`NO LIFE` 06/07, `STOP RUN` 08, camera 57/60, `LOVE` 27/28, `BAD DOG` 18–21, food 58/59 — off the grounded Expanse/Andor tone), civic-traffic family (22–26, 44–48 — reads suburban, not industrial oppression), skull/poison variants 14–16, 54–56 (duplicative of toxic/reactor), remaining rust-triangle variants (weaker reads of the accepted seven).

## 9. ambientCG CC0 imports — **5 accepted, 2 parked**

Imported from `Saved/CC0Staging/ambientcg` into `/Game/Art/Textures` (Color maps
only — the toon pipeline has no use for PBR normal/roughness). First batch via
`inventory_curation_pass.py` §B; follow-up batches via the reusable
`Tools/import_cc0_albedos.py` (2026-07-23: the grunge-replacement round —
`fetch_cc0.py` q=-search fix, `rust`/`corrugated` pulls). Provenance in
`Content/Art/Textures/SOURCES.md`.

| Asset | mean-lin | gain | Use |
|---|---|---|---|
| `/Game/Art/Textures/T_Metal046B_diff` (2K) | 0.051 | **19.76** | dark mottled steel: machine blocks, doors, BldgA shade surfaces (not placed yet) |
| `/Game/Art/Textures/T_Concrete042A_diff` (2K) | 0.112 | **8.94** | smooth shuttered concrete: perimeter-wall variant, bunkers (not placed yet) |
| `/Game/Art/Textures/T_Metal063_diff` (**2K re-import 2026-07-23**, was 1K) | 0.156 | **6.42** (was 6.39 @1K — slot value re-measured, same discipline) | rust-speckled blue-grey steel: containers, crates, gantry beams (not placed yet) |
| `/Game/Art/Textures/T_Metal041B_diff` (2K) **new** | 0.291 | **3.44** | heavy-rust grunge — **PLACED** in the DecoStain slot (TexWorldScale 400, mix 0.7); the CC0 stand-in for the scrapped Fab "Grungy Surface" |
| `/Game/Art/Textures/T_CorrugatedSteel007A_diff` (2K) **new** | 0.367 | **2.72** | rusty corrugated sheet — **PLACED** on the warehouse (BldgB, TexWorldScale 300, mix 0.45), replacing `T_corrugated_iron_02_diff` |

**Parked (staged, not imported):** `MetalWalkway014` (grating with an Opacity
map — not a tileable albedo; would need a masked-material path first),
`CorrugatedSteel009` (fetch by-catch; 007A is the accepted rusty sheet).

---

## Tally & owner clicks

**Accepted: 31** (9 meshes incl. 1 conditional + 20 textures/placards + 2 HUD-flagged) · **Rejected: ~340** (276 Paragon SM, 12 ivy, 5 proto meshes, 53 signs, 3 SciFi albedos + master/MIs, misc FX) · **Parked: 2** (ambientCG staging). Every accept renders through `M_EclipseToon` MIDs with the gains above; no raw material from any pack survives curation.

**Owner clicks: 0.** The former Grungy-Surface re-download click is scrapped by
owner decision (2026-07-23) — ambientCG `Metal041B` is the CC0-first replacement.
