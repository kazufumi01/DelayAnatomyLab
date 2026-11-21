#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include "Params.h"

// =============================================================================
//  ファクトリープリセット（ブラウザ版の7プリセットと同じ値・同じ解説）
//  sync: 0=Off, 1=1/16, 2=1/8T, 3=1/8, 4=1/8D, 5=1/4T, 6=1/4, 7=1/4D, 8=1/2
// =============================================================================
struct FxState
{
    int   machine = 0;                         // Machine（-1 = 機種まわりは今のまま：Age/Drive/Wow/Flutter も触らない）
    float age = 0, drive = 0, wow = 0, flutter = 0;
    float pitch = 0, pitchFine = 0;
    int   pitchMode = 0;                       // 0=In Loop 1=Output
    bool  reverse = false;
    float duckDb = 0, duckRel = 250;
};

struct FactoryPreset
{
    const char* name;
    const char* tag;
    DelayMode mode;
    float timeL, timeR;
    int   syncL, syncR;
    float feedbackPct, hp, lp, rate, depth, wetDb;
    FxState fx;
    const char* what;
    const char* use;
    const char* ear;
    const char* plugins;
    int shape = 0;                              // LFO 波形（0=Sine 1=Triangle 2=Saw 3=Square 4=Random）
};

inline juce::String jp (const char* s) { return juce::String::fromUTF8 (s); }

