# Schliere equivalence test

Compares the Rings port (`firmware/eurorack-mi-sgnir/rings/dsp/schliere.cc`) with the OWL
reference (`reference/schliere-owl/owl/schliere/`): same input, same knob settings, constant
CVs, variants S, O and Z.

```bash
c++ -std=c++17 -O1 -DTEST \
    -Ireference/schliere-owl/owl/test -Ireference/schliere-owl/owl/schliere \
    -Ifirmware/eurorack-mi-sgnir \
    -o "$TMPDIR/schliere_equiv" tests/schliere/equivalence_test.cpp \
    firmware/eurorack-mi-sgnir/rings/dsp/schliere.cc
"$TMPDIR/schliere_equiv"
```

Result 2026-11-20 [measured, host]: all 32 cases ok.

- Linear range (impulse and noise, knees not reached): max. difference ≤ 2.4e-6, i.e. float
  rounding. Most cases are bit-identical.
- Soft knees involved (feedback above 90 %, hot input): max. difference 1.1e-2, within the
  SoftClip-vs-tanh tolerance (ANLEITUNG.md section 4.3).
- O/S with a feedback CV: compared after 0.5 s. The reference upsamples the CV in volts and
  applies its dead zone at 96 kHz, the port upsamples the CV in knob units; the start-up
  transients differ (5.7e-4 over the whole run, 2.6e-7 after 0.5 s).

The port's own host tests (43 cases, ported from the reference suite plus spread, dry/wet,
excite and detent display) live in the firmware repo: `rings/test/schliere_test.cc`.
