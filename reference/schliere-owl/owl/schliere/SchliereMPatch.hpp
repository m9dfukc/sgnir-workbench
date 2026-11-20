#ifndef __SCHLIERE_M_PATCH_HPP__
#define __SCHLIERE_M_PATCH_HPP__

/* SchliereM (2026-11-19): Morph-Variante von Schliere. Tap überblendet nicht, sondern
 * ändert die Länge der Kette stufenlos. Die Kette bleibt in jeder Stellung ein reiner
 * Allpass, Tap-Modulation verschiebt nur die Laufzeit: langsam ein Vibrato, schnell eine
 * FM-artige Klangfarbe. Kerben gibt es an Out 4 (Summe) und über Feedback, und sie wandern
 * mit dem Tap, statt nur ein- und auszublenden. Dem Nutzer zu zahm (2026-11-19), deshalb
 * nicht mehr auf dem Modul. Messung: messlog D80.
 *
 * Belegung wie SchlierePatch (SchlierePatch.hpp), außer:
 *   Regler 3   Länge der Kette, stufenlos von 2 bis 98 Polen (L) bzw. 2 bis 122 Polen (R),
 *              ohne Haltezonen. Die Tap-Stellung läuft über dieselben zwei Tiefpässe wie
 *              die Gewichte in SchlierePatch.
 *   Out 1, 2   L 2 … 98 Pole, R 2 … 122 Pole stufenlos, an den Rastpunkten wie SchlierePatch
 *   Out 4      Summe = (L + R) / 2. R ist um ein Viertel länger als L, die Summe hat deshalb
 *              Kerben, deren Zahl mit dem Tap wächst. Weil sich die Länge stufenlos ändert,
 *              wandern sie dabei (ein Phaser).
 *
 * Morph-Stufe (SchliereMPatch): Bei einer gebrochenen Länge p laufen die Stufen
 * 1 … ⌊p⌋ voll, Stufe ⌊p⌋ + 1 läuft mit einer Eckfrequenz zwischen 0 Hz und Frequency.
 * Bei 0 Hz (g = 0) und leerem Zustand ist eine Stufe exakt durchlässig, deshalb müssen die
 * Stufen dahinter nicht gerechnet werden. Die Eckfrequenz der Teilstufe ist so gewählt,
 * dass sich ihre Phase bei Frequency genau linear mit dem Bruchteil ändert, von −2π (bei
 * 0 Hz, gleichwertig mit 0) bis −π (Lehrbuchformel nach g' aufgelöst):
 * g' = 2g·sin α / (k·cos α + √(k²cos²α + 4 sin²α)), α = π/2 · Bruchteil. An den ganzen Stufen
 * gibt es bei Frequency damit keinen Knick. Weil die Teilstufe von unten einsetzt, dreht
 * die Phase während des Einsetzens vorwärts: Ein langsam steigender Tap hebt die Tonhöhe
 * kurz an (bei Frequency und darunter am stärksten), ein fallender senkt sie. Das ist
 * eine Laufzeit-Modulation wie beim Vibrato, nur je Frequenz verschieden stark. Fällt
 * eine Stufe aus der Kette (g = 0), klingt ihr Rest im Zustand in 5 ms aus. L und R
 * brauchen hier je eine eigene Kette (bis zu 49 + 61 Stufen statt 61), die Rechenlast ist
 * entsprechend höher.
 */

#include "SchlierePatch.hpp"

