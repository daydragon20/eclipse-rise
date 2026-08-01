#!/usr/bin/env python3
"""The scene-script validator specified in phase0/SCRIPT_FORMAT.md section 6.

WHY THIS EXISTS, IN THE ARCHITECT'S OWN WORDS
---------------------------------------------
On 2026-07-31 five script errors were found by hand. Three of them were the
*architect's*, not the writers':

  * a `choice.set` on a flag nobody reads (decoration that reads as a choice)
  * a flag that grew three leaves at the SETTER (M1.5.S04) and stayed a boolean
    at the READER (M1.6.S06) -- a different mission, a different writer. One
    player in three would have got the wrong closing beat of a story thread,
    unfixable after generation. (RULING L1-R12)
  * conditions naming withdrawn flags, so those lines would never play -- but
    would still be generated and paid for.

"They were invisible from inside one mission and obvious from above."
Act 2 has 34 missions. That is what this tool is for.

THE TRAP THIS TOOL IS BUILT AGAINST
-----------------------------------
A check that produces confidence it has not earned is worse than no check.
That happened here the same evening: `check_voice_resolves.py` reported
`eclipse_fighter_a` as "CAST AND READY" because it validated key -> role and
never asked whether the *slot* had a voice bound.

Three structural defences, all of them load-bearing:

1. EVERY LINE OF EVERY FILE IS ACCOUNTED FOR. The YAML subset parser below is
   strict: a line it does not understand is a hard PARSE failure with a
   file:line, never a silent skip. A validator that quietly ignores the half of
   a file it cannot read is the fastest possible way to build a green bar that
   means nothing.
2. NOTHING IS RESTATED FROM CANON -- IT IS READ FROM CANON. Bands, the tag set,
   the glossary, the location registry, the flag register and the `run.` facts
   are parsed out of the documents that own them. A second copy of a truth is a
   divergence bug waiting (L1-R3), and this tool exists to catch exactly that
   class of bug.
3. EACH PARSED SOURCE HAS A SHAPE GUARD. If a doc is renamed, restructured or
   emptied, the loader raises instead of returning an empty set. An empty
   allow-list makes every check pass. That failure mode is silent, and it is
   the one that would kill this tool.

And: `Tools/tests/test_validate_script.py` holds one fixture per check, built
from the real errors of that evening, proving each check can go RED. A
validator that has only ever been green on the current corpus measures nothing.

VOICE RESOLUTION IS NOT REIMPLEMENTED. `check_voice_resolves.py` already owns
it (slot binding included, which is the part that was wrong once). This tool
imports and calls it. Two implementations of one question is how you get two
answers and no way to tell which is the truth.

Run:  python Eclipse/Tools/validate_script.py
      python Eclipse/Tools/validate_script.py --explain     (dump every table it read)
      python Eclipse/Tools/validate_script.py --root DIR    (validate a fixture tree)
      python Eclipse/Tools/validate_script.py --no-voice    (skip the delegated voice gate)

Exit: 0 = clean   1 = findings   2 = a source document changed shape / parse failure
"""
from __future__ import annotations

import argparse
import io
import re
import sys
from collections import defaultdict
from contextlib import redirect_stdout
from dataclasses import dataclass, field
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
ECLIPSE = TOOLS.parent
ROOT = ECLIPSE.parent

SCRIPTS = ECLIPSE / "Content" / "Script"
SCRIPT_FORMAT = ROOT / "phase0" / "SCRIPT_FORMAT.md"
ACT1_OVERVIEW = ROOT / "phase0" / "beats" / "ACT1_OVERVIEW.md"
INDEX = ROOT / "00_INDEX.md"
VOICE_PRODUCTION = ROOT / "19_voice_production.md"
EVENT_CATALOG = ECLIPSE / "Docs" / "EventCatalog.md"


class SourceShapeError(Exception):
    """A canon document no longer has the shape this tool reads it with.

    Raised instead of returning an empty set. An empty allow-list silently
    passes every check that consults it, which is the exact failure this whole
    file is written against.
    """


# ---------------------------------------------------------------------------
# 1. The canon loaders. Every table this tool checks against is READ, not
#    restated -- and every one of them is shape-guarded.
# ---------------------------------------------------------------------------

def _section(text: str, heading_re: str) -> str:
    """The body of one markdown section, up to the next heading of same depth."""
    m = re.search(heading_re, text, re.M)
    if not m:
        raise SourceShapeError(f"section not found: {heading_re}")
    depth = len(m.group(0)) - len(m.group(0).lstrip("#"))
    rest = text[m.end():]
    nxt = re.search(r"^#{1,%d} " % depth, rest, re.M)
    return rest[: nxt.start()] if nxt else rest


def load_bands() -> dict[str, tuple[int | None, int]]:
    """`type` -> (register floor, ceiling), from SCRIPT_FORMAT section 4.

    Bands are CEILINGS (RULING L1-R1). The lower number is the register the
    scene should reach *somewhere*, never a per-line minimum -- a validator
    that fails a line for being short is a broken validator, and SCRIPT_FORMAT
    says so in as many words about its own canonical example ("Thirteen.").
    """
    body = _section(SCRIPT_FORMAT.read_text("utf-8"),
                    r"^#+ `type` values.*$")
    bands: dict[str, tuple[int | None, int]] = {}
    for key, span in re.findall(r"^\|\s*`([\w-]+)`\s*\|\s*([^|]+?)\s*\|", body, re.M):
        rng = re.match(r"(\d+)\s*[\u2013\u2014-]\s*(\d+)", span)
        if rng:
            bands[key] = (int(rng.group(1)), int(rng.group(2)))
            continue
        cap = re.search(r"up to (\d+)", span)
        if cap:  # `oration`: a ceiling with no register floor
            bands[key] = (None, int(cap.group(1)))
    if len(bands) < 6:
        raise SourceShapeError(
            f"SCRIPT_FORMAT section 4 band table: read {len(bands)} bands, expected >=6")
    return bands


def load_tag_set() -> set[str]:
    """The approved audio tags, from 19_voice_production.md section 19.4."""
    body = _section(VOICE_PRODUCTION.read_text("utf-8"), r"^#+ Approved tag set\s*$")
    tags = {t.lower() for t in re.findall(r"`\[([^\]]+)\]`", body)}
    if len(tags) < 12:
        raise SourceShapeError(
            f"19.4 approved tag set: read {len(tags)} tags, expected >=12")
    return tags


