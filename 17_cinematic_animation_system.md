# PART 17 — CINEMATIC & ANIMATION SYSTEM DESIGN
**ECLIPSE: RISE OF THE RESISTANCE**
*Game Design Bible — Document 17 of 17+ | Character animation, cinematics, facial performance, dynamic camera*

> **Numbering note:** authored as "Part 16 — Cinematic"; filed as **17** (15 = Visual Quality, 16 = Audio).
>
> **Phase pacing (graybox rule):** Phase 1 has **no cinematics and no authored animation** beyond default locomotion (SPEC-P1-00). This is **forward infrastructure** — cinematic/animation authoring begins at **Phase 2** with the first MetaHuman characters and the vertical-slice missions, inside the 12.4 budgets. It never overrides the current ACTIVE_MILESTONE.

---

# 17.1 Cinematic Philosophy

Cinematics in ECLIPSE are **not separate movies** — they are an extension of gameplay. Every cinematic must strengthen the player's emotional connection, show the consequences of their actions, reveal character motivation, communicate the scale of the rebellion, and transition naturally back into play. The goal is not passive cutscenes; it is to make the player feel *inside a living war*.

---

# 17.2 Cinematic Architecture Overview

Five connected systems, all driven **through the Event Bus** — no mission system controls cinematics directly:

| System | Purpose | Unreal tech |
|---|---|---|
| Character Animation | Believable movement | MetaHuman + Control Rig + Motion Matching |
| Cinematic Sequencer | Story scenes & scripted events | Unreal Sequencer |
| Facial Animation | Emotional performance | MetaHuman Animator |
| Dynamic Camera | Film-quality presentation | Cine Camera Actor |
| Runtime Cinematic | Gameplay-triggered scenes | Custom C++ framework |

```
Mission Completed Event ──► Cinematic Manager ──► select cinematic sequence
```
Never `Mission.cpp → PlayCutscene()`.

---

# 17.3 Character Animation Pipeline

Characters move and behave like believable humans, using MetaHumans, motion matching, Control Rig, animation blueprints, and procedural animation.

```
AEclipseCharacter → Animation Component → Animation Blueprint → Motion Matching
```
Reacts to: movement speed · weapon type · injuries · stamina · emotional state · combat situation. *(The single-body rule from 12.3 holds — player, squad and enemies share `AEclipseCharacter`; performance differs by data and components, never a divergent class.)*

---

# 17.4 Motion Matching System

Movement should not feel like traditional clip-based animation. The system provides realistic walking, running, turning, stopping, climbing and combat movement by selecting from:
```
current movement + desired movement + environment + character state
```
A wounded soldier doesn't merely play slower — the system changes posture, speed, balance, and facial expression.

---

# 17.5 Combat Animation System

Combat animations communicate gameplay through **anticipation → action → recovery**.

- **Reload:** reach → remove magazine → insert → check → return to combat.
- **Melee:** attack decision → animation select → hit detection → reaction.

Recovery frames are readable so the player (and squad) can time around them (ties to 8.x combat feel).

---

# 17.6 Facial Animation System

Critical for companions, commanders, important NPCs, and cinematic moments. Technology: **MetaHuman Animator**, facial capture, audio-driven animation. Characters must show anger, fear, sadness, determination, hesitation.

---

# 17.7 AI-Driven Facial Performance

Dialogue drives facial performance. Inputs: voice emotion + character personality + relationship state + story context → facial expression, eye movement, body language, voice intensity. *(This is where Part 16's ElevenLabs emotion parameters feed the face — one emotion value drives both the voice and the performance.)*

- Confident commander → controlled movement, direct eye contact, minimal gestures.
- Scared civilian → unstable eyes, defensive posture, nervous expression.

---

# 17.8 Cinematic Camera System

The camera should feel like professional film. Technology: Cine Camera Actor, Sequencer, camera rigs. Decisions consider emotional importance, character status, environment scale, combat intensity.

**Dynamic camera rules:** avoid unnecessary cinematic interruptions. Prefer gameplay camera transitions, seamless third-person moments, and environmental storytelling. *Player enters a destroyed city* → the camera briefly reveals ruined buildings, civilians rebuilding, Dominion occupation — no dialogue required.

---

# 17.9 Runtime Cinematic System

Cinematics can trigger dynamically from state. Inputs: CampaignState · mission outcome · character relationships · faction control · player choices.

*The player loses a planet* → instead of a fixed scene: refugees appear, companions react, propaganda changes, enemy confidence rises.

---

# 17.10 Companion Performance System

Companions are central characters with animation personality, idle behaviours, combat reactions, and emotional states.
- Aggressive companion → faster movement, stronger gestures, direct posture.
- Cautious companion → defensive stance, slower reactions, careful movement.

---

# 17.11 Environmental Storytelling Animation

The world tells stories on its own.
- **Occupied city:** civilians avoiding patrols, soldiers checking documents, propaganda screens, prisoners transported.
- **Liberated city:** rebuilding, celebrations, new resistance symbols, increased civilian activity.

*(Driven by CampaignState faction control — the same data the audio director reads, so sight and sound agree.)*

---

# 17.12 Cinematic Quality Standards

Every cinematic must pass **Character** (believable facial animation, natural body movement, correct emotion), **Camera** (intentional composition, cinematic framing, professional pacing), and **World** (environment supports the scene, lighting enhances emotion, sound matches visuals — cross-check with Parts 15 & 16).

---

# 17.13 Unreal Engine Implementation

- **Sequencer** — story cinematics, scripted events, trailers.
- **Control Rig** — procedural adjustments, hand placement, posing.
- **MetaHuman Framework** — hero characters, realistic faces, facial animation.
- **Motion Matching** — realistic movement.

```
/Content/Cinematics
  /Sequences  /Cameras  /Characters  /Animations  /Facial  /ControlRigs

/Source/EclipseCinematics
  CinematicSubsystem  CameraManager  RuntimeSequenceManager
```

---

# 17.14 Development Rules

1. Cinematics must support gameplay, never replace it.
2. Never remove player agency unnecessarily.
3. Characters express emotion through movement.
4. Every major character has a unique animation identity.
5. Dynamic events use CampaignState.
6. Avoid expensive cinematic-only systems (12.4 budget).
7. Reuse gameplay animations wherever possible.

---

# 17.15 Definition of Done

Complete when: ✅ characters move naturally in all situations · ✅ facial animation matches dialogue emotion (Part 16 link) · ✅ cinematics transition smoothly from gameplay · ✅ companions feel like real people · ✅ camera presentation reaches AAA quality (Part 15 link) · ✅ environmental storytelling works without dialogue · ✅ animations stay within the 12.4 performance budget · ✅ cinematics react to player choices and world state.

---

*Prev: [16_audio_system.md](16_audio_system.md) · This is the final numbered document; see [00_INDEX.md](00_INDEX.md).*
