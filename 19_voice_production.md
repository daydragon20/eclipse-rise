# PART 19 — VOICE PRODUCTION
**ECLIPSE: RISE OF THE RESISTANCE**
*Game Design Bible — Document 19 of 19 | Casting, audio tags, the credit ladder, the August sprint*

---

> **Purpose.** `18_writing_standard.md` says how a line is written. This document says how it becomes audio without wasting a single credit. It is the operating manual for `voice-director`.
>
> **Relationship to Part 16.** `16_audio_system.md` defines the *architecture* (`UDialogueVoiceSubsystem`, `UEclipseVoiceGenerator`, the hash cache, the security rules). All of that stands unchanged. This document defines the *production policy* on top of it, and **amends §16.13** (see §19.1).

---

## 19.1 Amendment to §16.13 — AI voice is the shipping voice

Part 16 assumed final story VO would be professionally recorded, with ElevenLabs covering placeholders and ambient only. **Owner decision (2026-07-31): that is not the plan.** ECLIPSE is built by one person; there is no VO recording budget and there will not be one. ElevenLabs output is the **shipping voice** for the entire game.

Three consequences follow, and all three are load-bearing:

1. **Every generated line is final.** There is no "we'll re-record it properly later" safety net. The line that gets generated is the line that ships. This raises the bar on §18.9 — a scene that fails the anti-slop gate must never reach the generator.
2. **Voice casting is permanent.** The cache key is `hash(voiceId + text + emotion + modelId)`. Changing a character's voice ID **invalidates every line that character has ever spoken** and re-costs all of it. Casting is locked once, before bulk generation, in §19.3.
3. **Commercial rights must be confirmed.** ElevenLabs paid tiers grant commercial usage of generated audio, but the exact terms depend on the plan and on whether a voice comes from the shared Voice Library (some library voices carry their own terms). **Owner action, before any bulk spend: confirm commercial use rights on the current plan and note it in this document.** This is a shipping blocker, not a detail.

**Backlog #29 in `phase0/EXECUTION_PLAN.md` ("VO-opname + implementatie", Phase 5) is hereby narrowed** to "VO quality pass on AI voices" — no recording contracts.

---

## 19.2 The Budget — 310,000 credits, one month, no rollover

**The situation.** The account holds **310,000 credits**, they expire at the end of the month, and they do not roll over. A €11 top-up month is possible but the owner would rather not. So: spend it all, spend it on the right things, and spend nothing twice.

**The arithmetic.** ≈1 credit = 1 character. ~6 characters per word including spaces.

- 310,000 credits ≈ **~51,000 spoken words**
- The full spoken script is **~132,000 words** (§18.1) ≈ ~790,000 credits

**So roughly 40% of the game's speech fits in this month.** That is not a failure — it is the correct amount, because the other 60% is dialogue for acts whose beats are not yet locked, and generating those now would mean paying for lines that change.

### The credit ladder — spend in this order

Ordered by *lowest rewrite risk × highest perceived value*. Do not start a tier until the one above it is complete.

| Tier | What | Credits | ≈ Words | Why here |
|---|---|---|---|---|
| **0** | **Casting tests** — one signature line per candidate voice (§19.3) | 5,000 | 800 | Must happen first. Getting casting wrong after Tier 1 costs 100k to fix. |
| **1** | **Bark & systemic library** — the 16 priority triggers × 6–12 variants × 3 faction vocabularies | 95,000 | 15,800 | Safest spend on the board. The systems already exist, the lines never change, and this is the single biggest "the world is alive" gain per credit. |
| **2** | **Act 1 story dialogue** — Prologue + M1.1–M1.8 | 90,000 | 15,000 | Beats are locked (SPEC-P2-04 ACCEPTED, no prologue, recap opening). Low rewrite risk. |
| **3** | **Adaptive music** — main theme, Kessara layers, combat layers, Dominion faction stems | 45,000 | — | Front-loaded once, then near-zero forever (§16.7). Never-silent floor is a §16 requirement. |
| **4** | **Act 1 hub & companion conversations** — Mara, Dex, Brick, Reyes at Hollow Point | 35,000 | 5,800 | Establishes the companion voices the player lives with for 25 hours. |
| **5** | **Core SFX one-shots** — weapons, impacts, UI, ambient beds | 20,000 | — | Cheap, high polish return. |
| **—** | **Reserve** — retakes, mistakes, a mis-tagged batch | 20,000 | — | **Do not plan this away.** A single bad batch without reserve means a €11 top-up. |
| | **Total** | **310,000** | | |