def load_glossary_names() -> set[str]:
    """Canon person-names, UPPERCASED, from the 00_INDEX.md glossary.

    Deliberately generous: it harvests every capitalised token from the
    Protagonist / Companions / Villains lines, so rank words (ARBITER,
    MARSHAL) are in the set too, plus any token the glossary already writes in
    full caps (AEGIS -- which speaks). It is not a title police. Its job is the
    one L1-R6 gives it: catch a speaker that is not a canon name at all, and
    catch a NEAR-MISS on one, which is the failure the check exists for.
    """
    body = _section(INDEX.read_text("utf-8"), r"^#+ Canon Glossary.*$")
    names: set[str] = set()
    for label in ("Protagonist", "Companions", "Villains"):
        m = re.search(r"^\*\*%s:\*\*(.*)$" % label, body, re.M)
        if not m:
            raise SourceShapeError(f"glossary line missing: {label}")
        for tok in re.findall(r"\b[A-Z][A-Za-z'\u2019-]+\b", m.group(1)):
            names.add(tok.upper().strip("'\u2019"))
    names |= {t for t in re.findall(r"\b[A-Z]{3,}\b", body)}
    if len(names) < 10:
        raise SourceShapeError(f"glossary: read {len(names)} names, expected >=10")
    return names


def load_locations() -> set[str]:
    """Act-1 location strings, from ACT1_OVERVIEW section 7.

    SCRIPT_FORMAT section 6 originally pointed at 03_world_design.md; finding
    C-1 established that document has no district names, so section 7 is the
    canonical source until L4 lands.
    """
    body = _section(ACT1_OVERVIEW.read_text("utf-8"), r"^#+ 7\. Locatieregister.*$")
    locs = {m for m in re.findall(r"^\|\s*`([^`]+)`\s*\|", body, re.M)}
    if len(locs) < 10:
        raise SourceShapeError(f"ACT1_OVERVIEW section 7: read {len(locs)} locations, expected >=10")
    return locs


CATEGORY = ("choice", "beat", "char", "clue", "intel", "thread", "flag")


def fact_key(name: str) -> str:
    """Normalise a flag name so a script-side spelling and a gameplay tag meet.

    There is no mechanical rule in canon -- SCRIPT_FORMAT itself pairs
    `story.brick_recruited` with `Story.Beat.BrickRecruited` AND
    `story.m11_conscript_choice` with `Story.Choice.M11_Conscript`, which are
    two different conventions. So this normalises both sides down to what they
    genuinely share: letters and digits, minus the category word wherever it
    sits.

        story.m11_conscript_choice        -> m11conscript
        Story.Choice.M11_Conscript.Bound  -> m11conscript
        story.char_maradead               -> maradead
        Story.Char.MaraDead               -> maradead

    Run `--explain` to audit every binding this produces. A binder you cannot
    read is a binder you cannot trust, and an unaudited match is exactly the
    "cast and ready" failure in a new costume.
    """
    s = re.sub(r"^(story|run)\.", "", name, flags=re.I)
    s = re.sub(r"[^a-z0-9]", "", s.lower())
    for cat in CATEGORY:
        if s.startswith(cat) and len(s) > len(cat):
            return s[len(cat):]
        if s.endswith(cat) and len(s) > len(cat):
            return s[: -len(cat)]
    return s


@dataclass
class RegisterRow:
    tag: str
    leaves: tuple[str, ...]
    set_by: str
    read_by: str


def load_flag_register() -> dict[str, RegisterRow]:
    """The act-1 flag register, ACT1_OVERVIEW section 6, keyed by fact_key().

    Section 6 is the register the architect keeps, and its own rule is: "geen
    scene gebruikt een vlag in condition: voordat de tag bestaat". Its
    "gelezen door" column is, in its own words, "niet documentatie -- hij is de
    veiligheidscontrole", and L1-R12 is the record of what happens when it is
    not used.
    """
    body = _section(ACT1_OVERVIEW.read_text("utf-8"), r"^#+ 6\. Vlaggenregister.*$")
    rows: dict[str, RegisterRow] = {}
    ambiguous: dict[str, list[str]] = defaultdict(list)

    def add(tag: str, set_by: str = "", read_by: str = "") -> None:
        leaves: tuple[str, ...] = ()
        brace = re.search(r"\{([^}]*)\}", tag)
        if brace:
            leaves = tuple(x.strip().lower() for x in brace.group(1).split(",") if x.strip())
            tag = tag[: brace.start()].rstrip(".")
        key = fact_key(tag)
        ambiguous[key].append(tag)
        rows[key] = RegisterRow(tag, leaves, set_by.strip(), read_by.strip())

    # TABLE ROWS ONLY, PLUS THE "BESTAAND IN CODE" LINE.
    #
    # The first version of this loader swept every `Story.*` in backticks
    # anywhere in section 6. That silently registered
    # `Story.Choice.M11_ConscriptSpared` -- a flag the prose around it declares
    # WITHDRAWN (L1-R3) -- as a legitimate row, which would have made a
    # condition on a dead flag resolve cleanly. That is the exact shape of the
    # "CAST AND READY" failure this tool exists to prevent, and it was in the
    # tool itself. Prose is where flags get killed; only the table and the
    # code line say a flag lives.
    in_code_block = False
    for line in body.splitlines():
        if line.startswith("#"):
            in_code_block = "Bestaand in code" in line
            continue
        cells = [c.strip() for c in line.split("|")]
        if len(cells) >= 5 and cells[1].startswith("`Story."):
            add(cells[1].strip("`"), cells[2], cells[3])
        elif in_code_block:
            for tag in re.findall(r"`(Story\.[\w.{},]+)`", line):
                add(tag, set_by="EclipseGameplayTags.cpp", read_by="")

    # Second pass, belt and braces: a tag named in a sentence that retires it
    # is gone, even if a stale table row still carries it.
    for line in body.splitlines():
        if re.search(r"ingetrokken|withdrawn", line, re.I):
            for tag in re.findall(r"`(Story\.[\w.{},]+)`", line):
                rows.pop(fact_key(tag), None)

    for key, tags in ambiguous.items():
        if len(set(tags)) > 1 and key in rows:
            raise SourceShapeError(
                f"two register tags normalise to the same fact `{key}`: {sorted(set(tags))}. "
                "The binder cannot pick one and will not guess.")
    if len(rows) < 15:
        raise SourceShapeError(f"ACT1_OVERVIEW section 6: read {len(rows)} flags, expected >=15")
    return rows


def load_run_facts() -> set[str]:
    """The `run.` facts canon declares (SCRIPT_FORMAT section 4 / RULING L1-R4).

    Run facts live in FEclipseMissionOutcome and are set by systems, so they
    can never have a script-side setter. They are the one legitimate reason a
    `condition` resolves without any `choice.set` behind it -- which makes this
    list the tool's only blind spot, and the reason it is read from canon
    rather than typed here where it could quietly grow.
    """
    body = _section(SCRIPT_FORMAT.read_text("utf-8"),
                    r"^#+ `condition` and the two state namespaces.*$")
    facts = {f"run.{m}" for m in re.findall(r"`run\.(\w+)`", body)}
    if len(facts) < 3:
        raise SourceShapeError(f"SCRIPT_FORMAT section 4: read {len(facts)} run facts, expected >=3")
    return facts


