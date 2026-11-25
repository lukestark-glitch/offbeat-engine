// offbeat-render: rendert eine Note mit dem DSP-Kern in eine WAV-Datei und misst sie.
//
//   offbeat-render --preset "Kick - Hard Bounce" --note F1 --bpm 160 --out kick.wav --check
//   offbeat-render --mode bass --note F2 --length 150 --set bass_bounce=12
//   offbeat-render --list
#include "Engine.hpp"

#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace offbeat;

static int parseNote(const char* s)
{
    if (std::isdigit((unsigned char)s[0]) || s[0] == '-')
        return std::atoi(s);
    static const int pcs[7] = { 9, 11, 0, 2, 4, 5, 7 }; // A..G
    const int c = std::toupper((unsigned char)s[0]);
    if (c < 'A' || c > 'G')
        return -1;
    int pc = pcs[c - 'A'];
    ++s;
    if (*s == '#')
        ++pc, ++s;
    else if (*s == 'b')
        --pc, ++s;
    return (std::atoi(s) + 1) * 12 + pc; // F1 = 29, A4 = 69
}

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

static bool writeWav(const char* path, const std::vector<float>& l, const std::vector<float>& r, int sr)
{
    FILE* f = std::fopen(path, "wb");
    if (!f)
        return false;
    const uint32_t frames = (uint32_t)l.size(), dataBytes = frames * 8;
    auto u32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
    auto u16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
    std::fwrite("RIFF", 1, 4, f), u32(36 + dataBytes), std::fwrite("WAVEfmt ", 1, 8, f);
    u32(16), u16(3), u16(2), u32((uint32_t)sr), u32((uint32_t)sr * 8), u16(8), u16(32); // float32 stereo
    std::fwrite("data", 1, 4, f), u32(dataBytes);
    for (uint32_t i = 0; i < frames; ++i)
        std::fwrite(&l[i], 4, 1, f), std::fwrite(&r[i], 4, 1, f);
    return std::fclose(f) == 0;
}

static float toDb(float x) { return 20.0f * std::log10(x > 1e-12f ? x : 1e-12f); }

// Tonhöhe bei Zeit t: eine Periode aus den Nulldurchgängen rund um t.
static float pitchAt(const std::vector<float>& x, int sr, float tMs)
{
    std::vector<double> zc;
    for (size_t i = 1; i < x.size(); ++i)
        if ((x[i - 1] < 0.0f) != (x[i] < 0.0f))
            zc.push_back((double)(i - 1) + x[i - 1] / (double)(x[i - 1] - x[i]));
    const double t = tMs * 0.001 * sr;
    for (size_t k = 1; k + 1 < zc.size(); ++k)
        if (zc[k] <= t && t < zc[k + 1])
            return (float)(sr / (zc[k + 1] - zc[k - 1]));
    return 0.0f;
}

static float peakIn(const std::vector<float>& x, int sr, float fromMs, float toMs)
{
    float p = 0.0f;
    const size_t a = (size_t)(fromMs * 0.001f * sr), b = (size_t)(toMs * 0.001f * sr);
    for (size_t i = a; i < b && i < x.size(); ++i)
        p = std::fmax(p, std::fabs(x[i]));
    return p;
}

static void usage()
{
    std::puts("offbeat-render [--preset NAME|NR] [--mode kick|bass] [--note F1|29] [--velocity 0-127]\n"
              "               [--bpm 160] [--length MS] [--tail MS] [--sr 48000] [--set symbol=wert]...\n"
              "               [--out datei.wav] [--check] [--list]");
}

