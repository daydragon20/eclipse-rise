# Gates — what each one proves, and what it does not

A gate is only worth running if you know what a pass actually buys you. Treating
a green G4 as proof that the game *feels* right is how a loop converges on
something that compiles, tests clean, and plays terribly.

Paths assumed throughout:

- Repo root: `C:\Dev\ECLIPSE_GDD` (remote: `daydragon20/eclipse-rise`)
- UE project: `C:\Dev\ECLIPSE_GDD\Eclipse\Eclipse.uproject`
- Engine: `C:\Program Files\Epic Games\UE_5.8` (override with `$env:UE_ROOT`)
- Shell: Windows PowerShell 5.1 — no `pwsh`, no `&&`, no ternary

---

## Known repo defect — read before trusting CI

`Eclipse/.github/workflows/ci.yml` is **not at the repository root**, and GitHub
Actions only reads `.github/workflows/` from the root of a repo. The workflow
therefore never runs on push or PR. It also assumes the checkout root *is* the
Eclipse folder (`$env:GITHUB_WORKSPACE\Eclipse.uproject`), which does not match
the actual layout where `Eclipse/` is a subdirectory.

Practical consequence for this loop: **CI is not a gate right now.** Local gate
runs are the only real verification, so do not write "CI green" into a Definition
of Done check until this is fixed. Fixing it means moving the workflow to
`.github/workflows/ci.yml` and correcting the project paths to `Eclipse/…`. That
is a small, self-contained task worth doing early, because every other gate
below gets cheaper once it runs automatically.

---

## G1 — Build

```powershell
& "$env:UE_ROOT\Engine\Build\BatchFiles\Build.bat" EclipseEditor Win64 Development `
  -Project="C:\Dev\ECLIPSE_GDD\Eclipse\Eclipse.uproject" -WaitMutex -FromMsBuild
```

Proves the C++ compiles and links against UE 5.8. Nothing else. A green build
says nothing about behaviour, and Blueprint errors do not surface here at all —
they surface in G2/G4 or not until runtime.

Failure reading: compiler errors are usually literal and local. Link errors
usually mean a missing module dependency in `Eclipse.Build.cs`.

---

## G2 — Data validation

```powershell
& "$env:UE_ROOT\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "C:\Dev\ECLIPSE_GDD\Eclipse\Eclipse.uproject" `
  -run=EclipseValidateData -unattended -nopause -nosplash
```

Proves DataAssets load and satisfy the project's validation rules — GDD 12.2
rule 3 exists because balance data is where a designer-editable project rots
first. Since GDD 14.3 rule 5 forbids hard-depending on content, a *missing*
asset should log a warning and default gracefully rather than fail here; a G2
failure usually means malformed rather than absent.

---

## G3 — Event catalog

```powershell
python "C:\Dev\ECLIPSE_GDD\Eclipse\Tools\check_event_catalog.py"
```

Proves every event-bus tag is documented in `Eclipse/Docs/EventCatalog.md`
(GDD 14.2). Cheap and fast — seconds. Since the event bus is the project's entire
decoupling strategy (GDD 14.3 rule 1), an undocumented tag is an invisible
dependency between subsystems, which is exactly the failure this architecture was
designed to prevent.

Run this before the expensive gates when a change adds tags — it is the one gate
that costs nothing and catches a structural mistake.

---

## G4 — Automation tests

```powershell
& "$env:UE_ROOT\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "C:\Dev\ECLIPSE_GDD\Eclipse\Eclipse.uproject" `
  -ExecCmds="Automation RunTests Eclipse; quit" -unattended -nopause -nosplash -nullrhi `
  -log -ReportExportPath="C:\Dev\ECLIPSE_GDD\Eclipse\Saved\Automation"
```

Proves the pure-logic cores still behave: there are currently spec files for
audio, campaign, economy, event bus, missions, prep, roster, squad and strategy
under `Eclipse/Source/Eclipse/Tests/`.