def load_event_tags() -> set[str]:
    if not EVENT_CATALOG.is_file():
        raise SourceShapeError(f"missing {EVENT_CATALOG}")
    tags = set(re.findall(r"`(Event\.[\w.]+)`", EVENT_CATALOG.read_text("utf-8")))
    if not tags:
        raise SourceShapeError("EventCatalog.md documents no Event.* tags")
    return tags


# ---------------------------------------------------------------------------
# 2. The YAML subset parser.
#
#    Strict on purpose. It accepts exactly the shapes SCRIPT_FORMAT sections 3
#    and 5 define and rejects everything else with a file:line. PyYAML is not
#    installed and cannot be (no admin rights), so this is stdlib-only -- which
#    means the danger is a hand parser that MIS-reads a file and passes it.
#    Hence: unknown line shape = hard failure, never a skip.
# ---------------------------------------------------------------------------

SCALAR_KEY = re.compile(r"^(?P<key>[A-Za-z_][\w]*):(?:\s+(?P<val>\S.*?))?\s*$")
ITEM_KEY = re.compile(r"^-\s+(?P<key>[A-Za-z_][\w]*):(?:\s+(?P<val>\S.*?))?\s*$")


class ParseError(Exception):
    pass


def _scalar(raw: str | None):
    if raw is None:
        return None
    s = raw.strip()
    if s.startswith('"') and s.endswith('"') and len(s) >= 2:
        return s[1:-1].replace('\\"', '"')
    if s.startswith("'") and s.endswith("'") and len(s) >= 2:
        return s[1:-1].replace("''", "'")
    if s.startswith("[") and s.endswith("]"):
        inner = s[1:-1].strip()
        if not inner:
            return []
        return [_scalar(p.strip()) for p in inner.split(",")]
    if s == "null":
        return None
    if s in ("true", "false"):
        return s == "true"
    if re.fullmatch(r"-?\d+", s):
        return int(s)
    return s


def _ends_inside_quote(s: str) -> bool:
    """True if the line stops in the middle of a quoted scalar.

    Writers wrap long `note:` and `shot:` values over two lines; that is a real
    YAML folded flow scalar, not a typo, and P0.S01 already has one. Treating
    the continuation as a broken field would have been a false red -- and a
    false red is more expensive than no bar at all, because it teaches people
    to wave the bar away.
    """
    dq = sq = False
    i = 0
    while i < len(s):
        c = s[i]
        if dq:
            if c == "\\":
                i += 2
                continue
            if c == '"':
                dq = False
        elif sq:
            if c == "'":
                if i + 1 < len(s) and s[i + 1] == "'":
                    i += 2
                    continue
                sq = False
        elif c == '"':
            dq = True
        elif c == "'":
            sq = True
        elif c == "#":
            break
        i += 1
    return dq or sq


def logical_lines(text: str) -> list[tuple[int, str]]:
    """(first-line-number, folded-line) pairs. Folds multi-line quoted scalars."""
    phys = text.splitlines()
    out: list[tuple[int, str]] = []
    i = 0
    while i < len(phys):
        start, raw = i + 1, phys[i]
        stripped = raw.strip()
        if stripped and not stripped.startswith("#"):
            while _ends_inside_quote(raw) and i + 1 < len(phys):
                i += 1
                raw = raw.rstrip() + " " + phys[i].strip()
        out.append((start, raw))
        i += 1
    return out


@dataclass
class Node:
    """A parsed mapping plus, for every key, the 1-based line it came from."""
    data: dict = field(default_factory=dict)
    where: dict = field(default_factory=dict)

    def __contains__(self, k):
        return k in self.data

    def get(self, k, default=None):
        return self.data.get(k, default)

    def line(self, k, default=0):
        return self.where.get(k, default)


def parse_scene_file(path: Path) -> tuple[Node, list[Node]]:
    """Return (header, items). Raises ParseError on anything unrecognised."""
    header = Node()
    items: list[Node] = []
    seq_key: str | None = None
    map_key: str | None = None
    cur: Node | None = None
    nested: Node | None = None
    nested_indent = -1

    for n, raw in logical_lines(path.read_text("utf-8")):
        if not raw.strip() or raw.lstrip().startswith("#"):
            continue
        if _ends_inside_quote(raw):
            raise ParseError(f"{path}:{n}: quoted value never closes: {raw.strip()[:60]!r}")
        indent = len(raw) - len(raw.lstrip(" "))
        body = raw.strip()

        if indent == 0:
            m = SCALAR_KEY.match(body)
            if not m:
                raise ParseError(f"{path}:{n}: not a top-level `key: value`: {body!r}")
            key, val = m.group("key"), m.group("val")
            cur = nested = None
            nested_indent = -1
            if val is None:
                # Bewust een KLEINE lijst. De kop kende alleen sequenties
                # (`lines:`), en een blok `key: value` eronder viel om met
                # "field before any `- ` item". `silence:` heeft een mapping
                # nodig; alles wat NIET in HEADER_MAPS staat gedraagt zich
                # exact zoals eerst, zodat lines/variants niets merken.
                if key in HEADER_MAPS:
                    seq_key = None
                    map_key = key
                    header.data.setdefault(key, {"__line__": n})
                    header.where[key] = n
                    continue
                seq_key = key            # `lines:` / `variants:` block sequence
                map_key = None
                header.data.setdefault(key, [])
                header.where[key] = n
            else:
                seq_key = None
                map_key = None
                header.data[key] = _scalar(val)
                header.where[key] = n
            continue

        if map_key is not None:
            m = SCALAR_KEY.match(body)
            if not m or m.group("val") is None:
                raise ParseError(f"{path}:{n}: `{map_key}:` verwacht `waarde: reden`: {body!r}")
            header.data[map_key][m.group("key")] = _scalar(m.group("val"))
            continue

        if seq_key is None:
            raise ParseError(f"{path}:{n}: indented line outside any block: {body!r}")

        if body.startswith("- "):
            m = ITEM_KEY.match(body)
            if not m:
                raise ParseError(f"{path}:{n}: sequence item is not `- key: value`: {body!r}")
            cur = Node()
            cur.data["__line__"] = n
            items.append(cur)
            header.data[seq_key].append(cur)
            nested = None
            nested_indent = -1
            cur.data[m.group("key")] = _scalar(m.group("val"))
            cur.where[m.group("key")] = n
            continue

        if cur is None:
            raise ParseError(f"{path}:{n}: field before any `- ` item: {body!r}")

        m = SCALAR_KEY.match(body)
        if not m:
            raise ParseError(f"{path}:{n}: not a `key: value` field: {body!r}")
        key, val = m.group("key"), m.group("val")

        if nested is not None and indent > nested_indent:
            nested.data[key] = _scalar(val)
            nested.where[key] = n
            continue

        nested = None
        nested_indent = -1
        if val is None:
            nested = Node()
            nested.data["__line__"] = n
            nested_indent = indent
            cur.data[key] = nested
            cur.where[key] = n
        else:
            cur.data[key] = _scalar(val)
            cur.where[key] = n

    return header, items


