# ECLIPSE — MACHINE SETUP (toolchain install)
*Everything a fresh Windows PC needs to build & continue ECLIPSE. Written so Claude Code on the new machine can execute it (winget for CLI installs; kimi/WebBridge for the browser-only steps). Mirrors the toolchain proven on the original dev box.*

> After this, follow `HANDOFF.md` §6 (build/run) and §5 (what to continue).

---

## 0. What gets installed (and why)

| Tool | Why | Install method |
|---|---|---|
| **Git** (+ Git LFS, bundled) | version control / clone the repo | winget (CLI) |
| **GitHub CLI (`gh`)** | auth + clone the private repo | winget (CLI) |
| **Node.js LTS + npm** | runs Claude Code | winget (CLI) |
| **Claude Code** | the AI dev agent itself | npm (CLI) |
| **Python 3.x** | `Tools/` scripts (event-catalog check etc.) | winget (CLI) |
| **Visual Studio 2022** + "Game development with C++" | the C++ compiler UE builds with | winget (CLI, with workload) |
| **Epic Games Launcher** | installs & manages Unreal Engine | winget, then **GUI/kimi** |
| **Unreal Engine 5.8** | the engine (huge download, ~40–60 GB) | **through the Launcher (GUI/kimi)** |
| **WebBridge (kimi)** | lets Claude drive the browser for the GUI-only steps | replicate from the original box |

Versions proven on the original dev box (targets, newer is fine): Git 2.55 · gh 2.96 · Node 24 · npm 11 · Python 3.14 · Claude Code 2.1.x · VS Community 2022 17.14 · UE 5.8.

---

## 1. CLI tools via winget (Claude can run these directly)

```powershell
winget install --id Git.Git -e --source winget --accept-source-agreements --accept-package-agreements
winget install --id GitHub.cli -e --source winget --accept-package-agreements
winget install --id OpenJS.NodeJS.LTS -e --source winget --accept-package-agreements
winget install --id Python.Python.3.12 -e --source winget --accept-package-agreements
```

Then **open a new terminal** (so PATH refreshes) and install Claude Code:

```powershell
npm install -g @anthropic-ai/claude-code
```

Verify:

```powershell
git --version; gh --version; node --version; npm --version; python --version; claude --version
```

## 2. Visual Studio 2022 + "Game development with C++"

The single most important workload for UE is **NativeGame** (Game development with C++); it pulls MSVC + the Windows SDK. `NativeDesktop` is a useful companion.

```powershell
winget install --id Microsoft.VisualStudio.2022.Community -e --accept-package-agreements `
  --override "--quiet --wait --norestart --add Microsoft.VisualStudio.Workload.NativeGame --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended"
```

If VS is already installed, add the workload instead:

```powershell
$vsInstaller = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\setup.exe"
& $vsInstaller modify --installPath "C:\Program Files\Microsoft Visual Studio\2022\Community" `
  --add Microsoft.VisualStudio.Workload.NativeGame --add Microsoft.VisualStudio.Workload.NativeDesktop --quiet --norestart
```

Verify the workload:

```powershell
& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest `
  -requires Microsoft.VisualStudio.Workload.NativeGame -property displayName
```

## 3. Epic Games Launcher + Unreal Engine 5.8  (browser/GUI — kimi helps here)

The engine cannot be fully scripted; it's installed *through* the Launcher, which needs an Epic account login (browser) and a large download. **This is where kimi/WebBridge earns its place** — it can drive the Epic login and the Library "Install Engine 5.8" flow.

```powershell
# Install the launcher itself (CLI):
winget install --id EpicGames.EpicGamesLauncher -e --accept-package-agreements
```

Then (GUI, drive with kimi if headless):
1. Launch **Epic Games Launcher** → sign in with the Epic account.
2. **Unreal Engine** tab → **Library** → **+** → pick **5.8.x** → **Install** (choose a drive with ~80 GB free; Starter Content optional; on a strong PC keep DX12).
3. After install, note the path, typically `C:\Program Files\Epic Games\UE_5.8`, and set it:

```powershell
[Environment]::SetEnvironmentVariable("UE_ROOT", "C:\Program Files\Epic Games\UE_5.8", "User")
```

Verify:

```powershell
Test-Path "$env:UE_ROOT\Engine\Build\BatchFiles\Build.bat"
Test-Path "$env:UE_ROOT\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
```

## 4. WebBridge (kimi) — so Claude there can drive the browser too

WebBridge is a small local bridge that exposes a browser-automation endpoint (on the original box it listens on `http://127.0.0.1:10086/command`, config under `C:\Users\<user>\.kimi-webbridge\` — `bin/`, `logs/`, `identity.json`). The new machine needs the same bridge running for Claude to handle the Epic/GitHub/Claude-login browser steps.

**Replicate it from the original box:** copy the `.kimi-webbridge` setup (or re-run whatever installer set it up here) and start the bridge, then confirm:

```powershell
Invoke-RestMethod -Uri "http://127.0.0.1:10086/health" -TimeoutSec 5   # or check the bridge's own status route
```

> If WebBridge isn't available on the new machine, the three browser steps (Epic login + engine install, `gh auth login`, `claude` login) can each be done by hand once — everything else is CLI and Claude-drivable.

## 5. Logins (one-time, browser)

- **Claude Code:** run `claude` once → it opens a login in the browser (kimi can complete it).
- **GitHub:** `gh auth login --hostname github.com --git-protocol https --web` → authorize in the browser. (See HANDOFF.md §7 for the exact push flow.)
- **Epic:** done in step 3.

## 6. Get the project & build

Once the repo is on GitHub (see HANDOFF.md §7):

```powershell
git clone https://github.com/<owner>/<repo>.git C:\Dev\ECLIPSE_GDD
cd C:\Dev\ECLIPSE_GDD\Eclipse
& "$env:UE_ROOT\Engine\Build\BatchFiles\Build.bat" EclipseEditor Win64 Development -project="$PWD\Eclipse.uproject" -WaitMutex
```

Then open `Eclipse.uproject` in the editor and **Play**. Continue from `HANDOFF.md` §5.

---

## 7. Hand this machine's Claude a one-line brief

> *"Read `C:\Dev\ECLIPSE_GDD\SETUP.md` and install the full toolchain (use kimi/WebBridge for the Epic/GitHub/Claude browser steps). Then read `HANDOFF.md` and continue Phase 1 at §5, staying inside the ACTIVE_MILESTONE. Fidelity/graphics work starts at Phase 2 per `15_visual_quality_charter.md`."*
