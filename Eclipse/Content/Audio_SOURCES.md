# Audio sources — generated audio (SFX / Music / VO)

## ElevenLabs (api.elevenlabs.io) — owner account

All generated audio in this project comes from the **owner's ElevenLabs
account** (GDD 16.12); usage falls under that account's **commercial license**
(paid plan). The API key lives only in the environment variable
`ELEVENLABS_API_KEY` or the gitignored `Eclipse/Config/UserSecrets.ini` —
never in git, logs, manifests, or chat (16.15 rule 9).

| Set | Generator | Staging / cache | Import | Manifest |
|---|---|---|---|---|
| Squad-bark VO (8 lines, 2 voices) | `EclipseGenerateVoices` commandlet (C++, `Source/Eclipse/Audio/`) | `Content/Audio/Generated/*.wav` (committed cache) | commandlet auto-imports | `Content/Audio/Generated/VoiceCacheManifest.json` |
| First SFX set (7 one-shots/loops, 16.5) + mission-complete music sting (16.6) — generated 2026-07-23 | `Tools/generate_audio_assets.py` (python, sound-generation + music APIs) | `Saved/AudioStaging/{SFX,Music}/*.wav` (PCM16 stereo 44.1 kHz) | `Tools/import_generated_audio.py` → `/Game/Audio/{SFX,Music}` (run only when the owner editor is closed — see script header) | `Saved/AudioStaging/manifest.json` (full prompts, params, hashes, waveform stats) |

**Caching / cost rule (16.12/16.15):** every asset is keyed by a hash of its
generation request; a hit is never regenerated, so re-runs cost 0 credits.
Regenerate a single failed take only via `--force <name>` after judging the
waveform — no mass retries.

**Durable cache (16.12: "the cache travels with the repo, no machine
re-pays"):** `Saved/` is gitignored and volatile, so the paid output + full
manifest are mirrored to the committed `Eclipse/AudioCache/` (2026-07-23,
review follow-up). After a Saved-clean or on a fresh machine, restore staging
with a straight copy `AudioCache → Saved/AudioStaging` before running the
generator (it will then hit cache for everything already paid for).

**Placeholder status:** these SFX are stylized dev placeholders per 16.4/16.5 —
footsteps are interim until the Fab footstep packs land; final story-critical
audio follows the production pipeline. The Kessara industrial loop is the
Layer-1 atmosphere bed (16.7 never-silent floor).

**Credit note:** the scoped API key lacks the `user_read` permission, so exact
credit deltas cannot be read via the API; check usage in the ElevenLabs
dashboard. This first set is roughly ~1,000–1,500 credits (≈1% of the 121k
monthly pool): ~18.5 s of SFX plus one ~18 s music generation.
