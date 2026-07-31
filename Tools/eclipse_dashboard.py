#!/usr/bin/env python3
"""
eclipse_dashboard.py — de LIVE dashboardserver van ECLIPSE.

Waarom dit bestaat
------------------
De oude opzet was `python -m http.server` plus een PowerShell-script dat af en
toe `progress_auto.js` genereerde. Gevolg: de pagina was alleen vers als er net
een chatsessie had gedraaid, en de server viel om zodra die sessie eindigde.

Deze server draait zelfstandig, scant de repo in een achtergrondthread en
serveert alles als JSON op /api/state. De pagina pollt dat, dus hij is live
zonder dat er ook maar iets anders open hoeft te staan.

Draaien
-------
    python Tools/eclipse_dashboard.py
    start START_DASHBOARD.bat          (Windows, blijft draaien)

Daarna: http://127.0.0.1:8377/

Endpoints
---------
    /                 het dashboard
    /api/state        alle live data als JSON
    /api/doc?path=..  ruwe inhoud van een markdown-bestand (repo-relatief)
    /api/agent?id=..  laatste berichten uit een agent-transcript
    /shots/<naam>     screenshot-bestand
    /<bestand>        statische bestanden uit de repo-root
"""

from __future__ import annotations

import html
import json
import mimetypes
import os
import re
import subprocess
import threading
import time
import urllib.parse
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
PORT = int(os.environ.get("ECLIPSE_DASH_PORT", "8377"))
SCAN_INTERVAL = 5.0          # seconden tussen achtergrondscans
TRANSCRIPT_DIRS = [
    Path(r"C:\Dev\ECLIPSE_SECRETS\fable-config\projects\C--Dev-ECLIPSE-GDD"),
    Path.home() / ".claude" / "projects" / "C--Dev-ECLIPSE-GDD",
]
SHOTS_DIR = REPO / "Eclipse" / "Saved" / "Screenshots" / "WindowsEditor"
VOICE_BUDGET = 131_000

_state_lock = threading.Lock()
_state: dict = {"booting": True}


# ----------------------------------------------------------------- helpers --

def now_str() -> str:
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")


def ago(ts: float) -> str:
    """Menselijke 'hoe lang geleden'."""
    d = max(0.0, time.time() - ts)
    if d < 60:
        return f"{int(d)}s geleden"
    if d < 3600:
        return f"{int(d // 60)} min geleden"
    if d < 86400:
        return f"{int(d // 3600)} uur geleden"
    return f"{int(d // 86400)} dagen geleden"


def run_git(*args: str) -> str:
    try:
        out = subprocess.run(
            ["git", *args],
            cwd=REPO,
            capture_output=True,
            text=True,
            timeout=15,
            encoding="utf-8",
            errors="replace",
        )
        return out.stdout.strip()
    except Exception:
        return ""


def tail_bytes(path: Path, nbytes: int) -> str:
    """Laatste nbytes van een bestand als tekst (voor grote JSONL-transcripten)."""
    try:
        size = path.stat().st_size
        with path.open("rb") as fh:
            if size > nbytes:
                fh.seek(size - nbytes)
                raw = fh.read()
                raw = raw.split(b"\n", 1)[1] if b"\n" in raw else raw
            else:
                raw = fh.read()
        return raw.decode("utf-8", errors="replace")
    except Exception:
        return ""


# ------------------------------------------------------------------ git --

def scan_git() -> dict:
    log = run_git("log", "-25", "--pretty=format:%h|%ad|%s", "--date=format:%d-%m %H:%M")
    commits = []
    for line in log.splitlines():
        parts = line.split("|", 2)
        if len(parts) == 3:
            commits.append({"hash": parts[0], "date": parts[1], "msg": parts[2]})
    status = run_git("status", "--porcelain")
    dirty = len([l for l in status.splitlines() if l.strip()])
    branch = run_git("rev-parse", "--abbrev-ref", "HEAD") or "?"
    return {"commits": commits, "dirty": dirty, "branch": branch}


# -------------------------------------------------------------- agents --

def _extract_entry(obj: dict) -> dict | None:
    """Haalt uit een transcript-regel een toonbaar bericht."""
    etype = obj.get("type")
    if etype not in ("user", "assistant"):
        return None
    msg = obj.get("message") or {}
    content = msg.get("content")
    text_parts: list[str] = []
    tools: list[str] = []

    if isinstance(content, str):
        text_parts.append(content)
    elif isinstance(content, list):
        for block in content:
            if not isinstance(block, dict):
                continue
            btype = block.get("type")
            if btype == "text" and block.get("text"):
                text_parts.append(str(block["text"]))
            elif btype == "tool_use":
                tools.append(str(block.get("name", "tool")))
            elif btype == "tool_result":
                tools.append("resultaat")

    text = " ".join(t.strip() for t in text_parts if t and t.strip())
    text = re.sub(r"\s+", " ", text).strip()
    if not text and not tools:
        return None

    return {
        "role": etype,
        "time": obj.get("timestamp", ""),
        "text": text[:600],
        "tools": tools[:6],
    }


# Welk spoor hoort bij welke agent (SCRIPT_PRODUCTION_PLAN §5)
AGENT_SPOOR = {
    "story-architect": "A", "dialogue-writer": "A",
    "dialogue-critic": "A", "voice-director": "A",
    "hud-builder": "B", "element-builder": "B",
    "code-reviewer": "B", "art-reviewer": "B",
    "game-planner": "*",
}

_runs_cache: dict[str, tuple[float, list]] = {}


def scan_agent_definitions() -> list[dict]:
    """De agents zoals ze in .claude/agents/ gedefinieerd staan."""
    out = []
    adir = REPO / ".claude" / "agents"
    if not adir.is_dir():
        return out
    for md in sorted(adir.glob("*.md")):
        try:
            head = md.read_text(encoding="utf-8", errors="replace")[:1500]
        except OSError:
            continue
        name = md.stem
        desc = ""
        m = re.search(r"^description:\s*(.+)$", head, re.M)
        if m:
            desc = m.group(1).strip()
        out.append({
            "name": name,
            "desc": desc[:240],
            "spoor": AGENT_SPOOR.get(name, "?"),
        })
    return out


def _norm_key(text: str) -> str:
    return re.sub(r"\s+", " ", str(text))[:120].strip().lower()


