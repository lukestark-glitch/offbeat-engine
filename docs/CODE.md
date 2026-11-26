# Offbeat Engine – Code-Doku

Diese Seite erklärt den Aufbau des Codes, damit du die Engine nach deinen Vorstellungen anpassen kannst: einen neuen Regler, ein neues Preset, einen anderen Klang oder sogar einen neuen Modus.

## Überblick

```
offbeat-engine/
├── src/dsp/            DSP-Kern ohne DPF (läuft im Plugin und im Offline-Renderer)
│   ├── Params.hpp/.cpp   alle Parameter (kParams), Presets (kPresets), modeParams(), jsonParams()
│   ├── Engine.hpp/.cpp   Noten → Stimme des aktiven Modus → Drive → Tone → Width → Volume → Limiter
│   ├── Kick.hpp          KickVoice: Sinus mit zweistufigem Pitch-Fall, Sättigung, Klick, Cut
│   ├── Bass.hpp          BassVoice: 2 Oszillatoren + Sub, Filter-/Amp-Hüllkurve, Bounce, Glide, Roll
│   ├── Hat.hpp           HatVoice: 6 Rechtecke (metallisch) + Rauschen, HP/LP, Closed/Open
│   ├── Poly.hpp          PolyVoice: Supersaw-Stimme für Stab/Lead und Pad (bis 8 Stimmen)
│   ├── Riser.hpp         RiserVoice: Rauschen/Saw mit Filter-Sweep über n Takte, Sirene
│   └── Blocks.hpp, ParamInfo.hpp, ParamText.*   gemeinsame Bausteine (siehe unten)
├── plugin/             DPF-Hülle und Oberfläche
│   ├── DistrhoPluginInfo.h  Name, URI, IDs, Formate-Einstellungen
│   ├── OffbeatPlugin.cpp    Parameter, Programme (Presets), MIDI, Host-Tempo → Engine
│   ├── OffbeatUI.cpp        Oberfläche: Reiter, GLOBAL, Auto-Bild, Regler des Modus
│   └── OffbeatKit.hpp, OffbeatParam.hpp   gemeinsamer Look und DPF-Anbindung
├── tools/offbeat-render.cpp  Offline-Renderer: rendert Noten, misst, prüft (--check), JSON
├── assets/car.png      Bild in der Oberfläche
└── docs/images/        Bilder der Oberfläche für das README
```

## Signalfluss

```
MIDI ─► OffbeatPlugin::run() ─► Engine::noteOn/noteOff (Octave verschiebt die Note)
Host-Tempo ─► Engine::setTransport() (oder BPM-Regler, wenn Host-Tempo aus ist)

Engine::process() pro Sample:
  16tel-Raster (Roll) ─► Stimme des Modus:
      Kick:  2 KickVoice (abwechselnd, die alte blendet in 3 ms aus)
      Bass:  1 BassVoice (mono, Glide, Roll = Neuanschlag auf jedem 16tel)
      Hat:   2 HatVoice (Closed schneidet Open ab)
      Stab/Pad: 8 PolyVoice (Pad: Filter-LFO im Takt, danach Chorus)
      Riser: 1 RiserVoice (Fortschritt = Schläge seit Notenbeginn / (Bars·4))
  ─► Drive (tanh) ─► Tone (Low-Pass) ─► Width (Seite ab 150 Hz, 48 dB/Okt.) ─► Volume ─► softLimit
```

Der Modus ist der Parameter `mode`. Bei einem Moduswechsel werden alle Stimmen zurückgesetzt (`resetVoices()`).

## Häufige Anpassungen

### Klang eines Presets ändern
In `src/dsp/Params.cpp` stehen die Presets als Liste von Abweichungen vom Standard:

```cpp
const V kickHard[] = { { kMode, kModeKick }, { kKickBody, 70 }, { kKickDrive, 0.85f } };
```

Werte ändern, `make check` laufen lassen, `make install`. Neue Presets kommen **ans Ende** von `kPresets[]`, damit die Programm-Nummern in gespeicherten Projekten gleich bleiben.

Schneller ausprobieren geht ohne neu zu bauen: `bin/offbeat-render --preset 0 --note F1 --set kick_drive=0.6 --out test.wav`. Du kannst auch ein JSON aus der Oberfläche kopieren, in eine Datei legen und mit `--json datei.json` rendern.

### Neuen Regler hinzufügen
1. `Params.hpp`: neuen Eintrag **hinten** in `ParamId` (vor `kParamCount`).
2. `Params.cpp`: Zeile **hinten** in `kParams` (Name, Kurzname, Einheit, min, max, Standard, Flags).
3. DSP: den Wert über `params[kDeinRegler]` bzw. `p[kDeinRegler]` in der Stimme benutzen.
4. Oberfläche: Gehört der Regler zu einem Modus, ihn in `modeParams()` an der gewünschten Stelle einfügen (Beispiel: `kKickAttack` hinter `kKickCut`). Globale Regler setzt du in `OffbeatUI::drawContent()` mit `knob()` oder `compact()`.
5. Soll er in KOPIEREN auftauchen, ihn in `jsonParams()` aufnehmen.
6. `make check`, `make install`.

