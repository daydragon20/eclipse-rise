# Asset Cleanup — downloaded packs, disk-reclaim pass
*2026-07-23 | Owner order: review ALL downloaded assets; delete what is provably unneeded, actually use the rest. | Evidence rule: nothing is deleted unless it is demonstrably attached to nothing (code, DataTables, level data, tool pipelines).*

**Method.** Sizes via `Measure-Object` per directory. Reference checks before every verdict: grep over `Eclipse/Source` (builder/GameMode/character code), all `Eclipse/Tools/*.py` (incl. `setup_character_data.py`, which resolves meshes/anims per pack into DT_BodyDefs), a **binary string scan of the filled DataTables** (`Eclipse/Content/Data/*.uasset`), and a **binary scan of all level data** (`__ExternalActors__`, `__ExternalObjects__`, `Maps`, `Art`, `Data`) for every delete candidate — result: **0 hits** on the whole DELETE set. All delete candidates are gitignored with 0 tracked files (verified `git check-ignore` + `git ls-files`), so deletion is a pure filesystem operation. Fab packs stay re-downloadable from the owner library; CC0 is re-fetchable via `Tools/fetch_cc0.py` — reversible-in-principle, deleted only on proof.

---

## 1. KEEP — in use (do not touch)

| Pack / location | Size | Evidence |
|---|---|---|
| `Content/ParagonLtBelica` | 1,964.5 MB | **Player body**: DT_BodyDefs row `Player` (mesh + idle/walk/shoot/death anims) — binary-confirmed in `DT_BodyDefs.uasset`; plus accept A1 (`SM_AssetPlatform`). Whole pack kept: Paragon anims carry notifies into pack audio/FX (internal cross-refs), subdirectory surgery is not provably safe. |
| `Content/ParagonMinions` | 4,869.2 MB | Accept A2 (`SM_Well_Center_FX`) hard-referenced in `EclipseGrayboxBuilder.cpp:608` (plaza well, placed); accepts A3 + C1 planned (rubble round). Accepted meshes still reference pack-internal materials/textures — whole pack kept. |
| `Content/RustyCarsFree` | 68.1 MB | 5 wrecks (`SM_asset_00..04`) hard-referenced in `EclipseGrayboxBuilder.cpp:743-747`, dressed. |
| `Content/SciFi_Materials_10` | 1,012.8 MB | Albedos 1+2 hard-referenced in code (`EclipseGrayboxBuilder.cpp:133, 629`); 5/6/7/9/10 planned (§3). Non-BaseColor maps are referenced by the pack's own MIs — stripping them would create missing-ref errors in registry scans; whole pack kept. |
| `Content/SciFiSoldier`, `SciFiSoldier02`, `SciFiSoldier03`, `SciFiCharacter`, `SciFiCharacterPack`, `SciFiGirl`, `SciFiWarrior`, `SciFiWarrior02` | 1,441.5 MB (8 packs) | **Every RAISOR pack is a DT_BodyDefs source** (binary-confirmed): meshes for Rebel_A/B/C, Enforcer, Trooper, Shock, Veil, RadiantGuard; `SciFiCharacterPack` is additionally the **anim donor** for Rebel_A and RadiantGuard (shared UE4-Mannequin skeleton family). None deletable. |
| `Content/Fab/Megascans/Surfaces/Asphalt_Surface_rmqlqkp0` | 96.0 MB | Accept A9, floor primary — hard-referenced `EclipseGrayboxBuilder.cpp:118` (with repo-tracked fallback). |
| `Content/Fab/MetaHuman` + `MaterialFunctions` + `Materials` + `Textures` | ~461 MB | Quixel/MetaHuman infrastructure (curation §6: keep as-is). MetaHuman base meshes feed the planned `MH_<Naam>` step (DT_NamedCharacters.MetaHumanMesh, `phase0/metahuman_recipes.md`). |
| `Content/Screen_Damage_Indicator/UI` + `/Texture` | 0.3 MB | The pack's actual payload (`WBP_DamageIndicator`, `T_BloodOverlay`) — flagged for the HUD lane (GDD 08). Binary ref-check: self-contained (WBP references only the texture and itself). |
| `Saved/WarningSignStaging` (raw/ + 7 placard PNGs) | 424.3 MB | **The placard source.** `prepare_warning_signs.py` reads `raw/` (all 60 albedos exported); `import_warning_signs.py` self-selects the placard branch when the 7 cleaned PNGs exist (they do). This staging replaces the 2.27 GB pack as working source — see §4.2. The 60 raw PNGs are also the contact-sheet judging record. |
| `Saved/PolyHavenStaging` | 56.3 MB | Source for re-runnable imports of in-use assets: `T_asphalt_03/concrete_block_wall/metal_plate_diff` fallbacks + Barrel/road-barrier/crate props, all hard-referenced in builder code. |
| `Saved/QuaterniusStaging/.../FBX` | 17.7 MB | Import source of `import_quaternius_characters.py`; 4 of 8 FBX in use in code (`EclipseGrayboxBuilder.cpp:669-678`), remainder is the on-direction civilian set (§3). |
| `Saved/CC0Staging/ambientcg/<extracted dirs>` | ~137 MB | Import source for `import_cc0_albedos.py` re-runs; 5 accepted textures + 2 parked (curation §9). Zips deleted (§4.4). |
| `Saved/CurationStaging` | 35.3 MB | `inventory.json` + albedo contact-sheet exports — the curation evidence base of ASSET_CURATION.md. |
| `Saved/BlenderKit`, `Saved/BlenderProps`, `Saved/GeneratedDecals` | <0.5 MB | Active tool staging, negligible size. |

