# Generates the first SFX set + one music sting via the ElevenLabs
# sound-generation and music APIs (GDD 16.5/16.6/16.12) into
# Eclipse/Saved/AudioStaging/{SFX,Music}. System python, stdlib only, no UE.
#
# Pattern mirrors the voice pipeline (EclipseElevenLabsClient/FEclipseVoiceCache):
#   - API key from ELEVENLABS_API_KEY env var, else gitignored
#     Eclipse/Config/UserSecrets.ini [ElevenLabs] ApiKey. NEVER printed/logged.
#   - Every asset has a spec key = md5(endpoint + canonical request body):
#     a hit (file on disk + matching manifest entry) is never regenerated,
#     so re-runs cost 0 credits (16.15 rule 7 applied to SFX/music).
#   - We request raw PCM16 (output_format=pcm_<rate>) and wrap it in a WAV
#     ourselves — no audio decoder anywhere in the pipeline (16.12/16.14).
#     Channel count is not in the raw stream, so it is resolved from the
#     requested duration: measured-as-mono ~= 2x expected => stereo re-wrap.
#   - manifest.json carries full provenance (prompt, params, hashes, measured
#     waveform stats, credit usage) and never the key.
#
# Run:      python Eclipse/Tools/generate_audio_assets.py
# Options:  --dry-run        print the plan, no API calls
#           --only NAME      generate just one asset (exact name)
#           --force NAME     regenerate one asset even on a cache hit
#                            (use only after judging the waveform as failed —
#                             no mass retries, each take costs credits)
#
# Import into UE later via Tools/import_generated_audio.py (NOT while the
# owner editor is running — see that script's header).

import argparse
import configparser
import datetime as _dt
import hashlib
import json
import os
import struct
import sys
import urllib.error
import urllib.request
import wave
from array import array

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
STAGING = os.path.join(REPO, "Eclipse", "Saved", "AudioStaging")
MANIFEST_PATH = os.path.join(STAGING, "manifest.json")
SECRETS_INI = os.path.join(REPO, "Eclipse", "Config", "UserSecrets.ini")

API_BASE = "https://api.elevenlabs.io"
# Highest first; lower-tier fallbacks mirror the TTS pipeline's 22050 (known
# to work on this account). mp3 is the very last resort and is flagged
# needs_conversion because UE cannot import mp3 and this box has no ffmpeg.
PCM_FORMATS = ["pcm_44100", "pcm_24000", "pcm_22050"]
MP3_FALLBACK = "mp3_44100_128"

# ---------------------------------------------------------------------------
# Asset specs — one take each, short clips, credits stay low (16.13).
# SFX direction (16.5/15.5): punchy and stylized, not photoreal-shredded.
# Layer-1 ambient (16.7) must be seamless: the API's loop mode handles it.
# ---------------------------------------------------------------------------
SFX_SPECS = [
    {
        "name": "SFX_Weapon_RebelRifle_Shot_01",
        "prompt": (
            "Single gunshot from an improvised rebel energy rifle: one punchy, "
            "tight crack with a short mid-bass thump and a brief metallic tail. "
            "Stylized video game weapon one-shot, bold and readable, not a "
            "realistic battlefield recording. Dry, minimal reverb."
        ),
        "duration_seconds": 1.2,
        "prompt_influence": 0.45,
    },
    {
        "name": "SFX_Impact_BulletMetal_01",
        "prompt": (
            "Bullet impact on a hollow sheet-metal panel: one sharp metallic "
            "clank with a slight ring-out. Stylized video game hit sound, "
            "single one-shot, dry."
        ),
        "duration_seconds": 0.8,
        "prompt_influence": 0.45,
    },
    {
        "name": "SFX_UI_Tick_01",
        "prompt": (
            "Tiny dry user-interface tick: a single short clean digital click, "
            "neutral pitch, video game menu navigation blip."
        ),
        "duration_seconds": 0.5,
        "prompt_influence": 0.5,
    },
    {
        "name": "SFX_UI_Confirm_01",
        "prompt": (
            "Positive user-interface confirm chime: two quick ascending "
            "synthetic notes, clean and short, video game menu accept sound."
        ),
        "duration_seconds": 0.8,
        "prompt_influence": 0.5,
    },
    {
        # Layer-1 atmosphere bed for Kessara (16.7 never-silent floor, 16.10).
        "name": "SFX_Amb_Kessara_Industrial_Loop_01",
        "prompt": (
            "Industrial factory district ambience: a deep steady machine hum, "
            "rhythmic distant metal presses, a faint far-away alarm siren, an "
            "occasional short steam hiss. Oppressive but calm bed, no music, "
            "no voices, seamless loop."
        ),
        "duration_seconds": 14.0,
        "prompt_influence": 0.3,
        "loop": True,
    },
    {
        "name": "SFX_Foot_Asphalt_01",
        "prompt": (
            "One single footstep on asphalt: a boot stepping once on rough "
            "dry concrete-asphalt, short and dry, video game foley one-shot."
        ),
        "duration_seconds": 0.6,
        "prompt_influence": 0.5,
    },
    {
        "name": "SFX_Foot_Metal_01",
        "prompt": (
            "One single footstep on a metal grate walkway: a boot stepping "
            "once on hollow steel with a slight rattle, short and dry, video "
            "game foley one-shot."
        ),
        "duration_seconds": 0.6,
        "prompt_influence": 0.5,
    },
]

