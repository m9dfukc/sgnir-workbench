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
- Chord tables from lylepmills: later, optional.
- **Schliere on hardware** [open]: worst-case load (tap between detents, loud input, high
  feedback/resonance; measured so far only with knobs at noon: 81 % / 89 %); input/output
  levels → adjust `kOutKnee`/`kLoopKnee` (ANLEITUNG 4.4); excite level (`kExciteLevel`,
  [hypothesis] 0.5) and flavor by ear; CV smoothing for tap/feedback
  (`kSchliereCvLpCoefficient`, now 0.1 like Rings); LED PWM whine check (AC/DC lesson D66).
- Flash `rc5` (= sgnir `a922267`) for a listening check; push sgnir + workbench only after the
  user's explicit approval.
- Longer chain at 48 kHz (user idea, 2026-11-21, after the AC/DC variant SchliereL with 160
  stages): ≈ 0.56 % per extra stage [hypothesis, from diag7] → 91 stages ≈ 76 %, 98 ≈ 80 %;
  without the 96 kHz path. Open design questions: stage count and tap layout, replace S or
  add L, UI slot; benchmark build with two lengths first.
- Grounded-model load: diag7 read 71 / 76 / 84 % for the three models (page/polyphony
  unknown) against 43 % in diag6 ("user's setting", page unknown). Stock code, object-identical
  in the release build; a like-for-like reading (known model + polyphony) is [open].
- Schliere variants O/A on the right button (switches exist in `Schliere::Init`), after the
  excite flavor is decided.

### On the next flash
- Confirm that a firmware update preserves settings/calibration. [code]: yes (settings in
  flash sector 1, bootloader erases from sector 2). Before/after check: model, polyphony,
  V/OCT 1 V/3 V octave test. Then upgrade the marker in CLAUDE.md and sgnir
  `docs/FLASHING.md` ("not yet confirmed on hardware").

### Port details ([open] items from ANLEITUNG.md)
- Rings input/output voltage ranges; DC coupling of the input.
- `pitch_offset` zero point for the continuous 10-octave frequency knob.
- ~~Settings `State` padding: verify it is zeroed in stock-firmware saves.~~ Moot: the sgnir
  extension fields sit behind an `'SGN1'` marker and are initialized when it is missing
  [code: `rings/settings.cc`]. Likely 0xFF rather than zero anyway [hypothesis].

## Done
- 2026-11-21: **Schliere CPU steps 1–4** (sgnir `a922267`): no
  automatic FMA, chain as straight-line code, tap loop over active taps only, per-block excite
  check, inlined chain input/halfband, crossfade reciprocal; stale `diagnostics.h` comments
  fixed. Benchmark `diag7` [measured, flash log 2026-11-21]: Schliere 61 → 59 % (48 kHz) and
  72 → 69 % (96 kHz) for plain → one-vfma chain (v0.1.0: 81 / 89 %), base 13 %; the plain
  chain is fetch-bound, so the vfma chain became the only one (user). Cycle model 71.9 →
  48.1 %; output within 1.43e-6 of v0.1.0 on ARM; host tests 43/43, 32/32; release
  build: 66 of 67 objects identical to `409ab6a` (only `schliere.o`), +3 424 B flash, +4 B RAM.
- 2026-11-21: **Project rules restructured:** `hardware-build` skill with a build script
  (`.claude/skills/hardware-build/`; run on `ec4a7aa` it reproduces rc4's `rings.bin` and WAV
  byte for byte [measured, SHA-256]), path-scoped `.claude/rules/firmware.md`, CLAUDE.md
  status trimmed to the current state, flash-capable commands on `ask` in the project settings.
