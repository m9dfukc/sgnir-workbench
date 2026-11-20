#ifndef __SCHLIERE_A_PATCH_HPP__
#define __SCHLIERE_A_PATCH_HPP__

/* SchliereA = Schliere Fassung 3 (2026-11-18, messlog D70/D71), unverändert bis auf den
 * Klassennamen. Gebaut als „SchliereA“ bytegleich mit dem Build auf Slot 6. Übersicht über
 * alle Varianten: README.md in diesem Ordner.
 *
 * Schliere: lange Allpass-Kette für das AC/DC, nach dem Vorbild eines digitalen
 * Eurorack-Moduls mit 98 Allpass-Polen je Sektion (Nutzer-Wunsch 2026-11-18). Nachgebaut
 * ist eine Sektion: mono hinein, stereo heraus, dazu zwei abgeleitete Signale.
 *
 * Prinzip: 61 gleiche Allpässe 2. Ordnung in Reihe (1 Stufe = 2 Pole). Jede Stufe lässt
 * alle Frequenzen gleich laut durch und verzögert sie unterschiedlich lang. In der Summe
 * wird daraus eine hörbare, frequenzabhängige Laufzeit: Transienten verschmieren zu
 * einem Zwitschern, mit Feedback entsteht ein Resonator.
 *
 *   Regler 1   Frequency  20 Hz … 20 kHz: wo die Laufzeit wirkt
 *   Regler 2   Resonance  Q 0,01 … 16: niedrig = Bass kommt spät, hoch = ein schmales
 *                         Band um Frequency klingt nach
 *   Regler 3   Tap        Abgriff in der Kette, sieben Rastpunkte mit Haltezonen.
 *                         Zwischen zwei Rastpunkten wird übergeblendet, dabei entstehen
 *                         Kerben wie bei einem Phaser (je 2 Pole Abstand eine Kerbe).
 *   Regler 4   Feedback   0 … 97 %, Ausgang 1 zurück auf den Eingang
 *
 *   In 1       Audio (oder CV: der Signalweg ist DC-gekoppelt)
 *   In 2       Frequency-CV, 1 V/Okt, addiert zum Regler. Nicht kalibriert, ohne Totzone:
 *              Der Offset des offenen Eingangs (−0,2…−0,4 V, D36) verstimmt nur leicht.
 *   In 3       Tap-CV, ±5 V = voller Regelweg, wirkt pro Sample (bis Audio-Rate)
 *   In 4       Feedback-CV, ±5 V = voller Regelweg, wirkt pro Sample. Unter null kehrt
 *              sich die Schleife um (negatives Feedback, Spitzen und Täler tauschen).
 *              In 3 und In 4 sind unter ±0,5 V tot, damit der offene Eingang nichts
 *              verstellt.
 *
 *   Out 1 (L)  Abgriffe  2, 18, 34, 50, 66, 82,  98 Pole (Stufen 1, 9, 17 … 49)
 *   Out 2 (R)  Abgriffe  2, 22, 42, 62, 82, 102, 122 Pole (Stufen 1, 11, 21 … 61)
 *              Am untersten Rastpunkt sind L und R gleich (mono, fast unbearbeitet).
 *              Je höher der Abgriff, desto weiter laufen beide auseinander: Jedes Ohr
 *              bekommt alle Frequenzen gleich laut, aber zu anderen Zeiten.
 *   Out 3      Diff = (L − R) / 2: das, was die beiden Seiten unterscheidet. Hat Spitzen,
 *              wo die Monosumme L + R Kerben hat.
 *   Out 4      Tail = Ende der Kette (122 Pole), unabhängig vom Tap-Regler. Zusammen mit
 *              Out 1 lässt sich im Rack selbst zwischen früh und spät überblenden.
 *
 * Feedback: Out 1 (vor der Ausgangsstufe) geht über einen Hochpass (10 Hz) und eine
 * weiche Begrenzung (ab 5 V, höchstens 10 V) zurück auf den Eingang. Der Hochpass sitzt
 * nur in der Schleife: Der Vorwärtsweg bleibt DC-gekoppelt, aber Feedback verstärkt den
 * Eingangsoffset nicht (ohne Hochpass wären es bei 90 % zehnfach). Die Schleife schwingt
 * nicht von selbst (Verstärkung unter 1). Der Eingang wird mit √(1 − fb²) abgesenkt,
 * damit die Lautheit bei viel Feedback etwa gleich bleibt.
 *
 * Filterform: Zustandsvariablenfilter mit trapezförmiger Integration, Allpass = Eingang
 * − 2k · Bandpass. Die Direktform mit zwei Multiplikationen wäre billiger, rauscht in
 * float aber bei tiefen Frequenzen und hohem Q um 20 bis 60 dB mehr (Host-Vergleich,
 * 61 Stufen: 100 Hz, Q 16 → −71 gegen −115 dB). Alle Stufen teilen sich die Koeffizienten.
 * Frequency und Resonance werden einmal pro Block berechnet und über den Block linear
 * geführt, sehr schnelle Modulation der Frequenz wird dadurch geglättet.
 *
 * Eingang: In 1 läuft zuerst durch die Sperre für die Blockraten-Linien (AcdcInput.h).
 * Die ADC-Daten tragen eine Störung bei 6, 12 und 18 kHz, die Rechenlast dieses Patches
 * hebt sie um 3,8 dB (messlog D69), und ohne Sperre käme sie mit Faktor 1 an die
 * Ausgänge (D68). Die Sperre lernt die Störung in der Stille und zieht sie ab.
 *
 * Pegel: 1.0 = Vollausschlag ≈ 10 V. Die Ausgänge sind bis 7,5 V linear, darüber biegt
 * ein weiches Knie ab, mehr als 9,8 V kommen nicht heraus.
 *
 * LEDs (ohne USB-Aufnahme): LED 1–3 zeigen den Rastpunkt 1–7 binär in Rot (LED 1 = 1),
 * zwischen zwei Rastpunkten sind sie aus. LED 4 leuchtet rot, solange die Begrenzung in
 * der Schleife arbeitet. Die LEDs sind nur ganz aus oder rot, nie gedimmt: Grün wird von
 * der Firmware per PWM gedimmt (2320,54 Hz, Tastgrad = Wert), Rot ist ein statischer
 * Pin. Die erste Fassung ließ die LEDs dauerhaft mit 60 % leuchten, und der Nutzer hörte
 * ein Fiepsen (messlog D66, Nachtrag). Die LED-PWM ist als Störquelle bekannt (H3 in
 * docs/02), in den Aufnahmen sonst nur mit Signal, weil die LEDs sonst dunkel sind.
 *
 * Nicht nachgebildet: zweite Sektion und Reihenschaltung beider, Attenuverter,
 * Resonance-CV, einstellbare Abgriffe.
 *
 * basicmaths.h ersetzt expf() durch Tabellen-Näherungen. (expf)(x) in Klammern ruft die
 * genaue libm-Funktion.
 */

