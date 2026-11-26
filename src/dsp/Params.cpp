#include "Params.hpp"

#include <initializer_list>

namespace offbeat {

const char* const kModeNames[kModeCount] = { "Kick", "Bass", "Hat", "Stab", "Pad", "Riser", "Kick+Bass" };
static const char* const kChordNames[3] = { "Aus", "Moll", "Dur" };
static const char* const kPadLfoNames[5] = { "1/4", "1/2", "1 Takt", "2 Takte", "4 Takte" };
static const char* const kSirenNames[5] = { "1/1", "1/2", "1/4", "1/8", "1/16" };
const float kPadLfoBeats[5] = { 1, 2, 4, 8, 16 };
const float kSirenBeats[5] = { 4, 2, 1, 0.5f, 0.25f };

// clang-format off
const ParamInfo kParams[kParamCount] = {
    // name                symbol            unit    min     max      default  flags
    { "Mode",              "mode",           "",     0,      6,       0,       kFlagInt, kModeNames },
    { "Volume",            "volume",         "dB",   -40,    6,       -3,      0 },
    { "Drive",             "drive",          "",     0,      1,       0,       0 },
    { "Tone",              "tone",           "Hz",   200,    20000,   20000,   kFlagLog },
    { "Width",             "width",          "",     0,      1,       0.5f,    0 },

    { "Kick Start Pitch",  "kick_start",     "Hz",   100,    500,     280,     0 },
    { "Kick Tune to Note", "kick_tune",      "",     0,      1,       1,       kFlagBool },
    { "Kick End Pitch",    "kick_end",       "Hz",   30,     100,     43.65f,  0 },
    { "Kick Punch Time",   "kick_punch",     "ms",   5,      60,      18,      0 },
    { "Kick Body Time",    "kick_body",      "ms",   20,     200,     75,      0 },
    { "Kick Length",       "kick_length",    "ms",   60,     600,     185,     0 },
    { "Kick Cut",          "kick_cut",       "ms",   5,      60,      30,      0 },
    { "Kick Click",        "kick_click",     "",     0,      1,       0.2f,    0 },
    { "Kick Drive",        "kick_drive",     "",     0,      1,       0.5f,    0 },
    { "Kick Bounce Sync",  "kick_sync",      "",     0,      1,       0,       kFlagBool },

    { "Bass Wave",         "bass_wave",      "",     0,      1,       0,       0 },
    { "Bass Sub",          "bass_sub",       "",     0,      1,       0.5f,    0 },
    { "Bass Detune",       "bass_detune",    "ct",   0,      50,      8,       0 },
    { "Bass Cutoff",       "bass_cutoff",    "Hz",   40,     10000,   500,     kFlagLog },
    { "Bass Resonance",    "bass_reso",      "",     0,      1,       0.25f,   0 },
    { "Bass 24 dB",        "bass_slope",     "",     0,      1,       0,       kFlagBool },
    { "Bass Filter Env",   "bass_envamt",    "",     0,      1,       0.5f,    0 },
    { "Bass Filter Decay", "bass_fdecay",    "ms",   5,      1000,    90,      kFlagLog },
    { "Bass Amp Decay",    "bass_adecay",    "ms",   20,     2000,    170,     kFlagLog },
    { "Bass Bounce Pitch", "bass_bounce",    "st",   0,      24,      7,       0 },
    { "Bass Bounce Time",  "bass_btime",     "ms",   5,      60,      20,      0 },
    { "Bass Glide",        "bass_glide",     "ms",   0,      200,     0,       0 },
    { "Bass Roll",         "bass_roll",      "",     0,      1,       0,       kFlagBool },

    { "Hat Metal/Noise",   "hat_mix",        "",     0,      1,       0.45f,   0 },
    { "Hat Pitch",         "hat_pitch",      "x",    0.5f,   2,       1,       0 },
    { "Hat High-Pass",     "hat_hp",         "Hz",   2000,   8000,    6000,    kFlagLog },
    { "Hat Low-Pass",      "hat_lp",         "Hz",   6000,   14000,   7500,    kFlagLog },
    { "Hat Decay Closed",  "hat_closed",     "ms",   3,      50,      12,      0 },
    { "Hat Decay Open",    "hat_open",       "ms",   20,     300,     70,      0 },

    { "Stab Voices",       "stab_voices",    "",     1,      7,       7,       kFlagInt },
    { "Stab Detune",       "stab_detune",    "",     0,      1,       0.3f,    0 },
    { "Stab Mix",          "stab_mix",       "",     0,      1,       0.7f,    0 },
    { "Stab Cutoff",       "stab_cutoff",    "Hz",   100,    16000,   3000,    kFlagLog },
    { "Stab Resonance",    "stab_reso",      "",     0,      1,       0.2f,    0 },
    { "Stab Filter Env",   "stab_envamt",    "",     0,      1,       0.4f,    0 },
    { "Stab Filter Decay", "stab_fdecay",    "ms",   5,      2000,    200,     kFlagLog },
    { "Stab Attack",       "stab_attack",    "ms",   0.5f,   2000,    1,       kFlagLog },
    { "Stab Decay",        "stab_decay",     "ms",   5,      3000,    250,     kFlagLog },
    { "Stab Sustain",      "stab_sustain",   "",     0,      1,       0.3f,    0 },
    { "Stab Release",      "stab_release",   "ms",   5,      5000,    120,     kFlagLog },
    { "Stab Hoover Drop",  "stab_hoover",    "st",   0,      24,      0,       0 },
    { "Stab Chord",        "stab_chord",     "",     0,      2,       0,       kFlagInt, kChordNames },
    { "Stab Glide",        "stab_glide",     "ms",   0,      500,     0,       0 },

    { "Pad Detune",        "pad_detune",     "",     0,      1,       0.25f,   0 },
    { "Pad Triangle +12",  "pad_tri",        "",     0,      1,       0.4f,    0 },
    { "Pad Cutoff",        "pad_cutoff",     "Hz",   100,    12000,   1800,    kFlagLog },
    { "Pad LFO Rate",      "pad_lfo_rate",   "",     0,      4,       3,       kFlagInt, kPadLfoNames },
    { "Pad LFO Depth",     "pad_lfo_depth",  "",     0,      1,       0.4f,    0 },
    { "Pad Attack",        "pad_attack",     "ms",   5,      5000,    600,     kFlagLog },
    { "Pad Release",       "pad_release",    "ms",   20,     8000,    2000,    kFlagLog },
    { "Pad Chorus",        "pad_chorus",     "",     0,      1,       0.6f,    0 },

    { "Riser Bars",        "riser_bars",     "",     1,      32,      8,       kFlagInt },
    { "Riser Noise/Saw",   "riser_mix",      "",     0,      1,       0.4f,    0 },
    { "Riser Cutoff Start","riser_start",    "Hz",   100,    20000,   300,     kFlagLog },
    { "Riser Cutoff End",  "riser_end",      "Hz",   100,    20000,   12000,   kFlagLog },
    { "Riser Pitch Rise",  "riser_pitch",    "st",   0,      24,      12,      0 },
    { "Riser Volume Rise", "riser_vol",      "",     0,      1,       1,       0 },
    { "Riser Siren Depth", "riser_siren",    "st",   0,      12,      0,       0 },
    { "Riser Siren Rate",  "riser_rate",     "",     0,      4,       2,       kFlagInt, kSirenNames },

    { "Kick Attack",       "kick_attack",    "ms",   0,      30,      0,       0 }, // 0: keine Rundung am Anfang (Bounce)
    { "Octave",            "octave",         "",     -2,     2,       1,       kFlagInt }, // +1: gleicht den LMMS-Versatz aus
    { "Tempo from Host",   "tempo_host",     "",     0,      1,       1,       kFlagBool },
    { "Tempo BPM",         "tempo_bpm",      "BPM",  60,     220,     160,     0 },
    { "Combo Bass Note",   "combo_offset",   "st",   -24,    24,      12,      kFlagInt },
    { "Combo Bass Level",  "combo_level",    "dB",   -12,    12,      0,       0 },
    { "Combo Bass Bounce", "combo_bounce",   "st",   0,      36,      24,      0 },
    { "Combo Bounce Time", "combo_btime",    "ms",   5,      60,      12,      0 },
    { "Dichte",            "density",        "",     0,      1,       0,       0 },
};

namespace {
using V = Preset::Value;

const V kickHard[]  = { { kMode, kModeKick }, { kKickBody, 70 }, { kKickDrive, 0.85f } }; // Tonhöhe mittig in der Abnahme
const V kickSoft[]  = { { kMode, kModeKick }, { kKickDrive, 0.6f }, { kKickClick, 0.08f },
                        { kKickStart, 240 }, { kKickCut, 40 } };
const V bassF2[]    = { { kMode, kModeBass } };
const V bassRubber[]= { { kMode, kModeBass }, { kBassBounce, 14 }, { kBassBounceTime, 35 },
                        { kBassGlide, 60 }, { kBassAmpDecay, 260 }, { kBassWave, 0.3f } };
const V bassLate[]  = { { kMode, kModeBass }, { kBassAmpDecay, 90 }, { kBassFiltDecay, 45 },
                        { kBassBounceTime, 12 }, { kBassEnvAmt, 0.6f } };
const V bassRoll[]  = { { kMode, kModeBass }, { kBassRoll, 1 }, { kBassCutoff, 220 },
                        { kBassEnvAmt, 0.35f }, { kBassFiltDecay, 60 }, { kBassAmpDecay, 110 },
                        { kBassSub, 0.8f }, { kBassBounce, 0 } };
const V hatClosed[] = { { kMode, kModeHat } };
const V hatOpen[]   = { { kMode, kModeHat }, { kHatLowPass, 7000 }, { kHatMix, 0.5f } };
const V stabRave[]  = { { kMode, kModeStab }, { kStabDetune, 0.35f }, { kStabSustain, 0 },
                        { kStabDecay, 180 }, { kStabRelease, 80 }, { kStabCutoff, 2500 },
                        { kStabEnvAmt, 0.5f }, { kStabFiltDecay, 150 }, { kStabChord, 1 } };
const V leadFlow[]  = { { kMode, kModeStab }, { kStabVoices, 5 }, { kStabDetune, 0.2f },
                        { kStabGlide, 80 }, { kStabAttack, 5 }, { kStabDecay, 400 },
                        { kStabSustain, 0.7f }, { kStabRelease, 450 }, { kStabCutoff, 4000 },
                        { kStabEnvAmt, 0.25f } };
const V padMilk[]   = { { kMode, kModePad }, { kTone, 8000 }, { kWidth, 0.8f } };
const V riser8[]    = { { kMode, kModeRiser } };
const V siren16[]   = { { kMode, kModeRiser }, { kRiserBars, 16 }, { kRiserMix, 0.9f },
                        { kRiserStart, 1500 }, { kRiserEnd, 8000 }, { kRiserPitch, 7 },
                        { kRiserSiren, 5 }, { kRiserSirenRate, 2 }, { kRiserVolume, 0.6f } };

const V kickBounce160[] = { { kMode, kModeKick }, { kVolume, -6.8f }, { kDrive, 0.05f }, { kTone, 16000 },
                            { kKickStart, 300 }, { kKickTune, 0 }, { kKickEnd, 43.65f }, { kKickPunch, 38 },
                            { kKickBody, 50 }, { kKickLength, 180 }, { kKickCut, 30 }, { kKickAttack, 0 },
                            { kKickClick, 0.1f }, { kKickDrive, 0.7f } };
const V comboBounce160[] = { { kMode, kModeCombo }, { kVolume, 0 }, { kDrive, 0.05f }, { kTone, 12000 },
                             { kKickStart, 300 }, { kKickTune, 0 }, { kKickEnd, 43.65f }, { kKickPunch, 38 },
                             { kKickBody, 50 }, { kKickLength, 180 }, { kKickCut, 30 }, { kKickAttack, 0 },
                             { kKickClick, 0.1f }, { kKickDrive, 0.7f },
                             { kBassWave, 0.3f }, { kBassSub, 0.8f }, { kBassCutoff, 700 }, { kBassReso, 0.2f },
                             { kBassEnvAmt, 0.6f }, { kBassFiltDecay, 80 }, { kBassAmpDecay, 400 },
                             { kComboBounce, 24 }, { kComboBounceTime, 12 } };

#define PRESET(name, arr) { name, arr, (int)(sizeof(arr) / sizeof(arr[0])) }
} // namespace

const Preset kPresets[] = {
    PRESET("Kick - Hard Bounce", kickHard),
    PRESET("Kick - Bounce Soft", kickSoft),
    PRESET("Bass - Offbeat F2", bassF2),
    PRESET("Bass - Rubber", bassRubber),
    PRESET("Bass - Late Bounce", bassLate),
    PRESET("Bass - Rolling Sub", bassRoll),
    PRESET("Hat - Closed Soft", hatClosed),
    PRESET("Hat - Open Soft", hatOpen),
    PRESET("Stab - Rave", stabRave),
    PRESET("Lead - Flow", leadFlow),
    PRESET("Pad - Milchglas", padMilk),
    PRESET("Riser - 8 Takte", riser8),
    PRESET("Siren - 16 Takte", siren16),
    PRESET("Kick - Bounce 160", kickBounce160),
    PRESET("Kick+Bass - Bounce 160", comboBounce160),
};
// clang-format on

const int kPresetCount = (int)(sizeof(kPresets) / sizeof(kPresets[0]));

int modeParams(int mode, int out[16])
{
    // Erster Parameter jedes Modus; der Modus reicht bis zum ersten des nächsten.
    int n = 0;
    if (mode == kModeCombo) { // Kick und Bass stellt man in ihren Reitern ein; hier nur das Zusammenspiel
        for (int p : { kComboOffset, kComboLevel, kComboBounce, kComboBounceTime, kDensity, kKickLength,
                       kKickDrive, kBassAmpDecay, kBassCutoff, kBassSub })
            out[n++] = p;
        return n;
    }
    static const int first[kModeCombo + 1] = { kKickStart, kBassWave, kHatMix, kStabVoices, kPadDetune, kRiserBars, kRiserSirenRate + 1 };
    for (int p = first[mode]; p < first[mode + 1]; ++p) {
        out[n++] = p;
        if (p == kKickCut) // nachträglich angehängt, gehört hinter "Cut"
            out[n++] = kKickAttack;
    }
    if (mode == kModeKick || mode == kModeBass)
        out[n++] = kDensity;
    return n;
}

void applyDefaults(float out[kParamCount])
{
    for (int i = 0; i < kParamCount; ++i)
        out[i] = kParams[i].def;
}

void applyPreset(int index, float out[kParamCount])
{
    applyDefaults(out);
    if (index < 0 || index >= kPresetCount)
        return;
    const Preset& p = kPresets[index];
    for (int i = 0; i < p.count; ++i)
        out[p.values[i].id] = p.values[i].value;
}

int jsonParams(const float values[kParamCount], int* out)
{
    // Modus, globale Regler, Oktave/Tempo und die Regler des aktiven Modus
    const int head[] = { kMode, kVolume, kDrive, kTone, kWidth, kOctave, kTempoHost, kTempoBpm };
    int n = 0;
    for (int p : head)
        out[n++] = p;
    const int mode = (int)values[kMode];
    if (mode == kModeCombo) { // vollständiger Klang: alle Kick-, Bass- und Combo-Regler
        n += modeParams(kModeKick, out + n);
        n += modeParams(kModeBass, out + n) - 1; // Dichte steht schon bei Kick
        for (int p : { kComboOffset, kComboLevel, kComboBounce, kComboBounceTime })
            out[n++] = p;
        return n;
    }
    return n + modeParams(mode < 0 || mode >= kModeCount ? 0 : mode, out + n);
}

} // namespace offbeat