def _index_subagent_files() -> dict[str, Path]:
    """Koppelt de openingsprompt van een subagent-transcript aan zijn bestand."""
    idx: dict[str, Path] = {}
    for base in TRANSCRIPT_DIRS:
        if not base.is_dir():
            continue
        for sf in base.glob("*/subagents/*.jsonl"):
            try:
                with sf.open("rb") as fh:
                    first = fh.readline().decode("utf-8", errors="replace")
                obj = json.loads(first)
                content = (obj.get("message") or {}).get("content")
                if isinstance(content, list):
                    content = " ".join(
                        b.get("text", "") for b in content if isinstance(b, dict)
                    )
                if content:
                    idx[_norm_key(content)] = sf
            except Exception:
                continue
    return idx


def _parse_runs_from(jf: Path) -> list[dict]:
    """Haalt elke Task-aanroep (agent-spawn) uit een sessie-transcript."""
    runs: list[dict] = []
    pending: dict[str, dict] = {}
    try:
        with jf.open("r", encoding="utf-8", errors="replace") as fh:
            for line in fh:
                # goedkope voorfilter: JSON parsen is duur op 60 MB
                has_spawn = '"subagent_type"' in line
                has_result = '"tool_result"' in line
                if not (has_spawn or has_result):
                    continue
                try:
                    d = json.loads(line)
                except Exception:
                    continue
                content = (d.get("message") or {}).get("content")
                if not isinstance(content, list):
                    continue
                for b in content:
                    if not isinstance(b, dict):
                        continue
                    if b.get("type") == "tool_use":
                        inp = b.get("input") or {}
                        if "subagent_type" in inp:
                            run = {
                                "type": inp.get("subagent_type", "?"),
                                "desc": str(inp.get("description", ""))[:120],
                                "prompt": _norm_key(inp.get("prompt", "")),
                                "time": d.get("timestamp", ""),
                                "result": "",
                            }
                            runs.append(run)
                            if b.get("id"):
                                pending[b["id"]] = run
                    elif b.get("type") == "tool_result":
                        run = pending.pop(b.get("tool_use_id", ""), None)
                        if run is not None:
                            c = b.get("content")
                            if isinstance(c, list):
                                c = " ".join(
                                    x.get("text", "") for x in c if isinstance(x, dict)
                                )
                            run["result"] = re.sub(r"\s+", " ", str(c or ""))[:400]
    except OSError:
        pass
    return runs


def scan_agent_runs() -> list[dict]:
    """Alle agent-spawns over alle sessies, nieuwste eerst. Gecachet op mtime."""
    all_runs: list[dict] = []
    for base in TRANSCRIPT_DIRS:
        if not base.is_dir():
            continue
        for jf in base.glob("*.jsonl"):
            try:
                st = jf.stat()
            except OSError:
                continue
            key = str(jf)
            cached = _runs_cache.get(key)
            if cached and cached[0] == st.st_mtime:
                all_runs.extend(cached[1])
                continue
            runs = _parse_runs_from(jf)
            _runs_cache[key] = (st.st_mtime, runs)
            all_runs.extend(runs)

    subidx = _index_subagent_files()
    for r in all_runs:
        sf = subidx.get(r.get("prompt", ""))
        r["file"] = str(sf) if sf else ""
        r.pop("prompt", None)

    all_runs.sort(key=lambda r: r.get("time", ""), reverse=True)
    return all_runs


def scan_agents() -> dict:
    """De agent-tab: definities met gebruik, recente runs, en levende sessies."""
    runs = scan_agent_runs()

    per_agent: dict[str, list[dict]] = {}
    for r in runs:
        per_agent.setdefault(r["type"], []).append(r)

    definities = []
    for d in scan_agent_definitions():
        mine = per_agent.get(d["name"], [])
        last = mine[0] if mine else None
        definities.append({
            **d,
            "runs": len(mine),
            "lastTime": (last or {}).get("time", ""),
            "lastTask": (last or {}).get("desc", ""),
        })
    definities.sort(key=lambda a: (-a["runs"], a["name"]))

    # Sessies: alleen wat leeft. De grafvelden van oude chats hebben geen waarde.
    sessies = []
    cutoff = time.time() - 86400
    for base in TRANSCRIPT_DIRS:
        if not base.is_dir():
            continue
        for jf in base.glob("*.jsonl"):
            try:
                st = jf.stat()
            except OSError:
                continue
            if st.st_size < 400 or st.st_mtime < cutoff:
                continue
            chunk = tail_bytes(jf, 160_000)
            entries = []
            for line in chunk.splitlines():
                line = line.strip()
                if not line.startswith("{"):
                    continue
                try:
                    e = _extract_entry(json.loads(line))
                except Exception:
                    continue
                if e:
                    entries.append(e)
            last = entries[-1] if entries else None
            sessies.append({
                "id": jf.stem,
                "short": jf.stem[:8],
                "modified": ago(st.st_mtime),
                "active": (time.time() - st.st_mtime) < 180,
                "sizeMB": round(st.st_size / 1048576, 1),
                "lastText": (last or {}).get("text", ""),
                "lastRole": (last or {}).get("role", ""),
            })
    sessies.sort(key=lambda s: s["active"], reverse=True)

    for r in runs[:40]:
        r["ago"] = r.get("time", "")[:16].replace("T", " ")

    return {
        "definities": definities,
        "runs": runs[:40],
        "totaalRuns": len(runs),
        "sessies": sessies,
    }


def agent_messages(agent_id: str, limit: int = 60) -> list[dict]:
    for base in TRANSCRIPT_DIRS:
        jf = base / f"{agent_id}.jsonl"
        if not jf.is_file():
            continue
        chunk = tail_bytes(jf, 600_000)
        out: list[dict] = []
        for line in chunk.splitlines():
            line = line.strip()
            if not line.startswith("{"):
                continue
            try:
                obj = json.loads(line)
            except Exception:
                continue
            e = _extract_entry(obj)
            if e:
                out.append(e)
        return out[-limit:]
    return []


# ------------------------------------------------------------ scriptlaag --

