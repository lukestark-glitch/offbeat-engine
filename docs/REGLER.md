# Offbeat Engine – alle Regler

Oben in der Oberfläche wählst du den **Modus**. Darunter liegen die vier **globalen Regler**, die immer gelten, und die Regler des gewählten Modus. Ein Plugin spielt immer nur einen Modus. Für Kick, Bass und Hats nimmst du also je eine eigene Spur mit Offbeat.

**Bedienung:**
- Regler hoch oder runter ziehen, mit Shift geht es fein.
- Das Mausrad geht auch.
- Doppelklick setzt den Standardwert.
- An/Aus-Schalter schaltest du per Klick.
- **Klick auf den Wert** unter einem Regler (oder Rechtsklick auf den Regler) öffnet ein Eingabefeld. Zahl eintippen und Enter drücken, Esc bricht ab. Eingegeben wird in der Einheit der Anzeige: Prozent bei %-Reglern, `1.5k` für 1,5 kHz, Namen wie `Moll` und `an`/`aus` gehen auch.

Presets wählst du in Carla unter „Programs“.

## Global

| Regler | Bereich | Standard | Wirkung |
|---|---|---|---|
| Mode | Kick … Riser | Kick | Welcher Klang gespielt wird |
| Volume | −40 … +6 dB | −3 dB | Gesamtlautstärke (am Ende sitzt ein Limiter, der nicht übersteuert) |
| Drive | 0 … 100 % | 0 % | Sättigung auf alles, macht dichter und rauer |
| Tone | 200 Hz … 20 kHz | 20 kHz | Low-Pass auf alles, nach links wird es dumpfer |
| Width | 0 … 100 % | 50 % | Stereobreite; unter 150 Hz bleibt es immer mono |
| Octave | −2 … +2 | +1 | Verschiebt alle Noten um Oktaven. Der Standard **+1** gleicht LMMS aus: LMMS schickt Noten 12 Halbtöne tiefer, als die Piano-Roll zeigt (LMMS-Taste F1 kommt als F0 an). In anderen Hosts (z. B. Carla-Tastatur) auf 0 stellen |
| Host-Tempo | An/Aus | An | An: Tempo und Takt kommen vom Host. Aus (oder wenn der Host kein Tempo liefert): Es gilt BPM |
| BPM | 60 … 220 | 160 | Tempo für Bounce Sync, Roll, Pad-LFO und Riser, wenn kein Host-Tempo genutzt wird |

Octave, Host-Tempo und BPM stehen als kleine Felder in der Kopfzeile von GLOBAL: Ziehen ändert den Wert, ein Klick ohne Ziehen öffnet das Eingabefeld.

## Kick

| Regler | Bereich | Standard | Wirkung |
|---|---|---|---|
| Start Pitch | 100 … 500 Hz | 280 Hz | Tonhöhe am Anfang des Schlags (der „Punch“) |
| Tune to Note | An/Aus | An | An: Der Endton folgt der gespielten Note (F1 = 43,65 Hz). Aus: Es gilt End Pitch |
| End Pitch | 30 … 100 Hz | 43,65 Hz | Endton, wenn Tune to Note aus ist |
| Punch Time | 5 … 60 ms | 18 ms | Wie schnell der erste, steile Teil des Pitch-Falls ist |
| Body Time | 20 … 200 ms | 75 ms | Wie lange der zweite, langsame Teil braucht, also der „Bauch“ der Kick |
| Length | 60 … 600 ms | 185 ms | Gesamtlänge der Kick |
| Cut | 5 … 60 ms | 30 ms | Wie weich die Kick am Ende abgeschnitten wird |
| Attack | 0 … 30 ms | 1 ms | Weiche Einblendung am Anfang; 0 = harter Start mit Knack, länger = weicher |
| Click | 0 … 100 % | 20 % | Lautstärke des Klicks am Anfang |
| Drive | 0 … 100 % | 50 % | Verzerrung der Kick; hoch = härter, mehr Obertöne |
| Bounce Sync | An/Aus | Aus | An: Die Länge passt sich der Host-BPM an und endet kurz vor dem Offbeat |

## Bass

