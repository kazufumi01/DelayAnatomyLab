#pragma once
#include <juce_core/juce_core.h>
#include "Presets.h"
#include "Localisation.h"
#include "Machines.h"

// =============================================================================
//  マウスオーバーで右パネル上部に出す説明（日本語／英語）
//  ・パラメーターは ID から引く。ボタン群は番号から引く
// =============================================================================
struct HelpText { juce::String title, body; bool valid() const { return title.isNotEmpty(); } };

inline HelpText paramHelp (const juce::String& id, Lang lang)
{
    struct E { const char* id; const char* tj; const char* bj; const char* te; const char* be; };
    static const E t[] = {
        { "mode", "MODE（動作モード）",
          "Mono＝L/R共通の1本。Stereo＝L/Rで別々の Time。Ping-Pong＝左→右→左…と交互に返す（Time L → Time R の順）。Haas＝L は原音のまま R だけ 1〜30 ms 遅らせて横に広げる（Mono で痩せる）。",
          "MODE", "Mono: one shared line. Stereo: separate L/R times. Ping-Pong: bounces left, right, left (Time L then Time R). Haas: L stays dry, only R is delayed 1-30 ms to widen (thins in mono)." },
        { "timeL", "Time（遅れ時間）",
          "原音からエコーまでの時間。60〜120 ms＝スラップ、1/4音符（120 BPM で 500 ms）＝通常のエコー、30 ms 以下＝厚み（別のエコーに聞こえない）。機種ごとに動かせる範囲が違う。Sync が入っているとテンポに追従し、触ると同期が外れる。",
          "Time", "Delay from the source to the echo. 60-120 ms: slapback. A quarter note (500 ms at 120 BPM): normal echo. Under 30 ms: thickness. Each machine has its own range. With Sync on it follows tempo; touching it releases the sync." },
        { "timeR", "Time R（右の遅れ時間）",
          "Stereo / Ping-Pong / Haas で右側に使う時間。L と少しずらす（例 13 / 21 ms、1/4T と 1/4）と広がりが出る。Mono モードでは使わない。",
          "Time R", "The right-side time in Stereo / Ping-Pong / Haas. Offsetting it from L (for example 13 / 21 ms, or 1/4T and 1/4) adds width. Unused in Mono." },
        { "syncL", "SYNC L",
          "Time を音符で指定。ms＝手動。120 BPM で 1/4＝500 ms、付点8分＝375 ms、8分3連＝167 ms、1/4 3連＝333 ms。付点8分は『音符が倍に聞こえる』定番、3連は跳ねる。",
          "SYNC L", "Sets Time by note value. ms = manual. At 120 BPM: 1/4 = 500 ms, dotted 1/8 = 375 ms, 1/8T = 167 ms, 1/4T = 333 ms. Dotted 1/8 sounds like double the notes; triplets swing." },
        { "syncR", "SYNC R", "右側の Time を音符で指定。L と違う音符にするとリズムが絡む（例 L 1/8D・R 1/4）。",
          "SYNC R", "Sets the right Time by note value. A different value from L makes the rhythm interlock (for example L 1/8D, R 1/4)." },
        { "feedback", "Feedback（返す量）",
          "エコーを何回返すか。0％＝1回、25％＝3〜4回、50％＝長い余韻、85％以上は発振に注意（先に音量を下げる）。n 回目の大きさは (Feedback)^(n-1)。",
          "Feedback", "How many times the echo repeats. 0% = once, 25% = 3-4 repeats, 50% = a long tail, 85%+ risks runaway (lower the level first). Repeat n is (Feedback)^(n-1) in level." },
        { "hp", "High Pass（低域カット）",
          "エコーの低域を削る。200〜300 Hz で歌・ギターに被らない、ダブや厚い低域では 450〜900 Hz。リピートごとに掛かるので、回数が増えるほど細くなる。",
          "High Pass", "Cuts the echo's lows. 200-300 Hz keeps it off vocals and guitars; 450-900 Hz for dub and heavy lows. It acts on every repeat, so later repeats get thinner." },
        { "lp", "Low Pass（高域カット）",
          "エコーの高域を削る。5 kHz 前後で歌の後ろに、2〜3 kHz で暗く奥へ。リピートごとに掛かるので、後ろのリピートほど暗くなる。機種のローパス（Age）とは別に掛かる。",
          "Low Pass", "Cuts the echo's highs. Around 5 kHz sits behind a vocal; 2-3 kHz is dark and far back. It acts on every repeat, so later repeats get darker. Separate from the machine's own low-pass (Age)." },
        { "rate", "Mod Rate（揺れの速さ）",
          "Time を揺らす速さ。0.3〜0.7 Hz でゆったり、2 Hz 以上はヴィブラート風。Depth が 0 だと何も起きない。",
          "Mod Rate", "How fast Time sways. 0.3-0.7 Hz is slow and gentle; above 2 Hz it turns vibrato-like. Nothing happens while Depth is 0." },
        { "lfoShape", "LFO Shape（揺れの波形）",
          "Time を揺らす波形。Sine＝滑らかな標準。Triangle＝上り下りが一定の速さで、古い BBD コーラスの質感。Saw＝ゆっくり遅れていき急に戻る（ピッチが一方向にずれ続ける）。Square＝2つの遅れ時間を行き来する（切替時に音程が跳ねる）。Random＝周期ごとに揺れ幅がランダムに変わる（アナログの不規則さ）。",
          "LFO Shape", "The waveform that sways Time. Sine: smooth default. Triangle: constant-speed rise and fall, the old BBD chorus feel. Saw: drifts later slowly, then snaps back (pitch keeps leaning one way). Square: jumps between two delay times (pitch hops at each switch). Random: a new sway amount every cycle (analogue irregularity)." },
        { "depth", "Mod Depth（揺れの深さ）",
          "Time の揺れ幅（ms）。0.2 ms＝ほぼ分からない、1〜3 ms＝コーラス風、4 ms 以上＝揺れすぎ。短い Time（ダブリング）では 1 ms 前後が自然。",
          "Mod Depth", "How far Time sways (ms). 0.2 ms is barely audible, 1-3 ms is chorus-like, 4 ms+ is too wobbly. About 1 ms is natural for doubling." },
        { "wet", "Wet（エコーの量）",
          "原音（ドライ）を 0 dB のままにした時の、エコーの相対的な大きさ。−14 dB＝さりげない、−6 dB＝存在感、0 dB＝原音と同じ大きさ。センドで使う時は Wet Only と併用。",
          "Wet", "Echo level relative to the dry signal, which stays at 0 dB. -14 dB: subtle. -6 dB: present. 0 dB: as loud as the source. On a send, combine with Wet Only." },
        { "delayOff", "Bypass",
          "ディレイを切って原音だけにする。エコーが原音をどれだけ変えているか、A/B で確かめる時に。",
          "Bypass", "Turns the delay off, leaving only the source. Use it to A/B how much the echo changes the sound." },
        { "wetOnly", "Wet Only",
          "原音を切ってエコーだけを出す。センド／リターンで使う時、またはエコー自体の音色（暗さ・歪み・にじみ）だけを聴く時に。",
          "Wet Only", "Mutes the source and outputs only the echo. For sends and returns, or to hear the echo tone (darkness, distortion, smear) alone." },
        { "wetLock", "Wet Lock",
          "プリセットを切り替えても Wet の値を保つ。聴き比べで『音量の差』に惑わされないために。",
          "Wet Lock", "Keeps Wet unchanged when you switch presets, so level differences do not mislead an A/B comparison." },
        { "monoCheck", "Mono",
          "L+R を足して聴く。Haas やダブリングなど短いディレイで、位相が打ち消されて痩せていないかの確認用。",
          "Mono", "Sums L+R. Checks whether short delays such as Haas and doubling cancel and thin out in mono." },
        { "demo", "SAMPLE",
          "入力が無い時に鳴らす内蔵音。Off＝入力そのもの（ホスト音）。Pluck＝単音でエコーの減り方を聴く、Snare+Kick＝リズムの絡みを聴く、Vox＝語尾の残り方を聴く。",
          "SAMPLE", "Built-in sound for when there is no input. Off = the host signal. Pluck: hear how repeats decay. Snare+Kick: hear the rhythm. Vox: hear the tails." },
        { "machine", "MACHINE（方式）",
          "ディレイの方式。選ぶと、その機種らしい Age / Drive / Wow / Flutter が自動で入る。A/B で『何が足されたか』を聴き比べる。",
          "MACHINE", "The delay type. Choosing one loads typical Age / Drive / Wow / Flutter for it. A/B them to hear what each one adds." },
        { "age", "Age（劣化）",
          "機種ごとに意味が変わる。テープ＝高域が丸くなる、BBD＝帯域が狭まりノイズが増える、Oil Can＝にじみが増える、Early Digital＝ビット数・サンプルレートが下がる（12 bit→8 bit）。Clean では無効。",
          "Age", "Meaning depends on the machine. Tape: highs roll off. BBD: narrower band and more noise. Oil Can: more smear. Early Digital: lower bit depth and sample rate (12 -> 8 bit). Inactive on Clean." },
        { "drive", "Drive（歪み）",
          "入力を tanh に通してピークを丸める。上げるとリピートのアタックが太く丸くなり、リピートごとに歪みが積み重なる。30〜45％が温かい、80％以上は荒い。",
          "Drive", "Runs the input through tanh to round the peaks. Raising it fattens the attacks, and distortion stacks per repeat. 30-45% is warm; 80%+ is rough." },
        { "wow", "Wow（ゆっくりした揺れ）",
          "テープの遅い音程の揺れ（約 0.7 Hz）。10〜20％で温かみ、50％以上は酔う。テープ系の機種だけ有効。",
          "Wow", "Slow tape pitch drift (about 0.7 Hz). 10-20% adds warmth; 50%+ is seasick. Active on tape machines only." },
        { "flutter", "Flutter（速い震え）",
          "テープの細かい音程の震え（約 9 Hz）。10〜15％で古い質感、40％以上は震えが目立つ。Wow と片方ずつ戻すと違いが分かる。",
          "Flutter", "Fast tape pitch shiver (about 9 Hz). 10-15% gives an old feel; 40%+ is obvious. Pull Wow and Flutter back one at a time to hear the difference." },
        { "pitch", "Pitch（半音）",
          "リピートの音程をずらす。+12＝オクターブ上（きらめき）、−12＝オクターブ下（沈む）、+7＝5度上。In Loop ならリピートごとに積み上がる。",
          "Pitch", "Shifts the repeats' pitch. +12 = octave up (shimmer), -12 = octave down (sinking), +7 = a fifth up. With In Loop it stacks per repeat." },
        { "pitchFine", "Fine（セント）",
          "±50 cent の微調整。+14 cent と 28 ms 前後の短い Time で、ダブル録りのような広がり（ピッチ・ダブラー）になる。",
          "Fine", "Fine tuning within +-50 cents. About +14 cents with a 28 ms Time makes a double-tracked width (pitch doubler)." },
        { "pitchMode", "Pitch Mode",
          "In Loop＝リピートごとに音程が積み上がる（+12 なら 12, 24, 36…）。Output＝全リピートが同じ音程になる。",
          "Pitch Mode", "In Loop: pitch stacks per repeat (+12 gives 12, 24, 36...). Output: every repeat stays at the same pitch." },
        { "reverse", "Reverse（逆再生）",
          "Time の長さで切り取って逆向きに再生する。Feedback 0％・400〜800 ms で歌い出し前のスウェルに。",
          "Reverse", "Cuts a slice of length Time and plays it backward. At Feedback 0% and 400-800 ms it makes a swell before a vocal entrance." },
        { "duck", "Duck（ダッキング）",
          "入力が鳴っている間だけエコーを下げる（−36 dBFS から効き始め −12 dBFS で最大）。歌っている間は引っ込み、切れ目で咲く。6〜12 dB が目安。",
          "Duck", "Lowers the echo while the input is playing (starts at -36 dBFS, full at -12 dBFS). The echo steps back while singing and blooms in the gaps. 6-12 dB is typical." },
        { "duckRel", "Duck Release",
          "ダッキングから戻る速さ。100 ms＝すぐ戻る、300〜500 ms＝ゆっくり咲く。",
          "Duck Release", "How quickly the echo recovers after ducking. 100 ms returns fast; 300-500 ms blooms slowly." },
    };
    for (auto& e : t)
        if (id == e.id)
            return lang == Lang::Japanese ? HelpText { jp (e.tj), jp (e.bj) } : HelpText { e.te, e.be };
    return {};
}

