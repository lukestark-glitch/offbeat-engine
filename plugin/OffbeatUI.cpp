// Eigene Oberfläche (Phase 3): Modus-Reiter, globale Regler und die Regler des aktiven Modus.
// Look: Y2K-Rave/Tuning – Nachtblau, Chrom, Neon-Swoosh; jeder Modus hat seine eigene Farbe.
// Bedienung: ziehen (hoch/runter), Shift = fein, Mausrad, Doppelklick = Standardwert,
// Schalter (An/Aus) per Klick. Klick auf den Wert (oder Rechtsklick) = Zahl eintippen.
// KOPIEREN/EINFÜGEN: Einstellungen als JSON über die Zwischenablage (Format: docs/REGLER.md).
#include "DistrhoUI.hpp"
#include "ParamText.hpp"
#include "CarImage.h" // aus assets/car.png erzeugt (xxd -i), siehe plugin/Makefile

#include <cctype>
#include <cmath>
#include <cstdio>
#include <chrono>
#include <cstring>
#include <string>

START_NAMESPACE_DISTRHO

using namespace offbeat;

namespace {
constexpr int kUiWidth = 860, kUiHeight = 512;
constexpr int kCellW = 114, kCellH = 112, kCols = 7, kMargin = 20, kGridX = 31;
constexpr int kGlobalPanelY = 104, kModePanelY = 252, kLabelH = 22;
constexpr int kGlobalY = kGlobalPanelY + kLabelH, kModeY = kModePanelY + kLabelH;
constexpr float kPi = 3.14159265f;

const int kGlobals[4] = { kVolume, kDrive, kTone, offbeat::kWidth };

// Neon-Farbe pro Modus (wie die verschiedenfarbigen Duftbäume): Kick, Bass, Hat, Stab, Pad, Riser.
const Color kModeColor[kModeCount] = { Color(70, 95, 255), Color(120, 240, 60), Color(60, 225, 235),
    Color(255, 80, 200), Color(175, 115, 255), Color(255, 75, 60) };
const Color kNight(5, 7, 26), kNight2(14, 20, 72), kPlate(10, 14, 46), kTrack(32, 40, 100),
    kText(232, 236, 255), kDim(150, 162, 225), kBlue(45, 60, 245), kChromeHi(245, 247, 255),
    kChromeLo(120, 126, 155);

Color withAlpha(Color c, float a)
{
    c.alpha = a;
    return c;
}

void upper(const char* in, char* out, size_t size)
{
    size_t i = 0;
    for (; in[i] && i + 1 < size; ++i)
        out[i] = (char)std::toupper((unsigned char)in[i]);
    out[i] = 0;
}
} // namespace

