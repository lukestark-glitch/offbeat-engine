# OFFBEAT – Bounce-Synth (Spezifikation für Claude Code)

Ziel: Ein eigener Synthesizer für **Hard Bounce / Bounce Techno (150–165 BPM)**, der in **LMMS 1.2.2 auf macOS (Intel, MacBook Pro 2019)** über **Carla Rack** läuft. Ein Plugin, mehrere Modi, wenige, aber genau die richtigen Regler.

Diese Datei ist die Arbeitsgrundlage für Claude Code. Erst lesen, dann Plan vorschlagen, dann bauen.

---

## 1. Rahmenbedingungen (nicht verhandelbar)

- **Host-Kette:** LMMS 1.2.2 → Instrument „Carla Rack“ → Carla 2.5.10 → unser Plugin.
- **Formate:** **VST3** und **LV2** bauen. (Carla 2.5.x hostet auf macOS VST3, LV2, AU, VST2 – aber **noch kein CLAP**. CLAP optional zusätzlich, für spätere LMMS-Versionen.)
- **Framework:** **DPF (DISTRHO Plugin Framework)**, github.com/DISTRHO/DPF, als Git-Submodule. Grund: gleicher Autor wie Carla, baut VST3/LV2/CLAP aus einem Code, nur Make + Xcode Command Line Tools nötig.
- **Architektur:** x86_64 (Intel). Universal ist nicht nötig.
- **Erste Version ohne eigene Oberfläche:** Carla zeigt die Parameter als Regler an. Eine GUI kommt später (Phase 3).
- **Sprache:** C++17. DSP-Code getrennt von DPF-Code, damit er auch in einem Offline-Test-Tool läuft.
- **Installation:** `~/Library/Audio/Plug-Ins/VST3/` und `~/Library/Audio/Plug-Ins/LV2/`. Nach dem Build ad-hoc signieren: `codesign --force --deep -s - <bundle>`.

## 2. Klang-Referenz (gemessen)

Analyse eines Hard-Bounce-Referenztracks (160 BPM, Bass auf F2):

**Kick (Mittel über viele Schläge eines Drops)**
| Zeit | Tonhöhe |
|---|---|
| 0–8 ms | Klick, Schwerpunkt ~1,4 kHz |
| ~20 ms | ~150 Hz |
| ~36 ms | ~106 Hz |
| ~68 ms | ~74 Hz |
| ~97 ms | ~61 Hz |
| ~150 ms | ~48 Hz |
| ~170 ms | ~45 Hz (Endton ≈ F1) |

- Lautstärke: voll ab ~10 ms, bleibt bis ~150 ms fast konstant (stark gesättigt/verdichtet), harter Abbruch bis ~185 ms – **endet genau vor dem Offbeat** (bei 160 BPM nach 187,5 ms).
- Gute Näherung der Tonhöhenkurve: `f(t) = f_end + 200·e^(−t/18ms) + 60·e^(−t/75ms)`, f_end = 43,65 Hz (F1).

**Bass:** Offbeat (2. Achtel jedes Schlags), Grundton **F2 (≈ 87 Hz)**, kurz, Kick und Bass überlappen nicht.

**Hats (vom Nutzer gewünscht):** nicht spitz/glasig, wenig „tzzz“. Klangschwerpunkt ~6–7 kHz, kaum Energie über 10 kHz. Closed ~60 ms, Open ~220 ms.

**Stil-Ziel:** Hard-Bounce-Sound à la 240 KM/H: schnell, verspielte, groovige Offbeat-Bässe, harte Kicks, Rave-Stabs, Sirenen/Riser.

### Weitere Referenzen (6 Lieblingstracks des Nutzers, je 16 Takte aus dem lautesten Drop gemessen)

Bounce (160–165 BPM): What Goes Around (Edit), After Party, Bongos On My Mind, Cash In Da Back. Groove (143–146 BPM): This My Chip, God Save The Groove.

