#ifndef __SCHLIERE_O_PATCH_HPP__
#define __SCHLIERE_O_PATCH_HPP__

/* Schliere O: Überblendung wie SchlierePatch (Fassung 4), aber der Eingang der Kette wird mit
 * doppelter Abtastrate (96 kHz) gerechnet. Wunsch des Nutzers (2026-11-19): Fassung 3 hatte
 * ein wildes Feedback, Fassung 4 klingt zu zahm.
 *
 * Die Feedback-Menge multipliziert das Schleifensignal. Bei Fassung 3 geschah das ungeglättet
 * mit 48 kHz: Die Summenfrequenzen über 24 kHz falteten zurück, dazu schnitt eine harte Grenze
 * die Feedback-Menge ab. Fassung 4 glättet die Feedback-Menge (1,5 kHz) und nimmt damit auch
 * die neuen Obertöne weg, SchliereW (12 kHz) liegt dazwischen (messlog D80).
 *
 * Hier wird die Feedback-Menge nicht geglättet (bis auf die Regler-Glättung). Eingang (nach
 * der Blockraten-Sperre), Feedback-CV (In 4) und Schleifensignal werden auf 96 kHz
 * hochgerechnet, dort laufen Totzone, weiches Knie und beide Produkte (√(1 − fb²) · Eingang
 * und fb · Schleife), danach geht es zurück auf 48 kHz. Was bei 48 kHz über fs/2 läge, liegt
 * bei 96 kHz noch unter fs/2 und fällt im Tiefpass vor dem Heruntertakten weg.
 *
 * Filter: Polyphasen-IIR-Halbband, zwei Zweige aus Allpässen 1. Ordnung (Entwurf nach
 * Valenzuela/Constantinides, elliptisch, sechs Koeffizienten, Übergang 0,045). Durchlass bis
 * 20 kHz ohne Welligkeit, Sperre ab 28 kHz mindestens 77 dB. Die IIR-Form ist gewählt, weil
 * sie wenig verzögert: je Richtung ≈ 1,4 Samples bei 48 kHz (1 kHz), 1,7 (10 kHz), 4,5
 * (20 kHz). Die Schleife wird dadurch um ≈ 3 Samples länger als bei SchlierePatch, die
 * Spitzen des Feedbacks liegen deshalb etwas anders (bei 1 kHz ≈ 0,4 rad mehr Phase).
 *
 * Alles andere (Kette, Abgriffe, Tap-Glättung, Ausgänge, LEDs) wie SchlierePatch. Die
 * Überblendung ist hier abgeschrieben statt geerbt: Schon eine abgeleitete Klasse ändert das
 * Binary von SchlierePatch, und das soll bytegleich mit dem Build auf dem Modul bleiben.
 */

#include "SchlierePatch.hpp"

/* Halbband-Filter 2×: zwei Zweige aus Allpässen 1. Ordnung, jeder bei der niedrigen Rate.
 * H(z) = ½ (A0(z²) + z⁻¹ A1(z²)), A0 mit den Koeffizienten 0, 2, 4, A1 mit 1, 3, 5. */
class SchliereHalfband {
  static constexpr int N = 6;
  float xs[N] = {}, ys[N] = {};
protected:
  float branch(float x, int first){
    static const float coef[N] = { 0.06396838f, 0.22734146f, 0.42965512f,
                                   0.62193724f, 0.78589107f, 0.92904445f };
    for(int i = first; i < N; i += 2){
      const float y = coef[i] * (x - ys[i]) + xs[i];
      xs[i] = x;
      ys[i] = y;
      x = y;
    }
    return x;
  }
};
/* hochrechnen: ein Sample bei 48 kHz → zwei bei 96 kHz (a zuerst) */
class SchliereUp2x : public SchliereHalfband {
public:
  void process(float x, float& a, float& b){
    a = branch(x, 0);
    b = branch(x, 1);
  }
};
/* heruntertakten: zwei Samples bei 96 kHz → eins bei 48 kHz */
class SchliereDown2x : public SchliereHalfband {
  float odd = 0.0f;
public:
  float process(float a, float b){
    const float y = 0.5f * (branch(a, 0) + branch(odd, 1));
    odd = b;
    return y;
  }
};

