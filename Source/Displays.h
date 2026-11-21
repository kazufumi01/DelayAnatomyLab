#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "DelayEngine.h"
#include "Machines.h"
#include "Localisation.h"
#include "Theme.h"
#include "Interact.h"

// =============================================================================
//  図の窓：TIMELINE ／ 原理図 ／ リピート特性
//  どれも「音を解析した結果」ではなく、いまのパラメーターから理論値を描いている。
// =============================================================================
// UTF-8 の文字列リテラルを String にする（ASCII 前提の const char* 連結だと文字化けするため）
inline juce::String operator""_u (const char* s, size_t n) { return juce::String::fromUTF8 (s, (int) n); }

namespace dispx
{
    inline juce::String J (const char* s) { return juce::String::fromUTF8 (s); }

    struct Info
    {
        DelaySettings s;
        double bpm = 120.0, fs = 48000.0;
        Lang lang = Lang::Japanese;
        juce::String L (const char* ja, const char* en) const { return lang == Lang::Japanese ? J (ja) : juce::String (en); }
    };

    inline juce::String signedStr (float v, int dec = 0)
    {
        return (v > 0 ? "+" : "") + juce::String (v, dec);
    }

    // 1回通るたびの周波数特性（dB）：HP / LP（バターワース2次）＋機種のローパス
    inline float passDb (float f, const DelaySettings& s)
    {
        const float hpc = std::pow (s.hpHz / f, 4.0f), lpc = std::pow (f / s.lpHz, 4.0f);
        float db = -10.0f * std::log10 (1.0f + hpc) - 10.0f * std::log10 (1.0f + lpc);
        const int st = machineLpStages (s.machine);
        if (st > 0)
        {
            const float mc = std::pow (f / machineLpHz (s.machine, s.age, s.timeLms), 4.0f);
            db += -10.0f * std::log10 (1.0f + mc) * (float) st;
        }
        return db;
    }
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//  TIMELINE：理論上のエコーの並び。●をドラッグ
//    1つ目の棒  … 横＝Time（L / R）、縦＝Wet
//    2つ目の棒  … 縦＝Feedback（ピンポンでは横＝Time R）
//    Cmd を押しながら横に動かすと 1/16 音符のグリッドに吸着
// -----------------------------------------------------------------------------
class TimelineView : public HandleView
{
public:
    explicit TimelineView (juce::AudioProcessorValueTreeState& a) : HandleView (a) {}
    void update (const dispx::Info& i) { info = i; repaint(); }

