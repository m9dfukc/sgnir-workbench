# TODO

## Open

### Design (need the user)
- **Order** (user): only the 2x5 1.27 mm Cortex-Debug adapter set (board + 1.27 mm IDC ribbon,
  ~5–10 €, amazon.de "JTAG SWD Adapter 1.27mm 10 Pin"). No ST-Link — an RP2040 board with the
  raspberrypi/debugprobe firmware is the SWD probe (decision 2026-11-20).
- Flash the debugprobe UF2 onto an RP2040 board; verify the SWD/UART pin mapping against the
  debugprobe README [hypothesis: GP2 SWCLK, GP3 SWDIO, GP4/GP5 UART] and test OpenOCD
  (`interface/cmsis-dap.cfg`) against some expendable target before touching Rings.
- Prove the JP2 serial-bootloader path (2.54 mm 1x6 header, jumper wires + 3.3 V UART,
  RESET/SYSBOOT buttons): read-only flash dump as the test.

### Firmware work (in `firmware/eurorack-mi-sgnir`)
- Port **frequency locking** from the lylepmills master diff (~150 lines / 7 files, drop the
  MiniElements() guard), then **frequency knob alt** (~10 lines). Chord tables later, optional.
- Implement **Schliere** (variant S first, one class with variant switches; ANLEITUNG.md 4–5):
  replace `tanhf` with stmlib `SoftClip`, port host tests, profile with DWT cycle counter
  (budget 84 000 cycles/block, target < 80 %). Control mapping decided 2026-11-20 (see
  CLAUDE.md / `docs/smear-mapping-proposal.md`): POSITION = Spread, STRUM = Excite (implement
  both impulse and noise-burst flavors, pick by ear on hardware), left button = dry/wet toggle.

### Before the first flash
- Clarify [open]: does a firmware update preserve settings/calibration?
- Prove the audio bootloader procedure once with the stock firmware
  (official `backup/rings_1.1.wav` is the known-good rollback).

### Port details ([open] items from ANLEITUNG.md)
- Rings input/output voltage ranges; DC coupling of the input.
- `pitch_offset` zero point for the continuous 10-octave frequency knob.
- Settings `State` padding: verify it is zeroed in stock-firmware saves.

## Done
- 2026-11-20: **WAV encoder ported to Python 3** (`tools/`, uv project `sgnir-tools`:
  `qpsk-encode` + verification decoder `qpsk-decode`) and **verified against the official
  Rings 1.1 WAV [measured]**: payload extracted with 0 CRC failures, re-encode structurally
  identical (2015 encoder used 2.5 s sector pause vs. today's 3.5 s), samples within ±1 LSB
  (numpy/libm rounding). Official WAV + extracted bin archived in `backup/`
  (`rings_1.1.wav` sha256 5a013ee5…, `rings_1.1.bin` sha256 bfdff834…). Details: `tools/README.md`.
- 2026-11-20: **Smear→Rings mapping decided** (user, via `docs/smear-mapping-proposal.md`):
  POSITION = Spread, STRUM = Excite impulse (flavor by ear on hardware, both implemented),
  left button = dry/wet toggle. Workbench published as `m9dfukc/sgnir-workbench` (public, MIT).
- 2026-11-20: Workbench set up (CLAUDE.md rewrite, .gitignore, lexicon, README, project
  settings). Reference package copied from `_tmp/` to `reference/schliere-owl/` and verified:
  all 65 host tests pass.
- 2026-11-20: Decisions (user): two-level repo structure, snapshot fork from upstream master,
  vendored stmlib/bootloader, public + MIT, English artifacts, Schliere scope S-first
  (M dropped), chord tables later, UI = third bank on long-press, WAV encoder → py3,
  48 kHz target (32 kHz only after measurement + sign-off), buy ST-Link V2 clone + adapter.
- 2026-11-20: Research: Rings forks (`docs/research/notes/rings-forks-2026-11-20.md`,
  recommendation upstream + lylepmills patches) and ST-Link options
  (`docs/research/notes/stlink-options-2026-11-20.md`).
- 2026-11-20: **eurorack-mi-sgnir created** (https://github.com/m9dfukc/eurorack-mi-sgnir,
  public, MIT): pristine snapshot `d030009` + meta `1d6bea5`, submodule at
  `firmware/eurorack-mi-sgnir`. Stock build reproduced with Arm GNU 12.2
  (`firmware/builds/rings-stock-f4cfa769/`, rings.bin 84 016 B). Not flashed.
