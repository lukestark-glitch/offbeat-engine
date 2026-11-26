// offbeat-render: rendert eine Note mit dem DSP-Kern in eine WAV-Datei und misst sie.
//
//   offbeat-render --preset "Kick - Hard Bounce" --note F1 --bpm 160 --out kick.wav --check
//   offbeat-render --mode bass --note F2 --length 150 --set bass_bounce=12
//   offbeat-render --preset "Hat - Open Soft" --note A#3 --check
//   offbeat-render --json sound.json --note F2     (JSON wie aus „Kopieren“ in der Oberfläche)
//   offbeat-render --list
//
// --note ist die Taste wie in der LMMS-Piano-Roll: der Renderer schickt sie wie LMMS 12 Halbtöne tiefer.
#include "Engine.hpp"
#include "Render.hpp"
#include "Preview.hpp"
#include "ParamText.hpp"
#include "Measure.hpp"

#include <algorithm>
#include <cctype>
#include <complex>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace offbeat;
using namespace measure;

static int findPreset(const char* s)
{
    for (int i = 0; i < kPresetCount; ++i)
        if (std::strcmp(kPresets[i].name, s) == 0)
            return i;
    char* end;
    const long i = std::strtol(s, &end, 10);
    return *end == 0 && i >= 0 && i < kPresetCount ? (int)i : -1;
}

static int findParam(const std::string& symbol)
{
    for (int i = 0; i < kParamCount; ++i)
        if (symbol == kParams[i].symbol)
            return i;
    return -1;
}

