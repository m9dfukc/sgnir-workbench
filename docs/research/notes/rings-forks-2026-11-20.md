# Rings firmware forks — survey and base recommendation (2026-11-20)

Research question: which codebase should a slim custom Rings firmware repo start from, given the goals
(1) new "Schliere" alternative mode, (2) port only *frequency locking*, *frequency knob alt functionality*
and optionally the new chord tables from lylepmills, (3) slim repo with only `rings/`, `stmlib/`,
`stm_audio_bootloader/` and build files, (4) buildable with Arm GNU 12.2.

Method: GitHub REST API (branches, commits, trees, forks), source tarballs from codeload.github.com,
full `diff -r` of the `rings/` trees, and a **real test build** of upstream Rings with the locally
installed Arm GNU 12.2.rel1 toolchain (see "Toolchain reality check").

---

## 1. Upstream: pichenettes/eurorack

- Repo: <https://github.com/pichenettes/eurorack> (default branch `master`, 3369 stars, last push 2026-10-12).
- Recent activity is *not* Rings: `f4cfa76970` 2026-10-12 "Update README.md", `d8f4739152` 2026-10-11 "Beads code/sch".
- **`rings/` is frozen.** Full history of `rings/` on master (newest first):
  - `c5f910fb25264b7b45ec6249bfaa5050a9cb513d` 2022-01-22 "Fix various typos" (comments only)
  - `43b9c9f2976e62a862ace569fb48c39dc4c5b803` 2020-04-29 "Initialize previous_position_ prior to use" (last functional fix)
  - `35bf03aa35` 2018-10-05 "Rings: colorblind mode"
  - `2a468734bd` / `61d7146d93` / `3c23d0306d` 2019 renames/BOM, `1cf95b2f26` 2019, `3f8a69d254` 2016, `46acd575f1` 2016-02-27 first commit
- **Submodules, not plain folders** (`.gitmodules`): `stmlib` → `pichenettes/stmlib.git`,
  `stm_audio_bootloader` → `pichenettes/stm-audio-bootloader.git` (note the hyphens in the repo name),
  plus AVR libs irrelevant here. Pins on current master (git tree API):
  - `stmlib @ e3bd7c9cc00e4364166f9905c0509b6ffd0535ec` (2023-05-30 "Make tables const too"; stmlib master head is `d18def816c`, 2023-09-03)
  - `stm_audio_bootloader @ 1ec7d6374e2d305cb710170eb80de9b66e795792` (repo head `b376bd6448`, 2022-01-05; repo dormant since)
- **License:** STM32 projects are MIT (repo README and per-file headers in `rings/*.cc`); hardware cc-by-sa-3.0.
  A slim MIT-licensed extraction repo is unproblematic as long as headers stay intact.
- **Build system:** `rings/makefile` (FAMILY `f4xx`, F_CPU 168 MHz, `APPLICATION_LARGE = TRUE` → app at
  `0x08008000`, bootloader `rings_bootloader`) includes `stmlib/makefile.inc`. Default
  `TOOLCHAIN_PATH ?= /usr/local/arm-4.8.3/` — the original toolchain is gcc-arm-none-eabi 4.8 (2013q4).
  Flags: `-g -O2 -Wall -Werror … -fshort-enums`, **no `-std=` for C++** (compiler default applies),
  `CPPFLAGS = -fno-exceptions -fno-rtti` (plain `=`, so overridable from the make command line).
- **Firmware-update WAV:** `make -f rings/makefile wav` runs
  `python stm_audio_bootloader/qpsk/encoder.py -t stm32f4 -s 48000 -b 12000 -c 6000 -p 256 build/rings/rings.bin`.
  `qpsk/encoder.py` is **Python 2** (`#!/usr/bin/python2.5`, print statements; fails to parse as Python 3), needs numpy.
- Resources (`rings/resources.cc`) are checked in; the Python resource generator is not needed for a normal build.

## 2. lylepmills/eurorack

- Repo: <https://github.com/lylepmills/eurorack>, fork of pichenettes/eurorack, 134 stars, last push 2026-11-16.
- Alt-firmware docs: `alt_firmwares/README.md` on master; community thread
  <https://llllllll.co/t/mutable-instruments-alt-firmwares/63936>.