    void paint (juce::Graphics& g) override
    {
        using namespace dispx;
        beginPaint();
        const auto& s = info.s;
        auto area = getLocalBounds().toFloat();
        g.setColour (theme::well);
        g.fillRoundedRectangle (area, 6.0f);
        g.setColour (theme::line);
        g.drawRoundedRectangle (area.reduced (0.5f), 6.0f, 1.0f);

        struct Tap { float t, a; int lane; int n; float span; bool minor; };
        std::vector<Tap> taps;
        const float wet = s.wetGain, fb = s.feedback;
        const float lim = 0.0025f;
        const bool space = (s.machine == Machine::SpaceEcho);

        auto addSeries = [&] (float step, int lane)
        {
            float a = wet, t = step;
            for (int n = 1; n < 40 && t < 6000.0f; ++n)
            {
                taps.push_back ({ t, a, lane, n, s.reverse ? step : 0.0f, false });
                if (space)
                {
                    taps.push_back ({ t - step * (2.0f / 3.0f) + step, a * 0.30f, lane, 0, 0.0f, true });
                    taps.back().t = t - step * (1.0f / 3.0f);
                    taps.push_back ({ t - step * (2.0f / 3.0f), a * 0.20f, lane, 0, 0.0f, true });
                }
                a *= fb;  t += step;
                if (a < lim) break;
            }
        };

        switch (s.mode)
        {
            case DelayMode::Haas:   taps.push_back ({ s.timeRms, wet, 1, 1, s.reverse ? s.timeRms : 0.0f, false }); break;
            case DelayMode::Mono:   addSeries (s.timeLms, 2); break;
            case DelayMode::Stereo: addSeries (s.timeLms, 0); addSeries (s.timeRms, 1); break;
            case DelayMode::PingPong:
            {
                float a = wet, t = s.timeLms;
                for (int n = 1; n < 40 && t < 6000.0f; ++n)
                {
                    const float st = (n % 2) ? s.timeRms : s.timeLms;
                    taps.push_back ({ t, a, (n % 2) ? 0 : 1, n, s.reverse ? st : 0.0f, false });
                    a *= fb;  t += st;
                    if (a < lim) break;
                }
                break;
            }
        }

        float maxT = 0.0f;
        for (auto& tp : taps)
            if (tp.a > juce::Decibels::decibelsToGain (-36.0f))
                maxT = juce::jmax (maxT, tp.t);
        if (maxT <= 0.0f && ! taps.empty()) maxT = taps.front().t;
        const float winAuto = juce::jlimit (60.0f, 4000.0f, maxT * 1.18f);
        const float win = frozenWin > 0.0f ? frozenWin : winAuto;       // ドラッグ中は窓を固定（つかんだ棒が逃げない）
        lastWin = win;

        const float padL = 30.0f, padR = 12.0f, top = 22.0f, bottom = 24.0f;
        const float laneH = (area.getHeight() - top - bottom) / 2.0f;
        const float plotW = area.getWidth() - padL - padR;
        auto x = [&] (float t) { return padL + plotW * t / win; };

        g.setFont (theme::mono (10.0f));
        g.setColour (theme::onWell());
        g.drawText ("L", 6, (int) (top + laneH / 2 - 7), 14, 14, juce::Justification::centred);
        g.drawText ("R", 6, (int) (top + laneH * 1.5f - 7), 14, 14, juce::Justification::centred);
        g.setColour (theme::line);
        g.drawHorizontalLine ((int) (top + laneH + 2), padL, area.getWidth() - padR);

        // ヘッダー行：機種・FXの状態
        {
            juce::String hdr = "MACHINE " + juce::String (machineNames()[(int) s.machine]);
            if (s.machine != Machine::Clean)
            {
                float lo, hi; machineTimeRange (s.machine, lo, hi);
                hdr += "  (Time " + juce::String ((int) lo) + "-" + juce::String ((int) hi) + " ms)";
            }
            if (s.reverse) hdr += "   REVERSE";
            if (std::abs (s.pitchSemis) > 0.001f)
                hdr += "   PITCH " + signedStr (s.pitchSemis, std::abs (s.pitchSemis - std::round (s.pitchSemis)) > 0.01f ? 2 : 0) + "st " + (s.pitchInLoop ? "In Loop" : "Output");
            if (s.duckDb > 0.05f) hdr += "   DUCK -" + juce::String (s.duckDb, 1) + "dB / " + juce::String ((int) s.duckRelMs) + "ms";
            g.setFont (theme::mono (10.0f, true));
            g.setColour (theme::accent);
            g.drawText (hdr, 10, 3, (int) area.getWidth() - 20, 16, juce::Justification::centredLeft);
        }

        const float beat = (float) (60000.0 / juce::jmax (20.0, info.bpm));
        if (win > beat * 0.6f)
        {
            for (int k = 1; beat / 4 * k < win; ++k)
            {
                const bool isBeat = (k % 4 == 0);
                if (! isBeat && win > beat * 3) continue;
                g.setColour (theme::accent.withAlpha (isBeat ? 0.4f : 0.18f));
                g.drawVerticalLine ((int) x (beat / 4 * k), top, area.getHeight() - bottom);
            }
        }

        const float steps[] = { 1, 2, 5, 10, 20, 50, 100, 200, 250, 500, 1000 };
        float step = 1000.0f;
        for (auto st : steps) if (win / st <= 8.0f) { step = st; break; }
        g.setColour (theme::onWell());
        g.setFont (theme::mono (10.0f));
        for (float t = 0.0f; t <= win; t += step)
            g.drawText (juce::String ((int) t) + "ms",
                        juce::jlimit (0, (int) area.getWidth() - 54, (int) x (t) - 25), (int) (area.getHeight() - 18), 50, 14,
                        juce::Justification::centred);

        auto ampToY = [top, laneH] (float a, int lane)
        {
            const float db = juce::Decibels::gainToDecibels (juce::jmax (a, 1.0e-4f));
            const float h  = juce::jmax (2.0f, laneH * juce::jmax (0.0f, (db + 42.0f) / 42.0f));
            return top + (lane == 1 ? laneH + 4.0f : 0.0f) + laneH - h;
        };
        auto bar = [&] (float t, float a, int lane, juce::Colour c, int n, float span, const juce::String& tag)
        {
            const float db = juce::Decibels::gainToDecibels (juce::jmax (a, 1.0e-4f));
            const float h  = juce::jmax (2.0f, laneH * juce::jmax (0.0f, (db + 42.0f) / 42.0f));
            const float y0 = top + (lane == 1 ? laneH + 4.0f : 0.0f) + laneH - h;
            g.setColour (c);
            if (span > 0.0f)                          // Reverse：小さく始まって、最後が一番大きい
            {
                juce::Path p;
                const float xe = x (t), xs = x (juce::jmax (0.0f, t - span));
                p.startNewSubPath (xs, y0 + h);  p.lineTo (xe, y0);  p.lineTo (xe, y0 + h);  p.closeSubPath();
                g.fillPath (p);
            }
            else
                g.fillRect (x (t) - 4.0f, y0, 8.0f, h);
            if (n > 0 && n < 10 && h > 14.0f)
            {
                g.setColour (juce::Colour (0xff0b0c0e));
                g.setFont (theme::mono (10.0f));
                g.drawText (juce::String (n), (int) (x (t) - 4), (int) y0 + 1, 8, 11, juce::Justification::centred);
            }
            if (tag.isNotEmpty() && h > 10.0f)
            {
                g.setColour (theme::accent);
                g.setFont (theme::mono (9.0f, true));
                g.drawText (tag, (int) (x (t) - 20), (int) y0 - 11, 40, 10, juce::Justification::centred);
            }
        };

        if (! s.wetOnly)
        {
            bar (0.0f, 1.0f, 0, theme::onWell(), 0, 0.0f, {});
            if (s.mode != DelayMode::Haas)
                bar (0.0f, 1.0f, 1, theme::onWell(), 0, 0.0f, {});
        }

        if (! s.delayOff)
        {
            auto shadeK = [&] (int n)
            {
                const float h  = 1.0f / std::sqrt (1.0f + std::pow (4000.0f / s.lpHz, 4.0f));
                const float hh = 1.0f / std::sqrt (1.0f + std::pow (s.hpHz / 250.0f, 4.0f));
                float m = 1.0f;
                const int st = machineLpStages (s.machine);
                if (st > 0)
                    m = std::pow (1.0f / std::sqrt (1.0f + std::pow (4000.0f / machineLpHz (s.machine, s.age, s.timeLms), 4.0f)), (float) st);
                return 0.25f + 0.75f * std::pow (h * hh * m, (float) n);
            };
            const bool pitched = std::abs (s.pitchSemis) > 0.001f;
            const auto dark = theme::wellIsBright() ? theme::line : theme::zero;
            for (auto& tp : taps)
            {
                if (tp.t > win) continue;
                const float k = tp.minor ? 0.4f : shadeK (tp.n);
                juce::String tag;
                if (pitched && tp.n > 0 && tp.n <= 5)
                    tag = signedStr (s.pitchInLoop ? s.pitchSemis * (float) tp.n : s.pitchSemis, 0);
                if (tp.lane == 0 || tp.lane == 2)
                    bar (tp.t, tp.a, 0, dark.interpolatedWith (theme::laneL, k), tp.n, tp.span, tag);
                if (tp.lane == 1 || tp.lane == 2)
                    bar (tp.t, tp.a, 1, dark.interpolatedWith (theme::laneR, k), tp.n, tp.span, tp.lane == 1 ? tag : juce::String());
            }
        }

        // ---------------------------------------------------------------- ハンドル
        {
            const auto mach  = s.machine;
            const float wetDb = juce::Decibels::gainToDecibels (juce::jmax (s.wetGain, 1.0e-4f));
            const float fbShown = juce::jmax (fb, 0.05f);
            const bool  ghostFb = fb < 0.005f;
            const float beatQ = beat / 4.0f;
            auto msFromX = [padL, plotW, win] (float px) { return (px - padL) / plotW * win; };
            auto snapIf  = [beatQ] (float ms, const juce::MouseEvent& e) { return e.mods.isCommandDown() ? std::round (ms / beatQ) * beatQ : ms; };
            auto yToDb   = [top, laneH] (float y, int lane)
            {
                const float bottomY = top + (lane == 1 ? laneH + 4.0f : 0.0f) + laneH;
                return (bottomY - y) / laneH * 42.0f - 42.0f;
            };
            auto clampX = [padL, plotW] (float px) { return juce::jmin (px, padL + plotW - 6.0f); };

            // 1つ目の棒：横＝Time、縦＝Wet
            auto timeHandle = [&] (int lane, float tMs, const char* timeId, const char* syncId, const juce::String& name)
            {
                Handle h;
                h.pos = { clampX (x (tMs)), ampToY (s.wetGain, lane) };
                h.tip = name + " " + juce::String (tMs, 1) + " ms   Wet " + juce::String (wetDb, 1) + " dB";
                h.ids.push_back (timeId);
                h.ids.push_back (ParamID::wet);
                h.onGrab = [this, syncId] { io.releaseSync (syncId); };
                h.drag = [this, msFromX, snapIf, mach, timeId, lane, yToDb] (juce::Point<float> p, const juce::MouseEvent& e)
                {
                    io.set (timeId, machineClampTime (mach, snapIf (msFromX (p.x), e)));
                    io.set (ParamID::wet, juce::jlimit (-30.0f, 0.0f, yToDb (p.y, lane)));
                };
                addHandle (std::move (h));
            };
            // 2つ目の棒：縦＝Feedback
            auto fbHandle = [&] (int lane, float tMs, float prevAmp)
            {
                Handle h;
                h.pos = { clampX (x (tMs)), ampToY (prevAmp * fbShown, lane) };
                h.tip = "Feedback " + juce::String (fb * 100.0f, 1) + " %";
                h.ghost = ghostFb;
                h.ids.push_back (ParamID::feedback);
                h.drag = [this, yToDb, lane, wetDb] (juce::Point<float> p, const juce::MouseEvent&)
                {
                    const float v = std::pow (10.0f, (yToDb (p.y, lane) - wetDb) / 20.0f);
                    io.set (ParamID::feedback, juce::jlimit (0.0f, 0.95f, v) * 100.0f);
                };
                addHandle (std::move (h));
            };

            switch (s.mode)
            {
                case DelayMode::Haas:
                    timeHandle (1, s.timeRms, ParamID::timeR, ParamID::syncR, "Time R");
                    break;
                case DelayMode::Mono:
                    timeHandle (0, s.timeLms, ParamID::timeL, ParamID::syncL, "Time");
                    fbHandle (0, s.timeLms * 2.0f, s.wetGain);
                    break;
                case DelayMode::Stereo:
                    timeHandle (0, s.timeLms, ParamID::timeL, ParamID::syncL, "Time L");
                    timeHandle (1, s.timeRms, ParamID::timeR, ParamID::syncR, "Time R");
                    fbHandle (0, s.timeLms * 2.0f, s.wetGain);
                    break;
                case DelayMode::PingPong:
                {
                    timeHandle (0, s.timeLms, ParamID::timeL, ParamID::syncL, "Time L");
                    Handle h;                                   // 2つ目の棒：横＝Time R、縦＝Feedback
                    const float t2 = s.timeLms + s.timeRms;
                    h.pos = { clampX (x (t2)), ampToY (s.wetGain * fbShown, 1) };
                    h.tip = "Time R " + juce::String (s.timeRms, 1) + " ms   Feedback " + juce::String (fb * 100.0f, 1) + " %";
                    h.ghost = ghostFb;
                    h.ids.push_back (ParamID::timeR);
                    h.ids.push_back (ParamID::feedback);
                    h.onGrab = [this] { io.releaseSync (ParamID::syncR); };
                    const float t1 = s.timeLms;
                    h.drag = [this, msFromX, snapIf, mach, t1, yToDb, wetDb] (juce::Point<float> p, const juce::MouseEvent& e)
                    {
                        io.set (ParamID::timeR, machineClampTime (mach, snapIf (msFromX (p.x), e) - t1));
                        const float v = std::pow (10.0f, (yToDb (p.y, 1) - wetDb) / 20.0f);
                        io.set (ParamID::feedback, juce::jlimit (0.0f, 0.95f, v) * 100.0f);
                    };
                    addHandle (std::move (h));
                    break;
                }
            }
        }
        drawHandles (g);
    }

protected:
    void dragStateChanged (bool d) override { frozenWin = d ? lastWin : 0.0f; }

private:
    dispx::Info info;
    float frozenWin = 0.0f, lastWin = 0.0f;
};

// -----------------------------------------------------------------------------
//  リピート特性：1回目・2回目・3回目…の周波数特性。●をドラッグ
//    High Pass / Low Pass の肩 … 横＝周波数
//    機種のローパスの肩        … 横＝Age（機種の高域ロス）
//    2回目の曲線               … 縦＝Feedback
// -----------------------------------------------------------------------------
class RepeatView : public HandleView
{
public:
    explicit RepeatView (juce::AudioProcessorValueTreeState& a) : HandleView (a) {}
    void update (const dispx::Info& i) { info = i; repaint(); }

