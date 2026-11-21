---
name: hardware-build
description: Build a flashable test image of the sgnir Rings firmware and keep the paper trail around it - the build record in firmware/builds/, the handover to the user (who flashes it himself), and the flash log once he reports back. Use this whenever something is meant to run on the module - a test, diagnostic, benchmark, listening or release-candidate build, "build me a WAV", "mach einen Build zum Flashen", "ich will das auf dem Modul hören/messen" - and also whenever the user reports what a flashed build did (LED readings, load percentages, what he heard), even if he does not mention a log. Not for tagged GitHub releases; those are built by CI.
---

# Hardware test build

There is one Rings and no proven SWD recovery yet, so every image that goes onto the module
must be traceable: which commit, which uncommitted changes, which toolchain, and what it did on
the hardware. The loop is always the same:

**change → build record → handover → the user flashes → he reports → flash log**

Roles: you build and keep the records. The **user flashes** (he plays the WAV into the module
himself) and tells you what he saw and heard. A flash cycle costs him several minutes (power
cycle, 1.5 min of WAV, testing), so make each one count.

Where things are: what is on the module right now = the "On the module" line in `CLAUDE.md`
("Status"). Control names and what each knob, button and LED does in the Schliere bank = the
tables in `firmware/eurorack-mi-sgnir/README.md`; German terms = `docs/lexicon.md`.

## 1. Before building

- **Know what the build is for** and what the user should check on the module. If the request
  and the available instrument do not fit (see the switches below), ask before building; a
  build that cannot answer the question wastes a flash cycle.
- **Pick the label:** `diag<N>` measuring/diagnosing, `listen<N>` listening tests, `rc<N>`
  release candidates. N continues the series in `firmware/builds/` (`ls` it) across base
  commits: after `sgnir-81fe986-diag6` comes `sgnir-<new-sha>-diag7`. No label is only for a
  clean, committed tree (the record of exactly that commit).
- **Diagnostic switches** (`firmware/eurorack-mi-sgnir/rings/diagnostics.h`), on only in
  `diag` builds:
  - `SCHLIERE_CPU_METER`: Schliere runs normally, LED 1 shows the load in four color bands.
    Good for "does it survive this setting", poor for numbers (the noon reading is already
    in the top steady band).
  - `SCHLIERE_BENCHMARK` (needs the meter define too): blinks the load in percent, but
    Schliere runs only on every 16th block and the audio is dry. Good for numbers, useless
    for listening.

  Read how a display behaves from the code (`rings/ui.cc`, `rings/rings.cc`), not from header
  comments or older `BUILD_INFO.txt` files: thresholds, timing, what the buttons do in that
  mode, and which other LED states take priority over it.
- **Look for riders:** open on-module checks in `TODO.md` can often be done with the same
  flash. If a rider needs a "before" observation (e.g. note model and polyphony before
  updating), it goes to the top of the handover.

## 2. Build

```bash
bash .claude/skills/hardware-build/scripts/build.sh -m "<description>" <label>
```

The script runs the host tests, makes a clean firmware build, encodes the audio-bootloader
WAV, decodes it again and compares it with the binary, and only then writes
`firmware/builds/sgnir-<short-sha>[-<label>]/` with `rings.bin`, `sgnir-<label>.wav`,
`SHA256SUMS`, `BUILD_INFO.txt` and, for uncommitted changes, `uncommitted.diff` +
`status.txt`. On any failure it leaves no record behind. It takes about 10 seconds and never
flashes or plays anything (`--help` lists the options).

The description (`-m`) becomes the top of `BUILD_INFO.txt`. Write it for someone who finds the
folder in a month: what kind of build, what changed against the previous one, and for
diagnostic builds how to read the display. Its shape (wording from an older build, whose
button functions no longer exist):

```
Diagnostic build 6: diag5 minus RAM code (back in flash), flash prefetch on at boot.
Right short in the Schliere bank cycles: 96 kHz on (LED 2 green) -> off (yellow) -> base load
only (red). Display: 1.5 s off, tens red (0.8 s each), 1 s off, ones green, 2 s status.
```