class OffbeatUI : public UI {
public:
    OffbeatUI()
        : UI(kUiWidth, kUiHeight, true)
    {
        applyDefaults(values);
        loadSharedResources();
        car = createImageFromMemory(car_png, car_png_len, IMAGE_GENERATE_MIPMAPS);
    }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        if (index >= (uint32_t)kParamCount)
            return;
        values[index] = value;
        dirty = true;
        repaint();
    }

    void programLoaded(uint32_t index) override
    {
        applyPreset((int)index, values);
        dirty = true;
        repaint();
    }

    // Manche Hosts (Carla-Bridge auf macOS) zeichnen ein einzelnes repaint() aus einem Klick
    // erst beim nächsten Ereignis; hier wird im Idle-Takt sicher nachgezeichnet.
    void uiIdle() override
    {
        if (dirty)
            repaint();
    }

    void onNanoDisplay() override
    {
        dirty = false;
        const float s = getWidth() / (float)kUiWidth; // DPF skaliert das Fenster, wir die Zeichnung
        scale(s, s);
        fontFace(NANOVG_DEJAVU_SANS_TTF);
        const int mode = currentMode();
        const Color accent = kModeColor[mode];

        drawBackground();
        drawLogo();
        for (int m = 0; m < kModeCount; ++m)
            drawTab(m, m == mode);

        drawPanel(kGlobalPanelY, 1, "GLOBAL", kBlue);
        for (int i = 0; i < 4; ++i)
            drawControl(kGlobals[i], i, kGlobalY, kBlue);
        drawCar();

        char label[32];
        upper(kModeNames[mode], label, sizeof(label));
        drawPanel(kModePanelY, 2, label, accent);
        drawModeInfo(mode);
        int list[16];
        const int count = modeParams(mode, list);
        for (int n = 0; n < count; ++n)
            drawControl(list[n], n % kCols, kModeY + (n / kCols) * kCellH, accent);

        for (int b = 0; b < 2; ++b)
            drawButton(b);
        if (!status.empty() && Clock::now() < statusUntil) {
            fontSize(11);
            textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
            fillColor(kText);
            float bx, by, bw, bh;
            buttonRect(0, bx, by, bw, bh);
            text(bx - 12, by + bh / 2, status.c_str(), nullptr);
        }
    }

    bool onMouse(const MouseEvent& ev) override
    {
        const float s = getWidth() / (float)kUiWidth;
        const float x = ev.pos.getX() / s, y = ev.pos.getY() / s;

        if (ev.press && editParam >= 0) { // Klick irgendwohin übernimmt die Eingabe
            commitEdit();
            return true;
        }
        if (ev.press && ev.button == kMouseButtonRight) { // Rechtsklick: Zahl eintippen
            const int p = paramAt(x, y);
            if (p >= 0)
                startEdit(p);
            return p >= 0;
        }
        if (ev.button != kMouseButtonLeft)
            return false;

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

        for (int b = 0; b < 2; ++b) {
            float bx, by, bw, bh;
            buttonRect(b, bx, by, bw, bh);
            if (x >= bx && x < bx + bw && y >= by && y < by + bh) {
                b == 0 ? copySettings() : pasteSettings();
                return true;
            }
        }

        float cellY;
        const int p = paramAt(x, y, &cellY);
        if (p < 0)
            return false;
        if (cellY >= 82) { // Klick auf den Wert unter dem Regler
            startEdit(p);
            return true;
        }
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

    bool onKeyboard(const KeyboardEvent& ev) override
    {
        if (editParam < 0 || !ev.press)
            return false;
        if (ev.key == kKeyEnter)
            commitEdit();
        else if (ev.key == kKeyEscape)
            editParam = -1;
        else if ((ev.key == kKeyBackspace || ev.key == kKeyDelete) && !editText.empty())
            editText.pop_back();
        dirty = true;
        repaint();
        return true;
    }

    bool onCharacterInput(const CharacterInputEvent& ev) override
    {
        if (editParam < 0)
            return false;
        if (ev.character >= 32 && ev.character != 127 && editText.size() < 20)
            editText += ev.string;
        dirty = true;
        repaint();
        return true;
    }

private:
    using Clock = std::chrono::steady_clock;
    float values[kParamCount];
    int editParam = -1;
    std::string editText, status;
    Clock::time_point statusUntil;
    int dragParam = -1, lastClickParam = -1;
    uint lastClickTime = 0;
    float dragY = 0, dragNorm = 0;
    bool dirty = true;
    NanoImage car;

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
        dirty = true;
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
        w = 86, h = 34, y = 50 - h / 2;
        x = kUiWidth - kMargin - (kModeCount - m) * (w + 6) + 6;
    }

    // Parameter unter der Maus: globale Reihe oder Raster des aktiven Modus.
    // cellY: Höhe innerhalb der Zelle (Regler oben, Name und Wert unten).
    int paramAt(float x, float y, float* cellY = nullptr) const
    {
        const int col = (int)((x - kGridX) / kCellW);
        if (x < kGridX || col >= kCols)
            return -1;
        if (y >= kGlobalY && y < kGlobalY + kCellH) {
            if (cellY)
                *cellY = y - kGlobalY;
            return col < 4 ? kGlobals[col] : -1;
        }
        if (y < kModeY)
            return -1;
        const int row = (int)((y - kModeY) / kCellH), n = row * kCols + col;
        if (cellY)
            *cellY = y - kModeY - row * kCellH;
        int list[16];
        return n < modeParams(currentMode(), list) ? list[n] : -1;
    }

    void startEdit(int p)
    {
        if (dragParam >= 0)
            editParameter(dragParam, false);
        dragParam = -1;
        editParam = p;
        editText.clear();
        dirty = true;
        repaint();
    }

    void commitEdit()
    {
        const int p = editParam;
        editParam = -1;
        float v;
        if (!editText.empty() && parseParamText(p, editText.c_str(), v))
            setValue(p, v, true);
        else if (!editText.empty())
            showStatus("Ungültige Eingabe");
        dirty = true;
        repaint();
    }

    void showStatus(const std::string& text)
    {
        status = text;
        statusUntil = Clock::now() + std::chrono::seconds(4);
        dirty = true;
        repaint();
    }

    static void buttonRect(int b, float& x, float& y, float& w, float& h)
    {
        w = 92, h = 20, y = kGlobalPanelY + 3;
        x = kUiWidth - kMargin - 12 - (2 - b) * (w + 6) + 6;
    }

    void copySettings()
    {
        const std::string json = toJson(values);
        if (getWindow().setClipboard("text/plain", json.c_str(), json.size()))
            showStatus("Einstellungen kopiert");
        else
            showStatus("Kopieren ging nicht");
    }

    void pasteSettings()
    {
        size_t size = 0;
        const char* data = (const char*)getWindow().getClipboard(size);
        if (!data || !size) {
            showStatus("Zwischenablage ist leer");
            return;
        }
        const std::string json(data, size);
        float next[kParamCount];
        std::memcpy(next, values, sizeof(next));
        bool changed[kParamCount] = {};
        std::string errors;
        const int n = applyJson(json.c_str(), next, changed, errors);
        for (int p = 0; p < kParamCount; ++p)
            if (changed[p])
                setValue(p, next[p], true);
        char msg[64];
        std::snprintf(msg, sizeof(msg), "%d Werte gesetzt", n);
        showStatus(n == 0 ? "Kein gültiges JSON" : errors.empty() ? msg : std::string(msg) + " (" + errors + ")");
    }

    void drawButton(int b)
    {
        float x, y, w, h;
        buttonRect(b, x, y, w, h);
        beginPath();
        roundedRect(x, y, w, h, h / 2);
        fillColor(kPlate);
        fill();
        strokeColor(kBlue);
        strokeWidth(1.5f);
        stroke();
        fontSize(10);
        textLetterSpacing(1.5f);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        fillColor(kText);
        text(x + w / 2, y + h / 2 + 1, b == 0 ? "KOPIEREN" : "EINF\xc3\x9cGEN", nullptr);
        textLetterSpacing(0);
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

    // ---- Zeichnen ---------------------------------------------------------------------------

    // Nachtblauer Verlauf, Lochblech-Raster und zwei schräge Lichtstreifen wie in einer Garage.
    void drawBackground()
    {
        beginPath();
        rect(0, 0, kUiWidth, kUiHeight);
        fillPaint(linearGradient(0, 0, 0, kUiHeight, kNight, kNight2));
        fill();

        beginPath();
        for (int y = 9; y < kUiHeight; y += 16)
            for (int x = 9 + (y / 16 % 2) * 8; x < kUiWidth; x += 16)
                circle(x, y, 1.1f);
        fillColor(Color(120, 140, 255, 0.07f));
        fill();

        for (int i = 0; i < 2; ++i) {
            const float x0 = i == 0 ? 60.0f : kUiWidth - 60.0f, dx = i == 0 ? 220.0f : -220.0f;
            beginPath();
            moveTo(x0, 0);
            lineTo(x0 + 26 * (i ? -1 : 1), 0);
            lineTo(x0 + dx + 60 * (i ? -1 : 1), kUiHeight);
            lineTo(x0 + dx, kUiHeight);
            closePath();
            fillPaint(linearGradient(0, 0, 0, kUiHeight, Color(170, 190, 255, 0.10f), Color(170, 190, 255, 0.0f)));
            fill();
        }
    }

    // Eigenes Logo: Chrom-Plakette mit kursivem Schriftzug, umkreist von einem blauen Swoosh.
    void drawLogo()
    {
        const float cx = 145, cy = 50;
        // Swoosh als Ellipse: zwei Bögen, der vordere läuft über die Plakette
        auto swoosh = [&](float a0, float a1) {
            save();
            translate(cx, cy);
            rotate(-0.12f);
            scale(1.0f, 0.3f);
            beginPath();
            arc(0, 0, 132, a0, a1, CW);
            restore();
            lineCap(ROUND);
            strokeColor(withAlpha(kBlue, 0.35f));
            strokeWidth(13);
            stroke();
            strokeColor(kBlue);
            strokeWidth(6);
            stroke();
        };
        swoosh(kPi * 1.08f, kPi * 1.92f); // hinterer Bogen

        beginPath();
        roundedRect(cx - 105, cy - 25, 210, 50, 12);
        fillPaint(linearGradient(0, cy - 25, 0, cy + 25, kChromeHi, kChromeLo));
        fill();
        strokeColor(kBlue);
        strokeWidth(3);
        stroke();

        save();
        translate(cx - 4, cy + 1);
        skewX(-0.28f);
        fontSize(31);
        textLetterSpacing(3);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        fontBlur(5);
        fillColor(withAlpha(kBlue, 0.45f));
        text(0, 0, "OFFBEAT", nullptr);
        fontBlur(0);
        fillColor(Color(25, 35, 200));
        text(0, 0, "OFFBEAT", nullptr);
        restore();
        textLetterSpacing(0);

        swoosh(kPi * 0.08f, kPi * 0.92f); // vorderer Bogen

        fontSize(9);
        textLetterSpacing(2);
        textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
        fillColor(kText);
        text(cx + 105, cy + 36, "BOUNCE SYNTH", nullptr);
        textLetterSpacing(0);
    }

    // Reiter als Pille in der Farbe des Modus; der aktive leuchtet.
    void drawTab(int m, bool active)
    {
        float x, y, w, h;
        tabRect(m, x, y, w, h);
        const Color c = kModeColor[m];
        if (active) {
            beginPath();
            rect(x - 12, y - 12, w + 24, h + 24);
            fillPaint(boxGradient(x, y, w, h, h / 2, 14, withAlpha(c, 0.55f), withAlpha(c, 0.0f)));
            fill();
        }
        beginPath();
        roundedRect(x, y, w, h, h / 2);
        fillColor(active ? c : withAlpha(kPlate, 0.85f));
        fill();
        strokeColor(withAlpha(c, active ? 1.0f : 0.6f));
        strokeWidth(1.5f);
        stroke();

        char label[16];
        upper(kModeNames[m], label, sizeof(label));
        fontSize(13);
        textLetterSpacing(1.5f);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        fillColor(active ? kNight : kText);
        text(x + w / 2, y + h / 2 + 1, label, nullptr);
        textLetterSpacing(0);
    }

    // Abschnitt im Ticket-Stil: dunkle Plakette, dünner Neonrand, Kopfzeile mit Linie.
    void drawPanel(float top, int rows, const char* label, const Color& c)
    {
        const float h = kLabelH + rows * kCellH;
        beginPath();
        roundedRect(kMargin, top, kUiWidth - 2 * kMargin, h, 10);
        fillColor(withAlpha(kPlate, 0.78f));
        fill();
        strokeColor(withAlpha(c, 0.55f));
        strokeWidth(1);
        stroke();

        fontSize(11);
        textLetterSpacing(3);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        fillColor(c);
        const float tw = text(kMargin + 14, top + 13, label, nullptr) - (kMargin + 14);
        textLetterSpacing(0);
        beginPath();
        moveTo(kMargin + 24 + tw, top + 13);
        lineTo(kUiWidth - kMargin - 14, top + 13);
        strokeColor(withAlpha(c, 0.35f));
        stroke();
    }

    // Auto rechts neben den globalen Reglern; ragt etwas über den Rand der Plakette hinaus.
    // Die Lichtstreifen links laufen unter dem Width-Regler aus, deshalb vor den Reglern zeichnen.
    void drawCar()
    {
        if (!car.isValid())
            return;
        const float w = 410, h = w * 384.0f / 1024.0f;
        const float x = kUiWidth - w + 6, y = kGlobalPanelY + (kLabelH + kCellH) / 2.0f - h / 2 + 4;
        beginPath();
        rect(x, y, w, h);
        fillPaint(imagePattern(x, y, w, h, 0, car, 1.0f));
        fill();
    }

    // Kleines Etikett rechts in der Kopfzeile des Modus-Abschnitts (Ticket-Stil).
    void drawModeInfo(int mode)
    {
        char info[48];
        std::snprintf(info, sizeof(info), "MODE %d/%d  \xc2\xb7  VST3 / LV2", mode + 1, kModeCount);
        fontSize(9);
        textLetterSpacing(1.5f);
        textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
        Rectangle<float> bounds;
        const float x = kUiWidth - kMargin - 14, y = kModePanelY + 13;
        textBounds(x, y, info, nullptr, bounds);
        beginPath();
        rect(bounds.getX() - 10, y - 6, bounds.getWidth() + 10, 12);
        fillColor(kPlate);
        fill();
        fillColor(kDim);
        text(x, y, info, nullptr);
        textLetterSpacing(0);
    }

    // Regler: Neon-Bogen mit Glühen um eine Chrom-Kappe; An/Aus als leuchtende Pille.
    void drawControl(int p, int col, float top, const Color& c)
    {
        const ParamInfo& info = kParams[p];
        const float x = kGridX + col * kCellW;
        const float cx = x + kCellW / 2, cy = top + 36;

        if (info.flags & kFlagBool) {
            const bool on = values[p] > 0.5f;
            if (on) {
                beginPath();
                rect(cx - 36, cy - 24, 72, 48);
                fillPaint(boxGradient(cx - 24, cy - 12, 48, 24, 12, 10, withAlpha(c, 0.6f), withAlpha(c, 0.0f)));
                fill();
            }
            beginPath();
            roundedRect(cx - 24, cy - 12, 48, 24, 12);
            fillColor(on ? c : kTrack);
            fill();
            beginPath();
            const float kx = on ? cx + 12 : cx - 12;
            circle(kx, cy, 9);
            fillPaint(linearGradient(0, cy - 9, 0, cy + 9, kChromeHi, kChromeLo));
            fill();
        } else {
            const float n = std::fmin(1.0f, std::fmax(0.0f, toNorm(p, values[p])));
            const float a0 = 0.75f * kPi, a1 = a0 + 1.5f * kPi * n, r = 26;
            lineCap(ROUND);
            beginPath();
            arc(cx, cy, r, a0, 2.25f * kPi, CW);
            strokeWidth(5);
            strokeColor(kTrack);
            stroke();
            if (n > 0.001f) {
                beginPath();
                arc(cx, cy, r, a0, a1, CW);
                strokeWidth(11);
                strokeColor(withAlpha(c, 0.25f));
                stroke();
                strokeWidth(4);
                strokeColor(c);
                stroke();
            }
            lineCap(BUTT);

            beginPath(); // Chrom-Kappe mit Fase
            circle(cx, cy, 18);
            fillPaint(linearGradient(0, cy - 18, 0, cy + 18, kChromeHi, Color(70, 76, 110)));
            fill();
            beginPath();
            circle(cx, cy, 14.5f);
            fillPaint(linearGradient(0, cy - 15, 0, cy + 15, Color(160, 166, 195), kChromeHi));
            fill();

            beginPath();
            moveTo(cx + 5 * std::cos(a1), cy + 5 * std::sin(a1));
            lineTo(cx + 13 * std::cos(a1), cy + 13 * std::sin(a1));
            strokeWidth(3);
            lineCap(ROUND);
            strokeColor(kNight);
            stroke();
            lineCap(BUTT);
        }

        // Name ohne Modus-Vorsilbe ("Kick Start Pitch" → "START PITCH")
        const char* name = info.name;
        if (p >= kKickStart)
            if (const char* sp = std::strchr(name, ' '))
                name = sp + 1;
        char label[32];
        upper(name, label, sizeof(label));
        fontSize(10);
        textLetterSpacing(1);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        fillColor(kDim);
        text(cx, top + 76, label, nullptr);
        textLetterSpacing(0);

        if (p == editParam) { // Eingabefeld mit Cursor
            beginPath();
            roundedRect(cx - 44, top + 84, 88, 19, 4);
            fillColor(kNight);
            fill();
            strokeColor(c);
            strokeWidth(1.5f);
            stroke();
            fontSize(13);
            fillColor(kText);
            text(cx, top + 93, (editText + "|").c_str(), nullptr);
            return;
        }
        char buf[32];
        formatValue(p, buf, sizeof(buf));
        fontSize(13);
        fillColor(kText);
        text(cx, top + 93, buf, nullptr);
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OffbeatUI)
};

UI* createUI()
{
    return new OffbeatUI();
}

END_NAMESPACE_DISTRHO
