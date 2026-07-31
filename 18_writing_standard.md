# PART 18 — WRITING STANDARD
**ECLIPSE: RISE OF THE RESISTANCE**
*Game Design Bible — Document 18 of 19 | Dialogue craft, character voice, bark design, the anti-slop gate*

---

> **Purpose.** This document exists because ECLIPSE's script will be written by many agents working in parallel. Without a shared standard, forty writers produce forty voices and the game sounds like nobody. This is the single source of truth for **how a line is written**. `dialogue-writer` writes against it; `dialogue-critic` scores against it; no line ships that fails §18.9.
>
> **Authority.** Subordinate to `02_story_bible.md` (what happens) and `11_missions.md` (mission structure). This document governs *how it is said*, never *what happens*. A writer who needs to change an event escalates to `story-architect` — they never quietly rewrite canon.

---

## 18.1 Scale Reality — what we are actually committing to

Real shipped scripts, for calibration:

| Game | Script size | Note |
|---|---|---|
| The Witcher 3 | ~450,000 words | ~100 h; recording took 2.5 years |
| Dragon Age: Origins | ~740,000 words / 68,260 lines | Text-heavy RPG, huge branching |
| Mass Effect 3 | ~40,000 lines | Tighter, more cinematic |

ECLIPSE is 160+ hours, but **most of those hours are systemic** — the strategy layer, generated operations, base management. Authored dialogue concentrates in the 42 hand-authored missions (34 story + 8 loyalty), the hub conversations, and the bark library.

**Working target:**

| Layer | Words | Voiced? |
|---|---|---|
| 34 story missions × ~1,600 w | ~55,000 | Yes |
| 8 loyalty missions × ~1,800 w | ~14,000 | Yes |
| Hub / companion conversations (4 acts) | ~45,000 | Yes |
| Bark & systemic VO library (unique lines) | ~18,000 | Yes |
| Codex, letters, propaganda, terminals | ~30,000 | **No — text only** |
| **Total authored** | **~162,000** | **~132,000 spoken** |

That is one third of a Witcher 3 and it is achievable — *if* every line is written once, well. Rewriting is the only thing that can kill this schedule, which is why §18.9 exists.

**Codex and terminal text is never voiced.** It is the pressure-release valve: lore that wants to be long goes there, so dialogue can stay short.

---

## 18.2 The Five Laws

Every line obeys all five. A line that breaks one is not "stylistic" — it is broken.

1. **A line does at least two jobs.** It carries information *and* character, or emotion *and* pressure. A line that only conveys facts belongs in the codex, not in a mouth.
2. **Nobody says what they mean.** Characters deflect, understate, change the subject, answer a question with a question. Direct emotional statement is reserved — perhaps thirty times in the whole game — and lands hard *because* it is rationed.
3. **People talk unequally.** Real conversation is lopsided. One character takes four words, the next takes twenty-six. Symmetry is the loudest signal that a machine wrote it.
4. **The player must never be told what they feel.** No "That was intense." No "We did it, but at what cost?" The game shows the cost; the characters live in it.
5. **It must be sayable in one breath.** Read it aloud. If you need a second breath, split it into two lines or cut it. This is a hard TTS constraint, not a preference (§19).

---

## 18.3 Line Length — by delivery context

Nathan's note: *not too short, not too long.* Here is what that means concretely. Length is set by **where the player hears it**, never by the writer's mood.

| Context | Words | Why |
|---|---|---|
| **Combat bark** (heard under gunfire) | **3–8** | Competing with SFX. Anything longer is not heard. |
| **Callout / order response** | 2–6 | Must land inside the reaction window. |
| **Ambient bark** (patrol, idle, base chatter) | 6–16 | Player may walk away mid-line. |
| **In-mission radio / walk-and-talk** | 10–25 | Player is moving; keep one idea per line. |
| **Hub conversation turn** | 12–35 | Player is stationary and chose to listen. |
| **Cutscene / set-piece beat** | 20–60 | Camera is holding. Still broken into breath-sized lines. |
| **Speech / oration** (Sela, Vex, finale) | up to 120 | Rare. Maximum four in the entire game. |

**The variance rule:** inside any single conversation, line lengths must vary by at least a factor of three. If every turn in a scene is 18–22 words, the scene is rewritten. This one rule does more for authenticity than any other.

