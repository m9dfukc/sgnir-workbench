// Equivalence test: the Rings port of Schliere (firmware/eurorack-mi-sgnir,
// rings/dsp/schliere.cc) against the OWL reference (reference/schliere-owl).
//
// Same input, same knob settings, constant CVs -> the outputs must match up
// to float rounding. Settings that drive a soft knee (feedback above 90 %,
// levels above the output or loop knee) may differ by the SoftClip-vs-tanh
// approximation (ANLEITUNG.md section 4.3) and are checked with a looser bound.
//
// The reference runs in blocks of 8 with four outputs, the port in blocks of
// 24 with two (L, R at spread 0.5 = the reference's L/R layout). The
// reference's input canceller (AcdcInput.h) is transparent for these inputs:
// it only learns in silence and only removes components with period 8.
//
// Build and run: see tests/schliere/README.md.

#include "SchlierePatch.hpp"
#include "SchliereOPatch.hpp"
#include "SchliereSPatch.hpp"

#include "rings/dsp/schliere.h"

#include <cmath>
#include <cstdio>
#include <functional>
#include <vector>

static int fails;

static void check(const char* name, bool ok, double a, double b = 0) {
  printf("%-4s %-70s %12.3e %12.3e\n", ok ? "ok" : "FAIL", name, a, b);
  if (!ok) fails++;
}

static const double PI = 3.14159265358979, FS = 48000;
static const size_t kRingsBlock = 24, kOwlBlock = 8;

// Knob position as the OWL patch reads it (dead band at the stops).
static float raw(double x) { return (float)(0.03 + 0.94 * x); }
// The OWL dead zone of the CV inputs, in knob units (CV_SCALE * deadzone).
static double cvKnobUnits(double cv) {
  const double lo = 0.045, hi = 0.065, a = std::fabs(cv);
  double d;
  if (a <= lo) return 0.0;
  if (a >= hi) d = a - 0.5 * (lo + hi);
  else { const double w = hi - lo, y = (a - lo) / w; d = w * y * y * y * (1.0 - 0.5 * y); }
  return 2.0 * (cv < 0 ? -d : d);
}

struct Setting {
  const char* name;
  double frequency, resonance, tap, feedback;  // knob positions 0..1
  double cv2, cv3, cv4;  // reference CV inputs (1.0 = 10 V)
  double tolerance;
};

typedef std::function<double(size_t)> Signal;

struct Outputs { std::vector<float> l, r; };

template<typename P>
static Outputs runReference(const Setting& s, size_t n, Signal in) {
  P p;
  p.params[PARAMETER_A] = raw(s.frequency);
  p.params[PARAMETER_B] = raw(s.resonance);
  p.params[PARAMETER_C] = raw(s.tap);
  p.params[PARAMETER_D] = raw(s.feedback);
  Outputs o;
  for (size_t t = 0; t < n; t += kOwlBlock) {
    AudioBuffer b;
    b.ch.assign(4, std::vector<float>(kOwlBlock));
    for (size_t i = 0; i < kOwlBlock; ++i) {
      b.ch[0][i] = (float)in(t + i);
      b.ch[1][i] = (float)s.cv2;
      b.ch[2][i] = (float)s.cv3;
      b.ch[3][i] = (float)s.cv4;
    }
    p.processAudio(b);
    o.l.insert(o.l.end(), b.ch[0].begin(), b.ch[0].end());
    o.r.insert(o.r.end(), b.ch[1].begin(), b.ch[1].end());
  }
  return o;
}

static Outputs runPort(rings::SchliereVariant variant, const Setting& s,
                       size_t n, Signal in) {
  static rings::Schliere schliere;
  schliere.Init(variant);
  rings::SchliereParameters p;
  p.frequency = (float)s.frequency;
  p.octaves = (float)(10.0 * s.cv2);
  p.resonance = (float)s.resonance;
  p.tap = (float)s.tap;
  p.tap_cv = (float)cvKnobUnits(s.cv3);
  p.feedback = (float)s.feedback;
  p.feedback_cv = (float)cvKnobUnits(s.cv4);
  p.spread = 0.5f;
  p.mix = 1.0f;
  p.excite = false;
  p.excite_type = rings::SCHLIERE_EXCITE_IMPULSE;
  Outputs o;
  std::vector<float> buffer(kRingsBlock), l(kRingsBlock), r(kRingsBlock);
  for (size_t t = 0; t < n; t += kRingsBlock) {
    for (size_t i = 0; i < kRingsBlock; ++i) buffer[i] = (float)in(t + i);
    schliere.Process(p, buffer.data(), l.data(), r.data(), kRingsBlock);
    o.l.insert(o.l.end(), l.begin(), l.end());
    o.r.insert(o.r.end(), r.begin(), r.end());
  }
  return o;
}

static double maxDiff(const Outputs& a, const Outputs& b, size_t n,
                      size_t from = 0) {
  double d = 0;
  for (size_t i = from; i < n; ++i) {
    d = std::fmax(d, std::fabs(a.l[i] - b.l[i]));
    d = std::fmax(d, std::fabs(a.r[i] - b.r[i]));
  }
  return d;
}