MUSIC_SPECS = [
    {
        # Mission-complete beat (16.6 companion payoff moment; 16.8 Resistance
        # identity: human, strings, imperfect; leitmotif discipline 16.7C).
        "name": "MUS_Sting_MissionComplete_Resistance_01",
        "prompt": (
            "Short cinematic mission-complete sting for a sci-fi resistance "
            "story. Dark but hopeful: opens on low sustained strings over a "
            "deep drone, then a small heroic motif rises in warm brass and "
            "strings — a simple, memorable four-note resistance theme — with "
            "subtle industrial percussion underneath, ending on a resolved, "
            "quietly triumphant chord. Hybrid orchestral, restrained, "
            "emotional, about 18 seconds."
        ),
        "music_length_ms": 18000,
    },
]


# ---------------------------------------------------------------------------
# Key handling — mirrors EclipseElevenLabs::GetApiKeyFromEnvironment().
# The key must never appear in stdout, the manifest, or any error text.
# ---------------------------------------------------------------------------
def get_api_key():
    key = os.environ.get("ELEVENLABS_API_KEY", "").strip()
    if key:
        return key, "env:ELEVENLABS_API_KEY"
    if os.path.isfile(SECRETS_INI):
        cp = configparser.ConfigParser()
        cp.read(SECRETS_INI)
        if cp.has_section("ElevenLabs"):
            key = cp.get("ElevenLabs", "ApiKey", fallback="").strip()
            if key and "PUT_YOUR" not in key:
                return key, "Config/UserSecrets.ini"
    return "", "none"


def scrub(text, key):
    """Belt-and-braces: strip the key from any string we might print."""
    return text.replace(key, "***") if key else text


# ---------------------------------------------------------------------------
# HTTP
# ---------------------------------------------------------------------------
def api_request(key, method, path, body=None, timeout=120):
    url = API_BASE + path
    data = json.dumps(body).encode("utf-8") if body is not None else None
    req = urllib.request.Request(url, data=data, method=method)
    req.add_header("xi-api-key", key)
    if data is not None:
        req.add_header("Content-Type", "application/json")
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            return resp.status, resp.read(), ""
    except urllib.error.HTTPError as e:
        detail = ""
        try:
            detail = e.read().decode("utf-8", "replace")[:400]
        except Exception:
            pass
        return e.code, b"", scrub(detail, key)
    except Exception as e:  # network / timeout
        return 0, b"", scrub(str(e), key)[:400]


class UnmeasurableSpendError(RuntimeError):
    """The credit meter cannot read the account. Never generate blind."""