#include "Patch.h"
#include "../AcdcInput.h"
#include <math.h>
#include <stdint.h>

#ifndef SCHLIERE_STAGES
#define SCHLIERE_STAGES 61 // Länge der Kette in Stufen, mindestens 61 (letzter Abgriff von R)
#endif

class SchliereAPatch : public Patch {
  static constexpr size_t STAGES = SCHLIERE_STAGES;
  static constexpr size_t TAPS = 7;
  static constexpr float KNOB_DEAD = 0.03f;      // der Linksanschlag liest ≈ 0,016–0,023 (D51)
  static constexpr float CV_DEAD = 0.05f;        // 0,5 V
  static constexpr float CV_SCALE = 2.0f;        // 5 V ≙ voller Regelweg
  static constexpr float OCT_PER_FS = 10.0f;     // In 2: 1.0 ≙ 10 V ≙ 10 Oktaven
  static constexpr float F_CENTER = 632.4555f;   // Regler 1 auf 12 Uhr: √(20 · 20000) Hz
  static constexpr float F_LN_RANGE = 7.6009025f; // ln(2000): eine halbe Oktave Reserve je Seite
  static constexpr float F_MIN = 20.0f;
  static constexpr float F_MAX = 20000.0f;
  static constexpr float LN2 = 0.69314718f;
  static constexpr float FB_MAX = 0.97f;
  static constexpr float HOLD = 0.15f;           // Haltezone je Seite, Anteil am Abstand zweier Rastpunkte
  static constexpr float SMOOTH_S = 0.010f;      // Regler-Glättung, τ = 10 ms
  static constexpr float LOOP_HP_HZ = 10.0f;
  static constexpr float LOOP_KNEE = 0.5f;       // Schleife: linear bis 5 V
  static constexpr float LOOP_CEILING = 1.0f;
  static constexpr float OUT_KNEE = 0.75f;       // Ausgänge: linear bis 7,5 V
  static constexpr float OUT_CEILING = 0.98f;
  static constexpr float LED_RED = 1.0f;         // ab 4090 von 4095 schaltet die Firmware auf Rot
  static constexpr float RED_HOLD_S = 0.3f;

