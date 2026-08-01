"""Waar elke regio op het bord staat (REFERENTIE_BASE_MAP.md 1.4, rij 1).

Run headless:
  UnrealEditor-Cmd.exe Eclipse.uproject -run=pythonscript
    -script="Tools/author_region_layout.py" -unattended -nopause -nosplash

WAAROM DEZE COORDINATEN GEAUTHORD ZIJN EN NIET BEREKEND.

Een automatische layout is de kortste weg: hij heeft geen content nodig en hij
levert altijd iets op. Op zes knopen levert hij een willekeurige spreiding die
niets zegt over het district, en de staande opdracht van de owner
(21_quality_mandate.md) is "NOOIT de kortste weg naar het doel, altijd de beste
weg". Geauthorde coordinaten laten de VORM de fictie dragen, en dat is precies
wat GDD 3.1 van een kaart vraagt.

WAT DE VORM HIER ZEGT, per knoop:

  * Underworks ONDERAAN. Het is de onderstad: een oude onderhoudslaag onder de
    rest van het district. Elke andere plaatsing maakt van "under" een naam.
  * TransitCheckpoint BOVENAAN EN CENTRAAL. Het is de Gate Spire van dit bord
    (zie author_region_lanes.py) en hij kijkt neer op de twee wegen die hij
    poort. Een poort die naast zijn wegen ligt, leest niet als een poort.
  * CommsRelay HELEMAAL RECHTS, aan het verre eind van de gepoorte lane. De
    relaisweg is de lange haul (3 dagen) en de enige route erheen loopt langs
    het checkpoint; die afstand hoort te ZIEN te zijn voor je hem kiest.
  * WorkerHousing links tegen de Underworks aan -- de smokkelkruipgang naar het
    depot loopt daar onderdoor -- en FoundryRow midden rechts, tussen het
    checkpoint en de relais in.

Idempotent: het herschrijft de posities uit de tabel bij elke run.

TERUG TE DRAAIEN MET EEN VELD. Zet de posities op (-1,-1) en het bord valt terug
op de lijst, luid gemeld (EclipseStrategyMap::ComposeMapView), nooit stil op een
spreiding die zich als geografie voordoet.
"""
import unreal

DATA_PATH = "/Game/Data"
GRAPH_NAME = "DA_KessaraDistrictGraph"

# region_id -> (x, y) in genormaliseerde bordruimte: X naar rechts, Y OMLAAG,
# allebei 0..1. Buiten dat bereik telt als "niet geplaatst" -- een knoop die
# buiten het vlak valt is onzichtbaar, en dat mag geen stille uitkomst zijn.
#
# De getallen zijn EEN KEER BIJGESTELD op het frame van 01-08 en dat hoort zo:
# de eerste ronde zette WorkerHousing en SupplyDepot zo dicht op elkaar dat hun
# namen door de prijs van de smokkellane heen liepen. Een geauthorde indeling is
# geen som -- hij wordt beoordeeld op een opname, net als de rest van het beeld.
LAYOUT = {
    "TransitCheckpoint": (0.30, 0.10),
    "FoundryRow": (0.62, 0.36),
    "SupplyDepot": (0.36, 0.46),
    "WorkerHousing": (0.05, 0.64),
    "CommsRelay": (0.93, 0.58),
    "Underworks": (0.17, 0.95),
}


def apply_layout(graph):
    """Schrijf elke positie uit LAYOUT op de regio. Geeft het aantal terug."""
    regions = graph.get_editor_property("regions")
    known = {str(regions[i].get_editor_property("region_id")) for i in range(len(regions))}
    unknown = sorted(set(LAYOUT) - known)
    if unknown:
        # Luid, niet stil: een positie voor een regio die niet bestaat betekent
        # dat iemand een id hernoemd heeft, en dan staat de echte regio nergens.
        raise RuntimeError("LAYOUT noemt regio's die niet op het bord staan: %s" % ", ".join(unknown))
    missing = sorted(known - set(LAYOUT))
    if missing:
        # Ook luid: een halve graaf is erger dan geen graaf, en de tekenlaag
        # weigert hem toch. Dan liever hier stuk dan daar stil.
        raise RuntimeError("deze regio's krijgen geen plek op het bord: %s" % ", ".join(missing))

    # BY INDEX terugschrijven. Itereren over een unreal.Array van USTRUCTs geeft
    # een KOPIE per element, dus `for r in regions: r.set_editor_property(...)`
    # schrijft naar een tijdelijk object dat wordt weggegooid -- de asset saveT,
    # het log zegt succes, en er staat niets in. Gemeten op 31-07 bij de lanes.
    written = 0
    for index in range(len(regions)):
        region = regions[index]
        region_id = str(region.get_editor_property("region_id"))
        x, y = LAYOUT[region_id]
        region.set_editor_property("board_position", unreal.Vector2D(x, y))
        regions[index] = region
        written += 1
    graph.set_editor_property("regions", regions)

    # Teruglezen uit de ASSET en niet uit de lijst hierboven: de write-back is
    # nou juist de stap die stil kan overslaan.
    verify = graph.get_editor_property("regions")
    for index in range(len(verify)):
        region_id = str(verify[index].get_editor_property("region_id"))
        landed = verify[index].get_editor_property("board_position")
        want_x, want_y = LAYOUT[region_id]
        if abs(landed.x - want_x) > 1e-6 or abs(landed.y - want_y) > 1e-6:
            raise RuntimeError(
                "positie kwam niet aan op %s: wilde (%.2f, %.2f), asset heeft (%.2f, %.2f)"
                % (region_id, want_x, want_y, landed.x, landed.y)
            )
    return written


def main():
    path = "%s/%s" % (DATA_PATH, GRAPH_NAME)
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        raise RuntimeError("%s bestaat niet - draai eerst Tools/create_phase1_content.py." % path)

    graph = unreal.EditorAssetLibrary.load_asset(path)
    written = apply_layout(graph)

    # save_loaded_asset en NIET save_asset(path): die laatste staat standaard op
    # only_if_is_dirty=True, en set_editor_property op een struct-array maakt het
    # package niet vuil. Hij geeft dan True terug zonder iets te schrijven.
    graph.modify()
    if not unreal.EditorAssetLibrary.save_loaded_asset(graph, only_if_is_dirty=False):
        raise RuntimeError("save_loaded_asset('%s') faalde - er is niets weggeschreven." % path)
    return "%d regio's op het bord gezet op %s" % (written, GRAPH_NAME)


if __name__ != "author_region_layout":
    # unreal.log_* en niet print(): de pythonscript-commandlet routeert print()
    # niet naar het engine-log, en dan is een run die niets deed niet te
    # onderscheiden van een run die werkte.
    try:
        unreal.log_warning("author_region_layout: start")
        unreal.log_warning("author_region_layout: %s" % main())
    except Exception as Error:  # noqa: BLE001 - de commandlet slikt deze anders
        unreal.log_error("author_region_layout FAALDE: %r" % (Error,))
        raise
