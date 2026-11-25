// Eigene Oberfläche (Phase 3): Modus-Reiter, globale Regler und die Regler des aktiven Modus.
// Bedienung: ziehen (hoch/runter), Shift = fein, Mausrad, Doppelklick = Standardwert,
// Schalter (An/Aus) per Klick.
#include "DistrhoUI.hpp"
#include "Params.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

START_NAMESPACE_DISTRHO

using namespace offbeat;

namespace {
constexpr int kUiWidth = 810, kUiHeight = 480;
constexpr int kCellW = 110, kCellH = 118, kCols = 7, kMargin = 20;
constexpr int kHeaderH = 56, kGlobalY = 84, kModeY = 236;
constexpr float kPi = 3.14159265f;

// Erster Parameter jedes Modus; der Modus reicht bis zum ersten des nächsten.
const int kModeFirst[kModeCount + 1] = { kKickStart, kBassWave, kHatMix, kStabVoices, kPadDetune, kRiserBars, kParamCount };
const int kGlobals[4] = { kVolume, kDrive, kTone, offbeat::kWidth };

const Color kBack(22, 23, 27), kPanel(32, 34, 40), kTrack(58, 61, 70), kText(225, 226, 230),
    kDim(140, 143, 152), kAccent(255, 122, 26);
} // namespace

class OffbeatUI : public UI {
public:
    OffbeatUI()
        : UI(kUiWidth, kUiHeight, true)
    {
        applyDefaults(values);
        loadSharedResources();
    }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        if (index >= (uint32_t)kParamCount)
            return;
        values[index] = value;
        repaint();
    }

    void programLoaded(uint32_t index) override
    {
        applyPreset((int)index, values);
        repaint();
    }

    void onNanoDisplay() override
    {
        const float s = getWidth() / (float)kUiWidth; // DPF skaliert das Fenster, wir die Zeichnung
        scale(s, s);
        fontFace(NANOVG_DEJAVU_SANS_TTF);

        beginPath();
        rect(0, 0, kUiWidth, kUiHeight);
        fillColor(kBack);
        fill();

        fontSize(22);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        fillColor(kAccent);
        text(kMargin, kHeaderH / 2, "OFFBEAT", nullptr);

        const int mode = currentMode();
        for (int m = 0; m < kModeCount; ++m) {
            float x, y, w, h;
            tabRect(m, x, y, w, h);
            beginPath();
            roundedRect(x, y, w, h, 5);
            fillColor(m == mode ? kAccent : kPanel);
            fill();
            fontSize(14);
            textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
            fillColor(m == mode ? kBack : kText);
            text(x + w / 2, y + h / 2, kModeNames[m], nullptr);
        }

        sectionLabel(kGlobalY - 14, "GLOBAL");
        for (int i = 0; i < 4; ++i)
            drawControl(kGlobals[i], i, kGlobalY);

        sectionLabel(kModeY - 14, kModeNames[mode]);
        for (int p = kModeFirst[mode]; p < kModeFirst[mode + 1]; ++p) {
            const int n = p - kModeFirst[mode];
            drawControl(p, n % kCols, kModeY + (n / kCols) * kCellH);
        }
    }

    bool onMouse(const MouseEvent& ev) override
    {
        if (ev.button != 1)
            return false;
        const float s = getWidth() / (float)kUiWidth;
        const float x = ev.pos.getX() / s, y = ev.pos.getY() / s;

        if (!ev.press) {
            if (dragParam >= 0)
                editParameter(dragParam, false);
            dragParam = -1;
            return true;
        }

        for (int m = 0; m < kModeCount; ++m) {
            float tx, ty, tw, th;
            tabRect(m, tx, ty, tw, th);
            if (x >= tx && x < tx + tw && y >= ty && y < ty + th) {
                setValue(kMode, (float)m, true);
                return true;
            }
        }

        const int p = paramAt(x, y);
        if (p < 0)
            return false;
        const ParamInfo& info = kParams[p];
        if (info.flags & kFlagBool) {
            setValue(p, values[p] > 0.5f ? 0.0f : 1.0f, true);
            return true;
        }
        if (p == lastClickParam && ev.time - lastClickTime < 300) { // Doppelklick
            setValue(p, info.def, true);
            lastClickParam = -1;
            return true;
        }
        lastClickParam = p;
        lastClickTime = ev.time;
        dragParam = p;
        dragY = y;
        dragNorm = toNorm(p, values[p]);
        editParameter(p, true);
        return true;
    }

    bool onMotion(const MotionEvent& ev) override
    {
        if (dragParam < 0)
            return false;
        const float s = getWidth() / (float)kUiWidth;
        const float y = ev.pos.getY() / s;
        const float range = (ev.mod & kModifierShift) ? 1000.0f : 200.0f;
        dragNorm = std::fmin(1.0f, std::fmax(0.0f, dragNorm + (dragY - y) / range));
        dragY = y;
        setValue(dragParam, fromNorm(dragParam, dragNorm), false);
        return true;
    }

    bool onScroll(const ScrollEvent& ev) override
    {
        const float s = getWidth() / (float)kUiWidth;
        const int p = paramAt(ev.pos.getX() / s, ev.pos.getY() / s);
        if (p < 0 || ev.delta.getY() == 0.0)
            return false;
        const ParamInfo& info = kParams[p];
        const float dir = ev.delta.getY() > 0.0 ? 1.0f : -1.0f;
        float v;
        if (info.flags & (kFlagInt | kFlagBool))
            v = std::fmin(info.max, std::fmax(info.min, values[p] + dir));
        else
            v = fromNorm(p, std::fmin(1.0f, std::fmax(0.0f, toNorm(p, values[p]) + dir * 0.02f)));
        setValue(p, v, true);
        return true;
    }

