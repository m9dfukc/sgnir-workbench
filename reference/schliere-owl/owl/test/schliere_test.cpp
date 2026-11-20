// Host-Test für owl/schliere/ (SchlierePatch.hpp mit SchliereMPatch, SchliereWPatch.hpp, SchliereOPatch.hpp): Übertragungsfunktion aus der Impulsantwort,
// geprüft gegen die Zahlen aus dem Handbuch des Vorbilds und gegen die Rechnung.
#include "SchliereWPatch.hpp" // enthält SchlierePatch.hpp mit SchliereWPatch
#include "SchliereOPatch.hpp"
#include "SchliereAPatch.hpp"
#include "SchliereMPatch.hpp"
#include "SchliereSPatch.hpp"
#include <cmath>
#include <complex>
#include <cstdio>
#include <functional>
static int fails;
static void check(const char* name, bool ok, double a, double b = 0){ printf("%-4s %-66s %10.5f %10.5f\n", ok ? "ok" : "FAIL", name, a, b); if(!ok) fails++; }
static const double PI = 3.14159265358979, FS = 48000;
typedef std::complex<double> cplx;
typedef std::vector<std::vector<float>> Out;

// Lässt den Patch über n Samples laufen (Blöcke zu 8) und gibt alle vier Ausgänge zurück
static Out run(Patch& p, size_t n, std::function<double(int, size_t)> in){
  Out out(4);
  for(size_t t = 0; t < n; t += 8){
    AudioBuffer b; b.ch.assign(4, std::vector<float>(8));
    for(size_t i = 0; i < 8; ++i) for(int c = 0; c < 4; ++c) b.ch[c][i] = (float)in(c, t + i);
    p.processAudio(b);
    for(int c = 0; c < 4; ++c) out[c].insert(out[c].end(), b.ch[c].begin(), b.ch[c].end());
  }
  return out;
}
// Regler: Stellung 0…1 nach Abzug der Totzone am Anschlag
static float raw(double x){ return (float)(0.03 + 0.94 * x); }
static double freqKnob(double hz){ return 0.5 + std::log(hz / 632.4555) / std::log(2000.0); }
static double tapKnob(double point){ return (point - 1.0) / 6.0; } // Rastpunkt 1…7, 4.5 = zwischen 4 und 5
// Feedback: Stellung (Regler + CV) für die Menge fb. Linear bis 0,9, darüber tanh-Knie bis 1, mal 0,97
static double fbAmount(double fb){ const double v = fb / 0.97; return v <= 0.9 ? v : 0.9 + 0.1 * std::atanh((v - 0.9) / 0.1); }
static const double CV_OFFSET = 0.055; // Versatz der Totzone an In 3 und In 4 (0,55 V)
static void set(Patch& p, double hz, double q, double point, double fb){
  p.params[PARAMETER_A] = raw(freqKnob(hz)); p.params[PARAMETER_B] = raw(q);
  p.params[PARAMETER_C] = raw(tapKnob(point)); p.params[PARAMETER_D] = raw(std::fmin(fbAmount(fb), 1.0)); }
static const size_t T0 = 64; // Zeitpunkt des Impulses
// Impulsantwort der Länge n auf In 1, cv = feste Werte an In 2…4
static Out impulse(Patch& p, size_t n, double amp, double cv2 = 0, double cv3 = 0, double cv4 = 0){
  return run(p, T0 + n, [&](int c, size_t t){ return c == 0 ? (t == T0 ? amp : 0.0) : (c == 1 ? cv2 : (c == 2 ? cv3 : cv4)); });
}
static cplx response(const std::vector<float>& y, double f, double amp){
  cplx acc = 0; const cplx step = std::polar(1.0, -2 * PI * f / FS); cplx w = 1;
  for(size_t i = T0; i < y.size(); ++i){ acc += (double)y[i] * w; w *= step; }
  return acc / amp;
}
static double db(double x){ return 20 * std::log10(x + 1e-30); }
static double groupDelay(const std::vector<float>& y, double f, double amp){
  const double df = 0.1; double d = std::arg(response(y, f + df, amp) / response(y, f - df, amp));
  return -d / (2 * PI * 2 * df);
}
// Allpass 2. Ordnung nach der Lehrbuchformel (bilineare Transformation mit Vorverzerrung)
static cplx reference(double f, double f0, double q, int stages){
  const double w0 = 2 * PI * f0 / FS, al = std::sin(w0) / (2 * q); const cplx z = std::polar(1.0, -2 * PI * f / FS);
  const cplx h = ((1 - al) - 2 * std::cos(w0) * z + (1 + al) * z * z) / ((1 + al) - 2 * std::cos(w0) * z + (1 - al) * z * z);
  return std::pow(h, stages);
}
static double peak(const std::vector<float>& v, size_t from = 0){ double m = 0; for(size_t i = from; i < v.size(); ++i) m = std::fmax(m, std::fabs(v[i])); return m; }
static bool finite(const Out& o){ for(auto& c : o) for(float x : c) if(!std::isfinite(x)) return false; return true; }
static int notches(const std::vector<float>& y, double amp, double limitDb){
  int count = 0; const int points = 6000; std::vector<double> m(points);
  for(int i = 0; i < points; ++i) m[i] = db(std::abs(response(y, 20 * std::pow(1000.0, (double)i / (points - 1)), amp)));
  for(int i = 1; i + 1 < points; ++i) if(m[i] < m[i-1] && m[i] <= m[i+1] && m[i] < limitDb) count++;
  return count;
}