    void paint (juce::Graphics& g) override
    {
        using namespace dispx;
        beginPaint();
        const auto& s = info.s;
        auto area = getLocalBounds().toFloat();
        g.setColour (theme::well);
        g.fillRoundedRectangle (area, 6.0f);
        g.setColour (theme::line);
        g.drawRoundedRectangle (area.reduced (0.5f), 6.0f, 1.0f);

        g.setFont (theme::mono (10.0f, true));
        g.setColour (theme::accent);
        g.drawText (info.L ("リピート特性", "REPEATS"), 8, 1, 200, 13, juce::Justification::centredLeft);

        const float padL = 40.0f, padR = 14.0f, top = 16.0f, bottom = 64.0f;
        const auto plot = juce::Rectangle<float> (padL, top, area.getWidth() - padL - padR, area.getHeight() - top - bottom);
        const float fLo = 20.0f, fHi = 20000.0f, dbTop = 6.0f, dbBot = -60.0f;
        auto xf = [&] (float f) { return plot.getX() + plot.getWidth() * std::log (f / fLo) / std::log (fHi / fLo); };
        auto yd = [&] (float db) { return plot.getY() + plot.getHeight() * (dbTop - juce::jlimit (dbBot, dbTop, db)) / (dbTop - dbBot); };

        g.setFont (theme::mono (9.5f));
        for (float f : { 20.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f, 20000.f })
        {
            g.setColour (theme::grid.interpolatedWith (theme::line, 0.5f));
            g.drawVerticalLine ((int) xf (f), plot.getY(), plot.getBottom());
            if (f == 20.f || f == 50.f || f == 200.f || f == 2000.f) continue;      // 幅が狭いので間引く
            g.setColour (theme::onWell());
            g.drawText (f >= 1000.f ? juce::String (f / 1000.f, f == 20000.f ? 0 : (f == 1000.f || f == 2000.f || f == 5000.f || f == 10000.f ? 0 : 1)) + "k" : juce::String ((int) f),
                        (int) xf (f) - 20, (int) plot.getBottom() + 3, 40, 12, juce::Justification::centred);
        }
        for (float db : { 0.f, -12.f, -24.f, -36.f, -48.f, -60.f })
        {
            g.setColour (db == 0.f ? theme::zero : theme::grid.interpolatedWith (theme::line, 0.5f));
            g.drawHorizontalLine ((int) yd (db), plot.getX(), plot.getRight());
            g.setColour (theme::onWell());
            g.drawText (juce::String ((int) db), 2, (int) yd (db) - 6, (int) padL - 6, 12, juce::Justification::centredRight);
        }

        // 何回目まで描くか：Feedback 0% なら 1 回だけ
        const bool single = (s.mode == DelayMode::Haas) || s.feedback < 0.005f;
        int count = 1;
        if (! single)
        {
            for (count = 1; count < 8; ++count)
                if (20.0f * std::log10 (std::pow (s.feedback, (float) count)) < -54.0f) break;
            count = juce::jmax (2, count);
        }

        const int npts = 200;
        for (int n = count; n >= 1; --n)
        {
            const float lvl = 20.0f * std::log10 (juce::jmax (1.0e-6f, std::pow (juce::jmax (1.0e-6f, s.feedback), (float) (n - 1))));
            juce::Path p;
            for (int k = 0; k <= npts; ++k)
            {
                const float f = fLo * std::pow (fHi / fLo, (float) k / (float) npts);
                const float db = lvl + (float) n * passDb (f, s);
                const float px = xf (f), py = yd (db);
                if (k == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
            }
            const float k = count > 1 ? (float) (n - 1) / (float) (count - 1) : 0.0f;
            g.setColour (theme::accent.interpolatedWith (theme::wellIsBright() ? theme::muted : theme::faint, k).withAlpha (1.0f - 0.35f * k));
            g.strokePath (p, juce::PathStrokeType (n == 1 ? 2.6f : 1.8f));
            const float lf = 8000.0f;
            g.setFont (theme::mono (10.0f, true));
            g.drawText (juce::String (n), (int) xf (lf) - 6, (int) yd (lvl + (float) n * passDb (lf, s)) - 14, 12, 12, juce::Justification::centred);
        }

        // 注釈（短く）
        g.setFont (theme::ui (10.5f));
        g.setColour (theme::onWellText());
        juce::String note;
        if (single)
            note = s.mode == DelayMode::Haas ? info.L ("Haas：フィードバックなし、エコーは1回だけ。", "Haas: no feedback, a single echo.")
                                             : info.L ("Feedback 0%：曲線は1本。薄い●を上へ引くと2回目が現れる。", "Feedback 0%: one curve. Drag the faint dot up to add repeat 2.");
        else
            note = info.L ("n回目は (Feedback)^(n-1) で小さく、フィルターも n 回掛かる → 狭く・暗くなる。",
                           "Repeat n is (Feedback)^(n-1) quieter and filtered n times: narrower, darker.");
        if (s.machine != Machine::Clean && machineLpStages (s.machine) > 0)
            note += "  [" + juce::String (machineNames()[(int) s.machine]) + " LP ≈ "_u + juce::String (juce::roundToInt (machineLpHz (s.machine, s.age, s.timeLms))) + " Hz]";
        g.drawFittedText (note, juce::Rectangle<int> (8, (int) (area.getHeight() - 36), (int) area.getWidth() - 16, 32),
                          juce::Justification::topLeft, 3, 0.9f);

        // ---------------------------------------------------------------- ハンドル
        {
            auto fFromX  = [plot, fLo, fHi] (float px) { return fLo * std::pow (fHi / fLo, juce::jlimit (0.0f, 1.0f, (px - plot.getX()) / plot.getWidth())); };
            auto dbFromY = [plot, dbTop, dbBot] (float py) { return dbTop - (py - plot.getY()) / plot.getHeight() * (dbTop - dbBot); };

            {   // High Pass
                Handle h;
                h.pos = { xf (s.hpHz), yd (passDb (s.hpHz, s)) };
                h.tip = "High Pass " + juce::String (juce::roundToInt (s.hpHz)) + " Hz";
                h.ids.push_back (ParamID::hp);
                h.drag = [this, fFromX] (juce::Point<float> p, const juce::MouseEvent&) { io.set (ParamID::hp, fFromX (p.x)); };
                addHandle (std::move (h));
            }
            {   // Low Pass
                Handle h;
                h.pos = { xf (s.lpHz), yd (passDb (s.lpHz, s)) };
                h.tip = "Low Pass " + juce::String (juce::roundToInt (s.lpHz)) + " Hz";
                h.ids.push_back (ParamID::lp);
                h.drag = [this, fFromX] (juce::Point<float> p, const juce::MouseEvent&) { io.set (ParamID::lp, fFromX (p.x)); };
                addHandle (std::move (h));
            }
            if (machineLpStages (s.machine) > 0)
            {   // 機種のローパス → Age
                const float fm = machineLpHz (s.machine, s.age, s.timeLms);
                Handle h;
                h.pos = { xf (juce::jlimit (fLo, fHi, fm)), yd (passDb (fm, s)) };
                h.tip = "Age " + juce::String (s.age, 0) + " %   LP " + juce::String (juce::roundToInt (fm)) + " Hz";
                h.ids.push_back (ParamID::age);
                const auto mc = s.machine;
                const float T = s.timeLms;
                h.drag = [this, fFromX, mc, T] (juce::Point<float> p, const juce::MouseEvent&)
                {
                    const float f = fFromX (p.x);
                    float lo = 0.0f, hi = 100.0f, age;
                    if (f >= machineLpHz (mc, 0.0f, T))        age = 0.0f;
                    else if (f <= machineLpHz (mc, 100.0f, T)) age = 100.0f;
                    else
                    {
                        for (int i = 0; i < 24; ++i)
                        {
                            const float mid = 0.5f * (lo + hi);
                            if (machineLpHz (mc, mid, T) > f) lo = mid; else hi = mid;
                        }
                        age = 0.5f * (lo + hi);
                    }
                    io.set (ParamID::age, age);
                };
                addHandle (std::move (h));
            }
            if (s.mode != DelayMode::Haas)
            {   // Feedback：2回目の曲線の高さ（1 kHz の位置）
                const float fb = juce::jmax (s.feedback, 0.05f);
                const float f1k = 1000.0f;
                const float pd = passDb (f1k, s);
                Handle h;
                h.pos = { xf (f1k), yd (20.0f * std::log10 (fb) + 2.0f * pd) };
                h.tip = "Feedback " + juce::String (s.feedback * 100.0f, 1) + " %";
                h.ghost = s.feedback < 0.005f;
                h.ids.push_back (ParamID::feedback);
                h.drag = [this, dbFromY, pd] (juce::Point<float> p, const juce::MouseEvent&)
                {
                    const float v = std::pow (10.0f, (dbFromY (p.y) - 2.0f * pd) / 20.0f);
                    io.set (ParamID::feedback, juce::jlimit (0.0f, 0.95f, v) * 100.0f);
                };
                addHandle (std::move (h));
            }
        }
        drawHandles (g);
    }

private:
    dispx::Info info;
};


// -----------------------------------------------------------------------------
//  原理図：機種ごとの仕組みのアニメーション ＋「原理を数字で確かめる」
// -----------------------------------------------------------------------------
class DiagramView : public HandleView
{
public:
    explicit DiagramView (juce::AudioProcessorValueTreeState& a) : HandleView (a) {}
    void update (const dispx::Info& i) { info = i; }
    void tick() { phase += 1.0f / 30.0f; repaint(); }