### Neuen Modus hinzufügen
1. Stimme als eigene Klasse in `src/dsp/` schreiben. Vorbild ist `Hat.hpp` oder `Riser.hpp`: `trigger/start`, `release`, `process`, `active`.
2. `Mode`-Enum und `kModeNames` (Params) um den Modus **hinten** erweitern; `mode` max in `kParams` erhöhen.
3. Regler des Modus hinten anhängen und in `modeParams()` eintragen.
4. `Engine`: Stimme als Member, `noteOn`/`noteOff`/`resetVoices`/`process` um den Modus erweitern.
5. Oberfläche: Reiterfarbe in `kModeColor` (OffbeatUI.cpp); die Reiter passen sich der Anzahl an.
6. Renderer: Prüfungen für den Modus in `tools/offbeat-render.cpp` (Abschnitt „Prüfung“) und einen Aufruf in `make check`.

### Kick-Kurve verstehen
`Kick.hpp`: `f(t) = f_end + A1·e^(−t/Punch) + A2·e^(−t/Body)` mit A1:A2 = 200:60. Das ist eine Messung an Hard-Bounce-Referenzen. Die Länge endet mit einem Cosinus-Abbruch über `Cut`. „Bounce Sync“ setzt die Länge auf 95 % einer Achtel beim Host-Tempo, sodass die Kick genau vor dem Offbeat-Bass endet.

## Offline-Renderer

```sh
bin/offbeat-render --list                                  # Presets und alle Parameter
bin/offbeat-render --preset 0 --note F1 --check            # Kick rendern und prüfen
bin/offbeat-render --mode bass --note F2 --set bass_bounce=12 --out bass.wav
bin/offbeat-render --json sound.json --note F2             # JSON aus KOPIEREN
bin/offbeat-render --preset-json 2                         # Preset als JSON (wie KOPIEREN)
```

`--note` ist die Taste wie in der LMMS-Piano-Roll. Der Renderer schickt sie wie LMMS eine Oktave tiefer, und der Standard Octave +1 gleicht das wieder aus.

Ausgegeben werden:
- Spitze und RMS,
- die Länge bis −40 dB,
- Tonhöhe und Pegel über die Zeit (Kick/Bass),
- Oktavbänder, Schwerpunkt, Anteil über 10 kHz,
- L/R-Korrelation gesamt und unter 150 Hz,
- Anfang und Ende,
- Clipping, Denormals, NaN.

`make check` rendert jeden Modus mit seinem Preset und prüft ihn.

## Gemeinsame Bausteine der Offbeat-Familie

