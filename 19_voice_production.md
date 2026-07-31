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
3. **Commercial rights — CONFIRMED 2026-07-31, this blocker is closed.** The owner checked and reported: the subscription carries commercial rights, *and* whether an individual Voice Library voice carries them is visible on the voice card. Both halves matter, because they gate different things: the plan gates generating at all, the voice card gates which voices may be cast. The standing policy that follows is in §19.3 — `voice-director` rejects any library voice without a commercial licence during stage 1, before it can ever reach a paid stage-2 test.

**Backlog #29 in `phase0/EXECUTION_PLAN.md` ("VO-opname + implementatie", Phase 5) is hereby narrowed** to "VO quality pass on AI voices" — no recording contracts.

---

## 19.2 The Budget — 131,000 credits, one month, no rollover

**The situation.** The account holds **131,000 credits**. They **expire on 21 August 2026** (owner-confirmed 2026-07-31) and they do not roll over. A €11 top-up month is possible but the owner would rather not. So: spend it all, spend it on the right things, and spend nothing twice.

**That is 21 days.** The working deadline is **19 August**, not the 21st — two days of buffer, because a failed batch on the last evening cannot be re-run.

**The arithmetic.** ≈1 credit = 1 character. ~6 characters per word including spaces.

- 131,000 credits ≈ **~21,800 spoken words**
- The full spoken script is **~132,000 words** (§18.1) ≈ ~790,000 credits

> **Corrected 2026-07-31 (late).** This section said 310,000 for most of the day; the owner had misremembered his balance. The real figure is **131,000** — 42% of what every plan on this page was built on. Every tier below has been re-cut, not just re-labelled.

**So roughly 17% of the game's speech fits in this month**, not the 40% the earlier draft claimed. That is not a failure — it is the correct amount, because the other 60% is dialogue for acts whose beats are not yet locked, and generating those now would mean paying for lines that change.

### The credit ladder — spend in this order

Ordered by *lowest rewrite risk × highest perceived value*. Do not start a tier until the one above it is complete.

| Tier | What | Credits | ≈ Words | Why here |
|---|---|---|---|---|
| **0** | **Casting** — stage 1 free (library browsing), stage 2 deep test on the Act-1-critical roles only | 6,000 | 1,000 | Must happen first. Getting casting wrong later re-costs every line that character has. Villains (Vex, Kaine, Threx) appear in Act 3–4 and are cast later. |
| **1** | **Bark & systemic library** — the 16 priority triggers × 3 faction vocabularies | 48,000 | 8,000 | Still the safest spend on the board. The systems already exist, the lines never change, and this is the biggest "the world is alive" gain per credit. |
| **2** | **Act 1 story dialogue** — Prologue + M1.1–M1.8 | 45,000 | 7,500 | Beats are locked. Low rewrite risk. Includes the calibration mission. |
| **3** | **Adaptive music** — main theme + the never-silent floor only | 15,000 | — | Cut from the full stem library to the theme and one layer set. Front-loaded once, then near-zero forever. |
| **4** | **Core SFX one-shots** — weapons, impacts, UI | 7,000 | — | Cheap, high polish return. |
| **—** | **Reserve** — retakes, mistakes, a mis-tagged batch | 10,000 | — | Insurance *during* the sprint — see the rule below. |
| | **Total** | **131,000** | | |

> **What fell off the ladder at 131k:** the Act 1 hub & companion conversations (was Tier 4, 33k). Those are the Hollow Point scenes with Mara, Dex, Brick and Reyes — the voices the player lives with between missions. They stay *written*; they wait for next month's credits. If that is the wrong cut, the owner says so — see **O-10**.

### The reserve rule — owner-corrected 2026-07-31

The owner is right: **all 131,000 get spent.** Credits that are unused on 21 August are simply thrown away — expiring credits have no value at expiry, so "stop at 290,000" would mean burning 20,000 for nothing.

But the reserve is not savings, it is **insurance during the sprint**. If the full 131k is allocated to tiers on day one and a batch turns out wrong on day twelve, there is nothing left to fix it with. So:

1. **Keep 20,000 unallocated** — do not plan it into a tier.
2. **On 17 August**, release whatever is left of it into the next tier in the ladder.
3. **Target on 19 August: 131,000 spent, 0 remaining.**