    void paint (juce::Graphics& g) override
    {
        using namespace dispx;
        const auto& s = info.s;
        auto area = getLocalBounds().toFloat();
        g.setColour (theme::well);
        g.fillRoundedRectangle (area, 6.0f);
        g.setColour (theme::line);
        g.drawRoundedRectangle (area.reduced (0.5f), 6.0f, 1.0f);

        beginPaint();
        auto inner = getLocalBounds().reduced (12, 8);
        auto ctl = inner.removeFromBottom (juce::jmin (112, inner.getHeight() / 2));
        inner.removeFromBottom (6);
        drawSchematic (g, inner.toFloat());
        drawControls (g, ctl.toFloat());
        drawHandles (g);
    }

private:
    dispx::Info info;
    float phase = 0.0f;

    // ---- 小道具 ----
    static void arrow (juce::Graphics& g, juce::Point<float> a, juce::Point<float> b)
    {
        g.drawArrow (juce::Line<float> (a, b), 1.5f, 7.0f, 6.0f);
    }
    void box (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, bool hot = false) const
    {
        g.setColour (hot ? theme::accent.withAlpha (0.25f) : theme::panel);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (hot ? theme::accent : theme::line);
        g.drawRoundedRectangle (r, 4.0f, 1.2f);
        g.setColour (theme::text);                       // 箱の地は theme::panel なので、文字は theme::text（明るい配色でも見える）
        g.setFont (theme::mono (10.5f, true));
        g.drawFittedText (t, r.reduced (3.0f).toNearestInt(), juce::Justification::centred, 2, 0.8f);
    }