- **Tempo:** Die Angaben des Nutzers stimmen exakt (160 / 163 / 165 / 160 / 146 / 143).
- **Bass-Rhythmus ist nicht immer Offbeat.** Drei Muster kommen vor (16tel eines Schlags = 1 2 3 4):
  - *Offbeat:* Bass auf 3 (Bounce Some)
  - *Late Bounce:* Bass auf 4, also kurz vor der nächsten Kick (After Party, sehr kurze Kick von nur ~70 ms bis −12 dB)
  - *Rolling:* Bass auf 3 und 4 bzw. durchgehender Sub (Bongos On My Mind, Cash In Da Back, This My Chip)
  → Der Bass-Modus braucht kurze **und** rollende Einstellungen; Presets für alle drei Muster.
- **Kick-Länge** schwankt zwischen ~70 ms (After Party) und ~185 ms (Bounce Some). Bei rollenden Bässen geht die Kick nahtlos in den Sub über.
- **Bassnoten** liegen zwischen D1 und F#3; Molltonarten, F-Moll passt.
- **Frequenz-Balance (relativ zum Oktavband 63 Hz):** 125 Hz −1 bis −3 dB · 250 Hz −7 bis −13 dB · 1 kHz −11 bis −22 dB · 4 kHz −14 bis −21 dB · 8 kHz −15 bis −25 dB · 16 kHz −23 bis −31 dB. Das lauteste Band ist **63 Hz**.
- **Stereo:** unter 150 Hz komplett mono (Korrelation 1,0), Höhen über 2 kHz breit (Korrelation 0,01–0,87). → Width-Parameter wie in Abschnitt 3 beschrieben.
- **Lautheit:** Drops gemastert bei −6 bis −9 dBFS RMS, Spitzen ~0 dBFS. (Das Plugin selbst muss das nicht erreichen; nur sauber begrenzen.)
- **Zusätzlicher Wunsch daraus:** Bass-Modus mit Option **„Rumble / Roll“** (durchgehender, tief gefilterter Sub zwischen den Kicks, BPM-synchron in 16teln moduliert).

## 3. Modi und Parameter

Ein Parameter **Mode** (Kick / Bass / Hat / Stab / Pad / Riser). Jeder Modus nutzt eigene Parameter; nicht genutzte bleiben ohne Wirkung. Alle Parameter mit sinnvollem Default = Preset-Startwert. Globale Parameter: **Volume, Drive, Tone (Low-Pass nach Drive), Width** (Width wirkt nie unter 150 Hz – Bass bleibt mono).

### Kick (monophon, neuer Ton startet Kick neu)
- Start Pitch (100–500 Hz, Default 280)
- End Pitch: Hz **oder** aus MIDI-Note (Toggle „Tune to Note“, Default an, F1)
- Punch Time (5–60 ms, Default 18) – schnelle Phase der Tonhöhenkurve
- Body Time (20–200 ms, Default 75) – langsame Phase
- Length (60–600 ms, Default 185)
- Cut (5–60 ms, Default 30) – wie hart das Ende abbricht
- Click (0–1, Default 0,2) – gefiltertes Rauschen 0,9–3,5 kHz, 2–8 ms. Achtung: Nutzer fand starken Klick „wie Aux-Kabel einstecken“ → Default niedrig, sanfter Einsatz (≥ 0,5 ms Fade-in).
- Drive (0–1, Default 0,5) – tanh-Sättigung, hält den Body flach und laut
- Optional: „Bounce Sync“ – Length automatisch = 95 % einer Achtel bei Host-BPM

### Bass (Offbeat-Bounce, monophon mit Glide)
- Wave Mix: Saw ↔ Square
- Sub Level (Sinus −1 Oktave)
- Detune (zweiter Oszillator, Cent)
- Cutoff, Resonance (TPT-SVF oder Ladder, 12/24 dB)
- Filter Env Amount + Decay (Pluck)
- Amp Decay (Default kurz, Sustain 0)
- Bounce Pitch (0–24 Halbtöne, Default 7): Tonhöhe startet höher und fällt in „Bounce Time“ (5–60 ms) auf den Zielton – gibt den federnden „Boing“
- Glide (0–200 ms)