Two things worth knowing:

- `-nullrhi` means **no rendering at all**. G4 can never tell you anything about
  how the game looks or performs on screen. That is by design — it makes the gate
  fast and headless-safe (GDD 14.3 rule 2) — but it is also why G5–G7 exist.
- The process exit code is not a reliable pass/fail signal for UE automation.
  Read `Saved/Automation/index.json` for the real per-test verdicts. `run_gates.ps1`
  parses it when present and falls back to the exit code with an explicit warning
  when it is absent, because a silent fallback would let a failing suite read as
  a pass.

---

## G5 — Feel probes *(does not exist yet)*

The locked numbers in `phase0/graybox_feel_targets.md` are the contract this gate
would enforce: walk/run/sprint 1.8 / 4.2 / 6.5 m/s, input-to-motion latency
≤ 100 ms, player TTK ≈ 0.6 s, enemy TTK ≈ 2.5 s, head damage ×2.5, vault ≤ 2.4 m,
cover dash ≤ 8 m, dodge 0.5 s, order acknowledgment ≤ 1 s.

Build it in two layers, cheapest first:

1. **Data-level assertions as ordinary Automation specs** (headless, `-nullrhi`,
   joins G4). Read the movement and weapon DataAssets and assert the authored
   numbers match the locked doc. This catches the most common regression — someone
   retunes a value and the doc silently drifts — for almost no cost.
2. **Functional tests in the graybox map** (`Content/Maps/GrayboxDistrict.umap`,
   needs a real RHI). Script an encounter, measure elapsed time to kill, measure
   input-to-motion latency, assert against the bands.

Layer 1 is a couple of hours and pays immediately. Layer 2 is the real gate but
needs the map to be playable end-to-end first — check that before promising it.

---

## G6 — Performance *(does not exist yet)*

GDD 12.4 budgets: 60 fps at 1440p high on RTX 3070-class, ≤ 40 full-fidelity AI
agents in the combat bubble (+≤ 400 Mass crowds), ≤ 2 ms game thread for the
strategic tick, 1.5 GB streaming per planet cell, ≤ 10 s hybrid-battle front swap.

The automatable route is the CSV profiler rather than Unreal Insights: it emits a
parseable CSV instead of a trace you have to open by hand, which is what makes it
usable inside an unattended loop. Drive it with `-ExecCmds="CsvProfile start"` on
a fixed scene and seed, then parse the resulting CSV in `Saved/Profiling/` for the
game-thread and GPU columns.

Determinism matters more than precision here. A budget check that swings 30% run
to run will fail randomly, and a loop that hits random failures will "fix" things
that were never broken. Fix the map, the seed, and the camera path before trusting
any number.

Note GDD 14.6 Skill 7: measure before changing, and never trade squad-AI quality
for frames without design sign-off.

---

## G7 — Visual *(does not exist yet)*

The bar lives in `phase0/art_style_bible.md` and the World Builder checklist in
GDD 14.6 Skill 5: one-screenshot identity, landmark orientation, all three
occupation states dressed.

Mechanically this needs a real RHI — `-nullrhi` cannot produce an image. UE's
automation screenshot system (`FAutomationScreenshotOptions`) with a fixed camera,
fixed time of day and fixed seed is the sane basis; Gauntlet (present at
`Engine/Source/Programs/AutomationTool/Gauntlet`, driven via `RunUAT.bat`) can
orchestrate it, but there is no Gauntlet test class in this project yet, so treat
any Gauntlet command as unverified until one exists and you have run it.

Be honest about what automated visual comparison can and cannot judge. Pixel
diffing catches *regressions* — something moved, went black, lost a material. It
cannot tell you whether a scene has identity or reads well; that is a judgment
call needing a human or an explicit vision-model review pass with the art bible in
context. Do not let a green pixel diff stand in for "it looks good", because those
are different claims and only one of them is being tested.