// Abnahme der Vorschau (AUFTRAG_BOUNCE_PREVIEW §3):
//  1. renderPreview() – das, was die Oberfläche zeichnet – ist bitgleich zum Renderer mit denselben Noten.
//  2. Vorhören über den Plugin-Ausgang (Parameter preview) knackst nicht: Start, Loop, Stopp, Host-Note.
static int previewCheck()
{
    int failed = 0;
    auto expect = [&](const std::string& what, bool ok) {
        std::printf("  [%s] %s\n", ok ? "OK " : "FEHLER", what.c_str());
        failed += ok ? 0 : 1;
    };
    const int sr = 48000;
    const double bpm = 160.0;
    std::puts("== Vorschau bitgleich zum Renderer (alle Presets, jede Ebene und alle, 1 Schlag und 1 Takt)");
    int compared = 0, differing = 0;
    for (int p = 0; p < kPresetCount; ++p) {
        float values[kParamCount];
        applyPreset(p, values);
        const PreviewSpec spec = previewSpec((int)values[kMode]);
        for (double beats : { 1.0, 4.0 })
            for (int layer = -1; layer < spec.layers; ++layer) {
                std::vector<float> al, ar;
                renderPreview(values, layer, beats, bpm, sr, al, ar);
                // Renderer-Weg: gleiche Noten, Engine wie in main() eingestellt (alle Parameter gesetzt)
                Engine engine;
                engine.setSampleRate((float)sr);
                for (int i = 0; i < kParamCount; ++i)
                    engine.setParam(i, values[i]);
                bool mk, mb;
                previewMute((int)values[kMode], layer, mk, mb);
                engine.setPreviewMute(mk, mb);
                NoteEvent ev[64];
                const int n = previewEvents(values, layer, beats, bpm, sr, ev, 64);
                std::vector<float> bl(al.size()), br(al.size());
                renderEvents(engine, std::vector<NoteEvent>(ev, ev + n), bpm, sr, bl.data(), br.data(), (uint32_t)bl.size());
                ++compared;
                differing += (al == bl && ar == br) ? 0 : 1;
            }
    }
    char buf[160];
    std::snprintf(buf, sizeof(buf), "%d Vorschau-Renderings, davon %d nicht bitgleich", compared, differing);
    expect(buf, differing == 0);

    std::puts("== Vorhören über den Plugin-Ausgang (Kick+Bass – Bounce 160, Loop): Start, Stopp, Host-Note");
    float values[kParamCount];
    applyPreset(kPresetCount - 1, values);
    Engine engine;
    engine.setSampleRate((float)sr);
    for (int i = 0; i < kParamCount; ++i)
        engine.setParam(i, values[i]);
    engine.setParam(kPreviewLoop, 1);
    const uint32_t total = (uint32_t)(3.0 * sr);
    std::vector<float> l(total), r(total);
    float maxStep = 0.0f, maxStepSteady = 0.0f;
    bool playedAll = false;
    for (uint32_t pos = 0; pos < total; pos += 256) {
        const double t = (double)pos / sr;
        engine.setTransport(false, 0.0, 0.0); // Host läuft nicht
        if (pos == 256)
            engine.setParam(kPreview, 4 | 8); // „Alle zusammen“, Loop
        if (pos == 256 * 200)
            engine.setParam(kPreview, 0); // Stopp
        if (pos == 256 * 300)
            engine.setParam(kPreview, 1); // nur Kick
        if (pos == 256 * 400)
            engine.noteOn(17, 110); // Host-Note (F1 nach LMMS-Versatz) hat Vorrang
        const uint32_t n = std::min<uint32_t>(256, total - pos);
        engine.process(&l[pos], &r[pos], n);
        if (pos > 256 && pos < 256 * 200)
            playedAll |= engine.getParam(kPreviewActive) == 4.0f;
        for (uint32_t i = pos + 1; i < pos + n; ++i) {
            const float step = std::fabs(l[i] - l[i - 1]);
            // Übergänge des Vorhörens: ±10 ms um Start, Stopp und Ebenen-Wechsel
            for (uint32_t at : { 256u, 256u * 200, 256u * 300 })
                if (i + sr / 100 > at && i < at + sr / 100)
                    maxStep = std::fmax(maxStep, step);
            if (t > 0.2 && t < 1.0)
                maxStepSteady = std::fmax(maxStepSteady, step);
        }
    }
    expect("Vorhören startet ohne laufenden Host (Anzeige „alle“)", playedAll);
    {
        // Play im Host: beendet das Vorhören, und solange er spielt, startet es nicht
        Engine e;
        e.setSampleRate((float)sr);
        for (int i = 0; i < kParamCount; ++i)
            e.setParam(i, values[i]);
        e.setParam(kPreviewLoop, 1);
        std::vector<float> bl(256), br(256);
        e.setTransport(false, 0.0, 0.0);
        e.process(bl.data(), br.data(), 256);
        e.setParam(kPreview, 4 | 8);
        e.process(bl.data(), br.data(), 256);
        const bool running = e.getParam(kPreviewActive) == 4.0f;
        e.setTransport(true, 0.0, 160.0); // Play
        e.process(bl.data(), br.data(), 256);
        const bool stoppedByPlay = e.getParam(kPreviewActive) == 0.0f;
        e.setParam(kPreview, 4); // Wunsch während der Host spielt (z. B. wiederhergestellter Parameter)
        e.process(bl.data(), br.data(), 256);
        expect("Play im Host beendet das Vorhören", running && stoppedByPlay);
        expect("Solange der Host spielt, startet kein Vorhören", e.getParam(kPreviewActive) == 0.0f);
    }
    expect("Host-Note beendet das Vorhören", engine.getParam(kPreviewActive) == 0.0f);
    std::snprintf(buf, sizeof(buf), "Kein Knackser an Start/Stopp/Wechsel: größter Sprung %.3f ≤ größter Sprung im laufenden Klang %.3f", maxStep, maxStepSteady);
    expect(buf, maxStep <= maxStepSteady * 1.01f);
    std::printf("%s\n", failed ? "NICHT BESTANDEN" : "BESTANDEN");
    return failed ? 1 : 0;
}

static void usage()
{
    std::puts("offbeat-render [--preset NAME|NR] [--mode kick|bass|hat|stab|pad|riser] [--note F1|29[@ms]]... [--velocity 0-127]\n"
              "               [--bpm 160] [--length MS] [--tail MS] [--sr 48000] [--json datei] [--set symbol=wert]...\n"
              "               [--out datei.wav] [--check] [--list]");
}

