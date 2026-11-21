#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"

// =============================================================================
//  右パネル：折りたたみ式の項目を縦に並べる（全体の開閉はヘッダーのボタン）
//  ・各項目は「見出しボタン＋本体」。見出しを押すとその項目だけ開閉（開閉状態は呼び出し側が保存）
//  ・本体は外から渡す（文章、数値の図、比較表など）。高さは項目ごとの関数で決める
// =============================================================================
class TextBody : public juce::TextEditor
{
    juce::String bodyText;
public:
    TextBody()
    {
        setMultiLine (true, true);
        setReadOnly (true);
        setCaretVisible (false);
        setScrollbarsShown (false);
        setIndents (8, 2);
        setScrollToShowCursor (false);
        setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
        setFont (theme::ui (12.0f));
        refreshColours();
    }
    void refreshColours() { setColour (juce::TextEditor::textColourId, theme::text); applyFontToAllText (theme::ui (12.0f)); }
    void setBody (const juce::String& t)
    {
        bodyText = t;
        setText (t, false);
        applyFontToAllText (theme::ui (12.0f));
    }
    // 文章の高さ：TextEditor の測定は更新が遅れて余白が出るので、同じフォントで TextLayout を使って測る
    int fitHeight (int w)
    {
        juce::AttributedString as;
        as.append (bodyText, theme::ui (12.0f), theme::text);
        as.setWordWrap (juce::AttributedString::byWord);
        juce::TextLayout tl;
        tl.createLayout (as, (float) juce::jmax (40, w - 20));
        return juce::jmax (24, (int) std::ceil (tl.getHeight()) + 14);
    }
};

// マウスオーバーした部品の説明を出す欄（右パネルの上部に固定）
class HelpBox : public juce::Component
{
public:
    void setHelp (const juce::String& t, const juce::String& b)
    {
        if (t == title && b == body) return;
        title = t;  body = b;
        repaint();
    }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (theme::lessonBg);  g.fillRoundedRectangle (r, 6.0f);
        g.setColour (theme::lessonLn);  g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
        auto in = getLocalBounds().reduced (10, 6);
        g.setColour (theme::accent);
        g.setFont (theme::ui (12.5f, true));
        g.drawText (title, in.removeFromTop (18), juce::Justification::centredLeft);
        juce::AttributedString as;
        as.append (body, theme::ui (12.0f), theme::text);
        as.setWordWrap (juce::AttributedString::byWord);
        juce::TextLayout tl;
        tl.createLayout (as, (float) in.getWidth());
        tl.draw (g, in.toFloat());
    }
private:
    juce::String title, body;
};

class SidePanel : public juce::Component
{
public:
    SidePanel()
    {
        addAndMakeVisible (vp);
        vp.setViewedComponent (&inner, false);
        vp.setScrollBarsShown (true, false);
        vp.setScrollBarThickness (9);
    }

    // body は呼び出し側が所有する。heightFn(幅) → 本体の高さ
    void addSection (const juce::String& title, juce::Component* body, std::function<int (int)> heightFn)
    {
        auto s = std::make_unique<Section>();
        s->title = title;
        s->body = body;
        s->heightFn = std::move (heightFn);
        const int idx = (int) sections.size();
        s->head.onClick = [this, idx] { toggle (idx); };
        inner.addAndMakeVisible (s->head);
        inner.addChildComponent (body);
        sections.push_back (std::move (s));
        refreshHeads();
    }

    void setTitle (int i, const juce::String& t) { sections[(size_t) i]->title = t; refreshHeads(); }
    void setOpenMask (int mask) { openMask = mask; refreshHeads(); relayout(); }
    int  getOpenMask() const { return openMask; }
    std::function<void (int)> onMaskChanged;

    void resized() override
    {
        vp.setBounds (getLocalBounds().reduced (6, 6));
        relayout();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (theme::panel);      g.fillRoundedRectangle (r, 6.0f);
        g.setColour (theme::line);       g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
    }

    // 本文が変わって高さが変わる時に呼ぶ
    void relayout()
    {
        const int w = juce::jmax (100, vp.getMaximumVisibleWidth());
        int y = 0;
        for (size_t i = 0; i < sections.size(); ++i)
        {
            auto& s = *sections[i];
            s.head.setBounds (0, y, w, 24);
            y += 26;
            const bool open = (openMask >> i) & 1;
            s.body->setVisible (open);
            if (open)
            {
                const int h = s.heightFn ? s.heightFn (w) : 100;
                s.body->setBounds (0, y, w, h);
                y += h + 4;
            }
        }
        inner.setSize (w, y);
        repaint();
    }

    void refreshColours()
    {
        refreshHeads();
        repaint();
    }

private:
    struct Section
    {
        juce::String title;
        juce::TextButton head;
        juce::Component* body = nullptr;
        std::function<int (int)> heightFn;
    };

    void toggle (int i)
    {
        openMask ^= (1 << i);
        refreshHeads();
        relayout();
        if (onMaskChanged) onMaskChanged (openMask);
    }
    void refreshHeads()
    {
        for (size_t i = 0; i < sections.size(); ++i)
        {
            const bool open = (openMask >> i) & 1;
            const auto arrow = juce::String::fromUTF8 (open ? "\xe2\x96\xbc  " : "\xe2\x96\xb6  ");
            sections[i]->head.setButtonText (arrow + sections[i]->title);
            sections[i]->head.setColour (juce::TextButton::buttonColourId, open ? theme::accent.withAlpha (0.25f) : theme::well);
            sections[i]->head.setColour (juce::TextButton::textColourOffId, theme::text);
        }
    }

    juce::Viewport vp;
    juce::Component inner;
    std::vector<std::unique_ptr<Section>> sections;
    int openMask = 0b00011;
};
