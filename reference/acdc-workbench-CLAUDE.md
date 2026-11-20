# AC/DC Noise & Firmware – Projektkontext

Werkbank für mein **Befaco / Rebel Technology AC/DC** (Eurorack-USB-Audiointerface, OpenWare-Firmware). Anlass war das Rauschen des Moduls. Ziele, in dieser Reihenfolge:

1. **Werkbank:** Gerät auslesen, automatisch aufnehmen, Spektren auswerten, Firmware reproduzierbar bauen, sicher flashen. Hier liegt inzwischen der größte Nutzen (Nutzer, 2026-11-16).
2. **Störlinien verstehen und senken:** Rauschen und digitale Störlinien in den Aufnahmen. Hörbar war es immer, vor allem mit Kompressor auf der Spur.
3. **Standalone-FX und Interface:** Ohne Rechner läuft das AC/DC als FX, mit Rechner weiter als normales Audiointerface.

## Geräte

Ich habe **zwei AC/DC-Module**. **acdc1 = Testgerät**, **acdc2 = Referenz** (nicht flashen, nichts speichern, nichts löschen).

| Modul | Device-UUID (als Seriennummer) | Firmware | Slot 1 | Stand |
|---|---|---|---|---|
| acdc1 | `0039001c:31315101:39333437` | **Seit 2026-11-19, 11:46 geflasht: v22.4.r-fx7a** (Build 9, `cac6fee0…` = fx6a + Patch 18, LED-PWM 48 kHz, Abnahme vollständig, D78, D79, **Alltags-Firmware seit 2026-11-19**). Davor und Rückweg: **v22.4.r-fx6a** (Build 8, `6a3fc427…` = fx5a + Patch 17, LED-Modus, **Abnahme vollständig, Entscheidung offen**, D75–D77). Davor und Rückweg: **v22.4.r-fx5a** (eigener Build 7, `b69c25b3…` = fx4b + Patch 16, Alltags-Firmware seit 2026-11-18, D63–D65). Erster Rückweg fx4b `v22.4.r-fx4b` (Build 6b, `a21f35c9…`, Alltags-Firmware 2026-11-17 bis 18, D55–D59). Danach fx3l `v22.4.r-fx3l` (Build 5c, `b55b94d8…`), danach Build 3d `v22.4.r-fx2d` (`d212cf8d…`) | ACDC, 9,93 KB. Slots 2–11: TapeDelay, Grainz, RingsReverb, Tanh3 (D54), SchliereA (Fassung 3), SchliereM, SchliereZ (Fassung 4), SchliereW, SchliereO, SchliereS (alle D80) | owl_read 2026-11-19 |
| acdc2 (DIY) | `00250049:31315102:39333437` | v22.4.q (`e81cfc3`, Hack-Branch, nie veröffentlicht) | ACDC, 9,32 KB | Device-Status 2026-11-13 |