inline const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> presets
    {
        { "Default", "汎用", DelayMode::Mono, 500, 500, 6, 6, 25, 200, 6000, 0.5f, 0.2f, -12, {},
          "4分音符のエコーが、少しずつ暗くなりながら3〜4回返ってくる。ディレイの“いつもの出発点”。",
          "ボーカル、ギター、シンセ、何にでも。センド（Wet Only）に挿して、まずこの状態から好みに寄せていく。",
          "Feedbackで回数、Low Passで奥行き、Wetで存在感。この3つだけで多くの曲に馴染む。動かした量は右の比較表で確認できる。",
          "" },

        { "Slapback", "存在感", DelayMode::Mono, 110, 110, 0, 0, 5, 200, 5000, 0.5f, 0.0f, -9, { -1 },
          "原音の約0.1秒後に、高域を少し落とした反射が1回だけ返ってくる。50年代のテープ・スラップが原型。",
          "ボーカル、ギター、スネア。リバーブほど空間を濁さずに「前に出つつ立体的」な存在感を足せる。",
          "Bypassと切り替えて、歌の輪郭が太る感覚を確認。100msを境に、短いほどタイト、長いほどロカビリー的に。",
          "Ableton Echo：Stereo・Link ON／110ms／Feedback 5%／HP 200Hz・LP 5kHz／Wobble 5〜10%\n"
          "Valhalla Delay：Tape または HiFi／Single／110ms／Feedback 0〜10%\n"
          "EchoBoy：Single Echo／Studio Tape／110ms／Feedback 5〜10%" },

        { "Doubling", "太さ", DelayMode::Stereo, 13, 21, 0, 0, 0, 150, 9000, 0.7f, 1.2f, -4, { -1 },
          "約30ms以内の遅れは「別のエコー」ではなく「同じ音の厚み」として融合して聴こえる（先行音効果）。L/Rで時間を変え、ゆっくり揺らすと二度録り風に。",
          "ボーカルの太さ、シンセ・ギターの広がり。ダブル録りの代用。",
          "Mono CheckをONにすると、短いディレイ特有のコムフィルターが出る。Depthを0にすると“揺れない厚み”の不自然さも分かる。",
          "Ableton Echo：Stereo・Link OFF／L 13ms・R 21ms／Feedback 0%／Modulation Rate 0.7Hz\n"
          "Valhalla Delay：Dual／13ms・21ms／Feedback 0%／HiFi（またはPitchでDetune ±8〜12cent）\n"
          "EchoBoy：Dual Echo／13ms・21ms／Feedback 0%／Studio Tape＋Wobble少し" },

        { "Haas Wide", "左右へ広げる", DelayMode::Haas, 12, 12, 0, 0, 0, 20, 20000, 0.5f, 0.0f, 0, { -1 },
          "左は原音、右だけ12ms遅らせる。耳は早く届いた左に定位を感じつつ、右からも音が来るため横に広がって聴こえる。",
          "モノ素材（ギター、パッド、コーラス）をステレオに広げたい時。※トラックにインサートで使う。",
          "必ずMono CheckをON。L＋Rが足されると特定の周波数が打ち消し合い、音がペラペラになる。これがHaasの落とし穴。",
          "Ableton Delay：インサート／Link OFF／L 1ms・R 12ms／Feedback 0%／Dry/Wet 100%\n"
          "Valhalla Delay：Dual／L 0ms・R 12ms／Feedback 0%／Mix 100%\n"
          "EchoBoy：Dual Echoで片側だけ10〜20ms、Mix 100%" },

        { "Ping-Pong", "動き", DelayMode::PingPong, 250, 250, 3, 3, 45, 250, 4500, 0.5f, 0.0f, -11, { -1 },
          "エコーが左→右→左…と交互に返る。片側のフィードバックをもう片側へ入れる配線（クロスフィードバック）。",
          "シンセのアルペジオ、ギターのリフ、ボーカルの語尾。中央の主役を邪魔せず、左右に動きを作れる。",
          "タイムラインで青と赤が交互に並ぶのを確認。HPを250Hz以上にすると、エコーがベースやキックと喧嘩しない。",
          "Ableton Echo：Ping Pong／Sync 1/8／Feedback 40〜50%／HP 250Hz・LP 4.5kHz\n"
          "Valhalla Delay：PingPong／L・R 1/8／Feedback 45%\n"
          "EchoBoy：Ping-Pong／1/8 Sync／Feedback 45%" },

        { "Dotted 8th+1/4", "グルーヴ", DelayMode::Stereo, 300, 300, 4, 6, 35, 300, 3500, 0.4f, 0.3f, -12, { -1 },
          "左に付点8分、右に4分。ズレた2つのリズムが重なって、フレーズの隙間に16分の“合いの手”が生まれる。",
          "シンプルな8分のギターやシンセを細かいリズムに変える定番。ボーカルのロングトーンの後ろにも。",
          "タイムラインの拍線に対して、付点8分のエコーが裏に入るのを見る。LPを下げるほど、エコーが原音の後ろに下がる。",
          "Ableton Echo：Stereo・Link OFF／L 1/8D・R 1/4／Feedback 35%／LP 3.5kHz\n"
          "Valhalla Delay：Dual／L 1/8D・R 1/4／Feedback 35%／High Cut 3.5kHz\n"
          "EchoBoy：Dual Echo／L 1/8D・R 1/4／Feedback 35%" },

        { "Dub Throw", "質感・展開", DelayMode::Mono, 500, 500, 7, 7, 72, 450, 2200, 0.35f, 0.8f, -8, { -1 },
          "フィードバックを上げ、ループ内のHP/LPで1回ごとに帯域を狭める。リピートは細く・暗く・揺れながら消えていく。",
          "語尾やスネアの“投げ”（ディレイ・スロー）。センド量を1拍だけ上げるオートメーションで展開を作る。",
          "タイムラインの色が暗くなる＝高域が削れていく様子に注目。Feedback 90%付近は発振寸前（音量注意）。",
          "Ableton Echo：Sync 1/4D／Feedback 70%／HP 450Hz・LP 2.2kHz／Wobble 20%・Noise少量\n"
          "Valhalla Delay：Tape／Feedback 70%／Age 30%前後／Low Cut 450Hz・High Cut 2.2kHz\n"
          "EchoBoy：Space Echo／Feedback 70%／Saturationを上げて汚れを足す" },

        { "Short Ambience", "リバーブとの境界", DelayMode::Stereo, 43, 67, 0, 0, 48, 300, 3200, 0.3f, 0.6f, -10, { -1 },
          "40〜80msの短いエコーを左右別時間でフィードバックさせると、反射が密集して部屋鳴りのように聴こえ始める。",
          "リバーブだと奥に引っ込みすぎる時の“近い空気感”。ドラムやアコギを濁らせずに馴染ませる。",
          "Feedbackを0→60%と動かし、どこから「エコー」が「響き」に変わるかを探す。",
          "Ableton Echo：Stereo・Link OFF／L 43ms・R 67ms／Feedback 45〜50%／Reverb少量\n"
          "Valhalla Delay：Ratio（61.8%前後）／Feedback 50%／Diffusionで疑似リバーブ化\n"
          "EchoBoy：Dual Echo／43ms・67ms／Feedback 45%／Style Tweak › Diffusion" },
        { "Reverse Swell", "逆再生", DelayMode::Mono, 400, 400, 0, 0, 30, 200, 6000, 0.5f, 0.0f, -8,
          { -1, 0, 0, 0, 0, 0, 0, 0, true, 0, 250 },
          "Time の長さで切り取った音を逆再生して返す。音が「小さく始まって、最後に一番大きくなる」逆向きのエコーになる。",
          "ボーカルの語尾の前に仕込む逆回転感、スネアやシンバルのスウェル。展開の頭にも。",
          "Reverse を OFF/ON で比べる。Time を長くすると逆再生の“助走”が長くなる。Feedback を上げると逆再生が重なって幻想的に。",
          "Valhalla Delay：RevPitch の Reverse／Feedback 30%／Tape\n"
          "FabFilter Timeless 3：Reverse 系の FX（Feedback）／Time 400ms\n"
          "EchoBoy：Reverse Echo（Style Edit）" },

        { "Shimmer Echo", "ピッチ", DelayMode::Mono, 500, 500, 6, 6, 55, 250, 7000, 0.5f, 0.3f, -10,
          { -1, 0, 0, 0, 0, 12, 0, 0, false, 0, 250 },
          "リピートのたびに1オクターブずつ上がる。1回目＋12、2回目＋24 半音…と積み上がり、キラキラした尾になる（Pitch = In Loop）。",
          "パッド、ボーカルのロングトーン、アンビエント。LPで上が出過ぎないよう整える。",
          "Pitch Mode を Output にすると、全リピートが同じ高さ（＋12）に揃う。In Loop との違いがピッチシフトの肝。",
          "Valhalla Delay：Pitch MODE（Feedback 内）／＋12 st\n"
          "FabFilter Timeless 3：Pitch（Feedback effect）＋12／Feedback 55%\n"
          "EchoBoy：Shimmer 系 Style / Pitch" },

        { "Ducked Vocal", "ダッキング", DelayMode::Mono, 375, 375, 4, 4, 35, 250, 5000, 0.5f, 0.2f, -6,
          { -1, 0, 0, 0, 0, 0, 0, 0, false, 12, 250 },
          "歌っている間は Wet を最大12dB下げ、歌が止まるとエコーが立ち上がる。エコーが歌詞を邪魔しない定番のかけ方。",
          "ボーカル、ラップ、ナレーション。Wet を大きめにしても言葉が聞き取れる。",
          "Duck を 0 → 12 dB と動かして、語頭と語尾の間のエコーの出方を聴く。Release が短いほど早く戻り、長いほど控えめになる。",
          "FabFilter Timeless 3：Dynamics（Ducking）\n"
          "Valhalla Delay：専用なし（サイドチェインコンプで代用）\n"
          "EchoBoy：Ducking（Tweak）" },

        { "Tape Slap", "テープ・スラップ", DelayMode::Mono, 110, 110, 0, 0, 8, 150, 9000, 0.5f, 0.0f, -7, { 1, 25, 35, 10, 8, 0, 0, 0, false, 0, 250 },
          "Studio Tape の短いエコーを1回だけ返す。Drive がピークを丸め、高域が少し落ちて、原音の後ろにテープの温かさが付く。",
          "ボーカルやアコギを前に出したい時。Clean の Slapback との違いは、エコーの角が丸いこと。",
          "Clean の Slapback と交互に切り替えて、Drive と Age で高域と歯切れがどう変わるかを聴く。",
          "" },

        { "Tape Vocal Echo", "ボーカル定番", DelayMode::Mono, 250, 250, 3, 3, 30, 250, 7000, 0.5f, 0.1f, -10, { 1, 30, 30, 12, 8, 0, 0, 0, false, 0, 250 },
          "1/8音符のエコーを3〜4回。リピートごとにテープの高域ロスが積み重なり、歌の後ろへ自然に溶けていく。",
          "ボーカルのセンドに。Wet Only で使うのが基本。",
          "Feedback で回数、Age でどれだけ暗く溶けるかが決まる。Duck を足すと歌詞が聞き取りやすい（FULL表示）。",
          "" },

        { "Tape Saturated Throw", "飽和エコー", DelayMode::Mono, 500, 500, 6, 6, 55, 300, 5000, 0.4f, 0.3f, -9, { 1, 55, 70, 18, 12, 0, 0, 0, false, 0, 250 },
          "テープを強く押し込んだ（Drive 70%）1/4のエコー。リピートごとに飽和が重なり、細く歪みながら消えていく。",
          "ドラムの打ち込みやギターのスタブに。センドで1拍だけ上げる『投げ』に向く。",
          "リピートが進むほど歪みと暗さが増す。Clean の Dub Throw との違いは、歪みながら消えること。",
          "" },

        { "Echoplex Rockabilly", "ロカビリー", DelayMode::Mono, 140, 140, 0, 0, 20, 200, 6000, 0.5f, 0.0f, -6, { 2, 40, 55, 15, 12, 0, 0, 0, false, 0, 250 },
          "約140msの短いエコーに、プリアンプ的な Drive と軽い揺れ。50〜60年代のロックで使われた、太くて少しうねるスラップ。",
          "ギターソロ、ボーカルのスラップ。",
          "Tape Slap より Drive が強く Flutter が深い。ピッチが細かく震える。",
          "" },

        { "Echoplex Wobble", "ふらつき", DelayMode::Mono, 330, 330, 0, 0, 40, 250, 4500, 0.5f, 0.3f, -9, { 2, 60, 45, 70, 55, 0, 0, 0, false, 0, 250 },
          "Wow と Flutter を深くしたエコー。リピートごとにピッチが上下にふらつく、傷んだテープのような質感。",
          "ダブ、サイケ、ギターのアンビエンス。",
          "Wow（遅い揺れ）と Flutter（細かい揺れ）を別々に動かして、違いを聴き分ける。",
          "" },

        { "Echoplex Sound-on-Sound", "重ね録り風", DelayMode::Mono, 500, 500, 0, 0, 88, 300, 3500, 0.3f, 0.5f, -10, { 2, 50, 60, 25, 18, 0, 0, 0, false, 0, 250 },
          "Feedback 88%：エコーがほとんど減衰せず、自己発振に近づく。高域がリピートのたびに削れ、重ね録りのような厚い塊になる。",
          "ギターのフレーズを重ねるループ風、効果音的な演出。",
          "Feedback は音量に注意。ループ内の HP/LP が暴走を抑えている。",
          "" },

        { "Space Echo Dub", "ダブ", DelayMode::Mono, 750, 750, 7, 7, 62, 350, 3200, 0.35f, 0.6f, -8, { 3, 45, 50, 30, 22, 0, 0, 0, false, 0, 250 },
          "3ヘッド（1:2:3）が1回のリピートに3つの音を並べ、Feedback 62% で繰り返す。ダブ定番の、跳ねるように重なるエコー。",
          "スネア、スキャット、ギターのカッティング。",
          "TIMELINE で、リピートごとに3本ずつ並ぶ細い棒（タップ）を確認する。",
          "" },

        { "Space Echo Rhythm", "リズム", DelayMode::Stereo, 300, 500, 4, 6, 35, 250, 4500, 0.5f, 0.3f, -9, { 3, 35, 35, 18, 14, 0, 0, 0, false, 0, 250 },
          "L は付点8分、R は4分で3ヘッド。タップが左右で別のリズムを刻み、細かい『ばらけ』が生まれる。",
          "シンプルなコード、アルペジオ。",
          "Dotted 8th+1/4（Clean）と比べて、タップが加わって音符が増える。",
          "" },

        { "Space Echo Ambient", "アンビエント", DelayMode::Mono, 480, 480, 0, 0, 70, 400, 3000, 0.25f, 1.0f, -10, { 3, 55, 40, 35, 25, 0, 0, 0, false, 0, 250 },
          "Time 480 ms・Feedback 70% で3つのタップが折り重なり、ゆっくり揺れながら空間に溶ける。",
          "パッド、ボーカルのロングトーン。",
          "Mod Depth と Wow が同時に効いて、リピートが漂う。",
          "" },

        { "BBD Chorus Echo", "コーラス風", DelayMode::Mono, 45, 45, 0, 0, 15, 100, 8000, 0.6f, 2.5f, -6, { 4, 30, 20, 0, 0, 0, 0, 0, false, 0, 250 },
          "BBD の短い Time（45 ms）に深い Mod（2.5 ms）。ディレイ音がコーラスのように揺れて広がる。",
          "シンセ、クリーンギター、エレピ。",
          "Mod Depth を 0 にすると、コーラス感が消えてただの短いエコーになる。",
          "" },

        { "BBD Memory Style", "定番", DelayMode::Mono, 350, 350, 0, 0, 62, 250, 4500, 0.5f, 1.5f, -9, { 4, 45, 35, 0, 0, 0, 0, 0, false, 0, 250 },
          "350 ms・Feedback 62% の暗く甘いエコー。揺れながら、リピートごとに丸くなる。BBD ペダルの定番的な設定。",
          "ギターのソロ、リード。",
          "リピートが進むほど高域がなくなり、温かい残り香になる。",
          "" },

        { "BBD Dark Dub", "暗いダブ", DelayMode::Mono, 500, 500, 6, 6, 70, 400, 3000, 0.3f, 1.2f, -8, { 4, 70, 40, 0, 0, 0, 0, 0, false, 0, 250 },
          "1/4・Feedback 70%・LP 3000 Hz。BBD の暗さと Age で、高域が消えた低くこもったダブ。",
          "スネアの投げ、ボーカルの語尾。",
          "Clean の Dub Throw より輪郭がなくなり、霧のように消える。",
          "" },

        { "Oil Can Wash", "にじみ", DelayMode::Mono, 220, 220, 0, 0, 60, 300, 4000, 0.4f, 0.8f, -9, { 5, 75, 25, 10, 5, 0, 0, 0, false, 0, 250 },
          "Oil Can の Age 75% で、リピートの輪郭がオールパスでにじむ。エコーがリバーブに溶けていく。",
          "パッド、ボーカルの空間作り。",
          "Age を下げるとエコーが輪郭を取り戻す。Age が Diffusion を兼ねている。",
          "" },

        { "Oil Can Slap", "スラップ", DelayMode::Mono, 90, 90, 0, 0, 20, 200, 5500, 0.5f, 0.3f, -8, { 5, 40, 40, 8, 4, 0, 0, 0, false, 0, 250 },
          "90 ms の短い Oil Can。ほんの少しにじんだ、柔らかいスラップ。",
          "ボーカル、スネア。",
          "テープのスラップよりエッジが柔らかく、少し水っぽい。",
          "" },

        { "Digital Clean Quarter", "クリーン", DelayMode::Mono, 500, 500, 6, 6, 30, 200, 12000, 0.5f, 0.0f, -10, { 6, 0, 0, 0, 0, 0, 0, 0, false, 0, 250 },
          "Early Digital を Age 0%（16bit・フルレート）で。劣化しないクリーンなデジタル・エコー。",
          "80年代風のタイトなエコー、リズムの正確な反復。",
          "アナログ機種と比べると、減衰以外は原音そのままなのが分かる。",
          "" },

        { "Digital Lo-Fi 8bit", "ローファイ", DelayMode::Mono, 250, 250, 3, 3, 45, 250, 8000, 0.5f, 0.0f, -9, { 6, 100, 0, 0, 0, 0, 0, 0, false, 0, 250 },
          "Age 100%（8bit・サンプルレート1/4）。粗くざらついたエコーが返る。劣化が暗くではなく粗くなる。",
          "ローファイなビート、ゲーム風サウンド。",
          "ここで Age を下げていくと、量子化ノイズが減る様子が聴ける。",
          "" },

        { "Digital 80s Ping-Pong", "80年代", DelayMode::PingPong, 250, 250, 3, 3, 40, 200, 9000, 0.8f, 0.3f, -10, { 6, 40, 0, 0, 0, 0, 0, 0, false, 0, 250 },
          "Early Digital の左右ピンポン（1/8）。Age 40% で少し粗く、80年代シンセ・ポップの質感。",
          "シンセ、パッド、ギター。",
          "Clean の Ping-Pong より粒が粗く、80年代のデジタルの冷たさがある。",
          "" },

        { "Dotted-8th Rock", "ロックの定番", DelayMode::Mono, 375, 375, 4, 4, 30, 250, 6000, 0.5f, 0.2f, -9, { -1, 0, 0, 0, 0, 0, 0, 0, false, 0, 250 },
          "付点8分・Feedback 30%・LP 6kHz。ギターが倍の音符数に聞こえる、ロックの定番ディレイ。",
          "ギターのアルペジオ・リフ。",
          "テンポを動かしても付点8分に同期して追従する。",
          "" },

        { "Vocal Throw", "ボーカル投げ", DelayMode::Mono, 500, 500, 6, 6, 40, 300, 5000, 0.5f, 0.2f, -4, { -1, 0, 0, 0, 0, 0, 0, 0, false, 10, 300 },
          "1/4・Feedback 40%・Duck 10 dB。歌っている間は控えめで、語尾でエコーが立ち上がる。",
          "ボーカル、ナレーション。",
          "Duck を 0 にすると歌詞に被って聞き取りにくくなる。",
          "" },

        { "Reverse Vocal Lead-in", "逆回転", DelayMode::Mono, 600, 600, 0, 0, 0, 200, 7000, 0.5f, 0.0f, -6, { -1, 0, 0, 0, 0, 0, 0, 0, true, 0, 250 },
          "600 ms・Feedback 0% の逆再生。歌い出しの前に逆回転のスウェルが入る。",
          "ボーカルの歌い出し、サビ前。",
          "Time がそのまま逆再生の長さ。1小節前から仕込む感覚で。",
          "" },

        { "Shimmer Pad", "きらめき", DelayMode::Stereo, 417, 500, 5, 6, 60, 300, 6500, 0.3f, 0.8f, -9, { -1, 0, 0, 0, 0, 12, 0, 0, false, 0, 250 },
          "L/R を少しずらしたタイム（1/4T と 1/4）に +12 st In Loop。左右にきらめきが広がる。",
          "アンビエントなパッド、エレピ。",
          "In Loop なので、リピートごとに1オクターブずつ高く積み上がる。",
          "" },

        { "Octave-Down Dub", "オクターブ下", DelayMode::Mono, 500, 500, 6, 6, 55, 200, 3500, 0.4f, 0.5f, -8, { -1, 0, 0, 0, 0, -12, 0, 0, false, 0, 250 },
          "リピートごとに1オクターブずつ下がる。低く沈んでいく重いダブ。",
          "ベース感のあるシンセ、ドラム。",
          "−12, −24…と下がるので、数回で聞こえなくなる（低域に注意）。",
          "" },

        { "Pitch Detune Wide", "ピッチ・ダブラー", DelayMode::Mono, 28, 28, 0, 0, 0, 150, 9000, 0.5f, 0.4f, -3, { -1, 0, 0, 0, 0, 0, 14, 1, false, 0, 250 },
          "28 ms の短いディレイを +14 cent ずらして足す。二度録りのような広がりを作る。",
          "ボーカル、シンセ。",
          "Fine が 0 だと単なる短いディレイ。ずらすとコーラスのような太さになる。",
          "" },

        // ---- 使い分け（34〜45）：方式の聴き比べ A/B と、場面別の例 ----
        { "A/B 1 Clean", "方式比較 1/7", DelayMode::Mono, 300, 300, 0, 0, 40, 200, 12000, 0.5, 0, -9, { 0, 0, 0, 0, 0, 0, 0, 0, false, 0, 250 },
          "300 ms・Feedback 40%・LP 全開。劣化のない基準の音。7つの「A/B」プリセットは Time・Feedback・HP/LP・Wet がすべて同じで、違うのは方式（Machine）だけ。",
          "他の方式と切り替えて「何が足されたか」を聴くためのものさし。",
          "リピートが同じ音色のまま、同じ減り方で返ってくる。このあと A/B 2〜7 に切り替えて、暗さ・歪み・揺れ・にじみ・粗さの違いを聴き分ける。",
          "" },
        { "A/B 2 Studio Tape", "方式比較 2/7", DelayMode::Mono, 300, 300, 0, 0, 40, 200, 12000, 0.5, 0, -9, { 1, 25, 30, 10, 8, 0, 0, 0, false, 0, 250 },
          "同じ設定で Studio Tape。Drive がピークを丸め、Age で高域が少し丸くなる。リピートを重ねるたびに少しずつ暗くなる。",
          "「ボーカルに自然に馴染むエコー」の基準。A/B 1 との差が小さく上品なのが特徴。",
          "A/B 1 と切り替えて、アタックの角が丸くなること、リピートが後ろへ溶けていくことを聴く。Wow/Flutter は控えめなので、揺れよりも『丸さ』の差。",
          "" },
        { "A/B 3 Echoplex", "方式比較 3/7", DelayMode::Mono, 300, 300, 0, 0, 40, 200, 12000, 0.5, 0, -9, { 2, 45, 45, 20, 15, 0, 0, 0, false, 0, 250 },
          "同じ設定で Echoplex。ヒス風の暗さ、プリアンプの押し込み（Drive）、Wow/Flutter の揺れが一度に乗る。",
          "「古いテープエコー」らしさがはっきり出る方式。ギターやダブ、歪んだ質感が欲しい時。",
          "A/B 2 より明らかに暗く・太く・揺れる。リピートごとに歪みが積み重なるので、後ろのリピートほど荒くなる。",
          "" },
        { "A/B 4 Space Echo", "方式比較 4/7", DelayMode::Mono, 300, 300, 0, 0, 40, 200, 12000, 0.5, 0, -9, { 3, 40, 40, 25, 20, 0, 0, 0, false, 0, 250 },
          "同じ設定で Space Echo。リピートのあいだに複数のヘッド（タップ）が細かい反射を足し、同じ 300 ms でも密度が増す。",
          "「リピートが1本ではなく、小さな群れになる」質感。パッド、アルペジオ、ギターの奥行きに。",
          "A/B 3 と同じくらい暗いが、反射が複数あるぶん『ひと塊』に聞こえる。A/B 1 の一本ずつのリピートと比べると、リズムの輪郭が少し曖昧になる。",
          "" },
        { "A/B 5 BBD", "方式比較 5/7", DelayMode::Mono, 300, 300, 0, 0, 40, 200, 12000, 0.5, 0, -9, { 4, 40, 25, 0, 0, 0, 0, 0, false, 0, 250 },
          "同じ設定で BBD。Time が長いほどクロックが遅くなり、高域が出なくなる。300 ms では A/B 2〜4 よりもはっきり暗く、甘い。",
          "「暗くて甘いアナログ」の基準。シンセ、ギター、エレピを後ろに置きたい時。",
          "同じ LP 全開でも、BBD 自身が高域を削っている。Time を短くすると明るくなり、長くすると暗くなる（Time と音色が連動）。",
          "" },
        { "A/B 6 Oil Can", "方式比較 6/7", DelayMode::Mono, 300, 300, 0, 0, 40, 200, 12000, 0.5, 0, -9, { 5, 50, 30, 10, 5, 0, 0, 0, false, 0, 250 },
          "同じ設定で Oil Can。静電ドラムの「にじみ（Diffusion）」で、リピートが輪郭を失ってリバーブのように溶ける。",
          "エコーとリバーブの中間。『残響っぽいエコー』『SF 的な浮遊感』が欲しい時。",
          "A/B 1 のくっきりした一本ずつのリピートが、ぼやけた塊になる。Feedback を上げると、エコーというより空間の響きに変わる。",
          "" },
        { "A/B 7 Early Digital", "方式比較 7/7", DelayMode::Mono, 300, 300, 0, 0, 40, 200, 12000, 0.5, 0, -9, { 6, 50, 0, 0, 0, 0, 0, 0, false, 0, 250 },
          "同じ設定で Early Digital。Age 50% は 12 bit・約半分のサンプルレート相当。暗くはならず、ザラつく。",
          "80s シンセポップやローファイ。『暗い』ではなく『粗い』劣化の代表。",
          "A/B 5（暗さ）との違いを聴く：ここでは高域が残ったまま、量子化ノイズでざらつく。リピートが進むほどノイズが積み重なる。",
          "" },
        { "Scene Vocal Behind", "歌の後ろ", DelayMode::Mono, 500, 500, 6, 6, 25, 300, 5000, 0.5, 0.2, -14, { 0, 0, 0, 0, 0, 0, 0, 0, false, 6, 300 },
          "Clean の 1/4 音符・Feedback 25%・HP 300 Hz・LP 5 kHz・Wet −14 dB。さらに Duck 6 dB で、歌っている間はエコーが引っ込む。",
          "歌詞を邪魔せずに『歌の後ろに空間が広がる』ようにしたい時の標準。テンポ同期なので曲に溶ける。",
          "Bypass と比べて歌の輪郭が変わらないこと、フレーズの切れ目でエコーが咲くことを聴く。Duck を 0 にすると歌詞に被る。",
          "" },
        { "Scene Guitar Space Echo", "ギターの奥行き", DelayMode::Mono, 450, 450, 0, 0, 45, 250, 7000, 0.5, 0.3, -10, { 3, 40, 40, 25, 20, 0, 0, 0, false, 0, 250 },
          "Space Echo・450 ms・Feedback 45%。複数ヘッドの反射が、ギターのフレーズの後ろに小さな群れを作る。",
          "クリーンギター、アルペジオ、ローズなど。単音エコーだと音が離れすぎる時に、密度のある奥行きを足せる。",
          "Clean に切り替えるとリピートが1本ずつに戻り、フレーズの隙間が空いて聞こえる。Space Echo の方が空間が『埋まる』。",
          "" },
        { "Scene Dub Throw", "ダブのスロー", DelayMode::Mono, 450, 450, 7, 7, 70, 450, 2500, 0.5, 0.2, -8, { 2, 55, 55, 20, 15, 0, 0, 0, false, 0, 250 },
          "Echoplex・付点4分・Feedback 70%・HP 450 Hz・LP 2500 Hz。暗く歪んだリピートが積み重なる。",
          "ダブ、スネアやボーカルの『投げ』。センドに入れて Wet を手で上げ下げする。",
          "リピートが重なるたびに暗く・荒くなる。Time を動かすと、動かしている間だけピッチが滑って『テープが回っている』感じが出る。",
          "" },
        { "Scene Dark Synth BBD", "暗いシンセ", DelayMode::Stereo, 180, 270, 0, 0, 50, 200, 6000, 0.6, 2.5, -9, { 4, 40, 25, 0, 0, 0, 0, 0, false, 0, 250 },
          "BBD・L 180 ms／R 270 ms・Mod Depth 2.5 ms。長いほど暗くなる BBD の性質で、左右の暗さも少し違う。コーラス風の揺れが付く。",
          "シンセのリード、パッド、エレピ。後ろに置きたいが存在感は残したい時。",
          "R の方が長いので R の方が暗い。Mod Depth を 0 にすると揺れが消えて、ただの暗いエコーに。",
          "" },
        { "Scene Lo-fi Beat Digital", "ローファイ・ビート", DelayMode::PingPong, 250, 250, 3, 3, 40, 300, 5000, 0.5, 0, -10, { 6, 70, 0, 0, 0, 0, 0, 0, false, 0, 250 },
          "Early Digital・Ping-Pong 1/8・Age 70%。ビット数とサンプルレートが落ちた粗いエコーが左右に跳ねる。",
          "ローファイなビート、ゲーム風サウンド、ドラムのアクセント。",
          "暗くなるのではなく、粗くザラつく。Age を下げると粒立ちが戻る。BBD との『暗さ vs 粗さ』の違いを意識して聴く。",
          "" },
        { "Scene Melt Oil Can", "エコーを溶かす", DelayMode::Mono, 200, 200, 0, 0, 65, 250, 6000, 0.5, 0.3, -8, { 5, 70, 30, 10, 5, 0, 0, 0, false, 0, 250 },
          "Oil Can・200 ms・Feedback 65%・Age 70%。にじみ（Diffusion）が積み重なり、エコーの輪郭が消えて響きになる。",
          "エコーとリバーブの中間が欲しい時。パッド、ギター、ドラムの残響感を『エコーの延長』として足す。",
          "Clean に切り替えるとリピートがくっきり一本ずつに戻る。Oil Can ではリピートが溶けて、どこで切れたか分からなくなる。",
          "" },
        { "Chorus", "コーラス", DelayMode::Mono, 20, 20, 0, 0, 0, 20, 14000, 0.6f, 2.5f, -6, {},
          "Time 20 ms を 0.6 Hz・±2.5 ms でゆっくり揺らし、原音と混ぜる。遅れ時間が変わるたびにピッチが ±16 cent ほど上下し、二重に鳴っているような厚みとうねりが生まれる。Feedback は 0 %。",
          "ギター、エレピ、パッド、シンセ。ダブリングより『揺れる』厚みが欲しい時。Wet Only は切って、ドライと混ぜて使う。",
          "Wet Only を ON にすると揺れだけのピッチ変化が聴こえ、OFF（ドライと混ざる）で初めてコーラスらしいうねりになる。Mod Depth を 0 にすると、ただのダブリングに戻る。",
          "" },
        { "Chorus Wide", "コーラス（Stereo）", DelayMode::Stereo, 18, 24, 0, 0, 0, 20, 14000, 0.5f, 3.0f, -6, {},
          "Stereo で L 18 ms／R 24 ms。揺れは L と R で逆位相（L が遅くなる時 R は早くなる）に動くので、左右のピッチの上下が逆になり、モノのコーラスより大きく広がる。",
          "モノのギターやシンセを左右に広げたい時。Haas Wide と違い、コムフィルターの谷が動き続けるので、特定の帯域だけが痩せ続けない。",
          "Mono Check を ON にすると L/R が足されて、広がりと引き換えにうねりの聴こえ方が変わる（モノ互換の確認）。Time R を 24→18 ms に揃えても、逆位相の揺れで広がりは残る。",
          "" },
        { "Flanger", "フランジャー", DelayMode::Mono, 3, 3, 0, 0, 65, 20, 12000, 0.25f, 2.0f, -4, {},
          "Time 3 ms と極端に短い遅れを 0.25 Hz・±2 ms で揺らし、Feedback 65 % で戻す。原音との合成でコムフィルター（約 330 Hz 間隔の谷。1000÷3 ms）が生まれ、その谷が上下に動いて“ジェット機”のような音になる。",
          "ギター、ハイハットやドラムループ、シンセ。ミックスの一部に動きを足す定番エフェクト。",
          "Feedback を 0 にすると質感はコーラスに近づく。65→85 % にすると谷と山がくっきりして金属的に。Mod Depth を Time より大きくすると下限で止まり音が潰れる。",
          "" },
        { "Flanger Wide", "フランジャー（Stereo）", DelayMode::Stereo, 2.6f, 3.4f, 0, 0, 60, 20, 12000, 0.2f, 1.8f, -4, {},
          "Stereo で L 2.6 ms／R 3.4 ms。L と R のスイープが逆位相で動くため、谷が左右で反対方向に動き、音が左右を回るように聴こえる。",
          "ステレオのドラムループ、シンセ、ブレイク前後の演出。",
          "Mono Check で L/R を足すと、逆に動いていた谷が混ざって聴こえ方が変わる（モノ互換の確認）。Mod Rate を 0.2→0.1 Hz にするとゆっくり回る。",
          "" },
        { "Vintage Chorus", "古いコーラス（BBD）", DelayMode::Stereo, 20, 20, 0, 0, 0, 100, 7000, 0.7f, 3.0f, -6, { 4, 30, 25, 0, 0, 0, 0, 0, false, 0, 250 },
          "BBD・Stereo・Time 20 ms・LFO は三角波 0.7 Hz・±3 ms。三角波は上り下りの速さが一定なので、ピッチの揺れが一定幅（約 ±17 cent）で往復する。BBD の暗い高域、ノイズ、軽い歪みが重なり、70〜80年代のコーラス・ペダルの質感になる。",
          "クリーンギター、エレピ、シンセ・ストリングス。Clean のコーラスより暗く温かい。",
          "LFO Shape を Sine に変えると折り返しが滑らかでモダンに。Machine を Clean にすると、BBD の暗さ・ノイズ・歪みが何を足していたか分かる。Age 30→70 % でさらに古く。",
          "", 1 },
    };
    return presets;
}

