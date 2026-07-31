# SCRIPT FORMAT — the one file shape every scene uses
*Werkdocument | aangemaakt 2026-07-31 | eigenaar: story-architect | bindend voor dialogue-writer, dialogue-critic, voice-director*

---

## Why this exists

Forty writing agents working in parallel produce forty formats unless a format is imposed. Then nothing generates, nothing imports, and a month of writing is worthless.

**One scene = one file.** That file feeds three consumers without being rewritten for any of them:

1. the **writer** (human-readable, reviewable, diffable in git)
2. the **TTS generator** (voice ID, tags, cache key inputs)
3. the **UE import** (line IDs, conditions, DataAsset assignment)

Authoring format is **YAML**. `Tools/script_to_seed.py` converts a scene file to the dialogue-seed JSON the existing `UEclipseVoiceGenerator` pipeline already eats (§16.12). Writers never touch JSON.

---

## 1. Where files live

```
Eclipse/Content/Script/
  act1/
    M1.1_thirteen_bullets/
      M1.1.S01_briefing.yaml
      M1.1.S02_the_overpass.yaml
      M1.1.S03_the_ammo_count.yaml
      M1.1.S99_debrief.yaml
    hub/
      HUB.A1.dex_first_conversation.yaml
  barks/
    SQUAD.mate_down.yaml
    DOMINION.contact.yaml
    VEIL.target_lost.yaml
```

One directory per mission. Barks live outside missions because they are systemic.

---

## 2. ID scheme

```
M1.1.S03.010
│    │   └── line number, in TENS
│    └────── scene number
└─────────── mission (or HUB.A1 / BARK.SQ)
```

**Line numbers go up in tens.** This is not decoration: it means a line can be inserted at `015` six weeks later without renumbering the scene, without breaking the cache, and without invalidating a single generated asset. Renumbering is how a script loses its audio.

**IDs are permanent.** Once a line is generated, its ID never changes. If the text changes, the ID stays and the audio regenerates (and re-costs — see §19.5).

---

## 3. Scene file schema

```yaml
# ---- header ----
scene:        M1.1.S03                # required, matches filename
mission:      M1.1                    # required
title:        "The Ammo Count"        # required, human label
act:          1                       # required
location:     "Kessara / Foundry District / Overpass"
type:         in-mission-radio        # see §4
credit_tier:  2                       # per 19.2 ladder — drives generation order

# ---- the three-sentence contract (18.7) ----
want:         "Mara needs Voss moving before the patrol cycles back."
obstacle:     "Voss has counted his rounds and the number has him."
turn:         "She hands him the number back as a promise instead of a limit."

# ---- state ----
status:       draft                   # draft | critic-pass | generated
critic:       null                    # "GO 2026-08-03" | "NO-GO 2026-08-03: reason"
words:        112                     # filled by tooling, used for credit forecasting

# ---- the lines ----
lines:
  - id:      M1.1.S03.010
    speaker: MARA
    voice:   mara                     # role key from the casting table (§19.3)
    tags:    [quietly]
    text:    "Thirteen."
    shot:    "CU Mara. Hand on his shoulder. She is not looking at him."

  - id:      M1.1.S03.020
    speaker: VOSS
    voice:   voss                     # resolved to voss_m / voss_f per player gender at build
    text:    "That's not enough."
    variants:
      pragmatist: "That's four of them if I don't miss."
      idealist:   "That's not enough."

  - id:      M1.1.S03.030
    speaker: MARA
    voice:   mara
    tags:    [pause]
    text:    "It was enough for the ones who had six."
    shot:    "She moves off. Camera stays on Voss."
```

> **Note on this example.** `"Thirteen."` is one word inside a band written as "10–25". That is deliberate and it is legal: **bands are ceilings, not ranges** (§4). A validator that fails this line is a broken validator, not a broken line.

---

## 4. Field reference

### Header fields

