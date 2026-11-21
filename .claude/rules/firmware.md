---
paths:
  - "firmware/eurorack-mi-sgnir/**"
  - "tests/schliere/**"
  - "tools/src/**"
---

# Firmware work (eurorack-mi-sgnir)

`firmware/eurorack-mi-sgnir/` is a separate public repo (submodule, MIT). Its README is the
user-facing reference for controls, LEDs and the build; this file only holds what is easy to
get wrong. Images for the module are made with the `hardware-build` skill.

## Build and test

- Build command, host tests and expected output: sgnir README ("Building", "Host tests") and
  `tests/schliere/README.md`. Both suites must end with `all cases ok` before a hardware
  build and before a commit.
- CI builds on Linux with GCC and `-Wall -Werror` for the host tests; macOS `c++` (clang) is
  more lenient, which already broke a release run once (`-Wcomment`). Check host-compiled
  files the strict way before committing:
  `arm-none-eabi-g++ -std=c++11 -O2 -Wall -Werror -DTEST -I. -c -o /dev/null <file>`
  (exit status 0 counts; the "parameter passing ... changed in GCC 7.1" notes are harmless).
- A fresh clone may make `make` regenerate `rings/resources.cc` with the Python 2 resource
  compiler (`xrange` error): `touch rings/resources.cc rings/resources.h`. The checked-in
  resources are correct.
- zsh does not word-split unquoted variables: run multi-flag compiler lines through
  `bash -c` or a script, not via `$FLAGS`.

## CPU budget

- `FillBuffer()` has **84 000 cycles per block** (24 samples at 48 kHz, 168 MHz)
  [code: `kBlockBudgetCycles`]. Peaks with knobs at noon, no input: Schliere S 62 % (48 kHz)
  / 71 % (96 kHz feedback path), L (98 stages) 82 %, base load 13 % [measured, diag9, flash
  log 2026-11-21]; diag7 read 59 / 69 % for host-identical S code (v0.1.0: 81 / 89 %);
  grounded models 71–84 % depending on the model [measured, diag7]. ≈ 0.5 points per chain
  stage. With the excite flavors (diag10): idle S 61 / 71 %, L 82 %; a running ping adds
  1–3 points (L 85 %) — 4–6 times the static estimate, probably atanf() + cosf() in the
  trigger block [measured, diag10, flash log 2026-11-21-sgnir-c9aced5]. Worst-case settings
  with a signal are [open].
- The audio DMA interrupt outranks SysTick and the main loop. Over budget, the UI starves:
  LEDs dim, a 3 s long press takes about 10 s, buttons stop working [measured]. The overload
  guard (leaves the Schliere bank after 0.2 s over budget) is the safety net; do not weaken it.
- Static cycle estimates were off by a factor of two (55 % estimated, 105 % measured). Any
  change to Schliere's per-sample code needs a load reading on the module (benchmark switches
  in `rings/diagnostics.h`) before it is called done.
- Already tried: flash prefetch at boot saves about 10 % (keep); running the DSP from RAM is
  18 % slower on this MCU (do not retry) [measured].
- Keep `Schliere::state_` (the stage states) at the front of the class, within the ±1020-byte
  offset range of `vldr`/`vstr` from `this`. diag8 had it behind the scalars: GCC precomputed
  one address per stage, kept them on the stack and reloaded them in every stage (S 64 instead
  of 59 %, L91 88 instead of ≈ 77 %) [measured + disassembly]. Check new members with the
  disassembly: integer `ldr`/`add` between the `vfms` of the chain are the symptom.
- The unrolled chain is fetch-bound: code size per stage counts, not only cycles. The stage
  with one explicit `vfms` is 2.3 points slower in the cycle model but 2–3 points faster on
  the module [measured, diag7]. Check denser variants on the module, not in the model alone.
- Cycle model (`tools/cycles/`): good for relative comparisons; module/model ratio for
  `Process` was 0.94 on v0.1.0 and ≈ 0.95–1.06 for the vfma chain [measured].

## Settings layout

- `SettingsData` must stay **60 bytes** (`static_assert` in `rings/settings.h`): the storage
  layer rejects other sizes, which would discard the V/OCT calibration. No calibration backup
  exists yet.
- Stock fields and `CalibrationData` keep their offsets. New fields go into the sgnir block
  behind the `'SGN1'` marker (`kStateExtensionId`) with a default in `InitStateExtension()`,
  so settings saved by the stock firmware load cleanly and stock still reads ours after a
  rollback.

## Diagnostics

- `SCHLIERE_CPU_METER` / `SCHLIERE_BENCHMARK` in `rings/diagnostics.h` are off in every
  commit and release.
- The user reads LED blink codes by eye. Keep displays slow: 0.5 s per blink was too fast;
  the last benchmark display used 0.8 s per blink with 1–2 s pauses between the digits
  (`BUILD_INFO.txt` of `sgnir-81fe986-diag6`).

## What has to move together

- Controls, LEDs or saved settings changed → update the sgnir README tables in the same
  commit. `docs/FLASHING.md` is pasted into every release's notes.
- The QPSK encoder exists twice: workbench `tools/src/sgnir_tools/` (canonical, verified
  against the official 1.1 WAV) and sgnir `tools/*.py` (script copy for CI). Change one →
  change the other and confirm both produce byte-identical WAVs.
- After a commit in the submodule, the workbench needs its own commit for the new submodule
  pointer plus `TODO.md`/`CLAUDE.md`.

## Releases

Pushing a `v*` tag publishes a public release through GitHub Actions; do it only on the
user's explicit request. A tag whose run fails creates no release: fix the cause, then delete
and re-push the tag. "Run workflow" (manual) builds the artifacts, including ELF and map,
without releasing.
