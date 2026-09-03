"""Per-field level measurement and partial NTSC comb filter for ld-decode.

The measurements here are the ones the decode itself steers on:
detect_levels feeds the AGC and black_to_white_rf_ratio the MTF servo.
CombNTSC is the NTSC comb filter, used by the analysis oracles.
"""

import numpy as np

from .filters import inrange
from .dsp import nb_median, roundfloat


def detect_levels(rf, field, output_lines):
    """Sync, 0 IRE and 100 IRE levels of a field, from HSYNC areas and
    VITS white areas.  A pure per-field measurement (used by the AGC),
    so it can run wherever the field's data lives."""
    sync_hzs = []
    ire0_hzs = []
    ire100_hzs = []

    for wl in (
        rf.SysParams['LD_VITS_whitelocs'] + rf.SysParams['LD_VITS_code_slices']
    ):
        # Code slice areas have a fourth value for percentile.
        ls = field.lineslice(*wl[:3])
        cut = field.data['video']['demod'][ls]
        freq = np.percentile(cut, 50 if len(wl) == 3 else wl[3])
        freq_ire = rf.hztoire(freq, spec=True)

        if inrange(freq_ire, 95, 110):
            ire100_hzs.append(freq)

    for line in range(12, output_lines):
        lsa = field.lineslice(line, 0.25, 4)

        begin_ire0 = rf.SysParams["colorBurstUS"][1]
        end_ire0 = rf.SysParams["activeVideoUS"][0]
        lsb = field.lineslice(line, begin_ire0 + 0.25, end_ire0 - begin_ire0 - 0.5)

        # compute wow adjustment
        thislinelen = (
            field.linelocs[line + field.lineoffset]
            - field.linelocs[line + field.lineoffset - 1]
        )
        adj = rf.linelen / thislinelen

        if inrange(adj, 0.98, 1.02):
            sync_hzs.append(nb_median(field.data["video"]["demod_05"][lsa]) / adj)
            ire0_hzs.append(nb_median(field.data["video"]["demod_05"][lsb]) / adj)

    # if any of the levels are missing, use the default levels
    vsync_hz   = rf.iretohz(rf.DecoderParams["vsync_ire"])

    m_synchz   = np.median(sync_hzs)   if len(sync_hzs)   else vsync_hz
    m_ire0hz   = np.median(ire0_hzs)   if len(ire0_hzs)   else rf.iretohz(0)
    m_ire100hz = np.median(ire100_hzs) if len(ire100_hzs) else rf.iretohz(100)

    return m_synchz, m_ire0hz, m_ire100hz