> **Short per line, generous in count** (`21_quality_mandate.md`). These bands cap the length of *one line*, never the size of the work. The owner's standing instruction is explicit: *rather three dialogues of twenty lines than one of two.* So write the extra scene, write twelve bark variants instead of six, write the companion reaction the plot doesn't strictly require. Brevity is a delivery constraint; it is never a reason to write less.

---

## 18.4 Character Voice Fingerprints

This is the spine of parallel writing. Each character has a **syntax fingerprint** (mechanical, checkable), a **tic**, and a **never**. A writing agent that knows only these three things still produces a recognizable character.

### Companions

| Character | Syntax fingerprint | Tic | Never |
|---|---|---|---|
| **Mara Sovann** (52, mentor) | Short declaratives. Plural first person. | Says "we" where anyone else would say "I" | Never sarcastic. Never says "I" about the cause. |
| **Dex Callum** (31, engineer) | Fragments. Technical nouns used as insults. | Deflects any emotional beat with a hardware joke | Never states a feeling directly. Never finishes a sentimental sentence. |
| **Dr. Elin Reyes** (38, medic) | Complete, clinical sentences. **Drops contractions when stressed** — training reasserts. | Asks a question instead of accusing | Never swears. Never guesses out loud. |
| **Torren Vale** (45, ex-colonel) | Military economy. Issues orders phrased as suggestions. | Longest silences in the game — his beats are marked `[pause]` | Never raises his voice. Never uses a callsign he hasn't earned. |
| **Kaya Renn** (27, pilot) | Run-ons. Interrupts herself. Slang, lane-runner jargon. | Starts talking before the other person finishes | Never finishes a serious sentence without undercutting it. |
| **Whisper** (voice-only until Act 3) | No contractions. Conditionals. Passive constructions. | Says "it is known" instead of "I know"; never uses names | Never says "I". Never confirms; only implies. |
| **Sela Vann** (23, organizer) | Rhetorical structure even in private. Second person plural. | Reframes a personal question as a collective one | Never speaks about herself for more than one line. |
| **Brick** (34, heavy) | Fewest words of anyone. Often a single noun. | Answers difficult questions by naming a dead soldier | Never lies. Never speaks first in a group. |

### Recurring non-companion voices

Not squad members, but they speak across acts and therefore need fingerprints. Added by `story-architect` 2026-07-31 (ruling L1-R9); reasoning in `phase0/beats/RULINGS_L1.md`.

| Character | Syntax fingerprint | Tic | Never |
|---|---|---|---|
| **Petra Voss** (55, the aunt) | **Imperatives with the pronoun elided.** Household and kitchen nouns where everyone around her uses operational ones. Second-shortest lines in the game. | Answers a question she will not answer by assigning a task | **Never says "you" or "your".** Never accepts thanks. Never names what was done to her. |

**Petra vs. Brick — the two laconic characters must not blur.** Both are terse, so §18.9 C1's strip test needs a mechanical difference: **Brick hands you a noun, Petra hands you a verb.** He answers with a name; she answers with a job. Two short lines side by side should be reassignable on that alone.

**Petra vs. Mara — deliberately separated, and this one is load-bearing.** Mara's arc turns on a pronoun: she says "we" for the entire game and spends "you" exactly once, in `M1.8.S08`, and that word *is* the transfer of command (`ACT1_OVERVIEW.md` AR-5/AR-5b). Petra speaks in the same act climax, four scenes earlier. If she were also working in second person, that ladder would collapse.

She is not, because **her imperatives elide the pronoun**: "Sit down." contains no "you". So the word is scarce across the whole climax, and Mara's single use of it lands in a vacuum. This is checkable with a regex, which is the point — it is a designed separation, not a lucky one.

**Her one reserved break:** exactly once in the campaign, Petra **asks instead of instructs**. Not before Act 3. Unspent as of Act 1.

| Character | Syntax fingerprint | Tic | Never |
|---|---|---|---|
| **Malachar Vex** (68) | Longest sentences in the game. Subordinate clauses. Soft register. | Quotes pre-Dominion casualty statistics as comfort | Never raises his voice. Never says "I" — uses "one". |
| **Sera Kaine** (49) | Precise, honest, no euphemism. Says exactly what she will do. | States her intention before acting on it | Never lies. Never uses Veil language. |
| **Dahl Threx** (41) | Warm, intimate, question-heavy. Uses first names constantly. | Compliments you sincerely while hurting you | Never shouts. Never uses rank when a first name will wound more. |
| **Oren Callis** (56) | Passive voice. Bureaucratic hedging. Conditionals stacked. | Attributes every decision to a policy, never to himself | Never commits to anything in one sentence. |
| **AEGIS** | Present tense. No self-reference pronouns. No persuasion — only statement. | States probability where a human would state opinion | Never argues. Never expresses preference. |

