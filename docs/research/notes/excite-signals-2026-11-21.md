# Excitation signals for the Schliere allpass chain (2026-11-21)

Research question: why does the current noise-burst excite (LCG noise, 3 ms exponential decay,
one-pole lowpass at 4 kHz, level 0.5) sound underwhelming through Schliere's chain of identical
second-order allpasses with feedback, and which excitation signals would suit this structure
better, at what CPU cost and with which mapping onto Rings' controls?

Method: (1) group delay of the stage, computed analytically on the host from the stage's
bilinear (trapezoidal SVF) transfer function for the S chain (61 stages) and L chain (98);
(2) a host harness that links the **unmodified `rings/dsp/schliere.cc` of sgnir `c9aced5`
(v0.2.0)** and renders candidate excitations through the real `Schliere::Process()` (input
pre-divided by `sqrt(1 - fb^2)` so it enters the chain at the same level as an internal excite;
the firmware impulse and noise burst were rendered through the real `p.excite` path); (3)
primary sources: JOS/CCRMA, JAES/DAFx/EURASIP papers, the pichenettes/eurorack source. Scripts
lived in the session scratchpad, not in the repo. **All numbers below are computed on the host,
not measured on the module.**

Metrics used: *dispersive-band share* = fraction of the excitation's energy at frequencies where
the chain's group delay exceeds 25 % of its maximum; *smeared share* = fraction of the output
energy arriving later than the excitation's own length + 5 ms (i.e. energy the chain moved in
time). Unless stated otherwise: chain S, L tap at detent 7 (node 49), feedback 0, FREQUENCY at
noon (f0 = 632 Hz).

---

## 1. Where the chain's dispersion actually lives