// 上段のクイックボタンに並べる数（基本の11個）。残りはヘッダーのプリセット欄から選ぶ
inline constexpr int kQuickPresets = 11;

struct PresetCategory { const char* ja; const char* en; std::vector<int> ids; };
inline const std::vector<PresetCategory>& presetCategories()
{
    static const std::vector<PresetCategory> c
    {
        { "基本",             "Basic",           { 0, 1, 2, 3, 4, 5, 6, 7 } },
        { "使い分け：方式の聴き比べ（A/B）", "Choosing: A/B by machine", { 34, 35, 36, 37, 38, 39, 40 } },
        { "使い分け：場面別の例（10）", "Choosing: by scene (10)", { 41, 1, 2, 4, 42, 43, 44, 46, 45, 9 } },
        { "定番",             "Classic staples", { 28, 29 } },
        { "コーラス／フランジャー（LFO）", "Chorus / Flanger (LFO)", { 47, 48, 49, 50, 51 } },
        { "FX（ピッチ・逆再生・ダッキング）", "FX (pitch / reverse / duck)", { 8, 9, 10, 30, 31, 32, 33 } },
        { "Studio Tape",      "Studio Tape",     { 11, 12, 13 } },
        { "Echoplex",         "Echoplex",        { 14, 15, 16 } },
        { "Space Echo",       "Space Echo",      { 17, 18, 19 } },
        { "BBD",              "BBD",             { 20, 21, 22 } },
        { "Oil Can",          "Oil Can",         { 23, 24 } },
        { "Early Digital",    "Early Digital",   { 25, 26, 27 } },
    };
    return c;
}

