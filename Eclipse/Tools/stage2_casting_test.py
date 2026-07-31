#!/usr/bin/env python3
"""TIER 0 / STAGE 2 - deep test on the owner's finalists (19_voice_production.md §19.3).

Stage 1 was free: metadata and generic previews. That cannot tell you whether a
voice survives Mara's death scene or stays readable at five words over gunfire.
Only our own lines answer that, so this is where casting costs credits.

Per §19.3 every finalist speaks THREE lines:
  1. the signature line      - does the §18.4 fingerprint survive in this voice?
  2. an emotional extreme    - the place the character actually goes
  3. a short combat line     - the same voice at 5 words under gunfire

Both finalists of a role speak identical text; that is the whole point of a
comparison. Audio tags are from the approved set in §19.4, max one per line here.

Guards, none of them optional:
  - reuses require_usage_measurement() from generate_audio_assets, so this
    refuses to run while spend cannot be measured (missing `user_read` scope).
  - hard credit ceiling; refuses to start if the plan exceeds it.
  - never regenerates an existing clip (cache by role+rank+line).

Run:  python Eclipse/Tools/stage2_casting_test.py --dry-run   (plan + cost, free)
      python Eclipse/Tools/stage2_casting_test.py             (generate)
"""
import argparse
import json
import os
import sys
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import generate_audio_assets as gen  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent.parent
RESOLVED = ROOT / "phase0" / "CASTING_RESOLVED.json"
OUT = ROOT / "progress_media" / "casting" / "stage2"
MANIFEST = OUT / "stage2_manifest.json"

# Tier 0 is 6,000 credits on the corrected 131,000 budget. Stage 1 spent 0.
CREDIT_CEILING = 6000
# eleven_v3 confirmed available on this account 2026-07-31 via /v1/models.
# §19.4's audio tags only work on v3, and modelId is part of the cache key, so
# this is also the model Tier 1/2 must ship on. Do not change casually.
MODEL_ID = "eleven_v3"
OUTPUT_FORMAT = "mp3_44100_128"       # auditions are for listening, not shipping
SETTINGS = {"stability": 0.5, "similarity_boost": 0.75,
            "style": 0.0, "use_speaker_boost": True}

# Three lines per role. Written against the §18.4 fingerprints.
LINES = {
 "mara": [
  ("signature", "We don't get to be tired. Not yet."),
  ("extreme",   "[quietly] Don't carry me. Carry the work. "
                "We were always going to be more than one of us."),
  ("combat",    "Cover the stairwell. Go."),
 ],
 "dex": [
  ("signature", "That's not a reactor. That's a bomb with a job."),
  ("extreme",   "[nervous] Coolant's gone. Housing's cracked. Ninety seconds, maybe. "
                "I've got a joke about this one but it isn't any good."),
  ("combat",    "Breach charge is live! Back!"),
 ],
 "brick": [
  ("signature", "Tam. Oyelaran. Vic."),
  ("extreme",   "[grieving] I said their names. Somebody has to say them."),
  ("combat",    "Gun's dry. Ten seconds."),
 ],
 "reyes": [
  ("signature", "I do not need you to be brave. I need you to be still."),
  ("extreme",   "[cold] Four of them. Was that the plan, "
                "or was that simply what happened?"),
  ("combat",    "Pressure on the wound. Now."),
 ],
 "voss_m": [
  ("signature", "The coupling's already cracked. Give me thirty seconds and it's ours."),
  ("extreme",   "[angry] Don't tell me what it cost. I was standing in it."),
  ("combat",    "Contact left! Falling back!"),
 ],
 "voss_f": [
  ("signature", "The coupling's already cracked. Give me thirty seconds and it's ours."),
  ("extreme",   "[angry] Don't tell me what it cost. I was standing in it."),
  ("combat",    "Contact left! Falling back!"),
 ],
 "dominion_conscript": [
  ("signature", "[nervous] Contact, grid seven! Requesting authorization!"),
  ("extreme",   "[shouting] I can't see him! He's inside the perimeter, he's inside!"),
  ("combat",    "Reloading! Cover me!"),
 ],
 "veil_operative": [
  ("signature", "[cold] He is not armed. Continue the sweep."),
  ("extreme",   "[quietly] You will tell me. Everyone does. "
                "The only variable is when."),
  ("combat",    "Target displaced. Sweep left."),
 ],
 "eclipse_fighter": [
  ("signature", "Reyes is down - someone get to Reyes!"),
  ("extreme",   "[shouting] That's my sister's building! That's my sister's building!"),
  ("combat",    "Two of them, upstairs!"),
 ],
}

