# Schliere auf Rings portieren

Stand 2026-11-19. Anleitung für den Umzug des OWL-Patches **Schliere** (AC/DC, OpenWare) in eine
alternative Firmware für **Mutable Instruments Rings**, als zusätzlicher Modus neben den
Resonator-Modellen. Umgeschaltet wird über die Modus-Taste mit eigenem Blinkmuster.

Markierung wie im Ursprungsprojekt: **[Code]** = aus dem Quelltext abgeleitet, nicht auf Hardware
geprüft. **[Hypothese]** = Schätzung oder Schluss, ungeprüft. **[gemessen]** = am AC/DC oder am
Rechner gemessen (Messlog D80 im Ursprungsprojekt). **[offen]** = muss am Rings geklärt werden.

---

## 1. Inhalt des Pakets

```
ANLEITUNG.md                 diese Datei
owl/AcdcInput.h              Sperre für Störlinien des AC/DC (auf Rings nicht nötig, nur zum Kompilieren)
owl/schliere/README.md       die sechs Varianten (de/en): Belegung, Unterschiede, Messwerte
owl/schliere/SchlierePatch.hpp   gemeinsame Basis (SchliereBase) und Variante Z
owl/schliere/SchliereAPatch.hpp  Variante A (Fassung 3, das Original mit den Glitches)
owl/schliere/SchliereWPatch.hpp  Variante W (Feedback-Glättung 12 kHz)
owl/schliere/SchliereOPatch.hpp  Variante O (Feedback-Produkt mit 96 kHz Oversampling, Halbband-Filter)
owl/schliere/SchliereSPatch.hpp  Variante S (O mit ganz leicht geglättetem Feedback-CV)
owl/schliere/SchliereMPatch.hpp  Variante M (Morph, Länge der Kette stufenlos)
owl/test/Patch.h             Ersatz für die OWL-API, damit die Patches am Rechner laufen
owl/test/schliere_test.cpp   65 Host-Tests (Allpass gegen Lehrbuch, Faltung, Feedback, LEDs …)
```

Host-Tests am Rechner laufen lassen (im Paket-Ordner):

```bash
c++ -std=c++17 -O1 -Iowl/test -Iowl -Iowl/schliere -o schliere_test owl/test/schliere_test.cpp && ./schliere_test
```

Die Tests sind die Referenz für den Port: Was auf Rings läuft, soll bei gleichem Eingang dieselben
Zahlen liefern (bis auf die tanh-Näherung, siehe 4.3).

Die Quellen sind deutsch kommentiert, der Kopf von `SchlierePatch.hpp` beschreibt Algorithmus,
Belegung und alle Entscheidungen.

---

## 2. Machbarkeit

| | AC/DC | Rings | Folge für Schliere |
|---|---|---|---|
| MCU | STM32H750, Cortex-M7 | STM32F405, Cortex-M4F, 168 MHz | etwa ¼ der Rechenleistung |
| RAM | 1 MB + SDRAM | 192 KB | Schliere braucht ≈ 1–2 KB Zustand, kein Problem |
| Abtastrate, Block | 48 kHz, 8 | 48 kHz, 24 [Code: `rings/dsp/dsp.h`] | Formeln unverändert |
| Audio | 4 In, 4 Out, DC | 1 In, 2 Out (ODD, EVEN) | Diff und Tail fallen weg oder werden umschaltbar |
| Bedienung | 4 Regler, 4 CV | 5 Regler, 5 Attenuverter, 5 CV + V/OCT, Strum, 2 Tasten | mehr Möglichkeiten als am AC/DC |

**Rechenlast [Hypothese]:** Eine Stufe der Kette (Zustandsvariablenfilter, trapezförmig) hat
≈ 6 Multiplikationen und ≈ 5 Additionen in einer Abhängigkeitskette, auf dem M4 grob 20 Takte.
61 Stufen × 48 kHz ≈ 60 Mio. Takte/s ≈ 35 % von 168 MHz. Mit Überblendung, Ausgängen und bei
O/S dem Oversampling-Teil grob 40–50 %. Am AC/DC brauchen die Varianten 30–43 % [gemessen], die
Zahlen lassen sich aber nicht einfach umrechnen (anderer Kern, OWL-Rahmen, Block 8).
Zum Vergleich: Die alternative Rings-Firmware von Lyle Mills senkt für ihren „Mini-Elements“-Modus
die Abtastrate auf 32 kHz. Elements ist deutlich schwerer als Schliere.