    void drawSchematic (juce::Graphics& g, juce::Rectangle<float> a)
    {
        using namespace dispx;
        const auto& s = info.s;
        const auto m = s.machine;
        const float cy = a.getCentreY();

        g.setFont (theme::mono (11.0f, true));
        g.setColour (theme::accent);
        g.drawText (machineNames()[(int) m] + (m == Machine::Clean ? "  —  DELAY LINE"_u : ""), a.withHeight (16.0f).toNearestInt(), juce::Justification::centredLeft);
        a.removeFromTop (20.0f);

        g.setColour (theme::onWellText());
        auto flow = [&] (juce::Point<float> p0, juce::Point<float> p1)
        {
            g.setColour (theme::onWell()); arrow (g, p0, p1);
        };

        if (isTape (m))
        {
            const float rr = juce::jmin (a.getHeight() * 0.28f, 38.0f);
            const float yT = a.getY() + a.getHeight() * 0.62f;
            const float xL = a.getX() + rr + 30.0f, xR = a.getRight() - rr - 30.0f;
            // テープ
            g.setColour (theme::onWell());
            g.drawLine (xL, yT, xR, yT, 2.0f);
            // 流れる目印
            const float v = 60.0f;
            g.setColour (theme::accent);
            for (float xx = xL + std::fmod (phase * v, 26.0f); xx < xR; xx += 26.0f)
                g.fillEllipse (xx - 2.0f, yT - 2.0f, 4.0f, 4.0f);
            // リール
            for (int k = 0; k < 2; ++k)
            {
                const float cx = k == 0 ? xL : xR;
                const float ang = phase * (k == 0 ? 1.4f : 1.4f);
                g.setColour (theme::onWell());
                g.drawEllipse (cx - rr, yT - 2.0f * rr, rr * 2.0f, rr * 2.0f, 2.0f);
                for (int sp = 0; sp < 3; ++sp)
                {
                    const float th = ang + sp * juce::MathConstants<float>::twoPi / 3.0f;
                    const juce::Point<float> c { cx, yT - rr };
                    g.drawLine ({ c, c.getPointOnCircumference (rr * 0.85f, th) }, 1.6f);
                }
            }
            // ヘッド
            float lo, hi; machineTimeRange (m, lo, hi);
            const float span = (xR - 40.0f) - (xL + 40.0f);
            const float recX = xL + 40.0f;
            const float T = s.timeLms;
            float frac = juce::jlimit (0.05f, 1.0f, (T - lo) / (hi - lo) * 0.9f + 0.1f);
            const float playX = recX + span * frac;

            auto head = [&] (float hx, const juce::String& lab, bool play)
            {
                juce::Rectangle<float> hr (hx - 8.0f, yT + 3.0f, 16.0f, 22.0f);
                g.setColour (play ? theme::accent : theme::laneR);
                g.fillRoundedRectangle (hr, 3.0f);
                g.setColour (theme::onWell());
                g.setFont (theme::mono (9.5f, true));
                g.drawText (lab, (int) hx - 45, (int) yT + 27, 90, 12, juce::Justification::centred);
            };
            head (recX, "REC", false);
            if (m == Machine::SpaceEcho)
            {
                for (int k = 1; k <= 3; ++k)
                    head (recX + (playX - recX) * (float) k / 3.0f, "PLAY " + juce::String (k), true);
            }
            else head (playX, m == Machine::Echoplex ? "PLAY (MOVES)" : "PLAY", true);

            {   // ハンドル：再生ヘッドの位置 → Time
                Handle h;
                h.pos = { playX, yT + 14.0f };
                h.tip = "Time " + juce::String (T, 1) + " ms";
                h.ids.push_back (ParamID::timeL);
                h.onGrab = [this] { io.releaseSync (ParamID::syncL); };
                const auto mm = m;
                h.drag = [this, recX, span, lo, hi, mm] (juce::Point<float> p, const juce::MouseEvent&)
                {
                    const float fr = juce::jlimit (0.1f, 1.0f, (p.x - recX) / span);
                    io.set (ParamID::timeL, machineClampTime (mm, lo + (fr - 0.1f) / 0.9f * (hi - lo)));
                };
                addHandle (std::move (h));
            }
            {   // ハンドル：テープの揺れ → Wow
                Handle h;
                const float wv = s.wow / 100.0f;
                h.pos = { (xL + xR) * 0.5f, yT - 8.0f - wv * 28.0f };
                h.tip = "Wow " + juce::String (s.wow, 0) + " %";
                h.ids.push_back (ParamID::wow);
                const float y0w = yT - 8.0f;
                h.drag = [this, y0w] (juce::Point<float> p, const juce::MouseEvent&) { io.set (ParamID::wow, juce::jlimit (0.0f, 100.0f, (y0w - p.y) / 28.0f * 100.0f)); };
                addHandle (std::move (h));
            }
            // 距離の寸法線
            g.setColour (theme::onWell());
            const float yd = yT + 50.0f;
            g.drawLine (recX, yd, playX, yd, 1.0f);
            g.drawLine (recX, yd - 4.0f, recX, yd + 4.0f, 1.0f);
            g.drawLine (playX, yd - 4.0f, playX, yd + 4.0f, 1.0f);
            g.setFont (theme::mono (10.0f));
            const float cm = (m == Machine::StudioTape) ? 5.7f : (T / 1000.0f) * 19.05f;
            g.drawText ("d = " + juce::String (cm, 1) + " cm", juce::Rectangle<float> (recX, yd + 2.0f, playX - recX, 12.0f).toNearestInt(), juce::Justification::centred);

            const float yio = yT + 14.0f;                       // 入出力の矢印はテープの下（ヘッドの高さ）に通す
            flow ({ a.getX() + 26.0f, yio }, { recX - 11.0f, yio });
            g.setColour (theme::onWell()); g.setFont (theme::mono (10.0f, true));
            g.drawText ("IN", (int) a.getX(), (int) yio - 6, 24, 12, juce::Justification::centredLeft);
            flow ({ playX + 11.0f, yio }, { a.getRight() - 32.0f, yio });
            g.drawText ("OUT", (int) a.getRight() - 30, (int) yio - 6, 30, 12, juce::Justification::centredRight);
            return;
        }

        if (m == Machine::BBD)
        {
            const float cellW = 11.0f, cellH = 26.0f;
            const int cells = juce::jlimit (12, 60, (int) ((a.getWidth() - 90.0f) / (cellW + 3.0f)));
            const float x0 = a.getX() + 40.0f, y0 = cy - cellH / 2.0f;
            const int hot = (int) std::fmod (phase * 9.0f, (float) cells);
            const bool clk = std::fmod (phase * 9.0f, 1.0f) < 0.5f;
            for (int k = 0; k < cells; ++k)
            {
                juce::Rectangle<float> r (x0 + k * (cellW + 3.0f), y0, cellW, cellH);
                const bool on = (k == hot) || (k == hot - cells / 2 + cells * (hot < cells / 2 ? 1 : 0) && false);
                g.setColour (on ? theme::accent : theme::panel);
                g.fillRoundedRectangle (r, 2.0f);
                g.setColour (k % 2 == (clk ? 0 : 1) ? theme::laneL : theme::line);
                g.drawRoundedRectangle (r, 2.0f, 1.0f);
            }
            g.setColour (theme::onWell()); g.setFont (theme::mono (10.0f, true));
            g.drawText ("IN", (int) a.getX(), (int) cy - 6, 34, 12, juce::Justification::centredLeft);
            g.drawText ("OUT", (int) a.getRight() - 40, (int) cy - 6, 40, 12, juce::Justification::centredRight);
            g.drawText ("BUCKET-BRIGADE (N stages)", juce::Rectangle<float> (x0, y0 - 18.0f, cells * (cellW + 3.0f), 14.0f).toNearestInt(), juce::Justification::centred);
            // クロック波形
            const float yc = y0 + cellH + 26.0f;
            juce::Path p;
            float blo, bhi; machineTimeRange (m, blo, bhi);
            const float cw = juce::jmap (juce::jlimit (blo, bhi, s.timeLms), blo, bhi, 8.0f, 40.0f);
            float xx = x0;
            p.startNewSubPath (xx, yc);
            const float off = std::fmod (phase * 40.0f, cw * 2.0f);
            xx -= off;
            bool hi = true;
            for (int k = 0; k < 40 && xx < x0 + cells * (cellW + 3.0f); ++k)
            {
                const float nx = xx + cw;
                const float xa = juce::jmax (x0, xx), xb = juce::jmax (x0, nx);
                p.lineTo (xa, hi ? yc - 8.0f : yc + 8.0f);
                p.lineTo (xb, hi ? yc - 8.0f : yc + 8.0f);
                hi = ! hi; xx = nx;
            }
            g.setColour (theme::laneL);
            g.strokePath (p, juce::PathStrokeType (1.4f));
            g.setColour (theme::onWell());
            g.drawText ("CLOCK", (int) a.getX(), (int) yc - 6, 40, 12, juce::Justification::centredLeft);
            {
                Handle h;
                h.pos = { x0 + cw, yc - 8.0f };
                h.tip = "Time " + juce::String (s.timeLms, 1) + " ms  (clock period)";
                h.ids.push_back (ParamID::timeL);
                h.onGrab = [this] { io.releaseSync (ParamID::syncL); };
                const auto mm = m;
                h.drag = [this, x0, blo, bhi, mm] (juce::Point<float> p, const juce::MouseEvent&)
                {
                    const float fr = juce::jlimit (0.0f, 1.0f, (p.x - x0 - 8.0f) / 32.0f);
                    io.set (ParamID::timeL, machineClampTime (mm, blo + fr * (bhi - blo)));
                };
                addHandle (std::move (h));
            }
            flow ({ a.getX() + 22.0f, cy }, { x0 - 3.0f, cy });
            flow ({ x0 + cells * (cellW + 3.0f), cy }, { a.getRight() - 40.0f, cy });
            return;
        }

        if (m == Machine::OilCan)
        {
            const float R = juce::jmin (a.getHeight() * 0.40f, 60.0f);
            const juce::Point<float> c { a.getCentreX(), cy + 4.0f };
            g.setColour (theme::onWell());
            g.drawEllipse (c.x - R, c.y - R, 2 * R, 2 * R, 2.5f);
            g.drawEllipse (c.x - R * 0.35f, c.y - R * 0.35f, R * 0.7f, R * 0.7f, 1.2f);
            for (int k = 0; k < 8; ++k)                                // ドラム上の電荷の跡（回転）
            {
                const float th = phase * 1.1f + k * juce::MathConstants<float>::twoPi / 8.0f;
                const auto p = c.getPointOnCircumference (R, th);
                g.setColour (theme::accent.withAlpha (0.35f + 0.65f * (float) k / 8.0f));
                g.fillEllipse (p.x - 3.5f, p.y - 3.5f, 7.0f, 7.0f);
            }
            float lo, hi; machineTimeRange (s.machine, lo, hi);
            const float rdAng = -juce::MathConstants<float>::halfPi + (s.timeLms - lo) / (hi - lo) * juce::MathConstants<float>::pi * 1.4f + 0.3f;
            const float wrAng = -juce::MathConstants<float>::halfPi;
            auto headAt = [&] (float th, const juce::String& lab, juce::Colour col)
            {
                const auto p = c.getPointOnCircumference (R + 10.0f, th);
                g.setColour (col);
                g.fillRoundedRectangle (p.x - 7.0f, p.y - 7.0f, 14.0f, 14.0f, 3.0f);
                g.setColour (theme::onWell()); g.setFont (theme::mono (9.5f, true));
                const auto q = c.getPointOnCircumference (R + 32.0f, th);
                g.drawText (lab, (int) q.x - 22, (int) q.y - 6, 44, 12, juce::Justification::centred);
            };
            headAt (wrAng, "WRITE", theme::laneR);
            headAt (rdAng, "READ", theme::accent);
            {
                Handle h;
                h.pos = c.getPointOnCircumference (R + 10.0f, rdAng);
                h.tip = "Time " + juce::String (s.timeLms, 1) + " ms";
                h.ids.push_back (ParamID::timeL);
                h.onGrab = [this] { io.releaseSync (ParamID::syncL); };
                const auto mm = m;
                h.drag = [this, c, lo, hi, mm] (juce::Point<float> p, const juce::MouseEvent&)
                {
                    const float base = -juce::MathConstants<float>::halfPi + 0.3f;
                    float d = std::atan2 (p.y - c.y, p.x - c.x) - base;
                    while (d < 0.0f) d += juce::MathConstants<float>::twoPi;
                    while (d >= juce::MathConstants<float>::twoPi) d -= juce::MathConstants<float>::twoPi;
                    const float span = juce::MathConstants<float>::pi * 1.4f;
                    const float fr = d > span ? (d - span < (juce::MathConstants<float>::twoPi - span) * 0.5f ? 1.0f : 0.0f) : d / span;
                    io.set (ParamID::timeL, machineClampTime (mm, lo + fr * (hi - lo)));
                };
                addHandle (std::move (h));
            }
            g.setColour (theme::onWell()); g.setFont (theme::mono (10.0f, true));
            g.drawText ("ELECTROSTATIC DRUM", (int) (c.x - 70), (int) (c.y - 6), 140, 12, juce::Justification::centred);
            g.setFont (theme::mono (9.5f));
            g.drawText ("→ DIFFUSION"_u, (int) (c.x - 70), (int) (c.y + 8), 140, 12, juce::Justification::centred);
            return;
        }

        // Clean / Early Digital：ブロック図
        {
            const bool dig = (m == Machine::EarlyDigital);
            const float bh = dig ? 34.0f : 40.0f, y0 = dig ? a.getY() + 4.0f : cy - bh / 2.0f - 6.0f;
            const int nb = dig ? 5 : 4;
            const float gap = 26.0f;
            const float bw = (a.getWidth() - gap * (nb - 1)) / (float) nb;
            const float ageN = s.age / 100.0f;
            juce::StringArray labels;
            if (dig)
                labels = { "IN", "ADC " + juce::String (juce::roundToInt (16.0f - 8.0f * ageN)) + " bit", "RAM " + juce::String (juce::roundToInt (s.timeLms)) + " ms",
                           "DAC " + juce::String (info.fs / (1.0 + 3.0 * ageN) / 1000.0, 1) + " kHz", "OUT" };
            else
                labels = { "IN", "DELAY (RAM) " + juce::String (s.timeLms, 0) + " ms", "HP " + juce::String ((int) s.hpHz) + " / LP " + juce::String ((int) s.lpHz), "OUT" };
            for (int k = 0; k < nb; ++k)
            {
                juce::Rectangle<float> r (a.getX() + k * (bw + gap), y0, bw, bh);
                box (g, r, labels[k], k == 1 || (dig && k == 3) ? false : false);
                if (k < nb - 1) flow ({ r.getRight() + 3.0f, y0 + bh * 0.5f }, { r.getRight() + gap - 3.0f, y0 + bh * 0.5f });
            }
            // フィードバックの戻り道
            const float fbx0 = a.getX() + (nb - 2) * (bw + gap) + bw * 0.5f, fbx1 = a.getX() + 1 * (bw + gap) + bw * 0.5f;
            const float fbn = juce::jlimit (0.0f, 1.0f, s.feedback / 0.95f);
            const float ybTop = y0 + bh + 8.0f, ybRange = dig ? 14.0f : 22.0f;
            const float yb = ybTop + (1.0f - fbn) * ybRange;
            juce::Path p;
            p.startNewSubPath (fbx0, y0 + bh);  p.lineTo (fbx0, yb);  p.lineTo (fbx1, yb);  p.lineTo (fbx1, y0 + bh + 2.0f);
            g.setColour (theme::onWell());
            g.strokePath (p, juce::PathStrokeType (1.4f));
            g.setFont (theme::mono (10.0f, true));
            g.drawText ("× FEEDBACK "_u + juce::String (s.feedback * 100.0f, 0) + "%", (int) fbx1, (int) yb + 2, (int) (fbx0 - fbx1), 12, juce::Justification::centred);
            {   // ハンドル：戻り道の高さ → Feedback
                Handle h;
                h.pos = { (fbx0 + fbx1) * 0.5f, yb };
                h.tip = "Feedback " + juce::String (s.feedback * 100.0f, 1) + " %";
                h.ids.push_back (ParamID::feedback);
                h.ghost = s.feedback < 0.005f;
                h.drag = [this, ybTop, ybRange] (juce::Point<float> p, const juce::MouseEvent&)
                {
                    io.set (ParamID::feedback, juce::jlimit (0.0f, 1.0f, 1.0f - (p.y - ybTop) / ybRange) * 95.0f);
                };
                addHandle (std::move (h));
            }
            {   // ハンドル：Time（対数トラック）
                float tlo, thi; machineTimeRange (m, tlo, thi);
                const float ty = y0 - 8.0f;
                const float lg = std::log (thi / tlo);
                g.setColour (theme::line);
                g.drawLine (a.getX(), ty, a.getRight(), ty, 1.0f);
                Handle h;
                h.pos = { a.getX() + a.getWidth() * std::log (juce::jlimit (tlo, thi, s.timeLms) / tlo) / lg, ty };
                h.tip = "Time " + juce::String (s.timeLms, 1) + " ms";
                h.ids.push_back (ParamID::timeL);
                h.onGrab = [this] { io.releaseSync (ParamID::syncL); };
                const auto mm = m;
                const float ax = a.getX(), aw = a.getWidth();
                h.drag = [this, ax, aw, tlo, thi, lg, mm] (juce::Point<float> p, const juce::MouseEvent&)
                {
                    io.set (ParamID::timeL, machineClampTime (mm, tlo * std::exp (juce::jlimit (0.0f, 1.0f, (p.x - ax) / aw) * lg)));
                };
                addHandle (std::move (h));
            }
            // 流れる点
            const float u = std::fmod (phase * 0.4f, 1.0f);
            const float tx = a.getX() + u * (a.getWidth());
            g.setColour (theme::accent);
            g.fillEllipse (tx - 4.0f, y0 - 6.0f, 8.0f, 8.0f);
            if (dig)   // 量子化・サンプルホールドのイメージ（誇張）
            {
                const float wy = yb + 22.0f, wh = juce::jmax (16.0f, a.getBottom() - wy - 16.0f) * 0.5f;
                const float stepX = juce::jlimit (3.0f, 24.0f, (1.0f + 3.0f * ageN) * 5.0f);
                const int lv = juce::jmax (3, (int) std::lround (std::pow (2.0f, 6.0f - 4.0f * ageN)));
                juce::Path w2, q;
                for (float xx = 0; xx < a.getWidth(); xx += 1.0f)
                {
                    const float yy = std::sin ((xx + phase * 30.0f) * 0.045f);
                    const float held = std::sin ((std::floor (xx / stepX) * stepX + phase * 30.0f) * 0.045f);
                    const float qy = std::round (held * (float) lv) / (float) lv;
                    if (xx == 0) { w2.startNewSubPath (a.getX() + xx, wy + wh - yy * wh * 0.5f); q.startNewSubPath (a.getX() + xx, wy + wh - qy * wh * 0.5f); }
                    else { w2.lineTo (a.getX() + xx, wy + wh - yy * wh * 0.5f); q.lineTo (a.getX() + xx, wy + wh - qy * wh * 0.5f); }
                }
                g.setColour (theme::onWell().withAlpha (0.5f)); g.strokePath (w2, juce::PathStrokeType (1.0f));
                g.setColour (theme::accent);                     g.strokePath (q, juce::PathStrokeType (1.6f));
                g.setColour (theme::onWell()); g.setFont (theme::mono (9.5f));
                g.drawText (info.L ("※ビット数・サンプルレートの低下を誇張して描画", "* bit depth and sample-rate loss, exaggerated"), (int) a.getX(), (int) (wy + wh * 2.0f - 2.0f), (int) a.getWidth(), 12, juce::Justification::centredLeft);
            }
        }
    }