- **Branch landscape** (GitHub API, 2026-11-20): ~75 branches. All recent activity (2026, branches named
  `claude/*`, `codex/*`, `session/*`, merged into master via PRs) is **Plaits-only** (a "Plaits lab"/palette
  project, incl. `Dockerfile.plaits-builder` and `AGENTS.md`). Rings-relevant branches:

| Branch | Head | Date | Content | Relevant? |
|---|---|---|---|---|
| `master` | `0390557b34` | 2026-11-16 | Canonical released Rings alt firmware (merged Jan–Mar 2022) + all the 2026 Plaits work | **Yes — the reference for the Rings features** |
| `lyle--rings_revamp` | `5ff8e1cfa0` | 2022-01-16 | Dev branch of the alt firmware; contains reverts of the older 2020 "even/odd crossfading" locking. Merged into master | Superseded by master |
| `lyle--rings_flashing_yellow` | `e2ac597222` | 2022-07-16 | Unmerged experiment: "blinking yellow" 6th mode, extra root note on AUX, reverb buffer −1 KB, strummer → CCM | No |
| `lyle--rings_midi` | `edb5796477` | 2022-08-27 | Unfinished MIDI-in via debug port ("more tweaks, still dropping notes") | No |
| `lyle--update_wav_file` | `4acb36cb09` | 2023-09-01 | upstream merge `1ca7d5062e` + "space optimizations" `e7a1ad4016` + new release WAV. **`rings/` tree is byte-identical to master's `rings/`** (verified with `diff -rq`) | Confirms master is canonical |

- **Rings commits on master** (API, `?path=rings`):
  `4d6da31c3fa07f463cc5f06bb36c731ff65c80bc` 2020-12-06 "initial frequency locking" ·
  `3562b9698170bb33da3cb23daac1096d88a1a2fd` "improve save state logic, get octave shift working" ·
  `3dd114b933131e125ce18030e6e06bd9b9ecc76f` "[rings] frequency locking with even/odd crossfading" (later reverted) ·
  `8a1bcab5f7fb775695d72a9156ab99394c900455` 2022-01-18 "rings options menu" ·
  `e616fc634a` "elements exciter stuff" · `99bb7b36d8`/`32ba76e9a2` strum hold ·
  `b2dcbdd58d813a16c5a0670428c9d01347ecd731` calibration moved to startup ·
  `9d0bb12e93c73fc80bba845b6a0056af6c81015d` "fix access to calibration" ·
  `72ac1cd5a0e30807de1c4b8af3999540f959c6e2` 2022-03-06 "fm cv input option" ·
  `a40bcd9539f8b5631a5a99c9bed335ed8dc55ad7` 2022-03-08 "save state after long pressing model button" (**last Rings change**).
  History contains reverts, so cherry-picking literal commits is messy; porting from the final diff is cleaner.
- **Submodules unchanged:** pins `stmlib @ 0a641fb2fd1572b9d8c849a50b7d9bb70aeba3aa` (2022-01-05) and
  `stm_audio_bootloader @ b376bd644871edc06f9ee75fb95be516a10ab394` — plain pichenettes repos, no stmlib fork.
- **Global changes you would inherit with lyle's rings as a base** (diff vs upstream):
  - **Sample rate 48 kHz → 32 kHz** (`rings/dsp/dsp.h`, `kSampleRate`) to afford Mini-Elements CPU cost.
  - Mini-Elements: new files `dsp/exciter.{h,cc}`, `dsp/tube.{h,cc}`, `dsp/oscillator.h`, `dsp/fx/diffuser.h`,
    `samples/`, `resources/samples.py`, big additions to `part.cc` (+534 diff lines) and `string_synth_part.cc` (+241).
  - Easter-egg handshake removed (menu-only), calibration moved to power-on, **internal CV slew removed**
    (documented in the README "Technical notes"), settings struct grown to 14 bytes with a
    `kSettingsId = 259583919` version marker.
  - Diff sizes vs upstream: `ui.cc` 305 lines, `cv_scaler.cc` 128, `settings.{h,cc}` 107, `part.cc` 534.

### 2.1 Exact locations of the wanted features (branch `master`, all paths under `rings/`)

**Frequency locking** (self-contained, cleanly extractable):
- `settings.h`: `State` fields `locked_transpose` (float), `frequency_locked` (uint8_t, line ~50/55),
  `ToggleFrequencyLocking()` (line 82); `kSettingsId` versioning.
