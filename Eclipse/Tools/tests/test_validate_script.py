#!/usr/bin/env python3
"""The proof that every check in validate_script.py CAN GO RED.

Why this file exists, and it is not "for coverage". On the evening of
2026-07-31 `check_voice_resolves.py` reported `eclipse_fighter_a` as "CAST AND
READY" while that speaker had no voice bound at all. The check was green, the
green was worthless, and nobody could tell, because the check had never been
seen to fail. A validator that has only ever been green on the current corpus
measures nothing at all -- it is indistinguishable from `exit 0`.

So every check in validate_script.py owns a fixture that trips it, and each of
those fixtures is one of the real errors of that evening wherever there was
one:

    choice_orphan/  a `choice.set` on a flag nobody reads      (error 1)
    branch/         setter grew three leaves, reader handles two,
                    in a DIFFERENT MISSION by a DIFFERENT WRITER (error 2, L1-R12)
    condition/      a condition on a WITHDRAWN flag             (error 3)

And the other half of the proof, which is the half people skip: `clean/` must
produce ZERO findings. A validator that fires on everything is exactly as
useless as one that fires on nothing, and it fails in the more expensive
direction -- people learn to wave the bar away, and then the true red goes past
with it.

Run:  python Eclipse/Tools/tests/test_validate_script.py
"""
import io
import sys
import unittest
from contextlib import redirect_stdout
from pathlib import Path

HERE = Path(__file__).resolve().parent
FIXTURES = HERE / "script_fixtures"
sys.path.insert(0, str(HERE.parent))

import validate_script as vs  # noqa: E402


def run(fixture: str) -> tuple[int, str]:
    """Validate one fixture directory. Returns (exit code, output)."""
    buf = io.StringIO()
    with redirect_stdout(buf):
        rc = vs.main(["--root", str(FIXTURES / fixture), "--no-voice"])
    return rc, buf.getvalue()


def codes(out: str) -> set[str]:
    """The check codes that produced at least one finding."""
    return {c for c in vs.CHECKS if f"\n{c} -- " in "\n" + out}


class EveryCheckCanGoRed(unittest.TestCase):
    """One test per check. If a check has no test here, it does not exist."""

    def assertRed(self, fixture: str, code: str) -> None:
        rc, out = run(fixture)
        self.assertEqual(rc, 1, f"{fixture} should have produced findings:\n{out}")
        self.assertIn(code, codes(out),
                      f"{fixture} did not trip {code}. It tripped "
                      f"{sorted(codes(out)) or 'nothing'}.\n{out}")

    def assertOnly(self, fixture: str, code: str) -> None:
        """Trips its own check AND NOTHING ELSE.

        A fixture that fires two checks proves neither: you cannot tell which
        of the two the input actually violates.
        """
        self.assertRed(fixture, code)
        _, out = run(fixture)
        self.assertEqual(codes(out), {code},
                         f"{fixture} should isolate {code}, tripped {sorted(codes(out))}")

    def test_parse_a_line_it_cannot_read_is_never_skipped(self):
        self.assertOnly("parse", "PARSE")

    def test_schema_missing_and_unknown_fields(self):
        self.assertOnly("schema", "SCHEMA")

    def test_id_duplicate_and_out_of_order(self):
        self.assertOnly("id", "ID")

    def test_an_inserted_id_off_the_tens_grid_is_legal(self):
        """SCRIPT_FORMAT section 2 exists to allow `015`. An early draft of the
        ID check failed it, which would have stood red forever on three
        correct shipped lines. A validator gets its rules from canon or it
        does not get to have them."""
        rc, out = run("clean")
        self.assertEqual(rc, 0)
        header, items = vs.parse_scene_file(FIXTURES / "clean" / "M1.1.S03.yaml")
        v = vs.Validator(FIXTURES / "clean", vs.load_canon())
        self.assertEqual(v.check_id("x.yaml", "M1.1.S03",
                                    vs.Node({"id": "M1.1.S03.015"}, {}), 10), 15)
        self.assertEqual(v.findings, [])

    def test_speaker_non_canon_and_near_miss(self):
        self.assertOnly("speaker", "SPEAKER")

    def test_ceiling_line_over_its_band(self):
        self.assertOnly("ceiling", "CEILING")

    def test_register_scene_never_reaches_its_band_floor(self):
        self.assertOnly("register", "REGISTER")

    def test_spread_every_turn_the_same_length(self):
        self.assertOnly("spread", "SPREAD")

    def test_tag_outside_the_approved_set_and_stacked(self):
        self.assertOnly("tag", "TAG")

    def test_choice_a_set_that_nobody_reads(self):
        """Real error 1 of 2026-07-31, and it was the architect's own."""
        self.assertOnly("choice_orphan", "CHOICE")

    def test_choice_group_with_one_option(self):
        self.assertOnly("choice_lone", "CHOICE")

    def test_choice_group_writing_two_different_flags(self):
        self.assertOnly("choice_mixed", "CHOICE")

    def test_condition_on_a_withdrawn_flag(self):
        """Real error 3 of 2026-07-31: a line that never plays, still paid for."""
        self.assertOnly("condition", "CONDITION")

    def test_branch_setter_has_three_leaves_reader_handles_two(self):
        """Real error 2, RULING L1-R12 -- the costliest one missed that night.

        The setter and the reader are in different missions on purpose: that is
        exactly why it was invisible from inside either one of them.
        """
        rc, out = run("branch")
        self.assertEqual(rc, 1)
        self.assertIn("BRANCH", codes(out))
        self.assertIn("warned", out, "the finding must name the branch that is missing")
        self.assertIn("M1.6.S06", out, "the finding must point at the READER, "
                                       "which is where a flag-shape change breaks")

    def test_flagreg_flag_in_production_with_no_register_row(self):
        self.assertOnly("flagreg", "FLAGREG")

    def test_nameslot_name_and_a_pronoun_in_one_line(self):
        self.assertOnly("nameslot", "NAMESLOT")

    def test_location_outside_the_act_registry(self):
        self.assertOnly("location", "LOCATION")

    def test_guard_generated_on_a_no_go(self):
        """The money-saver: the mechanical reason a bad line cannot reach the API."""
        self.assertOnly("guard", "GUARD")

    def test_trigger_bark_pointing_at_an_invented_event(self):
        self.assertOnly("trigger", "TRIGGER")

    def test_every_check_has_a_fixture(self):
        """No check may be shipped without a proof that it can fail.

        VOICE is delegated to check_voice_resolves.py, which owns its own
        gate in verify.ps1; reimplementing it here would be the second copy of
        a truth this whole tool exists to argue against.
        """
        proven = {"PARSE", "SCHEMA", "ID", "SPEAKER", "CEILING", "REGISTER", "SPREAD",
                  "TAG", "CHOICE", "CONDITION", "BRANCH", "FLAGREG", "NAMESLOT",
                  "LOCATION", "GUARD", "TRIGGER"}
        self.assertEqual(set(vs.CHECKS) - proven, {"VOICE"})


