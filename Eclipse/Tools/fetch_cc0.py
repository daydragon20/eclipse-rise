#!/usr/bin/env python3
"""
fetch_cc0.py - CC0 asset-database downloader voor ECLIPSE.
Volledig autonoom (HTTP-API's, geen login, geen klikken). Alles is CC0/royalty-free.

Bronnen:
  * ambientCG   - PBR-materialen/texturen (https://ambientcg.com)
  * Poly Haven  - HDRIs, texturen, modellen (https://polyhaven.com)

Gebruik:
  python fetch_cc0.py ambientcg <trefwoord> <aantal> [resolutie=2K]
  python fetch_cc0.py polyhaven <hdris|textures|models> <aantal> [resolutie=2k] [categorie]
  python fetch_cc0.py list-ambientcg <trefwoord> [aantal]      # alleen tonen, niet downloaden
  python fetch_cc0.py list-polyhaven <type> [categorie]

Downloadt naar  <project>/Eclipse/Saved/CC0Staging/<bron>/  zodat een UE-Python-
import-stap (zie import_polyhaven_props.py) ze in /Game/Art kan trekken. Elke asset
gaat daarna verplicht door de toon-master (Borderlands-stijl, 15.5) - materialen
worden NIET als kant-en-klaar UE-materiaal geimporteerd.

Provenance: append naar Content/Art/Textures/SOURCES.md bij echt gebruik.
"""
import json
import os
import sys
import urllib.request
import urllib.parse
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT = os.path.abspath(os.path.join(HERE, ".."))
STAGING = os.path.join(PROJECT, "Saved", "CC0Staging")
UA = {"User-Agent": "ECLIPSE-fetch-cc0/1.0 (game-dev, CC0 assets)"}


def _get_json(url):
    req = urllib.request.Request(url, headers=UA)
    with urllib.request.urlopen(req, timeout=30) as r:
        return json.load(r)


def _download(url, dest):
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    req = urllib.request.Request(url, headers=UA)
    with urllib.request.urlopen(req, timeout=120) as r, open(dest, "wb") as f:
        while True:
            chunk = r.read(65536)
            if not chunk:
                break
            f.write(chunk)
    return os.path.getsize(dest)


# ---------------- ambientCG ----------------
def ambientcg_list(keyword, count=20):
    # q= is echte trefwoordzoektocht (tags + naam); id-substring miste bv. "grunge"
    url = ("https://ambientcg.com/api/v2/full_json?type=Material&sort=Popular"
           f"&limit={max(count, 20)}&q=" + urllib.parse.quote(keyword))
    data = _get_json(url)
    hits = [a["assetId"] for a in data.get("foundAssets", [])]
    return hits[:count]


def ambientcg_fetch(keyword, count, res="2K"):
    ids = ambientcg_list(keyword, count)
    if not ids:
        print(f"ambientCG: geen materiaal voor '{keyword}'")
        return
    out = os.path.join(STAGING, "ambientcg")
    for aid in ids:
        fname = f"{aid}_{res}-JPG.zip"
        url = f"https://ambientcg.com/get?file={fname}"
        dest = os.path.join(out, fname)
        try:
            size = _download(url, dest)
            with zipfile.ZipFile(dest) as z:
                z.extractall(os.path.join(out, aid))
            print(f"  OK  {aid}  ({size//1024} KB)")
        except Exception as e:
            print(f"  FOUT {aid}: {e}")
    print(f"ambientCG klaar -> {out}")


# ---------------- Poly Haven ----------------
def polyhaven_list(atype, category=None):
    url = f"https://api.polyhaven.com/assets?type={atype}"
    if category:
        url += f"&categories={urllib.parse.quote(category)}"
    return list(_get_json(url).keys())


def _walk_urls(node, res, out_files):
    """Verzamel {url,name} voor de gevraagde resolutie uit de geneste files-JSON."""
    if isinstance(node, dict):
        if "url" in node and isinstance(node["url"], str):
            out_files.append(node["url"])
            if "include" in node and isinstance(node["include"], dict):
                for inc in node["include"].values():
                    if isinstance(inc, dict) and "url" in inc:
                        out_files.append(inc["url"])
            return
        for k, v in node.items():
            if k in ("1k", "2k", "4k", "8k", "16k") and k != res:
                continue
            _walk_urls(v, res, out_files)


def polyhaven_fetch(atype, count, res="2k", category=None):
    ids = polyhaven_list(atype, category)[:count]
    if not ids:
        print(f"Poly Haven: niets voor type={atype} categorie={category}")
        return
    out = os.path.join(STAGING, "polyhaven", atype)
    for aid in ids:
        try:
            files = _get_json(f"https://api.polyhaven.com/files/{aid}")
            urls = []
            _walk_urls(files, res, urls)
            urls = [u for u in dict.fromkeys(urls) if res in u or atype == "hdris"]
            if not urls:
                _walk_urls(files, res, urls)
            n = 0
            for u in urls:
                name = os.path.basename(urllib.parse.urlparse(u).path)
                _download(u, os.path.join(out, aid, name))
                n += 1
            print(f"  OK  {aid}  ({n} bestanden)")
        except Exception as e:
            print(f"  FOUT {aid}: {e}")
    print(f"Poly Haven klaar -> {out}")


def main():
    a = sys.argv[1:]
    if not a:
        print(__doc__)
        return
    cmd = a[0]
    if cmd == "ambientcg":
        ambientcg_fetch(a[1], int(a[2]) if len(a) > 2 else 5, a[3] if len(a) > 3 else "2K")
    elif cmd == "polyhaven":
        polyhaven_fetch(a[1], int(a[2]) if len(a) > 2 else 5,
                        a[3] if len(a) > 3 else "2k", a[4] if len(a) > 4 else None)
    elif cmd == "list-ambientcg":
        print("\n".join(ambientcg_list(a[1], int(a[2]) if len(a) > 2 else 20)) or "(geen)")
    elif cmd == "list-polyhaven":
        print("\n".join(polyhaven_list(a[1], a[2] if len(a) > 2 else None)[:40]) or "(geen)")
    else:
        print(__doc__)


if __name__ == "__main__":
    main()