- `settings.cc`: defaults (lines ~69, 74).
- `cv_scaler.cc` `Read()` lines ~213–262: first-read sanity check of stored state, `transpose = locked_transpose`
  when locked, capture of `locked_transpose` at lock time, `settings->Save()` when dirty.
- `cv_scaler.h`: members `frequency_locked_`, `first_read_`, helper `frequency_pot_value()` (line ~220).
- `ui.h`: new mode `UI_MODE_DISPLAY_FREQUENCY_LOCKING` (line 48); `ui.cc`: medium-press (200 ms) of both
  buttons enters the mode (line ~110), LED feedback (lines ~203–213), toggle on release (lines ~381–408).
- `dsp/performance_state.h`: `frequency_locked`, `locked_frequency_pot_value` (lines 66–68).
- Only coupling to unwanted features: a `performance_state->MiniElements()` guard — trivially dropped.
- Estimated extraction onto upstream: **well under 150 lines, 7 files. Clean.**

**Frequency knob alt functionality**:
- Octave switch (all non-Mini-Elements modes): `cv_scaler.cc` lines 231–238:
  `octave_transpose = 12.0f * (floor(pot * 6.999f) - 3.0f)` added to the locked transpose → ±3 octaves in 7 steps.
  **~10 lines inside the locking branch. Trivial.**
- Exciter-timbre variant (Mini-Elements only): `dsp/part.cc` lines ~832–833
  (`update_timbre = performance_state.frequency_locked || …; timbre = locked_frequency_pot_value`).
  Not needed as-is, but it is the exact pattern for letting the locked frequency knob drive a **Schliere**
  parameter instead: `locked_frequency_pot_value` is already plumbed into `PerformanceState`.

**Alternate chord tables** (bigger but mechanical, cleanly extractable):
- `dsp/performance_state.h`: `enum ChordTable { CHORD_TABLE_BRYAN, CHORD_TABLE_JON, CHORD_TABLE_JOE }`,
  constants `kNumBryanChords=11`, `kNumJonChords=17`, `kNumJoeEasterEggChords=18`, `kMaxNumJoeChords=23`.
- `dsp/part.cc` lines 142/199/292: `bryanChords` / `jonChords` / `joeChords` `[kMaxPolyphony][N][8]`
  (quantized sympathetic-strings / "Western Chords" resonator); table selection in
  `ComputeSympatheticStringsNotes` around line 406.
- `dsp/string_synth_part.cc` lines 109/166/262: the three table sets for the easter egg (Disastrous Peace);
  selection around line 471.
- `cv_scaler.cc` line 151 (`chord_table` from settings) and lines 278–282: a `NumChords(state, polyphony)`
  helper replaces upstream's fixed `kNumChords` for the structure-pot quantization (table sizes differ).
- Upstream background: upstream selects between its two historical tables at **compile time** via
  `#define BRYAN_CHORDS` in `dsp/dsp.h`; lyle removed the define and made it a runtime option.
- "Jon" table derives from Jon Butler's Plaits chord engine, commit
  `8bd0b5e2b8088253e341ec1a50a876850e854c73` (`plaits/dsp/engine/chord_engine.cc` L42–62 in lylepmills/eurorack).
- Mostly constant data (~400 lines) plus small selection branches; no DSP algorithm changes. **Clean.**

**Options menu** (as a UI pattern for Schliere/table selection): commit `8a1bcab5f7`, implementation in
`ui.cc` (`UI_MODE_OPTIONS_MENU`, pages on left LED / values on right LED, enter via 3 s both-button hold
starting left, calibration via hold starting right) and `settings.{h,cc}` (`SwitchModeOption()` etc.).
This is the most entangled part (ui.cc +305 diff lines, includes new `CONTROL_SWITCH_HOLD` event handling),
but the user plans a custom menu anyway — use it as a reference, not a patch.

## 3. Other forks / alternatives

Surveyed the top ~100 forks of pichenettes/eurorack by stars (API `forks?sort=stargazers`), GitHub repo
search, and branch lists of candidates. **No other maintained Rings firmware fork exists.** Notables:

| Repo | Stars | Last push | Rings content |
|---|---|---|---|
| lylepmills/eurorack | 134 | 2026-11-16 | **The** Rings alt firmware (above) |
| mqtthiqs/parasites | 129 | 2023-06 | Tides/Warps/Clouds "Parasites" — no Rings |
| boourns/eurorack | 70 | 2020-12 | Peaks "Dead Man's Catch" — no Rings |
| VCVRack/pichenettes-eurorack | 31 | 2026-10-31 | VCV's maintained mirror backing Audible Instruments (desktop port, different build; DSP reference only) |
| jkammerl/eurorack | 29 | 2017 | Clouds "kammerl beat-repeat" — no Rings |
| qiemem/eurorack | — | 2026-05 | Stages-only branches (checked branch list) — no Rings |
| insolentconstruct/eurorack | 9 | 2026-11-20 | Beads "Drizzlet" — no Rings |
| andiogenes/frown-of-misery-1 | 0 | 2026-11 | Braids/Rings/Clouds on an M-VAVE FM-1 guitar pedal — curiosity, not a Eurorack base |
| c4pt0r/mi-sim | 0 | 2026-09 | Runs unmodified MI firmware (incl. Rings) in a desktop simulator — potentially useful as a **host-side test rig** for Schliere, not a base |

## 4. Toolchain reality check [measured, local build 2026-11-20]

Nobody in the ecosystem documents a modern-GCC build: upstream's default is `/usr/local/arm-4.8.3/`,
pichenettes/mutable-dev-environment is a Vagrant VM around the same era, and **even lylepmills' 2026
`Dockerfile.plaits-builder` still pins gcc-arm-none-eabi 4.8-2013q4** (downloaded from launchpad).

Local experiment: assembled upstream master + `stmlib @ master` + `stm_audio_bootloader @ b376bd6448` as
plain folders and built with **Arm GNU 12.2.rel1** (`/Applications/ArmGNUToolchain/12.2.rel1/`):

1. Stock flags fail: `core_cmFunc.h:414: error: ISO C++17 does not allow 'register' storage class specifier`
   (GCC 12 defaults to gnu++17; `-Werror` promotes it).
2. With `-std=gnu++11` one error remains: `core_cm4.h:85: listing the stack pointer register 'sp' in a
   clobber list is deprecated [-Werror=deprecated]` (from `__set_MSP`, hit via `stmlib/system/bootloader_utils.cc`).
3. **This builds cleanly, zero source changes:**

   ```sh
   make -f rings/makefile \
     TOOLCHAIN_PATH=/Applications/ArmGNUToolchain/12.2.rel1/arm-none-eabi/ \
     CPPFLAGS="-fno-exceptions -fno-rtti -std=gnu++11 -Wno-deprecated"
   ```

   Result: `rings.elf` / `rings.hex` / `rings.bin` — text 82416, data 1600, bss 181968; `rings.bin` 84 016 bytes
   (fits easily; app region starts at `0x08008000` of the 1 MB flash). `-Werror` stays on.
   **Not flashed to hardware** — codegen differences (gcc 4.8 → 12.2, `-fshort-enums`, `-O2`) are untested on a module.