**Der eine Stolperstein: `tanhf`.** Schliere ruft pro Sample fünfmal `tanhf` (weiche Knie an vier
Ausgängen und in der Schleife, bei O/S zusätzlich zweimal für das Knie der Feedback-Menge). Auf dem
M4 kostet libm-`tanhf` grob 100–300 Takte [Hypothese]. Das muss durch eine Näherung ersetzt werden
(4.3), sonst reicht die Rechenzeit nicht.

`sqrtf` ist auf dem M4 ein Hardware-Befehl. `expf`/`tanf` laufen nur einmal pro Block. `sinf`/`cosf`
braucht nur M (pro Sample, zweimal) – M ist ohnehin die am wenigsten gemochte Variante.

---

## 3. Vorbereitung

1. **Rings-Quellen:** `pichenettes/eurorack`, gebraucht werden `rings/`, `stmlib/` (eigenes Repo `pichenettes/stmlib`, im Klon als Submodul holen) und die
   Bootloader-Teile (`stm_audio_bootloader/`, laut `rings/makefile` Bootloader `rings_bootloader`)
   [Code]. Lizenz der Rings-Firmware: MIT (Kopf jeder Quelldatei) [Code].
2. **Erst die Original-Firmware bauen** und das Ergebnis aufheben, bevor irgendetwas geändert wird.
   Die Build-Kette stammt aus der Zeit von gcc 4.8. Mit einer aktuellen Arm-Toolchain (z. B. Arm
   GNU 12.2) sind vermutlich kleine Anpassungen nötig [Hypothese]. Jede Anpassung als eigenen
   Commit festhalten.
3. **Rückweg klären, bevor geflasht wird:** Die offizielle Firmware-WAV von Rings liegt bereit,
   und das Update-Verfahren aus dem Rings-Handbuch ist einmal mit der Original-Firmware erprobt.
   Rings hat einen Audio-Bootloader, das Update läuft über eine WAV-Datei in den Eingang
   [offen: genaue Prozedur und WAV-Erzeugung im Handbuch und im makefile nachsehen]. Alternativ
   SWD mit einem ST-Link.
4. **Profiling vorbereiten:** `rings/rings.cc` hat `PROFILE_INTERRUPT` mit `TIC`/`TOC` um
   `FillBuffer()` [Code]. Ohne Oszilloskop: DWT-Zykluszähler (`DWT->CYCCNT`) am Anfang und Ende von
   `FillBuffer()` lesen, Maximum merken und über eine LED oder den Debug-Port melden. Budget pro
   Block: 24 Samples / 48 kHz = 500 µs = 84 000 Takte.

---

## 4. Die DSP portieren

### 4.1 Von der OWL-API lösen

Schliere hängt an wenigen Stellen an OWL. Ersatz:

| OWL | Rings |
|---|---|
| `class … : public Patch` | eigene Klasse ohne Basis, z. B. `rings/dsp/schliere.h` |
| `processAudio(AudioBuffer&)` mit 4 Kanälen | `Process(const SchliereParams&, const float* in, float* out_l, float* out_r, size_t size)` |
| `getParameterValue(PARAMETER_A…D)` (0…1) | Felder in `SchliereParams`, aus `cv_scaler` gefüllt (4.4) |
| CV an In 2–4 pro Sample | Rings liest CV einmal pro Block [Code: `cv_scaler.Read()` pro `FillBuffer`]. Die Regler-/CV-Werte also pro Block übergeben und intern über den Block rampen (Schliere glättet ohnehin) |
| `setParameterValue(PARAMETER_AA…AD)` (LEDs) | weglassen, stattdessen Getter (`tap_point()`, `limiter_active()`) für die Rings-UI |
| `getSampleRate()`, `getNumberOfChannels()` | Konstanten (`kSampleRate`) |
| `AcdcBlockRateCanceller` (`AcdcInput.h`) | weglassen: Er entfernt Störlinien des AC/DC-Wandlers, Rings hat sie nicht |

Wichtig für Audio-Rate-Modulation: Am AC/DC wirken Tap- und Feedback-CV **pro Sample**. Auf Rings
kommen die CV-Werte nur einmal pro 24er-Block an [Code]. Audio-Rate-Modulation über die CV-Eingänge
wird dadurch auf ≈ 1 kHz begrenzt, und die Glitches von A und O klingen anders [Hypothese].
Wer pro Sample modulieren will, muss den ADC-Teil von Rings ändern. Für Feedback-Modulation mit
Audio bleibt dafür der Audio-Eingang: Man könnte zum Beispiel einen Untermodus anbieten, in dem
der IN-Eingang nicht ins Signal, sondern auf die Feedback-Menge geht.