    // ---- 下段：Mod 波形（Rate × Depth）／ Drive 曲線 ／ Age・Wow・Flutter のトラック ----
    void drawControls (juce::Graphics& g, juce::Rectangle<float> a)
    {
        using namespace dispx;
        const auto& s = info.s;
        const auto m = s.machine;
        g.setColour (theme::line);
        g.drawHorizontalLine ((int) a.getY() - 3, a.getX(), a.getRight());
        const float gap = 14.0f;
        const float wMod = a.getWidth() * 0.38f, wDrv = a.getWidth() * 0.24f;
        auto rMod = a.removeFromLeft (wMod);  a.removeFromLeft (gap);
        auto rDrv = a.removeFromLeft (wDrv);  a.removeFromLeft (gap);
        auto rTr  = a;

        auto cap = [&] (juce::Rectangle<float> r, const juce::String& t)
        {
            g.setFont (theme::mono (9.5f, true));
            g.setColour (theme::onWell());
            g.drawText (t, r.withHeight (12.0f).toNearestInt(), juce::Justification::centredLeft);
        };

        {   // Mod：横＝Rate（対数）、縦＝Depth
            cap (rMod, "MOD  Rate " + juce::String (s.rateHz, 2) + " Hz / Depth " + juce::String (s.depthMs, 2) + " ms");
            auto r = rMod.withTrimmedTop (14.0f).reduced (2.0f, 2.0f);
            g.setColour (theme::panel);  g.fillRoundedRectangle (r, 3.0f);
            const float mid = r.getCentreY(), amp = r.getHeight() * 0.5f - 4.0f;
            g.setColour (theme::line);   g.drawHorizontalLine ((int) mid, r.getX(), r.getRight());
            const float lo = 0.05f, hi = 6.0f;
            const float depthN = juce::jlimit (0.0f, 1.0f, s.depthMs / 5.0f);
            juce::Path p;
            const float cycles = 0.05f + s.rateHz * 0.45f;           // 窓の中の周期数（見た目用）
            for (float xx = 0.0f; xx <= r.getWidth(); xx += 1.0f)
            {
                const float ang = xx / r.getWidth() * cycles * juce::MathConstants<float>::twoPi * 2.0f + phase;
                const float uu  = ang / juce::MathConstants<float>::twoPi;
                const float fr  = uu - std::floor (uu);                                   // 1周期の中の位置 0..1
                auto hash = [] (int k) { return std::sin ((float) k * 12.9898f) * 43758.5453f - std::floor (std::sin ((float) k * 12.9898f) * 43758.5453f); };
                float wave;
                switch (s.lfoShape)
                {
                    case 1:  wave = fr < 0.25f ? 4.0f * fr : (fr < 0.75f ? 2.0f - 4.0f * fr : 4.0f * fr - 4.0f); break;
                    case 2:  wave = 2.0f * fr - 1.0f; break;
                    case 3:  wave = fr < 0.5f ? 1.0f : -1.0f; break;
                    case 4:  { const int k = (int) std::floor (uu);
                               const float a0 = hash (k) * 2.0f - 1.0f, a1 = hash (k + 1) * 2.0f - 1.0f;
                               const float t = 0.5f - 0.5f * std::cos (fr * juce::MathConstants<float>::pi);
                               wave = a0 + (a1 - a0) * t; break; }
                    default: wave = std::sin (ang); break;
                }
                const float y = mid - wave * amp * depthN;
                if (xx == 0.0f) p.startNewSubPath (r.getX(), y); else p.lineTo (r.getX() + xx, y);
            }
            g.setColour (theme::accent);  g.strokePath (p, juce::PathStrokeType (1.6f));
            Handle h;
            const float fx = std::log (juce::jlimit (lo, hi, s.rateHz) / lo) / std::log (hi / lo);
            h.pos = { r.getX() + 8.0f + (r.getWidth() - 16.0f) * fx, r.getBottom() - 6.0f - (r.getHeight() - 12.0f) * depthN };
            h.tip = "Rate " + juce::String (s.rateHz, 2) + " Hz   Depth " + juce::String (s.depthMs, 2) + " ms";
            h.ids = { ParamID::rate, ParamID::depth };
            const auto rr = r;
            h.drag = [this, rr, lo, hi] (juce::Point<float> q, const juce::MouseEvent&)
            {
                const float fr = juce::jlimit (0.0f, 1.0f, (q.x - rr.getX() - 8.0f) / (rr.getWidth() - 16.0f));
                io.set (ParamID::rate, lo * std::pow (hi / lo, fr));
                io.set (ParamID::depth, juce::jlimit (0.0f, 1.0f, (rr.getBottom() - 6.0f - q.y) / (rr.getHeight() - 12.0f)) * 5.0f);
            };
            addHandle (std::move (h));
        }
        {   // Drive：入力→出力の曲線
            const bool rel = (m != Machine::Clean);
            cap (rDrv, "DRIVE " + juce::String (s.drive, 0) + " %");
            auto r = rDrv.withTrimmedTop (14.0f).reduced (2.0f, 2.0f);
            g.setColour (theme::panel);  g.fillRoundedRectangle (r, 3.0f);
            g.setColour (theme::line);   g.drawLine (r.getX(), r.getBottom(), r.getRight(), r.getY(), 1.0f);
            const float k = 1.0f + 5.0f * s.drive / 100.0f;
            juce::Path p;
            for (float xx = 0.0f; xx <= 1.0001f; xx += 0.02f)
            {
                const float y = std::tanh (k * xx) / std::tanh (k);
                const float px = r.getX() + 2.0f + (r.getWidth() - 4.0f) * xx, py = r.getBottom() - 2.0f - (r.getHeight() - 4.0f) * y;
                if (xx == 0.0f) p.startNewSubPath (px, py); else p.lineTo (px, py);
            }
            g.setColour (rel ? theme::accent : theme::faint);  g.strokePath (p, juce::PathStrokeType (1.6f));
            Handle h;
            const float yh = std::tanh (k * 0.3f) / std::tanh (k);
            h.pos = { r.getX() + 2.0f + (r.getWidth() - 4.0f) * 0.3f, r.getBottom() - 2.0f - (r.getHeight() - 4.0f) * yh };
            h.tip = "Drive " + juce::String (s.drive, 0) + " %";
            h.ids = { ParamID::drive };
            h.ghost = ! rel;
            const auto rr = r;
            h.drag = [this, rr] (juce::Point<float> q, const juce::MouseEvent&)
            {
                const float y = juce::jlimit (0.31f, 0.99f, (rr.getBottom() - 2.0f - q.y) / (rr.getHeight() - 4.0f));
                // tanh(k*0.3)/tanh(k) = y を k について二分探索
                float lo = 1.0f, hi = 6.0f;
                for (int i = 0; i < 20; ++i) { const float mid = 0.5f * (lo + hi); if (std::tanh (mid * 0.3f) / std::tanh (mid) > y) lo = mid; else hi = mid; }
                io.set (ParamID::drive, juce::jlimit (0.0f, 100.0f, (0.5f * (lo + hi) - 1.0f) / 5.0f * 100.0f));
            };
            addHandle (std::move (h));
        }
        {   // Age / Wow / Flutter：横トラック
            struct Tr { const char* name; const char* id; float v; bool relevant; };
            const bool tape = isTape (m);
            const Tr trs[3] = { { "AGE", ParamID::age, s.age, m != Machine::Clean },
                                { "WOW", ParamID::wow, s.wow, tape },
                                { "FLUTTER", ParamID::flutter, s.flutter, tape } };
            const float rowH = rTr.getHeight() / 3.0f;
            for (int i = 0; i < 3; ++i)
            {
                const float y = rTr.getY() + rowH * ((float) i + 0.5f) + 2.0f;
                const float x0 = rTr.getX() + 58.0f, x1 = rTr.getRight() - 34.0f;
                g.setFont (theme::mono (9.5f, true));
                g.setColour (trs[i].relevant ? theme::onWellText() : theme::faint);
                g.drawText (trs[i].name, (int) rTr.getX(), (int) y - 6, 56, 12, juce::Justification::centredLeft);
                g.drawText (juce::String (trs[i].v, 0) + "%", (int) x1 + 4, (int) y - 6, 30, 12, juce::Justification::centredLeft);
                g.setColour (theme::line);
                g.fillRoundedRectangle (x0, y - 2.0f, x1 - x0, 4.0f, 2.0f);
                g.setColour (trs[i].relevant ? theme::accent.withAlpha (0.6f) : theme::faint.withAlpha (0.4f));
                g.fillRoundedRectangle (x0, y - 2.0f, (x1 - x0) * trs[i].v / 100.0f, 4.0f, 2.0f);
                Handle h;
                h.pos = { x0 + (x1 - x0) * trs[i].v / 100.0f, y };
                h.tip = juce::String (trs[i].name) + " " + juce::String (trs[i].v, 0) + " %";
                h.ids.push_back (trs[i].id);
                h.ghost = ! trs[i].relevant;
                const char* id = trs[i].id;
                h.drag = [this, x0, x1, id] (juce::Point<float> q, const juce::MouseEvent&)
                {
                    io.set (id, juce::jlimit (0.0f, 1.0f, (q.x - x0) / (x1 - x0)) * 100.0f);
                };
                addHandle (std::move (h));
            }
        }
    }
};

// -----------------------------------------------------------------------------
//  「原理を数字で確かめる」：右パネルに置く（図ではなくテキスト／数値）
// -----------------------------------------------------------------------------
class NumbersView : public juce::Component
{
public:
    void update (const dispx::Info& i) { info = i; repaint(); }
    void paint (juce::Graphics& g) override { drawNumbers (g, getLocalBounds()); }
private:
    dispx::Info info;
    void drawNumbers (juce::Graphics& g, juce::Rectangle<int> r) const
    {
        using namespace dispx;
        const auto& s = info.s;
        const auto m = s.machine;
        g.setColour (theme::panel);
        g.fillRoundedRectangle (r.toFloat(), 5.0f);
        g.setColour (theme::line);
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 5.0f, 1.0f);

