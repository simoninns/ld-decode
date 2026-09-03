# ld-decode command

ld-decode's execution is largely controlled by a number of command line switches:

```
ld-decode [-h] [--start file-location] [--length frames] [--seek frame] [--PAL] [--NTSC]
          [--NTSCJ] [--cvbs-encoding {CVBS_U10_4FSC,CVBS_U16_4FSC}] [-m mtf]
          [--MTF_offset mtf_offset] [-t threads] [--demod-threads-only] [--exact-speculation]
          [--noAGC] [--noDOD] [--noEFM] [--preEFM] [--tbc_efm] [--efm_demod {pll,timing}]
          [--efm_eq_taps EFM_EQ_TAPS] [--disable_analog_audio] [--AC3]
          [--start_fileloc start_fileloc] [--ignoreleadout] [--RF_TBC] [--lowband]
          [--no_chroma_dg] [--NTSC_color_notch_filter] [--V4300D_notch_filter]
          [--V4300D_coherent_subtract] [--rf_echo_cancel] [--rf_echo RF_ECHO]
          [--V4300D_no_defer] [--deemp_low deemp_low] [--deemp_high deemp_high]
          [--deemp_strength deemp_str]
          [--wow_level_adjust_smoothing WOW_LEVEL_ADJUST_SMOOTHING]
          [--wow_interpolation_method {linear,quadratic,cubic}] [-f FREQ]
          [--analog_audio_frequency AFREQ] [--ntsc_audio_rate] [--video_bpf_low FREQ]
          [--video_bpf_high FREQ] [--video_lpf FREQ] [--video_lpf_order VLPF_ORDER]
          [--audio_filterwidth FREQ] [--use_profiler] [--write-test-ldf output.ldf]
          infile outfile
```

## Synopsis

```bash
ld-decode [OPTIONS] infile outfile
```

## Positional Arguments

### infile
**Required.** Path to the source file containing the raw RF capture data.

### outfile
**Required.** Base name for destination files. The tool will create multiple output files with this base name and appropriate extensions (e.g., `.cvbs`, `.meta`, `.efm`, `_audio_0.wav`).

## Options

### Help and Version Information

#### `-h`, `--help`
Print the option summary and exit.

#### `--version`, `-v`
Display the version number of ld-decode and exit. Does not require positional arguments.

**Example:**
```bash
ld-decode --version
```

### Basic Decoding Options

#### `--start file-location`, `-s file-location`
Jump roughly to frame n of the capture before starting decoding.
- **Type:** Float
- **Default:** 0
- **Range:** Any non-negative number
- **Note:** This performs a rough seek; use `--seek` for precise frame seeking

**Example:**
```bash
ld-decode --start 1000 input.ldf output
```

#### `--length frames`, `-l frames`
Limit the number of frames to decode.
- **Type:** Integer
- **Default:** 110000
- **Range:** Any positive integer
- **Note:** Specifies the maximum number of video frames to process

**Example:**
```bash
ld-decode --length 5000 input.ldf output
```

#### `--seek frame`, `-S frame`
Seek to a precise frame number in the capture before starting decoding.
- **Type:** Integer
- **Default:** -1 (disabled)
- **Range:** Any non-negative integer
- **Note:** More precise than `--start`; requires valid frame synchronization data

**Example:**
```bash
ld-decode --seek 2500 input.ldf output
```

#### `--start_fileloc start_fileloc`
Jump to a precise sample number in the file.
- **Type:** Float
- **Default:** -1 (disabled)
- **Range:** Any non-negative number
- **Note:** Specifies the exact sample position in the RF capture file; overrides `--start`

**Example:**
```bash
ld-decode --start_fileloc 10000000 input.ldf output
```

### Video Standard Options

**Note:** Only one video standard can be selected. Selecting both PAL and NTSC (or NTSCJ) will result in an error.

#### `--PAL`, `-p`, `--pal`
Decode the source as PAL format.
- **Default:** If neither PAL nor NTSC is specified, NTSC is assumed
- **Incompatible with:** `--NTSC`, `--NTSCJ`

**Example:**
```bash
ld-decode --PAL input.ldf output
```

#### `--NTSC`, `-n`, `--ntsc`
Decode the source as NTSC format.
- **Default:** NTSC is the default if no standard is specified
- **Incompatible with:** `--PAL`
- **Note:** Uses IRE 7.5 black level (standard NTSC)

