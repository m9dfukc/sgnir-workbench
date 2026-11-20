#ifndef __SCHLIERE_PATCH_HPP__
#define __SCHLIERE_PATCH_HPP__

/* Schliere: lange Allpass-Kette für das AC/DC, nach dem Vorbild eines digitalen
 * Eurorack-Moduls mit 98 Allpass-Polen je Sektion (Nutzer-Wunsch 2026-11-18). Nachgebaut
 * ist eine Sektion: mono hinein, stereo heraus, dazu zwei abgeleitete Signale.
 *
 * Übersicht über alle Varianten (A, Z, W, O, M): README.md in diesem Ordner.
 * Diese Datei enthält SchlierePatch (als „SchliereZ“ gebaut) und, über SchliereWPatch.hpp,
 * SchliereWPatch. SchliereOPatch.hpp baut darauf auf.
 *
 *   SchlierePatch   Tap überblendet zwischen sieben festen Abgriffen der Kette, wie das
 *                   Vorbild. Zwischen zwei Abgriffen entstehen Kerben wie bei einem Phaser.
 *                   Moduliert man den Tap, blendet man die Kerben ein und aus: Das ist eine
 *                   Amplitudenmodulation und klingt bei Audio-Rate metallisch, nach Ringmod.
 *   SchliereWPatch  (über SchliereWPatch.hpp) wie SchlierePatch, aber die Feedback-Menge
 *                   wird nur mit 12 kHz statt 1,5 kHz geglättet (wild). Mit Audio an In 4, etwa einem Ausgang des
 *                   Patches, entstehen fast so viele neue Obertöne wie in Fassung 3, gefaltet
 *                   wird 9–13 dB weniger, und die Feedback-Menge wird nicht hart abgeschnitten
 *                   (messlog D80, zweiter Nachtrag). Der Nutzer fand Fassung 4 zu zahm.
 *
 * Prinzip: gleiche Allpässe 2. Ordnung in Reihe (1 Stufe = 2 Pole). Jede Stufe lässt
 * alle Frequenzen gleich laut durch und verzögert sie unterschiedlich lang. In der Summe
 * wird daraus eine hörbare, frequenzabhängige Laufzeit: Transienten verschmieren zu
 * einem Zwitschern, mit Feedback entsteht ein Resonator.
 *
 *   Regler 1   Frequency  20 Hz … 20 kHz: wo die Laufzeit wirkt
 *   Regler 2   Resonance  Q 0,01 … 16: niedrig = Bass kommt spät, hoch = ein schmales
 *                         Band um Frequency klingt nach
 *   Regler 3   Tap        Abgriff in der Kette, sieben Rastpunkte mit Haltezonen. Zwischen
 *                         zwei Rastpunkten wird übergeblendet, dabei entstehen Kerben (je
 *                         2 Pole Abstand eine Kerbe).
 *   Regler 4   Feedback   0 … 95 %, Ausgang 1 zurück auf den Eingang. Mit CV bis 97 %,
 *                         ab 90 % (Regler und CV zusammen) mit weichem Knie
 *
 *   In 1       Audio (oder CV: der Signalweg ist DC-gekoppelt)
 *   In 2       Frequency-CV, 1 V/Okt, addiert zum Regler. Nicht kalibriert, ohne Totzone:
 *              Der Offset des offenen Eingangs (−0,2…−0,4 V, D36) verstimmt nur leicht.
 *   In 3       Tap-CV, ±5 V = voller Regelweg, wirkt pro Sample (bis Audio-Rate)
 *   In 4       Feedback-CV, ±5 V = voller Regelweg, wirkt pro Sample. Unter null kehrt
 *              sich die Schleife um (negatives Feedback, Spitzen und Täler tauschen).
 *              In 3 und In 4 sind bis ±0,45 V tot, damit der offene Eingang nichts
 *              verstellt. Bis ±0,65 V setzt der CV weich ein, darüber wirkt er linear,
 *              um 0,55 V versetzt. Die weiche Kante ist wichtig bei bipolarer Modulation:
 *              Ein harter Knick im Nulldurchgang wirkt wie Übernahmeverzerrung und bringt
 *              Oberwellen des Modulators, die bei Audio-Rate über fs/2 falten.
 *
 *   Out 1 (L)  Abgriffe 2, 18, 34, 50, 66, 82,  98 Pole (Stufen 1, 9 … 49)
 *   Out 2 (R)  Abgriffe 2, 22, 42, 62, 82, 102, 122 Pole (Stufen 1, 11 … 61)
 *              Am untersten Rastpunkt sind L und R gleich (mono, fast unbearbeitet).
 *              Je höher der Abgriff, desto weiter laufen beide auseinander: Jedes Ohr
 *              bekommt alle Frequenzen gleich laut, aber zu anderen Zeiten.
 *   Out 3      Diff = (L − R) / 2: das, was die beiden Seiten unterscheidet. Hat Spitzen,
 *              wo die Monosumme L + R Kerben hat.
 *   Out 4      Tail = Ende der Kette (122 Pole), unabhängig vom Tap-Regler. Zusammen mit
 *              Out 1 lässt sich im Rack selbst zwischen früh und spät überblenden.
 *
 * Tap-Modulation (Fassung 4, 2026-11-19): Bei Audio-Rate am Tap-CV faltete Fassung 3
 * Seitenbänder über fs/2. Im Modell mit 5 kHz am Eingang und 441 Hz am Tap-CV lagen die
 * gefalteten Linien bei ±3 V nur 34 dB unter dem Signal. Ursachen waren die harte
 * Totzone, die Knicke am Rand der Haltezonen und der ungeglättete CV. Fassung 4 setzt den
 * CV weich ein (siehe In 3), überblendet am Rand der Haltezonen mit einer S-Kurve und
 * glättet die Gewichte der Abgriffe mit zwei Tiefpässen 1. Ordnung in Reihe (je 3 kHz,
 * zusammen ≈ 1,9 kHz, trapezförmig mit Nullstelle bei fs/2). Geglättet werden die
 * Gewichte, nicht der CV: Die Knicke entstehen erst in der Abbildung CV → Gewicht. Weil
 * alle Gewichte durch dasselbe lineare Filter laufen, bleibt ihre Summe 1. Modulation über
 * ≈ 2 kHz wird dadurch gedämpft, das Vorbild glättet ebenfalls (Forum, Thread 296092 #36).
 * Dasselbe gilt für die Feedback-Menge (In 4): Steckt man einen Ausgang des Patches auf
 * In 4, klang Fassung 3 nach hartem Clipping (Nutzer, 2026-11-19). Sie hat jetzt ein
 * weiches Knie statt der harten Grenze und zwei Tiefpässe mit je 1,5 kHz (messlog D80).
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
 * 61 Stufen: 100 Hz, Q 16 → −71 gegen −115 dB). Alle vollen Stufen teilen sich die
 * Koeffizienten. Frequency und Resonance werden einmal pro Block berechnet und über den
 * Block linear geführt, sehr schnelle Modulation der Frequenz wird dadurch geglättet.
 *
 * Eingang: In 1 läuft zuerst durch die Sperre für die Blockraten-Linien (AcdcInput.h).
 * Die ADC-Daten tragen eine Störung bei 6, 12 und 18 kHz, die Rechenlast dieses Patches
 * hebt sie um 3,8 dB (messlog D69), und ohne Sperre käme sie mit Faktor 1 an die
 * Ausgänge (D68). Die Sperre lernt die Störung in der Stille und zieht sie ab.
 *
 * Pegel: 1.0 = Vollausschlag ≈ 10 V. Die Ausgänge sind bis 7,5 V linear, darüber biegt
 * ein weiches Knie ab, mehr als 9,8 V kommen nicht heraus.
 *
 * LEDs (ohne USB-Aufnahme): LED 1–3 zeigen den Rastpunkt 1–7 von Regler 3 binär in Rot
 * (LED 1 = 1), zwischen zwei Rastpunkten sind sie aus. Ab Fassung 4 zählt nur der Regler,
 * nicht der CV, mit Hysterese (an ±12 % des Abstands an, ab ±18 % aus): Bei Tap-Modulation
 * schalteten die LEDs sonst im Blocktakt und gäben Knackser (D73). LED 4 leuchtet rot, solange die Begrenzung in der Schleife
 * arbeitet. Die LEDs sind nur ganz aus oder rot, nie gedimmt: Grün wird von der Firmware
 * per PWM gedimmt (2320,54 Hz bis fx6a, 48 kHz ab fx7a), Rot ist ein statischer Pin. Die
 * erste Fassung ließ die LEDs dauerhaft mit 60 % leuchten, und der Nutzer hörte ein
 * Fiepsen (messlog D66, Nachtrag). Jeder Wechsel einer LED verschiebt den Ausgang (D73).
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

/* Gemeinsamer Teil beider Patches: Regler, Koeffizienten, Eingang, Feedback, Ausgänge, LEDs */
class SchliereBase : public Patch {
protected:
  static constexpr size_t STAGES = SCHLIERE_STAGES;
  static constexpr size_t TAPS = 7;
  static constexpr float KNOB_DEAD = 0.03f;      // der Linksanschlag liest ≈ 0,016–0,023 (D51)
  static constexpr float CV_DEAD_LO = 0.045f;    // bis 0,45 V tot
  static constexpr float CV_DEAD_HI = 0.065f;    // ab 0,65 V linear
  static constexpr float CV_SCALE = 2.0f;        // 5 V ≙ voller Regelweg
  static constexpr float OCT_PER_FS = 10.0f;     // In 2: 1.0 ≙ 10 V ≙ 10 Oktaven
  static constexpr float F_CENTER = 632.4555f;   // Regler 1 auf 12 Uhr: √(20 · 20000) Hz
  static constexpr float F_LN_RANGE = 7.6009025f; // ln(2000): eine halbe Oktave Reserve je Seite
  static constexpr float F_MIN = 20.0f;
  static constexpr float F_MAX = 20000.0f;
  static constexpr float LN2 = 0.69314718f;
  static constexpr float FB_MAX = 0.97f;         // Grenze der Feedback-Menge (nur mit CV erreichbar)
  static constexpr float FB_KNEE = 0.9f;         // bis hier linear, darüber weiches Knie
  static constexpr float HOLD = 0.15f;           // Haltezone je Seite, Anteil am Abstand zweier Rastpunkte
  static constexpr float SMOOTH_S = 0.010f;      // Regler-Glättung, τ = 10 ms
  static constexpr float MOD_HZ = 3000.0f;       // Glättung der Tap-Modulation, je Pol (2 Pole)
  static constexpr float FB_MOD_HZ = 1500.0f;    // Glättung der Feedback-Menge, je Pol (2 Pole)
  static constexpr float FB_MOD_HZ_WILD = 12000.0f; // dasselbe für SchliereWPatch
  static constexpr float LOOP_HP_HZ = 10.0f;
  static constexpr float LOOP_KNEE = 0.5f;       // Schleife: linear bis 5 V
  static constexpr float LOOP_CEILING = 1.0f;
  static constexpr float OUT_KNEE = 0.75f;       // Ausgänge: linear bis 7,5 V
  static constexpr float OUT_CEILING = 0.98f;
  static constexpr float LED_RED = 1.0f;         // ab 4090 von 4095 schaltet die Firmware auf Rot
  static constexpr float RED_HOLD_S = 0.3f;
  static constexpr float LED_HYST = 0.03f;       // Hysterese der Rastpunkt-Anzeige, Anteil am Abstand