### Hat (Closed/Open)
- Metal ↔ Noise Mix (6 inharmonische Rechtecke, klassisches Drum-Machine-Prinzip; Frequenzverhältnisse 205,3 / 304,4 / 369,6 / 522,7 / 540 / 800 Hz, skalierbar)
- Pitch (Skalierung der Metall-Frequenzen)
- High-Pass (2–8 kHz), Low-Pass (6–14 kHz, Default 7,5 kHz)
- Decay Closed (Default 12 ms Zeitkonstante, ~60 ms Länge), Decay Open (Default 70 ms, ~220 ms Länge)
- Choke: MIDI-Note bestimmt Closed/Open (z. B. F#3 = Closed, A#3 = Open); Closed würgt eine klingende Open ab.

### Stab / Lead (polyphon, 8 Stimmen)
- Supersaw (1–7 Stimmen pro Note), Detune, Mix
- Filter + Filter-Env, Amp-ADSR
- „Hoover“-Pitch-Drop am Ton-Ende (optional)
- Chord-Hold (spielt auf eine Taste einen Moll-/Dur-Akkord)

### Pad (polyphon)
- Supersaw + Dreieck (+12), langsamer Attack, langer Release
- Low-Pass + langsamer LFO auf Cutoff (BPM-synchron)
- Eingebauter Chorus
- Ziel-Charakter „milchig / wie im Traum“: weich, gedämpfte Höhen, breit

### Riser
- Länge in Takten (1–32), **synchron zur Host-BPM** (DPF TimePosition)
- Noise ↔ Saw Mix
- Filter-Sweep (Start/End-Cutoff), Pitch-Rise (0–24 Halbtöne), Lautstärke-Rise
- Sirenen-Option: LFO auf Pitch (Rate in Notenwerten)

## 4. Presets (Werkspresets, F-Moll, 150–160 BPM)

1. **Kick – Hard Bounce** (Messwerte aus Abschnitt 2)
2. **Kick – Bounce Soft** (weniger Drive, weniger Klick)
3. **Bass – Offbeat F2** (Saw, Sub 50 %, Bounce Pitch 7, kurzer Decay)
4. **Bass – Rubber** (mehr Bounce Pitch, Glide)
4b. **Bass – Late Bounce** (sehr kurz, für Bass auf der 4. Sechzehntel, zusammen mit kurzer Kick ~70 ms)
4c. **Bass – Rolling Sub** (Rumble/Roll an, 16tel-Bewegung, für Groove-Tracks um 143–146 BPM)
5. **Hat – Closed Soft** / **Hat – Open Soft** (Werte aus Abschnitt 2)
6. **Stab – Rave** (7-Stimmen-Supersaw, kurz)
7. **Lead – Flow** (Glide, längerer Release)
8. **Pad – Milchglas** (siehe Pad-Ziel)
9. **Riser – 8 Takte** / **Siren – 16 Takte**

## 5. Test ohne DAW (wichtig)

Ein Kommandozeilen-Tool `offbeat-render` nutzt denselben DSP-Code, rendert Noten (Mode, Preset, Note, BPM, Länge) in eine WAV-Datei und gibt Messwerte aus. Damit lassen sich Sounds ohne LMMS prüfen und mit dem Nutzer vergleichen.

Abnahmekriterien Kick-Preset „Hard Bounce“ bei Note F1:
- Tonhöhe bei 20 ms: 140–160 Hz; bei 97 ms: 55–65 Hz; bei 150 ms: 45–52 Hz
- Pegel bei 190 ms mindestens 30 dB unter Maximum
- Kein Klick/Knacken am Anfang und Ende (Fades), keine Denormals, kein Clipping über 0 dBFS

## 6. Phasen

1. **Phase 1:** DPF-Gerüst, Kick- und Bass-Modus, Offline-Renderer, Build + Installation, Test in Carla und LMMS.
2. **Phase 2:** Hat, Stab, Pad, Riser, Presets.
3. **Phase 3 (optional):** Eigene Oberfläche (DPF DGL oder ImGui) mit großen Reglern pro Modus.

## 7. Arbeitsweise mit dem Nutzer

- Nutzer: Informatikstudent, kennt C, Python, Java. Erklärungen kurz und konkret, auf Deutsch.
- Nach jedem Schritt: was gebaut wurde, wie man es testet (in Carla: „Refresh“ → Plugin laden).
- Arbeitsregeln (Agents, Tests, Commits, Benachrichtigungen, Installation): siehe `CLAUDE.md`.
