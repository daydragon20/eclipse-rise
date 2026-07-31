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
    voice:   mara_sovann
    tags:    [quietly]
    text:    "Thirteen."
    shot:    "CU Mara. Hand on his shoulder. She is not looking at him."

  - id:      M1.1.S03.020
    speaker: VOSS
    voice:   voss                     # resolved per player gender at build
    text:    "That's not enough."
    variants:
      pragmatist: "That's four of them if I don't miss."
      idealist:   "That's not enough."

  - id:      M1.1.S03.030
    speaker: MARA
    voice:   mara_sovann
    tags:    [pause]
    text:    "It was enough for the ones who had six."
    shot:    "She moves off. Camera stays on Voss."
```

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
| `words` | auto | Tooling fills; drives credit forecasting |

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

`dialogue-critic` checks every line against its scene's band mechanically. This is why `type` is required.

### Line fields

| Field | Required | Notes |
|---|---|---|
| `id` | ✅ | Permanent. Tens. Never reused, never renumbered. |
| `speaker` | ✅ | UPPERCASE canon name from `00_INDEX.md` glossary |
| `voice` | ✅ | Key into the §19.3 casting table — **not** a raw ElevenLabs ID. Indirection means a re-cast is one table edit. |
| `text` | ✅ | The line. One breath. No stage directions inside the text. |
| `tags` | ○ | From the approved set (§19.4). Max two. |
| `shot` | ○ | Camera/staging note. **Never voiced.** Read by the cinematic pass. |
| `condition` | ○ | Flag expression, e.g. `story.brick_recruited == true` |
| `variants` | ○ | Voss personality-axis variants: `pragmatist`/`idealist`/`personal`/`strategic` |
| `interrupts` | ○ | `true` if this line cuts off the previous one (previous line must end `--`) |
| `direct_beat` | ○ | `true` marks one of the ~30 rationed lines allowed to state emotion outright (§18.9 A). Requires a one-line justification in `note`. |
| `note` | ○ | Writer's note to the critic. Never voiced, never shipped. |

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
- `{name}` is the roster-name slot. Where the roster is fixed (named companions), generate per-name; where it is procedural, the slot is spoken by a separate name-clip. `voice-director` decides which and records it.
- Every set needs **all three faction vocabularies** where the trigger can fire for all three. Same trigger, different words.

---

## 6. Validation

`Tools/validate_script.py` (to be built — small, one afternoon) runs in the green bar alongside `EclipseValidateData`:

| Check | Fails on |
|---|---|
| Schema | Missing required field, unknown field |
| ID uniqueness | Duplicate ID anywhere in `Content/Script/` |
| ID monotonicity | Line IDs out of order within a scene |
| Speaker canon | Speaker not in the `00_INDEX.md` glossary |
| Voice key | `voice` not in the §19.3 casting table |
| Tag set | Tag outside the §19.4 approved set without a `note` |
| Length band | Line outside its `type` band (§18.3) |
| Trigger | Bark `trigger` not in `EventCatalog.md` |
| Location | `location` not in `03_world_design.md` |
| Generation guard | `status: generated` on a scene whose `critic` is not `GO` |

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
