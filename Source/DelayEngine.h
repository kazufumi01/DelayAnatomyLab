#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>
#include "Params.h"
#include "Machines.h"

// =============================================================================
//  FractionalDelay：リングバッファ＋4点エルミート補間（小数サンプルの遅延が読める）
// =============================================================================
class FractionalDelay
{
public:
    void prepare (int maxDelaySamples)
    {
        int size = 1;
        while (size < maxDelaySamples + 8)
            size <<= 1;
        buffer.assign ((size_t) size, 0.0f);
        mask = size - 1;
        writePos = 0;
    }

    void clear() { std::fill (buffer.begin(), buffer.end(), 0.0f); }

    float read (float delaySamples) const
    {
        const float d    = juce::jlimit (3.0f, (float) (mask - 4), delaySamples);
        const float pos  = (float) writePos - d;
        const int   i    = (int) std::floor (pos);
        const float frac = pos - (float) i;

        const float xm1 = at (i - 1), x0 = at (i), x1 = at (i + 1), x2 = at (i + 2);
        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * frac + c2) * frac + c1) * frac + x0;
    }

    void write (float x)
    {
        buffer[(size_t) writePos] = x;
        writePos = (writePos + 1) & mask;
    }

private:
    float at (int i) const { return buffer[(size_t) (i & mask)]; }

    std::vector<float> buffer;
    int mask = 0, writePos = 0;
};

// =============================================================================
//  Granular：2つの読み出し点を交互にクロスフェードする「窓つき再生」
//   ratio = +1 … そのまま　2 … 1オクターブ上　-1 … 逆再生
//   ・Pitch（ratio = 2^(半音/12)）と Reverse（ratio = -1、窓長 = Time）で共用
// =============================================================================
class Granular
{
public:
    void prepare (int maxWin)
    {
        int size = 1;
        while (size < maxWin + 32)
            size <<= 1;
        buf.assign ((size_t) size, 0.0f);
        mask = size - 1;
        clear();
    }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; ph = 0.0f; }

    float process (float x, float ratio, float win)
    {
        win = juce::jlimit (64.0f, (float) (mask - 16), win);
        buf[(size_t) w] = x;

        float p2 = ph + 0.5f;
        p2 -= std::floor (p2);
        const float s1 = std::sin (juce::MathConstants<float>::pi * ph);
        const float s2 = std::sin (juce::MathConstants<float>::pi * p2);
        const float y  = rd (ph * win + 2.0f) * (s1 * s1) + rd (p2 * win + 2.0f) * (s2 * s2);

        ph += (1.0f - ratio) / win;
        ph -= std::floor (ph);
        w = (w + 1) & mask;
        return y;
    }

private:
    float rd (float d) const
    {
        const float pos = (float) w - d;
        const int i = (int) std::floor (pos);
        const float f = pos - (float) i;
        const float a = buf[(size_t) (i & mask)], b = buf[(size_t) ((i + 1) & mask)];
        return a + (b - a) * f;
    }
    std::vector<float> buf;
    int mask = 0, w = 0;
    float ph = 0.0f;
};

// Oil Can 用：Schroeder オールパス（にじみ）
struct Allpass
{
    std::vector<float> buf;
    int pos = 0;
    void prepare (int n) { buf.assign ((size_t) juce::jmax (2, n), 0.0f); pos = 0; }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); pos = 0; }
    float process (float x, float g)
    {
        const float d = buf[(size_t) pos];
        const float y = -g * x + d;
        buf[(size_t) pos] = x + g * y;
        if (++pos >= (int) buf.size()) pos = 0;
        return y;
    }
};

