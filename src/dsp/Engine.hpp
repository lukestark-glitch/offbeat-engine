// Engine: Modus-Auswahl, Noten-Verwaltung, BPM-Raster, globale Kette
// (Drive → Tone → Width → Volume → Limiter). Wird vom Plugin und von offbeat-render genutzt.
#pragma once

#include "Bass.hpp"
#include "Hat.hpp"
#include "Kick.hpp"
#include "Poly.hpp"
#include "Riser.hpp"

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
    void startPoly(int key, float note, float velocity);
    PolySettings polySettings() const;

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
    HatVoice hats[2]; // wie bei der Kick: neuer Schlag (oder Closed-Choke) blendet den alten aus
    int hatIndex = 0;
    static constexpr int kPolyVoices = 8;
    PolyVoice poly[kPolyVoices];
    uint32_t polyAge = 0;
    int lastPolyNote = -1;
    Noise rnd;
    RiserVoice riser;
    Chorus chorus;

    Svf toneL, toneR;
    Svf widthHp[4]; // Hochpass 48 dB/Okt. aufs Seitensignal: unter 150 Hz bleibt alles mono
    Smoother volume;
};

} // namespace offbeat
