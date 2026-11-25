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
  KickVoice / BassVoice je Modus eine Klasse (Phase 2: Hat, Stab, Pad, Riser)
plugin/                 DPF-Hülle: Parameter, Programme (Presets), MIDI, TimePosition
tools/offbeat-render.cpp  Offline-Renderer: WAV + Messwerte + Abnahmeprüfung
Makefile                `make` baut Plugin + Renderer, `make install` installiert + signiert
```

Die Parameter-Tabelle ist die einzige Quelle für Plugin und Renderer. Neue Parameter werden nur
**hinten** angehängt, damit gespeicherte Projekte in LMMS/Carla gültig bleiben.

## Phasen

### Phase 1 (dieser PR)
1. DPF-Gerüst, Build für VST3 + LV2 (x86_64), Installation + ad-hoc-Signatur.
2. Globale Parameter: Mode, Volume, Drive, Tone, Width (Width nur auf Seitensignal > 150 Hz).
   Ausgang sauber begrenzt (≤ −0,1 dBFS), Denormals aus (FTZ/DAZ).
3. Kick-Modus nach Abschnitt 3 inkl. Bounce Sync.
4. Bass-Modus nach Abschnitt 3 inkl. Rumble/Roll (16tel-synchron zur Host-BPM).
5. Presets für Kick und Bass (1–4c) als Plugin-Programme.
6. `offbeat-render` mit Messwerten; Abnahme „Kick – Hard Bounce“ automatisch prüfbar (`--check`).

### Phase 2
Hat, Stab/Lead, Pad, Riser + restliche Presets (5–9), jeweils mit Render-Messung.

### Phase 3 (optional)
Eigene Oberfläche.

## Offene Fragen

- **GitHub-Anmeldung:** Das `gh`-Token ist ungültig, und es gibt keine HTTPS-Zugangsdaten für
  git. Remote ist gesetzt (`lukestark-glitch/offbeat-synth`), Commits liegen lokal. Bitte
  `gh auth login -h github.com` ausführen, danach pushen und den PR öffnen.
- **Bass-Decay-Kurve:** „Amp Decay“ ist als Zeit bis −40 dB umgesetzt (exponentiell). Falls
  sich der Bass zu „plucky“ anhört, wäre eine Hold-Phase eine Alternative.
- **Bounce-Pitch-Kurve:** fällt exponentiell und erreicht den Zielton nach „Bounce Time“ zu ~95 %.
- **Roll-Modus:** Sub läuft durch, solange die Taste gehalten wird; Saw/Square und Filter werden
  jede 16tel neu angeschlagen. Ohne laufenden Host-Transport läuft das Raster frei ab Note-On.
- **Width bei Kick/Bass:** Die Kick ist mono, Width hat dort keine Wirkung. Beim Bass liegen die
  beiden verstimmten Oszillatoren links/rechts, Width regelt deren Breite oberhalb 150 Hz.
- **Velocity:** skaliert die Lautstärke linear. Falls LMMS-Velocity stört, abschaltbar machen?
- **CLAP:** vorerst nicht gebaut (Carla 2.5 kann es nicht), lässt sich später im Makefile ergänzen.