def get_usage(key):
    """Subscription credit counters (never contains the key).

    Returns (usage_dict, error_string). Exactly one of the two is truthy.

    This used to return a bare None on every failure, which main() then
    silently swallowed: on a scoped key the run generated audio, spent real
    credits, and wrote no usage_credits at all. A meter that is broken and
    does not say so is worse than no meter, because VOICE_LEDGER.md then
    reads as truth while nothing was measured. So failures are now typed and
    loud -- see require_usage_measurement().
    """
    status, payload, err = api_request(key, "GET", "/v1/user/subscription")
    if status == 200:
        try:
            d = json.loads(payload)
            return {
                "character_count": d.get("character_count"),
                "character_limit": d.get("character_limit"),
            }, ""
        except Exception as e:
            return None, f"subscription payload unparsable: {e}"
    if status == 401:
        scope = "user_read"
        if "permission" in err and "user_read" not in err:
            scope = err.split("permission", 1)[1].strip().split()[0]
        return None, (f"HTTP 401 - the API key lacks the '{scope}' scope. "
                      f"Grant it in the ElevenLabs dashboard, or spend is unmeasurable.")
    return None, f"HTTP {status} on /v1/user/subscription: {err[:180]}"


def require_usage_measurement(key, what="this run"):
    """Pre-flight gate: refuse to spend credits we cannot measure.

    Called BEFORE the first generating request. Raises instead of returning a
    falsy value, so no caller can accidentally continue on an empty result.
    """
    usage, err = get_usage(key)
    if usage is None:
        raise UnmeasurableSpendError(
            f"REFUSING TO GENERATE - credit spend cannot be measured for {what}.\n"
            f"  reason: {err}\n"
            f"  why this is fatal: every generated credit is spent permanently and\n"
            f"  phase0/VOICE_LEDGER.md is the project's record of truth. Generating\n"
            f"  without a before/after reading would put an unverifiable number in it.\n"
            f"  fix: give the key the scopes user_read + speech_history_read\n"
            f"       (ElevenLabs dashboard -> API keys), then re-run.")
    return usage


# ---------------------------------------------------------------------------
# WAV wrap + analysis (no ffmpeg on this machine — pure stdlib)
# ---------------------------------------------------------------------------
def write_wav(path, pcm, rate, channels):
    frame_bytes = 2 * channels
    usable = len(pcm) - (len(pcm) % frame_bytes)
    with wave.open(path, "wb") as w:
        w.setnchannels(channels)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(pcm[:usable])


def analyze_wav(path):
    with wave.open(path, "rb") as w:
        rate, channels, nframes = w.getframerate(), w.getnchannels(), w.getnframes()
        raw = w.readframes(nframes)
    samples = array("h")
    samples.frombytes(raw[: len(raw) - (len(raw) % 2)])
    if sys.byteorder == "big":
        samples.byteswap()
    peak = max((abs(s) for s in samples), default=0) / 32768.0
    rms = (sum(s * s for s in samples) / max(1, len(samples))) ** 0.5 / 32768.0
    return {
        "sample_rate": rate,
        "channels": channels,
        "duration_s": round(nframes / float(rate), 3),
        "peak": round(peak, 4),
        "rms": round(rms, 4),
    }


def resolve_channels(pcm_len_bytes, rate, expected_s):
    """Raw PCM has no header: infer mono vs stereo from the requested length.
    Measured-as-mono duration ~2x the expected duration => it was stereo."""
    mono_s = (pcm_len_bytes / 2.0) / rate
    if expected_s > 0 and abs(mono_s - 2.0 * expected_s) < abs(mono_s - expected_s):
        return 2
    return 1


# ---------------------------------------------------------------------------
# Generation
# ---------------------------------------------------------------------------
def spec_key(endpoint, body):
    canonical = endpoint + "|" + json.dumps(body, sort_keys=True)
    return hashlib.md5(canonical.encode("utf-8")).hexdigest()


