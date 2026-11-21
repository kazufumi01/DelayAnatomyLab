#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Params.h"
#include "Presets.h"
#include "Localisation.h"
#include "Theme.h"

// =============================================================================
//  ParamCompare：プリセットの初期値と、いまの値を並べて比較する表（現在値は直接入力できる）
//   ・数値の行：現在値をクリック → 数字を打って Enter で反映（ノブと同じパラメーター）
//   ・選択肢の行（Mode / Sync / Machine / Pitch Mode）：クリックでメニュー、Reverse はクリックで ON/OFF
//   ・変えた行はアクセント色で強調。左列＝Basic、右列＝Analog / FX（表示の VIEW に合わせて出し入れ）
// =============================================================================
class ParamCompare : public juce::Component
{
public:
    explicit ParamCompare (juce::AudioProcessorValueTreeState& a) : apvts (a)
    {
        using F = std::function<float (const PresetValues&, double)>;
        auto cur = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
        (void) cur;

        auto choice = [this] (const char* label, const char* id, int minView, std::function<juce::StringArray()> names, F pv)
        {
            Def d; d.label = label; d.id = id; d.minView = minView; d.kind = Choice; d.names = std::move (names); d.preset = std::move (pv);
            defs.push_back (std::move (d));
        };
        auto num = [this] (const char* label, const char* id, int minView, int decimals, const char* unit, F pv)
        {
            Def d; d.label = label; d.id = id; d.minView = minView; d.kind = Num; d.decimals = decimals; d.unit = unit; d.preset = std::move (pv);
            defs.push_back (std::move (d));
        };

        // ---- 左列：Basic ----
        choice ("Mode",   ParamID::mode,  0, [] { return modeNames(); }, [] (const PresetValues& p, double) { return (float) (int) p.mode; });
        choice ("Sync L", ParamID::syncL, 0, [] { return syncNames(); }, [] (const PresetValues& p, double) { return (float) p.syncL; });
        num    ("Time L", ParamID::timeL, 0, 1, " ms", [] (const PresetValues& p, double bpm) { return (float) resolveTimeMs (p.syncL, p.timeL, bpm); });
        choice ("Sync R", ParamID::syncR, 0, [] { return syncNames(); }, [] (const PresetValues& p, double) { return (float) p.syncR; });
        num    ("Time R", ParamID::timeR, 0, 1, " ms", [] (const PresetValues& p, double bpm) { return (float) resolveTimeMs (p.syncR, p.timeR, bpm); });
        num    ("Feedback",  ParamID::feedback, 0, 1, " %",  [] (const PresetValues& p, double) { return p.feedback; });
        num    ("High Pass", ParamID::hp,       0, 0, " Hz", [] (const PresetValues& p, double) { return p.hp; });
        num    ("Low Pass",  ParamID::lp,       0, 0, " Hz", [] (const PresetValues& p, double) { return p.lp; });
        num    ("Mod Rate",  ParamID::rate,     0, 2, " Hz", [] (const PresetValues& p, double) { return p.rate; });
        num    ("Mod Depth", ParamID::depth,    0, 2, " ms", [] (const PresetValues& p, double) { return p.depth; });
        choice ("LFO Shape", ParamID::lfoShape, 0, [] { return lfoShapeNames(); }, [] (const PresetValues& p, double) { return (float) p.lfoShape; });
        num    ("Wet",       ParamID::wet,      0, 1, " dB", [] (const PresetValues& p, double) { return p.wet; });
        basicCount = (int) defs.size();

        // ---- 右列：Analog / FX ----
        choice ("Machine", ParamID::machine, 1, [] { return machineNames(); }, [] (const PresetValues& p, double) { return (float) p.machine; });
        num    ("Age",     ParamID::age,     1, 0, " %", [] (const PresetValues& p, double) { return p.age; });
        num    ("Drive",   ParamID::drive,   1, 0, " %", [] (const PresetValues& p, double) { return p.drive; });
        num    ("Wow",     ParamID::wow,     1, 0, " %", [] (const PresetValues& p, double) { return p.wow; });
        num    ("Flutter", ParamID::flutter, 1, 0, " %", [] (const PresetValues& p, double) { return p.flutter; });
        num    ("Pitch",      ParamID::pitch,     2, 0, " st", [] (const PresetValues& p, double) { return p.pitch; });
        num    ("Pitch Fine", ParamID::pitchFine, 2, 0, " ct", [] (const PresetValues& p, double) { return p.pitchFine; });
        choice ("Pitch Mode", ParamID::pitchMode, 2, [] { return pitchModeNames(); }, [] (const PresetValues& p, double) { return (float) p.pitchMode; });
        choice ("Reverse",    ParamID::reverse,   2, [] { return juce::StringArray { "Off", "On" }; }, [] (const PresetValues& p, double) { return p.reverse ? 1.0f : 0.0f; });
        num    ("Duck",         ParamID::duck,    2, 1, " dB", [] (const PresetValues& p, double) { return p.duck; });
        num    ("Duck Release", ParamID::duckRel, 2, 0, " ms", [] (const PresetValues& p, double) { return p.duckRel; });

        for (size_t i = (size_t) basicCount; i < (size_t) basicCount + 5; ++i)
            defs[i].machineGroup = true;

        for (size_t i = 0; i < defs.size(); ++i)
        {
            auto* c = cells.add (new Cell());
            c->setJustificationType (juce::Justification::centredLeft);
            c->setMinimumHorizontalScale (0.8f);
            c->setBorderSize ({ 0, 2, 0, 2 });
            if (defs[i].kind == Num)
            {
                c->setEditable (true, false, false);       // クリックで入力
                c->onTextChange = [this, i] { commitText (i); };
                c->setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
                c->onDragBegin = [this, i]
                {
                    const std::string id (defs[i].id);
                    if (id == ParamID::timeL) setValue (ParamID::syncL, 0.0f);       // ms を動かす時は同期を外す
                    if (id == ParamID::timeR) setValue (ParamID::syncR, 0.0f);
                    if (auto* p = apvts.getParameter (defs[i].id)) p->beginChangeGesture();
                };
                c->onDragBy = [this, i] (float d)
                {
                    if (auto* p = apvts.getParameter (defs[i].id))
                        p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, p->getValue() + d));
                };
                c->onDragEnd = [this, i] { if (auto* p = apvts.getParameter (defs[i].id)) p->endChangeGesture(); };
            }
            else
            {
                c->onChoiceClick = [this, i] { showChoiceMenu (i); };
                c->setInterceptsMouseClicks (true, false);
            }
            addChildComponent (c);
        }
        rebuildVisible();
    }

    void setLanguage (Lang l) { if (l != lang) { lang = l; repaint(); } }

    // 右パネルに置く時：1列・固定の行高。preferredHeight() で必要な高さを返す
    void setNarrow (bool n) { if (n != narrow) { narrow = n; layout(); } }
    int preferredHeight() const { return 12 + kNarrowRowH * ((int) vis.size() + 1); }

    void setView (int v)
    {
        v = juce::jlimit (0, 2, v);
        if (v == view) return;
        view = v;
        rebuildVisible();
        repaint();
    }

    // base = 選んだプリセットの値（ファクトリー／ユーザー）。name は表のタイトルに出す名前
    void update (const PresetValues& base, const juce::String& name, double bpmNow)
    {
        bpm = bpmNow;
        const auto& pr = base;
        presetName = name;
        const bool wetLocked = apvts.getRawParameterValue (ParamID::wetLock)->load() > 0.5f;

        bool dirty = false;
        for (size_t k = 0; k < vis.size(); ++k)
        {
            const auto& d = defs[(size_t) vis[k]];
            const float a = d.preset (pr, bpm);
            float b = apvts.getRawParameterValue (d.id)->load();
            if (std::string (d.id) == ParamID::timeL) b = (float) resolveTimeMs ((int) apvts.getRawParameterValue (ParamID::syncL)->load(), b, bpm);
            if (std::string (d.id) == ParamID::timeR) b = (float) resolveTimeMs ((int) apvts.getRawParameterValue (ParamID::syncR)->load(), b, bpm);

            Row r;
            if (d.kind == Num)
            {
                r.preset = juce::String (a, d.decimals) + d.unit;
                r.now    = juce::String (b, d.decimals) + d.unit;
                r.changed = r.preset != r.now;
            }
            else
            {
                const auto names = d.names();
                r.preset = names[juce::jlimit (0, names.size() - 1, (int) std::lround (a))];
                r.now    = names[juce::jlimit (0, names.size() - 1, (int) std::lround (b))];
                r.changed = (int) std::lround (a) != (int) std::lround (b);
            }
            if (pr.keepMachine && d.machineGroup)
            {
                r.preset = juce::String::fromUTF8 ("\xe2\x80\x94");     // —（機種は今のまま）
                r.changed = false;
            }
            if (wetLocked && std::string (d.id) == ParamID::wet)
            {
                r.preset = ui (UiText::Locked, lang);
                r.changed = false;
            }

            if (r != rows[k]) { rows[k] = r; dirty = true; }
            auto* c = cells[vis[k]];
            if (! c->isBeingEdited())
            {
                if (c->getText() != r.now) c->setText (r.now, juce::dontSendNotification);
                c->setColour (juce::Label::textColourId, r.changed ? theme::accent : theme::text);
            }
        }
        if (dirty) repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (theme::panel);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (theme::line);
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);

        int changedCount = 0;
        for (auto& row : rows) if (row.changed) ++changedCount;

        g.setFont (theme::ui (fs, true));
        g.setColour (changedCount > 0 ? theme::accent : theme::green);
        const auto status = changedCount > 0 ? juce::String (changedCount) + ui (UiText::Changed, lang)
                                             : ui (UiText::SameAsPreset, lang);
        g.drawText (status, titleR, juce::Justification::centredRight);
        g.setColour (theme::text);
        g.drawText (ui (UiText::ColPreset, lang) + presetName, titleR.withTrimmedRight (juce::roundToInt (fs * 6.0f)), juce::Justification::centredLeft);

        g.setFont (theme::ui (fs * 0.92f));
        for (auto& h : headR)
        {
            g.setColour (theme::faint);
            auto hr = h;
            g.drawText (ui (UiText::ColParam, lang), hr.removeFromLeft (nameW), juce::Justification::centredLeft);
            g.drawText (ui (UiText::ColPresetCol, lang), hr.removeFromLeft (presetW), juce::Justification::centredLeft);
            g.drawText (lang == Lang::Japanese ? jp ("現在値（入力可）") : juce::String ("Now (editable)"), hr, juce::Justification::centredLeft);
            g.setColour (theme::line);
            g.drawHorizontalLine (h.getBottom(), (float) h.getX(), (float) h.getRight());
        }

        for (size_t k = 0; k < vis.size() && k < geo.size(); ++k)
        {
            const auto& gm = geo[k];
            const auto& row = rows[k];
            if (row.changed)
            {
                g.setColour (theme::accent.withAlpha (0.14f));
                g.fillRect (gm.band.toFloat());
            }
            g.setFont (theme::ui (fs));
            g.setColour (theme::muted);
            g.drawText (defs[(size_t) vis[k]].label, gm.name, juce::Justification::centredLeft);
            g.setFont (theme::mono (fs * 0.95f));
            g.setColour (theme::text);
            g.drawFittedText (row.preset, gm.preset, juce::Justification::centredLeft, 1, 0.8f);
            // 入力できる欄にうっすら枠
            g.setColour (theme::line.withAlpha (0.7f));
            g.drawRoundedRectangle (gm.now.toFloat().reduced (0.5f, 1.0f), 3.0f, 1.0f);
        }
    }

    void resized() override { layout(); }

    // 点 p（この部品の座標）にある行のパラメーター ID。無ければ nullptr（説明表示用）
    const char* idAt (juce::Point<int> p) const
    {
        for (size_t k = 0; k < vis.size() && k < geo.size(); ++k)
            if (geo[k].band.contains (p)) return defs[(size_t) vis[k]].id;
        return nullptr;
    }

