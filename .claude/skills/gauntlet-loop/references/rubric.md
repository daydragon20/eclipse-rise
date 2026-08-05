# Building the acceptance contract

The loop converges on whatever you measure. This is the part that decides whether
an unattended run produces the game in the Design Bible or something that merely
passes tests.

## Turning intent into a check

Design language is deliberately evocative — "weighty, deliberate, readable",
"the smartest person in a deadly room". That language is doing real work and
should not be flattened away, but it cannot be a gate. The move is to keep the
intent as the *reason* and derive a check from it.

**Example 1 — a soft target with a locked number available**

Intent: "cover use should be self-taught within two encounters" (graybox acceptance,
feel targets §3).
Check: enemy TTK vs. exposed player ≈ 2.5 s, measured in a scripted encounter with
a fixed seed; player standing in the open dies inside that band.
Why this works: the doc already reasoned from the feeling to the number. Reuse its
reasoning rather than inventing a second one.

**Example 2 — a soft target with no number**

Intent: "one-screenshot identity" for a planet (GDD 14.6 Skill 5).
Check: not automatable. Record it as an explicit *unverified* item with a named
reviewer, rather than inventing a proxy metric.
Why this works: a fabricated proxy ("at least 4 distinct materials in frame")
would pass while the scene stays generic, which is worse than an honest gap
because it looks like coverage.

**Example 3 — a structural rule**

Intent: subsystems must not reach into each other (GDD 14.3 rule 1).
Check: grep the diff for direct cross-subsystem member access; new tags present in
`Docs/EventCatalog.md` (G3).
Why this works: architecture rules are usually mechanically checkable even when
they sound abstract, and mechanising them is what stops slow erosion.

The general shape: **intent → the reason it matters → the smallest observation
that would be different if it were violated.** If nothing observable would differ,
you are not looking at a requirement, you are looking at a preference — say so.

## The locked numbers

`phase0/graybox_feel_targets.md` is marked **LOCKED**. It is the design authority
that in-project DataAssets must match, and the loop may not change it. Changing a
value follows the change-management flow (problem → proposal → affected systems →
doc updated in the same commit) and needs a human.

| Metric | Target |
|---|---|
| Walk / Run / Sprint | 1.8 / 4.2 / 6.5 m/s |
| Input-to-motion latency | ≤ 100 ms, all verbs |
| Vault / mantle | ≤ 2.4 m |
| Cover-to-cover dash | ≤ 8 m |
| Dodge | 0.5 s, i-frames vs. melee only |
| Camera | over-shoulder, FOV 90; +15% pullback in command mode |
| Player TTK vs. basic enemy | ≈ 0.6 s well-aimed |
| Enemy TTK vs. exposed player | ≈ 2.5 s |
| Locational damage | head ×2.5; limbs cripple |
| Ballistics | hitscan < 50 m, projectile ≥ 50 m |
| Order acknowledgment | ≤ 1 s (movement start or spoken refusal) |
| Difficulty | margins and information change — never HP scaling |

Performance budgets live in GDD 12.4 and are equally binding: 60 fps at 1440p high
on RTX 3070-class, ≤ 40 full-fidelity AI agents (+≤ 400 Mass crowds), ≤ 2 ms game
thread for the strategic tick, 1.5 GB streaming per planet cell, ≤ 10 s hybrid
battle front swap.

Two rules that look pedantic and are not:

- **A hardcoded gameplay constant is a defect** (GDD 14.2). If a check would pass
  only because a number is baked into C++, the check passed for the wrong reason.
- **Data must match the doc.** When they disagree, the doc is the authority and the
  data is the bug — never the other way around, or the locked contract silently
  becomes whatever was last typed.

## Contract template

Keep it short. A contract nobody reads is decoration.

```markdown
## Contract: <task>

GDD refs: <sections, quoted where the claim is specific>
Events consumed: <tags>       # GDD 14.3 rule 4 — before code
Events emitted: <tags>        # add to Docs/EventCatalog.md same commit

| # | Condition | Gate | Status |
|---|---|---|---|
| 1 | <measurable pass condition> | G4 | |
| 2 | <measurable pass condition> | G5-layer1 | |
| 3 | <judgment call> | none — human review | unverified |

Out of scope: <what this task deliberately does not do>
Blocked-on: <anything needing a human decision>
```

The `unverified` row is the important one. A contract with no unverified rows is
usually a contract where someone quietly downgraded the hard parts until
everything fit a gate — which is the same failure as weakening a gate, just moved
one step earlier where it is harder to notice.
