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
  input). SWD via a cheap ST-Link as the fallback/rollback path — not yet bought
  (research: `docs/research/notes/stlink-options-2026-11-20.md`).
- V/OCT is calibrated in the stock firmware; calibration data lives in flash settings.
  Whether a firmware update preserves calibration/settings is **[open]** — clarify before the
  first flash.

## Decisions (user, 2026-11-20)

- **Two-level structure:** this directory is the workbench (local git repo, not published for
  now). The firmware fork is a separate GitHub repo **`m9dfukc/eurorack-mi-sgnir`**, embedded
  here as a git submodule under `firmware/` (once created). Mutable Instruments asked forks not
  to reuse the original names — hence "sgnir"; folder names inside the repo (`rings/`, …) stay
  as upstream.
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
- **Flash path:** audio bootloader is primary; the Python-2 WAV encoder
  (`stm_audio_bootloader/qpsk/encoder.py`) gets ported to Python 3 as a uv tool in this
  workbench, verified against official WAVs (user, Q13). SWD (cheap ST-Link V2 clone +
  1.27 mm adapter, to be ordered) is fallback/rollback; Rings also has a serial-bootloader
  header JP2 + RESET/SYSBOOT buttons as a second recovery path (see ST-Link research note).
- **Sample rate:** target 48 kHz for Schliere; the 32 kHz fallback only after profiling shows
  it is needed, and only with the user's sign-off (user, Q14).
- **Smear→Rings control mapping:** still open — dedicated design round; a mapping proposal
  document is the discussion basis.

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

- **Never flash without the user's explicit approval.** There is only one Rings.
- Before the first flash: the official Rings firmware WAV is downloaded and archived, and the
  audio bootloader update procedure has been proven once with the stock firmware.
- Clarify first [open]: does a firmware update erase settings/calibration? If reachable, back up
  calibration before experiments.
- Build the **stock firmware first** and keep the artifact before changing anything. Every
  toolchain adaptation is its own commit.
- One log folder per flash (date, build SHA, procedure, result) under `firmware/flashlog/`.
- Host tests before hardware: the 65 tests in `reference/schliere-owl/` are the port reference —
  the Rings port must produce the same numbers for the same input (except for the documented
  tanh approximation, `ANLEITUNG.md` section 4.3).

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
| `firmware/builds/`, `firmware/flashlog/` | Build artifacts and flash logs, local only (gitignored). `rings-stock-f4cfa769/` = verified stock build |
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
- Git writes (`.git/`) are blocked by the sandbox and need the exception.
  **Commit and push only after user approval.** `gh` always with explicit `--repo m9dfukc/…`.
- Arm toolchain: **Arm GNU 12.2.Rel1** at `/Applications/ArmGNUToolchain/12.2.rel1/` (from the
  acdc project). The original Rings build chain is gcc-4.8 era; adaptations are expected
  [hypothesis] and must be committed individually.
- Before any commit: `git status --short` must show no binaries, WAVs, or `backup/` files.
  Nothing private beyond the username.
- Tool CLIs: English help/console texts, reproducible (arguments, no hard-coded paths).

## Status (2026-11-20)

- Workbench set up: reference package verified (all 65 host tests pass), fork and ST-Link
  research done (see notes folder), project settings in `.claude/settings.json`.
- **`eurorack-mi-sgnir` created and pushed** (public, MIT): pristine upstream snapshot
  (`d030009`) + meta commit (`1d6bea5`), embedded as submodule `firmware/eurorack-mi-sgnir`.
- **Stock firmware build reproduced [measured]** with Arm GNU 12.2 and the `gnu++11` flag
  override, zero source changes: `rings.bin` 84 016 bytes, artifact + SHA in
  `firmware/builds/rings-stock-f4cfa769/`. Not yet flashed to hardware.
- Next: Smear→Rings mapping design round (proposal doc), WAV encoder py3 port, port patches
  (frequency locking, knob alt, Schliere).

## Skills (project-local, `.claude/skills/`)

From `mattpocock/skills` @ `5863806` (2026-11-12): `grill-me`/`grilling` (stress-test a plan in
question rounds — use before firmware steps or design decisions), `handoff` (condense a session),
`research` (question → Markdown report, store in `docs/research/notes/`).
