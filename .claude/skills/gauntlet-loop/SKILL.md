---
name: gauntlet-loop
description: Autonomous build-verify-judge-fix loop for the ECLIPSE Unreal Engine 5 project. Use this whenever work touches the Eclipse UE5 codebase (C:\Dev\ECLIPSE_GDD) — implementing a GDD spec, tuning combat or movement feel, adding a subsystem, fixing a failing gate, or any request phrased as "build X and make sure it actually works", "run it until it passes", "iterate on this until it's right", or "gauntlet loop". Also use when asked to verify existing Eclipse work against the Design Bible, or when someone wants an agent to keep working unattended on the game. Do not use for the Bolwerk browser game or other non-Eclipse projects.
---

# Gauntlet Loop

An agent that writes code and declares victory is grading its own homework. This
skill replaces that with a loop where every claim has to survive a gate that the
agent does not control: the compiler, the test runner, the validator, a number
from the Design Bible.

The loop:

**contract → build → verify → judge → fix → repeat until two clean rounds**

Two clean rounds, not one. A single clean pass often means the last fix broke
something the gates only catch on the next lap.

## Why the contract comes first

The loop's ceiling is set entirely by the rubric. A vague bar ("make combat feel
good") produces a loop that spins and congratulates itself. A measurable bar
("player TTK vs. basic enemy ~0.6 s, verified from a scripted encounter") produces
a loop that converges.

So before writing any code, produce a short acceptance contract:

1. **Which GDD sections does this touch?** Quote the specific claims. The Bible
   wins over the task prompt (GDD 14.1) — if the prompt contradicts it, say so
   and stop rather than silently picking one.
2. **What are the pass conditions, as numbers or observable states?** Pull them
   from the locked docs where they exist — `phase0/graybox_feel_targets.md`
   holds movement, gunplay and command-layer targets; GDD 12.4 holds performance
   budgets. See `references/rubric.md` for turning soft intent into a check.
3. **Which events does this consume and emit?** GDD 14.3 rule 4 requires this
   before code exists, and it is also what tells you which gates matter.
4. **Which gate proves each condition?** If a condition has no gate, either build
   the gate or write down honestly that this part is unverified. An unverifiable
   claim is not a pass.

Write the contract into the task's working notes. You will judge against it later,
and a contract you can rewrite mid-loop is not a contract.

## Build and verify

The gates form a ladder, cheapest and most decisive first. Stop at the first
failure — a broken build makes every downstream result meaningless, and running
the full ladder anyway just buys expensive noise.

| Gate | Proves | Cost |
|---|---|---|
| G1 Build | It compiles | ~min |
| G2 Data validation | No DataAsset is malformed or orphaned | ~min |
| G3 Event catalog | Every new tag is documented (GDD 14.2) | secs |
| G4 Automation tests | Pure-logic cores still behave | ~min |
| G5 Feel probes | The locked numbers still hold | needs harness |
| G6 Performance | GDD 12.4 budgets respected | needs harness |
| G7 Visual | It looks like the art bible says | needs harness |

**G1–G4 work today** — they are the same commands `Eclipse/.github/workflows/ci.yml`
already runs, so they are proven, not hopeful. Run them with:

```bash
powershell -NoProfile -File .claude/skills/gauntlet-loop/scripts/run_gates.ps1
```

Note that this machine has Windows PowerShell 5.1 only — no `pwsh`, no `&&`, no
ternary operators. Scripts here are written to that dialect on purpose.

**G5–G7 do not exist yet.** They need a one-time harness (Gauntlet tests, a
screenshot pass, an Insights capture). Do not pretend otherwise: if a task's
contract depends on G5–G7 and the harness is absent, the honest move is to build
the harness as part of the task, or to report the condition as unverified. Silently
skipping a gate and reporting success is the single worst thing this loop can do,
because it manufactures false confidence at scale. `references/gates.md` has the
exact commands, what each gate can and cannot prove, and how to build the missing
three.

## Judge

Read the results through the role lenses in GDD 14.6 — they exist precisely so a
change gets looked at from more than one angle. For any given change only two or
three are relevant; pick them deliberately rather than performing all seven:

- **Architecture Expert** — has veto. Does this respect GDD 14.3.1–6? What breaks
  at campaign hour 150?
- **Unreal Engine Programmer** — is there an engine-native solution you skipped?
  Tick discipline? DataAsset-driven?
- **AI Systems Engineer** — is the behaviour *readable* by the player? Does a
  failed order get spoken aloud?
- **Combat Designer** — TTK bands respected? Tuned in data, not code?
- **World Builder** — one-screenshot identity? All three occupation states dressed?
- **Narrative Designer** — both sides of the choice defensible? Names match the
  00_INDEX glossary?
- **Optimization Expert** — measured before changing? Optimized the top cost, not
  the interesting one?

Every verdict needs evidence attached: a log line, a measured number, a screenshot
path. "Looks correct" is not a verdict. If you cannot point at the thing that
convinced you, you are not convinced.

## Fix

Fix the top failure, not the most interesting one. Then re-enter the loop from
G1 — a fix invalidates every gate above it.

Three rules keep an unattended loop from destroying the project while it "succeeds":

**Never weaken a gate to make it pass.** Lowering a target, deleting an assertion,
loosening a tolerance, or adding an exception for the failing case converts a real
failure into a fake success — and the loop will happily do this forever because it
optimises for green. If a target genuinely looks wrong, stop and say so; changing
`graybox_feel_targets.md` requires the change-management flow and a human, because
it is marked LOCKED.

**Three strikes on one gate and you stop.** If the same gate fails three times
under three different fixes, your model of the cause is wrong. More attempts just
dig deeper. Report what you tried, what the failures actually said, and what you
now suspect.

**Never commit through a red gate.** Definition of Done (GDD 14.4) is spec + code +
data + tests + docs + CI green. Commit format is `[System] Verb summary (GDD ref)`,
one system per commit.

## Stopping

Stop and report when any of these is true — the loop ending is a result, not a
failure:

- Two consecutive rounds pass every gate in the contract. *(success)*
- A gate fails three times under different fixes. *(you are stuck)*
- A fix would require weakening a locked target or violating GDD 14.3. *(needs a human)*
- The task prompt turns out to contradict the Design Bible. *(needs a human)*
- The remaining token budget will not cover another full build-verify round.

The report should state, plainly: what passed with what evidence, what failed and
why, what is unverified because a gate does not exist, and what you deliberately
did not do. Nathan asks for proof rather than reassurance — show the commands and
their output, and if something went wrong, say it flatly and fix it rather than
letting it surface later.

## Reference files

- `references/gates.md` — exact commands per gate, what each proves, how to build G5–G7
- `references/rubric.md` — turning design intent into checkable conditions; the locked numbers
- `scripts/run_gates.ps1` — runs G1–G4 in order, stops at first failure, emits a JSON verdict