private:
    float values[kParamCount];
    int dragParam = -1, lastClickParam = -1;
    uint lastClickTime = 0;
    float dragY = 0, dragNorm = 0;

    int currentMode() const
    {
        const int m = (int)std::lround(values[kMode]);
        return m < 0 ? 0 : (m >= kModeCount ? kModeCount - 1 : m);
    }

    void setValue(int p, float v, bool gesture)
    {
        const ParamInfo& info = kParams[p];
        if (info.flags & (kFlagInt | kFlagBool))
            v = std::round(v);
        if (gesture)
            editParameter(p, true);
        values[p] = v;
        setParameterValue(p, v);
        if (gesture)
            editParameter(p, false);
        repaint();
    }

    static float toNorm(int p, float v)
    {
        const ParamInfo& i = kParams[p];
        if (i.flags & kFlagLog)
            return std::log(v / i.min) / std::log(i.max / i.min);
        return (v - i.min) / (i.max - i.min);
    }

    static float fromNorm(int p, float n)
    {
        const ParamInfo& i = kParams[p];
        float v = (i.flags & kFlagLog) ? i.min * std::pow(i.max / i.min, n) : i.min + n * (i.max - i.min);
        if (i.flags & (kFlagInt | kFlagBool))
            v = std::round(v);
        return v;
    }

    static void tabRect(int m, float& x, float& y, float& w, float& h)
    {
        w = 84, h = 30, y = (kHeaderH - h) / 2;
        x = kUiWidth - kMargin - (kModeCount - m) * (w + 6) + 6;
    }

    // Parameter unter der Maus: globale Reihe oder Raster des aktiven Modus.
    int paramAt(float x, float y) const
    {
        const int col = (int)((x - kMargin) / kCellW);
        if (x < kMargin || col >= kCols)
            return -1;
        if (y >= kGlobalY && y < kGlobalY + kCellH)
            return col < 4 ? kGlobals[col] : -1;
        if (y < kModeY)
            return -1;
        const int mode = currentMode();
        const int p = kModeFirst[mode] + (int)((y - kModeY) / kCellH) * kCols + col;
        return p < kModeFirst[mode + 1] ? p : -1;
    }

    void sectionLabel(float y, const char* label)
    {
        fontSize(12);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        fillColor(kDim);
        text(kMargin, y, label, nullptr);
    }

    void formatValue(int p, char* buf, size_t size) const
    {
        const ParamInfo& i = kParams[p];
        const float v = values[p];
        if (i.labels)
            std::snprintf(buf, size, "%s", i.labels[(int)std::lround(v)]);
        else if (i.flags & kFlagBool)
            std::snprintf(buf, size, "%s", v > 0.5f ? "An" : "Aus");
        else if (i.flags & kFlagInt)
            std::snprintf(buf, size, "%d", (int)std::lround(v));
        else if (!std::strcmp(i.unit, "Hz") && v >= 1000.0f)
            std::snprintf(buf, size, "%.1f kHz", v / 1000.0f);
        else if (!std::strcmp(i.unit, "Hz") || !std::strcmp(i.unit, "ct") || !std::strcmp(i.unit, "ms"))
            std::snprintf(buf, size, v < 10.0f ? "%.1f %s" : "%.0f %s", v, i.unit);
        else if (i.unit[0])
            std::snprintf(buf, size, "%.1f %s", v, i.unit);
        else
            std::snprintf(buf, size, "%.0f %%", 100.0f * v);
    }

    void drawControl(int p, int col, float top)
    {
        const ParamInfo& info = kParams[p];
        const float x = kMargin + col * kCellW;
        const float cx = x + kCellW / 2, cy = top + 40;

        beginPath();
        roundedRect(x + 4, top, kCellW - 8, kCellH - 8, 6);
        fillColor(kPanel);
        fill();

        if (info.flags & kFlagBool) {
            const bool on = values[p] > 0.5f;
            beginPath();
            roundedRect(cx - 22, cy - 11, 44, 22, 11);
            fillColor(on ? kAccent : kTrack);
            fill();
            beginPath();
            circle(on ? cx + 11 : cx - 11, cy, 8);
            fillColor(kText);
            fill();
        } else {
            const float n = std::fmin(1.0f, std::fmax(0.0f, toNorm(p, values[p])));
            const float a0 = 0.75f * kPi, a1 = a0 + 1.5f * kPi * n, r = 24;
            beginPath();
            arc(cx, cy, r, a0, 2.25f * kPi, CW);
            strokeWidth(5);
            strokeColor(kTrack);
            stroke();
            if (n > 0.001f) {
                beginPath();
                arc(cx, cy, r, a0, a1, CW);
                strokeColor(kAccent);
                stroke();
            }
            beginPath();
            moveTo(cx + 0.35f * r * std::cos(a1), cy + 0.35f * r * std::sin(a1));
            lineTo(cx + 0.85f * r * std::cos(a1), cy + 0.85f * r * std::sin(a1));
            strokeWidth(3);
            strokeColor(kText);
            stroke();
        }

        // Name ohne Modus-Vorsilbe ("Kick Start Pitch" → "Start Pitch")
        const char* name = info.name;
        if (p >= kModeFirst[0])
            if (const char* sp = std::strchr(name, ' '))
                name = sp + 1;
        fontSize(12);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        fillColor(kText);
        text(cx, top + 78, name, nullptr);

        char buf[32];
        formatValue(p, buf, sizeof(buf));
        fillColor(kDim);
        text(cx, top + 96, buf, nullptr);
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OffbeatUI)
};

UI* createUI()
{
    return new OffbeatUI();
}

END_NAMESPACE_DISTRHO