Diese Dateien sind in allen vier Repos **gleich**. Sie sind eine bewusste Kopie; die Quelle ist [offbeat-engine](https://github.com/lukestark-glitch/offbeat-engine). Änderst du sie, dann zuerst dort und danach in die anderen Repos kopieren.

| Datei | Inhalt |
|---|---|
| `src/dsp/ParamInfo.hpp` | `ParamInfo`: Name, Kurzname (`symbol`), Einheit, Bereich, Standard, Flags (`kFlagLog`, `kFlagBool`, `kFlagInt`, `kFlagOutput`), Auswahl-Namen (`labels`) |
| `src/dsp/Blocks.hpp` | DSP-Bausteine: `Osc` (PolyBLEP-Saw/Square), `Sine`, `Tri`, `Noise`, `Svf` (TPT-State-Variable-Filter), `Smoother`, `Adsr`, `Chorus`, `DenormalGuard` (Flush-to-Zero auf Intel und ARM), Helfer wie `midiToHz`, `decayCoef`, `softLimit` |
| `src/dsp/ParamText.*` | Text ↔ Wert: `parseParamText` (Zahleneingabe am Regler), `applyJson`/`toJson` (KOPIEREN/EINFÜGEN, `--json`, `--preset-json`) |
| `plugin/OffbeatParam.hpp` | `fillParameter`: macht aus einem `ParamInfo` einen DPF-`Parameter` (Hints, Bereich, Auswahl-Werte) |
| `plugin/OffbeatKit.hpp` | `OffbeatKitUI`: Look und Bedienung aller Oberflächen (siehe unten) |
| `tools/Measure.hpp` | Messwerkzeuge der Renderer: WAV lesen/schreiben, Tonhöhe, Pegel, Spektrum, L/R-Korrelation |

### Ein Parameter, viele Stellen: alles kommt aus `kParams`

```
src/dsp/Params.cpp  kParams[]  ──┬─► DSP (Engine/Turbo/Pump liest params[id])
                                 ├─► plugin/*Plugin.cpp: initParameter() → fillParameter() → Host (VST3/AU/CLAP/LV2/…)
                                 ├─► OffbeatKitUI: Name, Bereich, Einheit, Anzeige, Zahleneingabe
                                 ├─► ParamText: JSON-Schlüssel = symbol
                                 └─► Renderer: --set symbol=wert, --list
```

Einen Regler hinzufügen heißt deshalb nur: Eintrag in `ParamId` (Params.hpp) **hinten anhängen**, Zeile in `kParams` (Params.cpp), im DSP benutzen und in der Oberfläche platzieren. Host-Anbindung, JSON und Zahleneingabe funktionieren dann von allein.

> **Nie** bestehende Einträge umsortieren oder löschen. Die Position ist die Parameter-ID in allen Formaten; gespeicherte Projekte würden sonst falsche Werte laden. Ebenso nie die Plugin-URI, VST3/AU-IDs (`DistrhoPluginInfo.h`) oder den Bundle-Namen (`plugin/Makefile: NAME`) ändern.

### OffbeatKitUI (`plugin/OffbeatKit.hpp`)

Die Oberflächen sind ein „Immediate-Mode“-GUI. `onNanoDisplay()` zeichnet bei jedem Bild alles neu. Jeder Baustein, der dabei gezeichnet wird, meldet seine Klickfläche an. Maus und Tastatur arbeiten dann mit den Flächen des zuletzt gezeichneten Bildes. Ein Plugin leitet von `OffbeatKitUI` ab und überschreibt:

| Methode | Zweck |
|---|---|
| `drawContent()` | alles zeichnen (Pflicht) |
| `onButton(id)` | eigene Knöpfe, z. B. Modus-Reiter |
| `controlLabel(p)` | kürzerer Name für einen Regler |

Bausteine:

| Baustein | Was er zeichnet |
|---|---|
| `drawBackground()` | Verlauf und Lochblech (automatisch) |
| `drawLogo(cx, cy)` | Chrom-Plakette mit Swoosh |
| `drawPanel(x, y, w, h, label, farbe)` | Abschnitt im Ticket-Stil |
| `knob(p, x, y, farbe)` | Regler in einer Zelle von 114 × 112 Pixeln; Schalter werden automatisch zur Pille |
| `bigKnob(p, cx, cy, r, farbe)` | großer Regler im Manometer-Look |
| `compact(p, x, y, w, farbe)` | kleiner Regler für Kopfzeilen |
| `pill(id, x, y, w, h, text, farbe, aktiv)` | Knopf |
| `copyPaste(xRechts, y)` | KOPIEREN/EINFÜGEN mit Statuszeile |
| `drawImage(bild, …)` | eingebettetes Bild |
| `drawTag(…)` | kleines Etikett |

Farben und Schriftzug kommen aus einem `kit::Theme`. Die Werte liegen in `values[]` und werden vom Host über `parameterChanged()` aktuell gehalten; `setValue()` schickt eine Änderung an den Host.

**Bedienung:**
- ziehen (mit Shift fein), Mausrad,
- Doppelklick setzt den Standard,
- Klick auf den Wert oder Rechtsklick öffnet die Zahleneingabe,
- Schalter per Klick,
- KOPIEREN/EINFÜGEN tauscht JSON über die Zwischenablage aus; welche Regler darin stehen, legt `jsonParams()` in Params.cpp fest.

**Bilder der Oberfläche** (für die Doku): `make standalone`, dann

```sh
OFFBEAT_SCREENSHOT=ui.ppm OFFBEAT_SCREENSHOT_PRESET=2 bin/<name>.app/Contents/MacOS/<name>
```

Die Oberfläche lädt das Preset, speichert sich als PPM und beendet sich. Die Bilder in `docs/images/` sind so entstanden (PPM → PNG umgewandelt).

### Formate und Build

`plugin/Makefile` nutzt DPF (`dpf/Makefile.plugins.mk`) und baut VST3, VST2, CLAP, LV2 und auf dem Mac AU, bei Turbo und Hydraulics dazu LADSPA. Das Makefile im Hauptordner ruft es auf und baut auf dem Mac ein Universal-Binary (`-arch x86_64 -arch arm64`, `NOOPT=true`). Außerdem erzeugt es die LV2-Beschreibungen (`generate-ttl.sh`) und baut den Offline-Renderer.

Das Bild in `assets/` wird beim Build mit `xxd -i` zu einem C-Array und fest in die Oberfläche eingebaut.

`.github/workflows/build.yml` baut bei jedem Push auf macOS, Windows (MSYS2/MinGW) und Linux und führt `make check` aus. Bei einem Tag `v*` legt es ein Release mit den ZIPs an.

### Echtzeit-Regeln im DSP

- In `process()` kein `new`/`malloc`, keine Locks, keine Datei-Zugriffe. Puffer werden in `setSampleRate()` angelegt.
- Am Anfang jedes `process()` steht `const DenormalGuard denormals;` (Flush-to-Zero).
- Parameter werden in `setParam()` auf ihren Bereich begrenzt; Sprünge glättet `Smoother`.
- Der Ausgang läuft am Ende durch `softLimit()` (Instrumente) bzw. das Ceiling (Turbo); es gibt kein hartes Übersteuern.

### Prüfen

`make check` startet den Offline-Renderer mit `--check-all`. Er rechnet Presets bzw. Testsignale ohne DAW, misst sie (Tonhöhe, Länge, Spektrum, L/R-Korrelation, True-Peak, …) und vergleicht sie mit festen Zielen. Jede Änderung am Klang sollte danach noch grün sein, oder die Ziele werden bewusst angepasst.