**Hard rules for the sprint:**

- **Nothing is generated that has not passed `dialogue-critic` with GO.** No exceptions, no "we'll check it after". A failed line that gets generated is money burned permanently.
- **Batch, never drip.** Generate per scene or per trigger-set, never line by line — batching is where mistakes get caught before they multiply.
- **Log every batch.** `voice-director` appends actual credit spend to `phase0/VOICE_LEDGER.md` after each run. The ladder above is a plan; the ledger is the truth.
- **Stop at 290,000.** The last 20,000 is reserve. If a tier is not reached, it is not reached — it waits for next month's writing, which will be better anyway.

**What happens to Acts 2–4?** They get *written* this month (writing costs nothing) and sit as text with complete script files. The moment credits exist again, they generate with one command and zero further authoring. That is the whole point of the layer discipline: the expensive-to-change work is done, and the cheap-to-defer work is deferred.

---

## 19.3 Casting

**Casting is locked before Tier 1 and never changed.** Re-casting a character re-costs every line they have.

### Protocol

1. `voice-director` shortlists 2–3 candidate ElevenLabs voices per role, matched on age, register, and accent-neutrality.
2. Each candidate speaks **the same signature line** — a line chosen to expose the character's fingerprint (§18.4). Suggested signature lines:
   - Mara: *"We don't get to be tired. Not yet."*
   - Dex: *"That's not a reactor. That's a bomb with a job."*
   - Reyes: *"I do not need you to be brave. I need you to be still."*
   - Torren: *"[pause] You already know what I'd do."*
   - Kaya: *"Okay so — bad news, worse news, or the news where we all die?"*
   - Whisper: *"It is known that the lane opens at third shift. What is done with that is not known."*
   - Sela: *"They didn't take your brother. We let them."*
   - Brick: *"Tam. Oyelaran. Vic."*
   - Vex: *"Before us, one in nine children on Meridia did not reach eleven. One asks what that was worth."*
   - Kaine: *"I am going to burn the eastern span. I am telling you so you can move your people."*
   - Threx: *"You have your aunt's hands. Did anyone ever tell you that?"*
   - AEGIS: *"Compliance probability is falling. Intervention is scheduled."*
3. **Owner listens and picks.** This is Nathan's call, not the agent's — voice is taste, and he has to live with these for a year.
4. The chosen `voiceId` goes into the casting table below **and into the dialogue DataAsset**, and is treated as canon.

### Casting table

*(to be filled by `voice-director` during Tier 0; committed once locked)*

| Character | Voice ID | Model | Base settings (stability / similarity / style / speed) | Locked |
|---|---|---|---|---|
| Voss "Cinder" (F) | — | — | — | ☐ |
| Voss "Cinder" (M) | — | — | — | ☐ |
| Mara Sovann | — | — | — | ☐ |
| Dex Callum | — | — | — | ☐ |
| Dr. Elin Reyes | — | — | — | ☐ |
| Torren Vale | — | — | — | ☐ |
| Kaya Renn | — | — | — | ☐ |
| Whisper | — | — | — | ☐ |
| Sela Vann | — | — | — | ☐ |
| Brick (Oram Bex) | — | — | — | ☐ |
| Malachar Vex | — | — | — | ☐ |
| Sera Kaine | — | — | — | ☐ |
| Dahl Threx | — | — | — | ☐ |
| Oren Callis | — | — | — | ☐ |
| AEGIS | — | — | — | ☐ |
| Dominion conscript A/B/C | — | — | — | ☐ |
| Veil operative A/B | — | — | — | ☐ |
| Eclipse fighter A/B/C/D | — | — | — | ☐ |

**Faction voice rule (§18.5):** the bark voices are not interchangeable filler. Dominion conscripts must sound young and procedural; Veil operatives calm and clinical; Eclipse fighters improvised and personal. Three distinct casting registers, chosen deliberately.

---

## 19.4 Audio Tags — the delivery standard

ElevenLabs v3 takes bracketed tags inline to direct delivery. They are powerful and easy to waste.

### Rules

