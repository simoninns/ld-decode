# File formats

The ld-decode application accepts FM RF captures input in '10-bit packed' format. This is a bit-stream of 10-bit unsigned integers produced by the Domesday Duplicator's capture GUI (typically with the .lds file extension).  The input bit-stream is expected to be the raw LaserDisc RF captured at 40 Million Samples Per Second (MSPS) with each sample being 10-bits.

*The decoder also supports FLAC compressed captures, and lower sample rates if the input frequency is defined and bit-depths such as 8-bit & 16-bit.

## Video output

ld-decode writes composite video as a `.cvbs` file with a `.meta` SQLite
sidecar, normatively defined by the
[CVBS file format specification](https://github.com/simoninns/cvbs-file-format-specification).
Samples are on the 4x subcarrier lattice, in one of two encodings:

- `CVBS_U10_4FSC` — the normative 10-bit sample value in an `s16le`
  container, with signed headroom below blanking. ld-decode's default
  for PAL.
- `CVBS_U16_4FSC` — the same signal scaled into the full 16-bit range.
  ld-decode's default for NTSC.

`--cvbs-encoding` selects the encoding explicitly. Frame geometry and
sample rates:

- PAL — 709379 samples/frame (1135.0064 samples/line, 625 lines) at
  17734475 Hz. The lattice is not line-locked, so a frame carries a
  4-sample slip.
- NTSC — 910x525 samples/frame at 14318181 Hz (orthogonal).

## Audio and metadata sidecars

- `<out>_audio_0.wav` — the analogue audio track, as the specification's
  SMPTE 272M profile: 48 kHz, 24-bit, synchronous to the stored frames.
- `<out>.meta` — the capture's SQLite metadata: preset, sample encoding,
  levels, per-frame lock state and sequence continuity.
- `<out>.dropouts.meta` — dropout runs, indexed per frame.

## EFM output sidecars

When digital audio is enabled (the default), the decoder writes the EFM
stream alongside the video output:

- `.efm` — one byte per recovered T-value, in disc order. Each byte
  packs the T-value into its low nibble and a 4-bit demodulator doubt
  into its high nibble (`t = byte & 0x0F`, `doubt = byte >> 4`; 0 = full
  trust — so trusted bytes stay plain T-values — and high values are
  Reed-Solomon erasure candidates). A `.efm.meta` SQLite sidecar indexes
  the stream per frame, as defined by the
  [EFM extension format](https://github.com/simoninns/cvbs-file-format-specification/blob/main/docs/extensions/efm-extension-format.md)
  of the CVBS file format specification.
- `.prefm` — the filtered EFM waveform before demodulation (int16 at the
  capture sample rate), written with `--preEFM` for debugging and filter
  research.

See [EFM decoding](efm-decoding.md) for the demodulators that produce
these streams and the quality oracle that scores them.

## Example file sizes

The following file sizes show the typical disc usage consumed by an end-to-end capture and decode of a LaserDisc.

Individual decodes will vary from disc-to-disc:

* NPE - PAL CAV disc with 54348 frames
* NPE - LDS (RF Capture 40MSPS 10-bit packed) = 109.4GB
* NPE - LDF (RF Capture 40MSPS 16-bit FLAC compressed) = 22.6GB (Estimate*)
* NPE - CVBS (CVBS_U10_4FSC) = 77.1GB
* NPE - WAV (48K 24-bit) = 626.1MB