That gives insurance for the risky middle of the sprint *and* zero waste at the end. Strictly better than both alternatives.

**Hard rules for the sprint:**

- **Nothing is generated that has not passed `dialogue-critic` with GO.** No exceptions, no "we'll check it after". A failed line that gets generated is money burned permanently.
- **Batch, never drip.** Generate per scene or per trigger-set, never line by line — batching is where mistakes get caught before they multiply.
- **Log every batch.** `voice-director` appends actual credit spend to `phase0/VOICE_LEDGER.md` after each run. The ladder above is a plan; the ledger is the truth.
- **Do not allocate the last 20,000 to a tier before 17 August** — it is insurance, not savings. On 17 August, release whatever remains of it into the next tier. Target on 19 August is **131,000 spent, 0 remaining** (see the reserve rule above). An earlier draft of this line said "stop at 290,000", which contradicted that rule and would have thrown away 20,000 credits at expiry.

**What happens to Acts 2–4?** They get *written* this month (writing costs nothing) and sit as text with complete script files. The moment credits exist again, they generate with one command and zero further authoring. That is the whole point of the layer discipline: the expensive-to-change work is done, and the cheap-to-defer work is deferred.

### The free tier cannot be used for this game

ElevenLabs' free plan gives ~10,000 credits/month, and the owner asked whether those can be added to the pool. **They cannot, for two reasons that both matter:**

1. **No commercial rights.** The free plan grants no commercial usage and requires attributing ElevenLabs in public content. Audio shipped inside a game that is sold would violate that.
2. **API access is not a production path** on the free tier — and this whole pipeline is API-driven and cached.

**The cheap fix, and it is genuinely cheap:** commercial rights start at the **Starter** plan, roughly **€5/month for 30,000 credits**. So after 21 August, Acts 2–4 can be generated at ~30k credits/month for the price of a sandwich, indefinitely, with the cache making sure nothing is ever paid for twice. That is the plan of record for the remaining ~60% of the script — not a €11 top-up, and certainly not the free tier.

*Verify against the current pricing page before committing; plan names and quotas change.*

---

## 19.3 Casting

**Casting is locked before Tier 1 and never changed.** Re-casting a character re-costs every line they have.

### Protocol — two stages, because casting is permanent

Owner instruction (2026-07-31): *"doe ook moeite om de juiste voice te gebruiken."* An earlier draft of this section called for two candidates and one line. That was the shortest path, and `21_quality_mandate.md` forbids it. Casting is the one decision that cannot be undone cheaply — a re-cast invalidates every line that character has ever spoken. So it gets the effort it deserves.

**Stage 1 — wide screen. COSTS ZERO CREDITS.**
Owner instruction (2026-07-31): *"om een stem te kiezen moet je niet eerst credits aan spenderen — kijk gewoon naar de eigenschappen van de stem."* Correct, and an earlier draft of this section wasted budget on it.

The Voice Library is browsable for free: every voice carries **metadata** (age, gender, accent, register, descriptive tags, use case) **and a free preview sample**. Generating costs credits; browsing and listening does not.

So: shortlist **6–8 candidates** per companion/villain and 3–4 per bark register **entirely from library metadata and free previews**. Match on age, register, accent-neutrality — and check the **commercial licence on the voice card** while you are there, so a non-commercial voice never reaches stage 2. Cut to **two finalists** per role. **Zero credits spent.**

> **Stage 1 is DONE — 2026-07-31, 0 credits.** 18 roles, 80 candidate slots, in
> `progress_media/casting/` — open **`CASTING.html`** to listen, grouped per character.
> Machine-readable: `casting_stage1.json`. Nothing was generated; these are the voices'
> own free preview samples.
>
> **The pool was deliberately limited to ElevenLabs' 21 premade voices.** They are
> ElevenLabs-owned (`sharing: null`), so they carry no third-party terms and are safe
> under any outcome of O-2. Voice Library voices were *not* shortlisted, because the
> licence check cannot be automated (see the Licence row below) and O-2 is not settled
> in a source this agent can point at.
>
> **Three gaps the premade pool left, all now closed by the library pass below:**
> 1. **No older female voice exists.** Mara is 52; the oldest premade female reads well
>    under 45. Kaine (49) had the same problem.
> 2. **Exactly one `old` voice in total** (Bill) — top candidate for Vex (68) *and* for
>    older readings of Torren and Callis, and it can only be cast once.
> 3. **20 of the 21 voices appear on more than one shortlist** (Alice, Daniel and Matilda
>    on six each). 21 voices cannot fill 18 roles with independent shortlists.

