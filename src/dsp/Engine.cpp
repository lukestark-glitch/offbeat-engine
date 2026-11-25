#include "Engine.hpp"

#if defined(__SSE__) || defined(__x86_64__)
#include <xmmintrin.h>
#define OFFBEAT_HAVE_SSE 1
#endif

namespace offbeat {

Engine::Engine()
{
    applyDefaults(params);
    setSampleRate(48000.0f);
}

void Engine::setSampleRate(float newSr)
{
    sr = newSr;
    bass.setSampleRate(sr);
    volume.setTime(10.0f, sr);
    resetVoices();
}

void Engine::setParam(int id, float value)
{
    if (id < 0 || id >= kParamCount)
        return;
    const ParamInfo& info = kParams[id];
    value = clampf(value, info.min, info.max);
    if (id == kMode && (int)value != (int)params[kMode])
        resetVoices();
    params[id] = value;
}

void Engine::resetVoices()
{
    for (KickVoice& k : kicks)
        k.active = false;
    bass.stop();
    heldCount = 0;
    toneL.reset();
    toneR.reset();
    widthHp.reset();
}

void Engine::setTransport(bool playing, double beatPosition, double hostBpm)
{
    if (hostBpm > 0.0)
        bpm = hostBpm;
    hostPlaying = playing;
    if (playing)
        beatPos = beatPosition;
}

void Engine::noteOn(int note, int velocity)
{
    const float vel = clampf((float)velocity / 127.0f, 0.0f, 1.0f);
    if (heldCount < 16)
        heldNotes[heldCount++] = note;

    switch ((int)params[kMode]) {
    case kModeKick: {
        const float endHz = params[kKickTune] >= 0.5f ? midiToHz((float)note) : params[kKickEnd];
        kicks[kickIndex].release();
        kickIndex ^= 1;
        kicks[kickIndex].trigger(params, endHz, vel, sr, (float)bpm);
        break;
    }
    case kModeBass:
        if (!hostPlaying && heldCount == 1) {
            beatPos = 0.0; // freies 16tel-Raster startet mit der Note
            lastSixteenth = 0;
        }
        bass.noteOn(params, note, vel);
        break;
    default:
        break; // Hat, Stab, Pad, Riser: Phase 2
    }
}

void Engine::noteOff(int note)
{
    int i = 0;
    while (i < heldCount && heldNotes[i] != note)
        ++i;
    if (i == heldCount)
        return;
    const bool wasLast = i == heldCount - 1;
    for (; i < heldCount - 1; ++i)
        heldNotes[i] = heldNotes[i + 1];
    --heldCount;

    if ((int)params[kMode] != kModeBass || !wasLast)
        return;
    if (heldCount > 0)
        bass.glideTo(heldNotes[heldCount - 1]);
    else
        bass.noteOff();
}

void Engine::allNotesOff()
{
    heldCount = 0;
    bass.noteOff();
}

void Engine::process(float* outL, float* outR, uint32_t frames)
{
#ifdef OFFBEAT_HAVE_SSE
    const unsigned int oldCsr = _mm_getcsr();
    _mm_setcsr(oldCsr | 0x8040); // Flush-to-Zero + Denormals-are-Zero
#endif
    const int mode = (int)params[kMode];
    const float drive = params[kDrive];
    const float widthGain = 2.0f * params[kWidth];
    const float volTarget = std::pow(10.0f, params[kVolume] / 20.0f);
    toneL.set(params[kTone], sr, 0.29f);
    toneR.a1 = toneL.a1, toneR.a2 = toneL.a2, toneR.a3 = toneL.a3, toneR.k = toneL.k;
    widthHp.set(150.0f, sr, 0.29f);
    const double beatStep = bpm / 60.0 / sr;
    const bool roll = mode == kModeBass && params[kBassRoll] >= 0.5f;
    if (!started) {
        volume.value = volTarget; // kein Einblenden beim allerersten Block
        started = true;
    }

    for (uint32_t i = 0; i < frames; ++i) {
        // 16tel-Raster für Roll
        const int64_t sixteenth = (int64_t)std::floor(beatPos * 4.0);
        if (sixteenth != lastSixteenth) {
            lastSixteenth = sixteenth;
            if (roll && bass.gate)
                bass.retrigger();
        }
        beatPos += beatStep;

        float l = 0.0f, r = 0.0f;
        if (mode == kModeKick) {
            l = kicks[0].process() + kicks[1].process();
            r = l;
        } else if (mode == kModeBass) {
            bass.process(params, l, r);
        }

        l = saturate(l, drive);
        r = saturate(r, drive);
        toneL.process(l);
        toneR.process(r);
        l = toneL.lp;
        r = toneR.lp;

        // Width nur auf dem Seitensignal oberhalb 150 Hz: der Bass bleibt mono.
        const float mid = 0.5f * (l + r);
        widthHp.process(0.5f * (l - r));
        const float side = widthGain * widthHp.hp;

        const float v = volume.process(volTarget);
        outL[i] = softLimit(v * (mid + side));
        outR[i] = softLimit(v * (mid - side));
    }
#ifdef OFFBEAT_HAVE_SSE
    _mm_setcsr(oldCsr);
#endif
}

} // namespace offbeat
