// Oberfläche von Offbeat Engine: Modus-Reiter, globale Regler (mit Oktave und Tempo in der
// Kopfzeile), Auto-Bild und die Regler des aktiven Modus. Look und Bedienung: OffbeatKit.hpp.
#include "OffbeatKit.hpp"
#include "CarImage.h" // aus assets/car.png erzeugt (xxd -i), siehe plugin/Makefile

START_NAMESPACE_DISTRHO

using namespace offbeat;
using kit::kCellH;
using kit::kCellW;
using kit::kLabelH;

namespace {
constexpr float kW = 860, kH = 512, kMargin = 20, kGridX = 31, kCols = 7;
constexpr float kGlobalPanelY = 104, kModePanelY = 252;

const int kGlobals[4] = { kVolume, kDrive, kTone, offbeat::kWidth };

// Neon-Farbe pro Modus (wie die verschiedenfarbigen Duftbäume): Kick, Bass, Hat, Stab, Pad, Riser.
const Color kModeColor[kModeCount] = { Color(70, 95, 255), Color(120, 240, 60), Color(60, 225, 235),
    Color(255, 80, 200), Color(175, 115, 255), Color(255, 75, 60) };

kit::Theme engineTheme()
{
    kit::Theme t;
    t.title = "OFFBEAT";
    t.subtitle = "ENGINE";
    t.brand = Color(45, 60, 245);
    t.brandInk = Color(25, 35, 200);
    t.night = Color(5, 7, 26);
    t.night2 = Color(14, 20, 72);
    t.light = Color(140, 165, 255);
    return t;
}
} // namespace

class OffbeatUI : public OffbeatKitUI {
public:
    OffbeatUI()
        : OffbeatKitUI(kW, kH, engineTheme())
    {
        car = createImageFromMemory(car_png, car_png_len, IMAGE_GENERATE_MIPMAPS);
    }

protected:
    void drawContent() override
    {
        const int mode = currentMode();
        const Color accent = kModeColor[mode];

        drawLogo(145, 50);
        for (int m = 0; m < kModeCount; ++m) {
            const float w = 86, x = kW - kMargin - (kModeCount - m) * (w + 6) + 6;
            pill(100 + m, x, 33, w, 34, kModeNames[m], kModeColor[m], m == mode);
        }

        drawPanel(kMargin, kGlobalPanelY, kW - 2 * kMargin, kLabelH + kCellH, "GLOBAL", th.brand);
        for (int i = 0; i < 4; ++i)
            knob(kGlobals[i], kGridX + i * kCellW, kGlobalPanelY + kLabelH, th.brand);
        // Auto rechts; ragt etwas über die Plakette hinaus, die Lichtstreifen laufen unter dem Width-Regler aus.
        const float carW = 410, carH = carW * 384.0f / 1024.0f;
        drawImage(car, kW - carW + 6, kGlobalPanelY + (kLabelH + kCellH) / 2 - carH / 2 + 4, carW, carH);
        compact(kOctave, 110, kGlobalPanelY + 3, 104, th.brand);
        compact(kTempoHost, 222, kGlobalPanelY + 3, 128, th.brand);
        compact(kTempoBpm, 358, kGlobalPanelY + 3, 116, th.brand);
        copyPaste(kW - kMargin - 12, kGlobalPanelY + 3);

        char label[32];
        kit::upper(kModeNames[mode], label, sizeof(label));
        drawPanel(kMargin, kModePanelY, kW - 2 * kMargin, kLabelH + 2 * kCellH, label, accent);
        char tag[48];
        std::snprintf(tag, sizeof(tag), "MODE %d/%d  \xc2\xb7  VST3 / LV2", mode + 1, kModeCount);
        drawTag(kW - kMargin - 14, kModePanelY + 13, tag);
        int list[16];
        const int count = modeParams(mode, list);
        for (int n = 0; n < count; ++n)
            knob(list[n], kGridX + (n % (int)kCols) * kCellW, kModePanelY + kLabelH + (n / (int)kCols) * kCellH, accent);
    }

    void onButton(int id) override
    {
        if (id >= 100 && id < 100 + kModeCount)
            setValue(kMode, (float)(id - 100), true);
    }

    // KOPIEREN: Modus, globale Regler, Oktave/Tempo und die Regler des aktiven Modus.
    int jsonParams(int* out, int) const override
    {
        const int head[] = { kMode, kVolume, kDrive, kTone, offbeat::kWidth, kOctave, kTempoHost, kTempoBpm };
        int n = 0;
        for (int p : head)
            out[n++] = p;
        return n + modeParams(currentMode(), out + n);
    }

    // Name ohne Modus-Vorsilbe ("Kick Start Pitch" → "Start Pitch"), kurze Namen für die Kopfzeile.
    const char* controlLabel(int p) const override
    {
        if (p == kTempoHost)
            return "Host-Tempo";
        if (p == kTempoBpm)
            return "BPM";
        const char* name = kParams[p].name;
        if (p >= kKickStart && p <= kKickAttack)
            if (const char* sp = std::strchr(name, ' '))
                return sp + 1;
        return name;
    }

private:
    NanoImage car;

    int currentMode() const
    {
        const int m = (int)std::lround(values[kMode]);
        return m < 0 ? 0 : (m >= kModeCount ? kModeCount - 1 : m);
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OffbeatUI)
};

UI* createUI()
{
    return new OffbeatUI();
}

END_NAMESPACE_DISTRHO