  /* linear bis knee, darüber tanh-Knie mit Asymptote ceiling */
  static float soft(float x, float knee, float ceiling){
    const float a = fabsf(x);
    if(a <= knee)
      return x;
    const float range = ceiling - knee;
    const float y = knee + range * (tanhf)((a - knee) / range);
    return x < 0.0f ? -y : y;
  }
  /* Totzone mit weicher Kante: 0 bis LO, dazwischen y³ − y⁴/2 (Steigung und Krümmung
   * stetig), ab HI linear. Versatz danach (LO + HI) / 2. */
  static float deadzone(float cv){
    const float a = fabsf(cv);
    if(a <= CV_DEAD_LO)
      return 0.0f;
    float d;
    if(a >= CV_DEAD_HI){
      d = a - 0.5f * (CV_DEAD_LO + CV_DEAD_HI);
    }else{
      const float w = CV_DEAD_HI - CV_DEAD_LO, y = (a - CV_DEAD_LO) / w;
      d = w * y * y * y * (1.0f - 0.5f * y);
    }
    return cv < 0.0f ? -d : d;
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
  /* Tiefpass 1. Ordnung, trapezförmig: Nullstelle bei fs/2. Ein einfacher Tiefpass
   * (y += (x − y)·a) dämpft dort nur 14 dB je Pol, zu wenig gegen Faltung. */
  static inline float lowpass(float x, float& s, float G){
    const float v = (x - s) * G;
    const float y = v + s;
    s = y + v;
    return y;
  }
  /* eine Stufe: Zustandsvariablenfilter (trapezförmig), Allpass = x − c · Bandpass */
  static inline float stage(float x, float* s, float a1, float a2, float g, float c){
    const float v3 = x - s[1];
    const float v1 = a1 * s[0] + a2 * v3;
    const float v2 = s[1] + g * v1;
    s[0] = 2.0f * v1 - s[0];
    s[1] = 2.0f * v2 - s[1];
    return x - c * v1;
  }
  float knob(PatchParameterId id){
    return clamp((getParameterValue(id) - KNOB_DEAD) / (1.0f - 2.0f * KNOB_DEAD), 0.0f, 1.0f);
  }

  float g = 0.0f, k = 2.0f;     // Koeffizienten, pro Sample weitergeführt
  float a1 = 0.0f, a2 = 0.0f, c = 0.0f;
  float gStep = 0.0f, kStep = 0.0f;
  float freqKnob = 0.5f, resKnob = 0.5f, tapKnob = 0.0f, fbKnob = 0.0f; // geglättet
  float targetTap = 0.0f, targetFb = 0.0f;
  AcdcBlockRateCanceller inputClean; // Sperre für die Blockraten-Linien auf In 1
  float loopLp = 0.0f;          // Tiefpass für den Hochpass in der Schleife
  float loopState = 0.0f;       // begrenztes Feedback-Signal des letzten Samples
  float loopPeak = 0.0f, fbNow = 0.0f;
  float fb1 = 0.0f, fb2 = 0.0f; // Zustand der beiden Tiefpässe für die Feedback-Menge
  float sampleRate;
  float loopHp;
  float modG, fbG;              // Beiwerte der Tiefpässe für Tap und Feedback-Menge
  float slewBlock = 0.0f, slewSample;
  size_t slewSize = 0;
  size_t redBlocks = 0;
  int ledPoint = 0;             // angezeigter Rastpunkt 1–7, 0 = keiner
  size_t channels;
  bool primed = false;

  void coefficients(float knobF, float knobQ, float octaves, float& gOut, float& kOut){
    float f = F_CENTER * (expf)(F_LN_RANGE * (knobF - 0.5f) + LN2 * octaves);
    f = clamp(f, F_MIN, F_MAX);
    gOut = (tanf)(3.14159265f * f / sampleRate);
    kOut = (expf)(-lnQ(knobQ));
  }

  SchliereBase(){
    channels = getNumberOfChannels();
    sampleRate = getSampleRate();
    inputClean.setSampleRate(sampleRate);
    loopHp = 1.0f - (expf)(-2.0f * 3.14159265f * LOOP_HP_HZ / sampleRate);
    const float gm = (tanf)(3.14159265f * MOD_HZ / sampleRate);
    modG = gm / (1.0f + gm);
    const float gf = (tanf)(3.14159265f * FB_MOD_HZ / sampleRate);
    fbG = gf / (1.0f + gf);
    slewSample = 1.0f - (expf)(-1.0f / (SMOOTH_S * sampleRate));
    registerParameter(PARAMETER_A, "Frequency");
    registerParameter(PARAMETER_B, "Resonance");
    registerParameter(PARAMETER_C, "Tap");
    registerParameter(PARAMETER_D, "Feedback");
  }

  /* Blockanfang: Regler lesen, Frequency und Resonance für das Blockende berechnen und
   * über den Block linear führen. Liest In 2 ganz, bevor die Ausgänge geschrieben werden.
   * Gibt true zurück im ersten Block (Glättungen ohne Einschwingen übernehmen). */
  bool beginBlock(FloatArray* io, size_t size){
    if(size != slewSize){
      slewSize = size;
      slewBlock = 1.0f - (expf)(-(float)size / (SMOOTH_S * sampleRate));
    }
    const float targetF = knob(PARAMETER_A), targetQ = knob(PARAMETER_B);
    targetTap = knob(PARAMETER_C);
    targetFb = knob(PARAMETER_D);
    const bool first = !primed;
    if(first){
      freqKnob = targetF; resKnob = targetQ; tapKnob = targetTap; fbKnob = targetFb;
      fb1 = fb2 = FB_MAX * soft(targetFb, FB_KNEE, 1.0f);
    }
    freqKnob += (targetF - freqKnob) * slewBlock;
    resKnob += (targetQ - resKnob) * slewBlock;
    float octaves = 0.0f;
    for(size_t i = 0; i < size; ++i)
      octaves += io[1][i];
    octaves *= OCT_PER_FS / size;
    float gTarget, kTarget;
    coefficients(freqKnob, resKnob, octaves, gTarget, kTarget);
    if(first){
      g = gTarget; k = kTarget; primed = true;
    }
    gStep = (gTarget - g) / size;
    kStep = (kTarget - k) / size;
    loopPeak = 0.0f;
    return first;
  }
  void stepCoefficients(){
    g += gStep;
    k += kStep;
    a1 = 1.0f / (1.0f + g * (g + k));
    a2 = g * a1;
    c = 2.0f * k;
  }
  /* Eingang der Kette: Sperre, dann Feedback (In 4 und Regler 4). Die Feedback-Menge
   * multipliziert das Signal der Schleife. Mit Audio an In 4 (etwa ein Ausgang des Patches
   * selbst) wäre das eine Ringmodulation pro Sample: Eine harte Grenze schneidet den CV ab,
   * und Summenfrequenzen über fs/2 falten zurück. Deshalb weiches Knie statt Grenze und
   * danach zwei Tiefpässe wie für den Tap, hier mit je 1,5 kHz (zusammen ≈ 1 kHz). */
  float chainInput(float in1, float cv4){
    fbKnob += (targetFb - fbKnob) * slewSample;
    const float target = FB_MAX * soft(fbKnob + CV_SCALE * deadzone(cv4), FB_KNEE, 1.0f);
    const float fb = lowpass(lowpass(target, fb1, fbG), fb2, fbG);
    fbNow = fb;
    return (sqrtf)(1.0f - fb * fb) * inputClean.process(in1) + fb * loopState;
  }
  /* Tap-Stellung 0 … TAPS − 1 aus Regler 3 (geglättet) und In 3 */
  float tapPosition(float cv3){
    tapKnob += (targetTap - tapKnob) * slewSample;
    return clamp(tapKnob + CV_SCALE * deadzone(cv3), 0.0f, 1.0f) * (TAPS - 1);
  }
  /* Schleife (Hochpass, weich begrenzen) und die vier Ausgänge */
  void outputs(FloatArray* io, size_t i, float left, float right, float out3, float out4){
    loopLp += (left - loopLp) * loopHp;
    const float loop = left - loopLp;
    loopState = soft(loop, LOOP_KNEE, LOOP_CEILING);
    if(fabsf(loop) > loopPeak) loopPeak = fabsf(loop);
    io[0][i] = soft(left, OUT_KNEE, OUT_CEILING);
    io[1][i] = soft(right, OUT_KNEE, OUT_CEILING);
    io[2][i] = soft(out3, OUT_KNEE, OUT_CEILING);
    io[3][i] = soft(out4, OUT_KNEE, OUT_CEILING);
  }
  /* Unsinn im Zustand (sollte nie vorkommen): Schleife und Ausgänge auf null. Die Kette
   * setzt der Patch selbst zurück. */
  bool resetIfBroken(FloatArray* io, size_t size, float probe){
    if(!broken(probe) && !broken(loopState) && !broken(loopLp))
      return false;
    loopLp = loopState = 0.0f;
    for(size_t ch = 0; ch < 4; ++ch)
      for(size_t i = 0; i < size; ++i) io[ch][i] = 0.0f;
    return true;
  }
  /* LEDs: Rastpunkt 1–7 binär, LED 4 = Begrenzung in der Schleife. Der Rastpunkt kommt nur
   * aus Regler 3, nicht aus dem CV, und hat eine Hysterese: Folgten die LEDs dem CV,
   * schalteten sie bei Modulation im Blocktakt, und jeder Wechsel einer LED gibt einen
   * Knack am Ausgang (D73). */
  void leds(size_t size){
    const float u = tapKnob * (TAPS - 1);
    if(ledPoint == 0 || fabsf(u - (float)(ledPoint - 1)) > HOLD + LED_HYST){
      const int nearest = (int)(u + 0.5f);
      ledPoint = fabsf(u - (float)nearest) <= HOLD - LED_HYST ? nearest + 1 : 0;
    }
    for(int bit = 0; bit < 3; ++bit)
      setParameterValue(PatchParameterId(PARAMETER_AA + bit), (ledPoint >> bit) & 1 ? LED_RED : 0.0f);
    if(loopPeak > LOOP_KNEE && fabsf(fbNow) > 1e-4f)
      redBlocks = (size_t)(RED_HOLD_S * sampleRate / size); // Rot hält nach, sonst flackert es nur
    else if(redBlocks > 0)
      redBlocks--;
    setParameterValue(PARAMETER_AD, redBlocks > 0 ? LED_RED : 0.0f);
  }
};

/* Überblendung zwischen sieben festen Abgriffen (wie das Vorbild) */
class SchlierePatch : public SchliereBase {
  float state[2 * STAGES] = {}; // je Stufe die beiden Integratoren
  float node[STAGES + 1] = {};  // Signal nach k Stufen, node[0] = Eingang der Kette
  float w1[TAPS] = {}, w2[TAPS] = {}; // Zustand der beiden Tiefpässe je Gewicht

public:
  SchlierePatch(){
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
      float x = chainInput(io[0][i], io[3][i]);

      // die Kette
      node[0] = x;
      float* s = state;
      for(size_t n = 1; n <= STAGES; ++n, s += 2){
        x = stage(x, s, a1, a2, g, c);
        node[n] = x;
      }

      // Abgriffe: Haltezone, dazwischen S-Kurve
      const float u = tapPosition(io[2][i]);
      const int index = u >= (float)(TAPS - 1) ? (int)TAPS - 2 : (int)u;
      float t = clamp((u - index - HOLD) / (1.0f - 2.0f * HOLD), 0.0f, 1.0f);
      t = t * t * (3.0f - 2.0f * t);

      // Gewichte je Abgriff, über zwei Tiefpässe geführt: Die Überblendung ist damit
      // bandbegrenzt, auch beim Wechsel über einen Rastpunkt hinweg
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
    }
    leds(size);
  }
};

#ifdef SCHLIERE_WILD_PATCH
/* Wie SchlierePatch, Feedback-Modulation mit mehr Bandbreite (wild). Nur über
 * SchliereWPatch.hpp: Schon eine abgeleitete Klasse ändert das Binary von SchlierePatch,
 * und das soll bytegleich mit dem Build auf dem Modul bleiben (SchliereZ). */
class SchliereWPatch : public SchlierePatch {
public:
  SchliereWPatch(){
    const float gf = (tanf)(3.14159265f * FB_MOD_HZ_WILD / sampleRate);
    fbG = gf / (1.0f + gf);
  }
};
#endif

#endif // __SCHLIERE_PATCH_HPP__