**Example:**
```bash
ld-decode --NTSC input.ldf output
```

#### `--NTSCJ`, `-j`
Decode the source as NTSC-J (Japanese NTSC) format.
- **Default:** Disabled
- **Incompatible with:** `--PAL`
- **Note:** Uses IRE 0 black level (Japanese standard) instead of IRE 7.5

**Example:**
```bash
ld-decode --NTSCJ input.ldf output
```

#### `--cvbs-encoding {CVBS_U10_4FSC,CVBS_U16_4FSC}`
Sample encoding preset for the `.cvbs` output. Both carry the same
normative 10-bit sample domain and differ only in the container, so a
file in either encoding measures identically.
- **Default:** `CVBS_U10_4FSC` for PAL, `CVBS_U16_4FSC` for NTSC
- **Note:** `CVBS_U10_4FSC` stores the 10-bit value itself in an `s16le`
  container, keeping signed headroom below blanking; `CVBS_U16_4FSC`
  scales it into the full 16-bit range. See
  [File formats](../technical/file-formats.md).

**Example:**
```bash
ld-decode --PAL --cvbs-encoding CVBS_U16_4FSC input.ldf output
```

### Video Processing Options

#### `-m mtf`, `--MTF mtf`
MTF (Modulation Transfer Function) compensation multiplier.
- **Type:** Float
- **Default:** 1.0
- **Range:** Typically 0.5 to 2.0
- **Note:** Adjusts the compensation for frequency-dependent signal loss; values > 1.0 increase high-frequency boost

**Example:**
```bash
ld-decode -m 1.5 input.ldf output
```

#### `--MTF_offset mtf_offset`
MTF compensation offset.
- **Type:** Float
- **Default:** 0
- **Range:** Any float value
- **Note:** Additional offset applied to MTF compensation

**Example:**
```bash
ld-decode --MTF_offset 0.1 input.ldf output
```

#### `--noAGC`
Disable Automatic Gain Control.
- **Default:** AGC is enabled
- **Note:** AGC normalizes signal levels; disabling may be useful for analyzing the raw signal

**Example:**
```bash
ld-decode --noAGC input.ldf output
```

#### `--noDOD`
Disable the dropout detector.
- **Default:** Dropout detection is enabled
- **Note:** Dropouts are signal losses; disabling detection means they won't be flagged in the output

**Example:**
```bash
ld-decode --noDOD input.ldf output
```

#### `--lowband`
Use more restricted RF settings optimized for noisier disks.
- **Default:** Disabled
- **Note:** Applies more conservative filtering suitable for degraded or noisy source material

**Example:**
```bash
ld-decode --lowband input.ldf output
```

### Wow Correction Options

Wow is the playback speed variation of a spinning disc. It shifts sample
timing, and because the video is FM demodulated it shifts amplitude with
it, so the decoder corrects both: sample positions are interpolated from
the measured line locations, and each line's level is scaled by its wow
factor. Both options below tune that correction; the defaults suit a
clean capture.

#### `--wow_level_adjust_smoothing WOW_LEVEL_ADJUST_SMOOTHING`
Smooth the brightness compensation over this many lines.
- **Type:** Float (lines)
- **Default:** 0 (no smoothing)
- **Note:** The wow factor is derived from hsync pulse positions, so a
  capture with noise around sync gets noisy wow estimates and therefore
  a noisy per-line level adjustment, seen as vertical brightness banding.
  A value above 0 low-pass filters the level adjustment across lines,
  smoothing the banding while staying quick enough to follow real
  (low-frequency) wow. Raise it until the banding goes; too high and
  genuine wow-induced level variation stops being corrected.

**Example:**
```bash
ld-decode --wow_level_adjust_smoothing 4 input.ldf output
```

#### `--wow_interpolation_method {linear,quadratic,cubic}`
Spline used to interpolate sample positions between measured line
locations when correcting wow.
- **Type:** String: `linear`, `quadratic` or `cubic`
- **Default:** `linear`
- **Note:** `linear` treats the speed as constant across each line;
  the higher orders fit a smoother speed curve through the line
  locations (`cubic` with natural end conditions). Smoother is not
  automatically better — a higher-order spline follows measurement
  noise in the line locations as readily as it follows real wow, so
  only move off `linear` if a capture visibly benefits.

