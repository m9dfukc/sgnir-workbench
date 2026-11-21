# sgnir-tools

Host tools for the sgnir workbench, Python 3 via [uv](https://docs.astral.sh/uv/).

```bash
cd tools
uv run qpsk-encode --help
uv run qpsk-decode --help
```

(In the Claude sandbox: `UV_CACHE_DIR=.uv-cache uv run …`.)

## qpsk-encode

Python 3 port of Mutable Instruments' `stm_audio_bootloader/qpsk/encoder.py`
(Python 2, MIT), source commit `1ec7d637` — converts a firmware `.bin` into the
`.wav` played into the module's audio input for the QPSK audio bootloader. The
port is line-for-line faithful; only py2→py3 semantics were adapted (integer
division, bytes, argparse).

Rings invocation (same parameters as the `wav` target in `rings/makefile`):

```bash
uv run qpsk-encode -t stm32f4 -s 48000 -b 12000 -c 6000 -p 256 path/to/rings.bin
```

## qpsk-decode

Verification counterpart (new, not in upstream): demodulates a bootloader
`.wav` back to the firmware payload, CRC-checking every packet. Assumes a
synthetically generated file (exact sample alignment); it is not a
general-purpose demodulator for recorded audio.

```bash
uv run qpsk-decode backup/rings_1.1.wav   # writes rings_1.1.bin next to it
```

## Verification against the official Rings 1.1 WAV (2026-11-20) [measured]

Official `rings_1.1.wav` (SHA-256 `5a013ee5…a16f27`, archived in `backup/`),
decoded with `qpsk-decode`: 384 packets, 0 CRC failures → 98 304-byte payload
(84 016 bytes firmware + `0xff` padding; archived as `backup/rings_1.1.bin`).
Re-encoding that payload with `qpsk-encode` and comparing against the official
file:

- Structure and timing identical, given a 2.5 s sector-erase pause: the 2015
  encoder used 2.5 s where today's upstream (and this port) uses 3.5 s per
  STM32F4 sector base address. With 2.5 s both files have identical length and
  identical silence/packet layout.
- Sample data: max deviation ±1 LSB (16-bit), on ~8 % of samples, at waveform
  peaks — float sin/cos rounding differences between 2015-era numpy/libm and
  today's, flipped by the writer's truncating quantizer. Functionally
  irrelevant for QPSK demodulation; bit-exact reproduction of the 2015 file is
  not achievable.
- Round-trip of this port's own output (encode → decode → compare) is exact,
  verified for both the recovered official payload and our stock build
  (`firmware/builds/rings-stock-f4cfa769/rings.bin`).

The 3.5 s default is kept (more erase margin, matches pinned upstream).

## cycles/cycle_model.py

Cycle model for the Schliere DSP (new, 2026-11-21). Builds
`rings/dsp/schliere.cc` with the firmware's compiler flags into a small
bare-metal ELF (`cycles/harness.cc`), runs `Schliere::Process` in the Unicorn
emulator and adds up the Cortex-M4 TRM cycle counts of every executed
instruction. Use it to compare compiler flags and code variants before a build
goes onto the module, and to check that a variant's output is bit-identical on
ARM.

Standalone script with inline dependencies (unicorn, capstone), not part of the
`sgnir-tools` package:

```bash
export TOOLCHAIN_PATH=/Applications/ArmGNUToolchain/12.2.rel1/arm-none-eabi/
cd tools/cycles
uv run cycle_model.py build --source ../../firmware/eurorack-mi-sgnir --out "$TMPDIR/cyc/base"
uv run cycle_model.py build --source ../../firmware/eurorack-mi-sgnir --out "$TMPDIR/cyc/nc" -- -ffp-contract=off
uv run cycle_model.py run "$TMPDIR/cyc/base/schliere_harness.elf"            # by function
uv run cycle_model.py run "$TMPDIR/cyc/base/schliere_harness.elf" --lines    # by source line
uv run cycle_model.py table --ref "$TMPDIR/cyc/base/schliere_harness.elf" \
    "$TMPDIR/cyc/base/schliere_harness.elf" "$TMPDIR/cyc/nc/schliere_harness.elf"
```

- Unicorn needs JIT memory, which the Claude sandbox blocks (the process dies
  with SIGILL): run the `run`/`table` commands outside the sandbox. `build`
  works inside it.
- The harness reads `rings::SchliereParameters` by layout (`write_parameters`
  in the script): update it when the struct changes.
- Scenarios: `noon` (all knobs at noon, the setting of the module readings),
  `tap-between`, `worst` (loud input, feedback into the limiter, moving tap /
  spread / frequency, noise bursts). `worst` is a guess at the worst case, not
  a proven bound.

Calibration against the module (sgnir `ec4a7aa`, flash log 2026-11-20): the
model gives 71.9 % of the block budget for `Schliere::Process` at 48 kHz and
80.6 % with the 96 kHz feedback path, knobs at noon; the module readings minus
the 13 % base load are 68 % and 76 % [measured]. Module ≈ 0.94 × model in both
points. The model is nominal (no flash wait states, 2 cycles pipeline refill per
taken branch): code that is denser than 16 bytes per 6 cycles may run slower
from flash than modelled [hypothesis]. A model number does not replace a load
reading on the module.

Second calibration point (diag7, flash log 2026-11-21): after the chain was unrolled into
straight-line code, the module ran the cycle-optimal stage (no FMA) *slower* than a stage with
one `vfms` that the model rates 2.3 points worse (module 61 vs. 59 % at 48 kHz) [measured].
The unrolled chain is fetch-bound; the model does not see that. Module/model ratio for
`Process`: plain chain 1.05–1.18, one-vfma chain 0.95–1.06.