def scan_script() -> dict:
    """Voortgang van de schrijflaag, per missie: skelet, dialoog, stem."""
    root = REPO / "Eclipse" / "Content" / "Script"
    counts = {"draft": 0, "critic-pass": 0, "generated": 0, "onbekend": 0}
    words = 0
    per_missie: dict[str, dict] = {}

    if root.is_dir():
        for yf in root.rglob("*.yaml"):
            try:
                text = yf.read_text(encoding="utf-8", errors="replace")
            except OSError:
                continue
            head = text[:4000]

            m = re.search(r"^status:\s*([\w-]+)", head, re.M)
            status = m.group(1) if m else "onbekend"
            counts[status] = counts.get(status, 0) + 1

            w = re.search(r"^words:\s*(\d+)", head, re.M)
            scene_words = int(w.group(1)) if w else 0
            words += scene_words

            t = re.search(r'^title:\s*"?([^"\n]+?)"?\s*$', head, re.M)
            mm = re.search(r"^mission:\s*(\S+)", head, re.M)
            missie = mm.group(1) if mm else (yf.parent.name or "overig")

            # aantal dialoogregels = aantal '- id:' onder lines:
            regels = len(re.findall(r"^\s+-\s+id:", text, re.M))

            am = re.search(r"^act:\s*(\d+)", head, re.M)
            act = int(am.group(1)) if am else 1

            groep = per_missie.setdefault(missie, {
                "missie": missie, "act": act, "scenes": [],
                "regels": 0, "woorden": 0,
            })
            groep["scenes"].append({
                "title": (t.group(1).strip() if t else yf.stem),
                "status": status,
                "regels": regels,
                "file": str(yf.relative_to(REPO)).replace("\\", "/"),
            })
            groep["regels"] += regels
            groep["woorden"] += scene_words

    missies = sorted(per_missie.values(), key=lambda g: g["missie"])

    # Nesten per act, zodat het dashboard een boom kan tonen:
    # act -> missies -> scenes -> dialoog.
    per_act: dict[int, dict] = {}
    for g in missies:
        a = g.get("act") or 1
        acts = per_act.setdefault(a, {"act": a, "missies": [], "scenes": 0,
                                      "regels": 0, "geschreven": 0})
        acts["missies"].append(g)
        acts["scenes"] += len(g["scenes"])
        acts["regels"] += g["regels"]
        acts["geschreven"] += sum(1 for s in g["scenes"] if s["regels"] > 0)

    # De acts die nog geen enkele scene hebben, tonen we ook — anders lijkt
    # het alsof de campagne uit één act bestaat.
    ACT_NAAM = {1: "Embers", 2: "The Spreading Dark", 3: "The Long War", 4: "Eclipse"}
    ACT_MISSIES = {1: 8, 2: 9, 3: 10, 4: 7}
    acts_out = []
    for a in (1, 2, 3, 4):
        d = per_act.get(a, {"act": a, "missies": [], "scenes": 0,
                            "regels": 0, "geschreven": 0})
        d["naam"] = ACT_NAAM[a]
        d["missiesVerwacht"] = ACT_MISSIES[a]
        acts_out.append(d)

    # beat-sheets = laag L1 (het skelet waar de dialoog aan hangt)
    beats_dir = REPO / "phase0" / "beats"
    beats = []
    if beats_dir.is_dir():
        for bf in sorted(beats_dir.glob("*.md")):
            try:
                st = bf.stat()
            except OSError:
                continue
            beats.append({
                "naam": bf.stem,
                "path": str(bf.relative_to(REPO)).replace("\\", "/"),
                "kb": round(st.st_size / 1024, 1),
            })

    total = sum(counts.values())
    met_regels = sum(1 for g in missies for s in g["scenes"] if s["regels"] > 0)
    return {
        "counts": counts,
        "total": total,
        "words": words,
        "regels": sum(g["regels"] for g in missies),
        "scenesMetDialoog": met_regels,
        "missies": missies,
        "acts": acts_out,
        "beats": beats,
    }


def scene_detail(rel: str) -> dict:
    """De inhoud van één scène: contract + de dialoogregels."""
    if not rel or ".." in rel or not rel.endswith(".yaml"):
        return {"fout": "ongeldig pad"}
    target = (REPO / rel).resolve()
    try:
        target.relative_to(REPO)
    except ValueError:
        return {"fout": "buiten de repo"}
    if not target.is_file():
        return {"fout": "niet gevonden"}
    try:
        text = target.read_text(encoding="utf-8", errors="replace")
    except OSError as exc:
        return {"fout": str(exc)}

    def veld(naam: str) -> str:
        m = re.search(rf'^{naam}:\s*"?(.*?)"?\s*$', text, re.M)
        return m.group(1).strip() if m else ""

    # dialoogregels uit het lines-blok
    regels = []
    blok = text.split("lines:", 1)
    if len(blok) > 1:
        for stuk in re.split(r"\n\s+-\s+id:", blok[1])[1:]:
            stuk = "id:" + stuk
            r = {}
            for k in ("id", "speaker", "voice", "text", "shot", "note"):
                m = re.search(rf'^\s*{k}:\s*"?(.*?)"?\s*$', stuk, re.M)
                if m and m.group(1):
                    r[k] = m.group(1).strip()
            tags = re.search(r"^\s*tags:\s*\[(.*?)\]", stuk, re.M)
            if tags:
                r["tags"] = tags.group(1).strip()
            if r.get("text") or r.get("speaker"):
                regels.append(r)

    # schrijversnotities van story-architect (regels met '# ')
    notities = [
        l.lstrip("# ").rstrip()
        for l in text.splitlines()
        if l.strip().startswith("#") and not l.strip().startswith("# ----")
    ]

    return {
        "path": rel,
        "titel": veld("title"),
        "missie": veld("mission"),
        "locatie": veld("location"),
        "type": veld("type"),
        "status": veld("status"),
        "want": veld("want"),
        "obstacle": veld("obstacle"),
        "turn": veld("turn"),
        "regels": regels,
        "notities": notities[:14],
    }


# --------------------------------------------------------------- credits --