class SchliereOPatch : public SchliereBase {
  float state[2 * STAGES] = {};
  float node[STAGES + 1] = {};
  float w1[TAPS] = {}, w2[TAPS] = {};
  SchliereUp2x upIn, upCv, upLoop;
  SchliereDown2x down;

  /* Eingang der Kette mit 96 kHz: Sperre, dann Feedback (In 4 und Regler 4) */
  float chainInput2x(float in1, float cv4){
    fbKnob += (targetFb - fbKnob) * slewSample;
    float in[2], cv[2], lp[2], x[2];
    upIn.process(inputClean.process(in1), in[0], in[1]);
    upCv.process(cv4, cv[0], cv[1]);
    upLoop.process(loopState, lp[0], lp[1]);
    float fb = 0.0f;
    for(int h = 0; h < 2; ++h){
      fb = FB_MAX * soft(fbKnob + CV_SCALE * deadzone(cv[h]), FB_KNEE, 1.0f);
      x[h] = (sqrtf)(1.0f - fb * fb) * in[h] + fb * lp[h];
    }
    fbNow = fb;
    return down.process(x[0], x[1]);
  }

public:
  SchliereOPatch(){
    static_assert(STAGES >= 61, "SCHLIERE_STAGES muss mindestens 61 sein");
  }

  void processAudio(AudioBuffer& buffer){
    if(channels < 4)
      return; // nur für das AC/DC gedacht
    static const uint8_t tapsL[TAPS] = { 1, 9, 17, 25, 33, 41, 49 };
    static const uint8_t tapsR[TAPS] = { 1, 11, 21, 31, 41, 51, 61 };
    const size_t size = buffer.getSize();
    FloatArray io[4] = { buffer.getSamples(0), buffer.getSamples(1),
                         buffer.getSamples(2), buffer.getSamples(3) };
    bool first = beginBlock(io, size);

    for(size_t i = 0; i < size; ++i){
      stepCoefficients();
      float x = chainInput2x(io[0][i], io[3][i]);

      // die Kette (wie SchlierePatch)
      node[0] = x;
      float* s = state;
      for(size_t n = 1; n <= STAGES; ++n, s += 2){
        x = stage(x, s, a1, a2, g, c);
        node[n] = x;
      }

      // Abgriffe (wie SchlierePatch): Haltezone, S-Kurve, Gewichte über zwei Tiefpässe
      const float u = tapPosition(io[2][i]);
      const int index = u >= (float)(TAPS - 1) ? (int)TAPS - 2 : (int)u;
      float t = clamp((u - index - HOLD) / (1.0f - 2.0f * HOLD), 0.0f, 1.0f);
      t = t * t * (3.0f - 2.0f * t);
      float left = 0.0f, right = 0.0f;
      for(int j = 0; j < (int)TAPS; ++j){
        const float target = j == index ? 1.0f - t : (j == index + 1 ? t : 0.0f);
        if(first)
          w1[j] = w2[j] = target;
        const float w = lowpass(lowpass(target, w1[j], modG), w2[j], modG);
        left += w * node[tapsL[j]];
        right += w * node[tapsR[j]];
      }
      first = false;

      outputs(io, i, left, right, 0.5f * (left - right), node[STAGES]);
    }

    if(resetIfBroken(io, size, node[STAGES])){
      for(size_t n = 0; n < 2 * STAGES; ++n) state[n] = 0.0f;
      for(size_t n = 0; n <= STAGES; ++n) node[n] = 0.0f;
      upIn = SchliereUp2x(); upCv = SchliereUp2x(); upLoop = SchliereUp2x(); down = SchliereDown2x();
    }
    leds(size);
  }
};

#endif // __SCHLIERE_O_PATCH_HPP__