- 2026-11-20: **Release `v0.1.0`** of eurorack-mi-sgnir (tag on `ec4a7aa`,
  https://github.com/m9dfukc/eurorack-mi-sgnir/releases/tag/v0.1.0). Fixes on the way:
  `bbff5c9` (GCC `-Wcomment` in the host test), `e832c6b` (touch the checked-in resources: a
  fresh clone made `make` regenerate them with the Python 2 compiler), `ec4a7aa` (ELF/map as a
  workflow artifact). The CI (Linux) binary differs from the local macOS build only in the
  placement of libm constant tables; all 190 functions have identical code [measured:
  normalized disassembly of both ELFs]. `rc4` on the module is this code.
- 2026-11-20: **Audio-bootloader flashing proven** [measured]: official 1.1
  (`backup/rings_1.1.wav`; second attempt, the first was too quiet), then several sgnir
  builds. Log: `firmware/flashlog/2026-11-20-stock-rings-1.1/` (local).
- 2026-11-20: **Schliere on the module, CPU** [measured, LED benchmark builds diag1–6, logs in
  `firmware/flashlog/` (local)]: first build 105 % peak → overload (crackle, starved UI and
  SysTick: long press took 10 s). Fixes: flash prefetch at boot (−10 %), store only tapped
  nodes, per-block a1/a2 ramp, folded stage update, skip settled taps, loop-invariant motion,
  CV interpolation instead of a third halfband; overload guard leaves the bank after 0.2 s over
  budget. Now 81 % (48 kHz) / 89 % (96 kHz), grounded models 43 %, base 13 %. RAM execution
  18 % slower (S-bus contention), dropped. 48 kHz default (user). Only-ODD = hardware mix of
  ODD+EVEN [sourced: manual] → comb notches; Spread CCW = mono. Committed as sgnir `2fa5fe3`,
  `41db973`; release pipeline `7738d7d` (GitHub Actions on v* tags, WAV + bin + flashing guide),
  first release `v0.1.0`.
- 2026-11-20: **Schliere S** in eurorack-mi-sgnir (`1904959`, `81fe986`):
  `rings/dsp/schliere.{h,cc}` — one class with the variant switches S/O/Z/W/A, `SoftClip`
  knees, CVs per block (ramped), no dead zone, plus spread (POSITION, hold zones at mono /
  classic / chain end), excite (STRUM jack only; impulse or noise burst), dry/wet steps.
  Host tests `rings/test/schliere_test.cc` 43/43; equivalence vs. the OWL reference
  `tests/schliere/` 32/32 (linear range ≤ 2.4e-6) [measured, host]. Integration:
  `CvScaler::ReadSchliere`, third audio branch in `FillBuffer`, third bank on the right long
  press, LEDs, settings in the former reserved bytes (zero defaults, no marker bump), DWT peak
  cycle counter. Build 97 056 B. Not flashed.
- 2026-11-20: **Frequency knob alt** in eurorack-mi-sgnir (`68906da`): while locked, the
  FREQUENCY knob snaps to whole octaves of the locked pitch,
  `offset = 12 * round((pot_transpose - locked_transpose) / 12)` with ±0.1 octave hysteresis —
  no jump at lock time, no pickup; result stays within ±6 semitones of the knob's own range.
  Host check of the formula: switch points ±6 st ± 1.2 st hysteresis, no flapping under ±0.9 st
  jitter [code]. Build 85 104 B. Not flashed.
- 2026-11-20: **Frequency locking** in eurorack-mi-sgnir (`07283e3`), behavior from lylepmills
  `0390557b34`, reimplemented on upstream (his diff is entangled with options menu/Mini-Elements).
  Toggle: both buttons >0.2 s, LEDs red = will lock / green = will unlock, toggles on release.
  **Calibration now needs both buttons held 6 s** (stock: any press of both; user decision);
  short press of both = no-op. Settings extension in the former `State` padding behind an
  `'SGN1'` marker, `SettingsData` stays 60 bytes (static_assert) so stock calibration survives
  [code]. Plus `ebb93b9`: CV offsets captured during calibration are applied only when C3
  succeeds (stock persisted them on the next button press after a failed calibration).
  User docs in the sgnir README (`60df4f6`). Build 84 880 B, Arm GNU 12.2. Not flashed.
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