def scan_voice() -> dict:
    ledger = REPO / "phase0" / "VOICE_LEDGER.md"
    spent = 0
    rows: list[dict] = []
    if ledger.is_file():
        try:
            txt = ledger.read_text(encoding="utf-8", errors="replace")
        except OSError:
            txt = ""
        # tabelregels: | datum | tier | scope | regels | credits | saldo |
        for line in txt.splitlines():
            cells = [c.strip() for c in line.split("|") if c.strip()]
            if len(cells) >= 5 and re.match(r"^\d{4}-\d{2}-\d{2}", cells[0]):
                num = re.sub(r"[^\d]", "", cells[4])
                if num:
                    spent += int(num)
                    rows.append({
                        "datum": cells[0], "tier": cells[1],
                        "scope": cells[2], "credits": int(num),
                    })
    return {
        "budget": VOICE_BUDGET,
        "spent": spent,
        "remaining": VOICE_BUDGET - spent,
        "pct": round(spent / VOICE_BUDGET * 100, 1) if VOICE_BUDGET else 0,
        "rows": rows[-15:],
    }


# ------------------------------------------------------------------ docs --

# Documenten die voor NATHAN geschreven zijn. Alles daarbuiten is voor de
# agents en hoeft hij nooit te openen — de dichtheid daarvan is met opzet
# hoog. Deze splitsing bestaat omdat hij anders tussen 90 agent-documenten
# moet zoeken naar de drie die hem aangaan.
OWNER_DOCS = {"JOUW_ACTIES.md", "STATUS.md", "BESTURING.md"}

# Hoe vers een owner-document hoort te zijn, in uren. Wordt er ouder
# gemeten, dan kleurt het dashboard hem — Nathan leest deze drie als
# achtergrond bij de keuzekaarten, dus verouderde info kost hem een
# verkeerde beslissing.
OWNER_DOC_VERS = {
    "STATUS.md": 24,        # bijwerken aan het eind van elke sessie
    "JOUW_ACTIES.md": 24,   # bijwerken zodra een owner-actie verandert
    "BESTURING.md": 168,    # bijwerken zodra de besturing verandert
}

DOC_GROUPS = [
    ("Voor jou", lambda p: p.name in OWNER_DOCS and len(p.parts) == 1),
    ("Game Design Bible", lambda p: re.match(r"^\d\d_", p.name)),
    ("Werkdocumenten (phase0)", lambda p: p.parts[0] == "phase0" if p.parts else False),
    ("Specs", lambda p: "specs" in p.parts),
    ("Archief", lambda p: "archief" in [x.lower() for x in p.parts]),
]

# Volgorde waarin de groepen op het dashboard komen
DOC_GROUP_ORDER = ["Voor jou", "Game Design Bible", "Werkdocumenten (phase0)",
                   "Specs", "Overig", "Archief"]


def scan_docs() -> dict:
    groups: dict[str, list[dict]] = {}
    skip_dirs = {".git", "Eclipse", "progress_media", "brand", "node_modules"}
    for md in REPO.rglob("*.md"):
        rel = md.relative_to(REPO)
        if rel.parts and rel.parts[0] in skip_dirs:
            if not (len(rel.parts) > 1 and rel.parts[0] == "Eclipse" and "Docs" in rel.parts):
                continue
        try:
            st = md.stat()
        except OSError:
            continue
        entry = {
            "path": str(rel).replace("\\", "/"),
            "name": md.name,
            "kb": round(st.st_size / 1024, 1),
            "modified": ago(st.st_mtime),
            "mtime": st.st_mtime,
        }
        limiet = OWNER_DOC_VERS.get(md.name)
        if limiet and len(rel.parts) == 1:
            uren = (time.time() - st.st_mtime) / 3600
            entry["uren"] = round(uren, 1)
            entry["limiet"] = limiet
            entry["oud"] = uren > limiet
        placed = False
        for label, test in DOC_GROUPS:
            try:
                if test(rel):
                    groups.setdefault(label, []).append(entry)
                    placed = True
                    break
            except Exception:
                pass
        if not placed:
            groups.setdefault("Overig", []).append(entry)

    for lst in groups.values():
        lst.sort(key=lambda e: e["path"])

    # In vaste volgorde teruggeven: "Voor jou" hoort bovenaan, niet ergens
    # tussen negentig agent-documenten.
    ordered: dict[str, list] = {}
    for label in DOC_GROUP_ORDER:
        if groups.get(label):
            ordered[label] = groups.pop(label)
    ordered.update(groups)
    return ordered


# ----------------------------------------------------------- screenshots --

def scan_shots(limit: int = 24) -> list[dict]:
    if not SHOTS_DIR.is_dir():
        return []
    files = [p for p in SHOTS_DIR.iterdir()
             if p.is_file() and p.suffix.lower() in {".png", ".jpg", ".jpeg"}]
    files.sort(key=lambda p: p.stat().st_mtime, reverse=True)
    out = []
    for p in files[:limit]:
        st = p.stat()
        out.append({
            "name": p.name,
            "url": f"/shots/{urllib.parse.quote(p.name)}",
            "modified": ago(st.st_mtime),
            "kb": round(st.st_size / 1024),
        })
    return out


OWNER_SHOTS = Path.home() / "Pictures" / "Screenshots"


def scan_findings() -> dict:
    """Bevindingen die screenshot-inspector uit de beelden heeft gehaald."""
    p = REPO / "phase0" / "SHOT_FINDINGS.md"
    rows: list[dict] = []
    beoordeeld = ""
    if p.is_file():
        try:
            text = p.read_text(encoding="utf-8", errors="replace")
        except OSError:
            text = ""
        m = re.search(r"\*\*Beoordeeld tot en met:\*\*\s*`?([^`\n]+)`?", text)
        if m:
            beoordeeld = m.group(1).strip()
        for line in text.splitlines():
            cells = [c.strip() for c in line.split("|")[1:-1]]
            if len(cells) >= 5 and re.match(r"^\d{4}-\d{2}-\d{2}", cells[0]):
                rows.append({
                    "datum": cells[0], "bestand": _clean_cell(cells[1]),
                    "ernst": cells[2].lower(), "wat": _clean_cell(cells[3]),
                    "oorzaak": _clean_cell(cells[4]),
                })

    # hoeveel beelden wachten nog op beoordeling?
    nieuw = 0
    laatste = 0.0
    for folder in (SHOTS_DIR, OWNER_SHOTS):
        if not folder.is_dir():
            continue
        try:
            for f in folder.iterdir():
                if f.is_file() and f.suffix.lower() in {".png", ".jpg", ".jpeg"}:
                    laatste = max(laatste, f.stat().st_mtime)
                    nieuw += 1
        except OSError:
            pass

    rows.reverse()
    return {
        "rows": rows[:25],
        "totaal": len(rows),
        "beoordeeldTot": beoordeeld,
        "beeldenTotaal": nieuw,
        "laatsteBeeld": ago(laatste) if laatste else "",
        "blokkeert": sum(1 for r in rows if r["ernst"].startswith("blokkeer")),
    }


