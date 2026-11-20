#ifndef __ACDC_INPUT_H__
#define __ACDC_INPUT_H__

/* Eingangsstufe für FX-Patches auf dem AC/DC: Sperre für die Blockraten-Linien.
 *
 * Anlass (messlog D68, D69): In den ADC-Daten des AC/DC steckt eine Störung im Takt der
 * Audio-Blöcke, fs/8 = 6000 Hz mit Vielfachen bei 12, 18 und 24 kHz. Die Rechenlast
 * eines Patches hebt sie an (Schliere: +3,8 dB), und ein FX-Patch reicht sie an die
 * Ausgänge durch. Ohne Eingangssignal ist sie als Fiepsen zu hören.
 *
 * Prinzip: Die Störung ist sample-synchron und wiederholt sich alle 8 Samples exakt.
 * Für jede der 8 Positionen führt die Sperre einen langsamen Mittelwert des Eingangs.
 * Die Abweichung dieses Mittelwerts vom Mittel über die letzten 8 Samples ist genau der
 * Anteil mit Periode 8. Er wird abgezogen. Gleichspannung und alles andere bleiben
 * unberührt: Die Sperre wirkt wie vier sehr schmale Kerben bei 6, 12, 18 und 24 kHz.
 *
 * Gelernt wird nur in der Stille. Als Maß dient der Unterschied zum Sample 8 Takte
 * vorher: Darin fehlen Gleichspannung und Störung, übrig bleiben Signal und Rauschen.
 * Liegt die Hüllkurve dieses Unterschieds über THRESHOLD (≈ 20 mV), bleibt die Schätzung
 * stehen. Laute Musik kann sie so nicht verziehen, und die Kerben klingen nach einem
 * lauten Signal nicht nach.
 *
 * Grenzen:
 *   - Ein Ton genau auf 6000,0 Hz (±0,3 Hz, im Takt des Geräts) sieht aus wie die
 *     Störung. Die Korrektur ist deshalb auf MAX_CORRECTION (≈ 10 mV) begrenzt, einem
 *     solchen Ton fehlen höchstens 13 mV (Host-Test).
 *   - Ändert sich die Rechenlast des Patches, während es laut ist, passt die Schätzung
 *     erst in der nächsten leisen Stelle wieder.
 *   - Die Mischprodukte aus Kamm und Blockrate (D69: −104 dB) haben nicht die Periode 8
 *     und bleiben, ebenso der Kamm selbst.
 *
 * Aufruf: process(sample) für jedes Sample eines Kanals, ohne Lücken. Die Position im
 * Takt zählt die Sperre selbst mit.
 *
 * basicmaths.h ersetzt expf() durch eine Tabellen-Näherung. (expf)(x) in Klammern ruft
 * die genaue libm-Funktion.
 */

#include <math.h>
#include <stddef.h>

class AcdcBlockRateCanceller {
public:
  static constexpr size_t PERIOD = 8;            // Audio-Blockgröße der Firmware
  static constexpr float LEARN_S = 0.5f;         // Zeitkonstante der Mittelwerte
  static constexpr float RELEASE_S = 0.05f;      // so lange nach einem lauten Signal ruht das Lernen
  static constexpr float THRESHOLD = 0.002f;     // 20 mV
  static constexpr float MAX_CORRECTION = 0.001f; // 10 mV

  void setSampleRate(float sr){
    learn = 1.0f - (expf)(-(float)PERIOD / (LEARN_S * sr));
    release = (expf)(-1.0f / (RELEASE_S * sr));
  }

  float process(float x){
    const size_t i = phase;
    phase = (phase + 1) % PERIOD;
    const float change = fabsf(x - last[i]);
    last[i] = x;
    envelope = change > envelope ? change : envelope * release;
    if(envelope < THRESHOLD){
      const float delta = learn * (x - mean[i]);
      mean[i] += delta;
      sum += delta;
      if(++drift >= 4096){ // Rundungsfehler der mitgeführten Summe ab und zu ausgleichen
        drift = 0;
        sum = 0.0f;
        for(size_t n = 0; n < PERIOD; ++n) sum += mean[n];
      }
    }
    float correction = mean[i] - sum * (1.0f / PERIOD);
    if(correction > MAX_CORRECTION) correction = MAX_CORRECTION;
    else if(correction < -MAX_CORRECTION) correction = -MAX_CORRECTION;
    return x - correction;
  }

private:
  float mean[PERIOD] = {};
  float last[PERIOD] = {};
  float sum = 0.0f;
  float envelope = 1.0f;     // beginnt „laut“: erst lernen, wenn ein voller Takt bekannt ist
  float learn = 0.000333f;   // 0,5 s bei 48 kHz
  float release = 0.999583f; // 50 ms bei 48 kHz
  size_t phase = 0;
  size_t drift = 0;
};

#endif // __ACDC_INPUT_H__
