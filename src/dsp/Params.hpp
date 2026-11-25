// Parameter-Tabelle und Werkspresets: einzige Quelle für Plugin und Renderer.
// Neue Parameter nur hinten anhängen, sonst passen gespeicherte Projekte nicht mehr.
#pragma once

#include <cstdint>

namespace offbeat {

enum Mode { kModeKick, kModeBass, kModeHat, kModeStab, kModePad, kModeRiser, kModeCount };

enum ParamId {
    kMode,
    kVolume,
    kDrive,
    kTone,
    kWidth,

    kKickStart,
    kKickTune,
    kKickEnd,
    kKickPunch,
    kKickBody,
    kKickLength,
    kKickCut,
    kKickClick,
    kKickDrive,
    kKickSync,

    kBassWave,
    kBassSub,
    kBassDetune,
    kBassCutoff,
    kBassReso,
    kBassSlope,
    kBassEnvAmt,
    kBassFiltDecay,
    kBassAmpDecay,
    kBassBounce,
    kBassBounceTime,
    kBassGlide,
    kBassRoll,

    kHatMix,
    kHatPitch,
    kHatHighPass,
    kHatLowPass,
    kHatClosed,
    kHatOpen,

    kStabVoices,
    kStabDetune,
    kStabMix,
    kStabCutoff,
    kStabReso,
    kStabEnvAmt,
    kStabFiltDecay,
    kStabAttack,
    kStabDecay,
    kStabSustain,
    kStabRelease,
    kStabHoover,
    kStabChord,
    kStabGlide,

    kPadDetune,
    kPadTri,
    kPadCutoff,
    kPadLfoRate,
    kPadLfoDepth,
    kPadAttack,
    kPadRelease,
    kPadChorus,

    kRiserBars,
    kRiserMix,
    kRiserStart,
    kRiserEnd,
    kRiserPitch,
    kRiserVolume,
    kRiserSiren,
    kRiserSirenRate,

    kKickAttack, // nachträglich angehängt, damit bestehende Parameter-Nummern gleich bleiben

    kParamCount
};

enum ParamFlags : uint32_t { kFlagLog = 1, kFlagBool = 2, kFlagInt = 4 };

struct ParamInfo {
    const char* name;
    const char* symbol;
    const char* unit;
    float min, max, def;
    uint32_t flags;
    const char* const* labels = nullptr; // Namen für ganzzahlige Auswahl-Parameter, sonst nullptr
};

extern const ParamInfo kParams[kParamCount];
extern const char* const kModeNames[kModeCount];

// Notenwerte in Schlägen für Pad-LFO und Sirene (Index = Parameterwert).
extern const float kPadLfoBeats[5];
extern const float kSirenBeats[5];

struct Preset {
    const char* name;
    struct Value { ParamId id; float value; };
    const Value* values;
    int count;
};

extern const Preset kPresets[];
extern const int kPresetCount;

// Schreibt alle Parameter-Werte des Presets (Defaults + Abweichungen) nach `out`.
void applyPreset(int index, float out[kParamCount]);
void applyDefaults(float out[kParamCount]);

} // namespace offbeat