def scan_tests() -> dict:
    auto = REPO / "progress_auto.js"
    if not auto.is_file():
        return {}
    try:
        txt = auto.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return {}
    m = re.search(r'"tests":\s*\{(.*?)\}', txt, re.S)
    if not m:
        return {}
    body = m.group(1)
    def num(key: str) -> int:
        mm = re.search(rf'"{key}":\s*(\d+)', body)
        return int(mm.group(1)) if mm else 0
    age = re.search(r'"reportAge":\s*"([^"]*)"', body)
    return {
        "ok": num("ok"), "failed": num("failed"), "total": num("total"),
        "age": age.group(1) if age else "",
    }


def housekeep_shots(keep: int = 50) -> int:
    """Houdt de screenshotmap op `keep` bestanden.

    De editor blijft tijdens het werk nieuwe screenshots wegschrijven, dus
    eenmalig opruimen volstaat niet. Deze server draait toch al continu, dus
    hij doet het onderhoud er zelf bij.
    """
    if not SHOTS_DIR.is_dir():
        return 0
    exts = {".png", ".jpg", ".jpeg", ".bmp", ".exr"}
    try:
        files = [p for p in SHOTS_DIR.iterdir() if p.is_file() and p.suffix.lower() in exts]
    except OSError:
        return 0
    if len(files) <= keep:
        return 0
    files.sort(key=lambda p: p.stat().st_mtime, reverse=True)
    removed = 0
    for p in files[keep:]:
        try:
            p.unlink()
            removed += 1
        except OSError:
            pass
    return removed


# ------------------------------------------------------- owner-blokkades --

# Waar owner-acties in de repo staan. Elk document heeft zijn eigen tabelvorm,
# dus de parser gaat op KOLOMNAMEN af en niet op kolomvolgorde.
OWNER_SOURCES = [
    ("STATUS.md", "STATUS"),
    ("JOUW_ACTIES.md", "JOUW_ACTIES"),
    ("phase0/EXECUTION_PLAN.md", "EXECUTION_PLAN §4"),
    ("phase0/SCRIPT_PRODUCTION_PLAN.md", "SCRIPT_PLAN §7"),
]

_ID_RE = re.compile(r"^~*\s*([OT]-\d+)\s*~*$", re.I)

# Welke kolomkop betekent wat
_COL_HINTS = {
    "actie": "actie", "wat": "actie", "item": "actie",
    "blokkeert": "blokkeert", "waarvoor": "blokkeert",
    "waarvoor / blokkeert": "blokkeert", "blokkeert wat": "blokkeert",
    "wanneer": "wanneer", "hoe lang": "duur", "duur": "duur",
}


def _clean_cell(text: str) -> str:
    """Markdown-opmaak eruit, zodat het dashboard leesbare tekst krijgt."""
    t = re.sub(r"<br\s*/?>", " · ", text)
    t = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", t)   # links
    t = re.sub(r"[*_`~]", "", t)                      # nadruk
    return re.sub(r"\s+", " ", t).strip()


def cols_index(cols: dict, key: str, fallback: int, cells: list) -> int:
    """Index van de kolom met betekenis `key`; anders een veilige terugval."""
    for idx, k in cols.items():
        if k == key and idx < len(cells):
            return idx
    nxt = fallback + 1
    return nxt if 0 <= nxt < len(cells) else max(0, min(fallback, len(cells) - 1))


def _parse_owner_table(text: str, bron: str) -> list[dict]:
    """Haalt O-/T-rijen uit elke markdown-tabel in een document."""
    found: list[dict] = []
    lines = text.splitlines()
    cols: dict[int, str] = {}

    for i, raw in enumerate(lines):
        line = raw.strip()
        if not line.startswith("|"):
            cols = {}
            continue

        cells = [c.strip() for c in line.split("|")[1:-1]]
        if not cells:
            continue

        # scheidingsregel -> de regel erboven was de kop
        if all(re.fullmatch(r":?-{2,}:?", c) for c in cells if c):
            header = [c.strip().lower() for c in lines[i - 1].split("|")[1:-1]] if i else []
            cols = {}
            for idx, h in enumerate(header):
                for hint, key in _COL_HINTS.items():
                    if hint in h:
                        cols[idx] = key
                        break
            continue

        # Het ID staat niet altijd in kolom 0 — in JOUW_ACTIES.md is kolom 0
        # "Wanneer" en zit het ID in "Wat". Zoek dus in elke cel.
        rid = None
        id_idx = -1
        for idx, cell in enumerate(cells):
            m = _ID_RE.match(cell) or re.match(r"^\**([OT]-\d+)\**\b", cell)
            if m:
                rid = m.group(1).upper()
                id_idx = idx
                break
        if not rid:
            continue

        row = {"id": rid, "bron": bron, "actie": "", "blokkeert": "", "wanneer": "", "duur": ""}
        for idx, cell in enumerate(cells):
            key = cols.get(idx)
            if key and not row[key]:
                row[key] = _clean_cell(cell)
        if not row["actie"]:
            # val terug op de cel waar het ID in stond, of de cel erna
            cand = cells[id_idx] if id_idx >= 0 else ""
            if _ID_RE.match(cand) and len(cells) > id_idx + 1:
                cand = cells[id_idx + 1]
            row["actie"] = _clean_cell(cand)

        # Afgerond = DOORGESTREEPT (~~) in de ruwe cel. Let op: _clean_cell
        # strípt de tildes, dus deze test moet op de ruwe tekst. Een losse ✓
        # midden in de zin telt NIET — T-2 zegt "login ✓ ... resterend zijn de
        # env-pack-pulls" en staat dus juist nog open.
        raw_id_cell = cells[id_idx] if id_idx >= 0 else ""
        raw_actie = cells[cols_index(cols, "actie", id_idx, cells)]
        row["klaar"] = bool(
            raw_id_cell.startswith("~~")
            or raw_actie.startswith("~~")
            or "afgerond" in raw_actie[:60].lower()
        )
        found.append(row)
    return found