4. Known cosmetic quirk: the `size` display target pipes through `figlet | cowsay`.
5. The **firmware-WAV encoder is Python 2** (`stm_audio_bootloader/qpsk/encoder.py`, needs numpy;
   lylepmills' Docker image installs python2.7 + numpy 1.16.6 exactly for this). Options for the slim repo:
   port the encoder to Python 3 (it is small: `qpsk/encoder.py` + `audio_stream_writer.py`), or keep a
   python2 container step. Flashing via SWD/openocd (`upload_combo_jtag_erase_first` target) avoids it entirely.

## 5. Recommendation

**Base the slim fork on upstream `pichenettes/eurorack` master (`rings/` tip `c5f910fb25`), vendoring
`stmlib @ e3bd7c9cc0` (upstream's pin) and `stm_audio_bootloader @ b376bd6448` as plain folders, and port
the two lylepmills features (plus optionally the chord tables) as small, reviewable patches taken from the
lylepmills master diff. Do not base on any lylepmills branch.**

Reasoning:

1. **lyle master is a superset of what you don't want.** Mini-Elements brings new DSP files, sample data,
   a 48→32 kHz sample-rate drop, removed CV slew, removed easter-egg handshake and a relocated calibration
   flow. Removing all that from lyle's tree is far more invasive (and harder to verify) than adding
   ~150 lines of locking + ~450 lines of chord tables to a frozen, known-good upstream.
2. **Upstream `rings/` is stable and effectively final** (no functional change since 2020); there is no
   ongoing upstream development you would miss by forking now, and no other fork worth tracking.
3. **The wanted features extract cleanly** (section 2.1): frequency locking is localized in
   `cv_scaler.cc/settings/ui/performance_state`; the octave alt-function is ~10 lines; chord tables are
   constant data plus a selection enum. The only lyle couplings are `MiniElements()` guards, trivially dropped.
   Port from the final diff, not by cherry-picking (history contains reverts of the 2020 implementation).
4. **48 kHz stays.** Schliere gets the full bandwidth; if 61 allpass sections ever exceed the STM32F405's
   budget (168 MHz M4 — far slower than the H750 the OWL version ran on at 33–54 % CPU), lyle's 32 kHz change
   is a documented one-line fallback (`rings/dsp/dsp.h`, `kSampleRate`), proven on hardware by his firmware.
5. **Build is a solved problem** with Arm GNU 12.2: two extra `CPPFLAGS` (`-std=gnu++11 -Wno-deprecated`),
   verified by an actual link today. Bake them into the slim repo's makefile.
6. **Useful patterns to copy from lyle even where code isn't:** the `kSettingsId` settings-version marker
   (new `State` fields shift the flash settings layout — detect and reinitialize), the two-LED options-menu
   UX (commit `8a1bcab5f7`) as the model for selecting Schliere and chord tables, and
   `locked_frequency_pot_value` as the plumbing for giving the locked frequency knob a Schliere function.

Suggested slim-repo layout: `rings/` (upstream tip + patches), `stmlib/` and `stm_audio_bootloader/`
(vendored at the pins above, MIT), a top-level makefile wrapper with the 12.2 flags, and the lyle feature
ports as separate commits/patch files so they remain diffable against both parents.

## 6. Sources

- Upstream repo: <https://github.com/pichenettes/eurorack> (master; `rings/` tip `c5f910fb25264b7b45ec6249bfaa5050a9cb513d`)
- Upstream submodule pins (git tree API): `stmlib e3bd7c9cc00e4364166f9905c0509b6ffd0535ec`, `stm_audio_bootloader 1ec7d6374e2d305cb710170eb80de9b66e795792`
- <https://github.com/pichenettes/stmlib> (head `d18def816c`), <https://github.com/pichenettes/stm-audio-bootloader> (head `b376bd644871edc06f9ee75fb95be516a10ab394`)
- lylepmills fork: <https://github.com/lylepmills/eurorack> (master `0390557b34`), branches via
  <https://api.github.com/repos/lylepmills/eurorack/branches>
- Alt-firmware README: <https://github.com/lylepmills/eurorack/blob/master/alt_firmwares/README.md>;
  release WAV `alt_firmwares/rings_mini_elements.wav`; discussion <https://llllllll.co/t/mutable-instruments-alt-firmwares/63936>
- Key lyle commits: `4d6da31c3fa07f463cc5f06bb36c731ff65c80bc`, `3562b9698170bb33da3cb23daac1096d88a1a2fd`,
  `8a1bcab5f7fb775695d72a9156ab99394c900455`, `72ac1cd5a0e30807de1c4b8af3999540f959c6e2`,
  `a40bcd9539f8b5631a5a99c9bed335ed8dc55ad7`, `b2dcbdd58d813a16c5a0670428c9d01347ecd731`,
  branch heads `5ff8e1cfa0` (revamp), `e2ac597222` (flashing_yellow), `edb5796477` (midi), `4acb36cb09` (update_wav_file)
- Jon Butler chord source: <https://github.com/lylepmills/eurorack/blob/8bd0b5e2b8088253e341ec1a50a876850e854c73/plaits/dsp/engine/chord_engine.cc#L42-L62>
- Fork survey: <https://api.github.com/repos/pichenettes/eurorack/forks?sort=stargazers>; checked
  mqtthiqs/parasites, boourns, jkammerl, qiemem (branch list), insolentconstruct, VCVRack/pichenettes-eurorack,
  andiogenes/frown-of-misery-1, c4pt0r/mi-sim
- Dev environment: <https://github.com/pichenettes/mutable-dev-environment> (Vagrant, gcc 4.8 era);
  lylepmills `Dockerfile.plaits-builder` (pins gcc-arm-none-eabi 4.8-2013q4)
- Local build evidence (this machine, 2026-11-20): upstream master + submodules in `$TMPDIR/rings-research/src/upstream`,
  Arm GNU 12.2.rel1, flags as in section 4; output `build/rings/rings.bin` (84 016 B)