**The Threx/Kaine contrast is load-bearing.** Threx is warm and unbearable; Kaine is cold and fair. If a reader can't tell them apart with the name stripped, both scenes are rewritten.

### The protagonist (Voss / "Cinder")

Voss is player-defined on two axes (Idealist↔Pragmatist, Personal↔Strategic). **Every Voss line ships in the variants the scene needs**, tagged per §`SCRIPT_FORMAT`. Voss's fixed traits regardless of axis: technician's vocabulary (sees systems, names mechanisms), and never claims credit in front of the people who did the work.

---

## 18.5 Bark Design

Barks are systemic dialogue: fired by triggers, heard hundreds of times, and the single biggest driver of whether the world feels alive or cheap. They are also the **safest credit spend** (§19) because they almost never change.

**Rules:**

1. **6–12 variants per trigger.** Below six, the player hears the repeat inside one firefight. Above twelve, cost outruns benefit.
2. **Three to eight words.** Non-negotiable under combat audio.
3. **A good bark carries two or three facts at once.** Weak: "He's over there!" Strong: "He's in the vents — I've lost him!" — that conveys location, that they lost you, *and* that they're rattled. Write for compression.
4. **No redundancy across a trigger set.** Twelve ways to say "reloading" is waste; twelve *states* around reloading is content.
5. **Barks carry faction identity.** Dominion conscripts sound frightened and procedural; Veil operatives sound calm and clinical; Eclipse fighters sound improvised and personal. Same trigger, three vocabularies.
6. **Squad barks use names.** "Reyes is down" beats "Man down" every time — it is the whole PEOPLE-NOT-UNITS pillar in one word. Names come from the live roster, so barks are written with a `{name}` slot and voiced per-name where the roster is fixed.

**Priority trigger set for the first sprint** (these exist in systems already built): contact, taking fire, reloading, out of ammo, grenade, flanking, enemy down, squadmate down, squadmate revived, order acknowledged, order refused, stealth-broken, target lost, low health, objective complete, extraction called.

---

## 18.6 Humour

Nathan asked for funny. Funny is a *character property*, not a joke quota.

- **Comedy carriers:** Dex (deflection), Kaya (velocity), Brick (understatement — the funniest character precisely because he tries least).
- **Comedy is never authorial.** No character makes a joke the character wouldn't make. No winking at the player. No pop-culture reference — there is no pop culture in the Vantara Expanse.
- **Ratio:** roughly one comic beat per six lines in downtime/base scenes; **zero** in aftermath, death, and Act 4 scenes. The game earns its darkness by having been funny earlier.
- **The best jokes cost something.** Dex jokes hardest when he's most frightened. That is the joke *and* the characterisation.

---

## 18.7 Scene Construction

Every authored scene has: **a want, an obstacle, and a turn.** If you cannot name all three in one sentence each, the scene is not ready to write.

- **Enter late, leave early.** Start after the greeting; end before the goodbye. Cut the first line and the last line of every draft scene — 80% of the time the scene improves.
- **One idea per line.** If a line has two ideas joined by "and" or a comma splice, it is two lines, possibly for two characters.
- **Interruption is written explicitly.** Use `--` at the end of an interrupted line. At least one interruption per multi-character scene.
- **Silence is written.** `[pause]` and `[beat]` are content. Torren's silences are characterisation.
- **The player is a participant, not an audience.** In any scene longer than six turns, the player gets an input — a choice, an interjection, or a movement beat.

---

## 18.8 What the player is allowed to not understand

Confidence in a script comes from restraint. The Vantara Expanse has six centuries of history and the player should feel every one of them *without a lecture*.

- Characters use jargon correctly and do not define it. "Lane's spiked past Kessara Gate" needs no gloss — context and repetition teach it.
- Named events (the Tithe of Hands, the Silent Coup) are referenced casually long before they are explained.
- **The codex explains; dialogue never does.** If a line exists to explain a term, delete it and write a codex entry.

---

## 18.9 THE ANTI-SLOP GATE