# ---------------------------------------------------------------------------
# 3. Findings
# ---------------------------------------------------------------------------

CHECKS = {
    "PARSE": "File shape -- a line this tool cannot read is never skipped",
    "SCHEMA": "Missing required field, unknown field, malformed grammar",
    "ID": "ID uniqueness, prefix and monotonicity (SCRIPT_FORMAT section 2)",
    "SPEAKER": "Speaker not a canon name and not a role speaker (L1-R6/R6b)",
    "VOICE": "Voice key resolves -- delegated to check_voice_resolves.py",
    "CEILING": "Line above its band's upper number (bands are ceilings, L1-R1)",
    "REGISTER": "Scene of >=6 lines that never reaches its band's lower number",
    "SPREAD": "Scene of >=6 lines whose longest / shortest is < 3x (18.3)",
    "TAG": "Tag outside the 19.4 approved set without a note, or more than two",
    "CHOICE": "Choice integrity -- lone option, mixed flags, or a set nobody reads",
    "CONDITION": "Condition resolves -- a fact nothing ever sets",
    "BRANCH": "Branch completeness -- N leaves set, fewer than N handled",
    "FLAGREG": "Flag used in production with no row in ACT1_OVERVIEW section 6",
    "NAMESLOT": "{name} line is not pronoun-free (L1-R8)",
    "TRIGGER": "Bark trigger not in Docs/EventCatalog.md",
    "LOCATION": "location not in the act's location registry (section 7, finding C-1)",
    "GUARD": "status: generated on a scene whose critic is not GO -- the money-saver",
    "SILENCE": "A declared silent branch that is empty, stale, or unknown (L1 must state WHY)",
}


@dataclass
class Finding:
    code: str
    where: str
    message: str

    def __str__(self) -> str:
        return f"  {self.where:<52} {self.message}"


# Kopvelden die een MAPPING zijn in plaats van een sequentie. Klein houden:
# elke naam hier verandert hoe de parser een blok leest.
HEADER_MAPS = ("silence",)

SCENE_REQUIRED = ("scene", "mission", "title", "act", "location", "type",
                  "credit_tier", "want", "obstacle", "turn", "status", "critic", "lines")
SCENE_OPTIONAL = ("words", "words_heard", "words_generated", "silence")
BARK_REQUIRED = ("bark_set", "trigger", "faction", "type", "credit_tier",
                 "status", "critic", "variants")
BARK_OPTIONAL = ("words", "words_heard", "words_generated")
LINE_REQUIRED = ("id", "speaker", "voice", "text")
LINE_OPTIONAL = ("tags", "band", "shot", "condition", "choice", "variants",
                 "interrupts", "direct_beat", "note", "__line__")
VARIANT_REQUIRED = ("id", "voice", "text")
VARIANT_OPTIONAL = ("tags", "note", "band", "condition", "speaker", "__line__")
CHOICE_REQUIRED = ("group", "set")
CHOICE_OPTIONAL = ("label", "__line__")
AXES = ("pragmatist", "idealist", "personal", "strategic")

# CHORUS added by L1-R30. The alternation was already a faction vocabulary
# everywhere except FIGHTER, which silently meant Ember -- so the first scene
# with a second cell's riflemen in it had nowhere to put them and took Ember's
# token and Ember's voice pool with it.
ROLE_SPEAKER = re.compile(
    r"^(FIGHTER|CONSCRIPT|VEIL|CIVILIAN|OFFICER|PRISONER|CHORUS)_[A-Z]$")
NAMED_ROLE = {"EMISSARY"}          # L1-R6b
COND_GRAMMAR = re.compile(r'^(story|run)\.[a-z0-9_]+\s*(==|!=)\s*(".*?"|true|false)$')
SET_GRAMMAR = re.compile(r'^(story|run)\.[a-z0-9_]+\s*=\s*(".*?"|true|false)$')
PRONOUN = re.compile(r"\b(he|him|his|she|her|hers|himself|herself)\b", re.I)


@dataclass
class Fact:
    """Everything the corpus knows about one story./run. fact."""
    name: str
    readers: list = field(default_factory=list)      # (where, op, value)
    setters: list = field(default_factory=list)      # (where, value, scene)
    scene_reads: dict = field(default_factory=lambda: defaultdict(set))


