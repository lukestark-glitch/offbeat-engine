// Hat-Modus: 6 inharmonische Rechtecke (Drum-Machine-Prinzip) und Rauschen, High-Pass 12 dB,
// Low-Pass 24 dB (wenig Energie über 10 kHz). Länge: Closed 5 τ, Open ≈ 3,15 τ (Spec: 60 / 220 ms).
#pragma once

#include "Blocks.hpp"
#include "Params.hpp"

namespace offbeat {

class HatVoice {
public:
    void trigger(const float* p, bool open, float velocity, float sr)
    {
        this->sr = sr;
        const float tauMs = open ? p[kHatOpen] : p[kHatClosed];
        decay = tauCoef(tauMs, sr);
        length = (uint32_t)(tauMs * (open ? 3.15f : 5.0f) * 0.001f * sr);
        fadeStart = length - length / 10;
        mix = p[kHatMix];
        pitch = p[kHatPitch];
        gain = velocity;
        for (int c = 0; c < 2; ++c) {
            hp[c].set(p[kHatHighPass], sr);
            lp1[c].set(p[kHatLowPass], sr);
            lp2[c].set(p[kHatLowPass], sr);
            hp[c].reset(), lp1[c].reset(), lp2[c].reset();
        }
        env = 1.0f;
        n = 0;
        fadeOut = 1.0f;
        fadeStep = 0.0f;
        active = true;
    }

    // Abwürgen (Choke) bzw. Ablösen durch einen neuen Schlag: 2 ms Ausblendung.
    void release() { fadeStep = 1.0f / (0.002f * sr); }

    void process(float& outL, float& outR)
    {
        outL = outR = 0.0f;
        if (!active)
            return;
        static const float kRatios[6] = { 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f };
        float metal = 0.0f;
        for (int i = 0; i < 6; ++i)
            metal += osc[i].process(kRatios[i] * pitch, sr, 1.0f);
        metal *= 1.0f / 6.0f;

        float amp = env * (n < (uint32_t)(0.0003f * sr) ? (float)n / (0.0003f * sr) : 1.0f);
        if (n >= fadeStart)
            amp *= 0.5f + 0.5f * std::cos(kPi * (float)(n - fadeStart) / (float)(length - fadeStart + 1));
        env *= decay;

        float out[2];
        for (int c = 0; c < 2; ++c) {
            const float x = metal + mix * (noise[c].next() - metal); // Rauschen links/rechts verschieden
            hp[c].process(x);
            lp1[c].process(hp[c].hp);
            lp2[c].process(lp1[c].lp);
            out[c] = 5.0f * gain * amp * fadeOut * lp2[c].lp;
        }
        outL = out[0];
        outR = out[1];

        if (fadeStep > 0.0f && (fadeOut -= fadeStep) <= 0.0f)
            active = false;
        if (++n >= length)
            active = false;
    }

    bool active = false;

private:
    Osc osc[6];
    Noise noise[2] = { { 0x9e3779b9u }, { 0x7f4a7c15u } };
    Svf hp[2], lp1[2], lp2[2];
    float sr = 48000.0f, decay = 0, env = 0, mix = 0, pitch = 1, gain = 1, fadeOut = 1, fadeStep = 0;
    uint32_t n = 0, length = 0, fadeStart = 0;
};

} // namespace offbeat
