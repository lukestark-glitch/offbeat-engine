#include "Params.hpp"

namespace offbeat {

const char* const kModeNames[kModeCount] = { "Kick", "Bass", "Hat", "Stab", "Pad", "Riser" };

// clang-format off
const ParamInfo kParams[kParamCount] = {
    // name                symbol            unit    min     max      default  flags
    { "Mode",              "mode",           "",     0,      5,       0,       kFlagInt },
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

#define PRESET(name, arr) { name, arr, (int)(sizeof(arr) / sizeof(arr[0])) }
} // namespace

const Preset kPresets[] = {
    PRESET("Kick - Hard Bounce", kickHard),
    PRESET("Kick - Bounce Soft", kickSoft),
    PRESET("Bass - Offbeat F2", bassF2),
    PRESET("Bass - Rubber", bassRubber),
    PRESET("Bass - Late Bounce", bassLate),
    PRESET("Bass - Rolling Sub", bassRoll),
};
// clang-format on

const int kPresetCount = (int)(sizeof(kPresets) / sizeof(kPresets[0]));

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

} // namespace offbeat