- **acdc2** = die **DIY-Variante** (M1–M3 wurden damit aufgenommen). „Gerät 1“ in älteren Docs = acdc1, „Gerät 2“ = acdc2.
- acdc1 trug ab Werk einen Hack-Branch-Build mit dem Etikett „v22.5.0“, nicht das offizielle Release (D3/D3a). Das Abbild davon liegt in `backup/acdc1/20261114/flash/acdc1_internal_flash.bin` (`433b3fd3…`) und ist der letzte Rückweg.
- Die offizielle GitHub-`ACDC.bin` v22.5.0 ist **kein** Rückweg: Laut antisvin (Thread 2083 #4) macht sie USB kaputt.
- Von acdc2 gibt es bisher kein Abbild. v22.4.q soll nicht verloren gehen (TODO „Werkzeuge“).
- Vor jedem Eingriff in ein Gerät: Settings und Slots auslesen und sichern.

## Aufbau im Alltag

- Zweites Interface: **Behringer UMC202HD**, hängt am selben Thunderbolt-Hub wie das AC/DC. Verträgt **keine Eurorack-Pegel** an den Eingängen.
- **Signalfluss (Nutzer, 2026-11-15):** AC/DC-Eingänge = Aufnahmen aus dem Eurorack. AC/DC-Ausgänge = CV oder Audio, das zurück ins Rack und wieder in die AC/DC-Eingänge geht. Der UMC ist nur Ausgabe an Boxen/Kopfhörer. **AC/DC-Out → UMC-In gibt es im Alltag nicht** (nur Messaufbau S3/G).
- **USB-Kette:** AC/DC → kleiner passiver USB-Hub → USB-Hub im Monitor → Thunderbolt-Hub. Der Mac wird über den TB-Hub versorgt, der Monitor hängt am TB-Hub.
- **Nur ein Rechner** vorhanden. Tests mit Host-Wechsel fallen weg.
- **Einbau:** Bis 2026-11-18 lag acdc1 bei allen Messungen und beim Hören neben dem Case. Seit dem 2026-11-18 (abends) ist es ins Case geschraubt (Nutzer, D66 Nachtrag). Bei neuen Messungen den Einbau notieren.
- Forum-Handle: `m9dfukc` (community.rebeltech.org), eigener Thread: #2244.

## Aktueller Stand (2026-11-19)

- **Auf acdc1 läuft seit 2026-11-19 Build 9 `v22.4.r-fx7a`, Abnahme vollständig (D78, D79). fx7a ist die Alltags-Firmware (Entscheidung des Nutzers, 2026-11-19), Rückweg ist fx6a:** fx6a + Patch 18, LED-PWM mit 48 000 Hz statt 2320,54 Hz, im Takt der Abtastung (Timer und SAI aus demselben Quarz, 0,1 ppm Abstand). Nur der Timer-Init ändert sich, `setLed()` und der ITCM bleiben bytegleich, LED-Werte unter 2,4 % bleiben dunkel. Gemessen: Die LED-PWM-Linie ist am Ausgang (gedimmtes Grün −85 statt −39,8 dBFS am UMC) und in den rohen Eingängen weg, Rauschen gleich fx6a, LED-Verschiebung unverändert, Loop-Test bestanden. Rückweg ist fx6a.
- **Build 8 `v22.4.r-fx6a` (seit 2026-11-18 auf acdc1, heute Rückweg von fx7a), Abnahme vollständig (D75–D77).** Neu ist nur der LED-Modus (Patch 17): im Boot-Menü wählt Regler 2 normal (linkes Drittel), nur Clip (Mitte) oder aus (rechts), gemerkt in der Ressource `ACDC.led`.
  - Bestanden: Flash mit Readback, A1, Modus aus (Verschiebung der Eingänge auf 1,7 %, etwa −37 dB), Modus normal wie fx5a.
  - Bestanden am 2026-11-19 (D75, Nachtrag): Nach Aus und Ein ohne Menü ist ein dunkler Modus aktiv, die Eingänge bleiben ohne Stream ruhig (Blinken 0,3 % von normal).
  - Bestanden am 2026-11-19 (D75, zweiter Nachtrag): Gemerkt ist nur Clip. Bei der Clip-Probe springen die offenen Eingänge an der Clip-Schwelle der Firmware, 220 ms vor dem Anschlag des Wandlers, um −2,0e-4 Vollausschlag (am Vorabend −1,1e-4): Das ist die rote LED 1, nicht das Übersteuern. Der Nutzer hat die rote LED bei einer Wiederholung gesehen.
  - Bestanden am 2026-11-19 (D75, dritter Nachtrag): nur Clip frisch aus dem Menü ist ohne Stream ruhig. Die LED-Modi sind damit am Gerät abgenommen.
  - Bestanden am 2026-11-19 (D75, vierter Nachtrag): Download-Stresstest ohne Stream, 240 von 240 in Ordnung.
  - Bestanden am 2026-11-19 (D76): Loop-Test mit Slot 1, mit Tanh3 und während 48 Downloads (0 Slips, 0 veraltete Samples).
  - **Neuer Fehler (D76), vermutlich älter als Build 8:** Ein Program Change bei offenem Stream bricht die USB-Wiedergabe bis zum nächsten Stream-Start (rund 10 % Stille an allen Ausgängen, `fb clamp`). Patch deshalb nur ohne offenen Stream wechseln. Unter fx5a genauso (D77), der Fehler ist also älter als Build 8.
  - Bestanden am 2026-11-19 (D77): Rauschen gegen fx5a ohne Hinweis auf einen Unterschied (fx5a liegt zwischen zwei fx6a-Kontrollen, die Sitzung driftet um 2 dB). **Die Abnahme ist vollständig, über die Alltags-Firmware ist nicht entschieden, Rückweg ist fx5a.**
  - Zustand am 2026-11-19, 11:23: fx6a nach zwei Flashes (fx5a und zurück), LED-Modus normal, es läuft Slot 1 per Program Change, gemerkt ist Slot 6 (Schliere), kein Kabel gesteckt.
  - Alles Folgende zu fx5a gilt auch für fx6a im Modus normal.
- **Davor lief seit 2026-11-18 fx5a** (`firmware/builds/acdc1-build7-fx5a/`, `b69c25b3…` = fx4b + Patch 16, MIDI-Sendeweg). Abnahme D62–D64: Download-Stresstest 240 von 240 in Ordnung, Loop-Test bestanden, Rauschen ohne Hinweis auf eine Änderung (Mischprodukte nicht entscheidbar). **fx5a ist die Alltags-Firmware (Entscheidung des Nutzers, D65)**, fx4b ist der erste Rückweg. Alles Folgende zu fx4b gilt auch für fx5a.
- **Davor war fx4b die Alltags-Firmware** (Entscheidung des Nutzers, D59): `firmware/builds/acdc1-build6b-fx4b/`, `a21f35c9…` = fx3l + Patch 15b. Abnahme bestanden: Boot-Menü am Gerät (D52/D53), Loop-Test auch unter 53 % Patch-Last (D57), Rauschen gleich fx3l innerhalb ±0,4 dB (D58).
- **Boot-Menü zur Patch-Wahl:**
  - Einschalten mit Regler 1, 2, 3 links und Regler 4 rechts öffnet das Menü. Regler 1 wählt unter den belegten Slots (zählt erst nach Bewegung), Regler 4 weg vom Anschlag bestätigt, Regler 3 weg vom Anschlag oder 30 s brechen ab.
  - Eingangs-LEDs zeigen den Slot binär (1–15, LED 1 = 1 … LED 4 = 8). Der gewählte Slot wird als Ressource `ACDC.slot` im Patch-Flash gemerkt, die Settings bleiben unberührt.
  - Ohne das Muster startet der gemerkte Slot, seine Nummer steht 1 s auf den LEDs.
  - Regler-Endwerte: links 2, rechts 127 von 127 (D51). Einzelheiten: `docs/05`, „Build 6“.
- **Verhalten sonst wie fx3l:**
  - Der Rechner nimmt die **rohen Eingänge** auf (`TX_PRE_FX`, vor dem Patch, der Gain-Knob wirkt nicht). Audiogerät „OWL-ACDC-FX“, der MIDI-Port heißt in CoreMIDI weiter „OWL-ACDC“.
  - Ausgang ohne Stream = Patch (Durchleitung ohne Rechner geht). Ausgang mit Stream = USB-Playback.
  - Nimmt eine App vom AC/DC auf, startet macOS auch den Playback-Stream. Dann ist die Durchleitung aus (D20).
  - Mit TX-Stream zeigen die Eingangs-LEDs den rohen Pegel, Rot ab −0,1 dBFS für 300 ms (D36).
  - Patch-Heap im CCM 39,2 KB statt 52 KB, weil der heiße Pfad im ITCM liegt.
- **Rauschen gegen 3d [gemessen, D48, gilt auch für fx4b, D58]:** fs/8 −5,1 dB, Mischprodukte −35 dB, Rest −1,25 dB, RMS −1,1 dB, Kamm +0,2…+0,4 dB. Der SOF-Kamm ist damit die stärkste verbleibende Störfamilie.
- **Auf acdc1 liegen elf Patches** (D54, D56, D66, D80): Slot 1 ACDC, 2 TapeDelay, 3 Grainz, 4 RingsReverb (je mit Limiter auf Out 1/2), 5 Tanh3 (Fassung 2 mit 4× Oversampling, 48 % CPU), 6 SchliereA (Fassung 3, „alt“, das Original mit den Glitches, die der Nutzer mag, D80 zweiter Nachtrag), 7 SchliereM (Tap ändert die Länge der Kette stufenlos, 49–54 % CPU je nach Tap, D80), 8 SchliereZ (Kette aus 61 Allpässen, Fassung 4 mit bandbegrenzter Tap- und Feedback-Modulation, „zahm“, 33 % CPU, seit 2026-11-19, D66–D71, D80), 9 SchliereW (Fassung 4 mit Feedback-Glättung 12 kHz statt 1,5 kHz, Zwischenstufe zwischen 3 und 4, 32–36 % CPU, D80 dritter Nachtrag), 10 SchliereO (Feedback ungeglättet, aber mit 96 kHz gerechnet: mehr Obertöne als Fassung 3, 10–15 dB weniger Faltung, 43 % CPU, D80 vierter Nachtrag), 11 SchliereS (O mit ganz leicht geglättetem Feedback-CV, 12 kHz je Pol, 39 % CPU, D80 siebter Nachtrag). Gemerkter Slot zuletzt 5. Die Builds aus D50 und Tanh3 Fassung 1 liegen in `owl-patches/builds-owl2/`. Wechsel vom Rechner aus per `owl_read.py --program N` (wird nicht gemerkt).
- **Slot-Downloads bei laufendem Patch sind bis fx4b unzuverlässig (D53/D54, D62):** Wettlauf im MIDI-Sendeweg der Original-Firmware [Code]. Auf fx4b waren 19 von 79 Downloads beschädigt, danach sendete acdc1 bis zum Neustart kein MIDI mehr [gemessen, D62]. **fx5a behebt das (Patch 16): 240 von 240 Downloads in Ordnung, kein Hänger (D63).** Unter fx4b und älter weiter `owl_write.py` mit `--backup-file` benutzen und bei `owl_read.py --dump` auf `ok` im Report achten. Test: `tools/dump_stress.py`.
- **Offen:** Störung auf Kanal 2 während Downloads (D63), Mischprodukte im Rausch-A/B nicht entscheidbar (D64). **Fiepsen in Schliere:** Fassung 1 fiepst durch die LED-PWM (2321 Hz mit Vielfachen an Out 1, bis −44 dBFS am UMC, folgt dem LED-Muster, D67). Fassung 2 (LEDs nur aus oder rot) senkt die Linie um 31 dB (D68). Was dann noch fiept, sind die Blockraten-Linien 6/12/18 kHz und der Kamm aus den offenen Eingängen, die Rechenlast von Schliere hebt fs/8 um 3,8 dB (D69). Fassung 3 hat dagegen eine Sperre am Eingang (`owl/AcdcInput.h`): an den Aufnahmen aus D69 −42…−47 dB bei fs/8 (D70), am Gerät an Out 1 gemessen −22…−25 dB (D71). Stärkste Linien am Ausgang sind danach die des USB-Kamms.  acdc1 ist seit 2026-11-18 erstmals ins Case geschraubt. **Schliere Fassung 4 und Schliere M (D80):** Tap- und Feedback-CV bandbegrenzt, gefaltete Linien bei Audio-Rate am Rechner um 30–50 dB kleiner, Rastpunkt-LEDs nur nach Regler 3. Alle Varianten liegen auf acdc1 (Slot 6 SchliereA = Fassung 3, 7 SchliereM, 8 SchliereZ = Fassung 4, 9 SchliereW, 10 SchliereO, 11 SchliereS), Quellen in `owl/schliere/` mit `README.md` (de/en), Builds in `owl-patches/builds/Schliere<Buchstabe>/`, bytegleich mit den Slots (ältere Builds in `Schliere-v1/` bis `-v3/`). Am Gerät nicht gemessen und nicht gehört, Rechenlast höchstens 54 % (Schliere M, Tap am Rechtsanschlag). Hörurteil zu Tanh3 und Schliere, Schliere am Gerät nachmessen (D66), Quelle der Regler-Linien (D61), Last von Tanh3 (D56). Alles Weitere: `TODO.md`, Abschnitt „Offen“.

## Dateien

| Datei | Inhalt |
|---|---|
| `TODO.md` | Offene Punkte nach Themen, Entfallenes, Chronik – **hier anfangen** |
| `README.md`, `README.de.md` | Einstieg für Außenstehende (englisch und deutsch): Werkzeuge, Befunde, Aufrufe. Bei neuen Befunden oder Werkzeugen beide nachziehen |
| `LICENSE.md`, `LICENSES/` | Lizenzen: Werkzeuge MIT, Patches GPL-2.0, Docs und Messdaten CC BY 4.0 |
| `docs/00-ausgangsfragen-und-verlauf.md` | Meine Ausgangsfragen (sprachlich überarbeitet), mein Forum-Thread, Recherche-Verlauf, Kurzantworten, Claudes Anmerkungen. Stand 2026-11-14, wird nicht fortgeschrieben |
| `docs/01-hardware-und-firmware.md` | Fakten zum Gerät: MCU, Codec, USB, Audio-Routing im Code, MIDI-Header auf der Rückseite (mit Skizze einer Zwischenplatine) |
| `docs/02-noise-befunde.md` | Was im Forum bekannt ist, Hypothesen zur Ursache mit Messstand, Isolator-Frage |
| `docs/03-messplan-python.md` | Messprotokoll + Spezifikation der Python-Analyse (uv) |
| `docs/04-software-filter.md` | Notch-Bank: Filterform, Python-Prototyp, Plugin, FAUST-Patch |
| `docs/05-firmware-plan.md` | OpenWare-Stand, Forks, Build-Plan und Ergebnis jedes Builds (chronologisch), Fork, Flashen |
| `docs/messlog.md` | **Alle eigenen Messungen** (M1 … D77): Setup, Dateien, Linien-Tabelle, Deutung, Einschränkungen |
| `docs/uebergaben/` | Falls es Übergaben zwischen Sitzungen gibt, liegen sie hier. **Nur lokal, nicht im Repo** (Nutzer-Entscheidung 2026-11-18): Arbeitszettel, die schnell überholt sind. Die Datei mit dem jüngsten Datum ist die aktuelle. Nie löschen, nicht committen. `CLAUDE.md` und die anderen Docs verweisen nicht auf einzelne Übergaben, Dauerhaftes gehört nach `TODO.md`, `CLAUDE.md` oder in den Messlog |
| `docs/99-quellen.md` | Alle Links |
| `docs/en/` | Englische Fassungen von `01`–`05` und `99`, aus dem Deutschen erzeugt. Nie von Hand ändern. Übersicht, Marker-Legende und Glossar in `docs/en/README.md` |
| `analysis/` | Analyse-Tool: `uv run python -m analysis <ID\|wav…> [--ref …]`, Selbsttest `python -m analysis.selftest`. Module und Grenzen: `docs/03-messplan-python.md` |
| `recordings/manifest.toml` | Metadaten jeder Messung (ID, Gerät, Gain, USB, Patch, Dateien, Notiz). **Neue Aufnahmen hier eintragen** |
| `recordings/*.wav` | Rohaufnahmen nach Namenskonvention (M1–M3 in `_legacy/`: 96 kHz / 16 Bit, mono pro Kanal) |
| `out/<ID>/` | Report, JSON, Plots je Messung. Dazu `out/*-session-table.md` (Sitzungstabellen), `out/owl_read/` (Geräte-Reports je Aufnahme), `out/v0/` (alte Plots des ersten Skripts, überholte Gain-Deutung im Text) |
| `firmware/` | Firmware-Arbeit: Quell-Klon, Quelltext-Patches, Builds, Flash-Logs. Übersicht in `firmware/README.md` |
| `backup/<gerät>/<datum>/` | Geräte-Backups: `flash/` (DFU-Readback), `owl/` (Slots, Settings, Report). Nie überschreiben. Nur lokal, nicht im Repo |
| `owl/` | Eigene OWL-Patch-Quellen: `Tanh3Patch.hpp` (dreifacher tanh-Waveshaper mit Mix-Kanal), `schliere/` (Schliere: Kette aus 61 Allpässen 2. Ordnung, mono hinein, stereo heraus. Sechs Varianten A, Z, W, O, S, M in je einer Datei, Übersicht de/en in `schliere/README.md`, D66–D80), `AcdcOutput.h` (Limiter-Hülle für FX-Patches), `AcdcInput.h` (Sperre für die Blockraten-Linien 6/12/18 kHz am Eingang, lernt in der Stille, D70/D71), `test/` (Host-Tests und die Mess-Patches `SchliereLedTestPatch.hpp` (LEDs nach der Uhr, D73) und `SchliereLedPathPatch.hpp` (Eingang oder Ausgang, misst selbst an den Eingängen, D74)). Der Ordnername bleibt, trotz der Nähe zu `owl-patches/` (Nutzer 2026-11-18) |
| `owl-patches/`, `vcv-patches/` | Nur lokal, nicht im Repo. `owl-patches/library/`: Quellen von OWL-Patches aus der Library (ohne Lizenz). `owl-patches/acdc-patches/` (`burstMembrane` `78ffa22`), `owl-patches/OwlProgram/` (SDK `60f44ab` + DaisySP `4263388`), `owl-patches/downloads/` (Tarballs), `owl-patches/builds/<name>/` (`patch.bin`, `<name>.syx`, `BUILD_INFO.txt`). `vcv-patches/`: Host-Patches der frühen Messungen |
| `tools/` | Geräte- und Build-Werkzeuge, siehe unten |
| `docs/research/report-2026-10-23.md` | Vollständiger Recherchebericht (Deep Research) |
| `docs/research/notes/*.md` | Rohnotizen der Recherche, englisch, mit allen URLs und Commit-Hashes: Forum-Berichte, Ground-Loop/Filter, OpenWare & Forks, Standalone-FX, Fork-Update 2026-11-14, Code-Lage ↔ Rauschen (`layout_noise_2026-11-16.md`), ITCM-Technik (`daisy_itcm_2026-11-16.md`), Upstream nach 2023 (`upstream_master_2026-11-16.md`), MIDI-Header, serielles MIDI, I2C und USB-Host (`midi_header_2026-11-18.md`) |

Bei Detailfragen zuerst `docs/research/notes/` durchsuchen, bevor neu recherchiert wird.

### Werkzeuge in `tools/`

| Werkzeug | Zweck |
|---|---|
| `acdc_rec.py` | Aufnahmen und Testsignale, Standard seit 2026-11-16. Auch für andere Geräte (`--device UMC202HD --input-only --channels 2`). `--prompt --progress 1` im Terminal des Nutzers gibt Start per Enter und eine Zeitanzeige im Sekundentakt |
| `owl_read.py` | Gerät per MIDI/SysEx auslesen (nur lesend), Neustart in den DFU-Modus |
| `owl_write.py` | Patch hochladen: `--run` (nur RAM) oder `--store N --yes` (Slot im Patch-Flash). Nur acdc1, sichert einen belegten Slot vorher, liest danach zurück. `--erase N --yes` löscht einen Slot ab 2, nur mit passender lokaler Kopie als PATCH (am Gerät noch nicht erprobt, D80). `--info` und `--syx-out` laufen offline |
| `build_owl_patch.sh` | C++-Patch mit OwlProgram bauen, SysEx nach `owl-patches/builds/` |
| `sim_boot_menu.sh` | Host-Test der Boot-Menü-Logik (Patch 15/15b, ab Build 8 auch Regler 2 für den LED-Modus) mit simulierten Reglern, dazu ab Build 8 die LED-Schleife je Modus (`firmware/sim/led_loop_sim.cpp`) |
| `test_owl_dsp.sh` | Host-Test der eigenen Patch-Quellen in `owl/` (Tanh3, Limiter, Schliere, Blockraten-Sperre, beide LED-Mess-Patches) |
| `dfu_readback.sh`, `compare_firmware.py` | Internes Flash lesen und mit einer Referenz vergleichen |
| `dump_stress.py` | Download-Stresstest für den MIDI-Sendeweg: lädt bei laufendem Patch alle Slots in Runden herunter und zählt beschädigte Downloads. Nur acdc1, nur lesend bis auf einen Program Change. Läuft wie `owl_read.py` mit Python 3.12 und mido. Bricht ab, wenn das Gerät nicht mehr antwortet (D62) |
| `flash_acdc.py` | Build auf acdc1 flashen: Offline-Prüfung, Auslesen, DFU-Neustart, Schreiben, Readback-Vergleich, Flash-Log. Danach Start der Firmware per `:leave` und A1 (D79), `--cold` für den Neustart von Hand, dann `--a1`. Ohne `--yes` nur die Offline-Prüfung |
| `build_acdc.sh` | Reproduzierbarer Firmware-Build aus Commit + Libraries-Tarball + Patches |
| `setup_repo.sh` | Frischen Klon einrichten: Submodul, `upstream`-Remote, Libraries-Tarball und offizielle `ACDC.bin` mit SHA-Prüfung |
| `i18n_check.py` | Listet veraltete englische Fassungen in `docs/en/` auf (Prüfsumme der deutschen Quelle im Kopf). `--stamp` schreibt den Kopf nach einer neuen Übersetzung |
| `check_itcm.py` | ITCM-Build am ELF prüfen (`--groups` für Varianten, `--same-itcm` für Füllvarianten) |
| `phase_slips.py`, `stale_samples.py` | Loop-Aufnahmen auf Sample-Slips und einzelne veraltete Samples prüfen |
| `family_table.py` | Familienpegel mehrerer Messungen, Δ gegen die Kontrolle |
| `crosstalk_lines.py`, `click_sensitivity.py` | Übersprechen der Ausgänge, Empfindlichkeit des Klick-Detektors |
| `line_levels.py` | Pegel einzelner Linien je Kanal direkt bei der Linienfrequenz, für mehrere Aufnahmen, mit Abstand zum Grundrauschen. Trifft die Handwerte aus D58 auf 0,1 dB |
| `knob_series.py` | Geführte Aufnahmeserie mit Reglern in vorgegebenen Stellungen: liest CC 20–23, wartet auf die Stellung, nimmt mit `acdc_rec.py` auf, am Ende Tabelle `out/<serie>-session-table.md`. Läuft wie `owl_read.py` mit Python 3.12 und mido, `--simulate` ohne Gerät |

## Konventionen

- Python über **uv** (`uv run`, `uv add`). Analyse-Code in `analysis/`, Aufnahmen in `recordings/` (WAV, nicht committen), Plots/Reports in `out/`.
- Aufnahmen benennen: `YYYYMMDD_<geraet>_<test>_<gain>_<usb-zustand>.wav`, z. B. `20261113_acdc1_open-inputs_g12_usb-stream.wav`.
- **Aufnahmen immer mit `tools/acdc_rec.py`** (Nutzer-Entscheidung 2026-11-16). Die frühen Messungen liefen über eine Host-Software mit Mehrkanal-Recorder. Die Gegenprobe beider Wege ist bestanden (D35).
  - Das Skript öffnet das Gerät duplex mit Nullen am Ausgang, 48 kHz, Block 256 (wie die frühere Host-Software). Es schreibt 4-Kanal-WAV 16 Bit nach Namenskonvention und legt den Manifest-Eintrag an (`scale = 2.0` → Datei-dBFS wie die frühen Aufnahmen).
  - Optional: `--reps` mit Neuöffnen dazwischen, `--owl-read` vor dem Schließen, `--analyze --ref …`. Länge, Gerät, Kanäle, Format usw. sind Argumente (`--help`).
  - Es läuft nur außerhalb des Sandbox (CoreAudio): `UV_CACHE_DIR=.uv-cache uv run python tools/acdc_rec.py --list`.
  - **Testsignale:** `--sine F1 F2 F3 F4` schickt je Ausgang einen echten Sinus (0 = still), Pegel `--sine-level` in Geräte-dBFS (Standard −6 ≙ 5 V Spitze am Ausgang), `--sine-fade` blendet ein. **Gleichanteil:** `--dc V1 … V4` in Vollausschlag (−1…1, 1 ≈ 10 V am Ausgang), `--dc-ramp SEK` fährt langsam hoch. `--simulate` + `--sine` = Loopback ohne Gerät.
  - Eine Host-Software (DAW oder modulare Umgebung) nur noch, wenn ein Patch im Rechner nötig ist.
- **Messungen mit offenen Eingängen doppelt aufnehmen** (2 × 60 s). Der Kamm streut zwischen Läufen um ≈ 0,4 dB (D35), einzelne Ausreißer gab es auch (D22/D23).
- **A/B zwischen Builds immer gegen eine Kontrolle in derselben Sitzung**, am besten am Anfang und am Ende. 3d ist in einer Sitzung schon um 0,4–0,8 dB gedriftet (D44).
- Jede Messung bekommt einen Eintrag in `docs/messlog.md`: Setup, Datei, Befund, Fazit.
- **Keine fremde Software beim Namen nennen** (Nutzer 2026-11-16): In Docs, Manifest und Werkzeugtexten stehen neutrale Begriffe wie „Host-Software“, „Mehrkanal-Recorder“, „Aufnahmeprogramm“. Ausnahme ist, was das Projekt selbst braucht (uv, dfu-util, Arm-Toolchain, OpenOCD, FAUST/OwlProgram).
- Aussagen in den Docs sind markiert:
  - **[belegt]**: Quelle wie Forum, Repo oder Hersteller vorhanden.
  - **[Code]**: aus dem OpenWare-Quelltext abgeleitet, nicht auf Hardware geprüft.
  - **[Hypothese]**: Schlussfolgerung, ungetestet.
  - **[gemessen]**: durch eine eigene Messung bestätigt oder widerlegt, mit Verweis auf den Messlog.

  Diese Markierung bitte beibehalten und nur mit Verweis auf den Messlog hochstufen.

## Wichtige Fakten (Kurzform)

- MCU sehr wahrscheinlich **STM32H750IB** (laut Build-Config, 128 KB internes Flash, Patches im QSPI). Die `.ioc` sagt H743, das ist ein Prototyp-Erbe.
- Codec **TI PCM3168A**, SAI1 TDM. Das Codec-Hochpassfilter ist aus (DC-gekoppelt). SAI1 liefert die ADC-Wörter 24 Bit rechtsbündig **ohne Vorzeichen** (`SAI_DATASIZE_24`) [Code, D28].
- **USB Full Speed**, UAC 1.0, 4 in / 4 out, 16 Bit, 48 kHz, async mit Feedback-Endpoint.
- **Samplerate je Firmware:** acdc1 exakt 48 kHz (Takt von v22.4.r), acdc2 mit v22.4.q ≈ 47 991 Hz, wie v22.5.0 [Code, gemessen: D10].
- Bekanntes Rauschbild im Forum (atau, Thread #2072): **Kamm bei k·1 kHz**, ca. −30 dB bei Gain 12 Uhr. Bleibt trotz USB-Isolator und Laptop im Akkubetrieb.
- Upstream: Letzter Firmware-Stand ist **v22.5.0 (2023-03-05)**. Am 2026-11-15 kam `master` = `36eaf1c4` dazu, das ändert nur die Build-Kette (GCC 13+). Einen offiziellen Fix für das Rauschen gibt es nicht.
- Der Gain-Knob ist im Default-Patch ein **digitaler** Gain. Mit `TX_PRE_FX` (acdc1) wirkt er nicht mehr auf die Aufnahme.
- Eingangsbereich ≈ ±9,5 V Vollausschlag [gemessen, indirekt, D29], Spezifikation ±10 V. Eingangsoffset ≈ −0,2…−0,4 V je Kanal (D36).
- **Pegel-Offset der frühen Aufnahmen (D7/D8):** Aufnahmen über die frühere Host-Software tragen einen Faktor 2 (+6 dB) zwischen Gerät und Datei („Host-Faktor 2“). Datei-dBFS − 6,02 dB = Geräte-dBFS. `acdc_rec.py` bildet das mit `scale = 2.0` nach, damit alte und neue Dateien vergleichbar bleiben.

## Befunde zum Rauschen (Kurzform, Details in `docs/02` und `docs/messlog.md`)

- **Vier Störfamilien** in den ADC-Daten, auf allen Kanälen stark korreliert:
  - **SOF-Kamm** bei k·≈1 kHz
  - **fs/8** = 6000,00 Hz (Audio-Blockrate bei Blockgröße 8, sample-synchron)
  - **Mischprodukte** n·f1k + m·6000 Hz
  - **LED-PWM** 2320,98 Hz auf acdc2, 2320,54 Hz auf acdc1 (in den Aufnahmen nur mit Signal oder hohem Gain). **Am Ausgang bewiesen [gemessen, D67]:** Ein Patch, der LEDs gedimmt leuchten lässt, bringt 2321 Hz mit Vielfachen auf den Ausgang, der Pegel folgt dem LED-Muster um bis zu 27 dB. Patches sollten LEDs deshalb nicht dauerhaft dimmen. Bei „Rot“ lässt die Firmware Grün mit Tastgrad 1023/1024 weiterlaufen und setzt den roten Pin alle 2 ms neu [Code], am Ausgang ist davon nichts messbar [gemessen, D72]
  - **Jeder Wechsel einer LED verschiebt den Ausgang [gemessen, D73]:** Dauerhaft rote LEDs stören nicht, aber jedes Ein- oder Ausschalten gibt einen Knack (je LED etwa das 6-Fache des Rauschens im Bass, additiv), Blinken mit 40 Hz hebt den Gesamtpegel um 17,6 dB. Patches sollten LEDs deshalb selten und nie flackernd schalten. Mess-Patch: `owl/test/SchliereLedTestPatch.hpp`
  - **Der LED-Strom verschiebt den Nullpunkt aller Eingänge [gemessen, D74]:** je leuchtender Eingangs-LED −2,9e-4 Vollausschlag (≈ −2,9 mV, −71 dBFS), an In 1–3 gleich, vom Patch selbst an den rohen Eingängen gemessen (`owl/test/SchliereLedPathPatch.hpp`, Meldung über `owl_read.py`). Direkt am Ausgang kommen bei digital stummem Ausgang etwa 0,15 mV je LED dazu. Die PWM-Linie ist derselbe Effekt, 2321-mal pro Sekunde geschaltet. Folge [Hypothese]: Die Pegel-LEDs der Firmware bewegen den Nullpunkt der Aufnahme im Takt der Hüllkurve
- **Der Kamm hängt am USB-SOF des Host-Controllers [gemessen, S2-6].** f0 hängt vom Anschluss ab: Mac-Port 999,99 Hz, TB-Hub 999,868 oder 999,997 Hz je Port (im acdc1-Takt). Ein Notch-Filter muss f0 je Modul und Anschluss messen.
- **Am Ausgang ist der Kamm ohne USB weg und schon ohne Stream da [gemessen, D20].** Standalone ist der Ausgang sauber.
- **Masseschleife [gemessen, D21]:** nur mit AC/DC am Mac-Port und UMC am Hub (+20 dB). Beide am Hub → weg. Kein Isolator nötig. Die Alltags-Kette ist für Aufnahmen in Ordnung (D22/D23).
- **Die Lage des Codes im Speicher verschiebt die Störlinien [gemessen, D33].** Ein reiner Füll-Build ohne Funktionsänderung ändert fs/8, Kamm und Mischprodukte um 1,5–4,5 dB. Liegt der heiße Pfad im ITCM, zählt die Flash-Lage nicht mehr (fx3lp ≤ 0,25 dB bei fx3l, D48).
- **Was die CPU im Blocktakt tut, erscheint bei fs/8 [gemessen, D42]:** Ein paar µs Abfrage eines DMA-Registers pro Block heben fs/8 um ≈ 10,6 dB. Mechanismus [Hypothese]: Busverkehr bzw. Strompuls.
- **Auch die Rechenlast eines Patches hebt fs/8 [gemessen, D69]:** Schliere (29 % CPU) gegen den Default-Patch (6 %) in derselben Sitzung: fs/8 +3,8 dB, Mischprodukte +13,4 dB, Rest und Kamm ±0,25 dB. Ein FX-Patch reicht diese Linien samt Kamm an die Ausgänge durch, ohne Eingangssignal sind sie das, was man hört (D68).
- **Einbau ins Case [gemessen, D69]:** in den ADC-Daten keine Verschlechterung gegen „neben dem Case“ (Rest −0,1 dB, fs/8 −0,2 dB, Kamm +0,65 dB bei zwei Stunden Abstand). Die Ausgänge sind dazu nicht gemessen.
- **Den Kamm senkt nur USB-Code im ITCM [gemessen, D42/D44]:** USB im Flash +2,5…+3,5 dB gegen 3d, im ITCM ±0.
- **Ungleich gestellte Regler heben eigene Linien an [gemessen, D58, D60, D61]:** 2200,95 / 4401,90 / 6602,85 Hz und 8549,33 Hz, an den Gerätetakt gekoppelt, unabhängig von der Firmware (fx3l = fx4b).
  - **Es zählt der Unterschied zwischen den Reglern, nicht der Anschlag [gemessen, D61].** Alle vier am selben Anschlag liegen wie 12 Uhr (−114…−119 dB). Ein Regler allein am Anschlag bringt ≈ −100 dB, abwechselnd links und rechts (LRLR) −94 dB. Der Pegel wächst stetig mit dem Abstand.
  - Das Muster bestimmt die Linie: LLRR hebt 2200,95 und 6602,85 Hz, LRLR hebt 4401,90 Hz.
  - Auf 12 Uhr bleiben sie 6…16 dB über dem Grundrauschen, weg sind sie nie.
  - [Hypothese] Der Abtastkondensator des Regler-ADC wird von Kanal zu Kanal umgeladen. Die Quelle der Frequenz 2200,95 Hz ist offen: Die ADC-Scanrate faltet auf 9633,80 Hz, die LED-PWM liegt bei 2320,54 Hz.
  - Ein FX-Patch reicht sie an den Ausgang durch [gemessen, D72]: Schliere mit Regler 3 am Linksanschlag −66…−73 dBFS am UMC, so stark wie die stärksten Kamm-Linien, in der Mitte des Regelwegs im Rauschen.
  - Rauschmessungen deshalb immer mit allen Reglern auf 12 Uhr.
- **Drei Linien stammen aus Takten der MCU [Code, gemessen, D60]:**
  - Scanrate des Regler-ADC 105 633,8 Hz, gefaltet auf 9633,80 und 19 267,6 Hz (acdc2: 9653,46 Hz), −116…−129 dB, fast unabhängig von den Reglern.
  - LED-PWM = 240 MHz / (101 × 1024) = 2320,54 Hz auf acdc1. Die 2320,98 Hz der frühen Messungen gelten für acdc2 (Abtastrate 47 991 Hz).
  - [Hypothese] 2000,0002 und 3000,0003 Hz sind Vielfache des 1-kHz-Systemtakts.
- Gewinn einer idealen Notch-Bank: 4–10 dB mit dem Default-Patch (M1/M2, Serie 2, beide Module), bei fx3l ≈ 5,8 dB (RMS gegen Rest, D48). Ein Filter im Rechner ist optional und hängt davon ab, wie viel Störgeräusch am Ende bleibt (`TODO.md`).

## Firmware (Kurzform, Details in `docs/05-firmware-plan.md`)

Alle eigenen Builds: Basis `db9d3dd` (v22.4.r, Branch `hack/acdc-output-levels`) + Libraries `9e8f64e` (altes H7-HAL), gebaut mit `tools/build_acdc.sh` und Arm GNU 12.2.Rel1 (`/Applications/ArmGNUToolchain/12.2.rel1/`), deterministisch. v22.4.s (`7f3cc41`) gibt auf acdc1 kein Audio (D10).

| Build | Version, SHA | Inhalt | Ergebnis |
|---|---|---|---|
| 1d | `v22.4.r-thru1d`, `f522560e…` | 01 GCC 12, 02 Revert `237c63d`, 04 Ausgang 125, 05 atomarer RX-Zähler, 06 Feedback-Regelung | Durchleitung ohne Rechner, 30 min ohne Block-Slips (D17–D20) |
| 2 | `v22.4.r-fx1d`, `7f9f9a34…` | 1d + 03 `TX_PRE_FX`, „OWL-ACDC-FX“ | A1–A6, B1–B3 bestanden. B3 = Basislinie der rohen Eingänge (D24–D27) |
| 3d | `v22.4.r-fx2d`, `d212cf8d…` | 2 + 07d Eingangs-LEDs roh, Code in `.late_text`/`.late_bss` | Rauschen = Build 2 (D34), Clip-Rot bestanden (D36). Zweiter Rückweg |
| 4, 4x, 4y, 4z | `fx3i`, `fx3i-noD/noC/onlyD`, `fx3j`, `fx3k` | 3d + 08 + ITCM-Varianten (11, 11a–f), RX-Fix 12 bzw. 12b | fs/8 −5 dB, aber je ein Mangel (D37–D44) |
| 5c | `v22.4.r-fx3l`, `b55b94d8…` | 3d + 08 + 11g (heißer Pfad inkl. USB im ITCM) + 12b + 14 | Alltagstest D47–D49, Kontrolle in D58. **Rückweg** |
| 6 | `v22.4.r-fx4a`, `e7ee9b90…` | fx3l + 15 (Boot-Menü, Slot wird gemerkt, Abbruch nach 60 s) | Menü am Gerät geprüft (D50–D53), abgelöst durch 6b |
| 6b | **`v22.4.r-fx4b`, `a21f35c9…`** | fx3l + 15b (wie 15, Abbruch nach 30 s). ITCM-Inhalt bytegleich mit fx4a | Alltags-Firmware bis 2026-11-18 (D55, D59). Loop-Test bestanden (D57), Rauschen = fx3l (D58). **Rückweg** |
| 7 | **`v22.4.r-fx5a`, `b69c25b3…`** | fx4b + 16 (MIDI-Sendeweg unteilbar, Lesezeiger erst nach Abschluss der Übertragung, Flusskontrolle beim Slot-Download). ITCM-Code +48 B | Alltags-Firmware 2026-11-18 bis 19 (D63, D65). Downloads 240/240 statt 60/79, Loop-Test bestanden (D63), Rauschen ohne Hinweis auf eine Änderung (D64) |
| 8 | `v22.4.r-fx6a`, `6a3fc427…` | fx5a + 17 (LED-Modus normal / nur Clip / aus, im Boot-Menü mit Regler 2, gemerkt in `ACDC.led`). ITCM-Ende und alle Funktionen außerhalb von `.text.onLoop` wie fx5a | Abnahme vollständig (D75–D77). Rückweg von fx7a |
| 9 | `v22.4.r-fx7a`, `cac6fee0…` | fx6a + 18 (LED-PWM 48 000 Hz statt 2320,54 Hz: TIM2/3/4/8 Vorteiler 4, Period 999, fest am Audio-Takt). Nur Timer-Init in `main.c`, ITCM bytegleich mit fx6a. Werte unter 2,4 % bleiben dunkel | **Alltags-Firmware auf acdc1** (2026-11-19, Nutzer). Abnahme vollständig (D78, D79): LED-Linie weg, Rauschen = fx6a, Loop-Test bestanden |

- **Vier Fehler im Original-Code, gefunden und behoben:**
  - **Feedback in offener Schleife:** macOS liefert ≈ 1 LSB zu wenig, der RX-Puffer läuft alle 2–3 min um einen Block leer. Patch 06 regelt nach dem Füllstand (D12–D18).
  - **RX-DMA-Wettlauf:** `audioCallback` hing am TX-DMA-Interrupt und gab die RX-Hälfte zu früh frei. Mit schnellem Audio-Task ist der letzte Eingangs-Sample jedes Blocks 16 Frames alt. Patch 12b startet den Block aus den RX-Callbacks (D40, D43).
  - **MIDI-Sendeweg ohne Schutz:** Audio-Task, Hauptschleife und USB-Interrupt teilen sich Sendepuffer und Endpoint, der Lesezeiger rückt auch ohne Senden und schon vor dem Ende der Übertragung vor. Slot-Downloads neben einem laufenden Patch kommen beschädigt an, MIDI kann bis zum Neustart hängen. Patch 16 sperrt kurz die Interrupts und gibt die Daten erst nach der Übertragung frei (D53, D54, D62, D63).
  - **SOF-Kette doppelt:** Mit USB im ITCM läuft `USBD_AUDIO_SOF` in ≈ 97 % der Frames zweimal, das Feedback wird falsch. Patch 14 wertet jede Frame-Nummer nur einmal aus (D46, D47).
- **Regeln für neue Firmware:**
  - Jeder Flash nur nach Freigabe, nur acdc1. Settings nie schreiben (E4).
  - Periodisch laufender Code gehört ins ITCM (Linker-Patch 11g), jeder ITCM-Build vor dem Flash durch `tools/check_itcm.py`.
  - Ohne ITCM gilt die alte Regel: neuen Code ans Ende legen (`.late_text`/`.late_bss`) oder die Adressen per `nm` gegen den Referenz-Build prüfen.
  - Keine Abfrage-Schleifen im Blocktakt (D42).
  - Loop-Prüfung immer mit `phase_slips.py` **und** `stale_samples.py` auf derselben WAV.
- **Rückwege für acdc1:** von fx7a aus fx6a (`6a3fc427…`), von fx6a aus fx5a (`b69c25b3…`), von fx5a aus fx4b (`a21f35c9…`), dann fx3l (`b55b94d8…`), dann 3d (`d212cf8d…`), dann Build 2, dann 1d, dann das Readback (`433b3fd3…`).
- **Fork:** https://github.com/m9dfukc/OpenWare
  - `acdc/thru-usb-feedback` = Build 1d + README, Spitze `edb32852`.
  - `acdc/fx1d` = Build 2 → 3d → fx3l → fx4b → fx5a → fx6a → fx7a + README, Spitze `13b58412` (gepusht 2026-11-19). Ein Build von `34782dea` ist bytegleich mit fx6a (`verify-fx1d-34782dea`), einer von `351b2071` bytegleich mit fx7a (`verify-fx1d-351b2071`). Ein Build von `5cb2c17c` ist bytegleich mit fx3l (`firmware/builds/verify-fx1d-5cb2c17c`), einer von `2c963e3a` bytegleich mit fx4b (`verify-fx1d-2c963e3a`), einer von `c222fa49` bytegleich mit fx5a (`verify-fx1d-c222fa49`).
  - Beide Spitzen sind gepusht: `edb32852` (2026-11-16) und `b4b95e43` (2026-11-18, mit den fx5a-Commits `bd04588e` und `c222fa49`, davor `27d3630b` mit den fx4b-Commits `7ef6284a` und `2c963e3a`).
  - **Releases am Fork** (2026-11-16): `acdc-v22.4.r-fx2d` (Build 3d, Tag auf `abc3a72a`) und `acdc-v22.4.r-fx3l` (Vorabversion, Tag auf `3e3a74fd`), je mit `ACDC-<version>.bin` und `BUILD_INFO`. Dazu seit 2026-11-17 `acdc-v22.4.r-fx4b` (Tag auf `27d3630b`, reguläres Release), seit 2026-11-18 `acdc-v22.4.r-fx5a` (Tag auf `b4b95e43`) und seit 2026-11-19 **`acdc-v22.4.r-fx7a`** (Tag auf `13b58412`, reguläres Release, „Latest“, deckt fx6a mit ab). Ein neuer Alltags-Build bekommt ein eigenes Release, nur nach Freigabe. `gh` immer mit `--repo m9dfukc/OpenWare` aufrufen, sonst zielt es auf das Upstream-Repo.
- **Später geplant:** Merge mit v22.5.0 bzw. dem aktuellen Upstream-`master` samt Fixes. Das ist eine Portierung (`docs/research/notes/upstream_master_2026-11-16.md`).

### Flash-Ablauf (ohne Pads, DFU über die vordere USB-B-Buchse)

1. `owl_read.py --out <flashlog>/owl_read_before`
2. `owl_read.py --reset-to-dfu --yes`
3. `dfu-util -d 0483:df11 -S 200364500000 -a 0 -s 0x08000000 -D <bin>`
4. Readback `-s 0x08000000:131072 -U`, mit dem Binary vergleichen und auf 0xFF prüfen
5. Start der Firmware: **Standard seit 2026-11-19 (D79)** ein zweites, nur lesendes Readback mit `-s 0x08000000:131072:leave -U`. Der ROM-DFU startet danach die Firmware, `dfu-util` endet dabei mit „Error during download get_status“ (normal). Kein Kaltstart, der Codec bleibt unter Strom. **Kaltstart** (Nutzer: Case aus, USB ab, Case an, USB an; nur Case aus reicht nicht, USB versorgt das Board) nur, wenn `:leave` scheitert, wenn ein Build am Einschalten etwas ändert (Takt, Codec-Init), wenn der Test das Einschalten betrifft (Boot-Menü, gemerkter Slot oder LED-Modus nach Aus und Ein) und auf dem Rückweg nach einem Problem
6. A1 per `owl_read.py`: Version, `SR/BS/CH 48000 8 4`, Slots unverändert

Je Flash ein Ordner in `firmware/flashlog/`.

**Als Skript (seit 2026-11-18): `tools/flash_acdc.py`**, außerhalb des Sandbox. Es führt die Schritte 1–6 aus, mit `--cold` nur 1–4 und Schritt 6 nach dem Neustart von Hand:

- `python3 tools/flash_acdc.py <build>`: nur die Offline-Prüfung (Größe, Vektortabelle, Version, SHA gegen `BUILD_INFO.txt`, `check_itcm.json`). `<build>` ist eine `.bin`, ein Build-Ordner oder ein Ordnername in `firmware/builds/`.
- `… <build> --yes [--name <log>]`: flasht acdc1. Geprüft werden die MIDI-Geräte-ID und die DFU-Seriennummer. Der Log-Ordner heißt `firmware/flashlog/<datum>_<name>` (Standard: Build-Ordner ohne `acdc1-`), ein vorhandener wird nie benutzt (dann z. B. `--name …-zurueck`).
- Standard nach dem Readback: `:leave`, warten, bis das DFU-Gerät verschwindet (`--leave-wait`, 10 s), dann A1 mit bis zu drei Versuchen. Exit 6: `:leave` hat nicht gewirkt oder acdc1 meldet sich nicht über MIDI, dann Neustart von Hand und `--a1`. Exit 7: A1 bestanden, aber das zweite Readback weicht ab.
- `--cold`: kein `:leave`, nach dem Readback auf den Neustart von Hand warten.
- `… --a1 <log-ordner>`: nach einem Neustart von Hand. Prüft Gerät, Version, 48000 Hz, Blockgröße 8 und die Slots gegen den Stand vor dem Flash. Die Kanalzahl prüft es nicht.
- `--from-dfu`: acdc1 steht schon im DFU-Modus (z. B. nach einem fehlgeschlagenen Flash). Dann nicht neu starten, sondern damit den Rückweg flashen.
- Bricht es ab, bevor etwas geschrieben wurde, benennt es den Log-Ordner in `…-abgebrochen-<zeit>` um.
- **Stand:** Am Gerät erprobt (2026-11-18, D63/D64): drei Flashes mit Readback und `--a1`, ohne Fehler. `--from-dfu` und der Abbruchweg liefen bisher nur gegen Attrappen. `:leave` von Hand am Gerät erprobt (2026-11-19, D79: zweimal, A1 bestanden, Rauschen gleich wie nach Kaltstart). Im Skript seit 2026-11-19, bisher nur gegen Attrappen (Erfolg, bleibt im DFU, kein MIDI, abweichendes Readback).

### USB-Stabilität prüfen

- Patchkabel Out n → In n, Gain 12 Uhr, `tools/acdc_rec.py --sine 440 530 670 790 --owl-read`, 60 s.
- **Vor jedem Kabel von einem Ausgang auf einen Eingang prüfen, welcher Patch läuft.** Ein FX-Patch, der diesen Eingang auf diesen Ausgang gibt (Schliere, Mess-Patches mit Verstärkung), bildet damit eine Rückkopplungsschleife. Erst auf Slot 1 schalten oder den Stream öffnen, dann stecken lassen (D75).
- Über den Loop lässt sich ein Eingang nur mit negativer Spannung übersteuern (`--dc … -1`), wegen des negativen Eingangsoffsets (D36, D75).
- Danach `tools/phase_slips.py --freqs 440 530 670 790` und `tools/stale_samples.py`.
- **Den Patch vor dem Öffnen des Streams wählen**, nie währenddessen: Ein Program Change bei offenem Stream bricht die Wiedergabe (D76).
- Die Geräte-Meldung lesen, **bevor** der Stream schließt (`--owl-read` tut das).
- Debug-Meldungen (PROGRAM_MESSAGE, nur die letzte bleibt):
  - `rx unf 0.9513 0.4722 4.0000` = Unterlauf mitten im Lauf
  - `rx unf 1.0000 … 32` = Puffer leer, typisch Stream-Ende
  - `tx ovf 1.0000 1.0000 32` = typisch Stream-Start
  - `fb <alt> <neu>` = Feedback-Messung verworfen, z. B. ≈ 32 ms ohne SOF (D18)
  - `fb clamp` = Regler an der Grenze, bei laufendem Stream ein Fehlerzeichen (D38). Tritt nach einem Program Change bei offenem Stream auf (D76)
  - `midi tx ovf` (ab fx5a) = MIDI-Sendepuffer voll, eine Nachricht wurde verworfen
  - `Sending 100` = Fortschrittsanzeige nach einem Slot-Download, kein Fehler

### MIDI/SysEx

`tools/owl_read.py` und `tools/owl_write.py` laufen nur außerhalb des Sandbox (CoreMIDI) und mit Python 3.12: `uv run --no-project --python 3.12 --with mido --with python-rtmidi python tools/owl_read.py`. OWL-Webseiten vorher schließen, sie pollen das Gerät.

- **Patch aufspielen:** erst `owl_write.py <datei> --run` (nur RAM), dann `--store N --yes`. Ist der Slot belegt, die Sicherung mit `--backup-file <lokale Kopie>` angeben (Größe und CRC müssen zum Slot passen), sonst lädt das Werkzeug den Slot herunter, und das ist bei laufendem Patch unzuverlässig (D54). Slot 1 braucht zusätzlich `--allow-slot1`. Nach einem Store läuft der gespeicherte Slot, `--program-after 1` schaltet zurück.
- **Regler lesen:** Der Settings-Request von `owl_read.py` liefert CC 20–23 (Regler 1–4, 7 Bit) im Report unter `settings_cc`.
- **Git und Tarballs im Sandbox:** `git clone` scheitert an `.git/config`. Fremde Quellen deshalb als Tarball von `codeload.github.com` holen (wie die Libraries).

## Git-Repo `acdc-workbench`

Das Projekt ist seit 2026-11-16 ein Git-Repo und liegt öffentlich auf https://github.com/m9dfukc/acdc-workbench (Remote `origin`, Branch `main`).

- **Im Repo:** `docs/` (ohne `docs/uebergaben/`), `tools/`, `analysis/`, `firmware/patches/`, `firmware/itcm-prep/`, `TODO.md`, `CLAUDE.md`, READMEs, `.claude/skills/`, `recordings/manifest.toml`. Aus `out/` nur `*_report.md` und `*-session-table.md`. Aus `firmware/builds/` und `firmware/flashlog/` nur Textdateien.
- **Nur lokal (`.gitignore`):** WAVs, Plots und JSON in `out/`, `.bin`/`.elf`/Readbacks, `backup/`, `docs/uebergaben/`, `owl-patches/`, `vcv-patches/`, `firmware/_work/`, `firmware/downloads/`, `firmware/releases/`, der Screenshot `docs/img/20261114_d8_vcv-patch.webp`, das Forum-Foto in `docs/research/img/`.
- **Plots für Docs und README** liegen als Kopie in `docs/img/` (z. B. `psd_fx3l_vs_3d.png`). Auf `out/**/*.png` nicht verlinken, die Dateien fehlen im Repo.
- **Commit und Push nur mit Freigabe.** Git-Schreibzugriffe (auch `git add`) sperrt die Sandbox (`.git/`), sie brauchen die Ausnahme. Push immer einzeln nachfragen, auch im Submodul.
- **Vor jedem Commit prüfen:** `git status --short` darf keine Binaries, WAVs oder Dateien aus `backup/` zeigen. Nichts Privates über Geräte-UUIDs, Heimpfad und Vorname hinaus (Nutzer-Entscheidung 2026-11-16).
- **Sprache:** Deutsch ist die Quelle (Docs, Messlog, `TODO.md`, `CLAUDE.md`, Kommentare im Code, Reports der Analyse). Englisch sind `README.md`, `LICENSE.md` sowie Hilfe- und Konsolentexte der Werkzeuge. Ein Sprachschalter für die Reports ist ein offener Punkt in `TODO.md`.
- **Konsolentexte der Werkzeuge sind seit 2026-11-16 englisch.** Ältere Einträge im Messlog zitieren noch die deutschen Zeilen (z. B. „Kanal 1 … Ereignisse: 0“ = heute „Channel 1 … Events: 0“). Was in Dateien landet (Reports, Tabellen von `family_table.py`, `check_itcm.md`, Manifest, JSON), bleibt deutsch.
- **Englische Fassungen (`docs/en/`):** Claude erzeugt sie vollständig aus der deutschen Quelle und stempelt sie mit `tools/i18n_check.py --stamp <datei>`. Aufgefrischt wird zu Meilensteinen (z. B. wenn ein Build zur Alltags-Firmware wird) und vor einem Push, wenn `tools/i18n_check.py` „stale“ meldet und der Nutzer zustimmt. Marker: [belegt] = [sourced], [Code] = [code], [Hypothese] = [hypothesis], [gemessen] = [measured]. Glossar in `docs/en/README.md`. Neue Paare in `PAIRS` im Skript eintragen.

## Skills (projektlokal, `.claude/skills/`)

Aus `mattpocock/skills` @ `5863806`, nur das Nötige:
- `/grill-me` (ruft `grilling` auf): Plan oder Entscheidung in Fragerunden durchlöchern, z. B. vor Firmware-Schritten oder Messreihen.
- `/handoff`: Gespräch für eine neue Session verdichten.
- `research`: Frage gegen Primärquellen recherchieren, Ergebnis als Markdown. Ablage in diesem Projekt: `docs/research/notes/`.

## Umgebung

- Im Sandbox ist `~/.cache/uv` nicht schreibbar. uv deshalb mit `UV_CACHE_DIR=.uv-cache uv run …` aufrufen. Matplotlib-Cache setzt `analysis/plots.py` selbst ins Temp-Verzeichnis.
- OpenWare-Quellcode: `firmware/OpenWare` ist ein **Submodul** dieses Repos (`.gitmodules`: `https://github.com/m9dfukc/OpenWare.git`, Branch `acdc/fx1d`). Im lokalen Klon heißen die Remotes `fork` (`m9dfukc/OpenWare`, SSH) und `upstream` (RebelTechnology, alle Branches als `upstream/*`, alle Tags), ein `origin` gibt es dort nicht. Ausgecheckt ist `acdc/fx1d`. Der ausgecheckte Commit ist der Stand, den das Hauptrepo festhält: Wer im Submodul umschaltet oder committet, muss danach im Hauptrepo `firmware/OpenWare` neu committen. Andere Stände weiter per `git show <commit>:<pfad>` bzw. `git archive` lesen, `tools/build_acdc.sh` hängt nicht am ausgecheckten Stand. Übersicht: `firmware/README.md`.
- Patch-Quellen aus der OWL-Library: `owl-patches/library/` (Default-Patch „ACDC“, „_Lich_Nichts“). Library-API: `https://www.rebeltech.org/api/patch/?seoName=<name>` (Metadaten inkl. Quell-URL), `/api/patches/` (Liste). Unveröffentlichte Patches sind nur so auffindbar.
- Fremde Werkzeuge und Vorlagen: **shmuga/lich-patcher** (Backup und Settings im Browser, nur lesend benutzen), **burstMembrane/acdc-patches** (FX-Vorlage).

## Arbeitsweise für Claude in diesem Projekt

- Firmware-Code aus fremden Repos und Forks erst **lesen und diffen**, nicht blind bauen oder flashen. Vor jedem Flash nachfragen.
- Analyse-Skripte sollen **reproduzierbar** sein (CLI mit Argumenten, keine hartcodierten Pfade) und Ergebnisse als PNG und JSON/Markdown nach `out/` schreiben.
- Frequenzen immer **gemessen** ermitteln (Peak-Interpolation) statt 1000 Hz anzunehmen.
