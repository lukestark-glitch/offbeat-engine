// Polyphone Supersaw-Stimme für Stab/Lead und Pad: 1–7 verstimmte Saws im Stereo verteilt,
// optional Dreieck +12, Low-Pass mit Hüllkurve, ADSR, Glide und Hoover-Drop beim Loslassen.
#pragma once

#include "Blocks.hpp"

namespace offbeat {

// Vom Modus abhängige Einstellungen; die Engine füllt sie aus den Stab- bzw. Pad-Parametern.
struct PolySettings {
    int voices = 7;
    float detuneCents = 15, sideMix = 0.7f, triLevel = 0;
    float cutoff = 3000, reso = 0.2f, envAmt = 0, filterDecay = 200;
    float attack = 1, decay = 250, sustain = 0.3f, release = 120;
    float hoover = 0, glide = 0;
};

class PolyVoice {
public:
    void start(int key, float note, float fromNote, float velocity, const PolySettings& s, Noise& rnd, float newSr)
    {
        sr = newSr;
        if (!amp.active()) {
            for (Osc& o : saws)
                o.phase = 0.5f + 0.5f * rnd.next(); // zufällige Phasen gegen Phasing
            fl.reset();
            fr.reset();
        }
        this->key = key;
        target = note;
        current = s.glide > 0.0f ? fromNote : note;
        hooverOffset = 0.0f;
        gain = velocity;
        held = true;
        count = s.voices < 1 ? 1 : (s.voices > 7 ? 7 : s.voices);
        float norm = 0.0f;
        for (int i = 0; i < count; ++i) {
            pos[i] = count == 1 ? 0.0f : -1.0f + 2.0f * (float)i / (float)(count - 1);
            level[i] = std::fabs(pos[i]) <= 1.0f / (float)(count > 1 ? count - 1 : 1) + 1e-4f ? 1.0f : s.sideMix;
            norm += level[i] * level[i];
        }
        norm = 1.0f / std::sqrt(norm > 0.0f ? norm : 1.0f);
        for (int i = 0; i < count; ++i) {
            panL[i] = level[i] * norm * std::sqrt(0.5f * (1.0f - 0.8f * pos[i]));
            panR[i] = level[i] * norm * std::sqrt(0.5f * (1.0f + 0.8f * pos[i]));
        }
        filterEnv = 1.0f;
        amp.gateOn();
    }

    void release()
    {
        held = false;
        amp.gateOff();
    }

    bool active() const { return amp.active(); }

    void process(const PolySettings& s, float cutoffMul, float& outL, float& outR)
    {
        outL = outR = 0.0f;
        if (!amp.active())
            return;
        if (s.glide > 0.0f)
            current = target + tauCoef(s.glide / 3.0f, sr) * (current - target);
        else
            current = target;
        if (!held && s.hoover > 0.0f)
            hooverOffset = -s.hoover + tauCoef(s.release / 3.0f, sr) * (hooverOffset + s.hoover);

        const float hz = midiToHz(current + hooverOffset);
        float l = 0.0f, r = 0.0f;
        for (int i = 0; i < count; ++i) {
            const float y = saws[i].process(hz * std::exp2(pos[i] * s.detuneCents / 1200.0f), sr, 0.0f);
            l += panL[i] * y;
            r += panR[i] * y;
        }
        if (s.triLevel > 0.0f) {
            const float t = s.triLevel * tri.process(2.0f * hz, sr);
            l += t;
            r += t;
        }

        filterEnv *= decayCoef(s.filterDecay, sr);
        fl.set(s.cutoff * cutoffMul * std::exp2(6.0f * s.envAmt * filterEnv), sr, s.reso);
        fr.a1 = fl.a1, fr.a2 = fl.a2, fr.a3 = fl.a3, fr.k = fl.k;
        fl.process(l);
        fr.process(r);

        const float a = gain * amp.process(s.attack, s.decay, s.sustain, s.release, sr);
        outL = a * fl.lp;
        outR = a * fr.lp;
    }

    int key = -1;
    bool held = false;
    uint32_t age = 0;

private:
    Osc saws[7];
    Tri tri;
    Svf fl, fr;
    Adsr amp;
    int count = 1;
    float pos[7] = {}, level[7] = {}, panL[7] = {}, panR[7] = {};
    float sr = 48000.0f, target = 0, current = 0, hooverOffset = 0, gain = 1, filterEnv = 0;
};

} // namespace offbeat