Each stage is `H(s) = (s^2 - k s + 1)/(s^2 + k s + 1)` (k = 1/Q), realised as a trapezoidal SVF
with `allpass = x - 2k * bandpass` [code: `Stage()` in `rings/dsp/schliere.cc`]. Group delay
`D(w) = -dTheta/dw` is the delay of the amplitude envelope of a narrowband component near w
[sourced: https://ccrma.stanford.edu/~jos/filters/Group_Delay.html]. For the analog prototype
the stage delay is `2Q/(pi f0)` at f0 and `1/(pi Q f0)` at DC [hypothesis, own derivation;
confirmed numerically below]. Consequence: the delay peak sits **at f0 only for Q > 1/sqrt(3) ≈
0.58**; below that (the knob's lower half, Q 0.01 … 0.51) the delay is lowpass-shaped and
largest at DC.

Computed on the host (S = 61 stages, full chain):

| f0 | Resonance knob (Q) | Peak delay | Band with delay > 50 % of peak | Delay at 10 kHz |
|---|---|---|---|---|
| 632 Hz | 0.5 (0.51) | 60 ms (at DC) | DC … 658 Hz | 0.3 ms |
| 632 Hz | 0.75 (3.81) | 235 ms | 544 … 710 Hz | 0.04 ms |
| 632 Hz | 1.0 (16) | 982 ms | 613 … 652 Hz | 0.01 ms |
| 100 Hz | 0.5 (0.51) | 378 ms | DC … 105 Hz | 0.04 ms |
| 4 kHz | 0.75 (3.81) | 39 ms | 3.5 … 4.5 kHz | 0.35 ms |

L (98 stages) scales all delays by 98/61 = 1.6. So the impulse response is: an undelayed click
from everything well above f0, then a **downward chirp** (highs first, bass last) at low Q, or
two branches converging onto f0 that ring there at high Q ("boing") [hypothesis, follows from
the curve]. This is the spectral-delay-filter / spring-reverb mechanism: cascades of low-order
allpasses have chirp-like impulse responses and are used to synthesize chirp-like sounds
[sourced: Välimäki, Abel, Smith, "Spectral Delay Filters", JAES 57(7/8), 2009,
https://aes2.org/publications/elibrary-page/?id=14832; Pekonen, Välimäki, Abel, Smith,
"Spectral Delay Filters with Feedback and Time-Varying Coefficients", DAFx-09,
https://dafx.de/paper-archive/details/1M6bLVzGrs4xwzv-QnYKhA]. In a spring, "the dispersivity …
bends the echoed impulses into chirps", and the parametric spring model is exactly long
allpass chains inside feedback loops [sourced: Parker, "Efficient Dispersion Generation
Structures for Spring Reverb Emulation", EURASIP JASP 2011, https://doi.org/10.1155/2011/646134;
Välimäki, Parker, Abel, "Parametric Spring Reverberation Effect", JAES 58(7/8) 547–562, 2010,
https://aes2.org/publications/elibrary-page/?id=15511] — structurally the same as Schliere.

**Verdict on the working hypothesis about the 4 kHz lowpass: refuted for f0 below about
2 kHz.** The dispersion is at and below f0, not in the highs. The lowpass actually *raises* the
burst's share in the dispersive band (23 % vs. 4.6 % for a flat spectrum at noon). It only
removes the dispersive region when FREQUENCY is above about 3 kHz.

## 2. Why the noise burst (and the impulse) sound weak

Computed (S, detent 7, fb 0, f0 632 Hz):

| Excitation (peak 0.5) | Energy in | Dispersive-band share, Q 0.51 / 3.81 | Smeared share, Q 0.51 / 3.81 |
|---|---|---|---|
| Impulse (firmware) | 0.25 | 4.6 % / 1.2 % | 7.9 % / 4.8 % |
| Noise burst (firmware) | 1.39 | 23 % / 10 % | 5.8 % / 13 % |

1. **Most of the energy sits where the chain does nothing** [computed]. 77–99 % of both
   excitations lies above the dispersive band. It leaves the chain almost undelayed, as a click
   (impulse) or a ≈ 10 ms "tsch" (burst), and the chirp behind it carries a few percent of the
   energy. That is the main reason both sound thin. The noise version puts a noisy attack in
   front of a quiet tail.
2. **Random phase blurs the glide only where dispersion is weak** [computed + hypothesis]. A
   burst of length T spreads each instant of the output over a band of width
   `Δf ≈ T / |dτ/df|`. At noon (Q 0.51, f0 632 Hz, node 49), `|dτ/df|` is 30–46 µs/Hz, so a
   3 ms burst gives Δf ≈ 65–100 Hz. That is one to two ERBs (Glasberg & Moore,
   https://doi.org/10.1016/0378-5955(90)90170-T), so the glide becomes a **swept noise band**,
   a "whoosh" rather than a "pew". Where dispersion is strong (Q 3.81 near f0: 1.4 ms/Hz;
   f0 95 Hz: 1.3–2 ms/Hz), Δf is 1–2 Hz, far below an ERB. There the burst still produces a
   clear glide, only with random amplitude ripple. So the working hypothesis ("random phase
   means it just sounds like smeared noise") holds for low Q and mid-to-high f0, but not in
   general.
3. **Envelope**: the burst's level gate (1e-4) makes it ≈ 28 ms long at a 3 ms time constant
   [code: `Excitation()`, `kBurstTime`]. That is comparable to the whole 30–60 ms chirp at
   noon, so the burst and its own smear overlap.

Conclusion: the decisive property is **where the excitation's energy lies relative to the
chain's dispersive band**, and only secondarily its phase coherence.

## 3. Candidate excitations

Computed, same setting (peak 0.5 for every candidate):

| Candidate | Energy in | Disp.-band share Q 0.51 / 3.81 | Smeared share Q 0.51 / 3.81 | Notes |
|---|---|---|---|---|
| Hann pulse 0.1 ms ("hard mallet") | 0.56 | 18 % / 4.8 % | 30 % / 19 % | still mostly click |
| Hann pulse, width 1/(2 f0) | 3.66 | 88 % / 22 % | 99 % / 88 % | the click disappears, the output is all chirp |
| Hann pulse, width 1/f0 | 7.22 | 99 % / 16 % | 100 % / 98 % | +14.6 dB smeared energy vs. impulse; output peak 0.16 (lower than the impulse's 0.24) |
| Doublet +/−, spacing 1/(2 f0) | 0.50 | 5.2 % / 2.3 % | 7 % / 5 % | barely differs from the impulse; only removes DC |
| Gabor grain at f0, 6 cycles | 21.3 | 100 % / 99 % | 100 % / 100 % | pitched "boing"; the best match at high Q |
| Velvet 2000 pulses/s, 20 ms | 1.27 | 5.6 % / 1.4 % | 3.8 % / 2.5 % | broadband pulses: same problem as the impulse |
| Velvet 300 pulses/s, 40 ms / flam of 3 clicks | 0.4–0.6 | ≈ 5 % / ≈ 1 % | 2 % | audible as separate clicks; chirps overlap |
| Exponential sweep 40 Hz → 8 kHz, 30 ms (against the chain's dispersion) | 116 | 70 % / 14 % | — | **compresses**: output peak 0.97 |
| Time-reversed chain IR (exact pre-compensation) | 1.1 | — | 0 % | output is a single click, peak 0.95 |
| Sweep 8 kHz → 40 Hz, 30 ms (with the chain's dispersion) / the IR itself | — | — | — | doubles the smear (like a chain twice as long) |

Notes per family:

- **Shaped pulses / mallet.** Elements' MALLET exciter is a single impulse with
  amplitude-from-table, then an SVF lowpass whose cutoff is set by TIMBRE ("hardness"). PLECTRUM
  is a small negative pre-impulse followed by the main impulse (a doublet). PARTICLES is sparse
  random impulses [code: `elements/dsp/exciter.cc`,
  https://raw.githubusercontent.com/pichenettes/eurorack/master/elements/dsp/exciter.cc]. Rings'
  modal internal exciter is an impulse scaled by `0.25 * SemitonesToRatio(fc^2 * 24) / fc` into
  a lowpass at Q 1.5. Its cutoff **tracks the note**, `frequency * 2^((brightness - 0.5) * 8)`
  [code: `rings/dsp/part.cc` `RenderModalVoice()`, lines 296–320 of the sgnir copy]. That
  tracking is exactly what this chain needs: a pulse width tied to f0 keeps the energy in the
  dispersive band at every FREQUENCY/V/OCT setting.
- **Gabor grain.** A Gaussian-like window has the minimum time-bandwidth product
  [sourced: https://ccrma.stanford.edu/~jos/sasp/Gaussian_Window_Transform.html], so a windowed
  sine at f0 with about 2Q cycles matches the stage bandwidth f0/Q [hypothesis]. At high Q
  that is the only candidate that puts nearly all its energy into the narrow delayed band
  [computed]. It sounds like a pitched, delayed "boing". With feedback it gives a train of
  pitched, progressively smeared echoes [hypothesis].
- **Chirps.** An exponential sweep against the chain's dispersion is pulse compression. It is
  the same principle as swept-sine measurement, where the sweep convolved with its
  time-reversed inverse filter collapses to an impulse [sourced: Farina, AES 108th Conv.
  preprint 5093, 2000, https://aes2.org/publications/elibrary-page/?id=10211]. On the host the
  exact pre-compensation gave a single click. The excitation itself is never heard (the dry
  path carries IN, not the excite [code: `Run()`]). So this flavor **cancels the module's
  character**: musically a curiosity, not a voice. A chirp with the dispersion doubles the
  smear, but a synthetic sweep does not follow FREQUENCY/Q unless its law is matched per
  trigger. An exact match needs a second chain (unaffordable). Not recommended.
- **Velvet noise.** Velvet noise sounds smoother than Gaussian noise from about 2000
  pulses/s [sourced: Välimäki, Lehtonen, Takanen, IEEE TASLP 21(7), 2013,
  https://doi.org/10.1109/TASL.2013.2255281]; the minimum for "smooth" is ≈ 1554 pulses/s
  broadband and drops for lowpassed noise (725 pulses/s at the lowest cutoff tested)
  [sourced: Roberts et al., DAFx-23, https://dafx.de/paper-archive/details/ETsfYrS9mVS9U7g8dK81DQ;
  origin: Järveläinen & Karjalainen, AES 30th Int. Conf. 2007,
  https://aes2.org/publications/elibrary-page/?id=13941]. Each pulse keeps its chirp coherent.
  Below ≈ 300 pulses/s you hear a cluster of separate "drips" (a spring-reverb rattle). But
  ±1 pulses are broadband, so velvet inherits the impulse's energy problem (≈ 5 % in band)
  [computed]. Useful only if each pulse is itself the f0-tracking Hann pulse (a "particles"
  flavor).
- **Injection point.** The excite is added after the `sqrt(1 - fb^2)` input scaling, so its
  level does not depend on feedback [code: `ChainInput()`, `Run()`]. Adding it to `loop_state_`
  instead is the same summing node, scaled by fb (silent at fb 0). Adding it before the loop
  limiter only gets it limited when the loop is hot. Neither is a gain. Injecting mid-chain
  (shorter smear) would add code inside the unrolled, fetch-bound chain (see
  `.claude/rules/firmware.md`). Not worth it. An untested alternative is a one-shot "strike" of
  the stage states (add a value to every stage's integrator state at a block boundary, ≈ 400
  cycles once) [hypothesis, sound unknown].

## 4. Successive triggers: polarity, feedback, round robin

- **Absolute polarity** is audible only in select cases: strongly asymmetric waveforms,
  headphones, worst-case lab conditions [sourced: Greiner & Melton, "Observations on the
  Audibility of Acoustic Polarity", JAES 42(4), 1994,
  https://aes2.org/publications/elibrary-page/?id=6950 (1991 convention version: id=5546);
  Lipshitz, Pocock, Vanderkooy, JAES 30(9), 1982,
  https://aes2.org/publications/elibrary-page/?id=3824 (abstract only, polarity remarks via
  secondary summaries)]. A dispersed chirp is close to symmetric, so alternating polarity of
  isolated hits is practically inaudible [hypothesis].
- **With feedback** [computed: two excites 50–500 ms apart, random spacing, 24 trials,
  output energy in the 500 ms after the second excite relative to the sum of two isolated
  excites]:
  - fb 0: exactly 1.00. The system is linear, so the two responses only add.
  - fb knob 0.8: impulse 0.99–1.01; Hann 1/f0 pulse 0.90–1.10.
  - fb knob 1.0 (0.97, limiter working): impulse 0.96–1.04; Hann pulse 0.69–1.31 (≈ ±1.2 dB).
  - The means for same and alternating polarity are identical (0.98–1.02).

  Whether a new excite adds to or cancels the ring depends on its **timing** relative to the
  loop's phase, not on its sign. Alternating polarity therefore does not systematically damp
  or build up energy. With a clocked sequencer the timing relation repeats, so polarity is one
  of the few cheap levers left to vary the interaction [hypothesis].
- **Round robin.** Sample-based drums cycle or randomize several takes because the identical
  hit repeated sounds "fake" / "inhuman" (the "machine gun effect"); a fixed cycle can itself
  sound artificial, so random choice or slight velocity variation is suggested [sourced:
  https://www.musicradar.com/tuition/tech/how-to-avoid-the-machine-gun-effect-using-round-robin-sampling-632302;
  Ableton round-robin guide,
  https://help.ableton.com/hc/en-us/articles/115000267664-How-to-create-Round-Robin-sample-playback].
  At feedback 0 with no input, Schliere is deterministic. The impulse flavor gives bit-identical
  hits, so the effect applies [code]. The noise burst already varies, because the LCG is not
  reseeded.
- **Recommendation** [hypothesis]: **do not cycle between flavors** per trigger. Flavor is a
  deliberate, saved setting shown on LED 1, and silently changing it would make the module
  unpredictable to play. Instead, add **subtle random micro-variation inside a flavor**:
  - pulse width ±10 %;
  - level ±1 dB;
  - random polarity.

  This is cheap (one LCG step per trigger), stays within one recognisable timbre and breaks
  machine-gun repetition. Check by ear whether it should be always on.

## 5. Cost and control mapping

Budget: 84 000 cycles per block. L peaks at 82 %, so about 15 000 cycles per block are left;
1 % = 35 cycles per sample [code/measured figures from `.claude/rules/firmware.md`]. The
excite only runs while active. Cycle counts below are estimates [hypothesis]; static estimates
have been off by 2x before, so check on the module.

| Candidate | Per-sample cost (Cortex-M4F) | Per-trigger cost | Tables / libm |
|---|---|---|---|
| Impulse | ≈ 1 | — | none |
| Noise burst (current) | ≈ 10–15 (LCG, int→float, one-pole, envelope) | — | none |
| Hann pulse | ≈ 5–8: recursive cosine `c[n] = 2 cos(w) c[n-1] - c[n-2]`, out `0.5 A (1 - c)` | `cos(w)` for `w = 2 pi / N` | none if `N = fs / f0`: `cos(2 pi f0/fs) = (1 - g^2)/(1 + g^2)` from the chain's own `g_` (exact for the trapezoidal tuning) |
| Impulse → 2-pole SVF lowpass (Elements/Rings style) | ≈ 8–10 | cutoff coefficient | `tan` or stmlib's approximations (`set_f_q<FREQUENCY_DIRTY>`) |
| Gabor grain at f0 | ≈ 10–12: sine rotation (2 mul + add) × Hann window (recursive) | none: `cos w0 = (1-g^2)/(1+g^2)`, `sin w0 = 2g/(1+g^2)` | none (`lut_sine`, 5121 entries, exists in `rings/resources.h` as a fallback) |
| Velvet / particles | ≈ 3–5 average (counter; LCG per pulse) | — | none |
| Synthetic sweep | ≈ 15–20 (phase accumulator, exponential increment, sine) | sweep law | `lut_sine` or polynomial |
| Exact pre-compensated or doubled chirp | a second chain (+≈ 30–50 %) | — | not viable |

All candidates are ≤ 0.5 % of the block budget, including on L [hypothesis]. Implementation
note [hypothesis]: render the excitation for the whole block into a 24-sample buffer before
`Run()`, as Rings does with `noise_burst_buffer_` [code: `part.cc`]. The sample loop then only
does `x += excite[i]`, which keeps the unrolled chain's register allocation and code size
untouched. Pulses of up to 10 ms span about 20 blocks, so the generator needs a sample
counter. A retrigger restarts it.

Mapping (no spare knob, no velocity):

- The left-button long press keeps cycling the flavors. LED 1 already uses green and yellow,
  so a third flavor can be **red**, the same three-color vocabulary as dry/wet and the
  variant.
- Instead of a new control, the excitation follows the controls that exist [hypothesis]:
  - pulse width and grain pitch come from FREQUENCY + V/OCT, sampled at the trigger;
  - grain length comes from RESONANCE (≈ 2Q cycles, clamped to 3–16 cycles and ≤ 40 ms).
- With STRUM and IN unpatched, a V/OCT note change already excites [code: sgnir README]. A
  pitch-tracking pulse or grain turns that into a playable voice.

## 6. Ranked recommendation (A/B on hardware)

1. **"Mallet": f0-tracking Hann pulse.** Replace the noise burst with it.
   - Width `N = fs / f0` samples (one period of f0), clamped to 0.1–10 ms (5–480 samples).
   - Peak 0.5 to start, with micro-variation: ±10 % width, ±1 dB level, random polarity.
   - Host result at noon: 99 % of the energy in the dispersive band, +14.6 dB more smeared
     energy than the impulse, at a lower output peak.
   - For A/B, also try a "harder" width `fs / (2 f0)` (88 % in band, a bit of attack).
   - Watch the level across f0. At constant peak the energy scales with width (≈ +8 dB at
     95 Hz vs. 632 Hz), so decide by ear whether to normalise, e.g. amplitude ∝
     sqrt(f0 / 632 Hz), clamped ≤ 0.5.
2. **"Ping": Gabor grain at f0.**
   - Hann-windowed sine at the chain's f0, length max(3, min(2Q, 16)) cycles, ≤ 40 ms,
     peak 0.5.
   - Strongest where the mallet weakens (high Q: 98–100 % in band vs. 16–22 %). Pitched, so it
     makes STRUM + V/OCT melodic.
   - No trig calls: coefficients come from `g_`.
3. **Keep the impulse** as the bright, dry reference flavor (green). It is the only "click"
   and costs nothing.
   - Optional later candidate: "particles", 4–8 f0-tracking Hann pulses at random spacing
     3–15 ms (≈ 100–300 pulses/s) over ≈ 40 ms, as a rattle/drip texture after Elements'
     PARTICLES.
   - Dropped: noise burst, chirps, doublet and plain velvet. Either they add nothing over the
     impulse here, or they work against the chain.

Order of the left-button cycle, proposed: impulse (green) → mallet (yellow) → ping (red).
Decide by ear, especially at mid Q/FREQUENCY, where mallet and ping overlap.

**[open]** All of the above is host computation and literature. The sound on the module, the
level balance across FREQUENCY, and the CPU load on L need a listening/benchmark build.