static double peak(const Outputs& a, size_t n) {
  double m = 0;
  for (size_t i = 0; i < n; ++i) m = std::fmax(m, std::fmax(std::fabs(a.l[i]), std::fabs(a.r[i])));
  return m;
}

template<typename P>
static void compare(rings::SchliereVariant variant, const char* label,
                    const Setting& s, size_t n, Signal in) {
  // n is a multiple of 24 (and therefore of 8)
  Outputs ref = runReference<P>(s, n, in);
  Outputs port = runPort(variant, s, n, in);
  // O and S upsample the feedback CV to 96 kHz. The reference upsamples the
  // CV in volts and applies its dead zone afterwards, the port upsamples the
  // CV in knob units (no dead zone on Rings). For a constant CV both settle
  // to the same value, but the start-up transients differ and the loop
  // carries them for a while: compare after 0.5 s (measured: 5.7e-4 over the
  // whole run, 2.6e-7 after 0.5 s).
  const bool oversampled = variant == rings::SCHLIERE_VARIANT_S ||
      variant == rings::SCHLIERE_VARIANT_O;
  const size_t from = oversampled && s.cv4 != 0 ? 24000 : 0;
  char name[160];
  snprintf(name, sizeof(name), "%s: %s%s (max |diff|, peak)", label, s.name,
           from ? ", after 0.5 s" : "");
  const double d = maxDiff(ref, port, n, from);
  check(name, d <= s.tolerance, d, peak(ref, n));
}

int main() {
  const size_t T0 = 72;
  const size_t n = 4 * 48000;  // 4 s, multiple of 24
  Signal impulse = [&](size_t t) { return t == T0 ? 0.25 : 0.0; };
  Signal noise = [](size_t t) {
    unsigned s = (unsigned)t * 2654435761u + 12345u;
    s ^= s >> 13; s *= 1274126177u; s ^= s >> 16;
    return 0.1 * ((s >> 8) / 8388608.0 - 1.0);
  };
  Signal sine = [](size_t t) { return 0.3 * std::sin(2 * PI * 997.0 * t / FS); };
  Signal hot = [](size_t t) { return 0.6 * ((t / 218) % 2 ? 1.0 : -1.0); };

  // Linear range: float rounding only.
  const double kExact = 1e-5;
  // Soft knees involved: SoftClip vs tanh (max deviation 0.024 of the knee
  // range, i.e. 0.0024 to 0.0058 absolute; feedback adds to it).
  const double kKnee = 0.03;

  const Setting linear[] = {
    { "1 kHz, Q 0.51, tap 7, no feedback", 0.5 + std::log(1000 / 632.4555) / std::log(2000.0), 0.5, 1.0, 0.0, 0, 0, 0, kExact },
    { "200 Hz, Q 16, tap 4, no feedback", 0.5 + std::log(200 / 632.4555) / std::log(2000.0), 1.0, 0.5, 0.0, 0, 0, 0, kExact },
    { "between taps 4 and 5, feedback 50 %", 0.5, 0.5, 3.5 / 6.0, 0.5, 0, 0, 0, kExact },
    { "tap 2.3, feedback 80 %, Q 3.8", 0.6, 0.75, 1.3 / 6.0, 0.8, 0, 0, 0, kExact },
    { "frequency CV +1 V", 0.5, 0.5, 1.0, 0.3, 0.1, 0, 0, kExact },
    { "tap CV to tap 5, knob at 0", 0.5, 0.5, 0.0, 0.3, 0, 0.055 + 4.0 / 12.0, 0, kExact },
    { "negative feedback by CV (-60 %)", 0.5, 0.5, 1.0, 0.0, 0, 0, -(0.055 + 0.3), kExact },
  };
  const Setting knee[] = {
    { "feedback knob 100 % (soft knee)", 0.5, 0.5, 1.0, 1.0, 0, 0, 0, kKnee },
    { "feedback 97 % with CV, Q 16, 200 Hz", 0.5 + std::log(200 / 632.4555) / std::log(2000.0), 1.0, 1.0, 1.0, 0, 0, 0.2, kKnee },
  };

  for (const Setting& s : linear) {
    compare<SchliereSPatch>(rings::SCHLIERE_VARIANT_S, "S impulse", s, 36000, impulse);
    compare<SchliereSPatch>(rings::SCHLIERE_VARIANT_S, "S noise", s, 48000, noise);
  }
  for (const Setting& s : linear) {
    compare<SchliereOPatch>(rings::SCHLIERE_VARIANT_O, "O noise", s, 48000, noise);
    compare<SchlierePatch>(rings::SCHLIERE_VARIANT_Z, "Z noise", s, 48000, noise);
  }
  for (const Setting& s : knee) {
    compare<SchliereSPatch>(rings::SCHLIERE_VARIANT_S, "S sine", s, n, sine);
    compare<SchliereSPatch>(rings::SCHLIERE_VARIANT_S, "S hot square", s, n, hot);
  }

  printf("%s\n", fails ? "FAILURES" : "all cases ok");
  return fails;
}
