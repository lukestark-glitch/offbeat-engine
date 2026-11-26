// Vorschau der Engine: welche Ebenen ein Modus hat und welche Noten sie spielen. Genutzt von der
// Oberfläche (Wellenform offline rendern), vom DSP (Vorhören über den Plugin-Ausgang) und von
// offbeat-render (--preview, Abgleich). Tasten wie im Renderer: gedachte Taste, Octave eingerechnet.
#pragma once

#include "Engine.hpp"
#include "Render.hpp"

namespace offbeat {

struct PreviewSpec {
    int layers;              // 1–3
    const char* names[3];    // Beschriftung der Vorhör-Knöpfe
};

PreviewSpec previewSpec(int mode);

// Noten einer Ebene (layer 0..layers-1, oder < 0 = alle) für einen Ausschnitt von beats Schlägen.
// Schreibt höchstens max Ereignisse nach out (ohne Speicher anzufordern, für den Audio-Thread).
int previewEvents(const float* params, int layer, double beats, double bpm, int sr, NoteEvent* out, int max);

// Ebenen, die sich einen Klang teilen (Kick+Bass): welcher Teil stumm ist.
void previewMute(int mode, int layer, bool& muteKick, bool& muteBass);

// Ebene offline rendern, genau wie offbeat-render (gleiche Engine, gleiche Blöcke).
void renderPreview(const float* params, int layer, double beats, double bpm, int sr, std::vector<float>& l, std::vector<float>& r);

} // namespace offbeat
