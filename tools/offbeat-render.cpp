// offbeat-render: rendert eine Note mit dem DSP-Kern in eine WAV-Datei und misst sie.
//
//   offbeat-render --preset "Kick - Hard Bounce" --note F1 --bpm 160 --out kick.wav --check
//   offbeat-render --mode bass --note F2 --length 150 --set bass_bounce=12
//   offbeat-render --preset "Hat - Open Soft" --note A#3 --check
//   offbeat-render --list
#include "Engine.hpp"

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

static float rmsIn(const std::vector<float>& x, int sr, float fromMs, float toMs)
{
    double sum = 0.0;
    size_t a = (size_t)(fromMs * 0.001f * sr), b = (size_t)(toMs * 0.001f * sr), n = 0;
    for (size_t i = a; i < b && i < x.size(); ++i, ++n)
        sum += (double)x[i] * x[i];
    return n ? (float)std::sqrt(sum / n) : 0.0f;
}

// Zeit, nach der die Spitze in 1-ms-Fenstern zuletzt über `threshold` liegt.
static float lengthTo(const std::vector<float>& x, int sr, float threshold)
{
    size_t last = 0;
    for (size_t i = 0; i < x.size(); ++i)
        if (std::fabs(x[i]) > threshold)
            last = i;
    return (float)last * 1000.0f / sr;
}

static void fft(std::vector<std::complex<double>>& a)
{
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap(a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        const std::complex<double> w = std::polar(1.0, -2.0 * M_PI / (double)len);
        for (size_t i = 0; i < n; i += len) {
            std::complex<double> wk = 1.0;
            for (size_t k = 0; k < len / 2; ++k, wk *= w) {
                const std::complex<double> u = a[i + k], v = a[i + k + len / 2] * wk;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
            }
        }
    }
}

static const float kBandHz[9] = { 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000 };

struct Spectrum {
    float bands[9];   // Oktavband-Energie in dB
    float centroid;   // Hz
    float above10k;   // Energieanteil über 10 kHz in dB
};

// Spektrum eines Abschnitts (Hann-Fenster, höchstens 2^17 Samples ab `from`).
static Spectrum spectrum(const std::vector<float>& x, int sr, size_t from, size_t to)
{
    to = std::min(to, x.size());
    const size_t len = std::min<size_t>(to > from ? to - from : 0, 1 << 17);
    size_t n = 1024;
    while (n < len)
        n <<= 1;
    std::vector<std::complex<double>> a(n);
    for (size_t i = 0; i < len; ++i)
        a[i] = x[from + i] * (0.5 - 0.5 * std::cos(2.0 * M_PI * (double)i / (double)len));
    fft(a);
    Spectrum s {};
    double band[9] = {}, total = 0.0, high = 0.0, weighted = 0.0;
    for (size_t k = 1; k < n / 2; ++k) {
        const double f = (double)k * sr / n, e = std::norm(a[k]);
        total += e;
        weighted += e * f;
        if (f > 10000.0)
            high += e;
        for (int b = 0; b < 9; ++b)
            if (f >= kBandHz[b] / M_SQRT2 && f < kBandHz[b] * M_SQRT2)
                band[b] += e;
    }
    for (int b = 0; b < 9; ++b)
        s.bands[b] = (float)(10.0 * std::log10(band[b] + 1e-30));
    s.centroid = total > 0.0 ? (float)(weighted / total) : 0.0f;
    s.above10k = (float)(10.0 * std::log10(high / (total + 1e-30) + 1e-30));
    return s;
}

// Korrelation L/R aus dem Spektrum, nur Anteile unter `maxHz` (0 = alle).
// Gibt 2 zurück, wenn dort kaum Energie liegt (unter −40 dB des Gesamtsignals).
static float correlation(const std::vector<float>& l, const std::vector<float>& r, float maxHz, int sr)
{
    const size_t len = std::min<size_t>(l.size(), 1 << 20);
    size_t n = 1024;
    while (n < len)
        n <<= 1;
    std::vector<std::complex<double>> a(n), b(n);
    for (size_t i = 0; i < len; ++i)
        a[i] = l[i], b[i] = r[i];
    fft(a);
    fft(b);
    double lr = 0, ll = 0, rr = 0, total = 0;
    for (size_t k = 1; k < n / 2; ++k) {
        total += std::norm(a[k]) + std::norm(b[k]);
        if (maxHz > 0.0f && (double)k * sr / n >= maxHz)
            continue;
        lr += std::real(a[k] * std::conj(b[k]));
        ll += std::norm(a[k]);
        rr += std::norm(b[k]);
    }
    if (ll + rr < 1e-4 * total || ll <= 0.0 || rr <= 0.0)
        return 2.0f;
    return (float)(lr / std::sqrt(ll * rr));
}