        juce::StringArray lines;
        auto add = [&] (const juce::String& t) { lines.add (t); };
        const float T = s.timeLms;
        const double N = std::round (T * info.fs / 1000.0);
        const float ageN = s.age / 100.0f;
        const float fbPct = s.feedback * 100.0f;

        add ("#" + info.L ("原理を数字で確かめる", "Check the principle with numbers"));
        switch (m)
        {
            case Machine::Clean:
                add (info.L ("Time = 遅らせるサンプル数 ÷ fs", "Time = delay samples / fs"));
                add (juce::String (T, 1) + " ms × "_u + juce::String ((int) info.fs) + " Hz ÷ 1000 = "_u + juce::String ((int) N) + info.L (" サンプル", " samples"));
                add (info.L ("劣化なし：Age / Drive / Wow は無効", "No degradation: Age / Drive / Wow are inactive"));
                break;
            case Machine::StudioTape:
            {
                const float d = 5.7f, v = d / (T / 1000.0f);
                add (info.L ("速度 = ヘッド間距離 ÷ Time", "speed = head gap / Time"));
                add ("5.7 cm ÷ "_u + juce::String (T / 1000.0f, 3) + " s = " + juce::String (v, 1) + " cm/s (" + juce::String (v / 2.54f, 1) + " ips)");
                add (info.L ("Time を半分にするには速度が2倍（15→30 ips で1オクターブ上）", "Half the Time needs double the speed (15 -> 30 ips = one octave up)"));
                break;
            }
            case Machine::Echoplex:
            {
                add (info.L ("距離 = Time × テープ速度（7.5 ips = 19.05 cm/s と仮定）", "gap = Time x tape speed (assuming 7.5 ips = 19.05 cm/s)"));
                add (juce::String (T / 1000.0f, 3) + " s × 19.05 cm/s = "_u + juce::String (T / 1000.0f * 19.05f, 1) + " cm");
                add (info.L ("Time を動かす＝ヘッドが動く → 動く間だけピッチが滑る", "Moving Time moves the head, so the pitch slides while it moves"));
                break;
            }
            case Machine::SpaceEcho:
                add (info.L ("3ヘッド：間隔 1 : 2 : 3（テープ速度 19.05 cm/s と仮定）", "3 heads at 1 : 2 : 3 (assuming 19.05 cm/s)"));
                add (juce::String (T / 3.0f, 0) + " / " + juce::String (T * 2.0f / 3.0f, 0) + " / " + juce::String (T, 0) + " ms  →  "_u
                     + juce::String (T / 3000.0f * 19.05f, 1) + " / " + juce::String (T * 2.0f / 3000.0f * 19.05f, 1) + " / " + juce::String (T / 1000.0f * 19.05f, 1) + " cm");
                add (info.L ("1回のリピートに3つの音（タップ）が並ぶ", "Each repeat carries three taps"));
                break;
            case Machine::BBD:
            {
                const float fc = 4096.0f / (2.0f * T / 1000.0f);
                add (info.L ("クロック f = N ÷ (2 × Time)　（N = 4096段と仮定）", "clock f = N / (2 x Time)   (assuming N = 4096)"));
                add ("4096 ÷ (2 × "_u + juce::String (T / 1000.0f, 3) + " s) = " + juce::String (fc / 1000.0f, 1) + " kHz");
                add (info.L ("ナイキスト = f ÷ 2 = ", "Nyquist = f / 2 = ") + juce::String (fc / 2000.0f, 1) + " kHz");
                add (info.L ("Time を2倍 → クロック半分 → 高域は出なくなる", "Double Time -> half the clock -> the highs disappear"));
                break;
            }
            case Machine::OilCan:
                add (info.L ("オールパス 4段 の g = 0.35 + 0.4 × Age", "4 all-pass stages, g = 0.35 + 0.4 x Age"));
                add ("g = " + juce::String (0.35f + 0.4f * ageN, 2) + info.L ("（大きいほど輪郭が崩れる）", " (bigger = blurrier)"));
                add (info.L ("Diffusion が Age に自動で連動する", "Diffusion follows Age automatically"));
                break;
            case Machine::EarlyDigital:
            {
                const float bits = 16.0f - 8.0f * ageN, sr = (float) info.fs / (1.0f + 3.0f * ageN);
                add (info.L ("ビット = 16 − 8 × Age", "bits = 16 - 8 x Age") );
                add (juce::String (bits, 1) + " bit → "_u + juce::String ((int) std::pow (2.0f, bits)) + info.L (" 段階", " levels") + "   SNR ≈ 6.02×"_u + juce::String (bits, 1) + "+1.76 = " + juce::String (6.02f * bits + 1.76f, 0) + " dB");
                add ("SR = fs ÷ (1 + 3 × Age) = "_u + juce::String (sr / 1000.0f, 1) + " kHz → "_u + info.L ("ナイキスト ", "Nyquist ") + juce::String (sr / 2000.0f, 1) + " kHz");
                break;
            }
        }

