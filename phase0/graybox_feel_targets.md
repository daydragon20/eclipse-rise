# Graybox Movement & Gunplay Feel Targets — LOCKED
*Phase 0 deliverable | GDD refs: 4.1.1 (movement), 8.1 (combat feel), 8.2 (gunplay) | Skill owner: Combat Designer*

**Status: locked.** These numbers are the Phase 1 graybox tuning contract. Changing any value follows the change-management flow (DOCUMENTATION_README): problem → proposal → affected systems → update this doc in the same commit. All values live in DataAssets in-project; this doc is the design authority they must match.

---

## 1. Feel statement (the test everything answers to)

*"The smartest person in a deadly room — not the strongest"* (GDD 8.1). Weighty, deliberate, readable. Rhythm: **read → plan → strike → adapt**, 20–60 s per encounter beat. Reference blend: TLOU2 lethality/traversal readability, Gears cover snappiness (but lighter), Wildlands open encounters, XCOM consequence weight in real time.

## 2. Movement targets (GDD 4.1.1)

| Metric | Target |
|---|---|
| Walk / Run / Sprint | 1.8 / 4.2 / 6.5 m/s |
| Sprint stamina | drains only above Medium armor |
| Crouch | stealth default; prone deferred (scoped zones, Phase 3) |
| Cover | contextual soft-attach, no sticky snap; corner lean; cover-to-cover dash ≤ 8 m |
| Vault/mantle | ≤ 2.4 m |
| Dodge | directional roll, 0.5 s, i-frames vs. melee only |
| Sprint-slide | into cover and through low gaps — the signature verb; must feel "soldier, not acrobat" |
| Camera | over-shoulder, swappable shoulder, FOV 90; +15% pullback in command mode; push-in on aim |

**Graybox acceptance:** a tester can chain sprint → slide-to-cover → lean-fire → dash-to-cover without menu knowledge, and calls it "heavy but responsive" not "sluggish." Input-to-motion latency ≤ 100 ms for all verbs.

## 3. Gunplay targets (GDD 8.1–8.2)

| Metric | Target |
|---|---|
| Player TTK vs. basic enemy | ~0.6 s well-aimed |
| Enemy TTK vs. exposed player | ~2.5 s |
| Locational damage | head ×2.5; limbs cripple |
| Ballistics | hitscan < 50 m, projectile ≥ 50 m |
| Recoil | learnable per-platform patterns (graybox: one AR, one sidearm) |
| Difficulty rule | margins & information change; **never** bullet-sponge HP scaling |
| 5+ enemies, no squad/prep | lethal — at every progression stage |

**Graybox acceptance:** exposed play dies fast enough that cover use is self-taught within two encounters; well-aimed play feels decisively lethal both ways.

## 4. Command layer target (Phase 1 subset)

Orders via reticle + hotkey (no time dilation in Phase 1; 30% dilation lands with Command Mode in Phase 2). Feel bar: order → visible acknowledgment ≤ 1 s (movement start or spoken refusal). "Feels obeyed" is the gate metric (SPEC-P1-06).

## 5. Reference clips (to capture internally — open Phase 0 item)

Lock the targets against footage, not memory. Capture list (30–60 s each, stored in `Eclipse/Docs/FeelReferences/`):

1. TLOU2 — traversal into cover under fire (readability, weight).
2. Gears 5 — cover entry/exit + corner lean cadence (snappiness ceiling — we sit *below* it).
3. Ghost Recon Wildlands — open-approach encounter start → collapse (encounter rhythm).
4. XCOM 2 — a soldier death and its aftermath (the consequence feeling our real-time must preserve).
5. Mass Effect 2 — power-wheel squad ordering (command clarity reference for Phase 2 Command Mode).

Each clip gets a one-line annotation: *what we take, what we deliberately don't.*
