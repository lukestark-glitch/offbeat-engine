// Bass-Modus: zwei verstimmte Saw/Square-Oszillatoren (links/rechts), Sub-Sinus −1 Okt.,
// TPT-Low-Pass mit Pluck-Hüllkurve, Bounce-Pitch, Glide. Roll: Sub hält, Rest pulsiert in 16teln.
#pragma once

#include "Blocks.hpp"
#include "Params.hpp"

namespace offbeat {

class BassVoice {
public:
    void setSampleRate(float newSr)
    {
        sr = newSr;
        releaseCoef = tauCoef(5.0f, sr);
        subGate.setTime(3.0f, sr);
    }

    void noteOn(const float* p, int note, float velocity)
    {
        target = (float)note;
        if (!active || p[kBassGlide] <= 0.0f)
            current = target;
        gain = velocity;
        gate = true;
        active = true;
        bounceEnv = 1.0f;
        retrigger();
    }

    // Legato zurück auf eine noch gehaltene Note, ohne neuen Anschlag.
    void glideTo(int note) { target = (float)note; }

    void noteOff()
    {
        gate = false;
        stage = kRelease;
    }

    // Neuer Anschlag von Amp- und Filter-Hüllkurve (Note-On und Roll-16tel).
    void retrigger()
    {
        stage = kAttack;
        filterEnv = 1.0f;
    }

    void stop()
    {
        active = gate = false;
        amp = 0.0f;
        subGate.value = 0.0f;
        for (Svf& f : filters)
            f.reset();
    }

    void process(const float* p, float& outL, float& outR)
    {
        outL = outR = 0.0f;
        if (!active)
            return;

        const float glide = p[kBassGlide];
        if (glide > 0.0f)
            current = target + tauCoef(glide / 3.0f, sr) * (current - target);
        else
            current = target;

        const float semis = current + p[kBassBounce] * bounceEnv;
        bounceEnv *= tauCoef(p[kBassBounceTime] / 3.0f, sr); // ~95 % nach Bounce Time
        const float hz = midiToHz(semis);
        const float spread = std::exp2(p[kBassDetune] / 2400.0f); // ±Detune/2
        const float wave = p[kBassWave];
        const float o1 = osc1.process(hz / spread, sr, wave);
        const float o2 = osc2.process(hz * spread, sr, wave);
        const float sub = subOsc.process(hz * 0.5f, sr);

        // Amp-Hüllkurve: 1 ms Anstieg (vom aktuellen Pegel aus), Abfall auf 0, kurzer Release.
        switch (stage) {
        case kAttack:
            amp += 1.0f / (0.001f * sr);
            if (amp >= 1.0f) {
                amp = 1.0f;
                stage = kDecay;
            }
            break;
        case kDecay:
            amp *= decayCoef(p[kBassAmpDecay], sr);
            break;
        case kRelease:
            amp *= releaseCoef;
            break;
        }
        filterEnv *= decayCoef(p[kBassFiltDecay], sr);

        const float cutoff = p[kBassCutoff] * std::exp2(6.0f * p[kBassEnvAmt] * filterEnv);
        const bool steep = p[kBassSlope] >= 0.5f;
        float l = o1, r = o2;
        for (int i = 0; i < 2; ++i) {
            Svf& fl = filters[i];
            Svf& fr = filters[i + 2];
            fl.set(cutoff, sr, p[kBassReso]);
            fr.a1 = fl.a1, fr.a2 = fl.a2, fr.a3 = fl.a3, fr.k = fl.k;
            fl.process(l);
            fr.process(r);
            l = fl.lp;
            r = fr.lp;
            if (!steep)
                break;
        }

        // Roll: Sub läuft durch, solange die Taste gehalten wird; sonst folgt er der Amp-Hüllkurve.
        const bool roll = p[kBassRoll] >= 0.5f;
        const float subAmp = roll ? subGate.process(gate ? 1.0f : 0.0f) : amp;
        const float s = 1.2f * p[kBassSub] * subAmp * sub;
        outL = gain * (amp * l + s);
        outR = gain * (amp * r + s);

        if (amp < 1e-5f && subAmp < 1e-5f && stage != kAttack)
            stop();
    }

    bool active = false;
    bool gate = false;

private:
    enum Stage { kAttack, kDecay, kRelease };
    Osc osc1, osc2;
    Sine subOsc;
    Svf filters[4]; // [0],[1] links (12/24 dB), [2],[3] rechts
    Smoother subGate;
    Stage stage = kDecay;
    float sr = 48000.0f, releaseCoef = 0.0f;
    float target = 0, current = 0, gain = 1, amp = 0, filterEnv = 0, bounceEnv = 0;
};

} // namespace offbeat