| Field | Required | Notes |
|---|---|---|
| `scene` | ✅ | Must match filename stem |
| `mission` | ✅ | `M1.1`–`M4.7`, `LOY.dex`, `HUB.A1`, `BARK.*` |
| `title` | ✅ | Human label, never shown in game |
| `act` | ✅ | 1–4 |
| `location` | ✅ | Planet / district / site — must exist in `03_world_design.md` |
| `type` | ✅ | Sets the line-length band (§18.3) — see below |
| `credit_tier` | ✅ | 0–5 per §19.2. `voice-director` generates in tier order. |
| `want` / `obstacle` / `turn` | ✅ | One sentence each. **A scene without all three is not written.** |
| `status` | ✅ | `draft` → `critic-pass` → `generated` |
| `critic` | ✅ | Verdict string. Only `GO` scenes may be generated. |
| `words` | auto | Written words — everything in the file. A workload figure. |
| `words_heard` | auto | What **one playthrough** hears: base lines, one variant per branch point. The pacing and quality figure. |
| `words_generated` | auto | What TTS must actually produce: every variant, ×2 for Voss gender lines. **This is the credit figure.** |

### Why three word counts — RULING L1-R15

A single `words` field punishes exactly the scenes `21_quality_mandate.md` asks for. M1.5 measures 1,865 written words against a §18.1 norm of ~1,600 and looks bloated; strip the ~50 variant lines and a playthrough hears ~1,250. **The scene was not over-long, it was well-branched, and the metric said the opposite of the truth.**

And neither number is the one that costs money. Credits are spent per *generated* line, so a four-axis Voss beat costs four generations — eight with gender — while contributing one line to `words_heard`. Forecasting Act 1 against `words` underestimates the spend; forecasting against `words_heard` underestimates it badly.

| Question | Field |
|---|---|
| Did the writer do enough work? | `words` |
| Is the scene the right length to play? | `words_heard` |
| What will this cost to generate? | **`words_generated`** |

`words_generated` is the input to the Q-7 variant-policy decision, and it is the only one of the three that belongs in a budget.

### `type` values — these set the enforced line-length band

| `type` | Band (§18.3) |
|---|---|
| `bark` | 3–8 words |
| `callout` | 2–6 |
| `ambient` | 6–16 |
| `in-mission-radio` | 10–25 |
| `walk-and-talk` | 10–25 |
| `hub` | 12–35 |
| `cutscene` | 20–60 |
| `oration` | up to 120 — **max four in the entire game** |

#### Bands are ceilings — RULING L1-R1 (story-architect, 2026-07-31)

Read §18.3's own justifications: *"anything longer is not heard"*, *"must land inside the reaction window"*, *"player may walk away mid-line"*, *"keep one idea per line"*, *"camera is holding"*. **Every one of them argues an upper limit. Not one explains why a line may not be shorter.** And `21_quality_mandate.md` §21.4 is explicit: *"Kort per regel. Gul in aantal."*

So the lower number is **the register the scene should reach somewhere**, never a per-line minimum:

| Check | Applies | Fails when |
|---|---|---|
| **Ceiling** | every line | word count **above** the band's upper number |
| **Register** | scenes of **≥6 lines** | **no** line in the scene reaches the band's lower number — i.e. the scene never uses the room its delivery context gives it |
| **Spread** | scenes of ≥6 lines | longest ÷ shortest **< 3×** (§18.3's variance rule) |

Scenes under six lines are exempt from register and spread: a four-line scene has no meaningful distribution, and compression is the point of a short scene.

**Why the old reading was arithmetically impossible.** Treating bands as ranges caps in-scene variance at 2.5× for `in-mission-radio` and 2.9× for `hub`, while §18.3 demands ≥3×. The two rules could not both be satisfied. With the floor lifted, ≥3× is easy to hit and still catches exactly the failure it was invented for — every turn the same length, the loudest tell that a machine wrote it (§18.9 D). Measured on the calibration mission: `M1.1.S03` scores 9×, `M1.1.S05` scores 15×.

#### Per-line band override — `band:`

A radio scene that contains a firefight needs callout-length lines inside it. That is most combat scenes in the game, so it gets a field rather than a convention:

```yaml
  - id:      M0.0.S00.010     # illustrative id only — act 0 does not exist
    speaker: MARA
    voice:   mara
    band:    callout          # overrides the scene's type-derived band for THIS line
    text:    "Take it."
```

> **Examples in this document use `M0.0.S00.*`, never a real ID.** The first draft of this section pointed at `M1.1.S04.070`, whose actual shipped text is something else entirely. IDs are permanent and forty writers copy from this file; an example that collides with a real line is how a scene loses its audio. Act 0 cannot exist, so `M0.0` can never collide.

Lines carrying `band:` are checked against the override and are **excluded from the register check** (they are not trying to use the scene's register). They still count toward spread.

`dialogue-critic` and `validate_script.py` check all three rules mechanically. This is why `type` is required.

### Line fields

| Field | Required | Notes |
|---|---|---|
| `id` | ✅ | Permanent. Tens. Never reused, never renumbered. |
| `speaker` | ✅ | UPPERCASE canon name from `00_INDEX.md` glossary, **or** a role speaker (below) |
| `voice` | ✅ | Role key from the casting table — **not** a raw ElevenLabs ID (below) |
| `text` | ✅ | The line. One breath. No stage directions inside the text. |
| `tags` | ○ | From the approved set (§19.4). Max two. |
| `band` | ○ | Overrides this line's length band. Use for combat callouts inside a radio scene. |
| `shot` | ○ | Camera/staging note. **Never voiced.** Read by the cinematic pass. |
| `condition` | ○ | State expression — see grammar below |
| `choice` | ○ | Marks this line as a player option — see below |
| `variants` | ○ | Voss personality-axis variants: `pragmatist`/`idealist`/`personal`/`strategic` |
| `interrupts` | ○ | `true` if this line cuts off the previous one (previous line must end `--`) |
| `direct_beat` | ○ | `true` marks one of the ~30 rationed lines allowed to state emotion outright (§18.9 A). Requires a one-line justification in `note`. |
| `note` | ○ | Writer's note to the critic. Never voiced, never shipped. |

### `voice` — the role-key namespace — RULING L1-R5

Three namespaces were in circulation: this document's `mara_sovann`, the live casting file's `mara`, and keys writers invented when neither fit (`dex_callum`, `elin_reyes`). **The casting file wins**, because its keys are the ones already bound to real ElevenLabs voice IDs and to the tier plan (`progress_media/casting/casting_stage1.json`).

| Use | Key |
|---|---|
| Named characters | `mara` `dex` `reyes` `brick` `sela` `torren` `kaya` `whisper` `threx` `aegis` `vex` `kaine` `callis` |
| Voss | **`voss`** — a logical key, resolved to `voss_m` / `voss_f` per player gender at build |
| Role pools | `eclipse_fighter_a`…`_d` · `dominion_conscript_a`…`_c` · `veil_operative_a` `_b` |
| Not yet cast | `petra` · `iron_chorus_emissary` · `dominion_officer` · `civilian_kessara_a` `_b` — **owner questions Q-3/Q-4/Q-5, blocking for Tier 2** |

**Renaming a `voice` key costs nothing, and this was measured, not assumed.** `EclipseGenerateVoicesCommandlet.cpp:325` builds the cache key from `Voice->ElevenLabsVoiceId`, not from the script-side key:

```cpp
const FString Key = UEclipseDialogueVoiceSubsystem::MakeCacheKey(
    Voice->ElevenLabsVoiceId, Voice->ModelId, Line.Emotion, Line.Text);
```

The indirection this document promised is real. The actual risk is not cache invalidation — it is a key that resolves to **nothing**, which either fails generation or silently falls back. Hence the validator check in §6.

### `speaker` — role speakers — RULING L1-R6

A pure glossary check can never pass, because `ACT1_OVERVIEW.md` AR-1 and AR-10 deliberately keep Ember's rank and file nameless. Speaker is valid if it is a glossary name, **or** matches `^(FIGHTER|CONSCRIPT|VEIL|CIVILIAN|OFFICER|PRISONER)_[A-Z]$`, **or** is a named-role token from this list:

| Role token | Who |
|---|---|
| `EMISSARY` | the Iron Chorus emissary (§18.4 row; owner question Q-4 may still give him a proper name) |

**Amended 2026-07-31 (L1-R6b), on a correct escalation from the M1.5 writer.** The original pattern justified itself on AR-1/AR-10, which are about Ember's *rank and file* — that reasoning never covered the second lead of a dialogue mission. Burying him in `FIGHTER_C` would have made sixty lines unreadable for the strip test, which is the one test that scene exists to pass. A named unnamed character is not a contradiction: he has a role, and his refusal to give names is his §18.4 "never".

Anything else fails — including a near-miss on a canon name, which is the failure the check exists to catch.

### `condition` and the two state namespaces — RULING L1-R3 / L1-R4

```yaml
condition: 'story.brick_recruited == true'            # persisted campaign state
condition: 'story.m11_conscript_choice == "bound"'    # persisted, multi-value
condition: 'run.zero_casualty == true'                # this mission run only
```

| Namespace | Lives in | Survives the mission | Example |
|---|---|---|---|
| `story.` | `FEclipseCampaignState.StoryFlags` | yes | `story.char_maradead` |
| `run.` | `FEclipseMissionOutcome` | **no** | `run.zero_casualty`, `run.alarm_raised`, `run.ghost` |

**Run facts are not story flags.** A debrief scene asking "did anybody go down" is asking about the run, and persisting that as campaign state would grow the save with transient data for every mission in the game. `run.zero_casualty` reads the existing downed-soldier latch (SPEC-P2-04 amendment); it needs no new state at all.

**Multi-value story flags use gameplay-tag leaves.** `StoryFlags` is a `TArray<FGameplayTag>` with no values, so a three-way choice is three mutually exclusive leaf tags under one parent:

```
story.m11_conscript_choice == "bound"   ⇄   Story.Choice.M11_Conscript.Bound
story.brick_recruited      == true      ⇄   Story.Beat.BrickRecruited   (presence)
```

Zero schema change, and the parent tag stays queryable as "has this choice been made at all". `script_to_seed.py` performs the mapping; writers only ever use the lowercase form. **Any choice with more than two outcomes uses this shape** — collapsing three outcomes into a boolean is story loss disguised as simplification.

### `silence` — a branch that deliberately plays nothing (01-08)

`BRANCH` asks: N leaves set, N leaves handled. That is the right question, and it caught
three real defects. But it has no way to hear **"nothing belongs here"** — so a scene the
critic has explicitly cleared stays red forever, and **a bar that is always red hides as
much as a test that never goes red.**

Optional scene-header field. Keys are flag *values*, values are the reason:

```yaml
silence:
  left: "Both existing echoes are ledger facts, and the left branch spends nothing, so the
         ledger has nothing to report. That player already gets the hardest echo of the
         three and it is on the right beat."
```

**It buys nothing for free.** Four things are enforced, each with a fixture that proves the
check can go red (`Eclipse/Tools/tests/script_fixtures/silence_*`):

| | |
|---|---|
| A reason under 40 characters | `SILENCE` — a silence you cannot explain is an oversight wearing the costume of a choice |
| Declared for a branch that **is** played | `SILENCE` — a stale silence covers the *next* real omission, which is worse than no silence at all |
| Declared for a value the flag does not have | `SILENCE` |
| Declared in a scene that does not read the flag | ignored — a silence in one story may never cover a hole in another |

**And it never becomes invisible.** Every accepted silence is printed above the findings —
*and on a completely clean run too*, which is where it would otherwise vanish exactly when
nobody is looking. That behaviour is pinned by a test, and the test found the bug: the first
version only printed on the findings path.

The scene header is otherwise flat, so `silence` is the one mapping the parser accepts there
(`HEADER_MAPS`). Keep that list short — every name in it changes how a block is read.

### `choice` — player options — RULING L1-R2

Forty-two missions of branching had no way to say "these lines are the player's options". It has one now:

```yaml
  - id:      M1.1.S05.160
    speaker: VOSS
    voice:   voss
    choice:
      group: m11_conscript                              # options sharing a group are one prompt
      set:   'story.m11_conscript_choice = "bound"'     # same grammar as condition
      label: "Dress the wound, then leave him."                                # optional wheel text — NOT the spoken line
    text:    "Dressing. Above the hip."
```

| Subfield | Required | Notes |
|---|---|---|
| `group` | ✅ | Options with the same `group` form one prompt. Order-independent; lines need not be contiguous. |
| `set` | ✅ | One or more assignments, `story.` or `run.` |
| `label` | ○ | Short wheel/summary text. **Never voiced.** Omit and the UI truncates `text`. |

`label` exists from day one on purpose: the wheel text and the spoken line are different pieces of writing in every game that has both, and retrofitting that distinction after twenty missions is how a script acquires a thousand mismatches.

---

## 5. Bark set schema

Barks are shaped differently — one trigger, many variants, no scene structure.

```yaml
bark_set:    SQUAD.MATE_DOWN
trigger:     Event.Squad.MateDown      # must exist in Eclipse/Docs/EventCatalog.md
faction:     eclipse                   # eclipse | dominion | veil
type:        bark
credit_tier: 1
status:      draft
critic:      null

variants:
  - id:    BARK.SQ.MD.010
    voice: eclipse_fighter_a
    tags:  [shouting]
    text:  "{name} is down! Cover — cover me!"

  - id:    BARK.SQ.MD.020
    voice: eclipse_fighter_b
    tags:  [urgent]
    text:  "They got {name}. Push up, push up!"

  - id:    BARK.SQ.MD.030
    voice: eclipse_fighter_c
    text:  "{name}! Talk to me!"
```

**Rules:**
- `trigger` must be a real event from `Eclipse/Docs/EventCatalog.md`. A bark set pointing at a non-existent event fails the critic. If the event does not exist yet, that is a systems task, not a writing task — escalate, do not invent.
- **6–12 variants** per set (§18.5).
- **`{name}` is the roster-name slot, and it is not bark-only — RULING L1-R8.** It is legal in any `text:`, in scenes as well as bark sets. `ACT1_OVERVIEW.md` AR-10 requires it in mission context: the fighter Threx names in M1.7.S04 is the fighter who dies in M1.8.S07, and that name comes from the live roster. Both uses obey the same two constraints: **the surrounding line must be pronoun-free** (no "he", no "her" — the roster decides gender at runtime), and the slot must refer to a roster member the player has met. Where the roster is fixed (named companions), generate per-name; where it is procedural, the slot is spoken by a separate name-clip. `voice-director` decides which and records it.
- Every set needs **all three faction vocabularies** where the trigger can fire for all three. Same trigger, different words.

---

## 6. Validation

`Tools/`validate_script.py` (bestaat en draait in de bar sinds 01-08) — small, one afternoon) runs in the green bar alongside `EclipseValidateData`:

| Check | Fails on |
|---|---|
| Schema | Missing required field, unknown field |
| ID uniqueness | Duplicate ID anywhere in `Content/Script/` |
| ID monotonicity | Line IDs out of order within a scene |
| Speaker canon | Speaker not in the `00_INDEX.md` glossary **and** not a role speaker (§4, L1-R6) |
| **Voice resolves** | `voice` does not resolve to a row in the casting table. **This is the one that matters** — an unresolvable key fails generation or silently falls back to a default voice. A key *rename* is harmless (§4, L1-R5). |
| **Length ceiling** | Line **above** its band's upper number. Never fails a line for being short. |
| **Register** | Scene of ≥6 lines where no non-`band:`-overridden line reaches the band's lower number |
| **Spread** | Scene of ≥6 lines whose longest ÷ shortest line is < 3× (§18.3) |
| Tag set | Tag outside the §19.4 approved set without a `note` |
| **Choice integrity** | A `choice.group` with fewer than two options; a `set` naming a flag no other scene or system reads; two options in one group setting different flags |
| **Condition resolves** | A `condition` naming a `story.`/`run.` fact that nothing ever sets |
| Trigger | Bark `trigger` not in `EventCatalog.md` |
| Location | `location` not in the act's location registry (`phase0/beats/ACT1_OVERVIEW.md` §7 — see finding C-1; **not** `03_world_design.md`, which has no district names) |
| Generation guard | `status: generated` on a scene whose `critic` is not `GO` |

**The two dangling-reference checks are worth more than they look.** A `condition` on a flag nothing sets is a line that never plays, and a `set` on a flag nothing reads is a choice with no consequence. Both are invisible in review and both are certain to happen across 42 missions.

**The generation guard is the money-saver.** It is the mechanical reason a bad line cannot reach the API.

---

## 7. Lifecycle

```
story-architect     writes the beat sheet         → scene stub with want/obstacle/turn
dialogue-writer     writes the lines              → status: draft
dialogue-critic     scores against 18.9           → status: critic-pass, critic: GO
voice-director      generates in credit_tier order → status: generated
                    appends spend to VOICE_LEDGER.md
```

A scene moves forward only. A `NO-GO` sends it back to `draft` with the reason in `critic:` — the writer fixes and resubmits. **No scene is generated on a `NO-GO`, ever, for any reason, including deadline pressure.** The deadline is not worth permanently shipping a bad line in a game that has no re-record budget.
