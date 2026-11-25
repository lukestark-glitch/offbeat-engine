// Kick-Modus: Sinus mit zweistufiger Tonhöhenkurve, Sättigung, Klick, harter Abbruch.
// f(t) = f_end + A1·e^(−t/Punch) + A2·e^(−t/Body), A1:A2 = 200:60 (Messung, Spec Abschnitt 2).
#pragma once

#include "Blocks.hpp"
#include "Params.hpp"

namespace offbeat {

class KickVoice {
public:
    void trigger(const float* p, float endHz, float velocity, float sr, float bpm)
    {
        this->sr = sr;
        const float diff = p[kKickStart] - endHz > 0.0f ? p[kKickStart] - endHz : 0.0f;
        fEnd = endHz;
        a1 = diff * (200.0f / 260.0f);
        a2 = diff * (60.0f / 260.0f);
        e1 = e2 = 1.0f;
        d1 = tauCoef(p[kKickPunch], sr);
        d2 = tauCoef(p[kKickBody], sr);

        float lengthMs = p[kKickLength];
        if (p[kKickSync] >= 0.5f && bpm > 0.0f)
            lengthMs = 0.95f * 30000.0f / bpm; // 95 % einer Achtel
        length = (uint32_t)(lengthMs * 0.001f * sr);
        const float cutMs = p[kKickCut] < lengthMs ? p[kKickCut] : lengthMs;
        cutStart = length - (uint32_t)(cutMs * 0.001f * sr);

        drive = p[kKickDrive];
        click = p[kKickClick];
        gain = velocity;
        clickHp.set(900.0f, sr);
        clickLp.set(3500.0f, sr);
        clickHp.reset();
        clickLp.reset();
        osc.phase = 0.0f;
        n = 0;
        fadeOut = 1.0f;
        fadeStep = 0.0f;
        active = true;
    }

    // Weicher Abbruch (3 ms), wenn eine neue Kick die alte ablöst.
    void release() { fadeStep = 1.0f / (0.003f * sr); }

    float process()
    {
        if (!active)
            return 0.0f;
        const float t = (float)n / sr;
        const float hz = fEnd + a1 * e1 + a2 * e2;
        e1 *= d1;
        e2 *= d2;
        float y = saturate(osc.process(hz, sr), drive);

        // Hüllkurve: 1 ms Einblendung, Halten, Cosinus-Abbruch über "Cut" bis "Length".
        float amp = 1.0f;
        if (t < 0.001f)
            amp = 0.5f - 0.5f * std::cos(kPi * t / 0.001f);
        if (n >= cutStart)
            amp *= 0.5f + 0.5f * std::cos(kPi * (float)(n - cutStart) / (float)(length - cutStart + 1));
        y *= amp;

        // Klick: Rauschen 0,9–3,5 kHz, 0,5 ms Einblendung, ausgeklungen nach 8 ms.
        if (click > 0.0f && t < 0.008f) {
            float env = t < 0.0005f ? t / 0.0005f : std::exp(-(t - 0.0005f) / 0.0015f);
            if (t > 0.006f)
                env *= 0.5f + 0.5f * std::cos(kPi * (t - 0.006f) / 0.002f);
            clickHp.process(noise.next());
            clickLp.process(clickHp.hp);
            y += 2.0f * click * env * clickLp.lp;
        }

        if (fadeStep > 0.0f) {
            fadeOut -= fadeStep;
            if (fadeOut <= 0.0f)
                active = false;
        }
        if (++n >= length)
            active = false;
        return y * gain * (fadeOut > 0.0f ? fadeOut : 0.0f);
    }

    bool active = false;

private:
    Sine osc;
    Noise noise;
    Svf clickHp, clickLp;
    float sr = 48000.0f, fEnd = 43.65f, a1 = 0, a2 = 0, e1 = 0, e2 = 0, d1 = 0, d2 = 0;
    float drive = 0, click = 0, gain = 1, fadeOut = 1, fadeStep = 0;
    uint32_t n = 0, length = 0, cutStart = 0;
};

} // namespace offbeat