private:
    enum Kind { Choice, Num };

    struct Def
    {
        juce::String label, unit;
        const char* id = "";
        int minView = 0, decimals = 0;
        bool machineGroup = false;      // Machine / Age / Drive / Wow / Flutter（keepMachine のプリセットでは「—」）
        Kind kind = Num;
        std::function<juce::StringArray()> names;
        std::function<float (const PresetValues&, double)> preset;
    };

    struct Row
    {
        juce::String preset, now;
        bool changed = false;
        bool operator== (const Row& o) const { return preset == o.preset && now == o.now && changed == o.changed; }
        bool operator!= (const Row& o) const { return ! (*this == o); }
    };

    struct Geo { juce::Rectangle<int> band, name, preset, now; };

    // 「選択肢」の行は、クリックで選択メニューを出すラベル
    class Cell : public juce::Label
    {
    public:
        std::function<void()> onChoiceClick;
        // 数値セル：左右／上下にドラッグして値を変える（動かさずに離すと従来どおり数字入力）
        std::function<void()> onDragBegin, onDragEnd;
        std::function<void (float)> onDragBy;
        void mouseDown (const juce::MouseEvent& e) override
        {
            if (onChoiceClick) { onChoiceClick(); return; }
            dragging = false;  last = e.position;
            juce::Label::mouseDown (e);
        }
        void mouseDrag (const juce::MouseEvent& e) override
        {
            if (! onDragBy) { juce::Label::mouseDrag (e); return; }
            if (! dragging)
            {
                if (e.getDistanceFromDragStart() < 4) return;
                dragging = true;  last = e.position;
                if (onDragBegin) onDragBegin();
                return;
            }
            const auto d = e.position - last;
            last = e.position;
            onDragBy ((d.x - d.y) * (e.mods.isShiftDown() ? 0.0004f : 0.006f));
        }
        void mouseUp (const juce::MouseEvent& e) override
        {
            if (dragging) { dragging = false; if (onDragEnd) onDragEnd(); return; }
            juce::Label::mouseUp (e);
        }
    private:
        bool dragging = false;
        juce::Point<float> last;
    };

    void setValue (const char* id, float v)
    {
        if (auto* p = apvts.getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (v));
            p->endChangeGesture();
        }
    }

    void commitText (size_t i)
    {
        const auto& d = defs[i];
        const auto t = cells[(int) i]->getText();
        if (! t.containsAnyOf ("0123456789"))
            return;
        const float v = t.getFloatValue();
        if (std::string (d.id) == ParamID::timeL) setValue (ParamID::syncL, 0.0f);     // ms を直接入れたら同期は外す
        if (std::string (d.id) == ParamID::timeR) setValue (ParamID::syncR, 0.0f);
        setValue (d.id, v);
    }

    void showChoiceMenu (size_t i)
    {
        const auto& d = defs[i];
        const auto names = d.names();
        const int now = (int) std::lround (apvts.getRawParameterValue (d.id)->load());
        if (names.size() == 2 && std::string (d.id) == ParamID::reverse)      // ON/OFF は押すたびに切り替え
        {
            setValue (d.id, now == 0 ? 1.0f : 0.0f);
            return;
        }
        juce::PopupMenu m;
        for (int k = 0; k < names.size(); ++k)
            m.addItem (k + 1, names[k], true, k == now);
        const juce::String id (d.id);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (cells[(int) i]),
                         [this, id] (int result)
                         {
                             if (result > 0) setValue (id.toRawUTF8(), (float) (result - 1));
                         });
    }

    void rebuildVisible()
    {
        vis.clear();
        for (size_t i = 0; i < defs.size(); ++i)
        {
            const bool show = defs[i].minView <= view;
            cells[(int) i]->setVisible (show);
            if (show) vis.push_back ((int) i);
        }
        rows.assign (vis.size(), Row {});
        layout();
    }

    void layout()
    {
        auto area = getLocalBounds().reduced (10, 6);
        const int cols = (view == 0 || narrow) ? 1 : 2;
        int rowsInCol[2] = { 0, 0 };
        for (int idx : vis)
            ++rowsInCol[(cols == 1 || idx < basicCount) ? 0 : 1];
        const int maxRows = juce::jmax (rowsInCol[0], rowsInCol[1]);
        const int rowH = narrow ? kNarrowRowH : juce::jmax (11, area.getHeight() / (maxRows + 2));
        fs = juce::jmin (12.0f, (float) rowH * 0.74f);

        titleR = area.removeFromTop (rowH);
        const int gap = 14;
        const int colW = (area.getWidth() - gap * (cols - 1)) / cols;
        nameW = colW * 34 / 100;
        presetW = colW * 28 / 100;

        headR.clear();
        for (int c = 0; c < cols; ++c)
            headR.push_back ({ area.getX() + c * (colW + gap), area.getY(), colW, rowH });

        geo.assign (vis.size(), Geo {});
        int used[2] = { 0, 0 };
        for (size_t k = 0; k < vis.size(); ++k)
        {
            const int c = (cols == 1 || vis[k] < basicCount) ? 0 : 1;
            const int x0 = area.getX() + c * (colW + gap);
            const int y0 = area.getY() + rowH * (1 + used[c]++);
            Geo gm;
            gm.band = { x0 - 3, y0, colW + 6, rowH };
            gm.name   = { x0, y0, nameW, rowH };
            gm.preset = { x0 + nameW, y0, presetW, rowH };
            gm.now    = { x0 + nameW + presetW, y0, colW - nameW - presetW, rowH };
            geo[k] = gm;
            auto* cell = cells[vis[k]];
            cell->setBounds (gm.now.reduced (1, 1));
            cell->setFont (theme::mono (fs * 0.95f));
        }
        repaint();
    }

    juce::AudioProcessorValueTreeState& apvts;
    std::vector<Def> defs;
    juce::OwnedArray<Cell> cells;
    std::vector<int> vis;
    std::vector<Row> rows;
    std::vector<Geo> geo;
    std::vector<juce::Rectangle<int>> headR;
    juce::Rectangle<int> titleR;
    static constexpr int kNarrowRowH = 20;
    bool narrow = false;
    int basicCount = 11, view = 2, nameW = 80, presetW = 70, shownPreset = 0;
    float fs = 11.0f;
    double bpm = 120.0;
    juce::String presetName;
    Lang lang = Lang::Japanese;

public:
    void setShownPreset (int i) { shownPreset = i; }
};
