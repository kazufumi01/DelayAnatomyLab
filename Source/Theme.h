#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

// =============================================================================
//  JPK デザイン（EQ Anatomy Lab と共通の6スキン）
//    0 Studio  1 Lab Light  2 Studio Night  3 Bridge  4 Blueprint  5 Lesson Warm
//  色は変数なので、実行中に全体を切り替えられる（theme::setSkin → LookAndFeel::apply）。
// =============================================================================
namespace theme
{
inline juce::Colour bg, header, panel, well, line, grid, zero, text, muted, faint, accent, onAccent,
                    green, lessonBg, lessonLn, knobBody, laneL, laneR, popup;
inline bool lightPanels = false;
inline int current = 0;

inline int numSkins() { return 6; }
inline const char* skinName (int i)
{
    static const char* n[] = { "Studio", "Lab Light", "Studio Night", "Bridge", "Blueprint", "Lesson Warm" };
    return n[juce::jlimit (0, 5, i)];
}

inline void setSkin (int i)
{
    current = juce::jlimit (0, 5, i);
    struct P { juce::uint32 bg, header, panel, well, line, grid, zero, text, muted, faint, accent, onAccent,
               lessonBg, lessonLn, ok, popup; bool light; };
    static const P p[] = {
        { 0xff121417, 0xff191c21, 0xff191c21, 0xff0d0f12, 0xff2a2e35, 0xff1d2126, 0xff353a42, 0xffe4e1da, 0xff8d929a, 0xff6c727b, 0xfff0b34a, 0xff1a1306,
          0xff1b1a16, 0xff5a4620, 0xff8fc779, 0xff1c2026, false },
        { 0xfff6f7f9, 0xffe9edf1, 0xffffffff, 0xfffbfcfd, 0xffc8cfd7, 0xffe6e9ed, 0xffb9c1ca, 0xff16202b, 0xff566271, 0xff7a8592, 0xff0d6575, 0xffffffff,
          0xfffbf3e8, 0xffe3c9a6, 0xff3c8a3a, 0xffffffff, true },
        { 0xff0b0c0e, 0xff121418, 0xff15181c, 0xff07080a, 0xff23272e, 0xff15191e, 0xff2c3139, 0xffeceae4, 0xff8d929a, 0xff646a73, 0xffffbf47, 0xff1a1306,
          0xff16150f, 0xff4e3d1b, 0xff8fd27a, 0xff121418, false },
        { 0xff17181a, 0xff1e1f22, 0xff26272a, 0xffefe4c8, 0xff3a3b3f, 0xffdccfb2, 0xffb8ad98, 0xffe8e3d8, 0xff8f8a80, 0xff7d776c, 0xffe8a33a, 0xff1b1205,
          0xff26221a, 0xff5a4620, 0xff4f8f5a, 0xff2a2621, false },
        { 0xffe1e6eb, 0xffd3dae2, 0xffeef1f4, 0xfff6f7f9, 0xffb9c1ca, 0xffdfe3e8, 0xffa9b2bd, 0xff16202b, 0xff566271, 0xff7a8592, 0xff8a5a1c, 0xfffbf3e8,
          0xfffbf3e8, 0xffe3c9a6, 0xff3c8a3a, 0xffffffff, true },
        { 0xff15140f, 0xff1d1b15, 0xff1f1d17, 0xff100f0b, 0xff34302a, 0xff1e1c16, 0xff3d382f, 0xffece5d4, 0xff9a9282, 0xff746d60, 0xffe8a33a, 0xff1b1205,
          0xff241f14, 0xff6a5222, 0xff8fc779, 0xff221f18, false },
    };
    const auto& s = p[current];
    auto C = [] (juce::uint32 v) { return juce::Colour (v); };
    bg = C (s.bg); header = C (s.header); panel = C (s.panel); well = C (s.well); line = C (s.line); grid = C (s.grid); zero = C (s.zero);
    text = C (s.text); muted = C (s.muted); faint = C (s.faint); accent = C (s.accent); onAccent = C (s.onAccent);
    lessonBg = C (s.lessonBg); lessonLn = C (s.lessonLn); green = C (s.ok); popup = C (s.popup); lightPanels = s.light;
    knobBody = lightPanels ? panel.darker (0.07f) : panel.brighter (0.16f);
    const bool brightWell = well.getPerceivedBrightness() > 0.6f;
    laneL = brightWell ? juce::Colour (0xff2f7fb0) : juce::Colour (0xff62b6ea);
    laneR = brightWell ? juce::Colour (0xffb83f36) : juce::Colour (0xffec7a92);
}

// 図の窓（Bridge ではクリーム色）に描く文字の色
inline bool wellIsBright() { return well.getPerceivedBrightness() > 0.6f; }
inline juce::Colour onWell()     { return wellIsBright() ? juce::Colour (0xff3a342a) : muted; }
inline juce::Colour onWellText() { return wellIsBright() ? juce::Colour (0xff231f18) : text; }
inline juce::Colour shade (float a)  { return lightPanels ? juce::Colours::black.withAlpha (a * 0.25f) : juce::Colours::black.withAlpha (a); }
inline juce::Colour hilite (float a) { return lightPanels ? juce::Colours::black.withAlpha (a * 0.6f) : juce::Colours::white.withAlpha (a); }

inline juce::Font ui (float size, bool bold = false)
{
   #if JUCE_MAC
    return juce::Font (juce::FontOptions ("Hiragino Sans", size, bold ? juce::Font::bold : juce::Font::plain));
   #elif JUCE_WINDOWS
    return juce::Font (juce::FontOptions ("Yu Gothic UI", size, bold ? juce::Font::bold : juce::Font::plain));
   #else
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
   #endif
}
inline juce::Font mono (float size, bool bold = false)
{
   #if JUCE_MAC
    return juce::Font (juce::FontOptions ("Menlo", size, bold ? juce::Font::bold : juce::Font::plain));
   #elif JUCE_WINDOWS
    return juce::Font (juce::FontOptions ("Consolas", size, bold ? juce::Font::bold : juce::Font::plain));
   #else
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, bold ? juce::Font::bold : juce::Font::plain));
   #endif
}

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel() { apply(); }

    void apply()
    {
        setColour (juce::TextButton::buttonColourId, panel);
        setColour (juce::TextButton::buttonOnColourId, accent);
        setColour (juce::TextButton::textColourOffId, text);
        setColour (juce::TextButton::textColourOnId, onAccent);
        setColour (juce::ComboBox::backgroundColourId, knobBody);
        setColour (juce::ComboBox::outlineColourId, line);
        setColour (juce::ComboBox::textColourId, text);
        setColour (juce::ComboBox::arrowColourId, muted);
        setColour (juce::PopupMenu::backgroundColourId, popup);
        setColour (juce::PopupMenu::textColourId, text);
        setColour (juce::PopupMenu::headerTextColourId, muted);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, accent);
        setColour (juce::PopupMenu::highlightedTextColourId, onAccent);
        setColour (juce::Label::textColourId, text);
        setColour (juce::Label::textWhenEditingColourId, text);
        setColour (juce::Label::backgroundWhenEditingColourId, well);
        setColour (juce::Label::outlineWhenEditingColourId, accent);
        setColour (juce::TextEditor::backgroundColourId, well);
        setColour (juce::TextEditor::textColourId, text);
        setColour (juce::TextEditor::highlightColourId, accent.withAlpha (0.4f));
        setColour (juce::TextEditor::highlightedTextColourId, text);
        setColour (juce::CaretComponent::caretColourId, accent);
        setColour (juce::Slider::textBoxTextColourId, text);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId, accent.withAlpha (0.4f));
        setColour (juce::ScrollBar::thumbColourId, zero);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = b.getToggleState();
        auto col = b.findColour (on ? juce::TextButton::buttonOnColourId : juce::TextButton::buttonColourId);
        if (! on && (over || down)) col = lightPanels ? col.darker (0.06f) : col.brighter (0.12f);
        g.setColour (col); g.fillRoundedRectangle (r, 4.0f);
        if (! on) { g.setColour (b.findColour (juce::ComboBox::outlineColourId)); g.drawRoundedRectangle (r, 4.0f, 1.0f); }
    }

    // ロータリーノブ：細いトラック＋アクセントの値アーク＋胴＋指針
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float a0, float a1, juce::Slider& s) override
    {
        auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
        const float rad = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;
        if (rad < 10.0f) return;
        const auto c = r.getCentre();
        const float ang = a0 + pos * (a1 - a0);
        const float ar = rad - 2.0f;
        const juce::PathStrokeType stroke (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

        juce::Path track; track.addCentredArc (c.x, c.y, ar, ar, 0.0f, a0, a1, true);
        g.setColour (line); g.strokePath (track, stroke);
        juce::Path val; val.addCentredArc (c.x, c.y, ar, ar, 0.0f, a0, ang, true);
        g.setColour (s.isEnabled() ? accent : muted); g.strokePath (val, stroke);

        const float br = rad - 7.0f;
        g.setColour (knobBody); g.fillEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f);
        g.setColour (line); g.drawEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f, 1.0f);

        g.setColour (text);
        g.drawLine ({ c.getPointOnCircumference (br * 0.3f, ang), c.getPointOnCircumference (br * 0.88f, ang) }, 2.0f);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return ui (juce::jmin (11.5f, h * 0.46f)); }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return ui (11.5f); }
    juce::Font getPopupMenuFont() override { return ui (13.0f); }
    juce::Font getLabelFont (juce::Label& l) override
    {
        if (dynamic_cast<juce::Slider*> (l.getParentComponent()) != nullptr)
            return mono (11.0f);                 // ノブの数値は Menlo
        return l.getFont();
    }
};

// 最初に描く前にデフォルトスキンを入れておく
inline const bool initialised = [] { setSkin (0); return true; }();
}
