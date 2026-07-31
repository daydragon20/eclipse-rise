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
VOICE_BUDGET = 310_000

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


def scan_agents() -> list[dict]:
    agents: list[dict] = []
    for base in TRANSCRIPT_DIRS:
        if not base.is_dir():
            continue
        for jf in base.glob("*.jsonl"):
            try:
                st = jf.stat()
            except OSError:
                continue
            if st.st_size < 400:
                continue

            chunk = tail_bytes(jf, 220_000)
            entries: list[dict] = []
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
                    entries.append(e)

            last = entries[-1] if entries else None
            tool_counts: dict[str, int] = {}
            for e in entries[-80:]:
                for t in e["tools"]:
                    tool_counts[t] = tool_counts.get(t, 0) + 1

            # subagents = submappen met dezelfde sessie-id
            subdir = base / jf.stem
            subagents = 0
            if subdir.is_dir():
                subagents = len(list(subdir.glob("*.jsonl")))

            agents.append({
                "id": jf.stem,
                "short": jf.stem[:8],
                "mtime": st.st_mtime,
                "modified": ago(st.st_mtime),
                "active": (time.time() - st.st_mtime) < 180,
                "sizeMB": round(st.st_size / 1048576, 1),
                "messages": len(entries),
                "subagents": subagents,
                "lastRole": last["role"] if last else "",
                "lastText": last["text"] if last else "",
                "topTools": sorted(tool_counts.items(), key=lambda kv: -kv[1])[:5],
            })

    agents.sort(key=lambda a: a["mtime"], reverse=True)
    return agents[:12]


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
    """Voortgang van de schrijflaag: .yaml-scènes per status."""
    root = REPO / "Eclipse" / "Content" / "Script"
    counts = {"draft": 0, "critic-pass": 0, "generated": 0, "onbekend": 0}
    words = 0
    scenes: list[dict] = []
    if root.is_dir():
        for yf in root.rglob("*.yaml"):
            try:
                head = yf.read_text(encoding="utf-8", errors="replace")[:4000]
            except OSError:
                continue
            m = re.search(r"^status:\s*([\w-]+)", head, re.M)
            status = m.group(1) if m else "onbekend"
            counts[status] = counts.get(status, 0) + 1
            w = re.search(r"^words:\s*(\d+)", head, re.M)
            if w:
                words += int(w.group(1))
            t = re.search(r'^title:\s*"?([^"\n]+)"?', head, re.M)
            scenes.append({
                "file": str(yf.relative_to(REPO)).replace("\\", "/"),
                "title": (t.group(1).strip() if t else yf.stem),
                "status": status,
            })
    total = sum(counts.values())
    return {"counts": counts, "total": total, "words": words, "scenes": scenes[:60]}


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

DOC_GROUPS = [
    ("Game Design Bible", lambda p: re.match(r"^\d\d_", p.name)),
    ("Werkdocumenten (phase0)", lambda p: p.parts[0] == "phase0" if p.parts else False),
    ("Specs", lambda p: "specs" in p.parts),
    ("Archief", lambda p: "archief" in [x.lower() for x in p.parts]),
]


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
    return groups


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

    def do_GET(self) -> None:  # noqa: N802
        parsed = urllib.parse.urlparse(self.path)
        path = urllib.parse.unquote(parsed.path)
        query = urllib.parse.parse_qs(parsed.query)

        if path in ("/", "/index.html", "/START_HIER.html", "/PROGRESS.html"):
            return self._serve_file(REPO / "DASHBOARD.html", "text/html; charset=utf-8")

        if path == "/api/state":
            with _state_lock:
                return self._json(_state)

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
