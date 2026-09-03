"""Unit tests for the calibration-redo loop in LDdecode._commit_entry.

When a calibration loop adjusts a parameter, the field is decoded again
under the new value.  What is committed afterwards must be the *redone*
field's outputs - its picture, its EFM slice and its RF-envelope ratio -
not the ones measured under the parameters that were just replaced.
Committing the stale picture is invisible in the decode log (the servos
trace identically) and shows up only as the first written frame
differing, which is what these tests pin.

The loop is driven on a stub decoder: the collaborators it reaches for
(decodefield, decode_stage2, calibrate, the pipeline flush) are recording
substitutes, so no capture data or thread pool is involved.
"""

import types

import pytest

from lddecode.decoder import LDdecode

pytestmark = [pytest.mark.unit, pytest.mark.decode]


def stub_field(tag):
    """A field carrying only what _commit_entry reads from one."""
    return types.SimpleNamespace(
        tag=tag,
        valid=True,
        sync_confidence=100,
        inlinelen=100.0,
        linelocs=[0.0] + [100.0 * n for n in range(1, 400)],
    )


def stub_decoder(calibrate_verdicts):
    """A decoder whose calibrate() returns each verdict in turn.

    decode_stage2 hands back outputs tagged with the field it was given,
    so the committed values can be traced to the decode they came from.
    """
    it = types.SimpleNamespace(
        committed=[],
        stage2_calls=[],
        calibrate_args=[],
        redo_count=0,
        output_lines=263,
        fieldstack=[stub_field("prev")],
        bw_ratios=[],
        fields_written=0,
        deemp_calibrated=True,
        pipeline_warm=True,
        block_cache=None,
        process_demod=False,
        use_field_jobs=False,
        _job_engine=None,
        _agc_adjusted_last=False,
        _fields_since_redo=0,
        mtf_level=1.0,
        fdoffset=0.0,
        rf=types.SimpleNamespace(linelen=100.0, DecoderParams={}),
    )

    verdicts = list(calibrate_verdicts)

    def calibrate(f, bw_ratio, redos):
        it.calibrate_args.append((f.tag, bw_ratio, redos))
        return verdicts.pop(0) if verdicts else False

    def decode_stage2(f):
        it.stage2_calls.append(f.tag)
        return (f"picture:{f.tag}", f"efm:{f.tag}", f"ratio:{f.tag}")

    def decodefield(start, mtf, prev, initphase, wide=False, trust_window=False):
        it.redo_count += 1
        return stub_field(f"redo{it.redo_count}"), 100.0

    def commit_field(f, picture, efm, audio=None, audio_ready=False):
        it.committed.append((f.tag, picture, efm, audio, audio_ready))
        return f

    it.calibrate = calibrate
    it.decode_stage2 = decode_stage2
    it.decodefield = decodefield
    it.commit_field = commit_field
    it._flush_pipeline = lambda: None
    it._restart_workers = lambda: None
    it._commit_entry = lambda entry: LDdecode._commit_entry(it, entry)
    return it


def entry_for(f):
    return {
        "f": f,
        "start": 0.0,
        "offset": 100.0,
        "independent": True,
        "initphase": False,
        "result": (f"picture:{f.tag}", f"efm:{f.tag}", f"ratio:{f.tag}"),
    }


def test_a_field_with_no_redo_commits_what_it_decoded():
    it = stub_decoder([False])
    it._commit_entry(entry_for(stub_field("first")))

    assert it.committed == [("first", "picture:first", "efm:first", None, False)]
    assert it.redo_count == 0


def test_a_redone_field_commits_the_redecoded_outputs():
    """The whole point of a redo: the committed picture must come from the
    decode under the adjusted parameters, not the one that triggered it."""
    it = stub_decoder([True, False])
    it._commit_entry(entry_for(stub_field("first")))

    assert it.redo_count == 1
    tag, picture, efm, _, _ = it.committed[0]
    assert tag == "redo1"
    assert (picture, efm) == ("picture:redo1", "efm:redo1")


def test_the_second_calibration_pass_sees_the_redecoded_measurement():
    """calibrate() decides on the RF-envelope ratio of the field in hand;
    handing it the pre-redo ratio would settle the loop on a measurement
    the decode has already replaced."""
    it = stub_decoder([True, True, False])
    it._commit_entry(entry_for(stub_field("first")))

    assert [(tag, ratio) for tag, ratio, _ in it.calibrate_args] == [
        ("first", "ratio:first"),
        ("redo1", "ratio:redo1"),
        ("redo2", "ratio:redo2"),
    ]
    assert it.committed[0][1] == "picture:redo2"


def test_the_redo_budget_stops_at_two():
    """Each loop is dead-banded or one-shot, so two redos converge; the
    cap keeps a pathological field from spinning."""
    it = stub_decoder([True] * 6)
    it._commit_entry(entry_for(stub_field("first")))

    assert it.redo_count == 2
    assert it.committed[0][0] == "redo2"


def test_a_redone_field_recomputes_its_audio():
    """The audio was pulled for the field that has just been replaced, so
    it must not travel with the commit."""
    it = stub_decoder([True, False])
    entry = entry_for(stub_field("first"))
    entry["audio"] = "stale-audio"
    it._commit_entry(entry)

    assert it.committed[0][3] is None
    assert it.committed[0][4] is False