  /* linear bis knee, darüber tanh-Knie mit Asymptote ceiling */
  static float soft(float x, float knee, float ceiling){
    const float a = fabsf(x);
    if(a <= knee)
      return x;
    const float range = ceiling - knee;
    const float y = knee + range * (tanhf)((a - knee) / range);
    return x < 0.0f ? -y : y;
  }
  static float deadzone(float cv){
    return cv > CV_DEAD ? cv - CV_DEAD : (cv < -CV_DEAD ? cv + CV_DEAD : 0.0f);
  }
  static float clamp(float x, float lo, float hi){
    return x < lo ? lo : (x > hi ? hi : x);
  }
  /* Inf oder NaN. Über die Bits geprüft: Mit -ffast-math darf der Compiler Vergleiche
   * mit NaN wegoptimieren. */
  static bool broken(float x){
    union { float f; uint32_t u; } v = { x };
    return (v.u & 0x7f800000u) == 0x7f800000u;
  }
  /* Resonance-Regler → ln(Q): Gerade zwischen den Stützpunkten 0,01 / 0,026 / 0,51 /
   * 3,81 / 16 bei 0, 25, 50, 75, 100 % */
  static float lnQ(float p){
    static const float table[5] = { -4.6051702f, -3.6496587f, -0.6733446f, 1.3376292f, 2.7725887f };
    const float u = clamp(p, 0.0f, 1.0f) * 4.0f;
    const int i = u >= 4.0f ? 3 : (int)u;
    return table[i] + (table[i + 1] - table[i]) * (u - i);
  }
  float knob(PatchParameterId id){
    return clamp((getParameterValue(id) - KNOB_DEAD) / (1.0f - 2.0f * KNOB_DEAD), 0.0f, 1.0f);
  }

  float state[2 * STAGES] = {}; // je Stufe die beiden Integratoren
  float node[STAGES + 1] = {};  // Signal nach k Stufen, node[0] = Eingang der Kette
  float g = 0.0f, k = 2.0f;     // Koeffizienten am Ende des letzten Blocks
  float freqKnob = 0.5f, resKnob = 0.5f, tapKnob = 0.0f, fbKnob = 0.0f; // geglättet
  AcdcBlockRateCanceller inputClean; // Sperre für die Blockraten-Linien auf In 1
  float loopLp = 0.0f;          // Tiefpass für den Hochpass in der Schleife
  float loopState = 0.0f;       // begrenztes Feedback-Signal des letzten Samples
  float sampleRate;
  float loopHp;
  float slewBlock = 0.0f, slewSample;
  size_t slewSize = 0;
  size_t redBlocks = 0;
  size_t channels;
  bool primed = false;

  void coefficients(float knobF, float knobQ, float octaves, float& gOut, float& kOut){
    float f = F_CENTER * (expf)(F_LN_RANGE * (knobF - 0.5f) + LN2 * octaves);
    f = clamp(f, F_MIN, F_MAX);
    gOut = (tanf)(3.14159265f * f / sampleRate);
    kOut = (expf)(-lnQ(knobQ));
  }

public:
  SchliereAPatch(){
    static_assert(STAGES >= 61, "SCHLIERE_STAGES muss mindestens 61 sein");
    channels = getNumberOfChannels();
    sampleRate = getSampleRate();
    inputClean.setSampleRate(sampleRate);
    loopHp = 1.0f - (expf)(-2.0f * 3.14159265f * LOOP_HP_HZ / sampleRate);
    slewSample = 1.0f - (expf)(-1.0f / (SMOOTH_S * sampleRate));
    registerParameter(PARAMETER_A, "Frequency");
    registerParameter(PARAMETER_B, "Resonance");
    registerParameter(PARAMETER_C, "Tap");
    registerParameter(PARAMETER_D, "Feedback");
  }