**Example:**
```bash
ld-decode --wow_interpolation_method cubic input.ldf output
```

### Video Filter Options

#### `--video_bpf_low FREQ`
Video band-pass filter low-end frequency.
- **Type:** FREQ (see Frequency Format section)
- **Default:** System-dependent (varies by PAL/NTSC)
- **Note:** Sets the lower cutoff frequency for the video band-pass filter

**Example:**
```bash
ld-decode --video_bpf_low 2.5MHz input.ldf output
```

#### `--video_bpf_high FREQ`
Video band-pass filter high-end frequency.
- **Type:** FREQ (see Frequency Format section)
- **Default:** System-dependent (varies by PAL/NTSC)
- **Note:** Sets the upper cutoff frequency for the video band-pass filter

**Example:**
```bash
ld-decode --video_bpf_high 13MHz input.ldf output
```

#### `--video_lpf FREQ`
Video low-pass filter frequency.
- **Type:** FREQ (see Frequency Format section)
- **Default:** System-dependent (varies by PAL/NTSC)
- **Note:** Sets the cutoff frequency for the video low-pass filter

**Example:**
```bash
ld-decode --video_lpf 5.0MHz input.ldf output
```

#### `--video_lpf_order VLPF_ORDER`
Video low-pass filter order.
- **Type:** Integer
- **Default:** -1 (use system default)
- **Range:** Positive integers (typically 1-10)
- **Note:** Higher orders provide sharper cutoff but more processing

**Example:**
```bash
ld-decode --video_lpf_order 8 input.ldf output
```

### NTSC-Specific Video Options

#### `--NTSC_color_notch_filter`, `-N`
Mitigate interference from analog audio in red colors in NTSC captures.
- **Default:** Disabled
- **Note:** Only effective with NTSC video standard; addresses crosstalk from audio carriers

**Example:**
```bash
ld-decode --NTSC --NTSC_color_notch_filter input.ldf output
```

### PAL-Specific Video Options

Pioneer LD-V4300D players leak their digital-audio master clock
(192 x 44.1 kHz = 8.4672 MHz, with weaker satellites at ±88.2 kHz
multiples) into the RF output when playing PAL discs with digital audio.
The tone beats against the video FM carrier and shows up as a fine wavy
pattern rolling through solid picture areas.

#### `--V4300D_coherent_subtract`
Remove the LD-V4300D spur by estimating the clock line (and its
satellites) coherently and subtracting the fitted tones from the
spectrum, without cutting holes in the video sidebands. Self-disabling
on captures without the spur, and inactive on blocks with no video
carrier, so it is safe from the first block and keeps the parallel
(multi-threaded) decode path available.
- **Default:** Disabled
- **Note:** Only effective with PAL video standard

**Example:**
```bash
ld-decode --PAL --V4300D_coherent_subtract input.ldf output
```

#### `--V4300D_notch_filter`, `-V`
Legacy alias for `--V4300D_coherent_subtract` (which supersedes the
original FFT-bin notch: it also removes the spur's leakage skirts and
its satellites). Kept so existing command lines continue to work.

#### `--V4300D_no_defer`
Obsolete; accepted for compatibility and ignored. The spur filter no
longer defers until sync acquisition and never forces a serial decode.

#### `--no_chroma_dg`
Disable the chroma differential gain and phase servo.
- **Default:** Enabled (the servo runs)
- **Note:** PAL only. The servo measures how chroma amplitude and phase
  vary with luminance from the ITS modulated staircase in the VITS, and
  the CVBS writer corrects both out. It is self-limiting: a disc with no
  modulated staircase never feeds the pool, and a measured slope inside
  the spec band is held at zero, so a conforming capture is corrected by
  nothing. Nothing about decoding depends on it — every other servo
  measures upstream of the correction — so disabling it changes only
  the written chroma. Use this to see the uncorrected signal, or if a
  capture's staircase is damaged enough to mislead the estimator. See
  [VITS servos](../technical/vits-servos.md) for the measurement and
  the corrector.

**Example:**
```bash
ld-decode --PAL --no_chroma_dg input.ldf output
```

### Deemphasis Options

Video signals are typically pre-emphasized during recording and must be de-emphasized during playback.

#### `--deemp_low deemp_low`
Deemphasis low frequency in nanoseconds.
- **Type:** Float
- **Default:** System-dependent
  - NTSC: 3.125MHz equivalent
  - PAL: 2.5MHz equivalent
