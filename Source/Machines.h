#pragma once
#include <cmath>
#include <juce_core/juce_core.h>
#include "Params.h"

// =============================================================================
//  機種ごとの設定値・説明（エンジン／表示／解説から共通で使う）
//  ※ いずれも「実機そのもの」ではなく、特徴をつかむための簡易モデル。
// =============================================================================
struct MachineDefaults { float age, drive, wow, flutter; };

// 機種ボタンを押した時に入る「その機種らしい」初期値（Age / Drive / Wow / Flutter）
inline MachineDefaults machineDefaults (int m)
{
    static const MachineDefaults d[] = { { 0, 0, 0, 0 },      // Clean
                                         { 25, 30, 10, 8 },   // Studio Tape
                                         { 45, 45, 20, 15 },  // Echoplex
                                         { 40, 40, 25, 20 },  // Space Echo
                                         { 40, 25, 0, 0 },    // BBD
                                         { 50, 30, 10, 5 },   // Oil Can
                                         { 50, 0, 0, 0 } };   // Early Digital
    return d[juce::jlimit (0, 6, m)];
}

inline bool isTape (Machine m)
{
    return m == Machine::StudioTape || m == Machine::Echoplex || m == Machine::SpaceEcho;
}

// 機種ごとに Time が動かせる範囲（ms）。Clean は制限なし。
inline void machineTimeRange (Machine m, float& lo, float& hi)
{
    switch (m)
    {
        case Machine::StudioTape:   lo = 30.0f;  hi = 600.0f;  break;
        case Machine::Echoplex:     lo = 60.0f;  hi = 750.0f;  break;
        case Machine::SpaceEcho:    lo = 60.0f;  hi = 900.0f;  break;
        case Machine::BBD:          lo = 20.0f;  hi = 600.0f;  break;
        case Machine::OilCan:       lo = 40.0f;  hi = 400.0f;  break;
        case Machine::EarlyDigital: lo = 1.0f;   hi = 1000.0f; break;
        default:                    lo = 1.0f;   hi = 2000.0f; break;
    }
}

inline float machineClampTime (Machine m, float ms)
{
    float lo, hi;
    machineTimeRange (m, lo, hi);
    return juce::jlimit (lo, hi, ms);
}

// Time が変わった時にどれだけ滑らかに追従するか（秒）。テープ＝ピッチが滑る／デジタル＝ほぼ瞬時
inline float machineGlideSeconds (Machine m)
{
    switch (m)
    {
        case Machine::StudioTape:
        case Machine::Echoplex:
        case Machine::SpaceEcho:    return 0.09f;
        case Machine::BBD:          return 0.05f;
        case Machine::OilCan:       return 0.07f;
        case Machine::EarlyDigital: return 0.002f;
        default:                    return 0.02f;
    }
}

// 機種が持つ「ループ内ローパス」のカットオフ（Age と Time で決まる）。無い機種は 20000。
inline float machineLpHz (Machine m, float agePct, float timeMs)
{
    const float a = juce::jlimit (0.0f, 1.0f, agePct / 100.0f);
    switch (m)
    {
        case Machine::StudioTape: return 16000.0f * std::pow (0.20f, a);
        case Machine::Echoplex:   return 12000.0f * std::pow (0.20f, a);
        case Machine::SpaceEcho:  return 11000.0f * std::pow (0.20f, a);
        case Machine::BBD:        // 長いほどクロックが遅い＝暗い
            return juce::jlimit (1200.0f, 12000.0f,
                                 8000.0f * std::sqrt (100.0f / juce::jlimit (20.0f, 600.0f, timeMs)) * std::pow (0.45f, a));
        case Machine::OilCan:     return 9000.0f * std::pow (0.35f, a);
        default:                  return 20000.0f;
    }
}

inline int machineLpStages (Machine m) { return m == Machine::BBD ? 2 : (m == Machine::Clean || m == Machine::EarlyDigital ? 0 : 1); }

// ---- 解説（プリセット解説の下に表示）----------------------------------------
struct MachineText
{
    const char* eraJa; const char* eraEn;
    const char* learnJa; const char* learnEn;
    const char* others;     // Timeless 3 / EchoBoy / ValhallaDelay での対応
};

inline const MachineText& machineText (int m)
{
    static const MachineText t[] =
    {
        { "Delay Basics と同じ劣化なしのエンジン", "The same clean engine as Delay Basics",
          "劣化なしの「ものさし」。他の機種とA/Bして、何が足されたのかを聴く。", "A no-degradation yardstick. A/B other machines against it to hear what each one adds.",
          "Timeless 3: FX すべてOFF／EchoBoy: クリーン系 Style／Valhalla: Digital・Age 0%" },

        { "1950s〜 2トラックのテープエコー", "1950s two-track tape echo",
          "ボーカル定番のエコーの基準。Driveがピークを丸め、Ageで高域が丸くなる。Wow/Flutterは控えめ。", "The vocal-echo reference. Drive rounds the peaks, Age rolls off the highs. Wow/Flutter stay subtle.",
          "Timeless 3: Drive少なめ＋ハイカット10kHz／EchoBoy: Studio Tape／Valhalla: HiFi・Chrome Tape" },

        { "1960s 可動ヘッドのテープエコー（Time＝ヘッド位置）", "1960s moving-head tape echo (Time = head position)",
          "Timeを動かすとピッチが滑る理由は「ヘッドとテープ速度」。ヒス＋高域ロス＋Driveの押し込みが特徴。", "Why moving Time slides the pitch: head distance over tape speed. Hiss, HF loss and driven preamp are its hallmark.",
          "Timeless 3: Drive＋LFO→Time／EchoBoy: EchoPlex系／Valhalla: Tape・ERA Past" },

        { "1970s 3ヘッドのテープエコー", "1970s three-head tape echo",
          "リピートが1回に複数本（ヘッド比 1:2:3）。「タップ」の概念と、Multi-Headの入口。", "Several taps per repeat (head ratio 1:2:3). The idea of taps, and the door to multi-head delays.",
          "Timeless 3: Taps 3本／EchoBoy: Space Echo系＋Rhythm／Valhalla: Tape＋STYLE Quad" },

        { "1970s〜80s アナログICのバケツリレー", "1970s-80s analog bucket-brigade chips",
          "Timeと音色が連動する仕組み。長くするほどクロックが遅くなり、高域が出なくなって暗く甘い音になる。", "Time and tone are linked: the longer the delay, the slower the clock and the darker the sound.",
          "Timeless 3: Lo-Fi＋ハイカット4kHz＋LFO／EchoBoy: Memory Man系・DM-2系／Valhalla: BBD・Analog" },

        { "1950s〜60s 静電ドラム（Oil Can）", "1950s-60s electrostatic drum (Oil Can)",
          "Diffusion（にじみ）とは何か。リピートが輪郭を失い、エコーがリバーブに溶けていく境目が分かる。", "What diffusion is: repeats lose their edges and echo melts into reverb.",
          "Timeless 3: Diffuse＋LFO／EchoBoy: TelRay／Valhalla: 専用なし→DIFFUSION" },

        { "1980s 初期デジタルディレイ", "1980s early digital delay",
          "劣化が「暗く」ではなく「粗く」なる。ビット数とサンプルレートが下がるとザラつく（Age 50% ≒ 12bit）。", "Degradation turns 'coarse' instead of 'dark'. Fewer bits and a lower sample rate sound gritty (Age 50% = 12 bit).",
          "Timeless 3: Lo-Fi／EchoBoy: クリーン系Style＋ハイカット／Valhalla: Digital・Age 50%" },
    };
    return t[juce::jlimit (0, 6, m)];
}
