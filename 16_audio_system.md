# PART 16 — AUDIO SYSTEM DESIGN
**ECLIPSE: RISE OF THE RESISTANCE**
*Game Design Bible — Document 16 of 17+ | Audio architecture, ElevenLabs pipeline, adaptive music, credit budget*

> **Numbering note:** this is the document authored as "Part 15 — Audio"; it is filed as **16** because `15_visual_quality_charter.md` already holds Part 15. Cinematics are Part **17**.
>
> **Phase pacing (graybox rule):** Phase 1 ships **text barks only** (SPEC-P1-00: "no VO/audio beyond debug barks (text)"). This document is **forward infrastructure** — the audio module can exist and the pipeline can be built, but real VO/music/SFX authoring begins at **Phase 2** and is validated against the performance budgets (12.4). Nothing here overrides the current ACTIVE_MILESTONE.

---

# 16.1 Audio Philosophy

Audio in ECLIPSE is not decoration. Sound is a gameplay system that communicates **danger, emotion, faction identity, world state, player consequences, and the scale of the rebellion.** The player should understand the galaxy through sound.

- A Dominion-controlled city must *feel* surveilled, oppressed, militarised.
- A liberated settlement must *sound* like hope, rebuilding, freedom.

The system reacts dynamically to: CampaignState · faction control · mission state · player reputation · squad morale · combat intensity · planet identity.

**The never-silent rule (§16.7 expands it):** the game is *never* fully silent during play. There is always at least an atmosphere bed and the recurring main-theme motif. Silence is only ever a deliberate dramatic stinger, never an empty state.

---

# 16.2 Audio Architecture Overview

Five connected systems, all communicating **through the Event Bus** — no gameplay system controls audio directly (GDD 12.2 rule 2, 14.3.1):

| System | Purpose | Unreal tech |
|---|---|---|
| Dialogue System | Character voices & conversations | MetaSounds + Dialogue Runtime + **ElevenLabs** |
| Music System | Adaptive cinematic soundtrack | Quartz + Audio Director |
| SFX System | Weapons, vehicles, environment | MetaSounds |
| World Audio System | Living environments | Audio Volumes + DataAssets |
| AI Audio Director | Global dynamic audio decisions | Custom C++ subsystem |

```
Weapon Fired Event  ──►  AudioSubsystem  ──►  select + play weapon sound
```
Never `Weapon.cpp → PlayExplosionSound()`. The seam is the bus.

---

# 16.3 Dialogue System

Characters must feel alive, memorable, reactive. Every important character has a unique voice, speaking style, emotional range, relationship state, and history with the player.

**`UEclipseCharacterVoiceData`** (DataAsset) — one per speaking character:
- Character name / id
- **ElevenLabs voice id** (§16.12)
- Voice profile, personality, speech pattern, emotion range, faction
- Relationship variables, preferred vocabulary

Example — *Grand Marshal Sera Kaine*: deep, controlled, military; short sentences, no emotional words; low anger, high confidence, high intimidation.

---

# 16.4 Voice Pipeline (dev vs. production)

**Development phase — AI voices for rapid iteration** (this is what we build now):
```
Dialogue writer → Dialogue database → ElevenLabs generation → Unreal import → Runtime dialogue system
```
Requirements: consistent character identity, emotional control, pronunciation control, multiple languages.

