#!/usr/bin/env python3
"""CI check: every `voice:` key used in Content/Script must resolve to a cast
voice, or be a known-uncast speaker.

Why this exists. EclipseGenerateVoicesCommandlet.cpp iterates over voice
*assets*, not over script keys. A voice asset with an empty ElevenLabsVoiceId is
logged at **Display** level and skipped (line ~312, "a data gap to report, not
an error"), and a script key with no asset at all is never seen by any counter
in the run summary. So an unresolvable speaker does not crash and does not fall
back to a default voice -- it silently produces nothing, and the summary line
looks healthy. That is the expensive kind of failure: you find it after the
sprint, when the credits are gone.

This check is pure text: no engine, no network, no credits.

Run:  python Eclipse/Tools/check_voice_resolves.py
Exit: 0 = every key resolves or is a declared uncast speaker
      1 = at least one key resolves to nothing
"""
import json
import re
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
SCRIPTS = ROOT / "Eclipse" / "Content" / "Script"
KEYMAP = ROOT / "Eclipse" / "Content" / "Audio" / "VoiceKeyMap.json"
RESOLVED = ROOT / "phase0" / "CASTING_RESOLVED.json"
VOICE_LINE = re.compile(r"^\s*voice:\s*([\w.]+)\s*$", re.M)


def main() -> int:
    if not SCRIPTS.is_dir():
        print("No Content/Script yet - nothing to check.")
        return 0

    usage = Counter()
    for y in SCRIPTS.rglob("*.yaml"):
        usage.update(VOICE_LINE.findall(y.read_text(encoding="utf-8", errors="replace")))
    if not usage:
        print("No `voice:` keys found in Content/Script.")
        return 0

    km = json.loads(KEYMAP.read_text("utf-8"))
    mapping, uncast = km["map"], km["uncast"]
    cast_roles = set()
    if RESOLVED.is_file():
        cast_roles = set(json.loads(RESOLVED.read_text("utf-8"))["rollen"])

    unmapped, mapped_uncast, ready, known_uncast = [], [], [], []
    for key, n in usage.most_common():
        if key in mapping:
            entry = mapping[key]
            roles = entry.get("variants") or [entry["role"]]
            missing = [r for r in roles if r not in cast_roles]
            (mapped_uncast if missing else ready).append((key, n, roles, missing))
        elif key in uncast:
            known_uncast.append((key, n))
        else:
            unmapped.append((key, n))

    total = sum(usage.values())
    print(f"{len(usage)} distinct speakers over {total} script lines "
          f"in {len(list(SCRIPTS.rglob('*.yaml')))} scenes.\n")

    if ready:
        print(f"CAST AND READY ({sum(x[1] for x in ready)} lines):")
        for key, n, roles, _ in ready:
            print(f"  {key:<24} {n:>4} lines -> {'+'.join(roles)}")
    if mapped_uncast:
        print(f"\nMAPPED BUT NOT YET CAST ({sum(x[1] for x in mapped_uncast)} lines) "
              f"- waiting on the owner's pick:")
        for key, n, roles, missing in mapped_uncast:
            print(f"  {key:<24} {n:>4} lines -> {'+'.join(roles)} "
                  f"(missing: {', '.join(missing)})")
    if known_uncast:
        print(f"\nNO ROLE IN THE CASTING TABLE ({sum(x[1] for x in known_uncast)} lines) "
              f"- declared in VoiceKeyMap.json 'uncast', still ungeneratable:")
        for key, n in known_uncast:
            print(f"  {key:<24} {n:>4} lines   {uncast[key]}")

    if unmapped:
        print(f"\nFAIL - {len(unmapped)} speaker(s) resolve to NOTHING. These would "
              f"generate silently as zero lines:")
        for key, n in unmapped:
            print(f"  {key:<24} {n:>4} lines")
        print("\nFix: add them to Eclipse/Content/Audio/VoiceKeyMap.json - under "
              "'map' if they have a casting role, under 'uncast' if they still "
              "need one from the owner.")
        return 1

    print("\nOK: every speaker resolves to a role or is a declared uncast speaker.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