### 4.2 Eine Klasse für alle Varianten

Statt sechs Klassen lohnt auf Rings eine Klasse mit Schaltern, damit die Modus-Taste die Variante
im Lauf umschalten kann. Die Varianten unterscheiden sich nur in diesen Punkten (aus den Quellen):

| Schalter | A | Z | W | O | S |
|---|---|---|---|---|---|
| Totzone der CV | hart (±0,5 V) | weich (0,45/0,65 V) | weich | weich | weich |
| Überblendung am Rand der Haltezone | linear | S-Kurve | S-Kurve | S-Kurve | S-Kurve |
| Glättung der Tap-Gewichte (2 Pole) | keine | 3 kHz | 3 kHz | 3 kHz | 3 kHz |
| Grenze der Feedback-Menge | hart, ±0,97 | weiches Knie ab 0,9 | weich | weich | weich |
| Glättung der Feedback-Menge (2 Pole) | keine | 1,5 kHz | 12 kHz | keine | Feedback-CV 12 kHz vor dem Knie |
| Feedback-Produkt mit 96 kHz Oversampling | nein | nein | nein | ja | ja |
| Rastpunkt-LEDs nach | CV + Regler | Regler | Regler | Regler | Regler |

M (Morph) ist ein anderer Kern (zwei Ketten, Teilstufe) und kostet am meisten. Für den Anfang
weglassen.

Hinweis: Bei A, Z, W, O wirkt die Feedback-Glättung **nach** der Kennlinie, bei S **vor** ihr. Auf
Rings kann man das vereinheitlichen (nach der Kennlinie ist sauberer, siehe Messlog D80 im
Ursprungsprojekt). Das S auf dem AC/DC hat die Glättung nur deshalb davor, weil O dort bytegleich
bleiben sollte.

### 4.3 tanh ersetzen

`soft()` in `SchlierePatch.hpp` ist: linear bis zum Knie, darüber
`knee + range · tanh((|x| − knee) / range)`. Ersatz für `tanh(u)` mit u ≥ 0, glatt und billig:

stmlib hat genau diese Näherung schon [Code: `dsp/dsp.h` im Repo `pichenettes/stmlib`, in
`eurorack` als Unterordner `stmlib/`]:

```cpp
inline float SoftLimit(float x) { return x * (27.0f + x * x) / (27.0f + 9.0f * x * x); }
inline float SoftClip(float x)  { /* ±1 außerhalb von ±3, sonst SoftLimit(x) */ }
```

`SoftClip(u)` statt `tanh(u)` einsetzen. Abweichung von tanh höchstens 0,024 (bei u ≈ 1,6), bei
u = 3 genau 1 und mit Steigung 0, also ohne Knick [gemessen, am Rechner nachgerechnet]. Die Knie
werden damit etwas härter als mit tanh. Die Host-Tests, die Pegel im Knie genau prüfen, brauchen
eine größere Toleranz.

### 4.4 Pegel

Auf dem AC/DC ist 1.0 = Vollausschlag ≈ 10 V. Die Knie liegen bei 7,5 V (Ausgänge) und 5 V
(Schleife). Rings wandelt Audio mit 16 Bit (`input[i].r / 32768`, Ausgabe über `Clip16`) [Code], der
Spannungsbereich am Eingang und an den Ausgängen ist [offen]. Nach dem ersten Test die Konstanten
`OUT_KNEE`, `OUT_CEILING`, `LOOP_KNEE` und `LOOP_CEILING` an den Rings-Pegel anpassen. Ob der
Rings-Eingang DC-gekoppelt ist, ist ebenfalls [offen]. Schliere braucht es nicht.

---

## 5. In Rings einbinden

### 5.1 Audio (`rings/rings.cc`)

`FillBuffer()` verzweigt schon für das Easter Egg [Code]. Daneben ein dritter Zweig:

```cpp
if (settings.state().schliere) {
  for (size_t i = 0; i < size; ++i) {
    in[i] = static_cast<float>(input[i].r) / 32768.0f;   // ohne das Noise Gate
  }
  SchliereParams sp;
  cv_scaler.ReadSchliere(&sp);                            // siehe 5.2
  schliere.Process(sp, in, out, aux, size);
} else if (settings.state().easter_egg) {
  …
```

