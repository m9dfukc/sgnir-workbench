# Smear → Rings control mapping — proposal (2026-11-20)

Discussion basis for the Schliere mode on Rings. Compares the real Smear feature set (product
page, FW 1.2.1) with Rings' controls, instead of just carrying over the reduced AC/DC mapping.
Starting point was `reference/schliere-owl/ANLEITUNG.md` section 5.3.

## What Smear actually is [sourced: smoothieaudio.com/products/smear]

- **Two identical sections** (default signal flow: stereo in → left section → right section →
  out; a **Split** switch makes them parallel, one per input). Per section: **Frequency**
  (20 Hz–20 kHz), **Resonance** (Q 0.01–16), **Tap** (chain length, 2–98 poles, crossfaded),
  plus Frequency-CV and Resonance-CV each with attenuverter, and a Tap CV.
- **Global Feedback** knob + CV ("resonant, flanger-like and physical-modelling sounds").
- 7 CV inputs, ±5 V, audio rate. 2 in / 2 out, 48 kHz, Electrosmith Patch SM (STM32H750).
- Marketing modes: transient smearing / spectral delay, resonator & percussion voice (pushed
  into feedback), morphing filter / phasor.

**Reality check for Rings (STM32F405, ~¼ of the H750's compute):** two full sections at 48 kHz
are out of reach; Schliere's single chain with two output taps (L up to 98 poles, R up to 122)
is the fitting one-section equivalent and already proven on the AC/DC. Rings reads CV once per
24-sample block (~2 kHz), so Smear's audio-rate CV cannot be matched on the CV inputs [code];
variant S band-limits those paths anyway.

## Rings panel inventory [code: cv_scaler, ANLEITUNG section 2]

5 knobs (FREQUENCY, STRUCTURE big; BRIGHTNESS, DAMPING, POSITION small), 5 CV inputs each with
attenuverter, calibrated V/OCT, STRUM (trigger, with auto-strum detection), 2 buttons, 2
tri-color LEDs, 1 audio in, 2 outs (ODD/EVEN).

## Proposed mapping

| Rings control | Schliere parameter | Notes |
|---|---|---|
| FREQUENCY + V/OCT + FM CV & att | **Frequency**, continuous, 10 octaves | Calibrated 1 V/oct — better than both the AC/DC and Smear. Bypasses Rings' semitone quantizer (ANLEITUNG 5.2) |
| STRUCTURE + CV + att | **Tap** (position, crossfade over the 7 detents) | Smear's per-section Tap; detents shown on LED 1 color, briefly, after knob movement |
| BRIGHTNESS + CV + att | **Resonance** (Q 0.01–16) | 1:1 Smear |
| DAMPING + CV + att | **Feedback** (knob 0…95 %, CV bipolar via attenuverter → negative feedback) | 1:1 Smear global feedback; AC/DC behavior kept |
| POSITION + CV + att | **Spread** — distance between the L and R taps along the chain | **The new idea, see below.** Noon ≈ the fixed AC/DC L/R layout; CCW → mono (taps coincide); CW → max spread (L early, R at the chain end = tail) |
| STRUM | **Excite**: inject a short impulse/burst into the chain input | Turns Schliere into Smear's "percussion voice" deliberately: with resonance/feedback up, STRUM + V/OCT = playable percussion/string hybrid. Uses Rings' existing strum detection |
| IN | Audio (bypasses the noise gate) | mono in, as on the AC/DC |
| ODD / EVEN | **L / R** | With Spread, Diff/Tail outputs become unnecessary: spread CCW→mono≈single tap, CW end = tail on R. No routing menu needed |
| Right button short (in Schliere bank) | Variant select, later (S → O → A; LED 2 green/yellow/red) | First build: S only, button does nothing |
| Right button long | Bank cycle (models → bonus → Schliere), decided Q12 | LED 2 hard-blinks in the Schliere bank |
| Left button | Reserve (e.g. dry/wet toggle or tap-LED mode); LED 1 flashes red while the loop limiter works | AC/DC LED 4 equivalent |

### Why Spread instead of a free POSITION assignment

ANLEITUNG 5.3 left POSITION "free: stereo width or dry/wet". Looking at Smear settles it:
Smear's defining stereo trick is **two different tap positions** (two sections in series).
A single chain with two *independently movable* taps reproduces exactly that character —
one knob morphs from mono through the classic Schliere stereo to "early reflection vs. tail".
Dry/wet, by contrast, is trivially patched externally (mult the input into a mixer) and would
waste the only free CV-able knob. Cost: the R tap gets the same crossfade machinery as L
(~a handful of multiplies per sample) — negligible next to the 61 filter stages [hypothesis,
to be profiled].

### What is consciously dropped or deferred

- **Second section / Split:** no CPU for it [hypothesis, profiling will confirm headroom].
- **Audio-rate CV:** impossible via Rings CV path (block-rate reads) [code]. Possible later as
  a sub-mode: IN modulates the feedback amount instead of feeding the chain (ANLEITUNG 4.1).
- **Dry/wet:** patch it externally; revisit only if the left button needs a job.
- **Chain clear / feedback freeze on STRUM:** excite is more musical; a "panic" clear can hide
  on a button combo later if runaway feedback proves annoying.
- **Frequency-locking synergy:** once the lylepmills frequency locking is in, the locked
  FREQUENCY knob is free — candidate alt function: fine spread or dry/wet. Decide after both
  features exist.

## Decision points for the user

1. **POSITION = Spread** (recommended) — or dry/wet, or keep ANLEITUNG's switchable output
   routing on the left button?
2. **STRUM = Excite impulse** (recommended) — or chain clear, or feedback freeze?
3. **Excite flavor** (if 1 ja): single-sample impulse (purest ring-out) vs. short filtered noise
   burst (more "mallet") — can also be variant-dependent or decided by ear on hardware.
4. **Left button**: reserve empty for now (recommended) vs. assign dry/wet toggle immediately?
