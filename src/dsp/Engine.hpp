// Engine: Modus-Auswahl, Noten-Verwaltung, BPM-Raster, globale Kette
// (Drive → Tone → Width → Volume → Limiter). Wird vom Plugin und von offbeat-render genutzt.
#pragma once

#include "Bass.hpp"
#include "Kick.hpp"

namespace offbeat {

class Engine {
public:
    Engine();

    void setSampleRate(float sr);
    void setParam(int id, float value);
    float getParam(int id) const { return params[id]; }

    void noteOn(int note, int velocity);
    void noteOff(int note);
    void allNotesOff();

    // Einmal pro Block: Host-Transport. Ohne Transport läuft das 16tel-Raster frei.
    void setTransport(bool playing, double beatPosition, double bpm);

    void process(float* outL, float* outR, uint32_t frames);

private:
    void resetVoices();

    float params[kParamCount];
    float sr = 48000.0f;
    double beatPos = 0.0, bpm = 160.0;
    bool hostPlaying = false, started = false;
    int64_t lastSixteenth = -1;

    int heldNotes[16];
    int heldCount = 0;

    KickVoice kicks[2]; // abwechselnd, damit eine neue Kick die alte weich ablösen kann
    int kickIndex = 0;
    BassVoice bass;

    Svf toneL, toneR, widthHp;
    Smoother volume;
};

} // namespace offbeat