class Validator:
    def __init__(self, root: Path, canon: dict):
        self.root = root
        self.canon = canon
        self.findings: list[Finding] = []
        self.facts: dict[str, Fact] = {}
        self.ids: dict[str, str] = {}
        self.scenes = 0
        self.written = 0
        self.stubs = 0
        self.barks = 0
        self.lines = 0
        # scene -> {vlagwaarde: reden}. Een BEWUST stille tak, opgeschreven.
        self.silences: dict[str, dict] = {}
        self.silence_used: set = set()
        # Feiten die op EEN registerrij worden vertrouwd in plaats van op een
        # `set:`. Dat is een bewuste doorlaat (zie CONDITION RESOLVES), maar hij
        # was STIL -- en op 01-08 bleek dat er een van doorheen liep die twee
        # betaalde regels aanstuurt zonder dat iets hem ooit zet.
        self.register_trusted: set = set()

    def add(self, code: str, where: str, msg: str) -> None:
        self.findings.append(Finding(code, where, msg))

    def fact(self, name: str) -> Fact:
        return self.facts.setdefault(name, Fact(name))

    # -- schema helpers -----------------------------------------------------
    def _fields(self, node: Node, req, opt, where_file: str, what: str) -> None:
        for k in req:
            if k not in node.data or (node.get(k) is None and k not in ("critic",)):
                self.add("SCHEMA", f"{where_file}:{node.data.get('__line__', 0)}",
                         f"{what}: missing required field `{k}`")
        for k in node.data:
            if k not in req and k not in opt:
                self.add("SCHEMA", f"{where_file}:{node.line(k)}",
                         f"{what}: unknown field `{k}`")

    # -- one file -----------------------------------------------------------
    def visit(self, path: Path) -> None:
        rel = path.relative_to(self.root).as_posix()
        try:
            header, items = parse_scene_file(path)
        except ParseError as e:
            self.add("PARSE", rel, str(e).split(": ", 1)[-1])
            return

        if "bark_set" in header:
            self.barks += 1
            self.visit_bark(rel, header, items)
            return

        self.scenes += 1
        self._fields(header, SCENE_REQUIRED, SCENE_OPTIONAL, rel, "scene header")

        scene = header.get("scene")
        stem = path.stem
        if scene and not stem.startswith(str(scene)):
            self.add("SCHEMA", f"{rel}:{header.line('scene')}",
                     f"`scene: {scene}` does not match filename stem `{stem}`")

        stype = header.get("type")
        band = self.canon["bands"].get(stype)
        if stype is not None and band is None:
            self.add("SCHEMA", f"{rel}:{header.line('type')}",
                     f"unknown `type: {stype}` -- no band in SCRIPT_FORMAT section 4")

        act = header.get("act")
        loc = header.get("location")
        if act == 1 and loc not in self.canon["locations"]:
            self.add("LOCATION", f"{rel}:{header.line('location')}",
                     f"`{loc}` is not in the act-1 location registry "
                     f"(ACT1_OVERVIEW section 7)")

        # ---- `silence:` -- een tak die BEWUST leeg is -----------------------
        # Zonder dit veld is er geen verschil tussen "vergeten" en "hier hoort
        # niets te staan", en dan blijft BRANCH voor altijd rood op een scene
        # die de criticus expliciet heeft goedgekeurd. Een bar die altijd rood
        # staat verbergt evenveel als een test die nooit rood wordt.
        #
        # De prijs is met opzet een ZIN: je mag een tak stil laten, maar je moet
        # opschrijven waarom, en dat staat dan in het bestand in plaats van in
        # een chatvenster. Een stilte zonder reden is nog steeds een bevinding.
        sil = header.get("silence")
        if sil is not None:
            if not isinstance(sil, dict) or not sil:
                self.add("SCHEMA", f"{rel}:{header.line('silence')}",
                         "`silence:` moet een blok van waarde -> reden zijn, en niet leeg")
            else:
                clean = {}
                for val, reason in sil.items():
                    if val == "__line__":
                        continue
                    txt = str(reason or "").strip()
                    if len(txt) < 40:
                        self.add("SILENCE", f"{rel}:{header.line('silence')}",
                                 f"`silence: {val}` heeft geen reden van betekenis "
                                 f"({len(txt)} tekens). Een stilte die je niet kunt "
                                 "uitleggen is een vergissing die zich voordoet als "
                                 "een keuze -- schrijf op waarom die speler niets hoort.")
                    else:
                        clean[str(val)] = txt
                if clean:
                    self.silences[rel] = clean

        if header.get("status") == "generated":
            critic = str(header.get("critic") or "")
            if not critic.startswith("GO"):
                self.add("GUARD", f"{rel}:{header.line('status')}",
                         f"status: generated but critic is `{header.get('critic')}` -- "
                         "this line must never reach the API")

        if not items:
            self.stubs += 1
            return
        self.written += 1

        lengths: list[int] = []
        register_hits: list[int] = []
        variant_hits: list[int] = []
        prev_num = -1
        for it in items:
            self.lines += 1
            self._fields(it, LINE_REQUIRED, LINE_OPTIONAL, rel, "line")
            self.visit_line(rel, header, it, band, lengths, register_hits, variant_hits)
            prev_num = self.check_id(rel, scene, it, prev_num)

        self.check_shape(rel, header, stype, band, items, lengths, register_hits, variant_hits)

    def visit_bark(self, rel: str, header: Node, items: list[Node]) -> None:
        self._fields(header, BARK_REQUIRED, BARK_OPTIONAL, rel, "bark header")
        trigger = header.get("trigger")
        if trigger and trigger not in self.canon["events"]:
            self.add("TRIGGER", f"{rel}:{header.line('trigger')}",
                     f"`{trigger}` is not in Docs/EventCatalog.md -- that is a systems "
                     "task, not a writing task (SCRIPT_FORMAT section 5)")
        if items and not 6 <= len(items) <= 12:
            self.add("SCHEMA", f"{rel}:{header.line('variants')}",
                     f"{len(items)} variants; section 18.5 wants 6-12")
        band = self.canon["bands"].get(header.get("type"))
        for it in items:
            self.lines += 1
            self._fields(it, VARIANT_REQUIRED, VARIANT_OPTIONAL, rel, "bark variant")
            self.visit_line(rel, header, it, band, [], [], [])

    # -- one line -----------------------------------------------------------
    def visit_line(self, rel: str, header: Node, it: Node,
                   band, lengths: list[int], register_hits: list[int],
                   variant_hits: list[int]) -> None:
        at = f"{rel}:{it.data.get('__line__', 0)}"
        lid = it.get("id") or "?"

        speaker = it.get("speaker")
        if speaker is not None:
            if not (speaker in self.canon["names"]
                    or ROLE_SPEAKER.match(str(speaker))
                    or speaker in NAMED_ROLE):
                self.add("SPEAKER", at,
                         f"{lid}: speaker `{speaker}` is not a canon name and not a role "
                         "speaker (L1-R6/R30: FIGHTER|CONSCRIPT|VEIL|CIVILIAN|OFFICER|"
                         "PRISONER|CHORUS + _A..Z, or EMISSARY)")

        tags = it.get("tags") or []
        if isinstance(tags, str):
            tags = [tags]
        if len(tags) > 2:
            self.add("TAG", at, f"{lid}: {len(tags)} tags; 19.4 rule 3 allows at most two")
        for t in tags:
            if str(t).lower() not in self.canon["tags"] and "note" not in it:
                self.add("TAG", at, f"{lid}: tag `[{t}]` is outside the 19.4 approved set "
                                    "and the line carries no `note` explaining why")

        if it.get("direct_beat") is True and "note" not in it:
            self.add("SCHEMA", at, f"{lid}: `direct_beat: true` requires a one-line "
                                   "justification in `note` (18.9 A)")

        text = it.get("text")
        line_band = self.canon["bands"].get(it.get("band"), band) if it.get("band") else band
        if it.get("band") and it.get("band") not in self.canon["bands"]:
            self.add("SCHEMA", at, f"{lid}: unknown `band: {it.get('band')}`")

        # A variant is a spoken line too: it is generated, it is heard, and it
        # costs. It is checked against the ceiling and excluded from the
        # scene's register/spread, which measure the scene's shape (L1-R15:
        # `words` and `words_heard` are different questions).
        spoken = [("", text)]
        variants = it.get("variants")
        if isinstance(variants, Node):
            for k, val in variants.data.items():
                if k == "__line__":
                    continue
                if k not in AXES:
                    self.add("SCHEMA", at, f"{lid}: unknown variant axis `{k}`")
                    continue
                spoken.append((k, val))

        for label, txt in spoken:
            if not isinstance(txt, str):
                continue
            n = len(txt.split())
            if label == "":
                lengths.append(n)
            if line_band and n > line_band[1]:
                self.add("CEILING", at,
                         f"{lid}{('.' + label) if label else ''}: {n} words, ceiling is "
                         f"{line_band[1]} -- \"{txt[:48]}...\"")
            if label == "" and not it.get("band") and line_band and line_band[0] \
                    and n >= line_band[0]:
                register_hits.append(n)
            # Varianten tellen NIET mee voor het register -- dat is opzet, anders
            # haalt een scene zijn ondergrens met tekst die de meeste spelers nooit
            # horen. Maar ze worden wel APART geteld, want er is een geval waarin
            # dat het oordeel omdraait: staat de variantset op de WENDING, dan
            # hoort elke speler er een, en dan is de bevinding waar over het
            # bestand en onwaar over de speelbeurt (M1.2.S01). Dat is geen reden
            # om de check te versoepelen; het is een reden om het erbij te zeggen.
            elif label and not it.get("band") and line_band and line_band[0] \
                    and n >= line_band[0]:
                variant_hits.append(n)
            if "{name}" in txt and PRONOUN.search(txt):
                self.add("NAMESLOT", at,
                         f"{lid}: uses {{name}} and a pronoun in the same line -- the roster "
                         "decides gender at runtime (L1-R8)")

        cond = it.get("condition")
        if isinstance(cond, str):
            if not COND_GRAMMAR.match(cond.strip()):
                self.add("SCHEMA", at, f"{lid}: condition does not parse: `{cond}`")
            else:
                name, op, val = self._split(cond, "==|!=")
                f = self.fact(name)
                f.readers.append((at, op, val, lid))
                if op == "==":
                    f.scene_reads[rel].add(val)
                else:
                    f.scene_reads[rel].add(("!", val))

        ch = it.get("choice")
        if isinstance(ch, Node):
            self._fields(ch, CHOICE_REQUIRED, CHOICE_OPTIONAL, rel, f"{lid}: choice")
            raw = ch.get("set")
            if isinstance(raw, str):
                for assign in [a.strip() for a in raw.split(";") if a.strip()]:
                    if not SET_GRAMMAR.match(assign):
                        self.add("SCHEMA", f"{rel}:{ch.line('set')}",
                                 f"{lid}: choice.set does not parse: `{assign}`")
                        continue
                    name, _, val = self._split(assign, "=")
                    self.fact(name).setters.append(
                        (f"{rel}:{ch.line('set')}", val, ch.get("group"), lid))

    @staticmethod
    def _split(expr: str, ops: str):
        m = re.match(r'^\s*([\w.]+)\s*(%s)\s*(.+?)\s*$' % ops, expr)
        name, op, val = m.group(1), m.group(2), m.group(3)
        return name, op, val.strip('"')

    def check_id(self, rel: str, scene, it: Node, prev_num: int) -> int:
        lid = it.get("id")
        at = f"{rel}:{it.data.get('__line__', 0)}"
        if not isinstance(lid, str):
            return prev_num
        if lid in self.ids:
            self.add("ID", at, f"duplicate id `{lid}` -- also at {self.ids[lid]}")
        else:
            self.ids[lid] = at
        if scene and not lid.startswith(f"{scene}."):
            self.add("ID", at, f"`{lid}` does not start with its scene `{scene}.`")
        tail = lid.rsplit(".", 1)[-1]
        if not tail.isdigit():
            self.add("ID", at, f"`{lid}` does not end in a number")
            return prev_num
        num = int(tail)
        if num <= prev_num:
            self.add("ID", at, f"`{lid}` is out of order (previous was {prev_num:03d})")
        # NO "MUST BE A MULTIPLE OF TEN" CHECK, AND THAT IS DELIBERATE.
        #
        # The first draft had one, and it fired on `M1.3.S05.125` and
        # `M1.5.S01.154` -- which are exactly what SCRIPT_FORMAT section 2
        # DESIGNED the gaps for: "a line can be inserted at 015 six weeks later
        # without renumbering the scene". Section 6 asks only for uniqueness
        # and monotonicity. The check was inventing a rule the spec it enforces
        # explicitly permits, and it would have stood red forever on three
        # correct lines. A validator gets its rules from canon or it does not
        # get to have them.
        return max(num, prev_num)

    def check_shape(self, rel, header, stype, band, items, lengths, register_hits,
                    variant_hits=None) -> None:
        """Register and spread -- scenes of >=6 lines only (L1-R1)."""
        if len(items) < 6 or not band or not lengths:
            return
        floor, _ = band
        gauged = [it for it in items if not it.get("band")]
        if floor and gauged and not register_hits:
            # Als de VARIANTEN de ondergrens wel halen, hoort dat in de bevinding.
            # Niet als vrijstelling -- de check blijft rood -- maar omdat het het
            # oordeel omdraait: staat die set op de wending, dan hoort elke speler
            # er een en is de bevinding waar over het BESTAND en onwaar over de
            # SPEELBEURT. Een bevinding die dat verzwijgt laat een schrijver een
            # goede scene repareren.
            staart = ""
            if variant_hits:
                staart = (f" -- LET OP: {len(variant_hits)} VARIANT(EN) halen {floor} wel "
                          f"(langste {max(variant_hits)}). Varianten tellen hier met opzet "
                          "niet mee, anders haalt een scene zijn register met tekst die de "
                          "meeste spelers nooit horen. Maar staat die set op de wending, dan "
                          "hoort elke speler er een: kijk voor je iets herschrijft.")
            self.add("REGISTER", f"{rel}:{header.line('type')}",
                     f"{len(items)}-line `{stype}` scene: no un-overridden line reaches "
                     f"{floor} words (longest is {max(lengths)}) -- the scene never uses "
                     "the room its delivery context gives it" + staart)
        lo, hi = min(lengths), max(lengths)
        if lo and hi / lo < 3:
            self.add("SPREAD", f"{rel}:{header.line('type')}",
                     f"{len(items)}-line scene: longest/shortest = {hi}/{lo} = "
                     f"{hi / lo:.1f}x, below the 3x that 18.3 asks -- every turn the same "
                     "length is the loudest tell that a machine wrote it (18.9 D)")

    # -- corpus-wide --------------------------------------------------------
    def cross_check(self) -> None:
        register = self.canon["register"]
        run_facts = self.canon["run_facts"]

        for name, f in sorted(self.facts.items()):
            row = register.get(fact_key(name))
            declared = name in run_facts

            # --- CONDITION RESOLVES ---------------------------------------
            if f.readers and not f.setters and not row and not declared:
                first = f.readers[0]
                self.add("CONDITION", first[0],
                         f"`{name}` is read by {len(f.readers)} line(s) and set by NOTHING: "
                         "no choice.set in the corpus, no row in the ACT1_OVERVIEW section 6 "
                         "flag register, not a declared run. fact. Those lines never play, "
                         "and they still generate and still cost.")
            elif f.readers and not f.setters and row and row.leaves and not declared:
                # The register says this is a MULTI-LEAF choice, and a
                # multi-leaf choice is by definition something the player
                # picks -- so it needs `choice:` options to be pickable. A
                # single-leaf flag is not reported here: plenty of those are
                # legitimately set by a scene commit or by a system, and
                # firing on them would make this check noise. Noise is how a
                # bar stops being read.
                self.add("CONDITION", f.readers[0][0],
                         f"`{name}` is read by {len(f.readers)} line(s) and no scene sets it. "
                         f"The section 6 register has `{row.tag}` with "
                         f"{len(row.leaves)} leaves, set by {row.set_by or 'an unnamed scene'!r} "
                         "-- so a `choice:` block should exist and does not. Either the "
                         "options are still marked with `note:` only (pre-L1-R2), or reader "
                         "and setter have drifted apart (the L1-R12 shape).")

            elif f.readers and not f.setters and row and not row.leaves and not declared:
                # EEN BLAD, GEEN ZETTER, WEL EEN REGISTERRIJ. Bewust geen
                # bevinding: veel eenbladige vlaggen worden legitiem door een
                # scene-commit of door een systeem gezet, en daarop vuren maakt
                # deze check ruis -- en ruis is hoe een bar ophoudt gelezen te
                # worden. Maar de doorlaat was STIL, en dat is iets anders dan
                # verantwoord. Hij wordt nu getoond met de zetter die het
                # register CLAIMT, zodat een mens kan zien of die zetter bestaat.
                self.register_trusted.add(
                    (f.readers[0][0], name, len(f.readers), row.set_by or "(geen zetter genoemd)"))

            # --- CHOICE INTEGRITY: a set nobody reads ---------------------
            if f.setters and not f.readers:
                self.add("CHOICE", f.setters[0][0],
                         f"`{name}` is set by {len(f.setters)} choice option(s) and read by "
                         "NOTHING -- a set on a flag nobody reads is a choice without a "
                         "consequence, and it reads to the player as a real one.")

            # --- FLAG REGISTER --------------------------------------------
            # Only for facts that DO have a script setter. A fact with no
            # setter at all is already reported once by CONDITION above, and
            # one problem reported twice reads as two problems.
            if name.startswith("story.") and not row and f.setters:
                self.add("FLAGREG", f.setters[0][0],
                         f"`{name}` is in production ({len(f.setters)} setter(s), "
                         f"{len(f.readers)} reader(s)) with no row in the ACT1_OVERVIEW "
                         "section 6 flag register -- so it has no gameplay tag and no "
                         "`gelezen door` column watching it. Section 6's own rule: no "
                         "scene uses a flag in a condition before the tag exists.")

            self.check_branches(name, f, row)

        self.check_groups()

    def check_branches(self, name: str, f: Fact, row: RegisterRow | None) -> None:
        """N leaves set -> N leaves handled. The costliest one missed on 07-31."""
        set_vals = {v for _, v, _, _ in f.setters}
        read_eq = {v for _, op, v, _ in f.readers if op == "=="}
        read_ne = {v for _, op, v, _ in f.readers if op == "!="}
        universe = set_vals | read_eq | read_ne | set(row.leaves if row else ())
        universe -= {"true", "false"}
        if len(universe) < 2:
            return

        # A `!= "none"` line plays for EVERY value except "none". Counting only
        # `==` readers reported `story.m15_pact = "limited"` as an empty branch
        # when four lines play for it -- caught by hand-checking the file this
        # tool had just accused. A check you have not tried to disprove is a
        # rumour.
        covered_somewhere = read_eq | {v for v in universe if any(v != w for w in read_ne)}
        # Een BEWUST stille tak telt hier ook, maar alleen als hij gedeclareerd
        # is in een scene die deze vlag daadwerkelijk LEEST. Anders zou een
        # stilte in het ene verhaal een gat in het andere afdekken.
        stil_hier = set()
        for sc in f.scene_reads:
            for val in self.silences.get(sc, {}):
                stil_hier.add(val)
                if val in set_vals - covered_somewhere:
                    self.silence_used.add((sc, name, val))
        for v in sorted(set_vals - covered_somewhere - stil_hier - {"true", "false"}):
            self.add("BRANCH", f.setters[0][0],
                     f"`{name}` can be set to \"{v}\" and not one line anywhere in the "
                     "corpus plays for it. The branch is reachable and empty.")

        if row and row.leaves:
            missing = {lf for lf in row.leaves} - {v.lower() for v in set_vals | read_eq}
            for lf in sorted(missing):
                self.add("BRANCH", (f.setters or f.readers)[0][0],
                         f"`{row.tag}` declares leaf `.{lf.capitalize()}` in the section 6 "
                         f"register and no script ever sets or reads `{name} == \"{lf}\"`.")

        for scene, vals in sorted(f.scene_reads.items()):
            positive = {v for v in vals if not isinstance(v, tuple)}
            excluded = {v for kind, v in (x for x in vals if isinstance(x, tuple))}
            covered = positive | (universe - excluded if excluded else set())
            gemist = universe - covered
            verklaard = self.silences.get(scene, {})

            # Een stilte die je declareert voor een tak die WEL bespeeld wordt,
            # is verouderd -- en dat is gevaarlijker dan geen stilte, want hij
            # dekt straks een echte omissie af.
            for val in sorted(set(verklaard) & covered):
                self.add("SILENCE", scene,
                         f"`silence: {val}` staat in de kop maar die tak WORDT bespeeld "
                         f"in deze scene. Een verouderde stilte dekt de volgende echte "
                         "omissie af; haal hem weg.")
            for val in sorted(set(verklaard) - universe):
                self.add("SILENCE", scene,
                         f"`silence: {val}` staat in de kop maar `{name}` kent die waarde "
                         f"niet. Bekend: {sorted(universe)}.")

            echt_gemist = gemist - set(verklaard)
            if len(positive) >= 2 and echt_gemist:
                where = next(r[0] for r in f.readers if r[0].startswith(scene))
                self.add("BRANCH", where,
                         f"this scene branches on `{name}` and handles "
                         f"{sorted(positive)} of {sorted(universe)} -- the player who took "
                         f"{sorted(echt_gemist)} gets the beat of a choice he did not "
                         "make, or none at all. Unfixable after generation.")
            for val in sorted(gemist & set(verklaard)):
                self.silence_used.add((scene, name, val))

    def check_groups(self) -> None:
        groups: dict[str, list] = defaultdict(list)
        for name, f in self.facts.items():
            for at, val, group, lid in f.setters:
                if group:
                    groups[str(group)].append((at, name, val, lid))
        for group, opts in sorted(groups.items()):
            if len(opts) < 2:
                self.add("CHOICE", opts[0][0],
                         f"choice group `{group}` has one option. A prompt with a single "
                         "answer is not a choice.")
            flags = {name for _, name, _, _ in opts}
            if len(flags) > 1:
                self.add("CHOICE", opts[0][0],
                         f"choice group `{group}` sets more than one flag: {sorted(flags)}. "
                         "Options in one prompt must write to one fact, or the reader can "
                         "never tell which option was taken.")


