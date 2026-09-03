"""Unit tests for the per-field measurement maths.

These are the numbers the AGC and the auto-calibration servos steer on, so
an error here is not cosmetic: it moves the decode.  Every signal below is
synthesised at a level the test states in IRE, and the expected answer is
the closed-form one for that level.

Both functions reach into a field's demodulated or raw data, but what they
do -- median the sync tip and the back porch of every usable line and the
VITS white bars (detect_levels), or take the RF envelope over a known-black
and a known-white window (black_to_white_rf_ratio) -- is per-field
measurement with no history, and a synthetic field pins it exactly.
"""

import numpy as np
import pytest

from lddecode.metrics import black_to_white_rf_ratio, detect_levels
from synthetic_field import make_field, make_rf

pytestmark = [pytest.mark.unit, pytest.mark.dsp]

SYSTEMS = ["NTSC", "PAL"]


@pytest.fixture(scope="module")
def rfs():
    return {system: make_rf(system) for system in SYSTEMS}


@pytest.fixture
def rf(rfs, request):
    return rfs[request.param]


parametrize_system = pytest.mark.parametrize("rf", SYSTEMS, indirect=True)


def picture_field(rf, ire, length=20000):
    """A field whose output picture sits at a constant IRE level."""
    field = make_field(rf)
    code = float(field.hz_to_output(rf.iretohz(0.0)))
    picture = np.full(length, code + np.asarray(ire) * field.out_scale)
    field.dspicture = np.round(picture).astype(np.uint16)
    return field


# --- detect_levels ------------------------------------------------------


def levels_field(rf, sync_ire=None, ire0=0.0, white_ire=100.0, linelen=None,
                 lines=280):
    """A field whose demodulated data sits at stated levels.

    demod_05 carries one sync pulse per line against a back porch at `ire0`
    (the two things detect_levels medians); demod is held at `white_ire`,
    which is where the VITS white bars are read from.
    """
    if sync_ire is None:
        sync_ire = rf.DecoderParams["vsync_ire"]
    if linelen is None:
        linelen = rf.linelen

    linelocs = np.arange(lines, dtype=np.float64) * linelen
    length = int(linelocs[-1] + 2 * linelen)

    field = make_field(rf, linelocs=linelocs,
                       video={"demod": np.full(length, rf.iretohz(white_ire)),
                              "demod_05": np.full(length, rf.iretohz(ire0))})

    hsync_us = rf.SysParams["hsyncPulseUS"]
    for i, loc in enumerate(linelocs[:-1]):
        width = field.usectoinpx(hsync_us, i)
        field.data["video"]["demod_05"][int(loc): int(loc + width)] = rf.iretohz(sync_ire)

    return field


@parametrize_system
def test_detect_levels_recovers_the_synthesised_levels(rf):
    field = levels_field(rf)
    sync_hz, ire0_hz, ire100_hz = detect_levels(rf, field, 200)

    assert sync_hz == pytest.approx(rf.iretohz(rf.DecoderParams["vsync_ire"]))
    assert ire0_hz == pytest.approx(rf.iretohz(0.0))
    assert ire100_hz == pytest.approx(rf.iretohz(100.0))


@parametrize_system
def test_detect_levels_tracks_a_mistuned_disc(rf):
    """The AGC exists because the levels drift; the measurement has to follow
    them rather than return the nominal values."""
    field = levels_field(rf, sync_ire=-35.0, ire0=3.0, white_ire=104.0)
    sync_hz, ire0_hz, ire100_hz = detect_levels(rf, field, 200)

    assert sync_hz == pytest.approx(rf.iretohz(-35.0))
    assert ire0_hz == pytest.approx(rf.iretohz(3.0))
    assert ire100_hz == pytest.approx(rf.iretohz(104.0))


@parametrize_system
def test_detect_levels_undoes_wow_before_medianing(rf):
    """A line 1% long was played 1% slow, so its demodulated frequencies are
    1% low; the measurement divides that back out before pooling lines."""
    field = levels_field(rf, linelen=rf.linelen * 1.01)
    sync_hz, ire0_hz, _ = detect_levels(rf, field, 200)

    assert sync_hz == pytest.approx(
        rf.iretohz(rf.DecoderParams["vsync_ire"]) * 1.01, rel=1e-6
    )
    assert ire0_hz == pytest.approx(rf.iretohz(0.0) * 1.01, rel=1e-6)


