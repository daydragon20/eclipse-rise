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

    # Slot bindings, per role: {"eclipse_fighter": {"A": {...}, "B": {...}}}
    bindings, unbound_slots = {}, {}
    if RESOLVED.is_file():
        _r = json.loads(RESOLVED.read_text("utf-8"))["rollen"]
        bindings = {rid: r.get("slot_binding", {}) for rid, r in _r.items()}
        unbound_slots = {rid: r.get("slots_unbound", []) for rid, r in _r.items()}

    # Rank-1 finalist per role, so "resolves" can be checked against an actual
    # voice id and not merely against the role existing.
    rank1 = {}
    if RESOLVED.is_file():
        for rid, r in json.loads(RESOLVED.read_text("utf-8"))["rollen"].items():
            top = [f for f in r.get("finalisten", []) if f.get("rang") == 1]
            if top:
                rank1[rid] = (top[0]["voice_id"], top[0]["stem"])

    unmapped, mapped_uncast, ready, known_uncast, unbound = [], [], [], [], []
    # voice_id -> [(speaker, lines, label)], to catch two characters sharing one
    # voice. See the collision block below for why this is a hard failure.
    by_voice = {}
    for key, n in usage.most_common():
        if key in mapping:
            entry = mapping[key]
            roles = entry.get("variants") or [entry["role"]]
            missing = [r for r in roles if r not in cast_roles]
            if missing:
                mapped_uncast.append((key, n, roles, missing))
                continue
            # A role having finalists is NOT the same as this speaker having a
            # voice. eclipse_fighter_a..._d are four people sharing one role, and
            # a slot with no bound voice_id generates nothing -- silently. This
            # check reported such keys as "ready" once; that was the bug.
            slot = entry.get("slot")
            if slot:
                bound = bindings.get(entry["role"], {}).get(slot)
                if not bound or not bound.get("voice_id"):
                    unbound.append((key, n, entry["role"], slot))
                    continue
                ready.append((key, n, [f"{entry['role']}:{slot}="
                                       f"{bound['stem']}"], []))
                by_voice.setdefault(bound["voice_id"], []).append(
                    (key, n, f"{entry['role']}:{slot}={bound['stem']}"))
            else:
                ready.append((key, n, roles, []))
                for r in roles:
                    if r in rank1:
                        vid, stem = rank1[r]
                        by_voice.setdefault(vid, []).append(
                            (key, n, f"{r}={stem}"))
        elif key in uncast:
            known_uncast.append((key, n))
        else:
            unmapped.append((key, n))

    # A voice id used by two different ROLES is not a near-miss, it is the same
    # performance twice: the cache key is hash(voiceId + text + emotion +
    # modelId), so two characters on one id are literally indistinguishable.
    # Resolving is not the same as resolving UNIQUELY, and this check reported
    # "CAST AND READY" for both halves of such a pair -- green on a corpus where
    # Mara and an Eclipse fighter shared one voice. Casting is permanent
    # (19.1 point 2), so a collision found after generation re-costs every line
    # of whichever character moves.
    #
    # Grouped by role label, not by speaker key: `threx` and `dahl_threx` are
    # two script keys for ONE character and are supposed to share a voice.
    # Counting keys flagged that as a collision; counting roles does not.
    collisions = {v: s for v, s in by_voice.items()
                  if len({x[2] for x in s}) > 1}

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

    if unbound:
        print(f"\nFAIL - {len(unbound)} speaker(s) map to a role but their SLOT has "
              f"no voice ({sum(x[1] for x in unbound)} lines). The role is cast; "
              f"these particular speakers are not:")
        for key, n, role, slot in unbound:
            need = unbound_slots.get(role, [])
            print(f"  {key:<24} {n:>4} lines -> {role} slot {slot} (unbound: "
                  f"{', '.join(need) or '?'})")
        print("\n  A bark register is not one character: the scripts address these "
              "as\n  separate people, and §18.5 wants them to sound like separate "
              "people.\n  Fix: the owner picks one more voice per unbound slot.")

    if unmapped:
        print(f"\nFAIL - {len(unmapped)} speaker(s) resolve to NOTHING. These would "
              f"generate silently as zero lines:")
        for key, n in unmapped:
            print(f"  {key:<24} {n:>4} lines")
        print("\nFix: add them to Eclipse/Content/Audio/VoiceKeyMap.json - under "
              "'map' if they have a casting role, under 'uncast' if they still "
              "need one from the owner.")

    if collisions:
        n_lines = sum(x[1] for s in collisions.values() for x in s)
        print(f"\nFAIL - {len(collisions)} voice id(s) are cast on more than one "
              f"role ({n_lines} lines). These roles would be the SAME voice:")
        for vid, sp in collisions.items():
            stem = sp[0][2].split("=")[-1]
            per_role = {}
            for k, n, label in sp:
                per_role.setdefault(label, []).append((k, n))
            print(f"  {stem} ({vid}):")
            for label, keys in per_role.items():
                who = ", ".join(f"{k} [{n} lines]" for k, n in keys)
                print(f"    {label:<26} <- {who}")
        print("\n  Two characters on one voice id are not similar, they are\n"
              "  identical - same id, same model, same cache key. Casting is\n"
              "  permanent (19.1 point 2), so fixing this after generation\n"
              "  re-costs every line of whichever character moves.\n"
              "  Fix: the owner assigns the reserve voice to one of each pair\n"
              "  (owner question O-13), then re-run.")

    if unmapped or unbound or collisions:
        return 1

    print("\nOK: every speaker resolves to a role or is a declared uncast speaker,\n"
          "and no two speakers share a voice id.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