class TheControl(unittest.TestCase):
    """The other half of the proof: correct input must stay silent."""

    def test_clean_fixture_produces_nothing(self):
        rc, out = run("clean")
        self.assertEqual(rc, 0, f"the control fixture must be clean:\n{out}")
        self.assertEqual(codes(out), set())

    def test_a_one_word_line_in_a_ten_to_twentyfive_band_is_legal(self):
        """SCRIPT_FORMAT, verbatim: "a validator that fails this line is a
        broken validator, not a broken line." Bands are ceilings (L1-R1), and
        the floor was lifted because every justification in 18.3 argues an
        upper limit and not one explains why a line may not be shorter."""
        rc, out = run("clean")
        self.assertEqual(rc, 0)
        self.assertNotIn("CEILING --", out, "the one-word line was reported")
        self.assertNotIn("Thirteen", out)

    def test_multi_line_folded_scalar_parses(self):
        """P0.S01 wraps a long `note:` over two physical lines. That is real
        YAML, not a typo, and calling it a broken field would be a false red --
        which costs more than no bar at all, because a bar people wave away
        stops working for the true reds too."""
        header, items = vs.parse_scene_file(FIXTURES / "clean" / "M1.1.S03.yaml")
        note = items[0].get("note")
        self.assertIn("canonical example", note)
        self.assertNotIn("\n", note)


class CanonIsReadNotRestated(unittest.TestCase):
    """The tables are parsed out of the documents that own them.

    A second copy of a truth is a divergence bug waiting to happen (L1-R3), and
    this tool exists to catch exactly that class of bug. It would be absurd for
    it to introduce one.
    """

    def test_bands_come_from_script_format(self):
        bands = vs.load_bands()
        self.assertEqual(bands["in-mission-radio"], (10, 25))
        self.assertEqual(bands["bark"], (3, 8))
        self.assertEqual(bands["oration"][1], 120)
        self.assertIsNone(bands["oration"][0], "oration is a ceiling with no floor")

    def test_run_facts_come_from_script_format(self):
        self.assertIn("run.zero_casualty", vs.load_run_facts())
        self.assertIn("run.alarm_raised", vs.load_run_facts())

    def test_withdrawn_flags_are_not_registered(self):
        """The bug this tool had in its own first draft.

        The register loader swept every backticked `Story.*` in ACT1_OVERVIEW
        section 6 -- including the sentence that says
        `Story.Choice.M11_ConscriptSpared` "is hiermee ingetrokken". A dead
        flag registered as live means a condition on it resolves cleanly, which
        is the "CAST AND READY" failure wearing a new hat, inside the tool
        built to prevent it. Prose is where flags get killed; only the table
        and the code line say a flag lives.
        """
        reg = vs.load_flag_register()
        self.assertNotIn("m11conscriptspared", reg)
        self.assertIn("m11conscript", reg)
        self.assertEqual(reg["m11conscript"].leaves, ("finished", "left", "bound"))

    def test_the_binder_pairs_both_spellings_canon_uses(self):
        """SCRIPT_FORMAT itself uses two conventions, so the binder normalises
        both sides down to what they genuinely share."""
        self.assertEqual(vs.fact_key("story.m11_conscript_choice"),
                         vs.fact_key("Story.Choice.M11_Conscript"))
        self.assertEqual(vs.fact_key("story.char_maradead"),
                         vs.fact_key("Story.Char.MaraDead"))
        self.assertEqual(vs.fact_key("story.brick_recruited"),
                         vs.fact_key("Story.Beat.BrickRecruited"))
        self.assertNotEqual(vs.fact_key("story.m15_pact"),
                            vs.fact_key("Story.Choice.M15_IronChorusPact"))

    def test_an_empty_allow_list_is_refused_not_accepted(self):
        """A loader that returns an empty set silently passes every check that
        consults it. That failure mode is invisible, so it raises instead."""
        with self.assertRaises(vs.SourceShapeError):
            vs._section("# a document with no such heading\n", r"^#+ 6\. Vlaggenregister.*$")


if __name__ == "__main__":
    unittest.main(verbosity=2)