> **Stage 1 extension — Voice Library, 2026-07-31, also 0 credits.** O-2 confirmed, so
> the library opened. **24 extra candidates** for exactly the five roles above:
> Mara (6), Kaine (5), Vex (5), Callis (4), Torren (4). Total now **104 candidates**.
>
> **Two gates were applied automatically; one cannot be.**
> - **Automated:** creator status `professional`/`high_quality`, accent-neutral, and a
>   **withdrawal notice period ≥ 365 days** — a creator who can pull their voice next
>   week is unusable for a game that ships for years. This rejected real candidates:
>   a strong Kaine voice had `notice_period: 0`.
> - **Not automatable — the commercial licence.** Measured, not assumed:
>   `/v1/shared-voices` exposes `free_users_allowed`, `rate`, `fiat_rate` and
>   `notice_period`, but **no licence field**, and a `commercial=true` filter returns
>   byte-identical results to a nonsense parameter — unknown params are ignored.
>   So every library candidate carries `licentie.status = NIET_GEVERIFIEERD` plus a
>   direct card URL, and **the card must be opened and read before that voice enters
>   stage 2.** Premade voices need none of this: they are ElevenLabs' own.

**Stage 2 — deep test on the finalists only (~12k credits). FROZEN until two things are true.**
It does not start until (a) **O-2 is answered** and (b) **the owner has picked a top 2 per role**
from stage 1. Running it earlier means paying to audition voices that may not be the finalists —
the exact waste stage 1 exists to prevent.
This is where credits are unavoidable, and why they are worth it: the library preview is *generic content*. It cannot tell you whether this voice survives Mara's death scene, whether it holds up at 5 words over gunfire, or whether it responds to the audio tags this character needs. Only your own lines answer that.
Each finalist speaks **three** lines, not one:

1. **The signature line** — does the fingerprint (§18.4) survive in this voice?
2. **An emotional extreme the character actually reaches** — Mara's last scene, Vex threatening, Kaya panicking, Brick grieving. A voice that carries a calm line beautifully and collapses at the extreme is the wrong voice, no matter how good it sounded first.
3. **A short combat/systemic line** — because the same voice must also work at 5 words over gunfire.

**Then five checks before anything is locked:**

| Check | Why it kills a candidate |
|---|---|
| **Licence** | Voice Library voices carry their own terms. Non-commercial or attribution-required = rejected, however good it sounds. **This check is manual, in the web app.** Measured 2026-07-31: `/v1/shared-voices` returns `free_users_allowed`, `rate` and `notice_period` but **no commercial-licence field** — the API cannot answer this question, so it cannot be automated. For the licence status of the plan itself see O-2 in `phase0/SCRIPT_PRODUCTION_PLAN.md`; do not restate it here as fact. |
| **Tag range** (§19.4) | Does the voice support the tags this character needs? A voice cast for shouting will not whisper. If the character needs both extremes, that decides the casting. |
| **In context, not in silence** | Audition over real game audio, not in a quiet browser tab. A voice that is gorgeous in silence can vanish in a firefight. |
| **Pairing** | Audition characters who share scenes **together**, never separately. Mara + Reyes, Dex + Kaya, Vex + Kaine. If two voices blur when heard back to back, one of them is recast now — not after 90k credits. |
| **Written justification** | One sentence per role on why this voice won. Prevents the decision being re-litigated in three weeks. |

**Cast in order of line count, not story order:** Voss, Mara, Dex, Brick and the Eclipse bark register carry Act 1 and get the most attention. Vex and Kaine appear in Act 3–4 and can be cast later if time runs short.

**The owner picks.** This is taste, not technique, and he has to live with it for a year.

### Signature lines
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
Once the owner has chosen, the `voiceId` goes into the casting table below **and into the dialogue DataAsset**, and is treated as canon. Locked means locked.

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
- [ ] `phase0/VOICE_LEDGER.md` reconciles to within 5% of **131,000** spent
- [ ] The 20,000 reserve stayed unallocated until 17 August, then went into the next tier — **0 credits left unused at expiry**
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