// Leistung einer Linie bei f (Blackman-Harris-Fenster über n Samples ab from)
static double line(const std::vector<float>& y, size_t from, size_t n, double f){
  cplx acc = 0; const cplx st = std::polar(1.0, -2 * PI * f / FS); cplx w = 1;
  for(size_t i = 0; i < n; ++i){ const double x = (double)i / (n - 1);
    const double win = 0.35875 - 0.48829 * std::cos(2 * PI * x) + 0.14128 * std::cos(4 * PI * x) - 0.01168 * std::cos(6 * PI * x);
    acc += (double)y[from + i] * win * w; w *= st; }
  return std::norm(acc);
}
// Eingang Sinus 5 kHz, Sinus 441 Hz mit volts an In cvIn (3 = Tap, 4 = Feedback). Gefaltete Linien
// (an fs/2 gespiegelt, nicht auf dem Raster 5000 + k * 441 Hz) gegen alle Linien des Rasters, dB
static double foldedDb(Patch& p, int cvIn, double volts){
  auto o = run(p, 3 * 48000, [&](int c, size_t t){ const double x = (double)t / FS;
    return c == 0 ? 0.3 * std::sin(2 * PI * 5000 * x) : (c == cvIn - 1 ? volts / 10 * std::sin(2 * PI * 441 * x) : 0.0); });
  double legit = 0, alias = 0;
  for(int k = -80; k <= 80; ++k){ const double a = std::fabs(5000 + k * 441.0);
    if(a < FS / 2) legit += line(o[0], 48000, 96000, a); else if(a < FS) alias += line(o[0], 48000, 96000, FS - a); }
  return 10 * std::log10(alias / legit + 1e-30);
}
// Out 2 auf In 4 gesteckt (ein Block Verzögerung), Eingang Sinus 5 kHz mit volts. Gefaltete
// Linien (Vielfache von 1 kHz, die keine von 5 kHz sind) gegen die Oberwellen von 5 kHz, dB
static double selfPatchedDb(Patch& p, double volts){
  std::vector<float> out, prev(8, 0.0f);
  for(size_t t = 0; t < 3 * 48000; t += 8){ AudioBuffer b; b.ch.assign(4, std::vector<float>(8));
    for(size_t i = 0; i < 8; ++i){ b.ch[0][i] = (float)(volts / 10 * std::sin(2 * PI * 5000 * (t + i) / FS)); b.ch[3][i] = prev[i]; }
    p.processAudio(b); prev = b.ch[1]; out.insert(out.end(), b.ch[0].begin(), b.ch[0].end()); }
  double legit = 0, alias = 0;
  for(int m = 1; m < 24; ++m){ const double v = line(out, 48000, 96000, m * 1000.0); if(m % 5 == 0) legit += v; else alias += v; }
  return 10 * std::log10(alias / legit + 1e-30);
}

