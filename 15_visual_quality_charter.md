# PART 15 — VISUAL QUALITY CHARTER
**ECLIPSE: RISE OF THE RESISTANCE**
*Art & Rendering Direction Bible — Document 15 of 15 | The standing target for how the shipping game looks, and how we get there without breaking the risk-first plan.*

---

## 15.0 Standing & Authority (read this first)

This charter defines the **visual quality target** for ECLIPSE and the studio discipline used to reach it. It is written as if by a full team — Technical Director, Senior Graphics Programmer, Environment Artist, Character Artist, Gameplay Engineer, Optimization Specialist — and it expects work at that level: think like a studio, not a code generator; find the weakest visual link each pass and fix it.

**Where it sits in the hierarchy (see DOCUMENTATION_README.md):**

1. It is **subordinate to the Game Design Bible (01–11).** If a shot looks amazing but breaks a pillar or the design, the design wins.
2. It is **bound by the Technical Design Bible (12)** — especially the performance budgets (12.4). A frame we cannot afford is not shippable, however pretty.
3. It is **paced by the Roadmap (13).** This is the load-bearing rule:

> **THE GRAYBOX RULE — non-negotiable.** Phase 1 is *deliberately ugly* (13.1, SPEC-P1-00): one graybox district, no art, debug UI. This charter does **not** authorize a single hour of fidelity work inside Phase 1. The three existential risks (ground↔strategy loop, squad-AI quality, economy/consequence chain) must prove out on graybox first. **This charter never overrides the current ACTIVE_MILESTONE.** Fidelity work begins at **Phase 2 (Vertical Slice, "Thirteen Bullets")** and intensifies through Phase 5 (Beta polish/optimization).

So: adopt the target now, apply it on schedule. Any instruction in this document that appears to ask for content beyond the active milestone is asking for it *at the phase where that content lives* — not today.

---

## 15.1 The Standard

Build the most visually impressive game realistically achievable with Unreal Engine 5.8, the available Epic-ecosystem assets, and the target hardware. The bar is **modern AAA third-person action** — readable at a squad-command distance, cinematic in its set-pieces, and consistent from the first graybox replacement to the final planet.

The identity we are dressing (from 01/03): a single-player third-person Action-Strategy RPG that spans **ten planets** of a fallen interstellar civilisation (the Vantara Expanse) — from the industrial underworld of **Kessara** to the blighted ruin of **Meridia**. Every environment must read as *lived-in, occupied, and worth liberating*. Grime, propaganda, checkpoints, and scale are the mood — not a neutral sci-fi backdrop.

---

## 15.2 Hardware Reality — target vs. dev box (Optimization Specialist's opening note)

A charter that ignores the machine is a wish list. Three different machines matter:

**A) Fidelity target — the shipping min/rec spec (what the visuals are authored FOR).**
A "powerful gaming PC," RTX-class. Hardware Lumen + hardware ray tracing available, Nanite throughout, Virtual Shadow Maps, 8+ GB VRAM for 4K texture streaming. All fidelity decisions in this charter are validated against this class of machine and scaled *down* via UE scalability for weaker specs — never the reverse.

**B) Laptop dev box — measured 2026-07-21 (graybox / systems / CI machine).**

| Component | Reality | Consequence |
|---|---|---|
| GPU | **NVIDIA GTX 1050, 4 GB VRAM** (+ Intel HD 630) | **No RT cores** → hardware ray tracing is *not available here*. Lumen only in reduced *software* mode. 4 GB VRAM cannot hold 8K/4K texture sets — expect streaming pressure. |
| CPU | Intel i7-7700HQ, 4c/8t @ 2.8 GHz (2017 mobile) | Fine for graybox logic + tests. Editor cook/light builds are slow; keep them off the hot loop. |
| RAM | 32 GB | Comfortable for the editor + UBT. |
| Display | 3840×2160 | Author at native, but profile at 1080p — the 1050 will not drive 4K fidelity. |

**C) Strong dev PC — measured 2026-07-22 (the machine fidelity work now runs on).**

| Component | Reality | Consequence |
|---|---|---|
| GPU | **NVIDIA GTX 1080 Ti, 11 GB VRAM** (Pascal) | Full DX12/SM6: **Nanite, VSM, TSR, and software Lumen all available and performant.** **No RT cores** → hardware ray tracing / HW-Lumen *not available here*; the stylized fidelity pass is authored on the tuned software-Lumen path (see the 15.5 fidelity revision). 11 GB comfortably streams 4K textures. |

Final HWRT/HW-Lumen validation still requires an RTX-class card per (A); everything else in this charter is now authorable on (C).

