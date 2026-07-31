"""Vind assets die niemand aanroept — de asset-tegenhanger van find_dead_fields.py.

Waarom dit bestaat: op 26-07-2026 bleken drie geluiden ongebruikt in de repo te
liggen (het wapenschot, de kogelinslag, twee voetstappen). Ze waren gegenereerd,
geïmporteerd, gecommit — en er was geen enkele regel code die ze afspeelde. Ik vond
ze bij toeval, tijdens het schrijven van de gevechts-audit. Toeval is geen methode.

**Wat "dood" hier betekent.** Een asset waarvan de NAAM in geen enkel `.cpp`, `.h`
of `.py` in het project voorkomt. Dat is precies de goede test voor de categorieën
die de code bij PAD moet laden — geluidscues, data-assets, kaarten — want die
kunnen niet per ongeluk gevonden worden.

**Wat het NIET betekent, en dat is belangrijk.** Assets waarnaar vanuit DATA wordt
verwezen (animaties in DT_BodyDefs, materialen op een mesh, texturen op een
materiaal) zijn hier niet aan te tonen: hun verwijzing staat in een `.uasset`, niet
in tekst. Die categorieën staan daarom in SKIP_DIRS. Een animatie die niemand
gebruikt vind je met `find_dead_fields.py` plus de logregels van
setup_character_data.py, niet hiermee.

Draaien:

    python Eclipse/Tools/find_dead_assets.py

Uitvoer is de bron; schrijf het getal niet over in een document (zie de docstring
van find_dead_fields.py voor waarom dat zes keer is misgegaan).
"""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTENT = ROOT / "Content"
SOURCE = ROOT / "Source"
TOOLS = ROOT / "Tools"

# Alleen mappen waar de code zijn assets bij PAD laadt. Character-, materiaal- en
# texturemappen horen hier niet: die worden vanuit data of vanuit andere assets
# gerefereerd, en dan zegt "de naam staat in geen enkele .cpp" niets.
# ALLEEN Audio, en dat is 26-07 avond opnieuw vastgesteld na het te hebben
# geprobeerd. De twee packs die die dag binnenkwamen (FreeWeaponSounds,
# Footsteps_Volume_02) worden bij pad geladen, maar met een SAMENGESTELD pad:
#
#     FString::Printf(TEXT(".../Cue/%s/Gunshots/%s_gunshot_%02d_Cue..."), ...)
#
# Dan staat er geen enkel letterlijk pad in de bron, ook niet van de map, en een
# tekstsweep verklaarde dertien cues dood die elke seconde afspelen. Een sweep die
# over-meldt is net zo onbruikbaar als een die te weinig meldt.
#
# Wat die packs WEL dekt: de runtime-meting. Eclipse.Mission.Playthrough.
# WeaponSoundSetsLoadAndTailIsBraked en FootstepsKnowTheirSurface tellen hoeveel
# varianten er echt geladen zijn — dat is sterker dan tekst zoeken, want het meet
# het laden zelf.
LOOK_IN = ("Audio",)

# Alleen cues: die worden door code bij pad geladen. Een ruw golfbestand zit in
# zijn cue en hoort daar niet los te staan.
# De oude eigen audio heet Cue_*; de twee packs noemen hun cues anders (een
# _Cue-SUFFIX). Beide vormen tellen, want beide worden bij pad geladen.
NAME_PREFIX = "Cue_"
NAME_SUFFIX = "_Cue"

# Binnen die mappen nog steeds overslaan: gegenereerde audio is per definitie
# hash-genoemd en wordt via het manifest gevonden, niet bij naam.
SKIP_DIRS = ("Generated",)

# BEWUST ONGEBRUIKT — door de owner beoordeeld, niet vergeten.
#
# Deze sweep vindt terecht dat niemand ze aanroept, maar een melding die elke keer
# terugkomt terwijl het antwoord al gegeven is, leert je de sweep te negeren. Dat is
# precies hoe de drie dode geluiden van 26-07 zo lang onopgemerkt bleven.
#
# De reden staat erbij, want een lijst met alleen namen wordt over een maand een
# lijst met vraagtekens.
BEWUST_ONGEBRUIKT = {
    "Audio/SFX/Cue_SFX_Foot_Metal_01":
        "Owner 27-07: LATEN LIGGEN. Vervangen door Footsteps_Volume_02 (zes varianten "
        "in plaats van een), dus niets roept hem nog aan. Het is betaalde audio, hij "
        "kost niets waar hij ligt, en weggooien is onomkeerbaar.",
}


def main() -> int:
    if not CONTENT.is_dir():
        print(f"Geen Content-map op {CONTENT}.")
        return 1

    haystack = []
    for folder in (SOURCE, TOOLS):
        for pattern in ("*.cpp", "*.h", "*.py", "*.ini", "*.json"):
            for path in folder.rglob(pattern):
                haystack.append(path.read_text(encoding="utf-8", errors="replace"))
    # De config leest ook paden (bv. GameplayCueNotifyPaths).
    for path in (ROOT / "Config").rglob("*.ini"):
        haystack.append(path.read_text(encoding="utf-8", errors="replace"))
    blob = "\n".join(haystack)

    dead = []
    parked = []
    checked = 0
    for asset in sorted(CONTENT.rglob("*.uasset")):
        relative = asset.relative_to(CONTENT)
        if not relative.parts or relative.parts[0] not in LOOK_IN:
            continue
        if any(skip in relative.parts for skip in SKIP_DIRS):
            continue
        if not (asset.stem.startswith(NAME_PREFIX) or asset.stem.endswith(NAME_SUFFIX)):
            continue
        checked += 1
        # Het PAD zoals de engine het kent (/Game/<map>/<naam>), niet de losse
        # naam. Zo telt alleen een echte laadverwijzing mee en niet een comment
        # die de naam noemt — zie de docstring voor waarom dat onderscheid deze
        # sweep maakt of breekt.
        game_path = "/Game/" + relative.with_suffix("").as_posix()
        if game_path not in blob:
            key = relative.with_suffix("").as_posix()
            if key in BEWUST_ONGEBRUIKT:
                parked.append((key, BEWUST_ONGEBRUIKT[key]))
            else:
                dead.append(relative.as_posix())

    print(f"{checked} audiocues gecontroleerd.")
    if parked:
        # Wel TONEN, niet meetellen: onzichtbaar maken is hoe een beslissing na een
        # half jaar weer een open vraag wordt.
        print("")
        print(f"{len(parked)} bewust ongebruikt (door de owner beoordeeld):")
        for key, reason in parked:
            print(f"  {key}")
            print(f"      {reason}")
    if not dead:
        print("Geen dode assets — elk /Game-pad hier wordt ergens geladen.")
        return 0

    print(f"\n{len(dead)} waarvan het /Game-pad nergens in de code staat:\n")
    for item in dead:
        print(f"  {item}")
    print("\nElk is óf aan te sluiten óf weg te halen. Betaalde audio weggooien is")
    print("een owner-beslissing; aansluiten meestal niet (zie de memory-regel:")
    print("dood is van mij, onbeslist is van de owner).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
