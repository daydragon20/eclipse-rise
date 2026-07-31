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

    written = 0
    for region in regions:
        region_id = str(region.get_editor_property("region_id"))
        lanes = by_region.get(region_id, [])
        region.set_editor_property("lanes", lanes)
        written += len(lanes)
    graph.set_editor_property("regions", regions)
    return written


def main():
    path = "%s/%s" % (DATA_PATH, GRAPH_NAME)
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        raise RuntimeError("%s does not exist - run Tools/create_phase1_content.py first." % path)

    graph = unreal.EditorAssetLibrary.load_asset(path)
    written = apply_lanes(graph)
    unreal.EditorAssetLibrary.save_asset(path)
    print("Authored %d lane halves (%d undirected lanes) on %s." % (written, len(LANES), GRAPH_NAME))


if __name__ == "__main__":
    main()