        if (m != Machine::Clean)
        {
            if (isTape (m))
            {
                const float wowT = T * 0.004f * s.wow / 100.0f, fluT = T * 0.0008f * s.flutter / 100.0f;
                add (info.L ("Wow ", "Wow ") + juce::String (s.wow, 0) + "% → ±"_u + juce::String (wowT, 2) + " ms → ±"_u
                     + juce::String (1200.0f * std::log2 (1.0f + wowT / 1000.0f * 6.283f * 0.7f), 1) + " cent");
                add ("Flutter " + juce::String (s.flutter, 0) + "% → ±"_u + juce::String (fluT, 3) + " ms → ±"_u
                     + juce::String (1200.0f * std::log2 (1.0f + fluT / 1000.0f * 6.283f * 9.0f), 1) + " cent");
            }
            const float fc = machineLpHz (m, s.age, T);
            if (fc < 19999.0f)
                add ("Age " + juce::String (s.age, 0) + "% → LP ≈ "_u + juce::String (juce::roundToInt (fc)) + " Hz"
                     + (m == Machine::OilCan ? "" : info.L ("，ヒスノイズ付加", ", hiss added")));
            add ("Drive " + juce::String (s.drive, 0) + "% → "_u + info.L ("入力 ×", "input x") + juce::String (1.0f + (m == Machine::BBD ? 3.0f : 5.0f) * s.drive / 100.0f, 1)
                 + " " + info.L ("を tanh に通して頭を丸める", "through tanh to round the peaks"));
        }

        add (info.L ("Feedback ", "Feedback ") + juce::String (fbPct, 0) + "% → 2"_u + info.L ("回目 ", "nd ") + juce::String (fbPct, 0) + "% / 3" + info.L ("回目 ", "rd ")
             + juce::String (fbPct * fbPct / 100.0f, 1) + "% (" + juce::String (juce::Decibels::gainToDecibels (juce::jmax (1.0e-4f, s.feedback)), 1) + " / "
             + juce::String (juce::Decibels::gainToDecibels (juce::jmax (1.0e-4f, s.feedback * s.feedback)), 1) + " dB)");

        if (std::abs (s.pitchSemis) > 0.001f)
        {
            const float ratio = std::pow (2.0f, s.pitchSemis / 12.0f);
            add ("Pitch " + signedStr (s.pitchSemis, 1) + " st → ×"_u + juce::String (ratio, 3)
                 + (s.pitchInLoop ? info.L ("  In Loop：1回目 ", "  In Loop: 1st ") + signedStr (s.pitchSemis, 0) + info.L (" / 2回目 ", " / 2nd ") + signedStr (s.pitchSemis * 2, 0) + info.L (" / 3回目 ", " / 3rd ") + signedStr (s.pitchSemis * 3, 0)
                                  : info.L ("  Output：全リピート ", "  Output: every repeat ") + signedStr (s.pitchSemis, 0)));
        }
        if (s.reverse)
            add (info.L ("Reverse：切り取り長 = Time = ", "Reverse: slice length = Time = ") + juce::String (T, 0) + " ms");
        if (s.duckDb > 0.05f)
            add ("Duck: " + info.L ("入力 −36 dBFS から効き始め −12 dBFS で最大 −", "starts at -36 dBFS, full at -12 dBFS: -") + juce::String (s.duckDb, 1) + " dB, Release " + juce::String ((int) s.duckRelMs) + " ms");

        auto area = r.reduced (10, 8);
        const float fsz = 10.5f;
        for (auto& l : lines)
        {
            const bool head = l.startsWith ("#");
            g.setFont (head ? theme::ui (12.0f, true) : theme::mono (fsz));
            g.setColour (head ? theme::accent : theme::text);
            const auto t = head ? l.substring (1) : l;
            const int hgt = head ? 18 : (juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), t) > area.getWidth() ? 28 : 15);
            g.drawFittedText (t, area.removeFromTop (hgt), juce::Justification::topLeft, 2, 0.85f);
            area.removeFromTop (head ? 2 : 3);
            if (area.getHeight() < 12) break;
        }
    }
};

// -----------------------------------------------------------------------------
//  3つの図を同時に並べる：上段 TIMELINE ／ 下段 左＝原理図、右＝リピート特性
// -----------------------------------------------------------------------------
class FiguresPanel : public juce::Component, private juce::Timer
{
public:
    explicit FiguresPanel (juce::AudioProcessorValueTreeState& a) : timeline (a), diagram (a), repeats (a)
    {
        addAndMakeVisible (timeline);
        addAndMakeVisible (diagram);
        addAndMakeVisible (repeats);
        startTimerHz (30);
    }
    void setLanguage (Lang l) { info.lang = l; pushInfo(); }
    void update (const DelaySettings& s, double bpm, double fs)
    {
        info.s = s;  info.bpm = bpm;  info.fs = fs;
        pushInfo();
    }
    const dispx::Info& getInfo() const { return info; }

    void resized() override
    {
        auto r = getLocalBounds();
        const int gap = 8;
        timeline.setBounds (r.removeFromTop (juce::roundToInt (r.getHeight() * 0.36f)));
        r.removeFromTop (gap);
        diagram.setBounds (r.removeFromLeft (juce::roundToInt (r.getWidth() * 0.60f)));
        r.removeFromLeft (gap);
        repeats.setBounds (r);
    }

private:
    void pushInfo()
    {
        timeline.update (info);
        diagram.update (info);
        repeats.update (info);
    }
    void timerCallback() override { if (isShowing()) diagram.tick(); }

    TimelineView timeline;
    DiagramView diagram;
    RepeatView repeats;
    dispx::Info info;
};