`out` geht nach ODD, `aux` nach EVEN (die bestehende Schleife am Ende von `FillBuffer()`).

### 5.2 Regler und CV (`rings/cv_scaler.cc`)

Rings liefert die Regler gefiltert in `adc_lp_[…]` [Code]. `Patch` hat `structure`, `brightness`,
`damping`, `position` (0…1, Regler + CV mit Attenuverter) [Code: `rings/dsp/patch.h`]. Die lassen
sich direkt verwenden. Für Frequency **nicht** den Rings-Weg nehmen: Dort ist der FREQUENCY-Regler
auf Halbtöne gerastert (`transpose_`, Hysterese) und deckt nur 5 Oktaven ab [Code]. Schliere
braucht einen stufenlosen Regler über 10 Oktaven (20 Hz … 20 kHz). Vorschlag:

```cpp
// in einer neuen Methode ReadSchliere()
float octaves = 10.0f * adc_lp_[ADC_CHANNEL_POT_FREQUENCY] - 5.0f;      // −5 … +5 um 632 Hz
octaves += (calibration_data_->pitch_offset
            + adc_lp_[ADC_CHANNEL_CV_V_OCT] * calibration_data_->pitch_scale) / 12.0f;
octaves += fm_cv_ * adc_lp_[ADC_CHANNEL_ATTENUVERTER_FREQUENCY] / 12.0f;  // FM-Eingang
```

V/OCT ist bei Rings kalibriert [Code]. Das Ursprungsmodul (und der AC/DC) können das nicht: Mit Rings
spielt Schliere echt 1 V/Okt. [offen: Nullpunkt von `pitch_offset` prüfen, der Wert ist für
Rings-Noten gedacht.]

### 5.3 Belegung (Vorschlag)

| Rings | Schliere |
|---|---|
| FREQUENCY, V/OCT, FM + Attenuverter | Frequency (stufenlos, 1 V/Okt) |
| STRUCTURE + CV + Attenuverter | Tap |
| BRIGHTNESS + CV + Attenuverter | Resonance |
| DAMPING + CV + Attenuverter | Feedback (bipolar über den Attenuverter) |
| POSITION + CV + Attenuverter | frei: Stereo-Breite (Abstand der Abgriffe von L und R) oder Mischung trocken/nass |
| STRUM | frei: z. B. Kette löschen (Zustand auf null) oder Feedback einfrieren |
| IN | Audio |
| ODD / EVEN | L / R (oder per Polyphonie-Taste umschaltbar: L/R → Diff/Tail → L/Tail) |

### 5.4 Umschalten über die Modus-Taste (`rings/ui.cc`)

So macht es Rings heute [Code]:

- **Rechte Taste (Modell), kurz:** Modell 0 → 1 → 2 → 0. LED 2 zeigt grün / gelb / rot.
- **Rechte Taste, lang (`kLongPressDuration` = 3000 ms):** wechselt zwischen den Grundmodellen 0–2
  und den Bonusmodellen 3–5. Die Bonusmodelle zeigt LED 2 mit **pulsierendem Licht** (Dreieck über
  die PWM, `triangle`).
- **Linke Taste:** Polyphonie, lang = Polyphonie-Modus 3 (pulsierende LED 1).
- **Beide Tasten:** Kalibrierung. Lang drücken in einer bestimmten Reglerstellung
  (`cv_scaler_->easter_egg()`): Easter Egg.
- Im Code gibt es schon zwei Blinktakte: `blink` (Periode 128 ms) und `slow_blink` (256 ms).

Vorschlag für Schliere, nah an der Bedienung des Originals:

- **Lange Taste rechts schaltet in drei Bänke statt zwei:** Grundmodelle (LED ruhig) → Bonusmodelle
  (LED pulsiert) → **Schliere (LED blinkt hart an/aus, `slow_blink`)** → zurück zu den
  Grundmodellen.
- **Kurze Taste rechts in der Schliere-Bank:** Variante, Farbe der LED 2 wie bei den Modellen.
  Vorschlag: grün = S, gelb = O, rot = A (die drei Lieblinge). Z und W bei Bedarf über die linke
  Taste oder eine zweite Bank.