APPROVED_TAGS = {"whispering", "shouting", "quietly", "nervous", "angry",
                 "exhausted", "amused", "cold", "urgent", "grieving",
                 "sighs", "laughs", "exhales", "clears throat", "pause", "beat"}


def check_tags():
    """§19.4: only the approved vocabulary, at most two tags per line."""
    import re
    bad = []
    for role, lines in LINES.items():
        for kind, text in lines:
            tags = re.findall(r"\[([^\]]+)\]", text)
            for t in tags:
                if t not in APPROVED_TAGS:
                    bad.append(f"{role}/{kind}: unapproved tag [{t}]")
            if len(tags) > 2:
                bad.append(f"{role}/{kind}: {len(tags)} tags, max 2")
    return bad


def build_plan():
    if not RESOLVED.is_file():
        print(f"No resolved casting yet - run resolve_casting_choice.py first.")
        return None, 0
    data = json.loads(RESOLVED.read_text("utf-8"))
    plan = []
    for rid, role in data["rollen"].items():
        lines = LINES.get(rid)
        if lines is None:
            print(f"  (no stage-2 lines written yet for '{rid}' - skipped)")
            continue
        for f in role["finalisten"]:
            for kind, text in lines:
                plan.append({
                    "role": rid, "label": role["label"], "rank": f["rang"],
                    "voice_name": f["stem"], "voice_id": f["voice_id"],
                    "kind": kind, "text": text, "chars": len(text),
                    "file": f"{rid}_keus{f['rang']}_{f['stem']}_{kind}.mp3",
                })
    return plan, sum(p["chars"] for p in plan)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--model", default=MODEL_ID)
    args = ap.parse_args()

    bad = check_tags()
    if bad:
        print("TAG CHECK FAILED (§19.4):")
        for b in bad:
            print("  " + b)
        return 1

    plan, total = build_plan()
    if plan is None:
        return 1

    roles = sorted({p["role"] for p in plan})
    print(f"\nStage 2 plan: {len(plan)} clips, {len(roles)} roles, "
          f"{len({(p['role'], p['rank']) for p in plan})} finalists")
    print(f"model: {args.model} | ceiling: {CREDIT_CEILING} credits\n")
    cur = None
    for p in plan:
        if p["role"] != cur:
            cur = p["role"]
            print(f"  {p['label']}")
        if p["kind"] == "signature":
            print(f"    keus {p['rank']} - {p['voice_name']}")
        print(f"       {p['kind']:<10} {p['chars']:>4} tekens  \"{p['text'][:64]}"
              + ("...\"" if len(p["text"]) > 64 else "\""))

    print(f"\nTOTAL: {total} credits "
          f"({'fits' if total <= CREDIT_CEILING else 'EXCEEDS'} ceiling {CREDIT_CEILING})")

    # HARD GATE, not a warning. This was a warning once and it cost 2,470 credits:
    # the run went ahead on multilingual_v2 and produced 21 tagged clips whose
    # delivery cannot be trusted. Measured on those clips: an identical line with
    # and without [grieving] differed by +2.30 s on a 2.14 s baseline, so a non-v3
    # model does something large and unintended with brackets. A casting decision
    # made on that audio would be made on the wrong evidence, permanently.
    tagged = sum(1 for p in plan if "[" in p["text"])
    if tagged and not args.model.startswith("eleven_v3"):
        print(f"\nREFUSING: {tagged} of {len(plan)} clips carry §19.4 audio tags, but "
              f"model\n  '{args.model}' does not support them. Tags are the whole point "
              f"of the\n  emotional-extreme line. Use --model eleven_v3, or strip the "
              f"tags first.")
        return 6
    if args.dry_run:
        print("\ndry run - nothing generated, 0 credits spent.")
        return 0
    if total > CREDIT_CEILING:
        print("REFUSING: plan exceeds the Tier 0 ceiling.")
        return 1

    key, source = gen.get_api_key()
    if not key:
        print("No ElevenLabs key found.")
        return 1
    print(f"key source: {source} (never printed)")
    try:
        before = gen.require_usage_measurement(key, "the stage 2 casting test")
    except gen.UnmeasurableSpendError as e:
        print(e)
        return 3
    print(f"credit meter OK: {before['character_count']}/{before['character_limit']}")

    # Model probe: ~5 credits settles whether this model exists on the plan.
    # Cheaper than discovering it 50 clips in, and the cache key contains
    # modelId -- a mid-run fallback would split the cache across two models.
    probe = urllib.request.Request(
        f"https://api.elevenlabs.io/v1/text-to-speech/{plan[0]['voice_id']}"
        f"?output_format={OUTPUT_FORMAT}",
        data=json.dumps({"text": "Test.", "model_id": args.model,
                         "voice_settings": SETTINGS}).encode(), method="POST")
    probe.add_header("xi-api-key", key)
    probe.add_header("Content-Type", "application/json")
    try:
        with urllib.request.urlopen(probe, timeout=60) as r:
            r.read()
        print(f"model probe OK: '{args.model}' works on this plan.")
    except Exception as e:
        print(f"MODEL UNAVAILABLE: '{args.model}' -> "
              f"{str(e).replace(key, '***')[:160]}")
        print("  Nothing else generated. Pick a model that exists, or get the "
              "`models_read` scope and list them.")
        return 5

    OUT.mkdir(parents=True, exist_ok=True)
    man = json.loads(MANIFEST.read_text("utf-8")) if MANIFEST.is_file() else {"clips": {}}
    spent = 0
    for p in plan:
        dest = OUT / p["file"]
        prev = man["clips"].get(p["file"])
        if dest.is_file() and prev and prev.get("text") == p["text"] \
                and prev.get("model_id") == args.model:
            print(f"[cache] {p['file']}")
            continue
        url = (f"https://api.elevenlabs.io/v1/text-to-speech/{p['voice_id']}"
               f"?output_format={OUTPUT_FORMAT}")
        body = json.dumps({"text": p["text"], "model_id": args.model,
                           "voice_settings": SETTINGS}).encode()
        req = urllib.request.Request(url, data=body, method="POST")
        req.add_header("xi-api-key", key)
        req.add_header("Content-Type", "application/json")
        try:
            with urllib.request.urlopen(req, timeout=180) as r:
                audio = r.read()
        except Exception as e:
            print(f"[FAIL] {p['file']}: {str(e).replace(key, '***')[:140]}")
            continue
        dest.write_bytes(audio)
        spent += p["chars"]
        man["clips"][p["file"]] = {k: p[k] for k in
                                   ("role", "rank", "voice_name", "voice_id",
                                    "kind", "text", "chars")}
        man["clips"][p["file"]]["model_id"] = args.model
        MANIFEST.write_text(json.dumps(man, indent=1, ensure_ascii=False), "utf-8")
        print(f"[ok]    {p['file']:<52} {p['chars']:>4}")

    # The account counter is eventually consistent: measured 2026-07-31, a real
    # 45-character generation read back as a delta of 0, and 341 credits landed
    # minutes later. So the before/after delta is a LAGGING CROSS-CHECK, not the
    # primary number. For TTS the billable quantity is exactly the characters we
    # sent (1 credit = 1 character), and that we know precisely.
    import time
    time.sleep(20)                       # let the counter settle before reading
    after, err = gen.get_usage(key)
    print(f"\nCHARACTERS SENT (authoritative for TTS): {spent}")
    if after is None:
        print(f"account read-back failed: {err}")
        print("-> log the characters-sent figure, and note the read-back failed.")
        return 4
    delta = (after["character_count"] or 0) - (before["character_count"] or 0)
    print(f"account read-back: {after['character_count']}/{after['character_limit']} "
          f"(delta {delta})")
    if abs(delta - spent) > max(50, spent * 0.1):
        print(f"  NOTE: delta {delta} differs from characters sent {spent}. Usually "
              f"counter lag;\n  re-read the account in a few minutes before closing "
              f"the ledger entry.")
    print("-> write CHARACTERS SENT into phase0/VOICE_LEDGER.md, and the account "
          "reading as the cross-check.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