**Production phase — final story-critical dialogue is professionally recorded**, processed, mixed, localized. AI voices remain for ambient NPC dialogue, procedural missions, temporary content, and dynamic reactions. *(This mirrors the roadmap's "AI acceleration is upside, not plan-of-record" — 13.2.)*

---

# 16.5 Dynamic Dialogue Generation

Dialogue reacts to CampaignState. Same event, different emotional context:

- City liberated, low losses → *"The rebels came."*
- City liberated, heavy civilian losses → *"The rebels came... but many did not survive."*

Inputs: faction reputation · planet control · previous choices · companion relationship · mission outcome · squad losses · player actions.

---

# 16.6 Companion Dialogue System

Companions (Mara, Dex, Reyes, Torren, Kaya, Whisper, Sela, Brick — see `00_INDEX`) have **personality** (brave/cautious/idealistic/pragmatic/angry/loyal) and **memory** tied to player choices, mission outcomes, and key events. Memory drives combat comments, conversations, loyalty missions, and endings.

- Lost their squad, in combat: *"Not again..."*
- Later victory: *"At least this time we brought everyone home."*

---

# 16.7 Adaptive Music System — the always-on theme (RESEARCH)

**Design goal (per direction): the game always has music. A main theme song is always present, but it continuously changes with the game state.** This is standard modern interactive-music practice; the two proven techniques, used together:

### A. Vertical layering (re-orchestration) — *the theme is always there, its intensity changes*
One piece authored as **stems/layers** that fade in and out on the same timeline, so the music never stops — it thickens or thins:
```
Layer 1  Atmosphere / drone      ← always on (never-silent floor)
Layer 2  Main-theme motif        ← the "theme song", always audible in some form
Layer 3  Melody / harmony
Layer 4  Percussion              ← fades in on threat
Layer 5  Combat brass/strings    ← fades in on engagement
Layer 6  Choir / epic finale     ← story/boss peaks
```
Exploration = layers 1–2. Enemy detected = +3/4. Full battle = +5. Story climax = +6. The Audio Director drives each layer's volume from state; transitions are smooth, not hard cuts.

### B. Horizontal re-sequencing (transitions) — *switching sections on the beat*
Moving between musical sections/tracks **quantised to musical boundaries** (bars/beats) using **UE Quartz**, so a change from "occupied tension" to "liberation" lands on a downbeat, never mid-phrase.

### C. Leitmotif discipline — *why it still feels like one theme*
The **main-theme motif** (a short melodic signature) recurs across every planet/faction track — reharmonised, reorchestrated, tempo-shifted — so however the music morphs, the player always recognises "the ECLIPSE theme". This is what makes "a theme that always plays but changes" cohere instead of feeling like a playlist.

### The never-silent contract
The Audio Director guarantees at minimum **Layer 1 (atmosphere) + Layer 2 (theme motif)** during play. True silence is only ever an authored **stinger** (a held beat before an execution, a reveal, a death) — deliberate, timed, and immediately resolved back into the bed.

Music state inputs: threat level · mission phase · planet identity · faction control · player reputation · combat intensity · story moments.

---

# 16.8 Faction Audio Identity

Each major faction has a distinct audio language:
- **The Dominion** — mechanical, industrial, controlled: heavy percussion, synthetic textures, military signals.
- **The Resistance / Eclipse** — human, emotional, improvised: strings, organic instruments, imperfect rhythms.
- **Ancient / Vantara relics** — mysterious, atmospheric, alien: unusual instruments and harmonics.

---

# 16.9 Combat Audio System

Combat audio *is information*: the player must read enemy position, weapon type, danger level, and battlefield changes by ear. Every weapon carries: fire · reload · impact · distance variation · material interaction · suppression. A shot into metal, wood, or alien material sounds different (ties to the hitscan material stub, SPEC-P1-05).

---

# 16.10 Environment Audio System

**`UEclipsePlanetAudioProfile`** (DataAsset) per location: biome · weather · ambient · wildlife · industrial · faction presence.

- **Kessara, day:** factories, machinery, Dominion announcements.
- **Kessara, night:** patrol drones, distant alarms, rebel activity.

---

# 16.11 AI Audio Director

**`UEclipseAudioDirectorSubsystem`** — global audio decisions. Inputs: CampaignState · mission state · player state · world state. Outputs: music selection/layer mix · ambient audio · dialogue priority (ducking) · combat intensity · cinematic audio.

Example — *occupied planet + high threat + legendary-rebel reputation* → more propaganda broadcasts, more civilian whispers, more intense score.

---

# 16.12 ElevenLabs Integration (our provider) — TTS, Music, Sound Effects

We use **ElevenLabs** for three generation modalities, all now available on the account: **Text-to-Speech, Music, and Sound Effects.** How each is used:

| Modality | Use in ECLIPSE | Cadence |
|---|---|---|
| **Text-to-Speech** | Dev/placeholder VO for all lines; permanent AI voices for ambient NPCs, procedural missions, dynamic barks | Bulk at dev time, cached |
| **Music** | Generate the **main theme** + per-planet/faction **stems & loops** used as the adaptive layers (§16.7); generated once, stored as assets, mixed at runtime | Rare, high-value |
| **Sound Effects** | One-shot SFX sets (weapons, impacts, UI, ambient beds) where a bespoke recording isn't yet available | As needed, cached |

### Runtime + tooling shape (matches the implementation task)
- **`UDialogueVoiceSubsystem`** (runtime, GameInstance) — resolves and plays a character's line; on a cache hit it plays the stored asset, never re-requests.
- **`UEclipseVoiceGenerator`** (editor tool / commandlet) — bulk-generates from the dialogue database: for each line it computes a **cache key = hash(voiceId + text + emotion + modelId)**, and:
  1. if an asset exists for that key → skip (never generate the same line twice);
  2. else → call ElevenLabs, save the audio locally, import as `USoundWave`, and **auto-assign it to the Dialogue DataAsset**.
- **Emotion parameters** map to ElevenLabs voice settings (stability / similarity / style / speed) per line.
- **Caching** is by content hash, so identical text for the same voice+emotion is only ever paid for once — the hard rule *never generate the same voice line twice* is enforced by the key, not by discipline.

### Security & config (CRITICAL — GDD 14.2 / security.md)
- The API key is **never** in source or git. The subsystem reads it from the environment variable **`ELEVENLABS_API_KEY`**, or from a **gitignored** `Eclipse/Config/UserSecrets.ini` (added to `.gitignore`).
- The key shared during setup should be **rotated** in the ElevenLabs dashboard, since a key transmitted in plaintext must be treated as exposed.
- Generated audio + a `VoiceCacheManifest.json` live under `Eclipse/Content/Audio/Generated/` (committed as assets) so the cache travels with the repo and no machine re-pays for existing lines.

---

# 16.13 Credit Budget — is 121,000 credits/month enough? (RESEARCH)

**The number.** For ElevenLabs Text-to-Speech, **≈ 1 credit = 1 character** of generated text. So **121,000 credits/month ≈ 121,000 characters ≈ ~20,000 words of speech per month** (~6 chars/word incl. spaces). Music and SFX generations **draw from the same monthly pool**, so real TTS headroom is lower once we generate tracks/effects.

**Is it enough for the whole game?** Honestly: **not in one month for full final VO.** A story-driven RPG of this scope carries on the order of **hundreds of thousands of words** of dialogue; ~20k words/month would take many months to voice once, before iteration. **But we do not need to voice the whole game with ElevenLabs**, and two properties make the budget very workable:

1. **Caching / never-regenerate (§16.12):** every line is paid for **once, ever**. Re-runs, other machines, and re-imports cost 0 credits. Iteration cost is only *new or changed* lines.
2. **Scope discipline (matches 16.4 + the roadmap):** final story-critical VO is **pro-recorded**, not AI-generated. ElevenLabs covers **dev placeholders + permanent ambient/procedural/bark VO**, which is a fraction of the word count.

**Recommended monthly allocation of the 121k pool** (tune as we learn real costs):
- ~**85k** credits → dialogue TTS (dev placeholders + ambient/bark VO), cached.
- ~**25k** credits → Music generations (theme + a few planet/faction stem sets) — front-loaded, then near-zero once the adaptive library exists.
- ~**11k** credits → Sound-effect one-shots.

**Spreading over two months (or more):** yes — for any large batch (e.g. "generate all of Act 1's placeholder VO"), **plan it across months.** Concretely:
- **Month 1:** the adaptive **music library** (theme + core layers) + the **prototype/vertical-slice** dialogue (the loop the tester actually hears) + core weapon/UI SFX.
- **Month 2+:** the next act's placeholder dialogue and planet ambiences, as content lands — pacing generation to the roadmap so we never generate content for phases we haven't built.

**Bottom line:** 121k/month is **more than enough for the prototype and for steady, cached, phase-paced development**; it is **not** a one-shot "voice the entire final game" budget, and it shouldn't be — final VO is recorded. Split large batches across months and let the cache do the rest.

---

# 16.14 Unreal Engine Implementation

- **MetaSounds** — weapons, vehicles, dynamic effects.
- **Quartz** — beat-synchronised music transitions & layer changes (§16.7).
- **Audio Modulation** — distance, environment, emotional filters.
- **DataAssets** — all audio data editable without code (14.2).
- **HTTP + Json modules** — ElevenLabs requests (added to the audio module's `Build.cs`).

```
/Content/Audio
  /Music        (theme + adaptive layers/stems)
  /Weapons  /Vehicles  /Environment
  /Dialogue     (per-character folders)
  /Factions  /MetaSounds
  /Generated    (ElevenLabs output + VoiceCacheManifest.json)

/Source/EclipseAudio        (runtime module)
  UDialogueVoiceSubsystem  UEclipseAudioDirectorSubsystem  MusicManager
  ElevenLabsClient
/Source/EclipseAudioEditor  (editor module)
  UEclipseVoiceGenerator (bulk tool)
```

---

# 16.15 Audio Development Rules

1. No critical gameplay information depends on UI alone — audio must communicate world state.
2. Every faction needs a unique sound identity.
3. Dynamic systems use DataAssets, never hardcoded values (14.2).
4. Important characters keep a consistent voice (fixed ElevenLabs voice id).
5. Procedural dialogue must respect personality and lore (`00_INDEX` glossary).
6. Audio is tested like graphics (14.4 extends to listening passes).
7. **Never generate the same voice line twice** (cache key enforces it).
8. The game is **never silent** during play (§16.7 never-silent contract).
9. The API key never touches source or git.

---

# 16.16 Definition of Done

The audio system is complete when: ✅ every major faction has a unique sound identity · ✅ characters have consistent voices · ✅ dialogue reacts to player actions and CampaignState · ✅ music is always present and changes dynamically (never-silent theme) · ✅ combat sounds communicate gameplay · ✅ every planet has a unique atmosphere · ✅ generation is fully cached (no duplicate API spend) · ✅ the key is out of git · ✅ performance stays within the 12.4 budget.

---

*Prev: [15_visual_quality_charter.md](15_visual_quality_charter.md) · Next: [17_cinematic_animation_system.md](17_cinematic_animation_system.md).*
