#ifndef __SCHLIERE_S_PATCH_HPP__
#define __SCHLIERE_S_PATCH_HPP__

/* Schliere S (sanft): SchliereO mit ganz leichter Glättung der Feedback-Modulation. Wunsch des
 * Nutzers (2026-11-19): O gefällt fast am besten, bei Feedback-Modulation aber ein ganz klein
 * wenig weniger Glitch.
 *
 * Der CV an In 4 läuft durch zwei Tiefpässe 1. Ordnung in Reihe (trapezförmig, Nullstelle bei
 * fs/2), bevor SchliereO ihn verarbeitet. Sonst ist alles wie bei SchliereO: Totzone, weiches
 * Knie und die Produkte mit 96 kHz. Geglättet wird hier vor der Kennlinie, nicht danach wie
 * bei SchlierePatch: SchliereO bleibt so unberührt und bytegleich mit seinem Slot (schon ein
 * Template oder eine Änderung in SchliereOPatch.hpp ändert dessen Binary). Die Kennlinie
 * rechnet SchliereO mit 96 kHz, ihre Oberwellen falten dort kaum.
 *
 * Regler 4 wird wie in allen Varianten nur mit 10 ms geglättet. Messung: messlog D80.
 */

#include "SchliereOPatch.hpp"

#ifndef SCHLIERE_S_HZ
#define SCHLIERE_S_HZ 12000.0f // Grenzfrequenz je Pol
#endif

class SchliereSPatch : public SchliereOPatch {
  float s1 = 0.0f, s2 = 0.0f; // Zustand der beiden Tiefpässe für In 4
  float G;
  bool started = false;
public:
  SchliereSPatch(){
    const float g = (tanf)(3.14159265f * SCHLIERE_S_HZ / getSampleRate());
    G = g / (1.0f + g);
  }
  void processAudio(AudioBuffer& buffer){
    FloatArray cv = buffer.getSamples(3);
    const size_t size = buffer.getSize();
    if(!started && size > 0){
      s1 = s2 = cv[0];
      started = true;
    }
    for(size_t i = 0; i < size; ++i)
      cv[i] = lowpass(lowpass(cv[i], s1, G), s2, G);
    SchliereOPatch::processAudio(buffer);
  }
};

#endif // __SCHLIERE_S_PATCH_HPP__
