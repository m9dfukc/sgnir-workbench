# Schliere

[Deutsch](#deutsch) · [English](#english)

---

## Deutsch

Schliere ist ein OWL-Patch für das AC/DC: eine lange Kette gleicher Allpässe 2. Ordnung
(1 Stufe = 2 Pole), nach dem Vorbild eines digitalen Eurorack-Moduls mit 98 Allpass-Polen je
Sektion. Jede Stufe lässt alle Frequenzen gleich laut durch und verzögert sie unterschiedlich
lang. In der Summe verschmieren Transienten zu einem Zwitschern, zwischen zwei Abgriffen
entstehen Kerben wie bei einem Phaser, mit Feedback wird daraus ein Resonator. Mono hinein,
stereo heraus.

Es gibt sechs Varianten. Alle haben dieselbe Belegung, sie unterscheiden sich darin, was
Regler 3 (Tap) tut und wie stark die Feedback-Modulation an In 4 wirken darf.

### Varianten

| Slot auf acdc1 | Name | Datei, Klasse | Tap | Feedback-Modulation (In 4) | CPU |
|---|---|---|---|---|---|
| 6 | **SchliereA** | `SchliereAPatch.hpp`, `SchliereAPatch` | Überblendung zwischen 7 Abgriffen, CV ungeglättet | ungeglättet, harte Grenze bei ±1 | 30 % |
| 7 | **SchliereM** | `SchliereMPatch.hpp`, `SchliereMPatch` | Länge der Kette stufenlos (Morph) | geglättet, 1,5 kHz | 49–54 % |
| 8 | **SchliereZ** | `SchlierePatch.hpp`, `SchlierePatch` | Überblendung, bandbegrenzt | geglättet, 1,5 kHz | 33 % |
| 9 | **SchliereW** | `SchliereWPatch.hpp`, `SchliereWPatch` | wie Z | geglättet, 12 kHz | 32–36 % |
| 10 | **SchliereO** | `SchliereOPatch.hpp`, `SchliereOPatch` | wie Z | ungeglättet, mit 96 kHz gerechnet | 43 % |
| 11 | **SchliereS** | `SchliereSPatch.hpp`, `SchliereSPatch` | wie Z | wie O, In 4 ganz leicht geglättet (12 kHz) | 39 % |

- **A (alt)** ist Fassung 3 vom 2026-11-18, das Original. Moduliert man Tap oder Feedback mit
  Audio-Rate, wird es glitchig und rau: Teile der Seitenbänder falten über fs/2 zurück, die
  Feedback-Menge wird hart abgeschnitten, und die Rastpunkt-LEDs schalten im Takt der
  Modulation (jeder Wechsel gibt einen Knack, messlog D73).
- **Z (zahm)** ist Fassung 4. Tap- und Feedback-CV setzen weich ein und werden bandbegrenzt,
  die LEDs folgen nur dem Regler. Kaum Faltung, aber die Feedback-Modulation erzeugt auch
  kaum neue Obertöne.
- **W (wild)** ist Z mit weniger geglätteter Feedback-Modulation: fast so viele neue
  Obertöne wie A, weniger Faltung, kein hartes Abschneiden.
- **O (Oversampling)** rechnet den Eingang der Kette mit doppelter Abtastrate. Die
  Feedback-Menge ist ungeglättet wie bei A, die Produkte laufen aber bei 96 kHz und werden
  danach gefiltert. Am meisten neue Obertöne, deutlich weniger Faltung als A. Die Schleife
  ist dadurch knapp 3 Samples länger, die Feedback-Spitzen liegen etwas anders.
- **S (sanft)** ist O mit einer ganz leichten Glättung: Der CV an In 4 läuft durch zwei
  Tiefpässe mit je 12 kHz (zusammen ≈ 7,7 kHz), bevor O ihn verarbeitet. Ein klein wenig
  weniger Glitch bei Feedback-Modulation, sonst gleich O.
- **M (Morph)** überblendet nicht, sondern ändert die Länge der Kette stufenlos. Die Kette
  bleibt in jeder Stellung ein reiner Allpass: Tap-Modulation verschiebt die Laufzeit
  (langsam ein Vibrato, schnell FM-artig) statt Kerben ein- und auszublenden. Out 4 ist hier
  die Summe (L + R)/2, deren Kerben mit dem Tap wandern. Dem Nutzer zu zahm, liegt aber
  weiter auf Slot 7.

Gemessen am Rechner mit dem Patch-Code (messlog D80). „Gefaltet“ = Linien, die durch Faltung
an fs/2 entstehen, gegen das Nutzsignal:

| Fall | A | Z | W | O | S | M |
|---|---|---|---|---|---|---|
| Tap-CV 441 Hz, ±3 V, gefaltet | −34 dB | −82 dB | −82 dB | −82 dB | −82 dB | −64 dB |
| Out 2 → In 4, 30 % Feedback, 6 V: neue Obertöne | −6 dB | −44 dB | −11 dB | +4 dB | −5 dB | – |
| dasselbe, gefaltet | −19 dB | −70 dB | −32 dB | −33 dB | −43 dB | −49 dB |
| Out 2 → In 4, 70 % Feedback, 6 V: neue Obertöne | −10 dB | −38 dB | −11 dB | +1 dB | +3 dB | – |
| dasselbe, gefaltet | −21 dB | −60 dB | −30 dB | −35 dB | −36 dB | −41 dB |

Mit 70 % Feedback und 6 V läuft die Schleife chaotisch, dort streuen die Werte von Lauf zu
Lauf um einige dB.

Am Gerät ist bisher nur die Rechenlast gemessen. Wie die Varianten klingen, entscheidet das
Hören.

### Belegung (alle Varianten)

| | |
|---|---|
| Regler 1 | Frequency, 20 Hz … 20 kHz: wo die Laufzeit wirkt |
| Regler 2 | Resonance, Q 0,01 … 16 |
| Regler 3 | Tap: Abgriff (A, Z, W, O) bzw. Länge der Kette (M) |
| Regler 4 | Feedback (A: 0 … 97 %, sonst 0 … 95 %, mit CV bis 97 %) |
| In 1 | Audio (DC-gekoppelt) |
| In 2 | Frequency-CV, etwa 1 V/Okt |
| In 3 | Tap-CV, ±5 V = voller Regelweg |
| In 4 | Feedback-CV, ±5 V = voller Regelweg, unter null negatives Feedback |
| Out 1, Out 2 | L (bis 98 Pole), R (bis 122 Pole) |
| Out 3 | Diff (L − R)/2 |
| Out 4 | Tail = Ende der Kette (A, Z, W, O) bzw. Summe (L + R)/2 (M) |
| LED 1–3 | Rastpunkt 1–7 von Regler 3, binär in Rot |
| LED 4 | Rot, solange die Begrenzung in der Schleife arbeitet |

Einzelheiten stehen im Kopf von `SchlierePatch.hpp`, für A in `SchliereAPatch.hpp`, für M in
`SchliereMPatch.hpp`, für O in `SchliereOPatch.hpp`, für S in `SchliereSPatch.hpp`.

### Bauen und testen

```bash
SRC=owl/schliere tools/build_owl_patch.sh SchliereZ SchlierePatch SchlierePatch.hpp
SRC=owl/schliere tools/build_owl_patch.sh SchliereA SchliereAPatch SchliereAPatch.hpp
SRC=owl/schliere tools/build_owl_patch.sh SchliereM SchliereMPatch SchliereMPatch.hpp
SRC=owl/schliere tools/build_owl_patch.sh SchliereW SchliereWPatch SchliereWPatch.hpp
SRC=owl/schliere tools/build_owl_patch.sh SchliereO SchliereOPatch SchliereOPatch.hpp
SRC=owl/schliere tools/build_owl_patch.sh SchliereS SchliereSPatch SchliereSPatch.hpp
tools/test_owl_dsp.sh
```

Die Builds landen in `owl-patches/builds/<Name>/` und sind bytegleich mit den Slots auf acdc1.
W, O, S und M stehen in eigenen Dateien: Schon eine abgeleitete Klasse in `SchlierePatch.hpp`
ändert das Binary von SchliereZ. Der Name steckt im Binary, deshalb
heißen die Builds wie die Slots.

---

## English

Schliere is an OWL patch for the AC/DC: a long chain of identical second-order allpass filters
(1 stage = 2 poles), modelled on a digital Eurorack module with 98 allpass poles per section.
Every stage passes all frequencies at the same level and delays them by different amounts.
Together, transients smear into a chirp, crossfading between two taps creates phaser-like
notches, and feedback turns it into a resonator. Mono in, stereo out.

There are six variants. They share the same controls and differ in what knob 3 (tap) does
and how strongly feedback modulation at in 4 may act.

### Variants

| Slot on acdc1 | Name | File, class | Tap | Feedback modulation (in 4) | CPU |
|---|---|---|---|---|---|
| 6 | **SchliereA** | `SchliereAPatch.hpp`, `SchliereAPatch` | crossfade between 7 taps, CV unsmoothed | unsmoothed, hard limit at ±1 | 30 % |
| 7 | **SchliereM** | `SchliereMPatch.hpp`, `SchliereMPatch` | chain length, continuous (morph) | smoothed, 1.5 kHz | 49–54 % |
| 8 | **SchliereZ** | `SchlierePatch.hpp`, `SchlierePatch` | crossfade, band-limited | smoothed, 1.5 kHz | 33 % |
| 9 | **SchliereW** | `SchliereWPatch.hpp`, `SchliereWPatch` | as Z | smoothed, 12 kHz | 32–36 % |
| 10 | **SchliereO** | `SchliereOPatch.hpp`, `SchliereOPatch` | as Z | unsmoothed, computed at 96 kHz | 43 % |
| 11 | **SchliereS** | `SchliereSPatch.hpp`, `SchliereSPatch` | as Z | as O, in 4 smoothed very lightly (12 kHz) | 39 % |

- **A (alt, "old")** is version 3 from 2026-11-18, the original. Audio-rate modulation of tap
  or feedback makes it glitchy and rough: part of the sidebands fold back at fs/2, the
  feedback amount is hard-limited, and the tap LEDs switch at the modulation rate (every
  switch clicks, measurement log D73).
- **Z (zahm, "tame")** is version 4. Tap and feedback CV start softly and are band-limited,
  the LEDs follow the knob only. Hardly any folding, but feedback modulation also adds
  hardly any new overtones.
- **W (wild)** is Z with less smoothing on the feedback modulation: almost as many new
  overtones as A, less folding, no hard limit.
- **O (oversampled)** computes the input of the chain at twice the sample rate. The feedback
  amount is unsmoothed as in A, but the products run at 96 kHz and are filtered afterwards.
  Most new overtones, much less folding than A. The loop gets almost 3 samples longer, so the
  feedback peaks sit slightly differently.
- **S (sanft, "gentle")** is O with very light smoothing: the CV at in 4 goes through two
  lowpass filters at 12 kHz each (≈ 7.7 kHz together) before O processes it. A tiny bit less
  glitch with feedback modulation, otherwise the same as O.
- **M (morph)** does not crossfade but changes the chain length continuously. The chain stays
  a pure allpass at every setting: tap modulation shifts the delay (slow: vibrato, fast:
  FM-like) instead of fading notches in and out. Out 4 is the sum (L + R)/2 here, its notches
  move with the tap. Too tame for the user, but still on slot 7.

Measured on the computer with the patch code (measurement log D80). "Folded" = lines created by
folding at fs/2, relative to the wanted signal:

| Case | A | Z | W | O | S | M |
|---|---|---|---|---|---|---|
| Tap CV 441 Hz, ±3 V, folded | −34 dB | −82 dB | −82 dB | −82 dB | −82 dB | −64 dB |
| Out 2 → in 4, 30 % feedback, 6 V: new overtones | −6 dB | −44 dB | −11 dB | +4 dB | −5 dB | – |
| same, folded | −19 dB | −70 dB | −32 dB | −33 dB | −43 dB | −49 dB |
| Out 2 → in 4, 70 % feedback, 6 V: new overtones | −10 dB | −38 dB | −11 dB | +1 dB | +3 dB | – |
| same, folded | −21 dB | −60 dB | −30 dB | −35 dB | −36 dB | −41 dB |

With 70 % feedback and 6 V the loop runs chaotically, there the values scatter by a few dB
from run to run.

On the device only the CPU load has been measured so far. How the variants sound is up to
listening.

### Controls (all variants)

| | |
|---|---|
| Knob 1 | Frequency, 20 Hz … 20 kHz: where the delay acts |
| Knob 2 | Resonance, Q 0.01 … 16 |
| Knob 3 | Tap: tap position (A, Z, W, O) or chain length (M) |
| Knob 4 | Feedback (A: 0 … 97 %, others 0 … 95 %, up to 97 % with CV) |
| In 1 | Audio (DC-coupled) |
| In 2 | Frequency CV, about 1 V/oct |
| In 3 | Tap CV, ±5 V = full travel |
| In 4 | Feedback CV, ±5 V = full travel, negative feedback below zero |
| Out 1, Out 2 | L (up to 98 poles), R (up to 122 poles) |
| Out 3 | Diff (L − R)/2 |
| Out 4 | Tail = end of the chain (A, Z, W, O) or sum (L + R)/2 (M) |
| LED 1–3 | Tap detent 1–7 of knob 3, binary, red |
| LED 4 | Red while the loop limiter works |

Details are in the header of `SchlierePatch.hpp`, for A in `SchliereAPatch.hpp`, for M in
`SchliereMPatch.hpp`, for O in `SchliereOPatch.hpp`, for S in `SchliereSPatch.hpp`, in German.

### Build and test

```bash
SRC=owl/schliere tools/build_owl_patch.sh SchliereZ SchlierePatch SchlierePatch.hpp
SRC=owl/schliere tools/build_owl_patch.sh SchliereA SchliereAPatch SchliereAPatch.hpp
SRC=owl/schliere tools/build_owl_patch.sh SchliereM SchliereMPatch SchliereMPatch.hpp
SRC=owl/schliere tools/build_owl_patch.sh SchliereW SchliereWPatch SchliereWPatch.hpp
SRC=owl/schliere tools/build_owl_patch.sh SchliereO SchliereOPatch SchliereOPatch.hpp
SRC=owl/schliere tools/build_owl_patch.sh SchliereS SchliereSPatch SchliereSPatch.hpp
tools/test_owl_dsp.sh
```

Builds go to `owl-patches/builds/<name>/` and are byte-identical to the slots on acdc1. W, O, S and M
live in files of their own: even a derived class in `SchlierePatch.hpp` changes the binary of
SchliereZ. The name is part of the binary, so the builds are named like the slots.
