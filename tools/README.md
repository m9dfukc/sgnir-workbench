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