- **Range:** Any positive float
- **Note:** Specifies the time constant for low-frequency deemphasis

**Example:**
```bash
ld-decode --deemp_low 320 input.ldf output
```

#### `--deemp_high deemp_high`
Deemphasis high frequency in MHz.
- **Type:** Float
- **Default:** System-dependent
  - NTSC: 8.33MHz
  - PAL: 10MHz
- **Range:** Any positive float
- **Note:** Specifies the frequency for high-frequency deemphasis

**Example:**
```bash
ld-decode --deemp_high 10.0 input.ldf output
```

#### `--deemp_strength deemp_str`
Strength of deemphasis filter.
- **Type:** Float
- **Default:** 1.0
- **Range:** Typically 0.0 to 2.0
- **Note:** Multiplier for deemphasis effect; 1.0 is standard, <1.0 reduces effect, >1.0 increases effect

**Example:**
```bash
ld-decode --deemp_strength 0.8 input.ldf output
```

### Audio Decoding Options

#### `--disable_analog_audio`, `--disable_analogue_audio`, `--daa`
Disable analog audio decoding.
- **Default:** Analog audio is enabled at 44100Hz
- **Note:** Useful when only video is needed or when processing digital-only sources

**Example:**
```bash
ld-decode --disable_analog_audio input.ldf output
```

#### `--analog_audio_frequency AFREQ`
Set the analog audio output sampling frequency.
- **Type:** Integer (Hz)
- **Default:** 44100
- **Range:** Typically 44100 or 48000
- **Note:** Output sample rate for analog audio tracks

**Example:**
```bash
ld-decode --analog_audio_frequency 48000 input.ldf output
```

#### `--ntsc_audio_rate`
Output analog audio locked to NTSC line timing instead of the default 44100Hz.
- **Default:** Off (analog audio is output at 44100Hz)
- **Effect:** Produces exactly 2.8 samples per line (1470 samples/frame, 735 per field), giving a sample rate of ~44055.944Hz that stays perfectly aligned to the NTSC video timing with no drift. The default 44100Hz rate corresponds to a non-integer 1471.47 samples/frame, which slowly drifts against the video.
- **Note:** CVBS output overrides this: the specification mandates SMPTE 272M audio (48 kHz, 24-bit, synchronous to video), so the analogue audio rate is forced to 48 kHz.
- **Note:** NTSC only. The flag is ignored (with a warning) for PAL, which is already frame-locked at 44100Hz (1764 samples/frame). Overrides `--analog_audio_frequency`.

**Example:**
```bash
ld-decode --ntsc_audio_rate input.ldf output
```

#### `--audio_filterwidth FREQ`
Set the analog audio filter width.
- **Type:** FREQ (see Frequency Format section)
- **Default:** System-dependent
- **Note:** Bandwidth of the analog audio channel filters

**Example:**
```bash
ld-decode --audio_filterwidth 150kHz input.ldf output
```

#### `--noEFM`
Disable EFM (Eight-to-Fourteen Modulation) front end for digital audio.
- **Default:** EFM decoding is enabled
- **Note:** EFM is used for digital audio (CD audio) on laserdiscs and for LV-ROM data; disabling skips digital audio extraction. See [EFM decoding](../technical/efm-decoding.md) for the EFM path, its tuning environment variables, and the `.efm` output format (including confidence-packed output).

**Example:**
```bash
ld-decode --noEFM input.ldf output
```

#### `--preEFM`
Write filtered but otherwise pre-processed EFM data.
- **Default:** Disabled
- **Note:** Outputs intermediate EFM data before full decoding; useful for debugging or custom processing

**Example:**
```bash
ld-decode --preEFM input.ldf output
```

#### `--tbc_efm`
Time-base-correct the EFM waveform onto the video line time-base before the EFM PLL.
- **Default:** Disabled (also enabled by `LDDECODE_TBC_EFM=1`)
- **Note:** Experimental; does not improve a single-capture decode. It aligns the pre-PLL EFM of multiple captures of the same disc onto a common disc-position time-base for cross-capture stacking and waveform research — see [EFM decoding](../technical/efm-decoding.md).

**Example:**
```bash
ld-decode --tbc_efm input.ldf output
```