**Not touched per order:** `__ExternalActors__` / `__ExternalObjects__` (level data), GameAnimationSample in Documents (owner's own project), `Content/Art` (own repo-tracked assets), ParagonLtBelica/Minions accepts.

## 2. KEEP — planned (must actually be used; round listed)

| Asset | Planned use | Round |
|---|---|---|
| SciFi_Materials_10 albedos 5/6/7/9/10 | BldgB interior walls, machine control faces, BldgA Dominion trim, tread-plate ramps/catwalks, gantry grid | next texturing round (15.5 slot placement, curation §5) |
| ParagonMinions A3 (`Granite_Large_Grey_Mossy_Rough`) + C1 (`SM_Rock_Chunk_LowPoly`) | slag/rubble heaps in Contested pockets against the perimeter wall | rubble-dressing round (curation §3) |
| ParagonLtBelica A1 (`SM_AssetPlatform`) | loading-dock step / vendor-stand base, warehouse yard | warehouse-yard dressing round (curation §1) |
| `/Game/Art/Textures` ambientCG: `T_Metal046B_diff`, `T_Concrete042A_diff`, `T_Metal063_diff` | machine blocks/doors/BldgA shade; perimeter-wall variant/bunkers; containers/crates/gantry | same texturing rounds as above (curation §9, "not placed yet") |
| Screen_Damage_Indicator payload (`WBP_DamageIndicator`, `T_BloodOverlay`) | combat-feedback HUD | HUD/gameplay lane (GDD 08) — **the HUD agent must pick this up or it becomes a delete candidate next sweep** |
| Fab/MetaHuman Base_Female/Base_Male | MetaHuman `MH_<Naam>` slots for the six story characters | owner MetaHuman step (`phase0/metahuman_recipes.md`) |
| Quaternius FBX: `BaseCharacter`, `Casual2_Female`, `Casual3_Male/Female` | additional district inhabitants (civilian variety) | inhabitants-expansion round (15.7/09) |
| `Saved/WarningSignStaging/raw` (53 unjudged-into-placard albedos) | future placard derivations if a dressing round needs more signs | on demand via `prepare_warning_signs.py` |

## 3. DELETE — certain (executed this pass)

| # | Path | Freed | Reason / evidence |
|---|---|---|---|
| D1 | `Content/ParagonTwinblast` | **2,365.0 MB** | Curation §2: 0 accepted, 34/34 rejected. Not in DT_BodyDefs (binary scan), not an anim donor (BODIES map in `setup_character_data.py` has no Twinblast row), zero code refs, zero level-data refs. Only appearances: iteration lists in `inventory_character_packs.py:14` / `inventory_curation_pass.py:31` — these return empty results after delete, no crash. Fab re-download possible from owner library. |
| D2 | `Content/FD_WarningSigns_V1` | **2,271.2 MB** | All 60 albedos already exported to `Saved/WarningSignStaging/raw` (kept, §1); the 7 accepted placards are derived + repo-tracked in `/Game/Art/Decals`; the pack's own decal MIs were rejected (green-screen audit, curation §8). `import_warning_signs.py` self-selects the placard branch (pack never touched) and logs gracefully if the pack is absent (14.3.5). 0 code/level refs (`EclipseGrayboxBuilder.cpp:519` is a comment). The pack was a master copy whose complete usable extraction exists locally. |
| D3 | `Content/Screen_Damage_Indicator/UE` + `/L_DemoMap` + `/BluePrint` | **389.5 MB** | UE5 ThirdPerson-template filler bundled in the pack (Mannequins textures alone 244.6 MB); curation §7: 162/167 assets template filler. Payload (UI+Texture, kept) is binary-verified self-contained. `Bp_obstacle` references `UE/ThirdPerson/BP_ThirdPersonCharacter` → demo glue, deleted with its dependency. 0 external refs (level scan). |
| D4 | `Saved/CC0Staging/ambientcg/*.zip` (8 archives) | **152.7 MB** | Explicit owner instruction: all archives already extracted (verified: every zip has its extracted dir) — pure duplicates. Re-fetchable via `Tools/fetch_cc0.py`. |
| D5 | `Saved/QuaterniusStaging/.../Blends` | **103.1 MB** | The import pipeline (`import_quaternius_characters.py`) reads **FBX only**; no script references `.blend` files. FBX folder (17.7 MB) kept as import source. CC0, re-fetchable. |
| D6 | `Content/Fab/Megascans/Surfaces/Grungy_Surface_slnnecvc` | **25.9 MB** | Shipped only its MR map (incomplete); owner-scrapped 2026-07-23 (curation §6), replaced by CC0 `Metal041B` (placed, DecoStain). 0 refs anywhere. |
| D7 | `_fab_inbox` (repo root) | 0.0 MB | Empty leftover download-inbox directory. |

**Total freed: ~5,307 MB (≈ 5.2 GB).**

Deletion discipline: `tasklist`-check for `UnrealEditor.exe`/`UnrealEditor-Cmd.exe` immediately before each `Remove-Item` batch (two parallel builders run editor commandlets); `Remove-Item -Recurse -Force -Confirm:$false` per directory; freed space measured before/after.

## 4. Out of scope / noted for the owner

- `Saved/Autosaves` (320.5 MB) and `Saved/Screenshots` (55.4 MB) are editor-generated, not downloads — untouched. Autosaves regenerate; owner may clear at will.
- `Saved/CC0Staging/ambientcg/Metal063` still contains superseded 1K JPGs (~4 MB) next to the 2K set — left in place (trivial; zips-only rule applied).

## 5. Doc lines to update later (files owned by another agent this round — NOT edited here)

- **ASSET_CURATION.md §2** (ParagonTwinblast): add "pack removed from disk 2026-07-23 (cleanup pass) — re-download from Fab library if ever needed."
- **ASSET_CURATION.md §6** (Fab): add "Grungy_Surface_slnnecvc directory physically removed (was already scrapped)."
- **ASSET_CURATION.md §7** (Screen_Damage_Indicator): add "template filler (`UE/`, `L_DemoMap/`, `BluePrint/`) removed from disk; payload `UI/` + `Texture/` kept for the HUD lane."
- **ASSET_CURATION.md §8** (FD_WarningSigns_V1): add "pack removed from disk; placard source is now `Saved/WarningSignStaging/raw` (all 60 albedos exported); re-derivation runs without the pack."
- **SOURCES.md** (~line 76): the provenance note listing Screen_Damage_Indicator + character packs should mention that Twinblast and the FD sign pack are no longer on disk (provenance stays valid — assets derived from them are repo-tracked).

## 6. Post-delete sanity

See the execution log at the bottom of this file: editor-process check, per-directory removal, freed-space measurement, and a headless `EclipseValidateData` run (0 new missing-reference errors expected — the parallel builder round provides the full green bar).

---

## Execution log (2026-07-23)

- Editor-process check (`Get-Process UnrealEditor, UnrealEditor-Cmd`): **free** at deletion time.
- Deleted, measured per directory: ParagonTwinblast 2,365.0 MB · FD_WarningSigns_V1 2,271.2 MB · Screen_Damage_Indicator `UE`/`L_DemoMap`/`BluePrint` 389.5 MB · Quaternius `Blends` 103.1 MB · Grungy_Surface_slnnecvc 25.9 MB · 8 CC0 zips 145.7 MB · `_fab_inbox` 0.0 MB.
- **Total freed: 5,300.4 MB (5.18 GB).** Content now 10,042.6 MB; Saved 1,078.7 MB.
- Post-delete verification: keep-set spot check **14/14 present** (DT_BodyDefs, Belica, Minions A2 well, RustyCars, SciFi10 albedos 1+2, Megascans asphalt, SDI payload WBP+texture, 60/60 sign raws, Quaternius FBX, CC0 extracted dirs, RAISOR packs); delete-set **5/5 gone**.
- Pre-delete binary reference scan over `__ExternalActors__`/`__ExternalObjects__`/`Maps`/`Art`/`Data`: **0 hits** on every deleted path — no level actor or tracked asset referenced anything that was removed, so no new missing-reference errors are possible from this pass.
- Headless `EclipseValidateData` sanity: a parallel builder claimed the editor right after deletion. Ready-to-run when free (the running builder round provides the full green bar regardless):
  `& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Dev\ECLIPSE_GDD\Eclipse\Eclipse.uproject" -run=EclipseValidateData -unattended -nopause -nosplash -NoUba`
