# ECLIPSE — PROJECT HANDOFF & PROGRESS
*Single "start here" page for a new machine or a new Claude session. Last updated: 2026-07-21.*

> **Read order for whoever picks this up:** this file → `DOCUMENTATION_README.md` → `00_INDEX.md` → `13_roadmap.md` (ACTIVE_MILESTONE) → `14_ai_dev_instructions.md`. Then the phase specs in `phase0/specs/`.

---

## 1. What this project is

**ECLIPSE: Rise of the Resistance** — a single-player third-person Action-Strategy RPG built in **Unreal Engine 5.8 (C++)**. Full design lives in the bible (docs `00`–`15`). It is a ~4.5-year, 6-phase game (roadmap `13`). We are in **Phase 1 — Prototype "The Loop"** (deliberately ugly graybox; prove the risks, not the graphics).

This is **not** a web app and does **not** deploy to Vercel — it is a native UE game that runs in the Unreal Editor / a packaged build.

---

## 2. Where everything lives (one repo)

As of the consolidation commit, the **design bible and the UE game are in ONE git repo** rooted at `C:\Dev\ECLIPSE_GDD\`, so a single `git clone` gets everything (docs + code + full history).

| What | Path (relative to repo root) |
|---|---|
| **Design bible** (docs 00–15, `phase0/` specs, this file, SETUP.md) | repo root |
| **The UE game** (code + content) | `Eclipse/` |
| **Save & push safety net** | `push-all.bat` / `push-all.ps1` |

Repo is local-only until pushed (see §7). Save-and-push any time with **`push-all.bat`** (double-click) — commits everything and pushes so the other machine can `git pull`.

**Progress is readable in two places:**
1. `git log` inside `Eclipse\` — each commit is one spec landed (the real build record).
2. `13_roadmap.md` → `ACTIVE_MILESTONE` block — the single source of "where we are."
3. This file's §4 (status) and §5 (what's next).

---

## 3. Hardware note (why a stronger machine matters)

This dev box is a **2017 laptop: NVIDIA GTX 1050 (4 GB), i7-7700HQ, 32 GB RAM**. It is a fine **graybox / logic / CI** box (Phase 1 runs on it), but it is **not** the fidelity target: no hardware ray tracing (no RT cores), Lumen only in reduced software mode, 4 GB VRAM can't stream 4K/8K. The **visual quality work (Phase 2+) belongs on a stronger RTX-class machine** — see the new `15_visual_quality_charter.md` (§15.2) for the dev-box-vs-target split. So handing the high-fidelity phases to a stronger PC is exactly the intended plan.

---

## 4. Current status (2026-07-21)

**Phase 1 — all 8 feature specs have landed and the tree is green.**

- Build: `EclipseEditor Win64 Development` **Succeeded** (UE 5.8).
- Automation tests: **28/28 pass** (`Automation RunTests Eclipse`, headless `-nullrhi`).
- Data validation: **clean** (`EclipseValidateData` commandlet, 0 errors).
- Event catalog: **19/19 in sync** (`Tools/check_event_catalog.py`).

**Systems in place (headless-proven):** event bus · campaign state + transaction API + save v0 · deterministic economy ledger · 6-node strategy mini-map · mission runtime + debrief consequences · squad of 2 with ordered actions and *reasoned refusals* (never-silent) · roster + permadeath + memorial stub · menu base hub + preparation flow · playable graybox layer (character/GAS health, hitscan combat, enemy AI, code-built district).

The full loop is **proven headless** (test `Eclipse.Base.Prep.FullCircleSmoke`: advance day → select → intel spend → launch → win → consequences → second loop) and is now **wired live** — playable in PIE with no console commands (menu base → launch → graybox mission with squad orders → extraction → debrief → base).

---

## 5. What's next

The **live playable loop is done** (commit `[Loop] Wire the live playable loop`): boot → menu base → launch → graybox mission with squad orders → extraction/lose → debrief → base, no console commands. The remaining Phase-1 step is the **gate question** — a human plays it and answers *"do testers voluntarily play a second loop?"* (13.2). That's the owner's call, not an automated check.

**Open (small, non-blocking) polish:**
- Move-order **stance stub** (ready/aggressive) — an in-scope SPEC-P1-06 bullet, still missing (no gate impact).
- Wire the hardcoded **squad tuning constants** (cover ring radius/samples, acceptance radii) into `DA_SquadTuning` (`14.2` compliance).
- **Audio** (Part 16, forward infra): runtime mp3→USoundWave playback, auto-assign to Dialogue DataAssets (WITH_EDITOR import), an editor bulk-gen UI, and the Music/SFX endpoints. Set `ELEVENLABS_API_KEY` (env) to generate; **rotate the key** that was shared in plaintext.

**Non-blocking carryover** (roadmap-allowed to lag): CI self-hosted runner, 10 concept-art pieces, 5 feel-reference clips. **Fidelity/art starts at Phase 2** (graybox rule); Parts 15–17 are the target for that.

---

## 6. How to build, test, and run (any Windows machine with the toolchain)

Prereqs: **UE 5.8** (Epic Launcher) + **Visual Studio 2022** with the "Game development with C++" workload. Set `UE_ROOT` to the engine, e.g. `C:\Program Files\Epic Games\UE_5.8`.

```powershell
# 1. Generate VS project files (right-click Eclipse.uproject → Generate, or:)
& "$env:UE_ROOT\Engine\Build\BatchFiles\Build.bat" EclipseEditor Win64 Development `
  -project="<repo>\Eclipse\Eclipse.uproject" -WaitMutex

# 2. Run the tests (same as CI)
& "$env:UE_ROOT\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "<repo>\Eclipse\Eclipse.uproject" -ExecCmds="Automation RunTests Eclipse; quit" `
  -unattended -nopause -nosplash -nullrhi -log -ReportExportPath="<repo>\Eclipse\Saved\Automation"

