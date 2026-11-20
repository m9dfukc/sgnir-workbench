# TODO

## Open

### Design (need the user)
- **Smear→Rings control mapping**: dedicated design round; Claude drafts a mapping proposal
  comparing the Smear feature set with Rings' controls (`ANLEITUNG.md` 5.3 is the starting
  point). Then: variant switches (A/O later), output routing, STRUM function.
- **ST-Link order** (user): ST-Link V2 clone + 2x5 1.27 mm Cortex-Debug adapter set
  (+ optional 3.3 V USB-UART as serial-bootloader fallback). Links and wiring:
  `docs/research/notes/stlink-options-2026-11-20.md`.

### Firmware work (in `firmware/eurorack-mi-sgnir`)
- Port the firmware-WAV encoder (`stm_audio_bootloader/qpsk/encoder.py`, Python 2) to
  Python 3 as a uv tool in this workbench; verify against an official Rings release WAV.
- Port **frequency locking** from the lylepmills master diff (~150 lines / 7 files, drop the
  MiniElements() guard), then **frequency knob alt** (~10 lines). Chord tables later, optional.
- Implement **Schliere** (variant S first, one class with variant switches; ANLEITUNG.md 4–5):
  replace `tanhf` with stmlib `SoftClip`, port host tests, profile with DWT cycle counter
  (budget 84 000 cycles/block, target < 80 %).

### Before the first flash
- Download and archive the official Rings firmware WAV (`backup/`).
- Clarify [open]: does a firmware update preserve settings/calibration? How does
  `rings/makefile` produce the update WAV?
- Prove the audio bootloader procedure once with the stock firmware.

### Port details ([open] items from ANLEITUNG.md)
- Rings input/output voltage ranges; DC coupling of the input.
- `pitch_offset` zero point for the continuous 10-octave frequency knob.
- Settings `State` padding: verify it is zeroed in stock-firmware saves.

## Done
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
