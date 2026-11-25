# Plan – Offbeat Bounce-Synth

Grundlage: `OFFBEAT_SYNTH_SPEC.md`. Ein Plugin (VST3 + LV2, DPF), Modus-Parameter wählt den Klang.

## Aufbau

```
dpf/                    DPF (Submodule, einzige Abhängigkeit)
src/dsp/                reiner DSP-Code, C++17, ohne DPF
  Blocks.hpp            Bausteine: Oszillatoren (PolyBLEP-Saw/Square, Sinus), Rauschen,
                        TPT-SVF, Glättung, Hilfsfunktionen – von allen Modi genutzt
  Params.hpp/.cpp       Parameter-Tabelle (Name, Symbol, Bereich, Default) + Presets
  Engine.hpp/.cpp       Modus-Auswahl, globale Kette (Drive → Tone → Width → Limiter)
  Kick/Bass/Hat/Riser.hpp  je Modus eine Stimme
  Poly.hpp              polyphone Supersaw-Stimme, gemeinsam für Stab/Lead und Pad
plugin/                 DPF-Hülle: Parameter, Programme (Presets), MIDI, TimePosition
tools/offbeat-render.cpp  Offline-Renderer: WAV + Messwerte + Abnahmeprüfung
Makefile                `make` baut Plugin + Renderer, `make install` installiert + signiert
```

Die Parameter-Tabelle ist die einzige Quelle für Plugin und Renderer. Neue Parameter werden nur
**hinten** angehängt, damit gespeicherte Projekte in LMMS/Carla gültig bleiben.

## Phasen

### Phase 1 – fertig (PR #1)
1. DPF-Gerüst, Build für VST3 + LV2 (x86_64), Installation + ad-hoc-Signatur.
2. Globale Parameter: Mode, Volume, Drive, Tone, Width (Width nur auf Seitensignal > 150 Hz).
   Ausgang sauber begrenzt (≤ −0,1 dBFS), Denormals aus (FTZ/DAZ).
3. Kick-Modus nach Abschnitt 3 inkl. Bounce Sync.
4. Bass-Modus nach Abschnitt 3 inkl. Rumble/Roll (16tel-synchron zur Host-BPM).
5. Presets für Kick und Bass (1–4c) als Plugin-Programme.
6. `offbeat-render` mit Messwerten; Abnahme „Kick – Hard Bounce“ automatisch prüfbar (`--check`).

### Phase 2 – gebaut (PR #2)
1. Hat: 6 inharmonische Rechtecke + Rauschen, HP 12 dB, LP 24 dB, Closed/Open per Note, Choke.
2. Stab/Lead: 8 Stimmen, Supersaw 1–7, Filter-Env, ADSR, Hoover-Drop, Chord-Hold, Glide.
3. Pad: dieselbe Stimme mit Dreieck +12, langsamer Attack, langer Release, BPM-LFO auf Cutoff,
   Stereo-Chorus.
4. Riser: Bars 1–32 synchron zur Host-BPM, Noise↔Saw, Filter-Sweep, Pitch-/Volume-Rise, Sirene.
5. Presets 5–9 (insgesamt 13). Width-Hochpass auf 48 dB/Okt., damit unter 150 Hz alles mono bleibt.
6. `offbeat-render`: mehrere Noten (`--note F3@100`), Spektrum, Länge bis −40 dB, Korrelation,
   `--check` für Kick, Hat, Riser und Mono-Bass bei Bass/Stab/Pad.

### Phase 3 – gebaut (PR #3)
Eigene Oberfläche mit DPF + NanoVG (`plugin/OffbeatUI.cpp`), 860×512, Look nach den Vibe-Bildern des Nutzers (Y2K-Rave/Tuning: Nachtblau, Chrom, Neon-Swoosh, Ticket-Etiketten, eine Farbe pro Modus), skaliert mit der Bildschirmauflösung.
- Oben die Modus-Reiter, darunter die globalen Regler (Volume, Drive, Tone, Width) und die Regler des aktiven Modus.
- Bedienung: ziehen hoch/runter, mit Shift fein, Mausrad, Doppelklick setzt den Standardwert, An/Aus-Schalter per Klick.
- Die Parameter-Liste (`Params.cpp`) liefert Namen, Bereiche und Einheiten, deshalb gibt es keine zweite Liste.
- LV2 ist jetzt eine einzelne Datei `offbeat.dylib` (DSP + UI). DPF warnt dabei wegen „direct access“; das ist harmlos, weil die UI nur über Parameter mit dem DSP spricht.

## Offene Fragen
- **Oberfläche:** Farben pro Modus und das eigene Offbeat-Logo habe ich selbst gewählt (inspiriert, nicht kopiert), alles leicht änderbar. Die Oberfläche hat keine Preset-Auswahl; Presets wählt man weiter über den Host (Carla „Programs“).

- **Bass-Decay-Kurve:** „Amp Decay“ ist als Zeit bis −40 dB umgesetzt (exponentiell). Falls
  sich der Bass zu „plucky“ anhört, wäre eine Hold-Phase eine Alternative.
- **Bounce-Pitch-Kurve:** fällt exponentiell und erreicht den Zielton nach „Bounce Time“ zu ~95 %.
- **Roll-Modus:** Sub läuft durch, solange die Taste gehalten wird; Saw/Square und Filter werden
  jede 16tel neu angeschlagen. Ohne laufenden Host-Transport läuft das Raster frei ab Note-On.
- **Width bei Kick/Bass:** Die Kick ist mono, Width hat dort keine Wirkung. Beim Bass liegen die
  beiden verstimmten Oszillatoren links/rechts, Width regelt deren Breite oberhalb 150 Hz.
- **Velocity:** skaliert die Lautstärke linear. Falls LMMS-Velocity stört, abschaltbar machen?
- **VST3 vs. LV2 in Carla:** VST3 meldet zusätzlich ~2080 MIDI-CC-Parameter (DPF-Standard für
  VST3), LV2 nur die 28 eigenen. Empfehlung: in Carla die LV2-Version nehmen.
- **Hard-Bounce-Preset:** Body Time 70 ms statt Default 75 ms. Damit liegen alle drei
  Tonhöhen-Messpunkte mittig im Toleranzband (bei 75 ms: 51,8 Hz bei 150 ms, Grenze 52).
- **Hat Closed/Open:** entscheidet die Note: Tonklasse A# (A#2, A#3 …) = Open, alles andere
  = Closed. Die Presets „Closed Soft“ und „Open Soft“ unterscheiden sich daher nur leicht im Klang
  (Open etwas dunkler, mehr Rauschen).
- **Hat-High-Pass-Default 6 kHz** statt eines mittleren Werts. Nur so liegt der Schwerpunkt
  bei ~5,9 kHz (Spec: 6–7 kHz) mit LP 7,5 kHz.
- **Stab – Rave** spielt mit Chord-Hold „Moll“ (ein Finger = Moll-Dreiklang). Für eigene
  Akkorde Chord auf „Aus“ stellen.
- **Glide im Stab-Modus** gleitet von der zuletzt gespielten Note aus, auch polyphon.
- **Riser** geht von 4/4-Takt aus (Bars × 4 Schläge). Ohne laufenden Transport läuft er
  frei mit der zuletzt bekannten BPM.
- **CLAP:** vorerst nicht gebaut (Carla 2.5 kann es nicht), lässt sich später im Makefile ergänzen.