/* Morph: Tap ändert die Länge der Kette stufenlos, L und R mit je eigener Kette */
class SchliereMPatch : public SchliereBase {
  static constexpr float LEAK_S = 0.005f;   // Rest einer abgeschalteten Stufe klingt so aus
  static constexpr float ZERO = 1e-9f;      // darunter gilt ein Zustand als leer
  /* Darunter ist die Teilstufe aus. Eine Teilstufe mit sehr kleinem Bruchteil hat eine
   * Eckfrequenz unter 1 Hz und braucht Sekunden, bis Gleichspannung durch ist: Der Ausgang
   * kriecht. Aus heißt g = 0, der Rest im Zustand klingt in 5 ms aus. */
  static constexpr float FRAC_MIN = 0.002f;
  static constexpr float STEP_L = 8.0f;     // Stufen je Rastpunkt: L 1 … 49
  static constexpr float STEP_R = 10.0f;    //                      R 1 … 61
  float stateL[2 * STAGES] = {}, stateR[2 * STAGES] = {};
  size_t topL = 1, topR = 1;   // Stufen 1 … top haben Zustand (top ≥ 1)
  float u1 = 0.0f, u2 = 0.0f;  // Zustand der beiden Tiefpässe für die Tap-Stellung
  float leak;

  /* eine Kette mit p Stufen, 1 ≤ p ≤ STAGES, p gebrochen */
  float chain(float x, float* state, size_t& top, float p){
    const size_t whole = (size_t)p;
    const float frac = p - (float)whole;
    float* s = state;
    size_t n = 1;
    for(; n <= whole; ++n, s += 2)
      x = stage(x, s, a1, a2, g, c);
    size_t used = whole;
    if(frac > FRAC_MIN && whole < STAGES){
      // Teilstufe: Phase bei Frequency linear in frac, von −2π (≡ 0) bis −π
      const float alpha = 1.5707963f * frac;
      const float sn = sinf(alpha), cs = cosf(alpha);
      const float kc = k * cs;
      const float gp = 2.0f * g * sn / (kc + (sqrtf)(kc * kc + 4.0f * sn * sn));
      const float pa1 = 1.0f / (1.0f + gp * (gp + k));
      x = stage(x, s, pa1, gp * pa1, gp, c);
      ++n; s += 2; used = whole + 1;
    }
    // abgeschaltete Stufen (g = 0, durchlässig bis auf den Rest im Zustand)
    for(; n <= top; ++n, s += 2){
      x -= c * s[0];
      s[0] *= leak;
      s[1] *= leak;
      if(fabsf(s[0]) < ZERO && fabsf(s[1]) < ZERO)
        s[0] = s[1] = 0.0f;
    }
    if(used > top)
      top = used;
    while(top > used && top > 1 && state[2 * (top - 1)] == 0.0f && state[2 * (top - 1) + 1] == 0.0f)
      --top;
    return x;
  }

public:
  SchliereMPatch(){
    static_assert(STAGES >= 61, "SCHLIERE_STAGES muss mindestens 61 sein");
    leak = (expf)(-1.0f / (LEAK_S * sampleRate));
  }

  void processAudio(AudioBuffer& buffer){
    if(channels < 4)
      return; // nur für das AC/DC gedacht
    const size_t size = buffer.getSize();
    FloatArray io[4] = { buffer.getSamples(0), buffer.getSamples(1),
                         buffer.getSamples(2), buffer.getSamples(3) };
    const bool first = beginBlock(io, size);

    float last = 0.0f;
    for(size_t i = 0; i < size; ++i){
      stepCoefficients();
      const float x = chainInput(io[0][i], io[3][i]);

      // Tap-Stellung über zwei Tiefpässe: Die Modulation der Länge ist bandbegrenzt
      const float u = tapPosition(io[2][i]);
      if(first && i == 0)
        u1 = u2 = u;
      const float us = clamp(lowpass(lowpass(u, u1, modG), u2, modG), 0.0f, (float)(TAPS - 1));

      const float left = chain(x, stateL, topL, 1.0f + STEP_L * us);
      const float right = chain(x, stateR, topR, 1.0f + STEP_R * us);
      outputs(io, i, left, right, 0.5f * (left - right), 0.5f * (left + right));
      last = left + right;
    }

    if(resetIfBroken(io, size, last)){
      for(size_t n = 0; n < 2 * STAGES; ++n) stateL[n] = stateR[n] = 0.0f;
      topL = topR = 1;
    }
    leds(size);
  }
};

#endif // __SCHLIERE_M_PATCH_HPP__