#### `--efm_demod`, `--efm-demod`
Select the EFM demodulator that turns the equalised EFM waveform into `.efm` T-values.
- **Default:** `timing`
- **Choices:** `timing` (symbol-rate timing-recovery demodulator: per-channel-bit Mueller & Müller loop with bit-domain frame sync, sync restoration and legalised T emission), `pll` (the previous zero-crossing run-length PLL)
- **Note:** `timing` recovers noticeably more valid frames on noisy or marginal captures — it met or beat the PLL on every validation capture — and derives per-T-value confidence from its framing state. Use `pll` to reproduce pre-switch `.efm` output byte for byte. See [EFM decoding](../technical/efm-decoding.md) for the architecture and its tuning environment variables.

**Example:**
```bash
ld-decode --efm_demod pll input.ldf output
```

#### `--efm_eq_taps EFM_EQ_TAPS`
Tap count for the timing demodulator's decision-directed adaptive
equaliser.
- **Type:** Integer
- **Default:** 0 (equaliser off)
- **Range:** 0, or an odd count from 3 to 15
- **Note:** **Experimental.** The equaliser is off by default because it
  measured neutral-to-harmful on the validation captures: the
  demodulator's per-bit decisions and sync flywheel already absorb
  static linear distortion, and the remaining failures are
  noise-dominated, where the adaptation hurts. It is kept for
  experimentation on badly distorted discs — score the result with
  `analysis/efm_quality.py` before trusting it. Only applies to
  `--efm_demod timing`. See
  [EFM decoding](../technical/efm-decoding.md) for the measurements.

**Example:**
```bash
ld-decode --efm_eq_taps 5 input.ldf output
```

#### `--AC3`
Enable AC3-RF audio demodulation (NTSC only).  On AC3 LaserDiscs the
analog right audio channel carries a QPSK signal at 2.88 MHz with Dolby
Digital data at 288 kbaud.  With this option, ld-decode demodulates that
signal (see `lddecode/ac3rf.py`) and writes the raw QPSK symbols to
`output.ac3sym` (one symbol per byte, values 0-3), in disc order across
the whole decode.

