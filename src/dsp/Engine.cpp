#include "Engine.hpp"


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
    riser.setSampleRate(sr);
    volume.setTime(10.0f, sr);
    resetVoices();
}

void Engine::setParam(int id, float value)
{
    if (id < 0 || id >= kParamCount)
        return;
    const ParamInfo& info = kParams[id];
    value = clampf(value, info.min, info.max);
    if ((id == kMode || id == kOctave) && (int)value != (int)params[id])
        resetVoices(); // bei neuer Oktave passen gehaltene Noten nicht mehr zu ihrem Note-Off
    params[id] = value;
}

void Engine::resetVoices()
{
    for (KickVoice& k : kicks)
        k.active = false;
    bass.stop();
    for (HatVoice& h : hats)
        h.active = false;
    for (PolyVoice& v : poly)
        v = PolyVoice();
    riser.stop();
    chorus.reset();
    heldCount = 0;
    toneL.reset();
    toneR.reset();
    for (Svf& f : widthHp)
        f.reset();
}

// Tempo vom Host nur, wenn "Tempo from Host" an ist und der Host eines liefert; sonst läuft das
// Raster frei mit "Tempo BPM" (Fallback, falls LMMS/Carla kein Tempo weitergibt).
void Engine::setTransport(bool playing, double beatPosition, double hostBpm)
{
    if (params[kTempoHost] < 0.5f || hostBpm <= 0.0) {
        bpm = params[kTempoBpm];
        hostPlaying = false;
        return;
    }
    bpm = hostBpm;
    hostPlaying = playing;
    if (playing)
        beatPos = beatPosition;
}

void Engine::noteOn(int note, int velocity)
{
    note = transpose(note);
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
    case kModeHat:
        // Note mit Tonklasse A# (z. B. A#2, A#3) = Open, alle anderen = Closed (würgt Open ab).
        hats[hatIndex].release();
        hatIndex ^= 1;
        hats[hatIndex].trigger(params, note % 12 == 10, vel, sr);
        break;
    case kModeStab: {
        static const int kChords[3][3] = { { 0, 0, 0 }, { 0, 3, 7 }, { 0, 4, 7 } };
        const int chord = (int)params[kStabChord];
        for (int k = 0; k < (chord ? 3 : 1); ++k)
            startPoly(note, (float)(note + kChords[chord][k]), vel);
        lastPolyNote = note;
        break;
    }
    case kModePad:
        startPoly(note, (float)note, vel);
        break;
    case kModeRiser:
        riser.start(note, vel, beatPos);
        break;
    default:
        break;
    }
}

void Engine::startPoly(int key, float note, float velocity)
{
    // Freie Stimme, sonst die älteste losgelassene, sonst die älteste überhaupt.
    PolyVoice* best = nullptr;
    for (PolyVoice& v : poly) {
        auto rank = [](const PolyVoice& x) { return !x.active() ? 0 : (!x.held ? 1 : 2); };
        if (!best || rank(v) < rank(*best) || (rank(v) == rank(*best) && v.age < best->age))
            best = &v;
    }
    const float from = lastPolyNote >= 0 ? note + (float)(lastPolyNote - key) : note;
    best->age = ++polyAge;
    best->start(key, note, from, 0.35f * velocity, polySettings(), rnd, sr);
}

PolySettings Engine::polySettings() const
{
    const float* p = params;
    PolySettings s;
    if ((int)p[kMode] == kModePad) {
        s.voices = 7;
        s.detuneCents = 40.0f * p[kPadDetune];
        s.sideMix = 0.8f;
        s.triLevel = 0.5f * p[kPadTri];
        s.cutoff = p[kPadCutoff];
        s.reso = 0.1f;
        s.attack = p[kPadAttack];
        s.sustain = 1.0f;
        s.release = p[kPadRelease];
        return s;
    }
    s.voices = (int)p[kStabVoices];
    s.detuneCents = 50.0f * p[kStabDetune];
    s.sideMix = p[kStabMix];
    s.cutoff = p[kStabCutoff];
    s.reso = p[kStabReso];
    s.envAmt = p[kStabEnvAmt];
    s.filterDecay = p[kStabFiltDecay];
    s.attack = p[kStabAttack];
    s.decay = p[kStabDecay];
    s.sustain = p[kStabSustain];
    s.release = p[kStabRelease];
    s.hoover = p[kStabHoover];
    s.glide = p[kStabGlide];
    return s;
}