This section is the quality bar. `dialogue-critic` runs it on every scene. **Any single failure = NO-GO, scene returns to the writer.**

### A. Banned constructions

Never ship a line containing:

- "We need to talk about —"
- "Look, I know —" / "Listen to me —"
- "I never asked for this."
- "This changes everything."
- "You don't understand. / You have no idea."
- "It's not that simple."
- "Are you sure about this?" (as a scene's only pushback)
- "…but at what cost?"
- "This war has changed us all." — and every thesis statement of that shape
- "I'm sorry for your loss."
- Any line where a character names their own emotion ("I'm angry", "I'm scared") — **unless** the character is Reyes, clinically, or it is one of the game's ~30 rationed direct beats and is marked as such in the script file.

### B. Structural failures

| Check | Fails when |
|---|---|
| **Length variance** | All turns in a scene fall within a 1.5× word-count band |
| **Symmetry** | Characters alternate strict A-B-A-B for more than six turns with no interruption or silence |
| **Name density** | Characters address each other by name more than once per eight turns |
| **Triads** | Any "X, Y, and Z" rhetorical triple more than once per scene |
| **Em-dash rate** | More than one dash-interrupted line per five lines |
| **Explaining** | Any line whose sole function is to convey information the codex could carry |
| **Articulacy** | Every character speaks in complete, grammatical sentences — nobody stumbles, repeats, or trails off |
| **Fingerprint** | A line could be moved to another character without editing (§18.4 violation) |
| **Breath** | Any line that cannot be read aloud in one breath (§18.2 law 5) |

### C. The three tests

1. **The strip test.** Remove all speaker names from the scene. Give it to a reader who knows the cast. If they cannot reassign at least 80% of lines correctly, the voices are mush. **Rewrite.**
2. **The deletion test.** Delete the final sentence of every line. If the scene still works — and it usually does — that deletion was the fix. AI over-explains at the end of lines; this test catches it mechanically.
3. **The read-aloud test.** Say it out loud. Anything you stumble over, an actor and a TTS model will stumble over too.

### D. Failure modes specific to machine writing

Watch for these; they are the tells:

- Every character is equally articulate and equally self-aware.
- Emotional beats are announced before they are earned.
- Nobody is boring, petty, or wrong in an uninteresting way — real people are all three.
- Conflict resolves inside the scene it starts in.
- Characters have no verbal habits, only opinions.
- Everyone agrees on what the scene is about.

---

## 18.10 Definition of Done — a scene is finished when

- [ ] Want / obstacle / turn are each nameable in one sentence
- [ ] Every line passes §18.9 A, B, and the three tests in C
- [ ] Line lengths obey §18.3 for their delivery context, with ≥3× variance inside the scene
- [ ] Every character's fingerprint (§18.4) is detectable in at least one line
- [ ] Voss variants exist for every axis the scene branches on
- [ ] Every line has an ID, speaker, voice-ID, audio tags, and credit tier per `phase0/SCRIPT_FORMAT.md`
- [ ] Continuity checked against `02_story_bible.md` canon glossary — no invented names, no contradicted events
- [ ] `dialogue-critic` returned **GO**

---

## 18.11 Sources

Craft principles here are grounded in published game-writing practice, not invention:

- [How a character says hello: writing "barks" for video games — Sarah Beaulieu](https://sarah-beaulieu.com/en/writing-barks-for-video-games) — bark triggers, variant counts, compression
- [8 Key Principles of Writing Effective Game Dialogue — Game Developer](https://www.gamedeveloper.com/game-platforms/8-key-principles-of-writing-effective-game-dialogue)
- [Videogame Dialogues: Writing Tools And Design Ideas — Game Developer](https://www.gamedeveloper.com/design/videogame-dialogues-writing-tools-and-design-ideas)
- [World Building With Dialogue Barks — Indie Game Writing](https://indiegamewriting.com/writing-2-world-building-with-barks/)
- [How Writers and Voice Actors Shape Game Characters at GDC](https://www.voiceactorsnews.com/2026/03/08/gdc-voice-actors-narrative-design/) — subtext, voice differentiation, line revision in performance
- [The Witcher 3's script: 450,000 words](https://wccftech.com/witcher-3-script-450000-words-4x-larger-average/) and [list of longest game scripts](https://gamicus.fandom.com/wiki/List_of_longest_video_game_scripts) — scale calibration

---

*Next: [19_voice_production.md](19_voice_production.md) — how these lines become audio.*
