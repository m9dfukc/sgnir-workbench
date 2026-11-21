# eurorack-mi-rings — Workbench for a custom Rings firmware

Workbench for developing **eurorack-mi-sgnir**, a slim custom firmware for the **Mutable
Instruments Rings** Eurorack module. Mutable Instruments no longer exists; the firmware is
community-maintained. Goals, in this order:

1. **Workbench:** build the Rings firmware reproducibly, flash it safely with a proven rollback
   path, test the DSP on the host before it touches hardware.
2. **Schliere port:** port the OWL patch **SchliereS** (developed in the user's `acdc-workbench`
   project) to Rings as an additional mode alongside the resonator models.
3. **Selected features from the lylepmills fork** (https://github.com/lylepmills/eurorack):
   **frequency locking** and **frequency knob alt functionality**; the new chord tables are
   optional. None of the other lylepmills features are wanted.

Inspiration for Schliere is the Smoothie Audio **Smear** module
(https://smoothieaudio.com/products/smear). On the AC/DC the patch was heavily reduced (4 knobs,
4 CV); Rings offers more controls, so the Smear→Rings mapping gets a fresh design round.

## Device

- **One original Rings** module, no reference device. A Rings clone could be borrowed from a
  friend if needed.
- MCU **STM32F405** (Cortex-M4F, 168 MHz), ≈ ¼ of the AC/DC's compute. 48 kHz, block size 24
  [code: `rings/dsp/dsp.h`]. Audio: 1 in, 2 out (ODD/EVEN), 16-bit codec path.
- Controls: 5 knobs, attenuverters, CV inputs + V/OCT + STRUM, 2 buttons, 2 bicolor LEDs
  [code: see `reference/schliere-owl/ANLEITUNG.md` section 2].
- Firmware update path: **audio bootloader** (official firmware ships as a WAV played into the
  input). SWD fallback: one of the user's **RP2040 boards with the raspberrypi/debugprobe
  firmware** (CMSIS-DAP + UART bridge) — only a 2x5 1.27 mm Cortex-Debug adapter set needs to
  be ordered for JP1. Second recovery path with zero purchases: JP2 is a plain 2.54 mm 1x6
  header [code: `rings_v30.brd`, package `1X06`] → ROM serial bootloader via any 3.3 V UART +
  RESET/SYSBOOT buttons (research: `docs/research/notes/stlink-options-2026-11-20.md`).
- V/OCT is calibrated in the stock firmware; calibration data lives in flash settings
  (sector 1). Audio-bootloader updates keep it [code: the bootloader erases from sector 2
  on]; the before/after check on the module is still **[open]** (`TODO.md`). No calibration
  backup exists yet (needs SWD or the serial bootloader).

## Decisions (user, 2026-11-20)

- **Two-level structure:** this directory is the workbench, public as
  **`m9dfukc/sgnir-workbench`** (user, 2026-11-20; remote `origin`, branch `main`, MIT).
  The firmware fork is a separate GitHub repo **`m9dfukc/eurorack-mi-sgnir`**, embedded
  here as a git submodule under `firmware/` (once created). Folder names inside the repo
  (`rings/`, …) stay as upstream.
- **Slim fork = snapshot, not a GitHub fork:** new repo initialized from only the needed folders
  (`rings/`, `stmlib/`, `stm_audio_bootloader/`, build files) at a pinned upstream commit. The
  base commit hash is documented in the repo (README/BUILD_INFO); all own changes are clean
  commits on top. Diffs against upstream/lylepmills are made against their repos via tarball.
- **`stmlib` and `stm_audio_bootloader` are vendored** (plain copies with their source commit
  documented), not submodules. `git clone && make` must suffice.
- **Public + MIT throughout.** The Rings firmware is MIT; the user relicenses his Schliere
  sources as MIT (he is the author). README credits Mutable Instruments, lylepmills,
  acdc-workbench.
- **Port scope:** variant **S** first, implemented as one class with the variant switches from
  `ANLEITUNG.md` section 4.2, so A and O become pure configuration later. M (morph) is dropped.
- **Fork base (decided 2026-11-20, after research):** upstream `pichenettes/eurorack` master
  (`f4cfa769`, rings tip `c5f910fb25`), stmlib `e3bd7c9c`, stm-audio-bootloader `1ec7d637` —
  NOT a lylepmills branch (his rings/ bundles unwanted features: Mini-Elements, 32 kHz, no CV
  slew). The lylepmills features are ported from the master *diff* (history contains reverts, so
  no cherry-picks). Details: `docs/research/notes/rings-forks-2026-11-20.md`.
- **Chord tables:** later, as an optional patch after the core features work (user, Q11).
- **UI approach:** third model bank on long-press of the model button (grounded models → bonus
  models → Schliere, LED 2 hard-blinking), per `ANLEITUNG.md` 5.4 — not an options menu.
  Frequency locking brings its own both-buttons toggle from lylepmills (user, Q12).
- **Calibration entry (user, 2026-11-20):** both buttons held **6 s** (stock: any press of both
  buttons); short press of both = no-op; 0.2–6 s = frequency-locking toggle. Single-button long
  presses stay at 3 s.
- **Frequency knob alt (user, 2026-11-20):** octave snapping relative to the locked pitch (knob
  keeps its 12-semitones-per-20 % scale, snaps to whole octaves, hysteresis) — not lylepmills'
  absolute 7-step switch, not a pickup mode.
- **Schliere controls (user, 2026-11-20):** left button = dry/wet in steps 100 / 75 / 50 / 25 %
  wet (default 100 %; revised from five steps incl. 0 %), shown on LED 1 for 1.5 s as red /
  yellow / green / off (no blinking). Full control/LED reference: sgnir README. Frequency = own continuous path over the whole knob (≈ 11 octaves) + calibrated
  V/OCT + FM; frequency locking freezes the knob, alt function = octave snapping (≈ 9 % travel
  per octave). Excite only on a trigger at the STRUM jack.
- **Schliere 48 kHz vs. 96 kHz (user, 2026-11-20, after measuring on the module):** default =
  variant S **without** its 96 kHz feedback path (81 % vs. 89 % peak load; audible difference,
  48 kHz judged fine). Right button in the Schliere bank (user, 2026-11-20): press = excite
  (on button down); held 1–3 s = 48/96 kHz toggle on release (LED 2 green = 96 kHz, yellow =
  48 kHz, preview after 1 s, saved); held 3 s = bank change as before. Excite flavor on the
  left button long press (LED 1 shows it for 1.5 s).
- **Flash path:** audio bootloader is primary; the Python-2 WAV encoder
  (`stm_audio_bootloader/qpsk/encoder.py`) gets ported to Python 3 as a uv tool in this
  workbench, verified against official WAVs (user, Q13). SWD is fallback/rollback — **revised
  2026-11-20 (user): no ST-Link purchase; an RP2040 board with the debugprobe firmware is the
  probe**, only the 2x5 1.27 mm adapter set gets ordered. Rings' serial-bootloader header JP2
  (2.54 mm, jumper-wire friendly) + RESET/SYSBOOT buttons are the second recovery path and
  work with hardware on hand (see ST-Link research note).
- **Sample rate:** target 48 kHz for Schliere; the 32 kHz fallback only after profiling shows
  it is needed, and only with the user's sign-off (user, Q14).
- **Smear→Rings control mapping (decided 2026-11-20, user):** per
  `docs/smear-mapping-proposal.md` — POSITION = Spread (L/R tap distance, CCW mono → CW
  early/tail); STRUM = Excite impulse into the chain input; excite flavor (single-sample
  impulse vs. filtered noise burst) implemented both ways, decided by ear on hardware; left
  button = dry/wet toggle from the start.

## Language policy (user, 2026-11-20)

- Conversation with the user: German or English — follow the user's language.
- **All artifacts are English:** Markdown docs, code comments, identifiers, commit messages,
  reports, TODO entries. (Unlike acdc-workbench, where German was the source language.)
- Imported reference material stays in its original language (`reference/schliere-owl/` is
  German). The German↔English term mapping lives in **`docs/lexicon.md`** — keep it current when
  translating acdc material.
- Evidence markers, translated from the acdc convention and to be kept in all docs:
  - **[sourced]**: external source exists (forum, repo, manufacturer).
  - **[code]**: derived from source code, not verified on hardware.
  - **[hypothesis]**: conclusion, untested.
  - **[measured]**: confirmed/refuted by own measurement, with a reference to the log.
  - **[open]**: must be clarified on the Rings itself.

  Only upgrade a marker with a reference to the measurement/source.

## Safety rules for the hardware

- **Never flash without the user's explicit approval.** There is only one Rings. **The user
  flashes himself** (he plays the WAV into the module); Claude builds and documents the image
  and never plays audio into the module or drives a programmer.
- Way back: `backup/rings_1.1.wav` (official firmware 1.1). The audio-bootloader procedure is
  proven with it and with sgnir builds [measured, 2026-11-20]; the SWD and serial recovery
  paths are not proven yet (`TODO.md`).
- Calibration: see "Device". If reachable (SWD/serial), back it up before experiments that
  touch the settings layout.
- Images for the module are made with the **`hardware-build` skill**: every build gets a
  record under `firmware/builds/`, every flash session a log under `firmware/flashlog/` (one
  folder per day and base commit, one section per flashed build: procedure, result, readings).
- The stock build (`firmware/builds/rings-stock-f4cfa769/`) stays as the toolchain reference.
  Every toolchain adaptation is its own commit.
- Host tests before hardware: the 65 tests in `reference/schliere-owl/` are the port reference —
  the Rings port must produce the same numbers for the same input (except for the documented
  tanh approximation, `ANLEITUNG.md` section 4.3). The port's own tests:
  `rings/test/schliere_test.cc` in the firmware repo and `tests/schliere/` (equivalence) here.

## Files

| Path | Content |
|---|---|
| `TODO.md` | Open points, done log — **start here** |
| `README.md` | Overview for outsiders (English) |
| `CLAUDE.md` | This file |
| `docs/lexicon.md` | German↔English term mapping |
| `docs/research/notes/` | Research reports with all URLs/hashes (Rings forks, ST-Link options). Search here before researching anew |
| `reference/schliere-owl/` | The port package from acdc-workbench (German, imported as-is): OWL sources of all six Schliere variants, `ANLEITUNG.md` (the port guide — read it before touching DSP), host tests. Do not edit; the port adapts *copies* in the firmware repo |
| `reference/acdc-workbench-CLAUDE.md` | Snapshot of the origin project's CLAUDE.md, for "what did we do there" lookups |
| `firmware/eurorack-mi-sgnir/` | Submodule: the firmware repo (https://github.com/m9dfukc/eurorack-mi-sgnir, fetch https, push ssh). Build docs in its README |
| `firmware/builds/`, `firmware/flashlog/` | Build records and flash logs, local only (gitignored). `rings-stock-f4cfa769/` = verified stock build |
| `tools/` | `sgnir-tools` (uv project): `qpsk-encode` / `qpsk-decode` for the audio-bootloader WAV, verified against the official 1.1 WAV |
| `tests/schliere/` | Equivalence test: Rings port vs. the OWL reference |
| `.claude/rules/firmware.md` | Firmware rules (build gotchas, CPU budget, settings layout, release flow). Loads by itself when files under `firmware/eurorack-mi-sgnir/`, `tests/schliere/` or `tools/src/` are read or edited |
| `.claude/skills/hardware-build/` | Skill + build script for images that go onto the module |
| `docs/handoffs/` | Session handoffs — **local only, gitignored, never commit**. Working notes that go stale fast; the newest file is the current one, never delete. CLAUDE.md and TODO.md must not reference individual handoff files — anything durable belongs in TODO.md, CLAUDE.md, or the docs |
| `backup/` | Device backups (firmware WAVs, calibration) — local only, never overwrite |
| `_tmp/` | User's scratch space. Ignored by git, never delete |

Host tests (sanity check of the reference package, passes on this machine 2026-11-20):

```bash
cd reference/schliere-owl
c++ -std=c++17 -O1 -Iowl/test -Iowl -Iowl/schliere -o "$TMPDIR/schliere_test" owl/test/schliere_test.cpp
"$TMPDIR/schliere_test"
```

## Conventions

- Python via **uv** (`uv run`, `uv add`). In the sandbox `~/.cache/uv` is not writable:
  `UV_CACHE_DIR=.uv-cache uv run …`.
- Foreign sources as **tarballs from codeload.github.com** (sandboxed `git clone` of external
  repos fails on `.git` writes), each into its own fresh directory.
- `git` and `gh` run outside the sandbox (`excludedCommands` in `.claude/settings.json`):
  inside it `.git/` writes are blocked and `gh`/GitHub API calls fail TLS through the proxy.
  **Commit and push only after the user's explicit approval that names the action** ("ja,
  committen und pushen"); an "ok" to something else does not count. `gh` always with explicit
  `--repo m9dfukc/…`.
- Arm toolchain: **Arm GNU 12.2.Rel1** at `/Applications/ArmGNUToolchain/12.2.rel1/` (from the
  acdc project; `TOOLCHAIN_PATH` is its `arm-none-eabi/` folder). The gcc-4.8-era build chain
  needs only a `-std=gnu++11` flag override, no source changes [measured]; CI uses the same
  toolchain version on Linux.
- Before any commit: `git status --short` must show no binaries, WAVs, or `backup/` files.
  Nothing private beyond the username.
- Tool CLIs: English help/console texts, reproducible (arguments, no hard-coded paths).

## Status (2026-11-21)

Current state only; history lives in `TODO.md` ("Done"), the sgnir git log and
`firmware/flashlog/` (local).

- **Released:** sgnir `v0.1.0` (tag on `ec4a7aa`): frequency locking, frequency knob alt
  (octave snapping), Schliere S as third bank.
  https://github.com/m9dfukc/eurorack-mi-sgnir/releases/tag/v0.1.0
- **On the module:** `diag7` (`firmware/builds/sgnir-409ab6a-diag7/`, benchmark build: LEDs
  blink the load, Schliere audio dry). Way back to a release: `rc4` = code of `v0.1.0`.
- **Committed, not released:** sgnir `a922267` (Schliere CPU steps 1–4, one-vfma chain);
  release candidate `rc5` (`firmware/builds/sgnir-409ab6a-rc5/`, same code) built, not flashed.
- **Measured on the module** (diag7, peaks): Schliere 59 % of the block budget at 48 kHz,
  69 % with the 96 kHz feedback path (v0.1.0: 81 / 89 %); base 13 %; grounded models 71–84 %
  depending on the model. Worst case [open].
- **Not proven yet:** calibration before/after check on an update; SWD and serial recovery
  paths (hardware prep in `TODO.md`).
- **Next:** listening/worst-case tests on the module, level adjustment, excite level by ear.

## Skills and rules (project-local, `.claude/`)

- `hardware-build` (own): build an image for the module with a build record, hand it to the
  user, write the flash log from his report. Use it for every build that is meant to be
  flashed.
- `.claude/rules/firmware.md` (own): see "Files".
- From `mattpocock/skills` @ `5863806` (2026-11-12): `grill-me`/`grilling` (stress-test a plan
  in question rounds — use before firmware steps or design decisions), `handoff` (condense a
  session), `research` (question → Markdown report, store in `docs/research/notes/`).