If the host tests fail, fix the cause instead of reaching for `--skip-tests`: the tests are
the only check the DSP gets before it runs on the one module. Compare the reported
`rings.bin` size with the previous record's `BUILD_INFO.txt`; a jump you cannot explain is
worth a look before anyone flashes it.

A build record is a record: never rebuild into, edit or delete an existing folder. Another
attempt gets the next label. `BUILD_INFO.txt` is not updated after flashing; that is what the
flash log is for. Images are only ever taken from a record, never from
`firmware/eurorack-mi-sgnir/build/`, which just holds whatever was compiled last.

## 3. Hand over

Tell the user, in his language and without making him open any file:

1. anything to note down before flashing (riders),
2. the WAV as a clickable path,
3. what is different from what is on the module now,
4. what to do and watch for, in the names printed on the panel (STRUCTURE, DAMPING, …): the
   settings to try, how to read the LED display (colors, timing), what can mask it, and what
   the display cannot tell,
5. what to report back (numbers or colors, what he heard, knob positions for each reading),
6. the way back: the build named in `CLAUDE.md` "On the module" if it is a release-type
   build, else the last `rc`/release record, or `backup/rings_1.1.wav` (official 1.1).

The update procedure is in `firmware/eurorack-mi-sgnir/docs/FLASHING.md`; he knows it, so at
most remind him of the two things that went wrong before: playback too quiet fails after
about 30 s (FREQUENCY is the input gain in the bootloader), and after an error a press of the
left button restarts reception without a power cycle.

Then stop and wait for his report. Do not play the WAV yourself (`afplay` and the like) and
do not drive a programmer: flashing is his action, every time. Until he says a build is on
the module, nothing you write may claim it was flashed.

Leave the diagnostic switches as they are while a diag series is running; the record's
`uncommitted.diff` keeps them anyway. Turn them off when the series ends and before any commit.

## 4. Flash log

When he reports back, write it down before doing anything else; his observations are
measurements and the next session has no other source for them. Open the record's
`BUILD_INFO.txt` first so the log describes the build that was actually flashed.

`firmware/flashlog/<YYYY-MM-DD>-sgnir-<short-sha>/README.md` (local only, gitignored): day of
flashing and base commit of the build; one folder per day and base commit, one `##` section
per flashed build. A new folder starts with
`# Flash <date>: eurorack-mi-sgnir <short-sha> (<what this series is about>)`. Older logs are
laid out more loosely; leave them, use this form for new entries:

```markdown
## <label> (`firmware/builds/<record>/`)

- Flashed by the user: <attempts and why one failed; "not reported" if unknown>.
- Observed (user): "<his words, original language>" — <English rendering> [measured, user report]
- Readings: <table with the conditions: bank, knob positions, switches; for color or blink
  displays also the legend, so the entry can be read without the build>
- Riders: <result of each piggybacked check, or "not done">
- Interpretation [hypothesis]: <your reading, kept apart from the observations>
- Follow-up: <next build or decision>
```

Keep observation and interpretation apart and mark them (`[measured]`, `[hypothesis]`,
`[code]`, `[sourced]`, `[open]`), because yesterday's hypothesis is easily read as fact
later. What was not reported is "not reported", not a guess. Check the report against how
the display works in the code; if something does not add up, log it as `[open]` instead of
smoothing it over. If a later report corrects an earlier entry, add the correction as a new
entry instead of rewriting the old one.

Then answer him: what you logged, what it means, and once, together, the conditions that
are missing and matter for the conclusion. Say which build is on the module now if it is not
a release-type one.

## 5. Afterwards

- Carry durable results out of the local log: `TODO.md` (done/open items), the "On the
  module" line in `CLAUDE.md` whenever the firmware on the module changed (diagnostic builds
  included), the load figures in `.claude/rules/firmware.md`, the sgnir README ("Profiling")
  for measurements users care about.
- Upgrade an evidence marker only as far as the observation reaches, with a reference to the
  log entry: one tested setting does not close "worst case", and a check of two out of three
  items leaves the third `[open]`.
- Changes worth keeping get committed in the firmware repo, diagnostics off, after the user
  approved the commit (`.claude/rules/firmware.md` has what has to move together). A record
  built from uncommitted changes stays valid; note in the flash log which commit later
  contained them.