def generate_audio(key, endpoint_paths, body, loop_field=None):
    """Try PCM formats highest-first, falling back across formats/endpoints.
    Returns (fmt, rate, payload_bytes, error)."""
    last_err = ""
    for path in endpoint_paths:
        for fmt in PCM_FORMATS + [MP3_FALLBACK]:
            attempt_body = dict(body)
            status, payload, err = api_request(
                key, "POST", f"{path}?output_format={fmt}", attempt_body,
                timeout=300)
            # Unknown-field rejection for optional params (e.g. loop): retry once without.
            if status in (400, 422) and loop_field and loop_field in attempt_body \
                    and loop_field in err:
                attempt_body.pop(loop_field)
                status, payload, err = api_request(
                    key, "POST", f"{path}?output_format={fmt}", attempt_body,
                    timeout=300)
            if 200 <= status < 300 and payload:
                rate = int(fmt.split("_")[1]) if fmt.startswith("pcm_") else 0
                return fmt, rate, payload, ""
            last_err = f"HTTP {status} on {path} ({fmt}): {err}"
            fmt_related = any(t in err.lower() for t in
                              ("output_format", "format", "tier", "plan", "upgrade"))
            if status in (404, 405):
                break  # wrong endpoint — try the next path
            if not fmt_related:
                return "", 0, b"", last_err  # real failure: do not spend more attempts
    return "", 0, b"", last_err


def load_manifest():
    if os.path.isfile(MANIFEST_PATH):
        with open(MANIFEST_PATH, "r", encoding="utf-8") as f:
            return json.load(f)
    return {
        "provider": "ElevenLabs (api.elevenlabs.io)",
        "license": "Generated on the owner's ElevenLabs account; usable per that "
                   "account's commercial license. Provenance: Eclipse/Content/Audio_SOURCES.md",
        "generated_by": "Eclipse/Tools/generate_audio_assets.py",
        "note": "No API key is ever stored here. Spec key = md5(endpoint|body): "
                "re-runs skip existing entries, so regeneration cost is zero.",
        "assets": {},
        "usage_credits": {},
    }


def save_manifest(manifest):
    # Persist immediately after every asset — a crash later must not lose
    # paid-for API work (same rationale as FEclipseVoiceCache::Add).
    os.makedirs(STAGING, exist_ok=True)
    with open(MANIFEST_PATH, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)
        f.write("\n")


def process(key, manifest, category, subdir, endpoint_paths, spec, expected_s,
            body, loop_field=None, force=False):
    name = spec["name"]
    skey = spec_key(endpoint_paths[0], body)
    entry = manifest["assets"].get(name)
    out_dir = os.path.join(STAGING, subdir)
    os.makedirs(out_dir, exist_ok=True)

    if entry and not force:
        existing = os.path.join(STAGING, entry["file"])
        if entry.get("spec_key") == skey and os.path.isfile(existing):
            print(f"[cache] {name} — hit, 0 credits ({entry['file']})")
            return True

    print(f"[gen]   {name} ...")
    fmt, rate, payload, err = generate_audio(key, endpoint_paths, body, loop_field)
    if not payload:
        print(f"[FAIL]  {name}: {err}")
        return False

    if fmt.startswith("pcm_"):
        channels = resolve_channels(len(payload), rate, expected_s)
        fname = f"{subdir}/{name}.wav"
        write_wav(os.path.join(STAGING, fname), payload, rate, channels)
        stats = analyze_wav(os.path.join(STAGING, fname))
        dur_ok = expected_s <= 0 or abs(stats["duration_s"] - expected_s) <= 0.2 * expected_s + 0.2
        loud_ok = stats["peak"] >= 0.02 and stats["rms"] >= 0.001
        status = "ok" if (dur_ok and loud_ok) else "suspect"
    else:
        fname = f"{subdir}/{name}.mp3"
        with open(os.path.join(STAGING, fname), "wb") as f:
            f.write(payload)
        stats = {}
        status = "needs_conversion"  # UE cannot import mp3; no ffmpeg on this box

    manifest["assets"][name] = {
        "file": fname,
        "category": category,
        "endpoint": endpoint_paths[0],
        "request": body,           # full prompt/params provenance, no key
        "output_format": fmt,
        "spec_key": skey,
        "sha256": hashlib.sha256(payload).hexdigest(),
        "bytes": len(payload),
        **stats,
        "status": status,
        "generated_at": _dt.datetime.now(_dt.timezone.utc).isoformat(timespec="seconds"),
    }
    save_manifest(manifest)
    extra = f" {stats['duration_s']}s peak {stats['peak']} rms {stats['rms']}" if stats else ""
    print(f"[done]  {name} -> {fname} [{fmt}] {status}{extra}")
    return status != "needs_conversion"


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--only")
    ap.add_argument("--force", help="regenerate this one asset despite a cache hit")
    args = ap.parse_args()

    plan = [("SFX", "SFX", ["/v1/sound-generation"], s) for s in SFX_SPECS] + \
           [("Music", "Music", ["/v1/music", "/v1/music/compose"], s) for s in MUSIC_SPECS]

    if args.dry_run:
        for cat, sub, eps, s in plan:
            print(f"{cat:5} {s['name']}")
        return 0

    key, source = get_api_key()
    if not key:
        print("ERROR: no ElevenLabs key (set ELEVENLABS_API_KEY or fill "
              "Eclipse/Config/UserSecrets.ini [ElevenLabs] ApiKey).")
        return 1
    print(f"key source: {source} (value never printed)")
    return main_with_key(key, args)