| Regler | Bereich | Standard | Wirkung |
|---|---|---|---|
| Wave | 0 … 100 % | 0 % | Wellenform: 0 = Sägezahn, 100 = Rechteck |
| Sub | 0 … 100 % | 50 % | Lautstärke des Sub-Oszillators eine Oktave tiefer (mono) |
| Detune | 0 … 50 ct | 8 ct | Verstimmung der zwei Oszillatoren (links/rechts) |
| Cutoff | 40 Hz … 10 kHz | 500 Hz | Grundhelligkeit des Filters |
| Resonance | 0 … 100 % | 25 % | Betonung an der Filterfrequenz |
| 24 dB | An/Aus | Aus | Steileres Filter (weicher, runder) |
| Filter Env | 0 … 100 % | 50 % | Wie stark das Filter beim Anschlag aufgeht |
| Filter Decay | 5 … 1000 ms | 90 ms | Wie schnell das Filter wieder zugeht |
| Amp Decay | 20 … 2000 ms | 170 ms | Wie lange der Ton klingt; kurz = knackiger Offbeat |
| Bounce Pitch | 0 … 24 st | 7 st | Pitch-Sprung von oben beim Anschlag, das ist der „Bounce“ |
| Bounce Time | 5 … 60 ms | 20 ms | Wie schnell der Bounce auf den Grundton fällt |
| Glide | 0 … 200 ms | 0 ms | Gleiten zwischen überlappend gespielten Noten |
| Roll | An/Aus | Aus | An: Der Ton wird solange die Taste gehalten wird auf jedem 16tel neu angeschlagen (rollender Bass) |

## Hat

Taste **A#** (z. B. A#3) spielt eine **Open** Hat, alle anderen Tasten spielen eine **Closed**. Eine Closed schneidet eine klingende Open ab.

| Regler | Bereich | Standard | Wirkung |
|---|---|---|---|
| Metal/Noise | 0 … 100 % | 45 % | Mischung aus metallischem Klang (0) und Rauschen (100) |
| Pitch | 0,5 … 2 x | 1 x | Tonhöhe des metallischen Anteils |
| High-Pass | 2 … 8 kHz | 6 kHz | Schneidet Tiefen weg; höher = dünner |
| Low-Pass | 6 … 14 kHz | 7,5 kHz | Schneidet Höhen weg; niedriger = weniger spitz |
| Decay Closed | 3 … 50 ms | 12 ms | Ausklingen der Closed Hat (Standard ≈ 60 ms Länge) |
| Decay Open | 20 … 300 ms | 70 ms | Ausklingen der Open Hat (Standard ≈ 220 ms Länge) |

## Stab (auch Lead)

Bis zu 8 Noten gleichzeitig, jede als „Supersaw“ aus mehreren verstimmten Sägezähnen.

| Regler | Bereich | Standard | Wirkung |
|---|---|---|---|
| Voices | 1 … 7 | 7 | Anzahl der Sägezähne pro Note; mehr = breiter, fetter |
| Detune | 0 … 100 % | 30 % | Wie stark sie gegeneinander verstimmt sind |
| Mix | 0 … 100 % | 70 % | Lautstärke der äußeren Sägezähne gegenüber den mittleren |
| Cutoff | 100 Hz … 16 kHz | 3 kHz | Grundhelligkeit des Filters |
| Resonance | 0 … 100 % | 20 % | Betonung an der Filterfrequenz |
| Filter Env | 0 … 100 % | 40 % | Wie stark das Filter beim Anschlag aufgeht |
| Filter Decay | 5 … 2000 ms | 200 ms | Wie schnell das Filter wieder zugeht |
| Attack | 0,5 … 2000 ms | 1 ms | Einschwingen |
| Decay | 5 … 3000 ms | 250 ms | Abfall nach dem Anschlag |
| Sustain | 0 … 100 % | 30 % | Pegel, solange die Taste gehalten wird |
| Release | 5 … 5000 ms | 120 ms | Ausklingen nach dem Loslassen |
| Hoover Drop | 0 … 24 st | 0 st | Ton rutscht beim Loslassen um so viele Halbtöne nach unten |
| Chord | Aus / Moll / Dur | Aus | Eine Taste spielt einen ganzen Dreiklang |
| Glide | 0 … 500 ms | 0 ms | Gleiten von der vorigen Note zur neuen |

## Pad

Weiche, breite Flächen mit Filter-Bewegung im Takt und Chorus.

| Regler | Bereich | Standard | Wirkung |
|---|---|---|---|
| Detune | 0 … 100 % | 25 % | Verstimmung der Sägezähne |
| Triangle +12 | 0 … 100 % | 40 % | Dreieck eine Oktave höher dazu, macht weicher und glasiger |
| Cutoff | 100 Hz … 12 kHz | 1,8 kHz | Grundhelligkeit des Filters |
| LFO Rate | 1/4 … 4 Takte | 2 Takte | Tempo der Filter-Bewegung, synchron zur Host-BPM |
| LFO Depth | 0 … 100 % | 40 % | Stärke der Filter-Bewegung |
| Attack | 5 … 5000 ms | 600 ms | Einblenden |
| Release | 20 … 8000 ms | 2000 ms | Ausklingen nach dem Loslassen |
| Chorus | 0 … 100 % | 60 % | Stärke des Stereo-Chorus |

## Riser

Taste halten, dann steigt der Klang über die eingestellte Zahl Takte an, synchron zur Host-BPM (4/4).