# ---------------------------------------------------------------------------
# 4. Voice -- delegated, never reimplemented
# ---------------------------------------------------------------------------

def delegate_voice_check() -> tuple[int, str]:
    sys.path.insert(0, str(TOOLS))
    import check_voice_resolves  # noqa: E402  (deliberately late)
    buf = io.StringIO()
    with redirect_stdout(buf):
        rc = check_voice_resolves.main()
    return rc, buf.getvalue()


# ---------------------------------------------------------------------------
# 5. main
# ---------------------------------------------------------------------------

def load_canon() -> dict:
    return {
        "bands": load_bands(),
        "tags": load_tag_set(),
        "names": load_glossary_names(),
        "locations": load_locations(),
        "register": load_flag_register(),
        "run_facts": load_run_facts(),
        "events": load_event_tags(),
    }


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--root", default=str(SCRIPTS),
                    help="script tree to validate (default: Eclipse/Content/Script)")
    ap.add_argument("--explain", action="store_true",
                    help="dump every canon table this tool read, then validate")
    ap.add_argument("--no-voice", action="store_true",
                    help="skip the delegated check_voice_resolves.py gate")
    args = ap.parse_args(argv)

    try:
        canon = load_canon()
    except SourceShapeError as e:
        print("SOURCE DOCUMENT CHANGED SHAPE -- refusing to run.")
        print(f"  {e}")
        print("\nThis is deliberate. An empty allow-list passes every check that")
        print("consults it, silently. Fix the loader or the document, never both by")
        print("guessing.")
        return 2

    root = Path(args.root).resolve()
    if not root.is_dir():
        print(f"No script tree at {root} - nothing to check.")
        return 0

    if args.explain:
        print("BANDS (SCRIPT_FORMAT section 4)")
        for k, (lo, hi) in sorted(canon["bands"].items()):
            print(f"  {k:<18} register {lo if lo else '-':>4}   ceiling {hi:>4}")
        print(f"\nAPPROVED TAGS (19.4): {', '.join(sorted(canon['tags']))}")
        print(f"\nRUN FACTS (SCRIPT_FORMAT section 4 / L1-R4): "
              f"{', '.join(sorted(canon['run_facts']))}")
        print(f"\nLOCATIONS (ACT1_OVERVIEW section 7): {len(canon['locations'])} strings")
        print(f"\nGLOSSARY TOKENS (00_INDEX): {len(canon['names'])} names")
        print("\nFLAG REGISTER BINDING (ACT1_OVERVIEW section 6)")
        print("  every binding below is derived, so every one of them is auditable")
        for key, row in sorted(canon["register"].items()):
            leaves = ("{" + ",".join(row.leaves) + "}") if row.leaves else ""
            print(f"  {key:<26} <- {row.tag}{leaves}")
        print()

    v = Validator(root, canon)
    for path in sorted(root.rglob("*.yaml")):
        v.visit(path)
    v.cross_check()

    # A CHECK THAT DID NOT RUN IS NOT A CHECK THAT PASSED.
    #
    # `skipped` exists so the summary can never list VOICE among the clean
    # checks when it was never asked. That distinction is the whole lesson of
    # 31-07: a green line for a question nobody put is worth less than no line,
    # because it is read as an answer.
    skipped: set[str] = set()
    voice_rc, voice_out = 0, ""
    if args.no_voice or root != SCRIPTS:
        skipped.add("VOICE")
    else:
        try:
            voice_rc, voice_out = delegate_voice_check()
        except Exception as e:                                   # noqa: BLE001
            voice_rc, voice_out = 1, f"delegation to check_voice_resolves.py failed: {e}\n"
        if voice_rc:
            v.add("VOICE", "check_voice_resolves.py",
                  "at least one `voice:` key resolves to nothing -- full report below")

    print(f"validate_script.py -- {v.scenes} scene file(s) "
          f"({v.written} written, {v.stubs} still stubs), {v.barks} bark set(s), "
          f"{v.lines} lines, {len(v.facts)} distinct story/run facts.")

    def _print_silences(v, blank_before=False):
        """Een doorgelaten stilte moet ZICHTBAAR blijven, ook op een schone bar.

        Dit stond eerst alleen op het bevindingenpad, en dan verdwijnt een
        stille tak precies wanneer alles verder groen is -- op het moment dat
        niemand meer kijkt. Anders koopt `silence:` groen met onzichtbaarheid,
        en dat is exact het gedrag dat deze tool bestaat om te vangen.

        Gevonden door de test die eist dat hij zichtbaar blijft, niet door de
        code te herlezen.
        """
        if not v.silence_used:
            return
        if blank_before:
            print()
        print(f"{len(v.silence_used)} BEWUST STILLE TAK(KEN) -- doorgelaten, niet verdwenen:")
        for scene, name, val in sorted(v.silence_used):
            print(f"  {scene}: `{name} == \"{val}\"` speelt niets af, met opgeschreven reden")
        if not blank_before:
            print()

    if v.register_trusted:
        print(f"{len(v.register_trusted)} FEIT(EN) OP HET REGISTER VERTROUWD -- doorgelaten, "
              "niet verdwenen:")
        for scene, name, n_readers, set_by in sorted(v.register_trusted):
            print(f"  {scene}: `{name}` wordt door {n_readers} regel(s) gelezen en door GEEN "
                  f"`set:` gezet. Het register noemt {set_by!r} als zetter -- deze check "
                  "gelooft dat, en controleert het niet.")
        print()

    by_code: dict[str, list[Finding]] = defaultdict(list)
    for f in v.findings:
        by_code[f.code].append(f)

    if not v.findings:
        print("\nOK: every check that ran is clean.")
        _print_silences(v, blank_before=True)
        for code in CHECKS:
            mark = "SKIPPED" if code in skipped else "clean  "
            print(f"  {mark}  {code:<10} {CHECKS[code]}")
        return 0

    print()
    for code in CHECKS:
        hits = by_code.get(code)
        if not hits:
            continue
        print(f"{code} -- {CHECKS[code]}  ({len(hits)})")
        for f in hits:
            print(f)
        print()

    if voice_rc:
        print("--- check_voice_resolves.py ---")
        print(voice_out.rstrip())
        print()

    _print_silences(v)

    print(f"{len(v.findings)} finding(s) over {len(by_code)} check(s):")
    for code in CHECKS:
        if code in by_code:
            print(f"  {len(by_code[code]):>4}  {code}")
    clean = [c for c in CHECKS if c not in by_code and c not in skipped]
    if clean:
        print(f"  clean: {', '.join(clean)}")
    if skipped:
        print(f"  NOT RUN (which is not the same thing as clean): "
              f"{', '.join(sorted(skipped))}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