int main(int argc, char** argv)
{
    int preset = -1, mode = -1, note = 29, velocity = 127, sr = 48000;
    float bpm = 160.0f, lengthMs = 300.0f, tailMs = 300.0f;
    const char* out = nullptr;
    bool check = false;
    std::vector<std::pair<int, float>> overrides;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        const char* v = i + 1 < argc ? argv[i + 1] : nullptr;
        if (a == "--list") {
            std::puts("Presets:");
            for (int p = 0; p < kPresetCount; ++p)
                std::printf("  %d  %s\n", p, kPresets[p].name);
            std::puts("Parameter (symbol: min..max, default):");
            for (const ParamInfo& p : kParams)
                std::printf("  %-14s %g..%g %s, %g\n", p.symbol, p.min, p.max, p.unit, p.def);
            return 0;
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
            note = parseNote(v);
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
    if (note < 0 || note > 127) {
        std::fputs("Ungültige Note\n", stderr);
        return 2;
    }

    float values[kParamCount];
    applyPreset(preset, values);
    if (mode >= 0)
        values[kMode] = (float)mode;
    for (const auto& o : overrides)
        values[o.first] = o.second;

    Engine engine;
    engine.setSampleRate((float)sr);
    for (int i = 0; i < kParamCount; ++i)
        engine.setParam(i, values[i]);

    // Transport läuft ab Schlag 0; Note startet auf Frame 0 und wird nach --length losgelassen.
    const uint32_t gateFrames = (uint32_t)(lengthMs * 0.001f * sr);
    const uint32_t total = gateFrames + (uint32_t)(tailMs * 0.001f * sr);
    std::vector<float> l(total), r(total);
    const uint32_t block = 256;
    engine.noteOn(note, velocity);
    for (uint32_t pos = 0; pos < total;) {
        engine.setTransport(true, (double)pos / sr * bpm / 60.0, bpm);
        uint32_t n = total - pos < block ? total - pos : block;
        if (pos < gateFrames && pos + n > gateFrames)
            n = gateFrames - pos; // Note-Off genau auf dem Frame
        engine.process(&l[pos], &r[pos], n);
        if (pos + n == gateFrames)
            engine.noteOff(note);
        pos += n;
    }

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

    std::printf("Mode %s, Preset %s, Note %d (%.2f Hz), %g BPM, %d Hz\n", kModeNames[(int)values[kMode]],
                preset >= 0 ? kPresets[preset].name : "(Defaults)", note, midiToHz((float)note), bpm, sr);
    std::printf("Spitze           %7.2f dBFS   RMS %7.2f dBFS\n", toDb(peak), toDb(std::sqrt(sumSq / total)));
    std::printf("Klingt bis       %7.1f ms\n", soundMs);
    std::puts("Tonhöhe:");
    const float times[] = { 5, 10, 20, 36, 68, 97, 150, 170 };
    for (float t : times)
        std::printf("  %5.0f ms  %7.1f Hz\n", t, pitchAt(mono, sr, t));
    std::puts("Pegel (Spitze im 10-ms-Fenster, relativ zum Maximum):");
    for (float t : { 10.0f, 50.0f, 100.0f, 150.0f, 170.0f, 180.0f, 190.0f })
        std::printf("  %5.0f ms  %7.1f dB\n", t, toDb(peakIn(mono, sr, t - 5, t + 5)) - toDb(peak));
    const float startDb = toDb(peakIn(mono, sr, 0, 0.25f)) - toDb(peak);
    const float endDb = toDb(peakIn(mono, sr, soundMs - 1.0f, soundMs + 1.0f)) - toDb(peak);
    std::printf("Anfang 0–0,25 ms %7.1f dB (erstes Sample %.5f)\n", startDb, std::fabs(mono[0]));
    std::printf("Ende (letzte ms) %7.1f dB\n", endDb);
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

    // Abnahmekriterien Spec Abschnitt 5 (Kick – Hard Bounce, Note F1).
    int failed = 0;
    auto expect = [&](const char* what, bool ok) {
        std::printf("  [%s] %s\n", ok ? "OK " : "FEHLER", what);
        failed += ok ? 0 : 1;
    };
    std::puts("Prüfung:");
    const float p20 = pitchAt(mono, sr, 20), p97 = pitchAt(mono, sr, 97), p150 = pitchAt(mono, sr, 150);
    expect("Tonhöhe 20 ms: 140–160 Hz", p20 >= 140 && p20 <= 160);
    expect("Tonhöhe 97 ms: 55–65 Hz", p97 >= 55 && p97 <= 65);
    expect("Tonhöhe 150 ms: 45–52 Hz", p150 >= 45 && p150 <= 52);
    expect("Pegel 190 ms ≥ 30 dB unter Maximum", toDb(peakIn(mono, sr, 190, 191)) - toDb(peak) <= -30.0f);
    expect("Weicher Anfang (erstes Sample < 0,001, 0–0,25 ms ≤ −12 dB)", std::fabs(mono[0]) < 1e-3f && startDb <= -12.0f);
    expect("Weiches Ende (letzte ms ≤ −30 dB)", endDb <= -30.0f);
    expect("Kein Clipping über 0 dBFS", over == 0 && peak <= 1.0f);
    expect("Keine Denormals, kein NaN/Inf", denormals == 0 && nonFinite == 0);
    std::printf("%s\n", failed ? "NICHT BESTANDEN" : "BESTANDEN");
    return failed ? 1 : 0;
}