def main_with_key(key, args=None):
    """Everything after key resolution. Split out so test_credit_meter.py can
    drive a full run with a stub key and assert that nothing generates when
    spend cannot be measured."""
    if args is None:
        args = argparse.Namespace(only=None, force=None)

    plan = [("SFX", "SFX", ["/v1/sound-generation"], s) for s in SFX_SPECS] + \
           [("Music", "Music", ["/v1/music", "/v1/music/compose"], s) for s in MUSIC_SPECS]

    manifest = load_manifest()

    # Pre-flight: no generation happens until spend can be measured (16.15 r7 +
    # 19.2 "log every batch"). Raises UnmeasurableSpendError on a scoped key.
    try:
        usage_before = require_usage_measurement(key, "the SFX/music batch")
    except UnmeasurableSpendError as e:
        print(e)
        return 3
    print(f"credit meter OK: {usage_before['character_count']}"
          f"/{usage_before['character_limit']} used before this run")

    ok = True
    for cat, sub, eps, s in plan:
        if args.only and s["name"] != args.only:
            continue
        force = args.force == s["name"]
        if cat == "SFX":
            body = {"text": s["prompt"],
                    "duration_seconds": s["duration_seconds"],
                    "prompt_influence": s["prompt_influence"]}
            if s.get("loop"):
                body["loop"] = True
            ok &= process(key, manifest, cat, sub, eps, s, s["duration_seconds"],
                          body, loop_field="loop", force=force)
        else:
            body = {"prompt": s["prompt"], "music_length_ms": s["music_length_ms"]}
            ok &= process(key, manifest, cat, sub, eps, s,
                          s["music_length_ms"] / 1000.0, body, force=force)

    usage_after, after_err = get_usage(key)
    if usage_after is None:
        # Credits are already gone at this point, so this cannot be a silent skip:
        # record the hole in the manifest and fail the run.
        manifest["usage_credits"] = {
            "last_run_delta": None,
            "measurement_failed": after_err,
            "measured_at": _dt.datetime.now(_dt.timezone.utc).isoformat(timespec="seconds"),
            "WARNING": "Credits were spent but the after-reading failed. The spend "
                       "for this run is UNMEASURED - do not copy a number into "
                       "phase0/VOICE_LEDGER.md as if it were measured.",
        }
        save_manifest(manifest)
        print(f"\nSPEND UNMEASURED after generating: {after_err}")
        print("  phase0/VOICE_LEDGER.md must record this run as an ESTIMATE.")
        return 4

    delta = (usage_after["character_count"] or 0) - (usage_before["character_count"] or 0)
    manifest["usage_credits"] = {
        "last_run_delta": delta,
        "account_used": usage_after["character_count"],
        "account_limit": usage_after["character_limit"],
        "measured_at": _dt.datetime.now(_dt.timezone.utc).isoformat(timespec="seconds"),
    }
    save_manifest(manifest)
    print(f"credits: this run {delta}, account "
          f"{usage_after['character_count']}/{usage_after['character_limit']}")
    return 0 if ok else 2


if __name__ == "__main__":
    sys.exit(main())