class CombNTSC:
    """*partial* NTSC comb filter with optional 3D inter-frame filtering.

    Accepts 2-4 fields.  Metrics are always for the last field (second field
    of the current frame).  With two frames (4 fields, or 2 same-parity
    fields) 3D comb filtering is applied in splitIQ_line via inter-frame
    subtraction of the 1D comb buffers — no motion correction.
    """

    def __init__(self, fields):
        if not isinstance(fields, (list, tuple)):
            fields = [fields]
        self.fields = fields
        self.cbuffer = [self.buildCBuffer(f) for f in fields]

    @property
    def field(self):
        return self.fields[-1]

    @property
    def has_3d(self):
        if len(self.fields) < 2:
            return False
        if len(self.fields) >= 4:
            return True
        return self.fields[0].isFirstField == self.fields[-1].isFirstField

    @property
    def _ref_idx(self):
        return max(0, len(self.fields) - 3)

    def getlinephase(self, fnum, line):
        fieldID = self.fields[fnum].fieldPhaseID

        if (line % 2) == 0:
            return (fieldID == 1) | (fieldID == 4)
        else:
            return (fieldID == 2) | (fieldID == 3)

    def buildCBuffer(self, field, subset=None):
        data = field.dspicture

        if subset is not None:
            data = data[subset]

        # 1D bandpass at fSC: tc1 = (line[h] - ((line[h-2] + line[h+2]) / 2))
        fldata = data.astype(np.float32)
        cbuffer = np.zeros_like(fldata)

        cbuffer[2:-2] = (fldata[:-4] + fldata[4:]) / 2
        cbuffer[2:-2] -= fldata[2:-2]

        return cbuffer

    def splitIQ_line(self, line, sl):
        """Demodulate chroma into I and Q for the primary field.

        When 3D is available, applies inter-frame subtraction before
        demodulation: C = (current_1d - prev_frame_1d) / 2
        """
        fnum = len(self.fields) - 1
        cbuffer = self.cbuffer[fnum][sl]

        if self.has_3d:
            cbuffer = (cbuffer - self.cbuffer[self._ref_idx][sl]) / 2

        linephase = self.getlinephase(fnum, line)

        sq = cbuffer[::2].copy()
        si = cbuffer[1::2].copy()

        if not linephase:
            si[0::2] = -si[0::2]
            sq[1::2] = -sq[1::2]
        else:
            si[1::2] = -si[1::2]
            sq[0::2] = -sq[0::2]

        return si, sq

    def calcLine19Info(self):
        """ returns color burst phase (ideally 147 degrees) and (unfiltered!) SNR """
        f = self.field

        l19_slice = f.lineslice_tbc(19, 0, 40)
        l19_slice_i70 = f.lineslice_tbc(19, 14, 18)

        ire_out = f.output_to_ire(f.dspicture[l19_slice_i70])
        if not ((np.max(ire_out) < 100) and (np.min(ire_out) > 40)):
            return None, None, None

        if self.has_3d:
            ref_f = self.fields[self._ref_idx]
            ire_ref = ref_f.output_to_ire(ref_f.dspicture[l19_slice_i70])
            if not ((np.max(ire_ref) < 100) and (np.min(ire_ref) > 40)):
                return None, None, None

        si, sq = self.splitIQ_line(19, l19_slice)

        sl = slice(110, 230)
        cdata = np.sqrt((si[sl] ** 2.0) + (sq[sl] ** 2.0))

        phase = np.arctan2(np.mean(si[sl]), np.mean(sq[sl])) * 180 / np.pi
        if phase < 0:
            phase += 360

        signal = np.mean(cdata)
        noise = np.std(cdata)
        snr = 20 * np.log10(signal / noise)

        return signal / (2 * f.out_scale), phase, snr


def black_to_white_rf_ratio(rf, f):
    """RF envelope ratio between a black line and a white VITS bar.

    The MTF servo's calibration input (LDdecode.bw_ratios): the standard
    deviation of the *raw* RF over a known-black line divided by the same
    over a known-white VITS bar, so it reports how far the disc's RF
    envelope is compressed relative to a nominal pressing.  Both windows
    are taken in raw-sample coordinates, shifted back by the demodulator's
    group delay for that signal level.

    The white bar is the first entry of SysParams["LD_VITS_whitelocs"]
    whose TBC line measures 90-110 IRE; a field carrying none (no VITS,
    or a mistracked one) has no reference and returns None.  The result is
    rounded to four decimal places, which is the precision the pool has
    always held.

    Returns the ratio (dimensionless) or None.
    """
    def envelope_rms(line_spec, delay_key):
        """Standard deviation of the raw RF over a TBC line window,
        shifted back into raw-sample coordinates by the demodulator's
        group delay for that signal level."""
        sl = f.lineslice(*line_spec)
        delay = int(rf.delays[delay_key])
        return np.std(f.rawdata[sl.start - delay: sl.stop - delay])

    white_rf = None
    for wl in rf.SysParams["LD_VITS_whitelocs"]:
        wl_slice = f.lineslice_tbc(*wl)
        if inrange(np.mean(f.output_to_ire(f.dspicture[wl_slice])), 90, 110):
            white_rf = envelope_rms(wl, "video_white")
            break

    if white_rf is None:
        return None

    black_rf = envelope_rms(rf.SysParams["blacksnr_slice"], "video_sync")

    ratio = float(roundfloat(black_rf / white_rf, places=4))
    return ratio if np.isfinite(ratio) else None
