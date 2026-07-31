"""Lane costs and lane status for the Kessara Foundry District (GDD 3.1 rules 2 and 4).

Run headless:
  UnrealEditor-Cmd.exe Eclipse.uproject -run=pythonscript
    -script="Tools/author_region_lanes.py" -unattended -nopause -nosplash

Why this is its own script and not inlined in create_phase1_content.py: that one
rewrites the whole Phase 1 content set (offers, production, roster, campaign
setup). This one touches exactly one asset, so it is safe to run on a shared
checkout while other work is in flight. create_phase1_content.py imports the
table below, so there is still only ONE place where a lane's price is written
down (GDD 14.2: numbers are data, and data has one home).

Idempotent: it rewrites the lane arrays from the table every run.

WHAT THE TABLE ENCODES, and why these three statuses on this board:

  * TransitCheckpoint is the district's Gate Spire. DT_MissionOffers already
    says so out loud - "every patrol in the sector rotates through this
    checkpoint, take it and the map opens" - and until now that sentence was
    flavour text with nothing behind it. It now gates FoundryRow <-> CommsRelay:
    the relay road runs under the checkpoint's guns, so while the Dominion holds
    the checkpoint no column uses that road, and the moment the player takes it
    the road opens. That is the sentence, made true.

  * WorkerHousing <-> SupplyDepot is SmugglerOnly: the maintenance crawl under
    the depot. Kaya's route. No node gates it, and taking the whole district
    does not widen it - a crawlway stays a crawlway.

  * Everything else is open, at real distances. Rule 4 says distance = time =
    risk, so the checkpoint road is short and watched (risk 8) while the long
    haul out to the relay is three days through open ground (risk 12).
"""
import unreal

DATA_PATH = "/Game/Data"
GRAPH_NAME = "DA_KessaraDistrictGraph"

OPEN = unreal.EclipseLaneStatus.OPEN
SPIRE_GATED = unreal.EclipseLaneStatus.SPIRE_GATED
SMUGGLER_ONLY = unreal.EclipseLaneStatus.SMUGGLER_ONLY

# (A, B, travel_days, risk, status, gate, smuggler_delay, smuggler_risk)
# Authored ONCE per undirected lane; apply_lanes mirrors both halves, so the
# symmetry the validator enforces cannot be broken by a typo here.
LANES = [
    ("Underworks", "TransitCheckpoint", 1, 8, OPEN, "", 1, 12),
    ("Underworks", "WorkerHousing", 1, 3, OPEN, "", 1, 8),
    ("TransitCheckpoint", "FoundryRow", 2, 10, OPEN, "", 1, 14),
    ("TransitCheckpoint", "SupplyDepot", 2, 9, OPEN, "", 1, 14),
    ("WorkerHousing", "SupplyDepot", 1, 6, SMUGGLER_ONLY, "", 1, 18),
    ("SupplyDepot", "CommsRelay", 3, 12, OPEN, "", 2, 16),
    ("FoundryRow", "CommsRelay", 2, 14, SPIRE_GATED, "TransitCheckpoint", 2, 22),
]


def make_lane(neighbor, days, risk, status, gate, smuggler_delay, smuggler_risk):
    lane = unreal.EclipseLaneDefinition()
    lane.set_editor_property("neighbor_region_id", unreal.Name(neighbor))
    lane.set_editor_property("travel_days", days)
    lane.set_editor_property("risk", risk)
    lane.set_editor_property("status", status)
    lane.set_editor_property("gate_region_id", unreal.Name(gate) if gate else unreal.Name(""))
    lane.set_editor_property("smuggler_delay_days", smuggler_delay)
    lane.set_editor_property("smuggler_risk_penalty", smuggler_risk)
    return lane


