#include "Preview.hpp"

#include <cmath>
#include <memory>

namespace offbeat {

PreviewSpec previewSpec(int mode)
{
    switch (mode) {
    case kModeKick: return { 1, { "Kick" } };
    case kModeBass: return { 1, { "Bass" } };
    case kModeHat: return { 2, { "Closed", "Open" } };
    case kModeStab: return { 1, { "Stab" } };
    case kModePad: return { 1, { "Pad" } };
    case kModeRiser: return { 1, { "Riser" } };
    case kModeCombo: return { 2, { "Kick", "Bass" } };
    default: return { 1, { "Ton" } };
    }
}

void previewMute(int mode, int layer, bool& muteKick, bool& muteBass)
{
    muteKick = mode == kModeCombo && layer == 1;
    muteBass = mode == kModeCombo && layer == 0;
}

int previewEvents(const float* p, int layer, double beats, double bpm, int sr, NoteEvent* out, int max)
{
    const int mode = (int)p[kMode];
    const int shift = 12 * (int)p[kOctave]; // gedachte Taste → MIDI (wie der Host sie schicken müsste)
    const double beatSamples = 60.0 / bpm * sr;
    int n = 0;
    auto note = [&](double atBeat, double lenBeats, int key) {
        if (n + 2 > max)
            return;
        const int midi = key - shift < 0 ? 0 : (key - shift > 127 ? 127 : key - shift);
        out[n++] = { (uint32_t)std::lround(atBeat * beatSamples), midi, 110, true };
        out[n++] = { (uint32_t)std::lround((atBeat + lenBeats) * beatSamples), midi, 110, false };
    };
    const int count = (int)std::lround(beats); // 1 Schlag oder 4 Schläge (1 Takt)
    for (int b = 0; b < count; ++b) {
        switch (mode) {
        case kModeKick:
        case kModeCombo:
            note(b, 0.4, 29); // F1, der Combo-Bass kommt von selbst eine Achtel später
            break;
        case kModeBass:
            note(b + 0.5, 0.4, 41); // F2 auf dem Offbeat
            break;
        case kModeHat:
            if (layer != 1)
                note(b + 0.25, 0.1, 54), note(b + 0.75, 0.1, 54); // Closed (F#3)
            if (layer != 0)
                note(b + 0.5, 0.2, 58); // Open (A#3)
            break;
        case kModeStab:
            note(b + 0.5, 0.25, 65); // F4 auf dem Offbeat
            break;
        default:
            break;
        }
    }
    if (mode == kModePad)
        note(0, beats - 0.05, 53); // F3 über den ganzen Ausschnitt
    else if (mode == kModeRiser)
        note(0, beats - 0.05, 41);
    return n;
}

void renderPreview(const float* params, int layer, double beats, double bpm, int sr, std::vector<float>& l, std::vector<float>& r)
{
    std::unique_ptr<Engine> engine(new Engine()); // groß (Chorus-Puffer): nicht auf den Stack
    engine->setSampleRate((float)sr);
    for (int i = 0; i < kParamCount; ++i)
        if (!(kParams[i].flags & (kFlagHidden | kFlagOutput)))
            engine->setParam(i, params[i]);
    // Tempo wie im Plugin: Host-Tempo bzw. BPM-Regler (setTransport entscheidet)
    bool muteKick, muteBass;
    previewMute((int)params[kMode], layer, muteKick, muteBass);
    engine->setPreviewMute(muteKick, muteBass);
    NoteEvent ev[64];
    const int n = previewEvents(params, layer, beats, bpm, sr, ev, 64);
    const uint32_t total = (uint32_t)std::lround(beats * 60.0 / bpm * sr);
    l.assign(total, 0.0f);
    r.assign(total, 0.0f);
    renderEvents(*engine, std::vector<NoteEvent>(ev, ev + n), bpm, sr, l.data(), r.data(), total);
}

} // namespace offbeat