1. **The voice must be able to do the tag.** A voice cast for shouting will not whisper convincingly. Tags direct within a voice's range; they do not replace casting. If a character needs both extremes, that is a casting constraint, decided at Tier 0.
2. **The text must support the tag.** `[nervous] Everything is fine.` works. `[nervous] I am experiencing significant fear.` does not — the tag and the line fight each other. Write the emotion into the words *and* the tag.
3. **One tag per line, at most two.** Stacked tags produce mush.
4. **Tags go at the start of the delivery they affect**, not at the start of the line by habit.
5. **`[pause]` and `[beat]` are content**, per §18.7. Torren's silences are written, not accidental.

### Approved tag set

Keep the vocabulary small so delivery stays consistent across 132,000 words.

| Category | Tags |
|---|---|
| **Volume** | `[whispering]` `[shouting]` `[quietly]` |
| **Emotion** | `[nervous]` `[angry]` `[exhausted]` `[amused]` `[cold]` `[urgent]` `[grieving]` |
| **Non-verbal** | `[sighs]` `[laughs]` `[exhales]` `[clears throat]` |
| **Timing** | `[pause]` `[beat]` |

Anything outside this set requires a note in the script file explaining why. Novelty tags are how a script drifts.

### Character defaults

Each character carries a default settings profile (stability/similarity/style/speed) in the casting table. Per-line tags modulate from that baseline — they do not replace it. Vex's baseline is slow and soft; Kaya's is fast. That difference lives in the casting table, not in every line's tags.

---

## 19.5 Generation Workflow

```
script file (SCRIPT_FORMAT)  →  dialogue-critic GO  →  voice-director
        → cache check (hash: voiceId + text + emotion + modelId)
        → batch generate (per scene / per trigger-set)
        → import as USoundWave → auto-assign to Dialogue DataAsset
        → append actual spend to phase0/VOICE_LEDGER.md
        → commit cache + manifest
```

**Non-negotiables** (these are already enforced in code per §16.12 — do not work around them):

- **Never generate the same line twice.** The hash key enforces it. If a batch shows unexpected credit spend, a text or tag changed — find out what before re-running.
- **The cache is committed to the repo** (`Eclipse/Content/Audio/Generated/` + `VoiceCacheManifest.json`) so no machine ever re-pays.
- **The API key is never in source or git** — env var `ELEVENLABS_API_KEY` or gitignored `Eclipse/Config/UserSecrets.ini`.
- **A changed line is a new line.** Fixing a typo after generation costs full price for that line. Proofread before, not after.

### The one-scene proof

Before Tier 1 bulk generation, `voice-director` runs the **full pipeline on a single scene** end to end — write → critic → generate → import → hear it in PIE. This costs perhaps 2,000 credits from the Tier 0 allocation and it validates every assumption in this document while it is still cheap to be wrong. See `phase0/SCRIPT_PRODUCTION_PLAN.md` §3.

---

## 19.6 Definition of Done — the sprint succeeded if

- [ ] Casting table complete, every row locked, owner-approved
- [ ] Commercial-rights confirmation recorded in §19.1
- [ ] Bark library live: 16 triggers × 3 faction vocabularies, playing in-game
- [ ] Act 1 fully voiced: prologue + M1.1–M1.8
- [ ] Adaptive music never-silent floor running (§16.7)
- [ ] `phase0/VOICE_LEDGER.md` reconciles to within 5% of 290,000 spent
- [ ] ≥20,000 credits unspent at month end **or** spent deliberately with the owner's say-so
- [ ] Every generated line traceable to a script file that passed `dialogue-critic`
- [ ] Acts 2–4 written and script-complete, awaiting credits only

---

## 19.7 Sources

- [ElevenLabs — Text-to-Speech best practices](https://elevenlabs.io/docs/overview/capabilities/text-to-speech/best-practices)
- [Audio tags 101: Directing emotional TTS in Eleven v3](https://elevenlabs.io/blog/v3-audiotags)
- [Eleven v3 audio tags: expressing emotional context](https://elevenlabs.io/blog/eleven-v3-audio-tags-expressing-emotional-context-in-speech)
- [Eleven v3 — character direction](https://elevenlabs.io/blog/eleven-v3-character-direction)
- [ElevenLabs — Text to Dialogue](https://elevenlabs.io/docs/overview/capabilities/text-to-dialogue)

---

*Back to [00_INDEX.md](00_INDEX.md).*