@parametrize_system
def test_lines_too_far_from_the_nominal_length_are_not_measured(rf):
    """Beyond +/-2% the line loc is more likely wrong than the disc slow, and
    the wow correction would inject a bogus level."""
    field = levels_field(rf, linelen=rf.linelen * 1.10)
    sync_hz, ire0_hz, _ = detect_levels(rf, field, 200)

    assert sync_hz == rf.iretohz(rf.DecoderParams["vsync_ire"])
    assert ire0_hz == rf.iretohz(0.0)


@parametrize_system
def test_a_white_bar_outside_spec_falls_back_to_the_nominal_level(rf):
    """The VITS white bar is only usable if it reads near 100 IRE; a disc
    without one (or a field that lost it) must not drag the reference down."""
    field = levels_field(rf, white_ire=60.0)
    _, _, ire100_hz = detect_levels(rf, field, 200)

    assert ire100_hz == rf.iretohz(100.0)


# --- black_to_white_rf_ratio --------------------------------------------


def rf_ratio_field(rf, white_ire=100.0, black_rf_rms=200.0,
                   white_rf_rms=800.0):
    """A field with a flat picture at `white_ire` over every VITS white
    window, and raw RF whose envelope is `black_rf_rms` under the black
    line and `white_rf_rms` under the white bar.

    The picture decides *whether* the white reference is usable; the raw
    data decides the ratio, so the two are set independently here.
    """
    field = make_field(rf)
    code = float(field.hz_to_output(rf.iretohz(0.0)))
    n_picture = (field.outlinecount + 1) * rf.SysParams["outlinelen"]
    picture = np.full(n_picture, code + white_ire * field.out_scale)
    field.dspicture = np.round(picture).astype(np.uint16)

    # A constant-amplitude square wave has an exact standard deviation, so
    # the expected ratio is closed form rather than a sampling estimate.
    raw = np.zeros(int(rf.linelen * (rf.SysParams["frame_lines"] // 2 + 20)))

    def fill(line_spec, delay_key, level):
        sl = field.lineslice(*line_spec)
        delay = int(rf.delays[delay_key])
        seg = slice(sl.start - delay, sl.stop - delay)
        n = seg.stop - seg.start
        raw[seg] = level * np.where(np.arange(n) % 2, 1.0, -1.0)

    fill(rf.SysParams["blacksnr_slice"], "video_sync", black_rf_rms)
    fill(rf.SysParams["LD_VITS_whitelocs"][0], "video_white", white_rf_rms)
    field.rawdata = raw
    return field


@parametrize_system
def test_the_ratio_is_the_black_rf_envelope_over_the_white_one(rf):
    field = rf_ratio_field(rf, black_rf_rms=200.0, white_rf_rms=800.0)

    assert black_to_white_rf_ratio(rf, field) == pytest.approx(0.25, abs=5e-5)


@parametrize_system
def test_a_compressed_envelope_raises_the_ratio(rf):
    """A disc whose white RF has fallen towards its black RF reads higher;
    that rise is what the MTF servo steers on."""
    nominal = black_to_white_rf_ratio(
        rf, rf_ratio_field(rf, black_rf_rms=200.0, white_rf_rms=800.0))
    compressed = black_to_white_rf_ratio(
        rf, rf_ratio_field(rf, black_rf_rms=200.0, white_rf_rms=400.0))

    assert compressed > nominal
    assert compressed == pytest.approx(0.5, abs=5e-5)


@parametrize_system
def test_a_field_with_no_white_reference_declines_to_measure(rf):
    """Without a VITS bar reading 90-110 IRE there is nothing to normalise
    against, so the field feeds no sample into the pool at all."""
    field = rf_ratio_field(rf, white_ire=55.0)

    assert black_to_white_rf_ratio(rf, field) is None


@parametrize_system
def test_the_ratio_is_rounded_to_four_places(rf):
    """The pool has always held four decimal places; the servo's dead-bands
    are set against that quantisation."""
    field = rf_ratio_field(rf, black_rf_rms=1.0, white_rf_rms=3.0)
    ratio = black_to_white_rf_ratio(rf, field)

    assert ratio == pytest.approx(0.3333, abs=1e-12)