void Engine::noteOff(int note)
{
    note = transpose(note);
    int i = 0;
    while (i < heldCount && heldNotes[i] != note)
        ++i;
    if (i == heldCount)
        return;
    const bool wasLast = i == heldCount - 1;
    for (; i < heldCount - 1; ++i)
        heldNotes[i] = heldNotes[i + 1];
    --heldCount;

    const int mode = (int)params[kMode];
    if (mode == kModeStab || mode == kModePad) {
        for (PolyVoice& v : poly)
            if (v.held && v.key == note)
                v.release();
        return;
    }
    if (mode == kModeRiser && heldCount == 0)
        riser.release();
    if (mode != kModeBass || !wasLast)
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
    for (PolyVoice& v : poly)
        v.release();
    riser.release();
}

void Engine::process(float* outL, float* outR, uint32_t frames)
{
    const DenormalGuard denormals; // Flush-to-Zero für diesen Block (Intel und Apple Silicon)
    const int mode = (int)params[kMode];
    const float drive = params[kDrive];
    const float widthGain = 2.0f * params[kWidth];
    const float volTarget = std::pow(10.0f, params[kVolume] / 20.0f);
    toneL.set(params[kTone], sr, 0.29f);
    toneR.a1 = toneL.a1, toneR.a2 = toneL.a2, toneR.a3 = toneL.a3, toneR.k = toneL.k;
    for (Svf& f : widthHp)
        f.set(150.0f, sr, 0.29f);
    const double beatStep = bpm / 60.0 / sr;
    const bool roll = mode == kModeBass && params[kBassRoll] >= 0.5f;
    const PolySettings ps = polySettings();
    const float lfoBeats = kPadLfoBeats[(int)params[kPadLfoRate]];
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
        } else if (mode == kModeHat) {
            for (HatVoice& h : hats) {
                float hl, hr;
                h.process(hl, hr);
                l += hl, r += hr;
            }
        } else if (mode == kModeStab || mode == kModePad) {
            // Pad: langsamer, BPM-synchroner LFO auf den Cutoff (±2 Oktaven bei Depth 1).
            const float lfo = mode == kModePad
                ? std::exp2(2.0f * params[kPadLfoDepth] * std::sin(2.0f * kPi * (float)std::fmod(beatPos / lfoBeats, 1.0)))
                : 1.0f;
            for (PolyVoice& v : poly) {
                float vl, vr;
                v.process(ps, lfo, vl, vr);
                l += vl, r += vr;
            }
            if (mode == kModePad)
                chorus.process(l, r, params[kPadChorus], sr);
        } else if (mode == kModeRiser) {
            riser.process(params, beatPos, l, r);
        }

        l = saturate(l, drive);
        r = saturate(r, drive);
        toneL.process(l);
        toneR.process(r);
        l = toneL.lp;
        r = toneR.lp;

        // Width nur auf dem Seitensignal oberhalb 150 Hz: der Bass bleibt mono.
        const float mid = 0.5f * (l + r);
        float side = 0.5f * (l - r);
        for (Svf& f : widthHp) {
            f.process(side);
            side = f.hp;
        }
        side *= widthGain;

        const float v = volume.process(volTarget);
        outL[i] = softLimit(v * (mid + side));
        outR[i] = softLimit(v * (mid - side));
    }
}

} // namespace offbeat
