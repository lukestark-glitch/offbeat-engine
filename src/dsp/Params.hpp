// Parameter-Tabelle und Werkspresets: einzige Quelle für Plugin und Renderer.
// Neue Parameter nur hinten anhängen, sonst passen gespeicherte Projekte nicht mehr.
#pragma once

#include "ParamInfo.hpp"

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
    kOctave,     // LMMS schickt Noten 12 Halbtöne tiefer als angezeigt
    kTempoHost,  // An: Tempo und Takt vom Host; Aus oder ohne Host-Tempo: Tempo BPM
    kTempoBpm,

    kParamCount
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

// Parameter eines Modus in Anzeige-Reihenfolge; Rückgabe: Anzahl (höchstens 16).
int modeParams(int mode, int out[16]);

// Parameter, die KOPIEREN (und topline/offbeat-render --preset-json) als JSON ausgibt.
int jsonParams(const float values[kParamCount], int* out);

} // namespace offbeat