**What this means, stated plainly:** this laptop is a perfectly good **graybox / systems / CI dev box** — exactly what Phase 1 needs, and it runs the ugly prototype fine. It is **not** the fidelity target and cannot represent shipping visuals. When Phase 2 fidelity work starts, high-fidelity scenes must be validated on target-class hardware (a stronger workstation, or the server/other machine), with this box running the editor at reduced dev-preview scalability. **We will not pretend a GTX 1050 renders RTX visuals.** Honesty here protects the schedule.

**Practical dev-box scalability preset (Phase 1, this machine):** run PIE at `sg.GlobalQuality` Medium or lower, software Lumen, VSM on but `r.Shadow.Virtual.ResolutionLodBiasLocal` raised, screen-percentage ~70%. Feel-target tuning (movement/gunplay) is judged on *responsiveness*, not on shadows — keep the graybox cheap so input latency, not GPU load, is what you're feeling.

---

## 15.3 Graphics Pipeline (Senior Graphics Programmer)

**Project baseline — already configured** in `Eclipse/Config/DefaultEngine.ini` (GDD 12.1), so the pipeline exists from day one and content grows into it:

- Lumen Global Illumination — `r.DynamicGlobalIlluminationMethod=1`
- Lumen Reflections — `r.ReflectionMethod=1`
- Virtual Shadow Maps — `r.Shadow.Virtual.Enable=1`
- Mesh Distance Fields — `r.GenerateMeshDistanceFields=True`
- DX12 / SM6, Desktop hardware class, Maximum performance target.

**To be enabled/authored as content arrives (Phase 2+):**

