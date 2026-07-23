# PART 13 — DEVELOPMENT ROADMAP
**ECLIPSE: RISE OF THE RESISTANCE**
*Game Design Bible — Document 13 of 14 | Assumption: 3–8 person core team + heavy AI-assisted development; marketplace/megascan assets where possible*

---

## ACTIVE_MILESTONE

> **Current milestone: PHASE 2 — VERTICAL SLICE, "Thirteen Bullets".**
> Scope: per `phase0/specs/SPEC-P2-00` (build order SPEC-P2-01…09) — Act 1's opening ~3 hours at target quality: squad of 4 with 3 classes, Command Mode final feel, Hollow Point walkable base, authored M1.1–M1.4, one liberation instance, save v1, UI v1, fidelity district per the 15.5 revision, audio infra (16.7).
> Entered 2026-07-23 on **explicit owner instruction** (autonomy-loop directive; DOCUMENTATION_README hierarchy rule 4 satisfied) with Phase 1 fully green (build ✓, 31/31 tests ✓, live loop playable). **Phase-1 carryover, standing owner action: the 13.2 gate playtest** ("do testers voluntarily play a second loop?") — the answer remains the owner's call and is recorded when played; it does not block Phase-2 work under the owner instruction. Phase 0 carryover unchanged: CI runner, 10 concept pieces, 5 feel-reference clips.
> Do not implement features beyond this milestone unless explicitly instructed (see DOCUMENTATION_README.md, Document Hierarchy rule 4).
> *Update this block — and nothing else in this file — when a milestone gate is passed.*

---

## 13.1 Guiding Sequencing Logic

**Build order principle: risk first, content last.** The project's existential risks are (1) the ground↔strategy state loop, (2) squad AI quality, (3) the hybrid battle. If those three are fun and stable, everything else is production. Therefore the prototype is NOT a pretty level — it is the ugliest possible version of the full loop.

**Scope ladders (pre-committed cut lines, decided now, not in crisis):**
- Planets: 10 → cut to 8 (merge Elystra content into Meridia; Vel'Naar into Krad-9 interiors) 
- Fleet: full system → cut presentation to 2D tactical map (simulator unchanged)
- Companions: 8 → 6 (Brick and Sela have highest cut-resistance: morale + politics carriers)
- NG+/variant starts: first post-launch item, never launch-blocking

## 13.2 Phases

### Phase 0 — PRE-PRODUCTION (3 months)
This document → detailed feature specs for Phase 1 only; UE project setup, CI (build + data-validation + save-migration tests from day one); art style bible + one concept piece per planet; dialogue-plugin buy/build decision; graybox movement/gunplay feel target locked with reference clips.

### Phase 1 — PROTOTYPE, "The Loop" (6 months) 
One graybox district + a 6-node strategy mini-map + a menu-only base. Prove: mission → consequence → strategy choice → preparation → mission (full circle); squad of 2 with orders that *feel obeyed*; permadeath + memorial stub; economy ledger driving one production choice. **Gate question: "Do testers voluntarily play a second loop?"** Kill/pivot honestly here.

### Phase 2 — VERTICAL SLICE, "Thirteen Bullets" (9 months)
Act 1's opening 3 hours at target quality: Kessara Underworks polished slice, Hollow Point walkable base (4 facilities), squad of 4 with 3 classes, Command Mode final feel, first authored missions (M1.1–M1.4), one liberation-template instance, save system v1, UI stack v1. Purpose: quality benchmark + (if sought) funding/partner material. **Gate: slice reviews as "I want the rest of this game."**

### Phase 3 — EARLY BUILD, "One Planet War" (12 months)
Kessara + Tarsis complete; Act 1 full + Act 2 skeleton; all 9 classes; base tiers 1–2; Dominion Response Tiers 0–3; Mission Generator v1 on both planets; hybrid battle v1 (the Siege of Hollow Point as its testbed); economy full loop; officer/company layer v1. Playable campaign of ~25 h. **Gate: 10-hour external playtests retention.**

### Phase 4 — ALPHA, "The Whole War, Ugly" (12 months)
All 10 planets graybox-to-dressed; all acts playable start-to-finish; fleet layer in; all twists/branches implemented; tech tree complete; content-complete definition: every mission/template/system exists, polish debt allowed. Begin monthly full-campaign automated playthrough (bot-driven smoke of the strategic layer) + save-integrity soak tests. **Gate: full campaign completable without dev intervention.**

### Phase 5 — BETA (8 months)
Content lock. Balance passes per act using telemetry (casualty rates, economy curves vs. Part 6 targets); VO recording + implementation; performance to budget on min-spec; accessibility completion; localization (EN VO; FIGS+NL+PL+BR+RU+ZH text); closed beta program per act. **Gate: crash-free rate ≥ 99.5%, act-completion funnel healthy.**

### Phase 6 — RELEASE + POST (3 months to gold + ongoing)
Gold, day-one patch discipline, launch-window hotfix team; post-launch order: NG+/variant starts → QoL from telemetry → free content drop (one new planet campaign: pre-designed "Ashfall" reserve concept) → evaluate expansion.

**Total: ~4.5 years.** Honest for this scope at this team size; compressible mainly by the Phase 3/4 content overlap if AI-assisted content pipelines (Part 14) prove out in Phase 2 — the roadmap treats AI acceleration as *upside, not plan-of-record* (Rule 4: realistic development).

## 13.3 Team Shape (core roles across phases)
Creative/design lead, systems programmer ×2 (or 1 + heavy AI agent usage), technical artist, environment artist, narrative designer (part-time until Phase 2), producer/QA hybrid; contract: VO, music, key art, additional environment art in Phases 4–5.

---

*Next document: [14_ai_dev_instructions.md](14_ai_dev_instructions.md).*