def scan_owner_actions() -> list[dict]:
    """Alle owner-blokkades uit alle documenten, ontdubbeld op ID."""
    merged: dict[str, dict] = {}
    for rel, label in OWNER_SOURCES:
        p = REPO / rel
        if not p.is_file():
            continue
        try:
            text = p.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        for row in _parse_owner_table(text, label):
            cur = merged.get(row["id"])
            if cur is None:
                merged[row["id"]] = row
                continue
            # rijker record wint per veld; "klaar" is besmettelijk
            for k in ("actie", "blokkeert", "wanneer", "duur"):
                if len(row.get(k, "")) > len(cur.get(k, "")):
                    cur[k] = row[k]
            cur["klaar"] = cur["klaar"] or row["klaar"]
            if label not in cur["bron"]:
                cur["bron"] += " + " + label

    # Een punt dat Nathan via een knop heeft beantwoord is klaar, ook als de
    # tabel in het document nog niet is bijgewerkt. Anders blijft T-10 als
    # "open" staan terwijl hij er al op geklikt heeft.
    answers = read_answers()
    for rid, row in merged.items():
        ans = answers.get(rid)
        if ans and ans.get("waarde") not in ("wacht", "later"):
            row["klaar"] = True
            row["beantwoord"] = ans.get("waarde", "")

    out = list(merged.values())
    out.sort(key=lambda r: (r["klaar"], r["id"][0], int(r["id"].split("-")[1])))
    return out


# --------------------------------------------------- vragen & antwoorden --

QUESTIONS_FILE = REPO / "phase0" / "owner_questions.json"
ANSWERS_FILE = REPO / "phase0" / "OWNER_ANSWERS.md"

_answers_lock = threading.Lock()


def read_answers() -> dict[str, dict]:
    """Antwoorden die Nathan al via het dashboard gegeven heeft."""
    out: dict[str, dict] = {}
    if not ANSWERS_FILE.is_file():
        return out
    try:
        text = ANSWERS_FILE.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return out
    # regels: | 2026-07-31 20:14 | O-6 | A | vrije tekst |
    for line in text.splitlines():
        cells = [c.strip() for c in line.split("|")[1:-1]]
        if len(cells) >= 3 and re.match(r"^\d{4}-\d{2}-\d{2}", cells[0]):
            out[cells[1].upper()] = {
                "tijd": cells[0],
                "waarde": cells[2],
                "tekst": cells[3] if len(cells) > 3 else "",
            }
    return out


def scan_questions() -> list[dict]:
    """Openstaande keuzevragen, met de stappen erbij en of ze al beantwoord zijn."""
    if not QUESTIONS_FILE.is_file():
        return []
    try:
        data = json.loads(QUESTIONS_FILE.read_text(encoding="utf-8", errors="replace"))
    except Exception:
        return []
    answers = read_answers()
    out = []
    for q in data.get("vragen", []):
        qid = str(q.get("id", "")).upper()
        item = {**q, "id": qid, "antwoord": answers.get(qid)}

        # Audio die bij deze vraag hoort, zodat hij het IN het dashboard
        # kan afspelen in plaats van mappen te moeten zoeken.
        item["audio"] = []
        rel = q.get("audio_map")
        if rel:
            folder = (REPO / rel)
            if folder.is_dir():
                clips = [
                    p for p in sorted(folder.rglob("*"))
                    if p.is_file() and p.suffix.lower() in {".wav", ".mp3", ".ogg", ".flac"}
                ]
                for p in clips[:40]:
                    item["audio"].append({
                        "naam": p.stem,
                        "url": "/audio/" + urllib.parse.quote(
                            str(p.relative_to(REPO)).replace("\\", "/")
                        ),
                    })
        out.append(item)

    orde = {"nu": 0, "gauw": 1, "later": 2}
    out.sort(key=lambda q: (q["antwoord"] is not None, orde.get(q.get("prio"), 1)))
    return out


def write_answer(qid: str, waarde: str, tekst: str = "") -> dict:
    """Legt een antwoord vast waar zowel het dashboard als de agents het zien."""
    qid = re.sub(r"[^A-Za-z0-9\-]", "", qid).upper()[:12]
    if not qid:
        return {"ok": False, "fout": "ongeldige vraag-id"}

    waarde = re.sub(r"\s+", " ", str(waarde))[:120].strip()
    tekst = re.sub(r"[|\r\n]+", " ", str(tekst))[:600].strip()
    stamp = datetime.now().strftime("%Y-%m-%d %H:%M")

    with _answers_lock:
        if not ANSWERS_FILE.is_file():
            ANSWERS_FILE.write_text(
                "# ANTWOORDEN VAN DE OWNER\n"
                "*Nathan beantwoordt vragen met een knop op het dashboard; ze komen hier terecht.*\n"
                "*Agents: LEES DIT ELKE SESSIE. Een antwoord hier is bindend en telt als owner-instructie.*\n"
                "*Voeg nieuwe vragen toe aan `phase0/owner_questions.json` — dan verschijnen ze bij hem op het scherm.*\n\n"
                "| Wanneer | Vraag | Antwoord | Toelichting |\n"
                "|---|---|---|---|\n",
                encoding="utf-8",
            )
        with ANSWERS_FILE.open("a", encoding="utf-8") as fh:
            fh.write(f"| {stamp} | {qid} | {waarde} | {tekst} |\n")

    return {"ok": True, "id": qid, "waarde": waarde, "tijd": stamp}


# ------------------------------------------------------------- casting --

CASTING_DATA = REPO / "progress_media" / "casting" / "casting_stage1.json"
CASTING_KEUZE = REPO / "phase0" / "CASTING_KEUZE.json"
_casting_lock = threading.Lock()

