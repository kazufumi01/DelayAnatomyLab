#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DelayEngine.h"
#include "Machines.h"
#include "DemoSynth.h"
#include "Localisation.h"
#include "PresetStore.h"

class DelayAnatomyLabProcessor : public juce::AudioProcessor,
                                private juce::AudioProcessorValueTreeState::Listener,
                                private juce::AsyncUpdater
{
public:
    DelayAnatomyLabProcessor();
    ~DelayAnatomyLabProcessor() override;

    // --- オーディオ処理 ---
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;   // double版を隠さない（警告対策）

    // --- エディタ ---
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // --- 基本情報 ---
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    // --- プリセット（ホストの「プログラム」としても見える） ---
    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    // --- 状態の保存／復元（DAWのプロジェクトに保存される） ---
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // 機種ボタン：機種を切り替え、Age / Drive / Wow / Flutter をその機種らしい値にする
    void applyMachine (int machineIndex);
    // 表示の切替（BASIC=0 / ANALOG=1 / FULL=2）。下の表示にすると、隠れる機能は無効値に戻す
    void applyView (int view);
    void setParam (const char* id, float value);

    // --- プリセットの状態・ユーザープリセット ---
    PresetValues captureValues() const;                 // 今のパラメーター値
    void applyValues (const PresetValues& v);           // 値をまとめて反映（Wet Lock を尊重）
    PresetValues getBaseline() const;                   // 比較表の「初期値」＝選んだプリセットの値
    juce::String getPresetDisplayName() const;
    bool isUserPreset() const { return userPresetName.isNotEmpty(); }
    int  getPresetSerial() const { return presetSerial; }   // プリセットを読み込むたびに増える
    bool isModified() const;                            // 選んだプリセットから動かしてあるか
    bool saveUserPreset (const juce::String& name);     // 今の設定を名前を付けて保存（同名は上書き）
    bool loadUserPreset (const juce::File& file);
    bool renameUserPreset (const juce::String& newName);
    bool deleteUserPreset();
    bool importPreset (const juce::File& file);         // 外部の .dalpreset を取り込んで読み込む
    bool exportCurrent (const juce::File& file) const;
    void revertToPreset();                              // 選んだプリセットの値に戻す

    // Editor から参照する
    juce::AudioProcessorValueTreeState apvts;
    double getHostBpm() const { return hostBpm.load(); }
    DelaySettings currentSettings() const;   // 今のパラメーター値をまとめて取得

    // 表示言語（オートメーション不要なのでパラメーターにせず、状態の中に保存）
    Lang getLanguage() const
    {
        const auto v = apvts.state.getProperty ("lang", (int) systemDefaultLang());
        return (int) v == 0 ? Lang::Japanese : Lang::English;
    }
    void setLanguage (Lang l) { apvts.state.setProperty ("lang", (int) l, nullptr); }

    // 画面の大きさなど（ScalingEditor が "zoom" を読み書きする。セッションに保存）
    juce::ValueTree uiState { "UI" };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    // Bypass が ON になったら Wet Only / Wet Lock を OFF にする（メッセージスレッドで実行）
    void parameterChanged (const juce::String& id, float newValue) override;
    void handleAsyncUpdate() override;

    DelayEngine engine;
    juce::AudioBuffer<float> dryCopy;            // Bypass 用：処理前の音を保存
    juce::SmoothedValue<float> bypassMix;        // 0=エフェクト音, 1=入力音（クリック防止で20msクロスフェード）
    DemoSynth demo;
    std::atomic<double> hostBpm { 120.0 };
    int currentProgram = 0;                      // ファクトリーの番号（ユーザープリセットでは「元にしたプリセット」）
    juce::String userPresetName;                 // 空 = ファクトリー
    PresetValues userBaseline;
    int presetSerial = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayAnatomyLabProcessor)
};