  void processAudio(AudioBuffer& buffer){
    if(channels < 4)
      return; // nur für das AC/DC gedacht
    static const uint8_t tapsL[TAPS] = { 1, 9, 17, 25, 33, 41, 49 };
    static const uint8_t tapsR[TAPS] = { 1, 11, 21, 31, 41, 51, 61 };
    const size_t size = buffer.getSize();
    FloatArray io[4] = { buffer.getSamples(0), buffer.getSamples(1),
                         buffer.getSamples(2), buffer.getSamples(3) };
    if(size != slewSize){
      slewSize = size;
      slewBlock = 1.0f - (expf)(-(float)size / (SMOOTH_S * sampleRate));
    }

    // Frequency und Resonance: einmal pro Block, über den Block linear geführt
    const float targetF = knob(PARAMETER_A), targetQ = knob(PARAMETER_B);
    const float targetTap = knob(PARAMETER_C), targetFb = knob(PARAMETER_D);
    if(!primed){ // erster Block: Regler ohne Einschwingen übernehmen
      freqKnob = targetF; resKnob = targetQ; tapKnob = targetTap; fbKnob = targetFb;
    }
    freqKnob += (targetF - freqKnob) * slewBlock;
    resKnob += (targetQ - resKnob) * slewBlock;
    float octaves = 0.0f;
    for(size_t i = 0; i < size; ++i)
      octaves += io[1][i];
    octaves *= OCT_PER_FS / size;
    float gTarget, kTarget;
    coefficients(freqKnob, resKnob, octaves, gTarget, kTarget);
    if(!primed){
      g = gTarget; k = kTarget; primed = true;
    }
    const float gStep = (gTarget - g) / size, kStep = (kTarget - k) / size;

    float loopPeak = 0.0f, fbNow = 0.0f, tapNow = 0.0f;
    int tapIndex = -1;
    for(size_t i = 0; i < size; ++i){
      g += gStep;
      k += kStep;
      const float a1 = 1.0f / (1.0f + g * (g + k));
      const float a2 = g * a1;
      const float c = 2.0f * k;

      fbKnob += (targetFb - fbKnob) * slewSample;
      const float fb = FB_MAX * clamp(fbKnob + CV_SCALE * deadzone(io[3][i]), -1.0f, 1.0f);
      float x = (sqrtf)(1.0f - fb * fb) * inputClean.process(io[0][i]) + fb * loopState;

      // die Kette
      node[0] = x;
      float* s = state;
      for(size_t n = 1; n <= STAGES; ++n, s += 2){
        const float v3 = x - s[1];
        const float v1 = a1 * s[0] + a2 * v3;
        const float v2 = s[1] + g * v1;
        s[0] = 2.0f * v1 - s[0];
        s[1] = 2.0f * v2 - s[1];
        x -= c * v1;
        node[n] = x;
      }

      // Abgriffe: Haltezone, dazwischen lineare Überblendung
      tapKnob += (targetTap - tapKnob) * slewSample;
      const float u = clamp(tapKnob + CV_SCALE * deadzone(io[2][i]), 0.0f, 1.0f) * (TAPS - 1);
      const int index = u >= (float)(TAPS - 1) ? (int)TAPS - 2 : (int)u;
      const float t = clamp((u - index - HOLD) / (1.0f - 2.0f * HOLD), 0.0f, 1.0f);
      const float la = node[tapsL[index]], ra = node[tapsR[index]];
      const float left = la + t * (node[tapsL[index + 1]] - la);
      const float right = ra + t * (node[tapsR[index + 1]] - ra);

      // Schleife: Hochpass, dann weich begrenzen
      loopLp += (left - loopLp) * loopHp;
      const float loop = left - loopLp;
      loopState = soft(loop, LOOP_KNEE, LOOP_CEILING);
      if(fabsf(loop) > loopPeak) loopPeak = fabsf(loop);

      io[0][i] = soft(left, OUT_KNEE, OUT_CEILING);
      io[1][i] = soft(right, OUT_KNEE, OUT_CEILING);
      io[2][i] = soft(0.5f * (left - right), OUT_KNEE, OUT_CEILING);
      io[3][i] = soft(node[STAGES], OUT_KNEE, OUT_CEILING);
      fbNow = fb;
      tapNow = t;
      tapIndex = index;
    }

    // Unsinn im Zustand (sollte nie vorkommen): alles zurücksetzen statt hängen zu bleiben
    if(broken(node[STAGES]) || broken(loopState) || broken(loopLp)){
      for(size_t n = 0; n < 2 * STAGES; ++n) state[n] = 0.0f;
      for(size_t n = 0; n <= STAGES; ++n) node[n] = 0.0f;
      loopLp = loopState = 0.0f;
      for(size_t ch = 0; ch < 4; ++ch)
        for(size_t i = 0; i < size; ++i) io[ch][i] = 0.0f;
    }

    // LEDs: Rastpunkt binär, nur in der Haltezone
    int number = 0;
    if(tapNow <= 0.0f) number = tapIndex + 1;
    else if(tapNow >= 1.0f) number = tapIndex + 2;
    for(int bit = 0; bit < 3; ++bit)
      setParameterValue(PatchParameterId(PARAMETER_AA + bit), (number >> bit) & 1 ? LED_RED : 0.0f);
    if(loopPeak > LOOP_KNEE && fbNow != 0.0f)
      redBlocks = (size_t)(RED_HOLD_S * sampleRate / size); // Rot hält nach, sonst flackert es nur
    else if(redBlocks > 0)
      redBlocks--;
    setParameterValue(PARAMETER_AD, redBlocks > 0 ? LED_RED : 0.0f);
  }
};

#endif // __SCHLIERE_A_PATCH_HPP__