- **Linke Taste in der Schliere-Bank:** Polyphonie hat hier keine Bedeutung. Frei für die
  Ausgangsbelegung (5.3), LED 1 zeigt sie. Ein kurzes rotes Aufleuchten von LED 1 kann zeigen,
  dass der Begrenzer in der Schleife arbeitet (am AC/DC war das LED 4).
- Der Wechsel in die Schliere-Bank darf `part_->set_model()` nicht mit Werten über 5 aufrufen.
  Bank und Variante deshalb in eigenen Feldern speichern, `model` unverändert lassen.

So machen es andere [belegt]: Die alternative Rings-Firmware von **Lyle Mills** legt alles hinter
ein Optionsmenü. Beide Tasten etwa 3 s, links zuerst = Menü (rechts zuerst = Kalibrierung). Im Menü
blättert die linke Taste die Seiten, die rechte die Werte, die LEDs zeigen Seite und Wert in
Farben, ruhig oder blinkend. Seite 1 wählt den Grundmodus (Rings, Rings + Wellenform,
Mini-Elements, Easter Egg). Das ist flexibler als die lange Taste, aber umständlicher. Wer mehr als
drei Varianten und weitere Optionen will, ist mit so einem Menü besser dran. Ob dort Einstellungen
gespeichert werden, ist aus der README nicht ersichtlich.

### 5.5 Einstellungen speichern (`rings/settings.h`)

`State` hat `polyphony`, `model`, `easter_egg`, `color_blind` und dahinter `padding[20]` [Code].
Neue Felder aus dem Padding nehmen, die Größe der Struktur bleibt gleich:

```cpp
struct State {
  uint8_t polyphony;
  uint8_t model;
  uint8_t easter_egg;
  uint8_t color_blind;
  uint8_t schliere;          // 0 = aus, 1 = Schliere-Bank aktiv
  uint8_t schliere_variant;  // 0 = S, 1 = O, 2 = A …
  uint8_t schliere_outputs;  // Ausgangsbelegung
  uint8_t padding[17];
};
```

[offen: prüfen, dass im gespeicherten Zustand der Original-Firmware das Padding null ist. Sonst
nach dem Update einmal auf gültige Werte begrenzen.] Schliere und Easter Egg schließen sich aus.

---

## 6. Prüfplan

1. Original-Firmware bauen, flashen, läuft. Rückweg erprobt.
2. DSP-Klasse am Rechner gegen die Host-Tests aus diesem Paket (gleiche Testsignale, gleiche
   Erwartungen). Danach je Variante einmal gegen die OWL-Fassung: gleiches Eingangssignal, Ausgänge
   vergleichen (Abweichung nur durch tanh-Näherung und Blockgröße).
3. Firmware mit Schliere bauen, Rechenzeit messen (3., Abschnitt 4): Spitze unter ≈ 80 % des
   Budgets, mit Reserve für UI und ADC.
4. Am Modul: Tasten und LEDs, Speichern über Aus/Ein, Pegel (4.4), V/OCT-Tracking (z. B. Frequency
   mit Resonance hoch über mehrere Oktaven).
5. Hören: Tap- und Feedback-Modulation, Selbst-Patch eines Ausgangs auf einen CV-Eingang.

---

## 7. Lizenz

Die Rings-Firmware steht unter MIT [Code]. Die Schliere-Quellen stammen aus dem Ursprungsprojekt
`acdc-workbench`. Dort ist `owl/` in der Lizenztabelle nicht aufgeführt. Vor einer Veröffentlichung
die Lizenz für Schliere festlegen (MIT würde zu Rings passen).

---

## Quellen

- Rings-Firmware: https://github.com/pichenettes/eurorack (Ordner `rings/`, `stmlib/`), gelesen:
  `rings/rings.cc`, `rings/ui.cc`, `rings/ui.h`, `rings/cv_scaler.cc`, `rings/cv_scaler.h`,
  `rings/settings.h`, `rings/dsp/dsp.h`, `rings/dsp/part.h`, `rings/dsp/patch.h`,
  `rings/dsp/performance_state.h`, `rings/drivers/adc.h`, `rings/makefile`
- Alternative Rings-Firmware von Lyle Mills (Optionsmenü, Mini-Elements, 32 kHz):
  https://github.com/lylepmills/eurorack/tree/master/alt_firmwares
- Bericht dazu: https://www.matrixsynth.com/2026/03/mutable-instruments-rings-alternative.html
- Schliere, Messungen und Entscheidungen: Messlog D66–D80 im Ursprungsprojekt