// =============================================================================
//  DelayEngine
//
//   入力 ─┬──────────────────────────────────── Dry ──┐
//         └→ [Delay] →(Reverse)→ HP → LP → [機種の色付け] →(Pitch In Loop)─┬→ Wet →(Pitch Output)→ Duck ┤→ 出力
//              ↑                                                         │
//              └──────────────── × Feedback ─────────────────────────────┘
//   機種の色付け：Drive（飽和）→ 機種ごとのローパス／ビット・SR劣化／にじみ ＋ ノイズ（Age）
// =============================================================================
struct DelaySettings
{
    DelayMode mode = DelayMode::Mono;
    float timeLms = 110.0f, timeRms = 110.0f;
    float feedback = 0.05f;              // 0..0.95
    float hpHz = 200.0f, lpHz = 5000.0f;
    float rateHz = 0.5f, depthMs = 0.0f;
    int   lfoShape = 0;                  // 0=Sine 1=Triangle 2=Saw 3=Square 4=Random
    float wetGain = 0.35f;               // リニア
    bool  delayOff = false, wetOnly = false, monoCheck = false;

    Machine machine = Machine::Clean;
    float age = 0.0f, drive = 0.0f, wow = 0.0f, flutter = 0.0f;   // いずれも 0..100 (%)
    float pitchSemis = 0.0f;             // 半音＋セント/100
    bool  pitchInLoop = true;
    bool  reverse = false;
    float duckDb = 0.0f, duckRelMs = 250.0f;
};

class DelayEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize)
    {
        fs = sampleRate;
        const int maxSamples = (int) std::ceil (fs * 4.0);      // 最大4秒

        for (auto& d : lines)
            d.prepare (maxSamples);
        for (auto& g : rev)   g.prepare (maxSamples);
        pitchWin = (float) (fs * 0.046);
        for (auto& g : pitIn)  g.prepare ((int) pitchWin + 64);
        for (auto& g : pitOut) g.prepare ((int) pitchWin + 64);

        juce::dsp::ProcessSpec spec { fs, (juce::uint32) maxBlockSize, 2 };
        hp.prepare (spec);  hp.setType (juce::dsp::StateVariableTPTFilterType::highpass);
        lp.prepare (spec);  lp.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        hp.setResonance (1.0f / std::sqrt (2.0f));
        lp.setResonance (1.0f / std::sqrt (2.0f));

        juce::dsp::ProcessSpec mono { fs, (juce::uint32) maxBlockSize, 1 };
        for (auto& ch : cl)
            for (auto& f : ch)
            {
                f.prepare (mono);
                f.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
                f.setResonance (1.0f / std::sqrt (2.0f));
            }

        const int base = (int) std::ceil (fs / 44100.0 * 1.0);
        static const int apLen[4] = { 113, 191, 277, 349 };
        for (auto& ch : ap)
            for (int k = 0; k < 4; ++k)
                ch[k].prepare (apLen[k] * base);

        fbSm.reset  (fs, 0.02);  wetSm.reset (fs, 0.02);  drySm.reset (fs, 0.02);
        depthSm.reset (fs, 0.05);
        reset();
    }

    void reset()
    {
        for (auto& d : lines)  d.clear();
        for (auto& g : rev)    g.clear();
        for (auto& g : pitIn)  g.clear();
        for (auto& g : pitOut) g.clear();
        for (auto& ch : ap) for (auto& a : ch) a.clear();
        for (auto& ch : cl) for (auto& f : ch) f.reset();
        hp.reset();
        lp.reset();
        phase = phW = phF = 0.0;
        lfoSm = lfoRnd = 0.0f;
        held[0] = held[1] = 0.0f;  holdAcc[0] = holdAcc[1] = 0.0f;
        env = 0.0f;  duckGain = 1.0f;  drift = 0.0f;
        firstBlock = true;
    }

    void process (juce::AudioBuffer<float>& buffer, int numInputChannels, const DelaySettings& s)
    {
        if (s.mode != lastMode || s.reverse != lastReverse)
        {
            reset();                    // 配線が変わる時は古いエコーを捨てる（別の経路で一瞬鳴るのを防ぐ）
            lastMode = s.mode;
            lastReverse = s.reverse;
        }

        const bool clean = (s.machine == Machine::Clean);
        const bool tape  = isTape (s.machine);
        const float ageN   = clean ? 0.0f : juce::jlimit (0.0f, 1.0f, s.age / 100.0f);
        const float driveN = clean ? 0.0f : juce::jlimit (0.0f, 1.0f, s.drive / 100.0f);
        const float pre    = 1.0f + (s.machine == Machine::BBD ? 3.0f : 5.0f) * driveN;
        const float wowN   = tape ? s.wow / 100.0f : 0.0f;
        const float fluN   = tape ? s.flutter / 100.0f : 0.0f;

        // 機種ごとの設定（ブロックごとに更新）
        float noiseAmt = 0.0f;
        switch (s.machine)
        {
            case Machine::StudioTape: case Machine::Echoplex: case Machine::SpaceEcho: noiseAmt = 0.004f * ageN; break;
            case Machine::BBD:    noiseAmt = 0.003f * ageN; break;
            case Machine::OilCan: noiseAmt = 0.002f * ageN; break;
            default: break;
        }
        const float apG    = 0.35f + 0.4f * ageN;
        const float bits   = 16.0f - 8.0f * ageN;
        const float levels = std::pow (2.0f, bits - 1.0f);
        const float srDiv  = 1.0f + 3.0f * ageN;
        const int   lpStages = machineLpStages (s.machine);
        const bool  spaceEcho = (s.machine == Machine::SpaceEcho);

        const float toSamples = (float) (fs / 1000.0);
        const float tgtL = s.timeLms * toSamples, tgtR = s.timeRms * toSamples;
        if (firstBlock) { tL = tgtL; tR = tgtR; }
        const float coef = 1.0f - std::exp (-1.0f / (float) (fs * machineGlideSeconds (s.machine)));

        setTarget (depthSm, s.depthMs * toSamples);
        setTarget (fbSm, s.mode == DelayMode::Haas ? 0.0f : juce::jlimit (0.0f, 0.95f, s.feedback));
        setTarget (wetSm, s.delayOff ? 0.0f : s.wetGain);
        setTarget (drySm, s.wetOnly ? 0.0f : 1.0f);
        firstBlock = false;

        hp.setCutoffFrequency (juce::jlimit (20.0f, 20000.0f, s.hpHz));
        lp.setCutoffFrequency (juce::jlimit (20.0f, (float) (fs * 0.45), s.lpHz));
        const float fcL = juce::jlimit (200.0f, (float) (fs * 0.45), machineLpHz (s.machine, s.age, s.timeLms));
        const float fcR = juce::jlimit (200.0f, (float) (fs * 0.45), machineLpHz (s.machine, s.age, s.timeRms));
        for (int k = 0; k < 2; ++k) { cl[0][k].setCutoffFrequency (fcL); cl[1][k].setCutoffFrequency (fcR); }

        const bool  pitchOn   = std::abs (s.pitchSemis) > 0.001f;
        const float pitchRatio = std::pow (2.0f, s.pitchSemis / 12.0f);
        const bool  duckOn    = s.duckDb > 0.05f;
        const float envAtk = 1.0f - std::exp (-1.0f / (float) (fs * 0.005));
        const float envRel = 1.0f - std::exp (-1.0f / (float) (fs * juce::jmax (0.02f, s.duckRelMs * 0.001f)));

        const double phaseInc = juce::MathConstants<double>::twoPi * s.rateHz / fs;
        const int    shape    = juce::jlimit (0, 4, s.lfoShape);
        // 角のある波形（Saw / Square）はクリックにならないよう約 60 Hz で丸める。Random は 1 周期かけて次の値へ滑らかに移る
        const float  lfoCoef  = shape == 4 ? 1.0f - std::exp (-5.0f * juce::jmax (0.05f, s.rateHz) / (float) fs)
                                           : 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 60.0f / (float) fs);
        const double wowInc   = juce::MathConstants<double>::twoPi * 0.7 / fs;
        const double fluInc   = juce::MathConstants<double>::twoPi * 9.0 / fs;
        const float  driftCoef = (float) (juce::MathConstants<double>::twoPi * 1.5 / fs);

        const int n = buffer.getNumSamples();
        auto* outL = buffer.getWritePointer (0);
        auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
        const float* inRptr = numInputChannels > 1 ? buffer.getReadPointer (1) : buffer.getReadPointer (0);

        // 1チャンネル分：読み出し → (Reverse) → HP/LP → 機種の色付け → (Pitch In Loop)
        struct Out { float fb, wet; };
        auto chain = [&] (int ch, float dly, float winSamples) -> Out
        {
            float r = lines[ch].read (dly);
            float extra = 0.0f;
            if (spaceEcho)                                           // 3ヘッド：1/3・2/3 の位置にもタップ
                extra = 0.6f * lines[ch].read (dly * (2.0f / 3.0f)) + 0.4f * lines[ch].read (dly * (1.0f / 3.0f));
            if (s.reverse)
                r = rev[ch].process (r, -1.0f, winSamples);

            float y = lp.processSample (ch, hp.processSample (ch, r));

            if (! clean)
            {
                y += noiseAmt * rnd();
                switch (s.machine)
                {
                    case Machine::StudioTape: case Machine::Echoplex: case Machine::SpaceEcho:
                        y = std::tanh (y * pre) / pre;
                        y = cl[ch][0].processSample (0, y);
                        break;
                    case Machine::BBD:
                        y = std::tanh (y * pre) / pre;
                        for (int k = 0; k < lpStages; ++k) y = cl[ch][k].processSample (0, y);
                        break;
                    case Machine::OilCan:
                        y = std::tanh (y * pre) / pre;
                        y = cl[ch][0].processSample (0, y);
                        for (int k = 0; k < 4; ++k) y = ap[ch][k].process (y, apG);
                        break;
                    case Machine::EarlyDigital:
                        y = std::tanh (y * pre) / pre;
                        holdAcc[ch] += 1.0f;
                        if (holdAcc[ch] >= srDiv) { holdAcc[ch] -= srDiv; held[ch] = y; }
                        y = std::round (held[ch] * levels) / levels;
                        break;
                    default: break;
                }
            }

            Out o { y, y + 0.5f * extra };
            if (pitchOn)
            {
                const float p = pitIn[ch].process (y, pitchRatio, pitchWin);      // 常に回して窓の状態を保つ
                if (s.pitchInLoop) { o.fb = p; o.wet = p + 0.5f * extra; }
                else               { o.wet = pitOut[ch].process (y + 0.5f * extra, pitchRatio, pitchWin); }
            }
            return o;
        };

        for (int i = 0; i < n; ++i)
        {
            const float inL  = outL[i];
            const float inR  = inRptr[i];
            const float mono = 0.5f * (inL + inR);

            // --- 時間の追従（機種ごとに速さが違う）＋ Mod ＋ Wow / Flutter ---
            tL += (tgtL - tL) * coef;
            tR += (tgtR - tR) * coef;

            const float u = (float) (phase / juce::MathConstants<double>::twoPi);     // 0..1
            float lfoRaw = 0.0f;
            switch (shape)
            {
                case 1:  lfoRaw = u < 0.25f ? 4.0f * u : (u < 0.75f ? 2.0f - 4.0f * u : 4.0f * u - 4.0f); break;   // Triangle
                case 2:  lfoRaw = 2.0f * u - 1.0f;                break;                                           // Saw（上昇→急に戻る）
                case 3:  lfoRaw = u < 0.5f ? 1.0f : -1.0f;        break;                                           // Square
                case 4:  lfoRaw = lfoRnd;                         break;                                           // Random（周期ごとの値）
                default: lfoRaw = (float) std::sin (phase);       break;                                           // Sine
            }
            lfoSm += (lfoRaw - lfoSm) * lfoCoef;
            const float mod = depthSm.getNextValue() * (shape == 0 ? lfoRaw : lfoSm);
            phase += phaseInc;
            if (phase > juce::MathConstants<double>::twoPi)
            {
                phase -= juce::MathConstants<double>::twoPi;
                lfoRnd = rnd();                                                   // 次の周期の値（-1..1）
            }

            float wf = 0.0f;                                    // 時間に対する比率の揺れ
            if (tape)
            {
                drift += (rnd() - drift) * driftCoef;
                const float w = (float) std::sin (phW) + 30.0f * drift;               // ゆっくりした揺れ＋ランダムなふらつき
                const float f = (float) std::sin (phF) + 0.35f * (float) std::sin (phF * 1.27);
                wf = wowN * 0.004f * w + fluN * 0.0008f * f;
                phW += wowInc;  if (phW > juce::MathConstants<double>::twoPi) phW -= juce::MathConstants<double>::twoPi;
                phF += fluInc;  if (phF > juce::MathConstants<double>::twoPi) phF -= juce::MathConstants<double>::twoPi;
            }

            const float dL = tL + mod + tL * wf;
            const float dR = (s.mode == DelayMode::Mono ? tL + mod + tL * wf : tR - mod + tR * wf);

            const auto cLo = chain (0, dL, tL);
            const auto cRo = chain (1, dR, tR);

            const float g = fbSm.getNextValue();
            float wetL = 0.0f, wetR = 0.0f, dryL = inL, dryR = inR;

            switch (s.mode)
            {
                case DelayMode::Mono:
                    lines[0].write (mono + g * cLo.fb);
                    lines[1].write (0.0f);
                    wetL = wetR = cLo.wet;
                    break;

                case DelayMode::Stereo:
                    lines[0].write (inL + g * cLo.fb);
                    lines[1].write (inR + g * cRo.fb);
                    wetL = cLo.wet;  wetR = cRo.wet;
                    break;

                case DelayMode::PingPong:
                    lines[0].write (mono + g * cRo.fb);
                    lines[1].write (g * cLo.fb);
                    wetL = cLo.wet;  wetR = cRo.wet;
                    break;

                case DelayMode::Haas:
                    lines[0].write (0.0f);
                    lines[1].write (mono);
                    dryL = mono;  dryR = 0.0f;
                    wetR = cRo.wet;
                    break;
            }

            // --- Duck：入力が鳴っている間は Wet を下げる ---
            float duckLin = 1.0f;
            if (duckOn)
            {
                const float lvl = juce::jmax (std::abs (inL), std::abs (inR));
                env += (lvl - env) * (lvl > env ? envAtk : envRel);
                const float db  = 20.0f * std::log10 (env + 1.0e-6f);
                const float red = s.duckDb * juce::jlimit (0.0f, 1.0f, (db + 36.0f) / 24.0f);   // −36dBFS から効き始め −12dBFS で最大
                duckLin = std::pow (10.0f, -red / 20.0f);
            }

            const float dg = drySm.getNextValue();
            const float wg = wetSm.getNextValue() * duckLin;
            float l = dryL * dg + wetL * wg;
            float r = dryR * dg + wetR * wg;

            if (s.monoCheck)
                l = r = 0.5f * (l + r);

            outL[i] = l;
            if (outR != nullptr)
                outR[i] = r;
        }
    }

private:
    float rnd() { seed = seed * 1664525u + 1013904223u; return (float) (int) seed * (1.0f / 2147483648.0f); }

    void setTarget (juce::SmoothedValue<float>& sv, float v)
    {
        if (firstBlock) sv.setCurrentAndTargetValue (v);
        else            sv.setTargetValue (v);
    }

    double fs = 44100.0, phase = 0.0, phW = 0.0, phF = 0.0;
    float  lfoSm = 0.0f, lfoRnd = 0.0f;
    bool firstBlock = true, lastReverse = false;
    DelayMode lastMode = DelayMode::Mono;
    float tL = 0.0f, tR = 0.0f, pitchWin = 2048.0f;
    float held[2] {}, holdAcc[2] {};
    float env = 0.0f, duckGain = 1.0f, drift = 0.0f;
    juce::uint32 seed = 12345u;
    FractionalDelay lines[2];
    Granular rev[2], pitIn[2], pitOut[2];
    Allpass ap[2][4];
    juce::dsp::StateVariableTPTFilter<float> hp, lp;
    juce::dsp::StateVariableTPTFilter<float> cl[2][2];
    juce::SmoothedValue<float> fbSm, wetSm, drySm, depthSm;
};