// プリセット1個ぶんの値（ファクトリー／ユーザー共通）。比較表・保存・読み込みで使う
struct PresetValues
{
    int   mode = 0, syncL = 0, syncR = 0;
    float timeL = 500, timeR = 500, feedback = 25, hp = 200, lp = 6000, rate = 0.5f, depth = 0.2f, wet = -12;
    int   lfoShape = 0;
    bool  wetOnly = false;
    bool  keepMachine = false;                  // true = Machine / Age / Drive / Wow / Flutter を触らない
    int   machine = 0;
    float age = 0, drive = 0, wow = 0, flutter = 0;
    float pitch = 0, pitchFine = 0;
    int   pitchMode = 0;
    bool  reverse = false;
    float duck = 0, duckRel = 250;
};

inline PresetValues valuesFromFactory (int index)
{
    const auto& p = factoryPresets()[(size_t) juce::jlimit (0, (int) factoryPresets().size() - 1, index)];
    PresetValues v;
    v.mode = (int) p.mode;
    v.syncL = p.syncL;  v.syncR = p.syncR;
    v.timeL = p.timeL;  v.timeR = p.timeR;
    v.feedback = p.feedbackPct;
    v.hp = p.hp;  v.lp = p.lp;  v.rate = p.rate;  v.depth = p.depth;  v.wet = p.wetDb;
    v.lfoShape = p.shape;
    v.keepMachine = p.fx.machine < 0;
    v.machine = juce::jmax (0, p.fx.machine);
    v.age = p.fx.age;  v.drive = p.fx.drive;  v.wow = p.fx.wow;  v.flutter = p.fx.flutter;
    v.pitch = p.fx.pitch;  v.pitchFine = p.fx.pitchFine;  v.pitchMode = p.fx.pitchMode;
    v.reverse = p.fx.reverse;
    v.duck = p.fx.duckDb;  v.duckRel = p.fx.duckRel;
    return v;
}
