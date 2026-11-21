# sgnir-workbench

Workbench for **eurorack-mi-sgnir** (in `firmware/`) — my modified, slim custom firmware for the Mutable Instruments
**Rings** Eurorack module (STM32F405). Mutable Instruments no longer exists; this project builds
on the community-maintained sources.

## Goals

1. Reproducible Rings firmware builds, safe flashing (audio bootloader, SWD fallback), host-side
   DSP tests.
2. Port of **Schliere** — a chain of 61 second-order allpass filters with feedback, inspired by
   the Smoothie Audio *Smear* — as an additional mode alongside the resonator models. Originally
   an OWL patch for the Befaco/Rebel Technology AC/DC, developed in
   [acdc-workbench](https://github.com/m9dfukc/acdc-workbench).
3. Port of *frequency locking* and *frequency knob alt functionality* (optionally the new chord
   tables) from the [lylepmills fork](https://github.com/lylepmills/eurorack).

The firmware itself will live in a separate slim repo, `m9dfukc/eurorack-mi-sgnir` (snapshot of
the needed upstream folders at a pinned commit, vendored `stmlib`/`stm_audio_bootloader`, MIT).
Named "sgnir" because Mutable Instruments asked forks not to reuse the original module names.

## Layout

- `CLAUDE.md` — project context and rules (start here, together with `TODO.md`)
- `docs/` — lexicon (DE↔EN), research notes with all sources
- `reference/schliere-owl/` — the Schliere port package: OWL sources, port guide
  (`ANLEITUNG.md`, German), 65 host tests that define the port's expected output
- `firmware/` — (planned) the eurorack-mi-sgnir submodule, builds, flash logs

## Credits

- [Mutable Instruments / Émilie Gillet](https://github.com/pichenettes/eurorack) — Rings
  firmware, stmlib, stm_audio_bootloader (MIT)
- [Lyle Mills](https://github.com/lylepmills/eurorack) — alternative Rings firmware features
- [Smoothie Audio Smear](https://smoothieaudio.com/products/smear) — inspiration for Schliere
