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
  `ANLEITUNG.md` section 4.2. M (morph) is dropped. **Revised 2026-11-21 (user):** the only
  planned variants are S (48 and 96 kHz, exists) and **L** (longer chain, 48 kHz only, after
  the AC/DC SchliereL); O, A, Z, W are not planned. **L = 98 stages** (196 poles, taps L every
  13 / R every 16–17 stages; user 2026-11-21, after the diag9 benchmark: 82 % vs. L91 77 %).
  Switching S ↔ L: wet fades out, chain restarts silent, fades in (user).
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
  yellow / green / off (no blinking). **Swapped 2026-11-21 (user):** left short = excite
  flavor (changed more often, LED 1 steady green / yellow / red), left held 3 s = dry/wet,
  now shown as 4 / 3 / 2 / 1 yellow blinks (0.3 s on / 0.3 s off) = 100 / 75 / 50 / 25 % wet,
  because both displays share LED 1 and its colors. Full control/LED reference: sgnir README. Frequency = own continuous path over the whole knob (≈ 11 octaves) + calibrated
  V/OCT + FM; frequency locking freezes the knob, alt function = octave snapping (≈ 9 % travel
  per octave). Excite on a trigger at the STRUM jack; with STRUM **and** IN both unpatched,
  a V/OCT note change excites (Rings' strummer, no onset detection; user 2026-11-21).
- **Schliere 48 kHz vs. 96 kHz (user, 2026-11-20, after measuring on the module):** default =
  variant S **without** its 96 kHz feedback path (81 % vs. 89 % peak load; audible difference,
  48 kHz judged fine). Right button in the Schliere bank (user, 2026-11-20): press = excite
  (on button down); held 3 s = bank change as before. Excite flavor on the left button long
  press (LED 1 shows it for 1.5 s; short press since 2026-11-21, see above). **With L (user, 2026-11-21):** held 1–3 s cycles S 48 →
  S 96 → L on release, LED 2 green / yellow / red in that order (panel label order; the S
  colors were swapped against v0.1.0), preview after 1 s, saved.
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
  impulse vs. filtered noise burst) implemented both ways, decided by ear on hardware.
  **Revised 2026-11-21 (user, after `docs/research/notes/excite-signals-2026-11-21.md`):**
  three flavors impulse → mallet (f0-tracking Hann pulse, replaces the noise burst) → ping
  (Hann-windowed sine at f0, ≈ 2 Q cycles), LED 1 green / yellow / red, with random
  micro-variation per hit (level ±1 dB, polarity, width ±10 %); no per-trigger flavor cycling; left
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

- **Released:** sgnir `v0.3.0` (tag on `8dadfc6`, 2026-11-21): excite flavors impulse /
  mallet / ping with micro-variation (noise burst dropped), STRUM via V/OCT note change with
  STRUM and IN unpatched, left button swapped (short = flavor, held 3 s = dry/wet as yellow
  blinks). Released on the user's request before rc7 (same code) ran on the module: the
  excite code was heard (listen2) and measured (diag10), the button swap and the blink display
  were not. https://github.com/m9dfukc/eurorack-mi-sgnir/releases/tag/v0.3.0
  Previous: `v0.2.0` (tag on `c9aced5`): Schliere L, S/L cycle; `v0.1.0` (tag on `ec4a7aa`):
  frequency locking, frequency knob alt, Schliere S.
- **On the module:** `diag10` (`firmware/builds/sgnir-c9aced5-diag10/`, benchmark build of
  the v0.3.0 excite code before the button swap; LEDs blink the load, Schliere audio dry).
  Way back to a release: `rc7` = code of `v0.3.0`, `rc6` = code of `v0.2.0`, or the release
  WAVs.
- **Measured on the module** (diag10, peaks, knobs at noon, no input): Schliere S 61 % of the
  block budget at 48 kHz, 71 % with the 96 kHz feedback path, L 82 %; a running ping +1–3
  points (L 85 %); base 13 % (diag9); grounded models 71–84 % depending on the model (diag7).
  Worst case with a signal [open].
- **Not proven yet:** calibration before/after check on an update (V/OCT 1 V / 3 V test needs
  an exact voltage source); SWD and serial recovery paths (hardware prep in `TODO.md`).
- **Next:** flash the `v0.3.0` release WAV (or rc7): check the left button (short = flavor,
  held = dry/wet blinks), then worst-case load tests, level adjustment.

## Skills and rules (project-local, `.claude/`)

- `hardware-build` (own): build an image for the module with a build record, hand it to the
  user, write the flash log from his report. Use it for every build that is meant to be
  flashed.
- `.claude/rules/firmware.md` (own): see "Files".
- From `mattpocock/skills` @ `5863806` (2026-11-12): `grill-me`/`grilling` (stress-test a plan
  in question rounds — use before firmware steps or design decisions), `handoff` (condense a
  session), `research` (question → Markdown report, store in `docs/research/notes/`).
