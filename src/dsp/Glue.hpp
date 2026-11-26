// Bounce-Sektion „Dichte“ für Kick, Bass und Kick+Bass (nach der Kette aus dem Hard-Bounce-Erklärvideo,
// docs/BOUNCE_VIDEO.md): EQ (Low-Cut 30 Hz, +3 dB bei 100 Hz, −2 dB bei 1 kHz) → Glue-Kompressor
// → Sättigung → Clipper. Ein Regler: density 0 = aus (bitgleich durchgereicht), 1 = maximal dicht.
#pragma once

#include "Blocks.hpp"

#include <initializer_list>

namespace offbeat {

class BounceGlue {
public:
    void setSampleRate(float newSr)
    {
        sr = newSr;
        reset();
    }

    void reset()
    {
        for (Svf* f : { &lowCut[0], &lowCut[1], &bell100[0], &bell100[1], &bell1k[0], &bell1k[1] })
            f->reset();
        env = 0.0f;
    }

    // Einmal pro Block: Regler übernehmen.
    void set(float density)
    {
        amount = clampf(density, 0.0f, 1.0f);
        if (amount <= 0.0f)
            return;
        const float eq = std::fmin(1.0f, 2.0f * amount); // EQ ist ab Dichte 50 % voll da
        for (int c = 0; c < 2; ++c) {
            lowCut[c].set(30.0f, sr, 0.0f);
            bell100[c].set(100.0f, sr, 0.5f);
            bell1k[c].set(1000.0f, sr, 0.5f);
        }
        gain100 = std::pow(10.0f, 3.0f * eq / 20.0f) - 1.0f;
        gain1k = std::pow(10.0f, -2.0f * eq / 20.0f) - 1.0f;
        // Glue: Schwelle −6 … −18 dBFS, Ratio 1:1 … 3:1, halbe Absenkung bei 0 dBFS als Make-up
        threshold = -6.0f - 12.0f * amount;
        slope = 1.0f / (1.0f + 2.0f * amount) - 1.0f;
        makeup = std::pow(10.0f, 0.5f * slope * threshold / 20.0f);
        atk = tauCoef(5.0f, sr);
        rel = tauCoef(60.0f, sr);
        drive = 1.0f + 1.5f * amount;          // Sättigung
        clipStart = 1.0f - 0.3f * amount;      // Clipper: weiches Knie bis 0,99
    }

    void process(float& l, float& r)
    {
        if (amount <= 0.0f)
            return;
        float x[2] = { l, r };
        for (int c = 0; c < 2; ++c) {
            lowCut[c].process(x[c]);
            float y = lowCut[c].hp;
            bell100[c].process(y);
            y += gain100 * bell100[c].k * bell100[c].bp; // k·bp = Bandpass mit Verstärkung 1 in der Mitte
            bell1k[c].process(y);
            y += gain1k * bell1k[c].k * bell1k[c].bp;
            x[c] = y;
        }

        const float level = std::fmax(std::fabs(x[0]), std::fabs(x[1]));
        env = level + (level > env ? atk : rel) * (env - level);
        const float over = 20.0f * std::log10(env + 1e-9f) - threshold;
        float gr = 0.0f;
        if (over >= 3.0f)
            gr = slope * over;
        else if (over > -3.0f)
            gr = slope * (over + 3.0f) * (over + 3.0f) / 12.0f; // weiches Knie, 6 dB
        const float g = std::pow(10.0f, gr / 20.0f) * makeup;

        for (int c = 0; c < 2; ++c) {
            float y = std::tanh(drive * g * x[c]) / std::tanh(drive);
            const float a = std::fabs(y);
            if (a > clipStart) {
                const float range = 0.99f - clipStart;
                const float s = clipStart + range * std::tanh((a - clipStart) / range);
                y = y < 0.0f ? -s : s;
            }
            x[c] = y;
        }
        l = x[0];
        r = x[1];
    }

private:
    Svf lowCut[2], bell100[2], bell1k[2];
    float sr = 48000.0f, amount = 0.0f, env = 0.0f;
    float gain100 = 0, gain1k = 0, threshold = 0, slope = 0, makeup = 1, atk = 0, rel = 0, drive = 1, clipStart = 1;
};

} // namespace offbeat
