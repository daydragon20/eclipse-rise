#!/usr/bin/env python3
"""CI check (GDD 16.15 rule 7, 19.2 "log every batch"): a generation run must
never be able to succeed without measured credit spend.

Why this test exists. `generate_audio_assets.py` measured spend by calling
get_usage(), which returned a bare None on any failure. The project's API key
is scoped and gets HTTP 401 (missing `user_read`) on /v1/user/subscription, so
get_usage() returned None, main() skipped the `if usage_before and usage_after`
block, and the run generated audio, spent real credits, and recorded nothing.
The meter was broken and said so nowhere -- while phase0/VOICE_LEDGER.md is by
its own header "the truth" against the ladder's "plan".

These tests pin the fixed behaviour. No network: the API layer is stubbed.

Run:  python Eclipse/Tools/test_credit_meter.py
"""
import io
import os
import shutil
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import generate_audio_assets as gen  # noqa: E402

UNAUTHORIZED = (401, b"", '{"detail":{"status":"missing_permissions",'
                          '"message":"missing the permission user_read"}}')
OK_BODY = b'{"character_count": 1000, "character_limit": 310000}'


class StubApi:
    """Stands in for api_request; records every call so we can assert that no
    generating request was made."""

    def __init__(self, subscription):
        self.subscription = subscription
        self.calls = []

    def __call__(self, key, method, path, body=None, timeout=120):
        self.calls.append((method, path))
        if path.startswith("/v1/user/subscription"):
            return self.subscription
        return (200, b"RIFFgenerated-audio", "")

    @property
    def generating_calls(self):
        return [c for c in self.calls if c[0] == "POST"]


class CreditMeterTests(unittest.TestCase):
    """Hermetic: no network, and no writes outside a temp dir.

    The staging redirect is not decorative. While building these tests a probe
    run wrote stub audio into the real Eclipse/Saved/AudioStaging and clobbered
    its manifest -- which is gitignored, so there was nothing to restore. Worse,
    a poisoned manifest makes a later real run report "[cache] hit, 0 credits"
    and silently skip generating the asset for real. Tests around a generator
    must never be able to reach the paths that generator writes to.
    """

    def setUp(self):
        self._real = gen.api_request
        self._tmp = tempfile.mkdtemp(prefix="eclipse_credit_meter_")
        self._paths = (gen.STAGING, gen.MANIFEST_PATH)
        gen.STAGING = self._tmp
        gen.MANIFEST_PATH = os.path.join(self._tmp, "manifest.json")

    def tearDown(self):
        gen.api_request = self._real
        gen.STAGING, gen.MANIFEST_PATH = self._paths
        shutil.rmtree(self._tmp, ignore_errors=True)

    # ---- get_usage reports failure instead of hiding it -------------------
    def test_get_usage_returns_reason_on_401(self):
        gen.api_request = StubApi(UNAUTHORIZED)
        usage, err = gen.get_usage("k")
        self.assertIsNone(usage)
        self.assertIn("user_read", err)
        self.assertIn("401", err)

    def test_get_usage_succeeds_when_scope_present(self):
        gen.api_request = StubApi((200, OK_BODY, ""))
        usage, err = gen.get_usage("k")
        self.assertEqual(err, "")
        self.assertEqual(usage["character_count"], 1000)

    # ---- the pre-flight gate refuses, loudly ------------------------------
    def test_preflight_raises_and_names_the_missing_scope(self):
        gen.api_request = StubApi(UNAUTHORIZED)
        with self.assertRaises(gen.UnmeasurableSpendError) as cm:
            gen.require_usage_measurement("k")
        msg = str(cm.exception)
        self.assertIn("REFUSING TO GENERATE", msg)
        self.assertIn("user_read", msg)

    def test_preflight_cannot_be_passed_by_a_falsy_return(self):
        """The old bug: a caller doing `if not usage: pass` would sail on.
        require_usage_measurement must raise, never return something falsy."""
        gen.api_request = StubApi(UNAUTHORIZED)
        try:
            result = gen.require_usage_measurement("k")
        except gen.UnmeasurableSpendError:
            return
        self.fail(f"gate returned {result!r} instead of raising - a caller that "
                  f"only checks truthiness would generate blind")

    # ---- THE load-bearing one --------------------------------------------
    def test_no_generation_happens_when_spend_is_unmeasurable(self):
        """If the meter is blind, not a single POST may reach the API."""
        stub = StubApi(UNAUTHORIZED)
        gen.api_request = stub
        buf = io.StringIO()
        with redirect_stdout(buf):
            rc = gen.main_with_key("k")
        self.assertNotEqual(rc, 0, "run reported success without measured spend")
        self.assertEqual(
            stub.generating_calls, [],
            f"credits were spent while unmeasurable: {stub.generating_calls}")
        self.assertIn("REFUSING TO GENERATE", buf.getvalue())

    def test_blind_run_writes_nothing_to_staging(self):
        """A refused run must leave no manifest and no audio behind. A stub or
        half-written manifest is worse than nothing: process() treats a manifest
        entry plus a file on disk as a cache hit and skips the real generation."""
        gen.api_request = StubApi(UNAUTHORIZED)
        with redirect_stdout(io.StringIO()):
            gen.main_with_key("k")
        leftovers = [p for p in Path(self._tmp).rglob("*") if p.is_file()]
        self.assertEqual(leftovers, [],
                         f"refused run left files behind: {leftovers}")

    def test_after_reading_failure_is_not_silent(self):
        """Credits are already gone by then, so a failed after-reading must fail
        the run and mark the manifest, never quietly write nothing."""
        class HalfBlind(StubApi):
            def __init__(self):
                super().__init__((200, OK_BODY, ""))
                self.seen_post = False

            def __call__(self, key, method, path, body=None, timeout=120):
                if method == "POST":
                    self.seen_post = True
                if path.startswith("/v1/user/subscription") and self.seen_post:
                    return UNAUTHORIZED          # scope revoked mid-run
                return super().__call__(key, method, path, body, timeout)

        gen.api_request = HalfBlind()
        buf = io.StringIO()
        with redirect_stdout(buf):
            rc = gen.main_with_key("k")
        self.assertEqual(rc, 4, "unmeasured spend must not report success")
        self.assertIn("SPEND UNMEASURED", buf.getvalue())
        import json
        man = json.load(open(gen.MANIFEST_PATH, encoding="utf-8"))
        self.assertIsNone(man["usage_credits"]["last_run_delta"])
        self.assertIn("WARNING", man["usage_credits"])


if __name__ == "__main__":
    result = unittest.main(exit=False, verbosity=2).result
    if result.wasSuccessful():
        print("\nCredit meter OK: generation is impossible without measured spend.")
    sys.exit(0 if result.wasSuccessful() else 1)
