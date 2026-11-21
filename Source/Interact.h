#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Params.h"
#include "Theme.h"
#include <atomic>

// =============================================================================
//  図の上で直接ドラッグできる「ハンドル」の共通部品
//   ・HandleView を継承した図は、paint() の中で addHandle() するだけでよい
//   ・ドラッグ中は仮想位置を積算（Shift で 1/25 の微調整）。つかんだ位置からは跳ばない
//   ・ダブルクリックで、そのハンドルのパラメーターを初期値に戻す
//   ・操作はパラメーターのジェスチャーとしてホストに伝わる（オートメーション記録／Undo 対応）
// =============================================================================
inline std::atomic<int>& handleDragCount() { static std::atomic<int> n { 0 }; return n; }

struct ParamIO
{
    juce::AudioProcessorValueTreeState* apvts = nullptr;

    float get (const char* id) const { return apvts->getRawParameterValue (id)->load(); }

    void begin (const juce::String& id) const
    {
        if (auto* p = apvts->getParameter (id)) p->beginChangeGesture();
    }
    void end (const juce::String& id) const
    {
        if (auto* p = apvts->getParameter (id)) p->endChangeGesture();
    }
    void set (const char* id, float v) const
    {
        if (auto* p = apvts->getParameter (id))
        {
            const auto r = p->getNormalisableRange();
            p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (r.start, r.end, v)));
        }
    }
    // テンポ同期が入っていたら外す（図でミリ秒を直接動かす時）
    void releaseSync (const char* syncId) const
    {
        if (auto* p = apvts->getParameter (syncId))
            if (p->getValue() > 0.0f)
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost (0.0f);
                p->endChangeGesture();
            }
    }
    void resetToDefault (const juce::String& id) const
    {
        if (auto* p = apvts->getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->getDefaultValue());
            p->endChangeGesture();
        }
    }
};

struct Handle
{
    juce::Point<float> pos;
    juce::String tip;                                                    // 触っている間に出す「名前 = 値」
    std::function<void (juce::Point<float>, const juce::MouseEvent&)> drag;   // 仮想位置 → パラメーター
    std::vector<juce::String> ids;                                       // ジェスチャー／初期値に戻す対象
    std::function<void()> onGrab;                                        // つかんだ瞬間（同期を外すなど）
    bool ghost = false;                                                  // 薄く描く（いまは効いていない位置の目印）
};

class HandleView : public juce::Component
{
public:
    explicit HandleView (juce::AudioProcessorValueTreeState& a) { io.apvts = &a; }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const int h = nearest (e.position);
        if (h != hover)
        {
            hover = h;
            setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
            repaint();
        }
    }
    void mouseExit (const juce::MouseEvent&) override
    {
        if (hover >= 0 && ! dragging) { hover = -1; repaint(); }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int h = nearest (e.position);
        if (h < 0 || e.mods.isPopupMenu()) return;
        grabbed = handles[(size_t) h];
        active = h;
        if (grabbed.onGrab) grabbed.onGrab();
        for (auto& id : grabbed.ids) io.begin (id);
        dragging = true;
        ++handleDragCount();
        virt = grabbed.pos;
        lastRaw = e.position;
        dragStateChanged (true);
        repaint();
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! dragging) return;
        const auto d = e.position - lastRaw;
        lastRaw = e.position;
        virt += d * (e.mods.isShiftDown() ? 0.04f : 1.0f);
        if (grabbed.drag) grabbed.drag (virt, e);
    }
    void mouseUp (const juce::MouseEvent&) override
    {
        if (! dragging) return;
        for (auto& id : grabbed.ids) io.end (id);
        dragging = false;
        --handleDragCount();
        active = -1;
        dragStateChanged (false);
        repaint();
    }
    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        const int h = nearest (e.position);
        if (h < 0) return;
        for (auto& id : handles[(size_t) h].ids) io.resetToDefault (id);
    }

    // いまマウスが乗っている●が操作するパラメーター（説明表示用）。無ければ空
    juce::String hoveredParamId() const
    {
        const int h = dragging ? active : hover;
        if (h >= 0 && h < (int) handles.size() && ! handles[(size_t) h].ids.empty())
            return handles[(size_t) h].ids[0];
        return {};
    }

protected:
    virtual void dragStateChanged (bool) {}

    void beginPaint() { handles.clear(); }
    void addHandle (Handle h) { handles.push_back (std::move (h)); }

    void drawHandles (juce::Graphics& g) const
    {
        for (size_t i = 0; i < handles.size(); ++i)
        {
            const auto& h = handles[i];
            const bool on = ((int) i == hover) || ((int) i == active);
            const float r = on ? 7.0f : 5.0f;
            const float a = h.ghost && ! on ? 0.5f : 1.0f;
            g.setColour (theme::well.withAlpha (a));
            g.fillEllipse (h.pos.x - r - 1.5f, h.pos.y - r - 1.5f, (r + 1.5f) * 2.0f, (r + 1.5f) * 2.0f);
            g.setColour (on ? theme::accent : theme::accent.withAlpha (0.85f * a));
            g.fillEllipse (h.pos.x - r, h.pos.y - r, r * 2.0f, r * 2.0f);
            if (! on)
            {
                g.setColour (theme::onAccent.withAlpha (0.8f * a));
                g.fillEllipse (h.pos.x - 1.5f, h.pos.y - 1.5f, 3.0f, 3.0f);
            }
        }
        for (size_t i = 0; i < handles.size(); ++i)
        {
            const bool on = ((int) i == hover) || ((int) i == active);
            if (! on || handles[i].tip.isEmpty()) continue;
            const auto& h = handles[i];
            const auto f = theme::mono (10.5f, true);
            const int w = juce::GlyphArrangement::getStringWidthInt (f, h.tip) + 12;
            float x = h.pos.x + 11.0f, y = h.pos.y - 24.0f;
            if (x + (float) w > (float) getWidth() - 4.0f) x = h.pos.x - 11.0f - (float) w;
            if (y < 2.0f) y = h.pos.y + 12.0f;
            const juce::Rectangle<float> box (x, y, (float) w, 17.0f);
            g.setColour (theme::panel.withAlpha (0.95f));
            g.fillRoundedRectangle (box, 4.0f);
            g.setColour (theme::accent);
            g.drawRoundedRectangle (box.reduced (0.5f), 4.0f, 1.0f);
            g.setColour (theme::text);
            g.setFont (f);
            g.drawText (h.tip, box.toNearestInt(), juce::Justification::centred);
        }
    }

    ParamIO io;
    std::vector<Handle> handles;
    int hover = -1, active = -1;
    bool dragging = false;

private:
    int nearest (juce::Point<float> p) const
    {
        int best = -1;
        float bestD = 13.0f * 13.0f;
        for (size_t i = 0; i < handles.size(); ++i)
        {
            const float dx = handles[i].pos.x - p.x, dy = handles[i].pos.y - p.y;
            const float d = dx * dx + dy * dy;
            if (d <= bestD) { bestD = d; best = (int) i; }
        }
        return best;
    }

    Handle grabbed;
    juce::Point<float> virt, lastRaw;
};