// ボタン群（番号つき）
inline HelpText modeHelp (int i, Lang l)
{
    static const char* ids[] = { "mode", "mode", "mode", "mode" };
    auto h = paramHelp (ids[juce::jlimit (0, 3, i)], l);
    static const char* ja[] = { "Mono：L/R 共通の1本。いちばん基本。", "Stereo：L と R で別々の Time。左右に広がる。", "Ping-Pong：Time L → Time R の順に左右へ交互に返す。", "Haas：L は原音、R だけ 1〜30 ms 遅らせて横に広げる。Mono Check で確認。" };
    static const char* en[] = { "Mono: one shared line. The basic one.", "Stereo: separate L and R times. Spreads sideways.", "Ping-Pong: bounces to the sides, Time L then Time R.", "Haas: L is dry, only R is delayed 1-30 ms. Check with Mono." };
    h.title = (l == Lang::Japanese ? jp ("MODE：") : juce::String ("MODE: ")) + juce::String (modeNames()[juce::jlimit (0, 3, i)]);
    h.body = l == Lang::Japanese ? jp (ja[juce::jlimit (0, 3, i)]) : juce::String (en[juce::jlimit (0, 3, i)]);
    return h;
}

inline HelpText syncHelp (int i, Lang l)
{
    static const char* nm[] = { "ms（手動）", "1/16", "1/8 3連", "1/8", "付点8分", "1/4 3連", "1/4", "付点4分", "1/2" };
    static const char* nmE[] = { "ms (manual)", "1/16", "1/8 triplet", "1/8", "dotted 1/8", "1/4 triplet", "1/4", "dotted 1/4", "1/2" };
    static const double beats[] = { 0, 0.25, 1.0 / 3.0, 0.5, 0.75, 2.0 / 3.0, 1.0, 1.5, 2.0 };
    i = juce::jlimit (0, 8, i);
    HelpText h;
    h.title = "SYNC: " + (l == Lang::Japanese ? jp (nm[i]) : juce::String (nmE[i]));
    if (i == 0)
        h.body = l == Lang::Japanese ? jp ("Time をミリ秒で手動指定。図の ● やノブを動かすとこちらに戻る。")
                                     : juce::String ("Set Time in milliseconds by hand. Moving a dot in the figures returns here.");
    else
        h.body = l == Lang::Japanese ? jp ("120 BPM で ") + juce::String (beats[i] * 500.0, 0) + jp (" ms。テンポが変わっても曲に合わせて追従する。")
                                     : "At 120 BPM: " + juce::String (beats[i] * 500.0, 0) + " ms. Follows the host tempo.";
    return h;
}