# 3. Data validation
& "$env:UE_ROOT\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "<repo>\Eclipse\Eclipse.uproject" -run=EclipseValidateData -unattended -nopause -nosplash

# 4. Play: open Eclipse.uproject in the editor → Play In Editor.
```

CI definition (needs a self-hosted Windows+UE5 runner): `Eclipse\.github\workflows\ci.yml`.

---

## 7. How another (stronger) computer takes over

The clean mechanism is **git**. The bible + code are already **one repo** (§2), local-only until pushed.

**One-time setup — private GitHub repo:**
1. `gh auth login --hostname github.com --git-protocol https --web` → authorize in the browser (kimi can drive it).
2. From the repo root: `gh repo create eclipse-rise --private --source . --remote origin --push` (creates the private repo AND pushes everything in one go).

**From then on — keep it synced (this is the auto-push command):**
- Double-click **`push-all.bat`** (or run `push-all.ps1`). It commits all current work and `git push`es. Use it before switching machines or whenever Claude nears a session limit.

**On the strong PC:** install the toolchain (`SETUP.md`), `git clone https://github.com/<owner>/eclipse-rise.git C:\Dev\ECLIPSE_GDD`, build (§6), read this file + `ACTIVE_MILESTONE`, continue at §5. **Full day-one migration guide — with UML/BPMN diagrams, the Max/Fable login, the secrets vault, and a paste-ready Fable prompt — is in [`MIGRATION_TO_STRONG_PC.md`](MIGRATION_TO_STRONG_PC.md).**

**Alternatives if you don't want GitHub yet:** `git bundle create eclipse.bundle --all` (one file, full history) transferred by USB/drive/server; or push to the existing Rocadelo server the project already came from.

**What the strong PC's Claude should be told:** *"Read `C:\...\ECLIPSE_GDD\HANDOFF.md`, then continue Phase 1 at §5. Stay inside the ACTIVE_MILESTONE. Fidelity work starts at Phase 2 per `15_visual_quality_charter.md`."*

---

## 8. Handy pointers

- Governance / coding rules: `14_ai_dev_instructions.md` (naming `AEclipse*/UEclipse*/FEclipse*/EEclipse*`; commits `[System] Verb summary (GDD ref)`; event-bus-only cross-system; tunables in DataAssets; PLACEHOLDER tags).
- Event registry: `Eclipse\Docs\EventCatalog.md` (keep in sync with code, checked by `Tools/check_event_catalog.py`).
- Save contract: `12_technical_design.md` §12.3.
- Performance budgets: `12_technical_design.md` §12.4.
- Visual target + hardware reality: `15_visual_quality_charter.md`.
