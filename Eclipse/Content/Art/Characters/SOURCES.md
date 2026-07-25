# Civilian/worker bodies — /Game/Art/Characters (imported set)

The 11 folders imported 2026-07-25 (`Tools/import_modular_civilians.py`, headless
commandlet slot) are the stylized civilian/worker harvest for Hollow Point crew,
Kessara idlers and mission bystanders (SPEC-P2-03 / SPEC-P2-04 surface work):

`Casual_2`, `Casual_F`, `Casual_Hoodie`, `Farmer_M`, `Formal_F`, `Punk_F`,
`Punk_M`, `Suit_F`, `Suit_M`, `Worker_F`, `Worker_M` — 26 assets each
(skeletal mesh + skeleton + 24 animation takes incl. Idle/Walk/Run/Interact/Wave).

- **Source:** Quaternius — *Ultimate Modular Men* / *Ultimate Modular Women*
  (quaternius.com), staged from `Saved/QuaterniusStaging` (contactsheets + the
  import script's provenance copy live there, machine-local).
- **License: CC0 1.0** (public domain, no attribution required — attribution
  given anyway as courtesy). Unlike the Fab content under `/Game/Art/Imported`,
  these files carry no redistribution restriction.
- **Import contract:** materials/textures deliberately NOT imported — bodies
  arrive material-less and render only through toon-palette MIDs per the
  ÉÉN-STIJL-WET (15.5); the separate material slots per clothing zone are the
  MID attachment points. Builder wiring is its own [Art] changeset
  (phase0/DRESSING_ITERATIE_2.md, wave 2 of cycle N+1) with a 15.8 shot round
  before commit.
- Rig: 79 bones incl. fingers, shared armature per gender line (retarget
  candidates for the Mannequin AnimBP set later; the imported takes cover
  ambient/idler behaviour without retargeting).

The four pre-existing folders (`BlueSoldier_Female`, `BlueSoldier_Male`,
`Casual2_Male`, `Casual_Bald`, 19 assets each) are the earlier Phase-1
placeholder bodies — different source, documented in the repo history; left
untouched by the import script (create-only per folder).