inline HelpText machineHelp (int m, Lang l)
{
    m = juce::jlimit (0, 6, m);
    const auto& mt = machineText (m);
    HelpText h;
    h.title = "MACHINE: " + machineNames()[m];
    h.body = l == Lang::Japanese ? jp (mt.eraJa) + "\n" + jp (mt.learnJa) : juce::String (mt.eraEn) + "\n" + mt.learnEn;
    return h;
}

inline HelpText presetHelp (int i, Lang l)
{
    HelpText h;
    if (! juce::isPositiveAndBelow (i, (int) factoryPresets().size())) return h;
    const auto& p = factoryPresets()[(size_t) i];
    const auto& en = presetTextEN (i);
    h.title = juce::String (p.name) + "  [" + (l == Lang::Japanese ? jp (p.tag) : juce::String (en.tag)) + "]";
    h.body = l == Lang::Japanese ? jp (p.what) : juce::String (en.what);
    return h;
}

// 名前つきの UI 部品
inline HelpText uiHelp (const juce::String& key, Lang l)
{
    struct E { const char* k; const char* tj; const char* bj; const char* te; const char* be; };
    static const E t[] = {
        { "zoom", "画面サイズ・配色", "ウィンドウ全体の拡大縮小と配色を選ぶ。右下の角をドラッグしても拡大縮小できる。", "Size and look", "Window zoom and colour scheme. You can also drag the bottom-right corner." },
        { "lang", "言語", "日本語／English の切り替え。説明文とラベルが変わる。", "Language", "Switches Japanese / English for the text and labels." },
        { "info", "解説パネル", "右の解説パネルを開閉する。左の図や操作部の大きさ・配置は変わらない。", "Info panel", "Opens and closes the right panel. The figures and controls on the left do not move or resize." },
        { "prev", "前のプリセット", "一つ前のプリセットに切り替える（ユーザープリセットも順に回る）。", "Previous preset", "Steps to the previous preset (user presets are included in the cycle)." },
        { "next", "次のプリセット", "一つ後のプリセットに切り替える。A/B 1〜7 を順に押すと方式の違いが聴き比べられる。", "Next preset", "Steps to the next preset. Step through A/B 1-7 to compare machines." },
        { "presetName", "プリセット一覧", "カテゴリ別のプリセット一覧。『使い分け』のカテゴリに、方式の聴き比べと場面別の例がある。", "Preset list", "Presets by category. The 'Choosing' categories hold machine A/B comparisons and scene examples." },
        { "save", "保存", "いまのつまみの値をユーザープリセットとして保存する。", "Save", "Saves the current values as a user preset." },
        { "manage", "管理", "上書き保存・名前変更・削除・元の値に戻す・読み込み・書き出し。", "Manage", "Overwrite, rename, delete, revert, import and export." },
        { "timeline", "TIMELINE（エコーの並び）",
          "エコーが何 ms にどの大きさで並ぶかの理論図。●をドラッグ：横＝Time、縦＝Wet（1つ目）／Feedback（2つ目）。Cmd を押しながらで 1/16 音符に吸着、Shift で微調整、ダブルクリックで初期値。",
          "TIMELINE", "Theoretical echo positions and levels. Drag the dots: horizontal = Time, vertical = Wet (first) / Feedback (second). Cmd snaps to 1/16 notes, Shift = fine, double-click = reset." },
        { "diagram", "原理図（機種の仕組み）",
          "機種ごとの仕組み。●をドラッグ：テープ＝再生ヘッド（Time）とテープの揺れ（Wow）、BBD＝クロック周期（Time）、Oil Can＝READ ヘッド（Time）、Clean/Digital＝戻り道の高さ（Feedback）と Time の線。下段は Mod・Drive・Age/Wow/Flutter。",
          "PRINCIPLE", "How the machine works. Drag the dots: tape = play head (Time) and tape sway (Wow), BBD = clock period (Time), Oil Can = READ head (Time), Clean/Digital = return path height (Feedback) and the Time track. The lower row holds Mod, Drive, Age/Wow/Flutter." },
        { "repeats", "リピート特性",
          "1回目・2回目・3回目…の周波数特性。●をドラッグ：High Pass / Low Pass の肩、機種のローパスの肩（Age）、2回目の高さ（Feedback）。リピートのたびにフィルターが掛かるので、後ろほど狭く暗くなる。",
          "REPEATS", "Frequency response of repeat 1, 2, 3... Drag the dots: High Pass / Low Pass corners, the machine low-pass corner (Age), and the height of repeat 2 (Feedback). Filters apply on every repeat, so later ones are narrower and darker." },
        { "compare", "比較表",
          "選んだプリセットの値と、いまの値の比較。変えた行は色が付く。現在値はクリックで数字入力、左右／上下にドラッグでも変えられる（Shift で微調整）。",
          "Comparison table", "The preset's values next to the current ones. Changed rows are highlighted. Click a current value to type, or drag it sideways/up-down (Shift = fine)." },
        { "stripBasic", "BASICS（基本ノブ）", "Time・Feedback・High Pass・Low Pass・Mod Rate / Depth のノブ。図の●と同じパラメーターを数値で正確に動かせる。Wet・スイッチ類はどちらの表示でも共通。",
          "BASICS", "Knobs for Time, Feedback, High Pass, Low Pass and Mod Rate / Depth: the same parameters as the dots in the figures, for precise values. Wet and the switches are shared by both pages." },
        { "stripFx", "FX（ピッチ・ダッキング・逆再生）", "Pitch・Fine・Duck・Release のノブと、Reverse / Pitch Mode。どの機種でも使える。Wet・スイッチ類は BASICS と共通。",
          "FX", "Knobs for Pitch, Fine, Duck and Release, plus Reverse / Pitch Mode. Works on any machine. Wet and the switches are shared with BASICS." },
        { "tip", "操作のヒント", "ボタン・図の ● ・表の行にマウスを乗せると、ここに説明が出ます。", "Hint", "Hover over buttons, dots in the figures or table rows to see an explanation here." },
    };
    for (auto& e : t)
        if (key == e.k)
            return l == Lang::Japanese ? HelpText { jp (e.tj), jp (e.bj) } : HelpText { e.te, e.be };
    return {};
}