# De ElevenLabs-eigenschappen zijn Engelse steekwoorden. Nathan leest
# Nederlands, en "male / young / american / chill" zegt hem minder dan
# "man · jong · Amerikaans accent · ontspannen".
_EIG = {
    "male": "man", "female": "vrouw", "neutral": "neutraal",
    "young": "jong", "middle-aged": "middelbaar", "middle aged": "middelbaar",
    "old": "oud",
    "american": "Amerikaans accent", "british": "Brits accent",
    "australian": "Australisch accent", "irish": "Iers accent",
    "transatlantic": "neutraal accent", "african": "Afrikaans accent",
    "chill": "ontspannen", "confident": "zelfverzekerd", "rough": "ruw",
    "professional": "zakelijk", "calm": "kalm", "warm": "warm",
    "deep": "diep", "raspy": "schor", "hoarse": "hees", "gravelly": "grind",
    "authoritative": "gezaghebbend", "intense": "intens", "soft": "zacht",
    "energetic": "energiek", "friendly": "vriendelijk", "serious": "ernstig",
    "narration": "verteller", "conversational": "spreektaal",
    "news": "nieuwslezer", "characters": "karakterstem", "meditative": "traag",
    "sassy": "brutaal", "upbeat": "opgewekt", "wise": "wijs",
    "expressive": "expressief", "casual": "informeel", "mature": "volwassen",
    "pleasant": "aangenaam", "crisp": "helder", "husky": "rokerig",
}


def _leesbaar(eig: str) -> str:
    delen = [d.strip().lower() for d in re.split(r"[/,]", str(eig)) if d.strip()]
    return " · ".join(_EIG.get(d, d) for d in delen)


def read_casting_keuze() -> dict:
    if not CASTING_KEUZE.is_file():
        return {}
    try:
        return json.loads(CASTING_KEUZE.read_text(encoding="utf-8", errors="replace"))
    except Exception:
        return {}


def scan_casting() -> dict:
    """De castingkandidaten met Nathans keuzes erbij."""
    if not CASTING_DATA.is_file():
        return {"rollen": [], "totaal": 0, "gekozen": 0}
    try:
        data = json.loads(CASTING_DATA.read_text(encoding="utf-8", errors="replace"))
    except Exception:
        return {"rollen": [], "totaal": 0, "gekozen": 0}

    keuze = read_casting_keuze()
    rollen = []
    for r in data.get("rollen", []):
        rid = r.get("rol", "")
        gekozen = keuze.get(rid, [])
        kand = []
        for k in r.get("kandidaten", []):
            bestand = str(k.get("bestand", "")).replace("\\", "/")
            # Verse kandidaten uit de bibliotheek hebben geen lokaal bestand
            # maar een preview-URL bij ElevenLabs; die speelt net zo goed af.
            if bestand:
                url = "/" + urllib.parse.quote(bestand)
            else:
                url = k.get("preview_url", "")
            kand.append({
                "nr": k.get("nr"),
                "stem": k.get("stem", ""),
                "voice_id": k.get("voice_id", ""),
                "eigenschappen": _leesbaar(k.get("eigenschappen", "")),
                "waarom": k.get("waarom", ""),
                "beschrijving": k.get("beschrijving", ""),
                "categorie": k.get("categorie", ""),
                "bron": k.get("bron", "premade"),
                "url": url,
                "gekozen": k.get("nr") in gekozen,
            })
        rollen.append({
            "rol": rid,
            "label": r.get("label", rid),
            "prio": r.get("prio", 9),
            "tier": r.get("tier", ""),
            "fingerprint": r.get("fingerprint", ""),
            "kandidaten": kand,
            "gekozen": gekozen,
            "klaar": len(gekozen) >= 2,
        })
    rollen.sort(key=lambda x: (x["klaar"], x["prio"]))
    return {
        "rollen": rollen,
        "totaal": len(rollen),
        "gekozen": sum(1 for r in rollen if r["klaar"]),
        "conflicten": data.get("stem_conflicten", []),
    }


def zet_casting_keuze(rol: str, nr: int) -> dict:
    """Zet een kandidaat aan of uit. Maximaal twee per rol — de oudste valt af."""
    rol = re.sub(r"[^a-zA-Z0-9_\-]", "", str(rol))[:48]
    if not rol:
        return {"ok": False, "fout": "ongeldige rol"}
    with _casting_lock:
        keuze = read_casting_keuze()
        huidig = list(keuze.get(rol, []))
        if nr in huidig:
            huidig.remove(nr)
        else:
            huidig.append(nr)
            if len(huidig) > 2:        # nieuwste twee behouden
                huidig = huidig[-2:]
        keuze[rol] = huidig
        CASTING_KEUZE.parent.mkdir(parents=True, exist_ok=True)
        CASTING_KEUZE.write_text(
            json.dumps(keuze, ensure_ascii=False, indent=2), encoding="utf-8"
        )
    return {"ok": True, "rol": rol, "gekozen": huidig}


def disk_info() -> dict:
    def size_of(p: Path) -> float:
        total = 0
        if p.is_dir():
            for f in p.rglob("*"):
                try:
                    if f.is_file():
                        total += f.stat().st_size
                except OSError:
                    pass
        return round(total / 1073741824, 2)
    return {
        "screenshotsGB": size_of(SHOTS_DIR),
        "shotCount": len(list(SHOTS_DIR.glob("*.png"))) if SHOTS_DIR.is_dir() else 0,
    }


# ------------------------------------------------------------ scanlus --

def rescan() -> dict:
    return {
        "generatedAt": now_str(),
        "git": scan_git(),
        "tests": scan_tests(),
        "agents": scan_agents(),
        "script": scan_script(),
        "voice": scan_voice(),
        "docs": scan_docs(),
        "shots": scan_shots(),
        "disk": disk_info(),
        "booting": False,
    }


def scanner_loop() -> None:
    global _state
    slow_counter = 0
    cached_docs: dict = {}
    cached_disk: dict = {}
    while True:
        try:
            fresh = {
                "generatedAt": now_str(),
                "git": scan_git(),
                "tests": scan_tests(),
                "agents": scan_agents(),
                "script": scan_script(),
                "voice": scan_voice(),
                "shots": scan_shots(),
                "booting": False,
            }
            # docs en schijfgebruik zijn duur: elke 12e ronde (~1 min)
            if slow_counter % 12 == 0 or not cached_docs:
                housekeep_shots(50)      # oude screenshots opruimen
                cached_docs = scan_docs()
                cached_disk = disk_info()
            fresh["owner"] = scan_owner_actions()
            fresh["vragen"] = scan_questions()
            fresh["bevindingen"] = scan_findings()
            fresh["casting"] = scan_casting()
            fresh["docs"] = cached_docs
            fresh["disk"] = cached_disk
            slow_counter += 1
            with _state_lock:
                _state = fresh
        except Exception as exc:  # nooit stilvallen
            with _state_lock:
                _state = {**_state, "error": f"{type(exc).__name__}: {exc}", "booting": False}
        time.sleep(SCAN_INTERVAL)


