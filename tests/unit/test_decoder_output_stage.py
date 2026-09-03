"""Unit tests for the split of LDdecode.writeout into its commit-time
half and the output-stage half (_write_field) that trails it on the
output lane with -t N.

The commit thread must do only what later commits depend on (the
metadata list, the written-field count), hand the field over as a view
frozen at commit-time parameters, and keep the queue in order.  These
tests drive the methods on a stub decoder with recording collaborators.
"""

import types

import numpy as np
import pytest

from lddecode import decoder as D
from lddecode.decoder import LDdecode

pytestmark = [pytest.mark.unit, pytest.mark.decode, pytest.mark.parallel]


class RecordingLane:
    def __init__(self):
        self.jobs = []

    def submit(self, fn, *args):
        self.jobs.append((fn, args))


def stub_field(vsync_ire=-40.0, first=True):
    rf = types.SimpleNamespace(DecoderParams={
        "vsync_ire": vsync_ire, "chroma_dg_slope": 0.002, "chroma_dg_phase": 0.0})
    return types.SimpleNamespace(rf=rf, isFirstField=first,
                                 dspicture=np.arange(8, dtype=np.uint16))


def stub_decoder(lane=None, cvbs=None):
    it = types.SimpleNamespace(
        fieldinfo=[], fields_written=3, cvbs_writer=cvbs,
        _output_lane=lane, fdoffset=12345.0, written=[],
    )
    it.writeout = lambda ds: LDdecode.writeout(it, ds)
    it._pair_cvbs_view = lambda v: LDdecode._pair_cvbs_view(it, v)
    it._write_field = lambda job: it.written.append(job)
    it._log_speculation = lambda r, d="": LDdecode._log_speculation(it, r, d)
    return it


def dataset(f, picture=None, audio=None, efm=None):
    fi = {"isFirstField": True}
    if picture is None:
        picture = f.dspicture
    return (f, fi, picture, audio, efm)


def test_commit_half_records_the_field_and_queues_the_rest_in_order():
    lane = RecordingLane()
    it = stub_decoder(lane=lane, cvbs=object())
    f = stub_field()
    audio = np.zeros(20, dtype=np.int16)

    it.writeout(dataset(f, audio=audio))
    it.writeout(dataset(f))

    assert len(it.fieldinfo) == 2
    assert it.fields_written == 5
    assert it.written == []  # nothing written on the commit thread
    assert len(lane.jobs) == 2
    assert all(fn is it._write_field for fn, _ in lane.jobs)
    # the queued metadata dicts are the ones appended, in commit order
    assert [args[0][1] for _, args in lane.jobs] == it.fieldinfo


def test_the_queued_field_is_a_view_frozen_at_commit_time_parameters():
    lane = RecordingLane()
    it = stub_decoder(lane=lane, cvbs=object())
    f = stub_field(vsync_ire=-40.0)
    it.writeout(dataset(f))
    f.rf.DecoderParams["vsync_ire"] = -43.0   # AGC moves after the commit

    view = lane.jobs[0][1][0][0]
    assert view is not f and view.dspicture is f.dspicture
    assert view.rf.DecoderParams["vsync_ire"] == -40.0


def test_without_a_lane_the_field_is_written_inline():
    it = stub_decoder(cvbs=object())
    it.writeout(dataset(stub_field()))
    assert len(it.written) == 1 and it.fields_written == 4


def test_the_picture_is_left_to_the_cvbs_writer():
    it = stub_decoder(cvbs=object())
    f = stub_field()
    it.writeout(dataset(f))
    assert it.written[0][2] is f.dspicture


def test_a_cvbs_frame_is_written_under_its_second_fields_parameters():
    """The writer resamples both fields when the second arrives, so the
    inline write read the second field's commit-time parameters for the
    first field too; the queued views must agree."""
    lane = RecordingLane()
    it = stub_decoder(lane=lane, cvbs=object())
    a = stub_field(first=True)
    it.writeout(dataset(a))
    a.rf.DecoderParams["chroma_dg_phase"] = 0.03   # trim between the fields
    b = stub_field(first=False)
    b.rf.DecoderParams["chroma_dg_phase"] = 0.03
    it.writeout(dataset(b))

    view_a = lane.jobs[0][1][0][0]
    view_b = lane.jobs[1][1][0][0]
    assert view_a.rf is view_b.rf
    assert view_a.rf.DecoderParams["chroma_dg_phase"] == 0.03
    # and the pairing is consumed: a later first field starts afresh
    it.writeout(dataset(stub_field(first=True)))
    assert lane.jobs[2][1][0][0].rf is not view_b.rf


def test_a_speculation_reject_is_logged_without_touching_the_output_lane(
        monkeypatch):
    """The reject is a diagnostic only: it goes to the decode log and
    queues nothing behind the fields still to be written."""
    seen = []
    monkeypatch.setattr(D.logs, "logger", types.SimpleNamespace(
        debug=lambda *a, **k: seen.append(a)))

    lane = RecordingLane()
    it = stub_decoder(lane=lane, cvbs=object())
    it._log_speculation("stale-imtf", "detail")

    assert lane.jobs == []
    assert len(seen) == 1 and "stale-imtf" in seen[0]
