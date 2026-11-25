# CLAUDE.md – Offbeat Engine (Repo offbeat-engine)

Projekt: eigener Synth-Plugin (VST3 + LV2, DPF) für Hard Bounce / Bounce Techno. Läuft in LMMS 1.2.2 über Carla Rack auf einem Intel-Mac (MacBook Pro 2019). Die vollständige Aufgabe steht in `OFFBEAT_SYNTH_SPEC.md`: zuerst lesen.

## Arbeitsweise

- **Suchen:** Wenn `graphify-out/` existiert, zuerst den Graphen nutzen (deckt Code und Doku ab). Er sollte zeigen, wo ein neuer Modus andockt: die Modus-Auswahl im DSP-Kern, die Parameter-Liste des DPF-Plugins, die Preset-Tabelle und der Offline-Renderer `offbeat-render`. Gezieltes grep nur für das, was der Graph nicht zeigt. Solange es noch keinen Graphen gibt, gezielt mit grep/glob suchen. Große Dateien nicht komplett lesen, wenn ein Ausschnitt reicht. `graphify-out/` nicht bearbeiten und nicht committen. Den DPF-Submodule nicht durchsuchen, außer für eine konkrete API-Frage.
- **Agents:** Bis auf Weiteres dürfen Subagents ohne Bedenken genutzt werden, auch parallel und mit jedem Modell, wenn das die Bauzeit verkürzt. **Sobald der Nutzer sagt, sparsamer zu sein, gilt ab dann:** einfache, klar umrissene Jobs (Dateien und Funktionen finden, Doku aktualisieren, Presets eintragen) gehen an Subagents mit Modell haiku oder sonnet; Opus nur für den Entwurf (DSP-Design, Architektur) und das Prüfen des Ergebnisses. Die übrigen Regeln gelten in beiden Fällen.
- **Erst schauen, dann bauen.** Vorhandenes wiederverwenden (DSP-Bausteine wie Oszillatoren, Hüllkurven, Filter nur einmal schreiben und von allen Modi nutzen). So wenig neuer Text wie möglich, keine Extra-Features über die Spezifikation hinaus.
- **Abhängigkeiten:** Einzige externe Abhängigkeit ist DPF (Git-Submodule). Keine weiteren Bibliotheken.
- **Tests:** Nur die betroffenen Tests laufen lassen. Ändert sich ein Modus, mit `offbeat-render` genau diesen Modus rendern und gegen die Abnahmekriterien der Spezifikation messen. Reine Doku-Änderungen brauchen keinen Testlauf.

## Offbeat-Familie

- Dieses Repo ist **Offbeat Engine** (früher offbeat-synth). Geschwister: offbeat-topline, offbeat-turbo, offbeat-hydraulics. Übersicht: `docs/FAMILIE.md`.
- **Gemeinsamer Code** (bewusste Kopie in jedem Repo, Quelle ist dieses Repo; bei Änderungen in die anderen zurückspielen): `src/dsp/ParamInfo.hpp`, `src/dsp/ParamText.*`, `src/dsp/Blocks.hpp`, `plugin/OffbeatKit.hpp`, `plugin/OffbeatParam.hpp`, `tools/Measure.hpp`.
- **Nie ändern:** LV2-URI (`https://github.com/lukestark-glitch/offbeat-synth`, enthält absichtlich den alten Repo-Namen), VST3-IDs (`Ofbt`/`OfbS`), Bundle-Name `offbeat`. Sonst laden gespeicherte LMMS-Projekte nicht mehr. Parameter nur **hinten anhängen**.
- **LMMS:** schickt Noten 12 Halbtöne tiefer als angezeigt (Regler Octave). Tempo kommt laut LMMS-Symbolen (`CarlaInstrument::handleGetTimeInfo`, `Song::getTempo`) über Carla an; Fallback: Host-Tempo aus + BPM.
- PRs dürfen nach grünen Checks selbst gemergt werden. `docs/HANDOFF.md` kurz aktuell halten.

## Regeln

- **Keine Screenshots, keine Browser-Tests.** Klang wird über `offbeat-render` und Messwerte geprüft. Was nur der Nutzer prüfen kann (Laden in Carla, Hören in LMMS), kommt als kurze Checkliste in den PR.
- **Entscheidungen, die beim Nutzer liegen:** die vorsichtigere, leicht änderbare Option wählen, unter „Offene Fragen“ im Plan notieren und weitermachen. Nicht warten.
- **Installation:** Das eigene Offbeat-Plugin in `~/Library/Audio/Plug-Ins/VST3/` und `~/Library/Audio/Plug-Ins/LV2/` darf ohne Nachfrage überschrieben werden. Andere Plugins und Dateien außerhalb des Projektordners nicht anfassen.
- **Komplett blockiert:** Push-Benachrichtigung senden und, wenn möglich, an einem anderen Teil weiterarbeiten.
- **Sichern:** Nach getaner Arbeit committen (mit den üblichen Attribution-Zeilen) und pushen, damit nichts verloren geht. Falls noch kein Remote existiert: lokal committen und unter „Offene Fragen“ notieren, kein Repository eigenmächtig anlegen. Wenn `docs/HANDOFF.md` existiert, kurz aktualisieren (was fertig ist, was als Nächstes kommt); sonst überspringen.

## Benachrichtigungen (PushNotification)

- Eine kurze Nachricht (1 Satz), wenn der PR offen ist, mit dem PR-Link.
- Eine, wenn du blockiert bist oder wegen des Session-Limits aufhörst, mit Stand und nächstem Schritt.
- Keine anderen Fortschrittsmeldungen.
