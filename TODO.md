# TODO

## Open

### Decisions (need the user)
- **Fork base**: upstream `pichenettes/eurorack` + cherry-picked lylepmills patches, or a
  lylepmills branch — decide once `docs/research/notes/rings-forks-2026-11-20.md` is complete.
- **UI approach**: third model bank on long-press (ANLEITUNG 5.4) vs. lylepmills-style options
  menu. Depends on fork research.
- **Smear→Rings mapping**: dedicated design round; Claude drafts a mapping proposal comparing
  the Smear feature set with Rings' controls (ANLEITUNG 5.3 is the starting point).
- **ST-Link purchase**: pick a model once `docs/research/notes/stlink-options-2026-11-20.md`
  is in.

### Setup
- `gh` auth is broken (invalid token in keyring). User runs: `gh auth refresh -h github.com`.
- Create GitHub repo `m9dfukc/eurorack-mi-sgnir` (after the fork-base decision): snapshot of
  `rings/`, `stmlib/`, `stm_audio_bootloader/` + build files at a pinned upstream commit,
  MIT license, credits (Mutable Instruments, lylepmills, acdc-workbench). Then add it here as
  submodule `firmware/`.
- Reproduce the stock Rings firmware build with Arm GNU 12.2; keep the artifact; commit each
  toolchain fix separately.

### Before the first flash
- Download and archive the official Rings firmware WAV (`backup/`).
- Clarify [open]: does a firmware update preserve settings/calibration? How is the update WAV
  produced from a build (`rings/makefile`, stm_audio_bootloader)?
- Prove the audio bootloader procedure once with the stock firmware.

### Port (from ANLEITUNG.md, [open] items)
- Rings input/output voltage ranges; DC coupling of the input.
- `pitch_offset` zero point for the continuous 10-octave frequency knob.
- Settings `State` padding: verify it is zeroed in stock-firmware saves.
- Replace `tanhf` with `stmlib` `SoftClip` and widen host-test tolerances accordingly.
- CPU budget: profile `FillBuffer()` via DWT cycle counter; target < 80 % of 84 000 cycles/block.

## Done
- 2026-11-20: Workbench set up (CLAUDE.md rewrite, .gitignore, lexicon, README). Reference
  package copied from `_tmp/` to `reference/schliere-owl/` and verified: all 65 host tests pass.
- 2026-11-20: morluto/rea reviewed — nothing to adopt
  (`docs/research/notes/rea-review-2026-11-20.md`).
- 2026-11-20: Decisions taken (user): two-level repo structure, snapshot fork, vendored
  stmlib/bootloader, public + MIT, English artifacts, port scope S-first (M dropped).