int main(){
  const double A = 0.25;
  { // eine Stufe: Betrag und Phase wie die Lehrbuchformel, für alle Stützpunkte des Resonance-Reglers
    const double knobs[5] = { 0, 0.25, 0.5, 0.75, 1 }, qs[5] = { 0.01, 0.026, 0.51, 3.81, 16 }; double err = 0;
    for(int i = 0; i < 5; ++i){ SchlierePatch p; set(p, 1000, knobs[i], 1, 0); auto o = impulse(p, 96000, A);
      for(double f : { 300.0, 900.0, 1000.0, 1150.0, 4000.0 }) err = std::fmax(err, std::abs(response(o[0], f, A) - reference(f, 1000, qs[i], 1))); }
    check("one stage matches the textbook allpass for Q 0.01 … 16 (max error)", err < 2e-3, err); }
  { // Rastpunkt 7, kein Feedback: alle Frequenzen gleich laut, an L, R und Tail
    SchlierePatch p; set(p, 1000, 0.5, 7, 0); auto o = impulse(p, 16384, A); double dev = 0;
    for(int c : { 0, 1, 3 }) for(double f : { 30.0, 200.0, 1000.0, 3000.0, 15000.0 }) dev = std::fmax(dev, std::fabs(db(std::abs(response(o[c], f, A)))));
    check("top tap, no feedback: level flat at L, R and tail (max deviation, dB)", dev < 0.01, dev);
    double err = std::abs(response(o[0], 700, A) - reference(700, 1000, 0.51, 49)) + std::abs(response(o[1], 700, A) - reference(700, 1000, 0.51, 61));
    check("  L = 49 stages (98 poles), R = 61 stages (122 poles)", err < 5e-3, err); }
  { // Handbuch: 98 Pole bei 1 kHz, Q 16 -> Gruppenlaufzeit 0,50 s
    SchlierePatch p; set(p, 1000, 1, 7, 0); auto o = impulse(p, 4 * 48000, A); double gd = groupDelay(o[0], 1000, A), tail = groupDelay(o[3], 1000, A);
    check("98 poles at 1 kHz, Q 16: group delay 0.50 s (manual)", std::fabs(gd - 0.501) < 0.005, gd, 0.501);
    check("  tail, 122 poles: 0.50 s * 61 / 49", std::fabs(tail - 0.501 * 61 / 49) < 0.007, tail, 0.501 * 61 / 49); }
  { // Handbuch: 98 Pole bei 400 Hz, Q 16 -> 1,25 s
    SchlierePatch p; set(p, 400, 1, 7, 0); auto o = impulse(p, 10 * 48000, A); double gd = groupDelay(o[0], 400, A);
    check("98 poles at 400 Hz, Q 16: group delay 1.25 s (manual)", std::fabs(gd - 1.249) < 0.012, gd, 1.249); }
  { // Handbuch: zwischen 50 und 66 Polen 8 Kerben. R überblendet dort 62 -> 82 Pole: 10 Kerben
    SchlierePatch p; set(p, 1000, 0.5, 4.5, 0); auto o = impulse(p, 8192, A); int l = notches(o[0], A, -25), r = notches(o[1], A, -25);
    check("halfway between taps 4 and 5: 8 notches at L (manual)", l == 8, l, 8);
    check("  10 notches at R (taps 20 poles apart)", r == 10, r, 10);
    check("  LEDs 1-3 off between two taps", p.params[PARAMETER_AA] == 0 && p.params[PARAMETER_AB] == 0 && p.params[PARAMETER_AC] == 0, p.params[PARAMETER_AA]); }
  { // unterster Rastpunkt: L und R identisch, Diff still
    SchlierePatch p; set(p, 1000, 0.5, 1, 0); unsigned s = 1;
    auto o = run(p, 48000, [&](int c, size_t){ s = s * 1664525u + 1013904223u; return c == 0 ? 0.4 * ((s >> 8) / 8388608.0 - 1.0) : 0.0; });
    double diff = 0; for(size_t i = 0; i < o[0].size(); ++i) diff = std::fmax(diff, std::fabs(o[0][i] - o[1][i]));
    check("bottom tap: L and R identical, diff output silent", diff == 0 && peak(o[2]) == 0, diff, peak(o[2])); }
  { // Regler am Anschlag: 20 Hz und 20 kHz (eine Stufe dreht bei der Mittenfrequenz um 180 Grad)
    SchlierePatch lo; set(lo, 1000, 0.5, 1, 0); lo.params[PARAMETER_A] = 0.02f; auto a = impulse(lo, 96000, A);
    SchlierePatch hi; set(hi, 1000, 0.5, 1, 0); hi.params[PARAMETER_A] = 1.0f; auto b = impulse(hi, 4096, A);
    double e1 = std::abs(response(a[0], 20, A) + 1.0), e2 = std::abs(response(b[0], 20000, A) + 1.0);
    check("frequency knob at the stops: 20 Hz and 20 kHz", e1 < 0.01 && e2 < 0.01, e1, e2); }
  { // In 2: +1 V = eine Oktave
    SchlierePatch p; set(p, 1000, 0.5, 1, 0); auto o = impulse(p, 4096, A, 0.1); double e = std::abs(response(o[0], 2000, A) + 1.0);
    check("frequency CV: +1 V moves 1 kHz to 2 kHz", e < 0.01, e); }
  { // In 3: Tap-CV. Regler 0, CV so, dass Rastpunkt 5 getroffen wird (2 * (cv - 0.055) = 4/6)
    SchlierePatch p; set(p, 1000, 0.5, 1, 0); auto o = impulse(p, 8192, A, 0, CV_OFFSET + 4.0 / 12.0);
    double err = std::abs(response(o[0], 700, A) - reference(700, 1000, 0.51, 33));
    check("tap CV reaches tap 5 (66 poles) with the knob at 0", err < 5e-3, err);
    check("  LEDs show the knob, not the CV: 1 = red, off, off", p.params[PARAMETER_AA] == 1 && p.params[PARAMETER_AB] == 0 && p.params[PARAMETER_AC] == 0, p.params[PARAMETER_AA], p.params[PARAMETER_AC]); }
  { // Tap-CV mit Audio-Rate über Rastpunkte hinweg: LEDs schalten nicht (jeder Wechsel knackt, D73)
    SchlierePatch p; set(p, 1000, 0.5, 4, 0); int changes = 0; float last[3] = { -1, -1, -1 };
    run(p, 48000, [&](int c, size_t t){ if(c == 0 && t % 8 == 0 && t >= 16){ for(int l = 0; l < 3; ++l){ if(last[l] >= 0 && p.params[PARAMETER_AA + l] != last[l]) changes++; last[l] = p.params[PARAMETER_AA + l]; } }
      return c == 2 ? 0.3 * std::sin(2 * PI * 441 * t / FS) : 0.0; });
    check("tap CV 441 Hz +-3 V across taps: rastpunkt LEDs never switch", changes == 0, changes);
    // Hysterese: Regler langsam über den Rand der Haltezone und zurück, mit 0,5 % Zittern
    SchlierePatch q; set(q, 1000, 0.5, 4, 0); changes = 0; float prev = -1;
    for(int step = 0; step < 4000; ++step){ const double pos = 4 + 0.15 * std::sin(PI * step / 4000.0) + 0.005 * ((step * 7919) % 3 - 1);
      q.params[PARAMETER_C] = raw(tapKnob(pos)); AudioBuffer b; b.ch.assign(4, std::vector<float>(8)); q.processAudio(b);
      if(prev >= 0 && q.params[PARAMETER_AA] != prev) changes++; prev = q.params[PARAMETER_AA]; }
    check("  knob jittering at the edge of the hold zone: LED switches at most twice", changes <= 2, changes); }
  { // offene CV-Eingänge mit Offset -0,3 V: Tap und Feedback bleiben unberührt
    SchlierePatch p; set(p, 1000, 0.5, 1, 0); auto o = impulse(p, 8192, A, 0, -0.03, -0.03);
    double err = std::abs(response(o[0], 700, A) - reference(700, 1000, 0.51, 1));
    check("open tap and feedback inputs (-0.3 V offset) change nothing", err < 2e-3 && p.params[PARAMETER_AD] == 0, err); }
  { // Gleichspannung: Vorwärtsweg DC-gekoppelt
    SchlierePatch p; set(p, 1000, 0.5, 7, 0); auto o = run(p, 48000, [](int c, size_t){ return c == 0 ? 0.2 : 0.0; });
    double e = std::fmax(std::fabs(o[0].back() - 0.2), std::fmax(std::fabs(o[1].back() - 0.2), std::fabs(o[3].back() - 0.2)));
    check("DC passes at unity (forward path is DC coupled)", e < 1e-4, e); }
  { // Feedback 90 %: Gleichspannung wird nicht verstärkt (Hochpass in der Schleife)
    SchlierePatch p; set(p, 1000, 0.5, 7, 0.9); auto o = run(p, 4 * 48000, [](int c, size_t){ return c == 0 ? 0.2 : 0.0; });
    const double want = 0.2 * std::sqrt(1 - 0.81);
    check("feedback 90 %: DC is not amplified (0.2 * sqrt(1 - fb^2))", std::fabs(o[0].back() - want) < 1e-3, o[0].back(), want); }
  { // Feedback 90 %, kleines Signal: Spitzen und Täler bei +-12,8 dB, mit negativem Feedback vertauscht
    const double a = 0.001, want = db(std::sqrt(1.9 / 0.1));
    SchlierePatch p; set(p, 1000, 0.5, 7, 0.9); auto o = impulse(p, 4 * 48000, a);
    double hi = -1e9, lo = 1e9, fHi = 0; for(double f = 900; f < 1100; f += 0.25){ double m = db(std::abs(response(o[0], f, a))); if(m > hi){ hi = m; fHi = f; } lo = std::fmin(lo, m); }
    check("feedback 90 %: peaks at +12.8 dB", std::fabs(hi - want) < 0.3, hi, want);
    check("  troughs at -12.8 dB", std::fabs(lo + want) < 0.3, lo, -want);
    SchlierePatch n; set(n, 1000, 0.5, 7, 0); auto on = impulse(n, 4 * 48000, a, 0, 0, -(CV_OFFSET + fbAmount(0.9) / 2));
    double swapped = db(std::abs(response(on[0], fHi, a)));
    check("  negative feedback by CV: the peak becomes a trough", std::fabs(swapped + want) < 0.5, swapped, -want); }
  { // alles am Anschlag, heißes Signal: beschränkt, endlich, Begrenzung in der Schleife zeigt Rot
    SchlierePatch p; set(p, 200, 1, 7, 0.97); unsigned s = 7;
    auto o = run(p, 10 * 48000, [&](int c, size_t t){ s = s * 1664525u + 1013904223u; return c == 0 ? 0.6 * ((t / 218) % 2 ? 1.0 : -1.0) + 0.3 * ((s >> 8) / 8388608.0 - 1.0) : 0.0; });
    double m = std::fmax(std::fmax(peak(o[0]), peak(o[1])), std::fmax(peak(o[2]), peak(o[3])));
    check("9 V square plus noise, feedback and Q at maximum: outputs stay below 9.8 V", finite(o) && m <= 0.98, m, 0.98);
    check("  LED 4 red: the loop limiter works", p.params[PARAMETER_AD] == 1.0f, p.params[PARAMETER_AD]); }
  { // keine Selbstschwingung: nach dem Signal klingt die Schleife aus
    SchlierePatch p; set(p, 1000, 0.5, 7, 0.97);
    auto o = run(p, 14 * 48000, [&](int c, size_t t){ return c == 0 && t < 48000 ? 0.5 * std::sin(2 * PI * 997 * t / FS) : 0.0; });
    double early = peak(std::vector<float>(o[0].begin() + 48000, o[0].begin() + 2 * 48000)), late = peak(o[0], 13 * 48000);
    check("feedback at maximum: the loop rings out, no self-oscillation (dB after 12 s)", db(late / early) < -60, db(late / early), -60); }
  { // Modulation aller CV-Eingänge mit Audio-Rate
    SchlierePatch p; set(p, 800, 0.8, 4, 0.5); unsigned s = 3;
    auto o = run(p, 5 * 48000, [&](int c, size_t t){ s = s * 1664525u + 1013904223u; const double x = 2 * PI * t / FS;
      return c == 0 ? 0.5 * ((s >> 8) / 8388608.0 - 1.0) : (c == 1 ? 0.3 * std::sin(1300 * x) : (c == 2 ? 0.5 * std::sin(900 * x) : 0.5 * std::sin(2100 * x))); });
    double m = std::fmax(std::fmax(peak(o[0]), peak(o[1])), std::fmax(peak(o[2]), peak(o[3])));
    check("audio-rate modulation of frequency, tap and feedback: finite and bounded", finite(o) && m <= 0.98, m, 0.98); }
  { // schneller Sprung der Frequenz bei hohem Q und klingendem Filter: keine Pegelspitze
    SchlierePatch p; set(p, 500, 1, 7, 0);
    auto o = run(p, 3 * 48000, [&](int c, size_t t){ return c == 0 ? 0.25 * std::sin(2 * PI * 500 * t / FS) : (c == 1 ? ((t / 4800) % 2 ? 0.2 : -0.2) : 0.0); });
    check("frequency jumping by 4 octaves at Q 16 while ringing: no level burst", finite(o) && peak(o[0]) < 0.75, peak(o[0]), 0.75); }
  { // Störung im Blocktakt am Eingang (Periode 8 Samples, wie am Gerät): kommt nicht an die Ausgänge
    const double w[8] = { 9e-5, -4e-5, 2e-5, -7e-5, 5e-5, -1e-5, -6e-5, 2e-5 }; unsigned s = 9;
    SchlierePatch p; set(p, 1000, 0.5, 7, 0);
    auto o = run(p, 6 * 48000, [&](int c, size_t t){ s = s * 1664525u + 1013904223u; return c == 0 ? -0.03 + w[t % 8] + 5e-5 * ((s >> 8) / 8388608.0 - 1.0) : 0.0; });
    double worst = 0;
    for(int c : { 0, 1, 3 }){ double r[8] = {}, all = 0; for(size_t t = 4 * 48000; t < 6 * 48000; ++t) r[t % 8] += o[c][t];
      for(double& v : r){ v /= 12000; all += v / 8; } for(double v : r) worst = std::fmax(worst, std::fabs(v - all)); }
    check("block-rate interference at the input (9e-5) is cancelled at L, R and tail", worst < 3e-6, worst, 3e-6);
    check("  the input offset still passes at unity", std::fabs(o[0].back() + 0.03) < 2e-4, o[0].back(), -0.03); }
  { // LEDs nur aus oder rot, nie gedimmt: Gedimmtes Grün läuft über die LED-PWM (2320 Hz) und fiepst
    SchlierePatch p; set(p, 800, 0.8, 1, 0.5); bool dimmed = false; unsigned s = 5; float tap = 0;
    for(size_t t = 0; t < 4 * 48000; t += 8){ AudioBuffer b; b.ch.assign(4, std::vector<float>(8));
      tap = (float)t / (4 * 48000); p.params[PARAMETER_C] = tap; p.params[PARAMETER_D] = 1.0f - tap;
      for(size_t i = 0; i < 8; ++i){ s = s * 1664525u + 1013904223u; b.ch[0][i] = (float)(0.8 * ((s >> 8) / 8388608.0 - 1.0)); }
      p.processAudio(b);
      for(int l = 0; l < 4; ++l){ float v = p.params[PARAMETER_AA + l]; dimmed |= v != 0.0f && v != 1.0f; } }
    check("LED values are only 0 or 1 (never dimmed) over a sweep of tap and feedback", !dimmed, dimmed); }
  { // Fassung 4: Tap-CV mit Audio-Rate faltet kaum noch (Fassung 3: -34 dB)
    SchlierePatch p; set(p, 632.4555, 0.5, 3.52, 0); double d = foldedDb(p, 3, 3);
    check("tap CV 441 Hz +-3 V, 5 kHz in: folded lines (dB, Fassung 3: -34)", d < -75, d, -75); }
  { // Fassung 4: Feedback-CV mit Audio-Rate
    SchlierePatch p; set(p, 632.4555, 0.5, 3.52, 0.485); double d = foldedDb(p, 4, 3);
    check("feedback CV 441 Hz +-3 V, 5 kHz in: folded lines (dB, Fassung 3: -69)", d < -100, d, -100); }
  { // Fassung 4: Out 2 auf In 4 (Nutzer, 2026-11-19: klang nach hartem Clipping)
    SchlierePatch p; set(p, 632.4555, 0.5, 3.52, 0.66); double d = selfPatchedDb(p, 6);
    check("out 2 patched to in 4, feedback 70 %, 6 V in: folded lines (dB, Fassung 3: -21)", d < -55, d, -55); }

  // ---- SchliereMPatch (Morph) ----
  { // ganze Rastpunkte: L = 1 + 8 * (Punkt - 1) Stufen, R = 1 + 10 * (Punkt - 1)
    SchliereMPatch p; set(p, 1000, 0.5, 4, 0); auto o = impulse(p, 16384, A);
    double err = std::abs(response(o[0], 700, A) - reference(700, 1000, 0.51, 25)) + std::abs(response(o[1], 700, A) - reference(700, 1000, 0.51, 31));
    check("morph, tap 4: L = 25 stages, R = 31 stages", err < 5e-3, err); }
  { // gebrochene Länge: weiter ein Allpass, Phase bei Frequency = -pi * p
    SchliereMPatch p; set(p, 1000, 0.5, 4.3, 0); auto o = impulse(p, 32768, A); double dev = 0;
    for(int c : { 0, 1 }) for(double f : { 30.0, 200.0, 700.0, 1000.0, 1500.0, 3000.0, 15000.0 }) dev = std::fmax(dev, std::fabs(db(std::abs(response(o[c], f, A)))));
    check("morph, tap 4.3 (L 27.4, R 34 stages): level flat at L and R (dB)", dev < 0.01, dev);
    // 27 volle Stufen je -pi, die Teilstufe setzt von 0 Hz her ein: -2 pi + 0,4 pi
    const double want = -PI * 27 - 2 * PI + 0.4 * PI, got = std::arg(response(o[0], 1000, A));
    const double e = std::fabs(std::remainder(got - want, 2 * PI));
    check("  phase of L at Frequency = -27 pi + 0.4 pi (partial stage, rad error)", e < 2e-3, e);
    double sumErr = 0, diffErr = 0;
    for(size_t i = 0; i < o[0].size(); ++i){ sumErr = std::fmax(sumErr, std::fabs(o[3][i] - 0.5 * (o[0][i] + o[1][i]))); diffErr = std::fmax(diffErr, std::fabs(o[2][i] - 0.5 * (o[0][i] - o[1][i]))); }
    check("  out 4 = (L + R) / 2, out 3 = (L - R) / 2", sumErr < 1e-6 && diffErr < 1e-6, sumErr, diffErr); }
  { // langsamer Lauf über alle Stufen: kein Einbruch, kein Knacken (die Überblendung hätte hier Kerben)
    SchliereMPatch p; set(p, 1000, 0.5, 1, 0); std::vector<float> out;
    for(size_t t = 0; t < 4 * 48000; t += 8){ AudioBuffer b; b.ch.assign(4, std::vector<float>(8));
      p.params[PARAMETER_C] = raw(std::fmin(1.0, (double)t / (3.5 * 48000)));
      for(size_t i = 0; i < 8; ++i) b.ch[0][i] = (float)(0.25 * std::sin(2 * PI * 1000 * (t + i) / FS));
      p.processAudio(b); out.insert(out.end(), b.ch[0].begin(), b.ch[0].end()); }
    double lo = 1e9, hi = 0;
    for(size_t w = 4800; w + 480 <= out.size(); w += 480){ double m = 0; for(size_t i = w; i < w + 480; ++i) m = std::fmax(m, std::fabs(out[i])); lo = std::fmin(lo, m); hi = std::fmax(hi, m); }
    check("morph, tap swept 1 -> 7 in 3.5 s, 1 kHz: envelope stays within 0.1 dB", db(hi / 0.25) < 0.1 && db(lo / 0.25) > -0.1, db(lo / 0.25), db(hi / 0.25)); }
  { // schnell zurück auf Rastpunkt 1: abgeschaltete Stufen klingen aus, danach L = R
    SchliereMPatch p; set(p, 1000, 0.5, 7, 0); unsigned s = 11;
    auto o = run(p, 48000, [&](int c, size_t t){ if(t == 24000) p.params[PARAMETER_C] = raw(0); s = s * 1664525u + 1013904223u; return c == 0 ? 0.4 * ((s >> 8) / 8388608.0 - 1.0) : 0.0; });
    check("morph, tap jumps 7 -> 1: L = R again after 0.2 s (diff output)", peak(o[2], 24000 + 9600) < 1e-6, peak(o[2], 24000 + 9600)); }
  { // offene Eingänge, Gleichspannung, LEDs
    SchliereMPatch p; set(p, 1000, 0.5, 1, 0); auto o = impulse(p, 8192, A, 0, -0.03, -0.03);
    check("morph: open tap and feedback inputs (-0.3 V) change nothing", std::abs(response(o[0], 700, A) - reference(700, 1000, 0.51, 1)) < 2e-3, std::abs(response(o[0], 700, A) - reference(700, 1000, 0.51, 1)));
    SchliereMPatch q; set(q, 1000, 0.5, 5.6, 0); auto d = run(q, 48000, [](int c, size_t){ return c == 0 ? 0.2 : 0.0; });
    check("morph: DC passes at unity at L, R and sum", std::fabs(d[0].back() - 0.2) < 1e-4 && std::fabs(d[1].back() - 0.2) < 1e-4 && std::fabs(d[3].back() - 0.2) < 1e-4, d[0].back()); }
  { // Tap-CV mit Audio-Rate: Laufzeit-Modulation, kaum Faltung
    SchliereMPatch p; set(p, 632.4555, 0.5, 3.52, 0); double d = foldedDb(p, 3, 3);
    check("morph: tap CV 441 Hz +-3 V, 5 kHz in: folded lines (dB)", d < -55, d, -55); }
  { // alles am Anschlag mit Audio-Rate an allen CV-Eingängen: endlich, beschränkt, LEDs nur 0 oder 1
    SchliereMPatch p; set(p, 200, 1, 4, 0.97); unsigned s = 3; bool dimmed = false;
    auto o = run(p, 5 * 48000, [&](int c, size_t t){ s = s * 1664525u + 1013904223u; const double x = 2 * PI * t / FS;
      for(int l = 0; l < 4; ++l){ float v = p.params[PARAMETER_AA + l]; dimmed |= v != 0.0f && v != 1.0f; }
      return c == 0 ? 0.6 * ((s >> 8) / 8388608.0 - 1.0) : (c == 1 ? 0.3 * std::sin(1300 * x) : (c == 2 ? 0.5 * std::sin(900 * x) : 0.5 * std::sin(2100 * x))); });
    double m = std::fmax(std::fmax(peak(o[0]), peak(o[1])), std::fmax(peak(o[2]), peak(o[3])));
    check("morph: audio-rate CV everywhere, Q and feedback at max: finite, below 9.8 V", finite(o) && m <= 0.98, m, 0.98);
    check("  LED values only 0 or 1", !dimmed, dimmed); }
  { // keine Selbstschwingung
    SchliereMPatch p; set(p, 1000, 0.5, 7, 0.97);
    auto o = run(p, 14 * 48000, [&](int c, size_t t){ return c == 0 && t < 48000 ? 0.5 * std::sin(2 * PI * 997 * t / FS) : 0.0; });
    double early = peak(std::vector<float>(o[0].begin() + 48000, o[0].begin() + 2 * 48000)), late = peak(o[0], 13 * 48000);
    check("morph: feedback at maximum rings out (dB after 12 s)", db(late / early) < -60, db(late / early), -60); }
  // ---- SchliereWPatch (Feedback-Modulation mit 12 kHz) ----
  { // ohne Modulation wie SchlierePatch
    SchliereWPatch w; set(w, 1000, 0.5, 4.5, 0.5); SchlierePatch p; set(p, 1000, 0.5, 4.5, 0.5);
    auto a = impulse(w, 16384, A), b = impulse(p, 16384, A); double d = 0;
    for(int c = 0; c < 4; ++c) for(size_t i = 0; i < a[c].size(); ++i) d = std::fmax(d, std::fabs(a[c][i] - b[c][i]));
    check("wild: without CV identical to Schliere (max difference)", d < 1e-6, d); }
  { // Out 2 auf In 4: mehr Faltung als Fassung 4, weniger als Fassung 3 (-21 dB)
    SchliereWPatch p; set(p, 632.4555, 0.5, 3.52, 0.66); double d = selfPatchedDb(p, 6);
    check("wild: out 2 patched to in 4, feedback 70 %, 6 V in: folded lines (dB, Fassung 3: -21)", d < -25, d, -25); }
  { // alles am Anschlag mit Audio-Rate an allen CV-Eingängen
    SchliereWPatch p; set(p, 200, 1, 4, 0.97); unsigned s = 3;
    auto o = run(p, 5 * 48000, [&](int c, size_t t){ s = s * 1664525u + 1013904223u; const double x = 2 * PI * t / FS;
      return c == 0 ? 0.6 * ((s >> 8) / 8388608.0 - 1.0) : (c == 1 ? 0.3 * std::sin(1300 * x) : (c == 2 ? 0.5 * std::sin(900 * x) : 0.9 * std::sin(2100 * x))); });
    double m = std::fmax(std::fmax(peak(o[0]), peak(o[1])), std::fmax(peak(o[2]), peak(o[3])));
    check("wild: audio-rate CV everywhere, Q and feedback at max: finite, below 9.8 V", finite(o) && m <= 0.98, m, 0.98); }
  { // keine Selbstschwingung
    SchliereWPatch p; set(p, 1000, 0.5, 7, 0.97);
    auto o = run(p, 14 * 48000, [&](int c, size_t t){ return c == 0 && t < 48000 ? 0.5 * std::sin(2 * PI * 997 * t / FS) : 0.0; });
    double early = peak(std::vector<float>(o[0].begin() + 48000, o[0].begin() + 2 * 48000)), late = peak(o[0], 13 * 48000);
    check("wild: feedback at maximum rings out (dB after 12 s)", db(late / early) < -60, db(late / early), -60); }
  // ---- SchliereOPatch (Eingang der Kette mit 96 kHz) ----
  { // Halbband: Spiegelbilder beim Hochrechnen, Faltung beim Heruntertakten
    double image = -1e9, alias = -1e9;
    // Linie bei f in einem Signal mit Rate fs (Blackman-Harris über die zweite Hälfte)
    auto at = [](const std::vector<float>& y, double fs, double f){ const size_t n = y.size() / 2; cplx acc = 0;
      for(size_t i = 0; i < n; ++i){ const double x = (double)i / (n - 1);
        const double w = 0.35875 - 0.48829 * std::cos(2 * PI * x) + 0.14128 * std::cos(4 * PI * x) - 0.01168 * std::cos(6 * PI * x);
        acc += (double)y[n + i] * w * std::polar(1.0, -2 * PI * f * (n + i) / fs); }
      return std::abs(acc); };
    for(double f : { 1000.0, 10000.0, 20000.0 }){ SchliereUp2x u; std::vector<float> y; // 96 kHz, verschachtelt
      for(int i = 0; i < 48000; ++i){ float a, b; u.process((float)std::sin(2 * PI * f * i / FS), a, b); y.push_back(a); y.push_back(b); }
      image = std::fmax(image, db(at(y, 96000, 48000 - f) / at(y, 96000, f))); }
    for(double f : { 30000.0, 40000.0 }){ SchliereDown2x d; std::vector<float> y, ref;
      for(int i = 0; i < 48000; ++i){ y.push_back(d.process((float)std::sin(2 * PI * f * 2 * i / 96000), (float)std::sin(2 * PI * f * (2 * i + 1) / 96000))); ref.push_back((float)std::sin(2 * PI * (48000 - f) * i / FS)); }
      alias = std::fmax(alias, 10 * std::log10(line(y, 24000, 24000, 48000 - f) / line(ref, 24000, 24000, 48000 - f))); }
    check("halfband: images when going up to 96 kHz (dB)", image < -75, image, -75);
    check("halfband: folding when going down from 30/40 kHz (dB)", alias < -75, alias, -75); }
  { // ohne Feedback: Allpass wie SchlierePatch, nur ≈ 3 Samples später
    SchliereOPatch o; set(o, 1000, 0.5, 7, 0); SchlierePatch p; set(p, 1000, 0.5, 7, 0);
    auto a = impulse(o, 16384, A), b = impulse(p, 16384, A); double dev = 0, mag = 0;
    for(double f : { 30.0, 200.0, 1000.0, 3000.0, 15000.0 }){ dev = std::fmax(dev, std::fabs(db(std::abs(response(a[0], f, A))))); mag = std::fmax(mag, std::fabs(db(std::abs(response(a[3], f, A))))); }
    check("oversampled: top tap, no feedback: level flat at L and tail (dB)", dev < 0.01 && mag < 0.01, dev, mag);
    const double extra = groupDelay(a[0], 1000, A) - groupDelay(b[0], 1000, A);
    check("  extra delay at 1 kHz: 2.8 samples (two halfbands)", std::fabs(extra * FS - 2.8) < 0.3, extra * FS, 2.8); }
  { // Out 2 auf In 4: weniger Faltung als Fassung 3 (-21 dB) bei mehr Obertönen
    SchliereOPatch p; set(p, 632.4555, 0.5, 3.52, 0.66); double d = selfPatchedDb(p, 6);
    check("oversampled: out 2 patched to in 4, feedback 70 %, 6 V in: folded lines (dB, Fassung 3: -21)", d < -30, d, -30); }
  { // alles am Anschlag mit Audio-Rate an allen CV-Eingängen
    SchliereOPatch p; set(p, 200, 1, 4, 0.97); unsigned s = 3; bool dimmed = false;
    auto o = run(p, 5 * 48000, [&](int c, size_t t){ s = s * 1664525u + 1013904223u; const double x = 2 * PI * t / FS;
      for(int l = 0; l < 4; ++l){ float v = p.params[PARAMETER_AA + l]; dimmed |= v != 0.0f && v != 1.0f; }
      return c == 0 ? 0.6 * ((s >> 8) / 8388608.0 - 1.0) : (c == 1 ? 0.3 * std::sin(1300 * x) : (c == 2 ? 0.5 * std::sin(900 * x) : 0.9 * std::sin(2100 * x))); });
    double m = std::fmax(std::fmax(peak(o[0]), peak(o[1])), std::fmax(peak(o[2]), peak(o[3])));
    check("oversampled: audio-rate CV everywhere, Q and feedback at max: finite, below 9.8 V", finite(o) && m <= 0.98, m, 0.98);
    check("  LED values only 0 or 1", !dimmed, dimmed); }
  { // keine Selbstschwingung, Gleichspannung läuft durch
    SchliereOPatch p; set(p, 1000, 0.5, 7, 0.97);
    auto o = run(p, 14 * 48000, [&](int c, size_t t){ return c == 0 && t < 48000 ? 0.5 * std::sin(2 * PI * 997 * t / FS) : 0.0; });
    double early = peak(std::vector<float>(o[0].begin() + 48000, o[0].begin() + 2 * 48000)), late = peak(o[0], 13 * 48000);
    check("oversampled: feedback at maximum rings out (dB after 12 s)", db(late / early) < -60, db(late / early), -60);
    SchliereOPatch q; set(q, 1000, 0.5, 7, 0); auto d = run(q, 48000, [](int c, size_t){ return c == 0 ? 0.2 : 0.0; });
    check("oversampled: DC passes at unity", std::fabs(d[0].back() - 0.2) < 1e-4, d[0].back(), 0.2); }
  // ---- SchliereAPatch (Fassung 3, nur umbenannt) ----
  { // Allpass wie das Lehrbuch, Rastpunkt 7
    SchliereAPatch p; set(p, 1000, 0.5, 7, 0); auto o = impulse(p, 16384, A);
    double err = std::abs(response(o[0], 700, A) - reference(700, 1000, 0.51, 49)) + std::abs(response(o[1], 700, A) - reference(700, 1000, 0.51, 61));
    check("Fassung 3 (A): top tap, L = 49 stages, R = 61 stages", err < 5e-3, err); }
  { // Fassung 3 faltet bei Tap-Modulation stark (der Grund für Fassung 4, D80)
    SchliereAPatch p; set(p, 632.4555, 0.5, 3.52, 0); double d = foldedDb(p, 3, 3);
    check("Fassung 3 (A): tap CV 441 Hz +-3 V folds as measured in D80 (-34 dB)", std::fabs(d + 34.1) < 1.0, d, -34.1); }
  // ---- SchliereSPatch (O mit leicht geglättetem Feedback-CV) ----
  { // ohne CV an In 4 wie SchliereO
    SchliereSPatch sp; set(sp, 1000, 0.5, 4.5, 0.5); SchliereOPatch op; set(op, 1000, 0.5, 4.5, 0.5);
    auto a = impulse(sp, 16384, A), b = impulse(op, 16384, A); double d = 0;
    for(int c = 0; c < 4; ++c) for(size_t i = 0; i < a[c].size(); ++i) d = std::fmax(d, std::fabs(a[c][i] - b[c][i]));
    check("sanft: without CV at in 4 identical to SchliereO (max difference)", d == 0, d); }
  { // Feedback-CV bei 3 kHz und 15 kHz: 3 kHz kommt fast ganz durch, 15 kHz gedämpft
    auto rmsDiff = [&](double f){ SchliereSPatch sp; SchliereOPatch op; set(sp, 1000, 0.5, 4, 0.3); set(op, 1000, 0.5, 4, 0.3);
      auto in = [&](int c, size_t t){ return c == 0 ? 0.3 * std::sin(2 * PI * 700 * t / FS) : (c == 3 ? 0.3 * std::sin(2 * PI * f * t / FS) : 0.0); };
      auto a = run(sp, 48000, in), b = run(op, 48000, in);
      SchliereOPatch q; set(q, 1000, 0.5, 4, 0.3); auto r = run(q, 48000, [&](int c, size_t t){ return c == 0 ? 0.3 * std::sin(2 * PI * 700 * t / FS) : 0.0; });
      double ea = 0, eb = 0; for(size_t i = 24000; i < 48000; ++i){ ea += std::pow(a[0][i] - r[0][i], 2); eb += std::pow(b[0][i] - r[0][i], 2); }
      return db(std::sqrt(ea / eb)); };
    const double d3 = rmsDiff(3000), d15 = rmsDiff(15000);
    check("sanft: feedback modulation at 3 kHz passes (dB vs O)", d3 > -2.5 && d3 < 0.5, d3);
    check("  at 15 kHz it is damped (dB vs O)", d15 < -6, d15, -6); }
  { // Out 2 auf In 4
    SchliereSPatch p; set(p, 632.4555, 0.5, 3.52, 0.66); double d = selfPatchedDb(p, 6);
    check("sanft: out 2 patched to in 4, feedback 70 %, 6 V in: folded lines (dB, O: -34.4)", d < -30, d, -30); }
  { // alles am Anschlag mit Audio-Rate
    SchliereSPatch p; set(p, 200, 1, 4, 0.97); unsigned s = 3;
    auto o = run(p, 5 * 48000, [&](int c, size_t t){ s = s * 1664525u + 1013904223u; const double x = 2 * PI * t / FS;
      return c == 0 ? 0.6 * ((s >> 8) / 8388608.0 - 1.0) : (c == 1 ? 0.3 * std::sin(1300 * x) : (c == 2 ? 0.5 * std::sin(900 * x) : 0.9 * std::sin(2100 * x))); });
    double m = std::fmax(std::fmax(peak(o[0]), peak(o[1])), std::fmax(peak(o[2]), peak(o[3])));
    check("sanft: audio-rate CV everywhere, Q and feedback at max: finite, below 9.8 V", finite(o) && m <= 0.98, m, 0.98); }
  printf("%s\n", fails ? "FAILURES" : "all cases ok"); return fails;
}