The `.ac3sym` file is not playable audio by itself: framing, Reed-Solomon
error correction and AC3 frame assembly are performed downstream by
[decode-orc](https://github.com/simoninns/decode-orc)'s *AC3 RF Sink*
stage, which reads the video output, its metadata, and the `.ac3sym`
file and writes the final playable `.ac3` file.

- **Default:** Disabled
- **Note:** Only compatible with NTSC; attempting to use with PAL will result in an error
- **Incompatible with:** `--PAL`

**Example:**
```bash
ld-decode --NTSC --AC3 input.ldf output
```

The demodulator has a self-contained unit test (synthetic QPSK loopback,
no capture files needed), runnable from the repository root:
```bash
python3 -m tests.test_ac3rf
```
A quick sanity check on real output: `output.ac3sym` should grow by
about 288,000 symbols (bytes) per second of decoded video.

### RF Correction Options

A reflection in the capture path — inside the player, or in the cabling
to the capture hardware — adds a delayed copy of the RF to itself. In
the picture this is a "ghost": a faint, displaced repeat of high
contrast edges. The correction estimates the delay and amplitude of the
reflection and applies the inverse filter.

#### `--rf_echo_cancel`
Detect and cancel the reflection automatically.
- **Default:** Disabled
- **Note:** Taps are found in the RF cepstrum and re-estimated as the
  decode moves across the disc, and the correction is applied only when
  it measurably reduces the echo — on a capture with no reflection it
  is a no-op. Because the estimator carries state from block to block,
  demodulation is no longer a pure function of the block, so this forces
  a serial demod and gives up the `-t` speedup. If you already know the
  taps, use `--rf_echo` instead and keep the parallel path.

**Example:**
```bash
ld-decode --rf_echo_cancel input.ldf output
```

#### `--rf_echo RF_ECHO`
Cancel the reflection using taps you supply.
- **Type:** String: comma-separated `delay_samples:amplitude` pairs
- **Default:** Empty (use `--rf_echo_cancel` for auto-detection)
- **Note:** Delays are in input samples at the capture rate, amplitudes
  are relative to the direct signal. Supplying taps turns the correction
  on by itself (`--rf_echo_cancel` is not also needed) and overrides
  auto-detection. A fixed tap list is a plain inverse filter with no
  state, so unlike auto-detection it keeps full parallel decoding. The
  usual way to get the numbers is to run `--rf_echo_cancel` once: on
  the first detection it logs `RF echo detected - cancelling taps
  17:0.110, 28:0.050`, in the format this option accepts.

**Example:**
```bash
ld-decode --rf_echo 17:0.11,28:0.05 -t 8 input.ldf output
```

### RF Sampling Options

#### `-f FREQ`, `--frequency FREQ`
RF sampling frequency of the source file.
- **Type:** FREQ (see Frequency Format section)
- **Default:** 40MHz
- **Note:** If the source file has a different sample rate, specify it here; the decoder will resample to 40MHz internally

**Example:**
```bash
ld-decode -f 28.636363MHz input.ldf output
ld-decode -f 8fsc input.ldf output
```

### Processing Options

#### `-t threads`, `--threads threads`
Number of worker processes to decode fields with.
- **Type:** Integer
- **Default:** 0 (auto): the machine's physical cores minus 2, capped at 10
- **Range:** 1 (serial decode) to number of CPU cores
- **Note:** The workers are FFT-bound, so counting SMT (hyper-threading) siblings as cores oversubscribes the machine; an 8-core/16-thread CPU decodes faster with 6 workers than with 10. Output does not depend on the thread count beyond the calibration tolerance described under `--exact-speculation`.

**Example:**
```bash
ld-decode -t 8 input.ldf output
```

#### `--demod-threads-only`
Keep block demodulation in threads instead of decoding whole fields in
worker processes.
- **Default:** Disabled (whole-field worker processes)
- **Note:** Only meaningful alongside `-t`. Slower than the default, but
  it avoids the per-worker memory cost (~150-200 MB each), so it is the
  option to reach for on a memory-constrained machine. Some modes select
  it implicitly: `--RF_TBC` and `--AC3` consume raw RF samples at write
  time and so fall back to block-level parallelism on their own.

**Example:**
```bash
ld-decode -t 8 --demod-threads-only input.ldf output
```

#### `--exact-speculation`
Discard every field decoded ahead under superseded decoder parameters
whenever calibration adjusts one.
- **Default:** Disabled (tolerant speculation)
- **Note:** Fields are decoded speculatively, so some are already in
  flight when a calibration loop adopts a new value. By default a field
  decoded under an MTF level within 0.10 of the current one is kept, as
  are dead-band trims of the chroma differential gain — the visual
  difference is fractions of a dB at high frequencies, and a hard flush
  costs the whole pipeline. `--exact-speculation` keeps nothing a serial
  decode would not have used, making the output byte-identical to `-t 1`
  across mid-run calibration changes. Use it when comparing decodes or
  measuring conformance; every reject is logged with its cause at DEBUG
  level either way. No effect at `-t 1`.

**Example:**
```bash
ld-decode -t 8 --exact-speculation input.ldf output
```

### Output Options

#### `--RF_TBC`
Create a `.tbc.ldf` file containing time-base corrected RF data.
- **Default:** Disabled
- **Note:** Outputs the RF signal after time-base correction; useful for archival or analysis

**Example:**
```bash
ld-decode --RF_TBC input.ldf output
```

#### `--ignoreleadout`
Continue decoding after detecting the lead-out section.
- **Default:** Disabled (stop at lead-out)
- **Note:** Lead-out marks the end of the disc content; this option processes beyond that marker

**Example:**
```bash
ld-decode --ignoreleadout input.ldf output
```

### Debugging and Development Options

#### `--write-test-ldf output.ldf`
Write the input portion being decoded to a `.ldf` file for bug reporting.
- **Type:** String (filename)
- **Default:** Disabled
- **Note:** Creates a reproducible test case containing the input samples that were decoded; useful for submitting bug reports. The output file cannot be the same as the input file.

**Example:**
```bash
ld-decode --write-test-ldf test-case.ldf input.ldf output
```

#### `--use_profiler`
Enable line_profiler on select functions for performance analysis.
- **Default:** Disabled
- **Note:** Development tool for identifying performance bottlenecks; requires line_profiler to be installed

**Example:**
```bash
ld-decode --use_profiler input.ldf output
```

## Frequency Format

Many options accept frequency values with the `FREQ` type. These can be specified in several formats:

### Bare Number
A number without a suffix is interpreted as **MHz**.
```bash
--frequency 40      # 40 MHz
```

### With Suffix (case-insensitive)
- **Hz**: Hertz
  ```bash
  --frequency 40000000Hz
  ```
- **kHz**: Kilohertz (10³ Hz)
  ```bash
  --frequency 40000kHz
  ```
- **MHz**: Megahertz (10⁶ Hz)
  ```bash
  --frequency 40MHz
  ```
- **GHz**: Gigahertz (10⁹ Hz)
  ```bash
  --frequency 0.04GHz
  ```
- **fSC**: NTSC color subcarrier frequency (315/88 MHz ≈ 3.579545 MHz)
  ```bash
  --frequency 8fsc    # 8× NTSC subcarrier ≈ 28.636 MHz
  ```
- **fSCPAL**: PAL color subcarrier frequency (283.75 × 15625 + 25 Hz ≈ 4.43361875 MHz)
  ```bash
  --frequency 8fscpal # 8× PAL subcarrier ≈ 35.469 MHz
  ```

## Common Usage Examples

### Basic PAL Decode
```bash
ld-decode --PAL input.ldf output
```

### NTSC Decode with Custom Length
```bash
ld-decode --NTSC --length 30000 input.ldf output
```

### High-Quality PAL Decode with Custom Settings
```bash
ld-decode --PAL -m 1.2 --lowband -t 8 input.ldf output
```

### NTSC with AC3 Audio
```bash
ld-decode --NTSC --AC3 input.ldf output
```

### Decode Specific Frame Range
```bash
ld-decode --PAL --start 1000 --length 5000 input.ldf output
```

### Custom Sample Rate Input
```bash
ld-decode --PAL -f 28.636363MHz input.ldf output
```

### NTSC with Color Notch Filter
```bash
ld-decode --NTSC --NTSC_color_notch_filter input.ldf output
```

### PAL V4300D Capture
```bash
ld-decode --PAL --V4300D_coherent_subtract input.ldf output
```

### Video Only (No Audio)
```bash
ld-decode --PAL --disable_analog_audio --noEFM input.ldf output
```

### Create Test Case for Bug Report
```bash
ld-decode --PAL --start 5000 --length 10 --write-test-ldf bug-report.ldf input.ldf output
```

## Output Files

Based on the base name provided as `outfile`, ld-decode creates several output files:

- **`outfile.cvbs`**: Composite video, on the 4x subcarrier lattice
- **`outfile.meta`**: Capture metadata (SQLite), as the CVBS specification defines it
- **`outfile.dropouts.meta`**: Dropout runs, indexed per frame (SQLite)
- **`outfile_audio_0.wav`**: Analogue audio, 48 kHz 24-bit (if enabled)
- **`outfile.efm`**: Digital audio EFM data, with a `.efm.meta` frame index (if enabled)
- **`outfile.log`**: Detailed log file
- **`outfile.tbc.ldf`**: TBC'd RF data (if `--RF_TBC` is used)

## Exit Behavior

The decoder will stop processing when:
1. The requested number of frames (`--length`) has been decoded
2. Lead-out is detected (unless `--ignoreleadout` is specified)
3. End of file is reached
4. An error occurs
5. User interrupts with Ctrl+C (SIGINT)

## Error Conditions

### PAL/NTSC Conflict
```
ERROR: Can only be PAL or NTSC
```
Occurs when multiple video standards are specified.

### AC3 with PAL
```
ERROR: AC3 audio decoding is only supported for NTSC
```
AC3 audio is only available on NTSC laserdiscs.

### Write Test LDF Collision
```
ERROR: --write-test-ldf output file cannot be the same as input file
```
The test output file must be different from the input file.

### Seek Failure
```
ERROR: Seeking failed
```
Unable to seek to the requested frame; may indicate corrupted data or invalid frame number.

## Notes

- **Default Video Standard**: If neither `--PAL` nor `--NTSC` is specified, NTSC is assumed.
- **Thread Count**: The default of 4 threads is suitable for most systems; adjust based on your CPU core count.
- **Sample Rate**: The decoder internally works at 40MHz; all other sample rates are resampled.
- **Frame Counting**: Frames are counted as complete video frames; internally, the decoder processes fields (2 fields = 1 frame).
- **Lead-out**: The lead-out section marks the end of valid disc content; decoding typically stops here unless `--ignoreleadout` is used.

## Version Information

To check the installed version:
```bash
ld-decode --version
```

or

```bash
ld-decode -v
```