def apply_lanes(graph):
    """Rewrite every region's lane array from LANES. Returns lanes written."""
    by_region = {}
    for a, b, days, risk, status, gate, delay, smuggler_risk in LANES:
        by_region.setdefault(a, []).append(make_lane(b, days, risk, status, gate, delay, smuggler_risk))
        by_region.setdefault(b, []).append(make_lane(a, days, risk, status, gate, delay, smuggler_risk))

    regions = graph.get_editor_property("regions")
    known = {str(r.get_editor_property("region_id")) for r in regions}
    unknown = sorted(set(by_region) - known)
    if unknown:
        # Loud, not silent: a lane to a region that is not on the board would
        # leave the other end orphaned and the validator would report it far
        # from here (GDD 14.3.5 - degrade loudly, never quietly).
        raise RuntimeError("LANES references regions not in the graph: %s" % ", ".join(unknown))

    # Assign back BY INDEX. Iterating an unreal.Array of USTRUCTs hands out a
    # COPY per element, so `for region in regions: region.set_editor_property(...)`
    # writes to a temporary that is thrown away - the asset saves, the file grows,
    # every log line says success, and the lanes on disk are still the defaults.
    # Measured on 31-07: the shipped-board test read 0 gated / 0 smuggler-only /
    # 0 lanes over one day out of an asset the script had "authored" four times.
    written = 0
    for index in range(len(regions)):
        region = regions[index]
        region_id = str(region.get_editor_property("region_id"))
        lanes = by_region.get(region_id, [])
        region.set_editor_property("lanes", lanes)
        regions[index] = region
        written += len(lanes)
    graph.set_editor_property("regions", regions)

    # Read back from the ASSET, not from the list we just built: the whole point
    # is that the write-back is the step that silently does not happen.
    verify = graph.get_editor_property("regions")
    landed = sum(len(verify[i].get_editor_property("lanes")) for i in range(len(verify)))
    if landed != written:
        raise RuntimeError("lane write-back did not land: expected %d, asset holds %d" % (written, landed))
    return written


def main():
    path = "%s/%s" % (DATA_PATH, GRAPH_NAME)
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        raise RuntimeError("%s does not exist - run Tools/create_phase1_content.py first." % path)

    graph = unreal.EditorAssetLibrary.load_asset(path)
    written = apply_lanes(graph)

    # save_loaded_asset, and NOT save_asset(path): save_asset defaults to
    # only_if_is_dirty=True, and set_editor_property on a struct array does not
    # dirty the package. It then returns True having written nothing at all -
    # measured on 31-07, three "authored 14 lane halves" runs in a row while the
    # file on disk kept its timestamp from nine days earlier. A success value is
    # not evidence of an effect; the file's mtime is.
    graph.modify()  # the struct-array writes above do not dirty the package on their own
    if not unreal.EditorAssetLibrary.save_loaded_asset(graph, only_if_is_dirty=False):
        raise RuntimeError("save_loaded_asset('%s') failed - nothing was written." % path)
    return "authored %d lane halves (%d undirected lanes) on %s" % (written, len(LANES), GRAPH_NAME)


# Runs unless imported as a module - which is exactly what create_phase1_content.py
# does, and that caller applies the lanes itself at the right moment (after the
# regions exist, before its own save loop).
#
# `__name__` under `-run=pythonscript -script=...` is plain `'__main__'`; that was
# measured, not assumed, after this guard was briefly suspected of being the reason
# the script "did nothing". It was not - see save_loaded_asset in main().
if __name__ != "author_region_lanes":
    # unreal.log_* and not print(): the pythonscript commandlet does not route
    # print() into the engine log, so a run that did nothing is indistinguishable
    # from a run that worked. Measured on 31-07 - three "executed successfully"
    # runs in a row that had authored nothing at all.
    try:
        unreal.log_warning("author_region_lanes: start")
        unreal.log_warning("author_region_lanes: %s" % main())
    except Exception as Error:  # noqa: BLE001 - the commandlet swallows these otherwise
        unreal.log_error("author_region_lanes FAILED: %r" % (Error,))
        raise