static void usage()
{
    std::puts("offbeat-render [--preset NAME|NR] [--mode kick|bass|hat|stab|pad|riser] [--note F1|29[@ms]]... [--velocity 0-127]\n"
              "               [--bpm 160] [--length MS] [--tail MS] [--sr 48000] [--set symbol=wert]...\n"
              "               [--out datei.wav] [--check] [--list]");
}

int main(int argc, char** argv)
{
    int preset = -1, mode = -1, note = 29, velocity = 127, sr = 48000;
    std::vector<std::pair<int, float>> notes; // Note, Start in ms
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
    for (const auto& o : overrides)
        values[o.first] = o.second;

    Engine engine;
    engine.setSampleRate((float)sr);
    for (int i = 0; i < kParamCount; ++i)
        engine.setParam(i, values[i]);

    // Transport läuft ab Schlag 0. Jede Note startet zu ihrer Zeit (--note F1@100) und wird
    // nach --length losgelassen; Ereignisse fallen genau auf ihren Frame.
    struct Event { uint32_t frame; int note; bool on; };
    std::vector<Event> events;
    float endMs = 0.0f;
    for (const auto& nt : notes) {
        events.push_back({ (uint32_t)(nt.second * 0.001f * sr), nt.first, true });
        events.push_back({ (uint32_t)((nt.second + lengthMs) * 0.001f * sr), nt.first, false });
        endMs = std::max(endMs, nt.second + lengthMs);
    }
    std::stable_sort(events.begin(), events.end(), [](const Event& a, const Event& b) { return a.frame < b.frame; });
    const uint32_t total = (uint32_t)((endMs + tailMs) * 0.001f * sr);
    std::vector<float> l(total), r(total);
    const uint32_t block = 256;
    size_t next = 0;
    for (uint32_t pos = 0; pos < total;) {
        while (next < events.size() && events[next].frame <= pos) {
            const Event& e = events[next++];
            if (e.on)
                engine.noteOn(e.note, velocity);
            else
                engine.noteOff(e.note);
        }
        engine.setTransport(true, (double)pos / sr * bpm / 60.0, bpm);
        uint32_t n = total - pos < block ? total - pos : block;
        if (next < events.size() && events[next].frame < pos + n)
            n = events[next].frame - pos;
        engine.process(&l[pos], &r[pos], n);
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

    const int modeIdx = (int)values[kMode];
    const float lenMs = lengthTo(mono, sr, peak * 0.01f); // bis −40 dB
    std::printf("Mode %s, Preset %s, Note %d (%.2f Hz), %g BPM, %d Hz\n", kModeNames[modeIdx],
                preset >= 0 ? kPresets[preset].name : "(Defaults)", note, midiToHz((float)note), bpm, sr);
    std::printf("Spitze           %7.2f dBFS   RMS %7.2f dBFS\n", toDb(peak), toDb(std::sqrt(sumSq / total)));
    std::printf("Klingt bis       %7.1f ms (−40 dB: %.1f ms)\n", soundMs, lenMs);
    if (modeIdx == kModeKick || modeIdx == kModeBass) {
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
    if (modeIdx == kModeKick) {
        // Spec Abschnitt 5 (Kick – Hard Bounce, Note F1).
        const float p20 = pitchAt(mono, sr, 20), p97 = pitchAt(mono, sr, 97), p150 = pitchAt(mono, sr, 150);
        expect("Tonhöhe 20 ms: 140–160 Hz", p20 >= 140 && p20 <= 160);
        expect("Tonhöhe 97 ms: 55–65 Hz", p97 >= 55 && p97 <= 65);
        expect("Tonhöhe 150 ms: 45–52 Hz", p150 >= 45 && p150 <= 52);
        expect("Pegel 190 ms ≥ 30 dB unter Maximum", toDb(peakIn(mono, sr, 190, 191)) - toDb(peak) <= -30.0f);
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