- **Nanite** on all suitable high-detail static geometry (modular kits, hero props, rubble, façades). Not on: skeletal meshes, translucency, foliage that needs WPO wind until Nanite foliage is validated per-scene.
- **Hardware ray tracing** *where the machine has it* (target box) for Lumen HWRT reflections/GI quality; software Lumen is the guaranteed floor.
- **Volumetric fog + exponential height fog** for atmosphere and depth (Kessara smog, Meridia dust). Local fog volumes for interiors.
- **Sky/atmosphere**: Sky Atmosphere + Volumetric Clouds where the planet's sky is visible; per-planet sky is an identity lever (03).
- **Post-processing**: physically-based auto-exposure (metering, not eyeadapt cheese), tone-mapped filmic look, bloom, subtle chromatic aberration/vignette reserved for set-pieces, motion blur budget-capped.
- **Anti-aliasing**: TSR (Temporal Super Resolution) as the primary AA/upscaler on the target box; it is the single biggest quality-per-frame lever at 4K.
- **Screen-space** effects (SSAO/contact shadows/SSR fallback) only where they add over Lumen, never fighting it.
- **Cinematic camera** (Sequencer + CineCameraActor) for signature moments (01's "signature moments"): DoF, exposure ramps, focal choices — set-pieces only, never the play camera's default.

Every one of these ships behind **UE scalability buckets** so the same content renders on min-spec.

---

## 15.4 Assets & Content (Environment / Character Artists)

Highest-quality source that the pipeline and licence allow. Priority order:

1. **MetaHuman** (Creator/Animator) for principal humans — Cinder/Voss, the companions (Mara, Dex, Reyes, Torren, Kaya, Whisper, Sela, Brick), key villains (Malachar Vex, Sera Kaine). See 15.6.
2. **Quixel Megascans / Fab** for realistic surfaces, rock, debris, industrial kit — free with UE via Fab; the backbone of environment realism.
3. **Fab marketplace** for professional-grade kits that survive a bespoke art-direction pass (never dropped in raw — see 15.5).
4. **Authored PBR materials** with a shared master-material + instance system (metal/roughness, layered wear, decals for propaganda/grime).
5. **4K textures on target hardware**, streamed; **virtual textures** for large surfaces. 8K reserved for hero surfaces only, and never on the 4 GB dev box.

**Replace-up rule:** when a placeholder or low-quality asset can be swapped for a realistic, on-direction alternative *within the current phase's scope*, do it — but a Megascan rock is not art direction. It is raw material for 15.5.

---

## 15.5 Art Direction (the anti-generic clause)

## CHOSEN DIRECTION — stylized, Borderlands-leaning (locked)

ECLIPSE's look is **stylized, not photoreal — the Borderlands family of style is the north star**: bold shapes, strong silhouettes, hand-authored surface character, ink/edge outlines, punchy readable colour, and a graphic-novel confidence. This is a deliberate choice and it is the smart one for this team size (pillar 5, ACHIEVABLE AMBITION): a stylized target reads better at command distance, ages well, forgives imperfection, and is *far* cheaper to produce at quality than photoreal — while UE 5.8's full tech stack (Nanite, Lumen, VSM, TSR, volumetrics) still drives it, now serving a stylized look instead of a photoreal one (cel/toon post + outline materials + physically-plausible-but-stylized lighting).

**Fidelity revision (owner instruction, 2026-07-22) — sharper, still stylized.** The Borderlands-leaning direction stays locked, but the fidelity bar *within* it is raised:

- **Higher polygon density via Nanite** — dense modular kits, hero props, debris. Silhouettes carry this style, so geometry gets real detail; low-poly "toon = cheap" is explicitly rejected.
- **More GI detail via software Lumen** — software Lumen is the tuned-*up* guaranteed floor (Lumen scene detail / final-gather quality raised), because the current strong dev PC has no RT cores (15.2C). Hardware RT remains an *additive* path for RTX-class machines only.
- **Richer post-processing** — SSAO layered where it adds over Lumen, punchy bloom, and **subtle film grain** as part of the graphic-novel look.
- **More particles & atmospherics** — sparks, smoke, embers, drifting ash; density within the 12.4 budgets.
- **Better anti-aliasing** — TSR at high quality; edge stability matters doubly with ink outlines.

All of it stays inside the **cel-shaded palette, the light-band toon shading, and the ink outlines** — fidelity serves the style, it never dilutes it.

**Work like a full AAA graphics studio, not a code generator** (the standing mandate): every meaningful surface gets deliberate attention. Aim for the best the engine and hardware can give, then keep pushing — **review screenshots every pass**, find the weakest element, and fix it (lighting → materials → textures → meshes → effects → detail). **Every detail of every environment matters**: no filler geometry, no bare untouched surfaces, no "good enough" corners. Grime, decals, propaganda, wear, edge highlights, small props, atmosphere — the world must feel *authored*, occupied, and specific. Ask each scene *"does this look like a modern AAA game in this style?"* and if not, iterate (see §15.8/§15.9).

Beyond the stylization, the anti-generic rules still hold — **specific to ECLIPSE, never stock.** The failure mode we reject: a technically-correct scene that looks like a marketplace demo. Each environment must feel authored by a team with a point of view:

- **Hierarchy & silhouette** — the eye is led; hero elements read at command distance.
- **Occupation & story** — checkpoints, Dominion propaganda, Veil surveillance, worker districts, damage history. The world is *ruled*, and that shows.
- **Per-planet identity** (03): Kessara's stacked underworld haze vs. Meridia's engineered-blight desolation vs. a Gate Spire's alien monumentality. Palette, sky, and material story change per planet.
- **Light as narrative** — sodium checkpoints, cold Dominion interiors, the warm contraband of a rebel safehouse.
- **Restraint** — grain/atmosphere/lens effects serve the shot; they are not a filter slapped on everything.

Reference-driven: gather real references per environment before building (art_style_bible.md in phase0 is the seed). Consistency across planets is the Golden Rule (DOCUMENTATION_README): one coordinated team, not isolated sessions.

---

## 15.6 World Design (Environment Art + Gameplay Engineer)

Open-world-grade attention to detail, applied to ECLIPSE's district/mission structure (not literal open world — the game is districts + strategy map, 03/11):

- **Modular kit + World Partition** for large districts; streaming so a planet loads within budget.
- **Detailed architecture & streets** with real wear, layered decals, and readable navigation for third-person combat + squad orders.
- **Landscape** with Nanite where appropriate, Megascan-driven materials, runtime virtual textures for blends.
- **Foliage/vegetation systems** (Procedural Content Generation / PCG) where a planet is vegetated (Sylvaris); scatter driven by rules, not hand-placed one-offs.
- **Weather & time-of-day** as a per-mission mood lever (dynamic sky/sun, volumetric response) — where it serves the mission, not everywhere for its own sake.
- **Dynamic lighting** end-to-end (Lumen); no baked-only levels.
- **Interactive elements** tied to gameplay/mission systems (cover, destructible dressing within budget, objective props → the existing objective-trigger system).
- **PCG** where it raises quality *and* saves hand-labour: debris fields, crowd-less occupation dressing, foliage, modular façade variation.

All of it inside the 12.4 budgets — a district that blows the frame budget is a bug, not a flex.

---

## 15.7 Characters (Character Artist + AI/Gameplay)

MetaHumans for principals; component-composed bodies for the squad (per 12.3, soldiers are player-quality — this already matches the code's single `AEclipseCharacter` body):

- **Faces**: MetaHuman Animator for performance capture on story beats; expression sets for barks/command feedback.
- **Motion**: high-quality locomotion + combat sets (Motion Matching where it earns its cost), control rig for adjustments; squad orders must *look* obeyed (the 9.5 promise the prototype proves in graybox first).
- **Cloth & gear**: layered outfits reading rank/faction (Dominion enforcer vs. rebel scavenged — the loadout system already gates "Scavenged"); Chaos Cloth on heroes within budget.
- **Hair & skin**: groom-based hair + subsurface skin shaders on principals; cheaper cards/approximations on crowd/enemy tiers.
- **NPC behaviour** hooked to 09's perception model — presence and reactivity read as "alive," not idle props.

Enemies and squadmates share the fairness rule (8.3/9.3): what the player's people can do, the enemy can, and vice-versa.

---

## 15.8 Continuous Improvement Loop (every phase, after the milestone gate)

Per the roadmap gates — **not** mid-graybox — run a fidelity pass:

1. **Assess** current visual quality against target references (screenshots at fixed cameras).
2. **Find the weakest link** — the one thing dragging the scene (a flat material, a lighting dead-zone, a silhouette that doesn't read, a stutter).
3. **Fix, in priority order**: lighting → materials → textures → meshes/detail → effects → animation → performance.
4. **Iterate** until the scene reads as target-AAA *and* holds budget.
5. **Bank it** as a new reference so the bar ratchets up, never down.

This loop is a Phase-2-onward ritual. In Phase 1 the equivalent loop targets *feel and correctness*, not pixels.

---

## 15.9 Quality Control

Regular, evidence-based checks (Web/Playwright-style rigour, applied to a game):

- **Play it** — does it feel and look like a modern AAA stylized (Borderlands-tier) third-person action game at command distance and in a firefight?
- **Screenshot review every pass** — capture fixed cameras at target breakpoints (1080p/1440p/4K on target hardware), study them, and treat every prop, surface, and corner as something that must earn its place. No bare or untouched geometry ships.
- **Lighting analysis** — exposure sanity, no blown/crushed zones, GI leaks, shadow acne/peter-panning.
- **Performance capture** — `stat unit`, GPU visualizer, Insights traces against 12.4 budgets.
- **Fix visual bugs** before they compound.

The recurring question — *"Does this look like a modern AAA game?"* — has one honest answer per scene. If **no**, it goes back into 15.8. (In Phase 1 the honest answer is "it's graybox, and that's correct.")

---

## 15.10 Optimization (Optimization Specialist)

Maximum *quality per frame*, not maximum settings for their own sake:

- **GPU**: profile before optimizing; Nanite cluster/overdraw discipline, Lumen quality vs. cost tuning, VSM page pressure, translucency/overdraw budgets.
- **CPU**: keep systems event-driven (14.2 — already the code's default: `bCanEverTick=false`), avoid per-frame work, async where safe.
- **Memory & streaming**: virtual textures, texture streaming pools sized to spec, World Partition streaming, Nanite streaming pool tuned.
- **Scalability**: every fidelity feature ships behind `sg.*` buckets and console-var scalability so min-spec is a setting, not a rebuild.
- **Budgets are law** (12.4): frame-time, draw-call, memory, and VRAM ceilings per platform tier. Over budget = defect.

Preserve maximum fidelity **without** creating performance problems. On the dev box specifically: profile at 1080p/reduced scalability and trust the target-hardware validation for the real numbers.

---

## 15.11 Working Method (studio cadence)

Foundation first, then dress on schedule:

1. **Strong technical base** — the architecture spine (12), event bus, deterministic cores. *(In hand: Phase 1.)*
2. **Prototype gameplay** on graybox — prove the risks. *(Active: Phase 1.)*
3. **High-fidelity environment** — the vertical-slice district at target quality. *(Phase 2.)*
4. **Characters** — MetaHuman principals, quality squad/enemy tiers. *(Phase 2→3.)*
5. **Improve graphics** — the 15.8 loop per gate. *(Phase 2→5.)*
6. **Test** — QC (15.9) each pass.
7. **Optimize** — to budget (15.10). *(Continuous, hard lock in Phase 5.)*
8. **Repeat.**

Make independent decisions that raise quality; prefer the professional solution over the quick one — **within the current milestone's scope.** Never trade the risk-first sequence for early polish. The prototype earns the right to be beautiful by first proving it's worth building.

---

## 15.12 Definition of "AAA-ready" (per fidelity scene, Phase 2+)

A scene clears the bar when: it reads at command distance; lighting is filmic and leak-free; materials and silhouettes are ECLIPSE-specific (15.5), not stock; it holds the 12.4 budget on target hardware at target resolution; and a fresh eye answers *"modern AAA?"* with yes. Until all five hold, it re-enters 15.8.

---

*Prev: [14_ai_dev_instructions.md](14_ai_dev_instructions.md). This is the final numbered document; see [00_INDEX.md](00_INDEX.md) for the full map.*
