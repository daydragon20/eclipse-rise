# Dialogue Plugin: Buy vs. Build — Decision
*Phase 0 deliverable | GDD refs: 12.3 (dialogue system), 12.2 (in-house plugin list), 13.2 | Skill owner: Game Architecture Expert*

---

## Decision

**Build in-house (`EclipseDialogue` plugin), phased.** Phase 1 ships no dialogue; Phase 2 builds the minimal runtime; a licensed *editor front-end* is re-evaluated at the Phase 2 gate only if authoring throughput proves painful.

## Requirements (from the Bible)

1. Condition queries against `FCampaignState` + the two personality axes (GDD 2.4, 12.3) — deeply bespoke.
2. **Barks are the volume driver** (GDD 4.2.2, 12.3): tag-queried line pools (soldier persona × event × campaign state), not branching trees. This is the workload that must scale to thousands of lines.
3. Branching conversations for companions/story (GDD 2.5, 2.10 dialogue-boss finale) — moderate node-graph needs, heavy condition needs.
4. Save/serialization integration under our versioned save contract (GDD 12.3, 14.3.6).
5. TTS placeholder pipeline during development; recorded VO at production.

## Options considered

| Option | Verdict |
|---|---|
| **Licensed UE dialogue plugins** (branching-tree oriented) | Rejected as the core: they model requirement 3 well, requirements 1–2 poorly. The bark matrix — our real volume — would be fought through a tree UI it wasn't built for; CampaignState conditions and save versioning would need invasive forks anyway. |
| **articy:draft + importer** | Strong authoring UX for writers, but an external source-of-truth conflicts with "the campaign is one struct tree" and adds pipeline/licensing weight a 3–8 person team shouldn't carry in pre-production. Reconsider only as a *front-end* at Phase 2 gate. |
| **In-house, phased** (chosen) | The runtime is small because the hard parts (conditions, state, save) must be bespoke regardless; the bark system is a tag-query over line pools — closer to our event bus than to any tree plugin. GDD 12.2 already reserves `EclipseDialogue` as an in-house plugin; this decision confirms it. |

## Phasing

- **Phase 1:** nothing. Squad acknowledgment/refusal lines (SPEC-P1-06) are plain `DT_SquadOrderDefs` line pools — deliberately *not* the dialogue system, so the prototype can't grow a premature dependency.
- **Phase 2 (vertical slice):** `EclipseDialogue` v1 — bark resolver (tag query → line pool → cooldown/priority), linear + simple-branch conversation runtime as a DataAsset graph, condition function library against CampaignState, DataTable authoring (no custom editor yet). TTS placeholder hookup.
- **Phase 2 gate review:** if writers' throughput on DataTable authoring is the bottleneck, evaluate a licensed graph editor or articy as *authoring front-end only*, compiling into our runtime assets. The runtime is never licensed.

## Risk accepted

An in-house editor UX will be worse than commercial tools in Phase 2. Accepted because the slice's dialogue volume is small (M1.1–M1.4 + base scenes), and the alternative risk — a licensed runtime fighting our save/state architecture for the whole project — violates priority order 14.1 (architecture wins over speed).
