// Riser-Modus: Rauschen ↔ Saw durch einen Low-Pass-Sweep, Tonhöhe und Lautstärke steigen über
// "Bars" Takte (4/4, synchron zum Schlag-Takt der Engine). Sirene: Sinus-LFO auf die Tonhöhe.
#pragma once

#include "Blocks.hpp"
#include "Params.hpp"

namespace offbeat {

class RiserVoice {
public:
    void setSampleRate(float newSr)
    {
        sr = newSr;
        gate.setTime(15.0f, sr); // weicher Ein-/Ausstieg
    }

    void start(int note, float velocity, double beat)
    {
        this->note = (float)note;
        gain = velocity;
        startBeat = beat;
        held = true;
        if (!active) {
            fl.reset();
            fr.reset();
            gate.value = 0.0f;
        }
        active = true;
    }

    void release() { held = false; }

    void stop()
    {
        active = held = false;
        gate.value = 0.0f;
    }

    void process(const float* p, double beat, float& outL, float& outR)
    {
        outL = outR = 0.0f;
        if (!active)
            return;
        const double elapsed = beat - startBeat;
        const float progress = clampf((float)(elapsed / (p[kRiserBars] * 4.0)), 0.0f, 1.0f);
        const float sirenBeats = kSirenBeats[(int)p[kRiserSirenRate]];
        const float siren = p[kRiserSiren] * std::sin(2.0f * kPi * (float)(elapsed / sirenBeats));
        const float hz = midiToHz(note + p[kRiserPitch] * progress + siren);
        const float saw = saw1.process(hz, sr, 0.0f);

        const float cutoff = p[kRiserStart] * std::pow(p[kRiserEnd] / p[kRiserStart], progress);
        fl.set(cutoff, sr, 0.35f);
        fr.a1 = fl.a1, fr.a2 = fl.a2, fr.a3 = fl.a3, fr.k = fl.k;
        const float mix = p[kRiserMix];
        fl.process(saw + (1.0f - mix) * (noiseL.next() - saw));
        fr.process(saw + (1.0f - mix) * (noiseR.next() - saw));

        const float depth = p[kRiserVolume];
        const float swell = (1.0f - depth) + depth * progress * progress;
        const float g = gate.process(held ? 1.0f : 0.0f);
        const float a = 0.6f * gain * g * swell;
        outL = a * fl.lp;
        outR = a * fr.lp;
        if (!held && g < 1e-4f)
            stop();
    }

    bool active = false;

private:
    Osc saw1;
    Noise noiseL { 0x2545f491u }, noiseR { 0x6c8e9cf5u };
    Svf fl, fr;
    Smoother gate;
    double startBeat = 0.0;
    float sr = 48000.0f, note = 60, gain = 1;
    bool held = false;
};

} // namespace offbeat