int main(int argc, char** argv)
{
    int preset = -1, mode = -1, note = 29, velocity = 127, sr = 48000;
    std::vector<std::pair<int, float>> notes; // Note, Start in ms
    float bpm = 160.0f, lengthMs = 300.0f, tailMs = 300.0f;
    const char* out = nullptr;
    const char* jsonFile = nullptr;
    bool check = false;
    bool bounce160 = false; // --target bounce160: Abnahme nach den Messwerten des Erklärvideos (docs/BOUNCE_VIDEO.md)
    std::vector<std::pair<int, float>> overrides;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        const char* v = i + 1 < argc ? argv[i + 1] : nullptr;
        if (a == "--preview-check")
            return previewCheck();
        if (a == "--preset-json" && v) { // Preset als JSON, genau wie KOPIEREN in der Oberfläche
            float vals[kParamCount];
            applyPreset(std::atoi(v), vals);
            int list[kParamCount];
            std::fputs(toJson(vals, list, jsonParams(vals, list)).c_str(), stdout);
            return 0;
        }
        if (a == "--list") {
            std::puts("Presets:");
            for (int p = 0; p < kPresetCount; ++p)
                std::printf("  %d  %s\n", p, kPresets[p].name);
            std::puts("Parameter (symbol: min..max, default):");
            for (const ParamInfo& p : kParams)
                std::printf("  %-14s %g..%g %s, %g\n", p.symbol, p.min, p.max, p.unit, p.def);
            return 0;
        }
        if (a == "--target" && v && std::string(v) == "bounce160") {
            bounce160 = true;
            ++i;
            continue;
        }
        if (a == "--check") {
            check = true;
            continue;
        }
        if (!v) {
            usage();
            return 2;
        }
        ++i;
        if (a == "--preset" && (preset = findPreset(v)) < 0) {
            std::fprintf(stderr, "Unbekanntes Preset: %s\n", v);
            return 2;
        } else if (a == "--mode") {
            for (int m = 0; m < kModeCount; ++m)
                if (strcasecmp(v, kModeNames[m]) == 0)
                    mode = m;
        } else if (a == "--note")
        {
            const char* at = std::strchr(v, '@');
            notes.emplace_back(parseNote(v), at ? (float)std::atof(at + 1) : 0.0f);
        }
        else if (a == "--velocity")
            velocity = std::atoi(v);
        else if (a == "--bpm")
            bpm = (float)std::atof(v);
        else if (a == "--length")
            lengthMs = (float)std::atof(v);
        else if (a == "--tail")
            tailMs = (float)std::atof(v);
        else if (a == "--sr")
            sr = std::atoi(v);
        else if (a == "--out")
            out = v;
        else if (a == "--json")
            jsonFile = v;
        else if (a == "--set") {
            const std::string kv = v;
            const size_t eq = kv.find('=');
            const int id = eq == std::string::npos ? -1 : findParam(kv.substr(0, eq));
            if (id < 0) {
                std::fprintf(stderr, "Unbekannter Parameter: %s\n", v);
                return 2;
            }
            overrides.emplace_back(id, (float)std::atof(kv.c_str() + eq + 1));
        } else if (a != "--preset") {
            usage();
            return 2;
        }
    }
    if (notes.empty())
        notes.emplace_back(note, 0.0f);
    note = notes[0].first; // Messungen und Prüfungen beziehen sich auf die erste Note
    bool badNote = false;
    for (const auto& nt : notes)
        badNote |= nt.first < 0 || nt.first > 127;
    if (badNote) {
        std::fputs("Ungültige Note\n", stderr);
        return 2;
    }

    float values[kParamCount];
    applyPreset(preset, values);
    if (mode >= 0)
        values[kMode] = (float)mode;
    if (jsonFile) {
        std::string json, errors;
        if (FILE* f = std::fopen(jsonFile, "rb")) {
            char buf[4096];
            for (size_t r; (r = std::fread(buf, 1, sizeof(buf), f)) > 0;)
                json.append(buf, r);
            std::fclose(f);
        }
        bool changed[kParamCount] = {};
        const int n = applyJson(json.c_str(), values, changed, errors);
        std::printf("JSON: %d Werte gesetzt%s%s\n", n, errors.empty() ? "" : "; ", errors.c_str());
        if (n == 0)
            return 2;
    }
    for (const auto& o : overrides)
        values[o.first] = o.second;

    Engine engine;
    engine.setSampleRate((float)sr);
    for (int i = 0; i < kParamCount; ++i)
        engine.setParam(i, values[i]);

    // Transport läuft ab Schlag 0. Jede Note startet zu ihrer Zeit (--note F1@100) und wird
    // nach --length losgelassen; Ereignisse fallen genau auf ihren Frame.
    std::vector<NoteEvent> events;
    float endMs = 0.0f;
    for (const auto& nt : notes) {
        // Wie LMMS: die Taste der Piano-Roll kommt 12 Halbtöne tiefer an (Octave +1 gleicht das aus)
        const int midi = nt.first - 12 < 0 ? 0 : nt.first - 12;
        events.push_back({ (uint32_t)(nt.second * 0.001f * sr), midi, velocity, true });
        events.push_back({ (uint32_t)((nt.second + lengthMs) * 0.001f * sr), midi, velocity, false });
        endMs = std::max(endMs, nt.second + lengthMs);
    }
    const uint32_t total = (uint32_t)((endMs + tailMs) * 0.001f * sr);
    std::vector<float> l(total), r(total);
    renderEvents(engine, events, bpm, sr, l.data(), r.data(), total); // wie im Plugin und in der Vorschau

    // Messwerte
    std::vector<float> mono(total);
    float peak = 0.0f, sumSq = 0.0f;
    int denormals = 0, over = 0, nonFinite = 0;
    size_t lastSound = 0;
    for (uint32_t i = 0; i < total; ++i) {
        for (float s : { l[i], r[i] }) {
            const float a = std::fabs(s);
            if (!std::isfinite(s))
                ++nonFinite;
            if (a > 0.0f && a < FLT_MIN)
                ++denormals;
            if (a > 1.0f)
                ++over;
        }
        mono[i] = 0.5f * (l[i] + r[i]);
        peak = std::fmax(peak, std::fabs(mono[i]));
        sumSq += mono[i] * mono[i];
        if (std::fabs(mono[i]) > 1e-5f)
            lastSound = i;
    }
    const float soundMs = (float)lastSound * 1000.0f / sr;

    const int modeIdx = (int)values[kMode];
    const float lenMs = lengthTo(mono, sr, peak * 0.01f); // bis −40 dB
    std::printf("Mode %s, Preset %s, Note %d (%.2f Hz), %g BPM, %d Hz\n", kModeNames[modeIdx],
                preset >= 0 ? kPresets[preset].name : "(Defaults)", note, midiToHz((float)note), bpm, sr);
    std::printf("Spitze           %7.2f dBFS   RMS %7.2f dBFS\n", toDb(peak), toDb(std::sqrt(sumSq / total)));
    std::printf("Klingt bis       %7.1f ms (−40 dB: %.1f ms)\n", soundMs, lenMs);
    if (modeIdx == kModeKick || modeIdx == kModeBass || modeIdx == kModeCombo) {
        std::puts("Tonhöhe (Nulldurchgänge, nur bei sinusartigem Signal aussagekräftig):");
        for (float t : { 5.0f, 10.0f, 20.0f, 36.0f, 68.0f, 97.0f, 150.0f, 170.0f })
            std::printf("  %5.0f ms  %7.1f Hz\n", t, pitchAt(mono, sr, t));
        std::puts("Pegel (Spitze im 10-ms-Fenster, relativ zum Maximum):");
        for (float t : { 10.0f, 50.0f, 100.0f, 150.0f, 170.0f, 180.0f, 190.0f })
            std::printf("  %5.0f ms  %7.1f dB\n", t, toDb(peakIn(mono, sr, t - 5, t + 5)) - toDb(peak));
    }
    const Spectrum spec = spectrum(mono, sr, 0, lastSound + 1);
    std::printf("Oktavbänder rel. 63 Hz:");
    for (int k = 0; k < 9; ++k)
        std::printf(" %g:%+.0f", kBandHz[k], spec.bands[k] - spec.bands[0]);
    std::printf("\nSchwerpunkt      %7.0f Hz   Anteil > 10 kHz %.1f dB\n", spec.centroid, spec.above10k);
    const float corrLow = correlation(l, r, 150.0f, sr);
    std::printf("Korrelation L/R  %7.2f gesamt, ", correlation(l, r, 0.0f, sr));
    if (corrLow > 1.5f)
        std::puts("unter 150 Hz kaum Energie");
    else
        std::printf("%.3f unter 150 Hz\n", corrLow);
    const float startDb = toDb(peakIn(mono, sr, 0, 0.25f)) - toDb(peak);
    const float endDb = toDb(peakIn(mono, sr, soundMs - 1.0f, soundMs + 1.0f)) - toDb(peak);
    const bool endsInRender = lastSound + (size_t)(0.002f * sr) < total;
    std::printf("Anfang 0–0,25 ms %7.1f dB (erstes Sample %.5f)\n", startDb, std::fabs(mono[0]));
    std::printf("Ende (letzte ms) %7.1f dB%s\n", endDb, endsInRender ? "" : " (klingt beim Render-Ende noch)");
    std::printf("Über 0 dBFS %d, Denormals %d, NaN/Inf %d\n", over, denormals, nonFinite);

    if (out) {
        if (!writeWav(out, l, r, sr)) {
            std::fprintf(stderr, "Konnte %s nicht schreiben\n", out);
            return 1;
        }
        std::printf("WAV: %s\n", out);
    }

    if (!check)
        return 0;

    int failed = 0;
    auto expect = [&](const char* what, bool ok) {
        std::printf("  [%s] %s\n", ok ? "OK " : "FEHLER", what);
        failed += ok ? 0 : 1;
    };
    std::puts("Prüfung:");
    // Bounce 160 (Erklärvideo): Kick-Tonhöhe ab Einsatz 208/152/97/69 Hz bei 20/36/68/97 ms (±10 %).
    if (bounce160 && (modeIdx == kModeKick || modeIdx == kModeCombo)) {
        const float ms[4] = { 20, 36, 68, 97 }, hz[4] = { 208, 152, 97, 69 };
        for (int k = 0; k < 4; ++k) {
            const float f = pitchAt(mono, sr, ms[k]);
            char what[96];
            std::snprintf(what, sizeof(what), "Bounce 160: Tonhöhe %.0f ms %.0f Hz ±10 %% (gemessen %.1f)", ms[k], hz[k], f);
            expect(what, std::fabs(f / hz[k] - 1.0f) <= 0.10f);
        }
    }
    if (modeIdx == kModeCombo) {
        // Kick (bis zum Offbeat) und Bass (Offbeat bis nächster Schlag) gleich laut; Dichte = Crest über einen Schlag.
        const float eighthMs = 30000.0f / bpm;
        // gleich laut = gleiche RMS der Abschnitte (so ist es im Erklärvideo gemessen; die Spitze kommt vom Bounce-Sprung)
        const float kickDb = toDb(rmsIn(mono, sr, 0, eighthMs - 5)), bassDb = toDb(rmsIn(mono, sr, eighthMs + 5, 2 * eighthMs - 5));
        const float crest = toDb(peakIn(mono, sr, 0, 2 * eighthMs)) - toDb(rmsIn(mono, sr, 0, 2 * eighthMs));
        char what[120];
        std::snprintf(what, sizeof(what), "Kick und Bass gleich laut ±1 dB (RMS Kick %.1f, Bass %.1f dBFS)", kickDb, bassDb);
        expect(what, std::fabs(kickDb - bassDb) <= 1.0f);
        if (bounce160) {
            std::snprintf(what, sizeof(what), "Dichte: Crest über einen Schlag 6,5–8 dB (gemessen %.1f)", crest);
            expect(what, crest >= 6.5f && crest <= 8.0f);
        } else {
            std::printf("  Crest über einen Schlag %.1f dB\n", crest);
        }
    }
    if (modeIdx == kModeKick && !bounce160) {
        // Spec Abschnitt 5 (Kick – Hard Bounce, Note F1).
        const float p20 = pitchAt(mono, sr, 20), p97 = pitchAt(mono, sr, 97), p150 = pitchAt(mono, sr, 150);
        expect("Tonhöhe 20 ms: 140–160 Hz", p20 >= 140 && p20 <= 160);
        expect("Tonhöhe 97 ms: 55–65 Hz", p97 >= 55 && p97 <= 65);
        expect("Tonhöhe 150 ms: 45–52 Hz", p150 >= 45 && p150 <= 52);
        expect("Pegel 190 ms ≥ 30 dB unter Maximum", toDb(peakIn(mono, sr, 190, 191)) - toDb(peak) <= -30.0f);
        if (values[kKickAttack] > 0.0f) // Attack 0: harter Start gewollt (keine Rundung am Anfang)
            expect("Weiche Einblendung (0–0,25 ms ≤ −12 dB)", startDb <= -12.0f);
    } else if (modeIdx == kModeHat) {
        // Spec Abschnitt 2: Schwerpunkt ~6–7 kHz, kaum Energie über 10 kHz, Closed ~60 ms, Open ~220 ms.
        const bool open = note % 12 == 10;
        expect("Schwerpunkt 5,5–7,5 kHz", spec.centroid >= 5500 && spec.centroid <= 7500);
        expect("Energie über 10 kHz ≤ −10 dB", spec.above10k <= -10.0f);
        if (open)
            expect("Open: Länge (−40 dB) 190–250 ms", lenMs >= 190 && lenMs <= 250);
        else
            expect("Closed: Länge (−40 dB) 50–70 ms", lenMs >= 50 && lenMs <= 70);
    } else if (modeIdx == kModeRiser) {
        // Synchron: Anstieg über "Bars" Takte bei der BPM, lauter und heller zum Ende hin.
        const float riseMs = values[kRiserBars] * 4.0f * 60000.0f / bpm;
        const float q = riseMs * 0.1f;
        const float rmsStart = rmsIn(mono, sr, 0, q), rmsEnd = rmsIn(mono, sr, riseMs - q, riseMs);
        const Spectrum a0 = spectrum(mono, sr, 0, (size_t)(q * 0.001f * sr));
        const Spectrum a1 = spectrum(mono, sr, (size_t)((riseMs - q) * 0.001f * sr), (size_t)(riseMs * 0.001f * sr));
        std::printf("  Riser %.0f ms: RMS Anfang %.1f dB, Ende %.1f dB; Schwerpunkt %.0f → %.0f Hz\n", riseMs,
                    toDb(rmsStart), toDb(rmsEnd), a0.centroid, a1.centroid);
        expect("Note lang genug (--length ≥ Riser-Dauer)", lengthMs >= riseMs);
        expect("Lautstärke steigt (Ende ≥ Anfang + 6 dB)", toDb(rmsEnd) >= toDb(rmsStart) + 6.0f);
        expect("Filter-Sweep (Schwerpunkt Ende > Anfang)", a1.centroid > a0.centroid);
    }
    if (modeIdx == kModeBass || modeIdx == kModeStab || modeIdx == kModePad)
        expect("Mono unter 150 Hz (Korrelation ≥ 0,98)", corrLow >= 0.98f);
    expect("Kein Sprung am Anfang (erstes Sample < 0,001)", std::fabs(mono[0]) < 1e-3f);
    if (endsInRender)
        expect("Weiches Ende (letzte ms ≤ −30 dB)", endDb <= -30.0f);
    expect("Kein Clipping über 0 dBFS", over == 0 && peak <= 1.0f);
    expect("Keine Denormals, kein NaN/Inf", denormals == 0 && nonFinite == 0);
    std::printf("%s\n", failed ? "NICHT BESTANDEN" : "BESTANDEN");
    return failed ? 1 : 0;
}