| Regler | Bereich | Standard | Wirkung |
|---|---|---|---|
| Bars | 1 … 32 | 8 | Länge des Anstiegs in Takten |
| Noise/Saw | 0 … 100 % | 40 % | Mischung aus Rauschen (0) und Sägezahn (100) |
| Cutoff Start | 100 Hz … 20 kHz | 300 Hz | Filter am Anfang |
| Cutoff End | 100 Hz … 20 kHz | 12 kHz | Filter am Ende |
| Pitch Rise | 0 … 24 st | 12 st | Wie weit die Tonhöhe ansteigt |
| Volume Rise | 0 … 100 % | 100 % | Wie stark es lauter wird (0 = gleich laut) |
| Siren Depth | 0 … 12 st | 0 st | Sirenen-Effekt: Tonhöhe schwingt auf und ab |
| Siren Rate | 1/1 … 1/16 | 1/4 | Tempo der Sirene, synchron zur Host-BPM |

## Einstellungen als JSON (Kopieren / Einfügen)

Oben rechts im Abschnitt GLOBAL gibt es zwei Knöpfe:
- **KOPIEREN** legt Modus, globale Regler (mit Oktave und Tempo) und die Regler des aktiven Modus als JSON in die Zwischenablage. So kannst du Claude genau zeigen, wie dein Sound eingestellt ist.
- **EINFÜGEN** liest JSON aus der Zwischenablage und stellt alle genannten Regler darauf ein. Regler, die im JSON nicht vorkommen, bleiben wie sie sind.

Beispiel:

```json
{
  "mode": "Kick",
  "volume": -3,
  "kick_start": 260,
  "kick_attack": 3,
  "kick_drive": 0.8,
  "kick_sync": true
}
```

**Regeln für das JSON:**
- **Schlüssel** sind die Kurznamen der Regler (`symbol`, Liste unten).
- **Zahlen** stehen in der Einheit des Plugins, also Hz, ms, dB oder st.
- **%-Regler** werden als **0 … 1** geschrieben, 0.8 bedeutet also 80 %.
- **Auswahl-Regler** gehen als Name oder Nummer, z. B. `"mode": "Bass"` oder `"stab_chord": "Moll"`.
- **Schalter** gehen als `true`/`false` oder `1`/`0`.
- **Zu große oder zu kleine Werte** werden auf den erlaubten Bereich begrenzt.
- **Unbekannte Schlüssel:** Die Statuszeile neben den Knöpfen meldet sie.

**Kurznamen:**
- **Global:** `mode volume drive tone width octave tempo_host tempo_bpm`
- **Kick:** `kick_start kick_tune kick_end kick_punch kick_body kick_length kick_cut kick_attack kick_click kick_drive kick_sync`
- **Bass:** `bass_wave bass_sub bass_detune bass_cutoff bass_reso bass_slope bass_envamt bass_fdecay bass_adecay bass_bounce bass_btime bass_glide bass_roll`
- **Hat:** `hat_mix hat_pitch hat_hp hat_lp hat_closed hat_open`
- **Stab:** `stab_voices stab_detune stab_mix stab_cutoff stab_reso stab_envamt stab_fdecay stab_attack stab_decay stab_sustain stab_release stab_hoover stab_chord stab_glide`
- **Pad:** `pad_detune pad_tri pad_cutoff pad_lfo_rate pad_lfo_depth pad_attack pad_release pad_chorus`
- **Riser:** `riser_bars riser_mix riser_start riser_end riser_pitch riser_vol riser_siren riser_rate`

Das gleiche JSON versteht auch der Offline-Renderer: `bin/offbeat-render --json sound.json --note F1`.

## Presets

| Nr. | Name | Modus | Idee |
|---|---|---|---|
| 1 | Kick – Hard Bounce | Kick | Harte, gesättigte Bounce-Kick (auf F1 spielen) |
| 2 | Kick – Bounce Soft | Kick | Weicher, weniger Klick |
| 3 | Bass – Offbeat F2 | Bass | Klassischer kurzer Offbeat-Bass |
| 4 | Bass – Rubber | Bass | Großer Bounce, Glide, länger |
| 5 | Bass – Late Bounce | Bass | Sehr kurz, für das 4. 16tel vor der Kick |
| 6 | Bass – Rolling Sub | Bass | Rollender Sub auf 16teln |
| 7 | Hat – Closed Soft | Hat | Weiche Hats |
| 8 | Hat – Open Soft | Hat | Etwas dunkler, mehr Rauschen |
| 9 | Stab – Rave | Stab | Kurzer Moll-Akkord-Stab |
| 10 | Lead – Flow | Stab | Gehaltener Lead mit Glide |
| 11 | Pad – Milchglas | Pad | Weiche, breite Fläche |
| 12 | Riser – 8 Takte | Riser | Rausch-Riser über 8 Takte |
| 13 | Siren – 16 Takte | Riser | Sirene über 16 Takte |