# ------------------------------------------------------------- webserver --

class Handler(BaseHTTPRequestHandler):
    server_version = "EclipseDash/1.0"

    def log_message(self, fmt, *args):  # stil
        pass

    def _send(self, code: int, body: bytes, ctype: str, cache: bool = False) -> None:
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        if not cache:
            self.send_header("Cache-Control", "no-store, must-revalidate")
        self.end_headers()
        try:
            self.wfile.write(body)
        except (BrokenPipeError, ConnectionAbortedError):
            pass

    def _json(self, obj) -> None:
        self._send(200, json.dumps(obj, ensure_ascii=False).encode("utf-8"),
                   "application/json; charset=utf-8")

    def do_POST(self) -> None:  # noqa: N802
        """Nathan beantwoordt een vraag of kiest een stem, met een knop."""
        pad = urllib.parse.urlparse(self.path).path
        if pad not in ("/api/answer", "/api/casting"):
            return self._send(404, b"niet gevonden", "text/plain; charset=utf-8")
        if pad == "/api/casting":
            try:
                length = int(self.headers.get("Content-Length") or 0)
                payload = json.loads(self.rfile.read(min(length, 8_000)).decode("utf-8"))
            except Exception as exc:
                return self._json({"ok": False, "fout": f"onleesbaar: {exc}"})
            res = zet_casting_keuze(payload.get("rol", ""), int(payload.get("nr", 0)))
            if res.get("ok"):
                with _state_lock:
                    _state["casting"] = scan_casting()
            return self._json(res)
        try:
            length = int(self.headers.get("Content-Length") or 0)
            body = self.rfile.read(min(length, 64_000)).decode("utf-8", errors="replace")
            payload = json.loads(body) if body else {}
        except Exception as exc:
            return self._json({"ok": False, "fout": f"onleesbaar verzoek: {exc}"})

        result = write_answer(
            str(payload.get("id", "")),
            str(payload.get("waarde", "")),
            str(payload.get("tekst", "")),
        )
        # meteen verversen zodat de pagina het antwoord direct terugziet
        if result.get("ok"):
            with _state_lock:
                _state["vragen"] = scan_questions()
        return self._json(result)

    def do_GET(self) -> None:  # noqa: N802
        parsed = urllib.parse.urlparse(self.path)
        path = urllib.parse.unquote(parsed.path)
        query = urllib.parse.parse_qs(parsed.query)

        if path in ("/", "/index.html", "/START_HIER.html", "/PROGRESS.html"):
            return self._serve_file(REPO / "DASHBOARD.html", "text/html; charset=utf-8")

        # De losse castingpagina is vervangen door het tabblad Casting: daar
        # kan hij ook KIEZEN, en twee implementaties lopen gegarandeerd uit
        # elkaar. Oude link blijft werken, hij komt alleen op de goede plek uit.
        if path.lower().endswith("/casting.html"):
            self.send_response(302)
            self.send_header("Location", "/#casting")
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            return

        if path == "/api/state":
            with _state_lock:
                return self._json(_state)

        if path == "/api/scene":
            return self._json(scene_detail((query.get("path") or [""])[0]))

        if path == "/api/doc":
            rel = (query.get("path") or [""])[0]
            return self._serve_doc(rel)

        if path == "/api/agent":
            aid = (query.get("id") or [""])[0]
            if not re.fullmatch(r"[A-Za-z0-9\-]{6,64}", aid or ""):
                return self._json({"error": "ongeldige id"})
            return self._json({"id": aid, "messages": agent_messages(aid)})

        if path.startswith("/shots/"):
            name = Path(path[len("/shots/"):]).name
            return self._serve_file(SHOTS_DIR / name, None, cache=True)

        if path.startswith("/audio/"):
            rel = path[len("/audio/"):]
            target = (REPO / rel).resolve()
            try:
                target.relative_to(REPO)
            except ValueError:
                return self._send(403, b"verboden", "text/plain; charset=utf-8")
            if target.suffix.lower() not in {".wav", ".mp3", ".ogg", ".flac"}:
                return self._send(403, b"geen audiobestand", "text/plain; charset=utf-8")
            return self._serve_file(target, None, cache=True)

        # statische bestanden uit de repo-root
        candidate = (REPO / path.lstrip("/")).resolve()
        try:
            candidate.relative_to(REPO)
        except ValueError:
            return self._send(403, b"verboden", "text/plain; charset=utf-8")
        if candidate.is_file():
            return self._serve_file(candidate, None, cache=True)

        self._send(404, b"niet gevonden", "text/plain; charset=utf-8")

    def _serve_doc(self, rel: str) -> None:
        if not rel or ".." in rel:
            return self._json({"error": "ongeldig pad"})
        target = (REPO / rel).resolve()
        try:
            target.relative_to(REPO)
        except ValueError:
            return self._json({"error": "buiten de repo"})
        if not target.is_file() or target.suffix.lower() not in {".md", ".txt", ".yaml", ".yml"}:
            return self._json({"error": "geen leesbaar document"})
        try:
            text = target.read_text(encoding="utf-8", errors="replace")
        except OSError as exc:
            return self._json({"error": str(exc)})
        if len(text) > 400_000:
            text = text[:400_000] + "\n\n… (afgekapt)"
        return self._json({"path": rel, "text": text})

    def _serve_file(self, p: Path, ctype: str | None, cache: bool = False) -> None:
        if not p.is_file():
            return self._send(404, b"niet gevonden", "text/plain; charset=utf-8")
        ctype = ctype or (mimetypes.guess_type(p.name)[0] or "application/octet-stream")
        try:
            return self._send(200, p.read_bytes(), ctype, cache=cache)
        except OSError as exc:
            return self._send(500, str(exc).encode(), "text/plain; charset=utf-8")


def main() -> int:
    threading.Thread(target=scanner_loop, daemon=True).start()
    srv = ThreadingHTTPServer(("127.0.0.1", PORT), Handler)
    print("=" * 62)
    print("  ECLIPSE — live dashboard")
    print(f"  http://127.0.0.1:{PORT}/")
    print(f"  repo: {REPO}")
    print("  Laat dit venster open staan. Sluiten = dashboard uit.")
    print("=" * 62)
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        print("\nGestopt.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
