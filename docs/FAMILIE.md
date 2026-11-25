# Offbeat-Familie

Vier Plugins für Hard Bounce / Bounce Techno, gebaut mit DPF. Sie teilen Look und Bedienung:
- Regler ziehen, Shift fein, Doppelklick setzt den Standardwert.
- Klick auf den Wert öffnet die Zahleneingabe.
- KOPIEREN/EINFÜGEN überträgt die Einstellungen als JSON.
- Jedes Plugin hat einen Offline-Renderer mit `make check`.

| Plugin | Repo | Rolle | Typ | Formate | Farbe |
|---|---|---|---|---|---|
| **Offbeat Engine** | [offbeat-engine](https://github.com/lukestark-glitch/offbeat-engine) | Kick, Bass, Hat, Stab, Pad, Riser | Instrument | LV2, VST3 | Blau, Auto |
| **Offbeat Topline** | [offbeat-topline](https://github.com/lukestark-glitch/offbeat-topline) | Melodie: Supersaw, Stab, Pluck, Whistle | Instrument | LV2, VST3 | Rot/Schwarz, Motorrad |
| **Offbeat Turbo** | [offbeat-turbo](https://github.com/lukestark-glitch/offbeat-turbo) | Master-Lautheit (Glue, Clipper, True-Peak-Limiter) | Effekt | LV2, VST3, **LADSPA** | Giftgrün, Turbolader |
| **Offbeat Hydraulics** | [offbeat-hydraulics](https://github.com/lukestark-glitch/offbeat-hydraulics) | Pumpen/Sidechain im Takt der Kick | Effekt | LV2, VST3, **LADSPA** | Violett, Lowrider |

Alle Regler stehen jeweils in `docs/REGLER.md` des Repos.

## Installation

In jedem Repo:

```
git clone --recurse-submodules https://github.com/lukestark-glitch/<repo>
cd <repo>
make install
```

`make install` baut das Plugin, signiert es ad-hoc und kopiert es:
- **VST3** nach `~/Library/Audio/Plug-Ins/VST3/`
- **LV2** nach `~/Library/Audio/Plug-Ins/LV2/`
- **LADSPA** (nur Turbo und Hydraulics) nach `~/Library/Audio/Plug-Ins/LADSPA/` und in den LMMS-LADSPA-Ordner (`laddir` aus `~/.lmmsrc.xml`, z. B. `~/Documents/lmms/plugins/ladspa/`)

Danach Carla bzw. LMMS neu starten.

## LMMS 1.2.2: das Wichtigste

- **Instrumente in Carla als LV2 laden.** VST3 meldet in Carla rund 2000 MIDI-CC-Parameter; LV2 zeigt nur die echten Regler. CLAP kann Carla 2.5 nicht.
- **Noten-Versatz:** LMMS schickt Noten **12 Halbtöne tiefer**, als die Piano-Roll zeigt (LMMS-Taste A4 kommt als MIDI 45 an). Deshalb bei Engine und Topline den Regler **Octave auf +1** stellen. Dann klingt F1 wie F1 (Kick auf F1, Bass auf F2).
- **Tempo:** Laut seinen Symbolen ruft LMMS das Tempo und die Taktposition für Carla ab (`CarlaInstrument::handleGetTimeInfo`, `Song::getTempo`).
  - Engine, Topline und Hydraulics nutzen dieses Host-Tempo.
  - Kommt es nicht an, schaltest du **Host-Tempo aus** und stellst **BPM** von Hand. Das betrifft bei Hydraulics die Quelle „Host-Takt“.
  - Ob das Host-Tempo in Carla Rack wirklich ankommt, muss noch ein Test in LMMS zeigen.
- **Effekte auf dem Master oder auf Spuren als LADSPA laden.** Ein Carla Rack ist in LMMS ein *Instrument*; Effekte darin wirken nur auf Klänge aus demselben Carla. Darum gibt es Turbo und Hydraulics zusätzlich als LADSPA. Im FX-Mixer (F9) fügst du sie mit „Add effect“ ein.
  - LADSPA hat keine eigene Oberfläche (nur einfache Regler) und kein Tempo.
- **Pumpen im Takt (Hydraulics in LMMS):** Auf der Kick-Spur einen **Peak Controller** einfügen und ihn mit dem Regler **Trigger** von Hydraulics verbinden. Das ergibt echtes Sidechain-Pumpen, unabhängig vom Tempo. Die Schritte stehen in `docs/REGLER.md` von offbeat-hydraulics.
- **Lautheit prüfen:** Den Song als WAV exportieren und `bin/turbo-render --in song.wav` ausführen (im Turbo-Repo). Es zeigt RMS und True-Peak vorher und nachher. Ziel im Drop sind −6 bis −9 dBFS RMS.

## Gemeinsamer Code

Die gemeinsamen Teile sind eine bewusste Kopie in jedem Repo; die Quelle ist offbeat-engine:
- `src/dsp/ParamInfo.hpp`, `src/dsp/ParamText.*`, `src/dsp/Blocks.hpp`
- `plugin/OffbeatKit.hpp`, `plugin/OffbeatParam.hpp`
- `tools/Measure.hpp`

Warum Kopie und kein Submodule, steht in `docs/PLAN.md`. Änderungen macht man zuerst hier und kopiert sie dann in die anderen Repos.

## Feste IDs (nie ändern)

| Plugin | LV2-URI | VST3 | Bundle |
|---|---|---|---|
| Engine | `https://github.com/lukestark-glitch/offbeat-synth` (alter Name, absichtlich) | `Ofbt`/`OfbS` | `offbeat` |
| Topline | `https://github.com/lukestark-glitch/offbeat-topline` | `Ofbt`/`OfbT` | `offbeat-topline` |
| Turbo | `https://github.com/lukestark-glitch/offbeat-turbo` | `Ofbt`/`OfbU` | `offbeat-turbo` (+ `offbeat-turbo-ladspa.dylib`) |
| Hydraulics | `https://github.com/lukestark-glitch/offbeat-hydraulics` | `Ofbt`/`OfbH` | `offbeat-hydraulics` (+ `offbeat-hydraulics-ladspa.dylib`) |

Parameter werden immer nur hinten angehängt.
