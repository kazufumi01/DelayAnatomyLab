#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Theme.h"
#include "ScalingEditor.h"
#include "Displays.h"
#include "ChoiceButtons.h"
#include "ParamCompare.h"
#include "FineSlider.h"
#include "SidePanel.h"
#include "Help.h"

// ヘッダーのプリセット名ボタン（右端に ▼ を描く）
class PresetNameButton : public juce::TextButton
{
public:
    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        juce::TextButton::paintButton (g, over, down);
        const auto r = getLocalBounds().toFloat();
        const float cx = r.getRight() - 14.0f, cy = r.getCentreY();
        juce::Path tri;
        tri.addTriangle (cx - 4.0f, cy - 2.0f, cx + 4.0f, cy - 2.0f, cx, cy + 3.0f);
        g.setColour (theme::muted);
        g.fillPath (tri);
    }
};

// =============================================================================
//  画面（左 960 × 860 は固定。右パネルを開くと右に 320 だけ広がる）
//    header 44 · PRESET · SAMPLE / MODE · MACHINE
//    · 図 3つを同時表示（TIMELINE ／ 原理図 ／ リピート特性）。●をドラッグして直接操作
//    · 値ストリップ（Wet バランス / Pitch / Duck / Reverse / スイッチ類）
//    · SYNC L / R
//    右パネル（開閉）：マウスオーバー説明 ／ プリセット解説 ／ 設定のコツ ／ 比較表
// =============================================================================
class DelayAnatomyLabView : public juce::Component,
                            private juce::Timer
{
public:
    static constexpr int leftW = 960, panelW = 320, fullH = 860;
    static int baseWidthFor (DelayAnatomyLabProcessor& p)
    {
        const bool open = p.uiState.hasProperty ("rightOpen") ? (bool) p.uiState.getProperty ("rightOpen") : true;
        return leftW + (open ? panelW : 0);
    }

    ZoomButton zoom;                       // ScalingEditor が操作する「100%」ボタン（サイズ＋配色メニュー）
    std::function<void (int, int)> requestBaseSize;     // 右パネルの開閉でウィンドウの基準サイズを変えてもらう

    explicit DelayAnatomyLabView (DelayAnatomyLabProcessor&);
    ~DelayAnatomyLabView() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent& e) override { updateHelp (e); }

private:
    void timerCallback() override;
    void showText();
    void applyLanguage (Lang l);
    void applySkin (int index);
    void updateHelp (const juce::MouseEvent& e);
    void setRightOpen (bool open);
    void refreshColours();
    void followSync (const char* timeId, const char* syncId, double bpm);
    void releaseSync (const char* syncId);

    // --- プリセット欄（ヘッダー）・保存・管理 ---
    void onPresetLoaded();
    void syncQuickButtons();
    void refreshPresetBar();
    void showPresetMenu();
    void showManageMenu();
    void stepPreset (int dir);
    void loadEntry (int i, const juce::Array<juce::File>& files);
    void doSave();
    void doSaveAs();
    void doRename();
    void doDelete();
    void doImport();
    void doExport();
    void openDialog (const juce::String& title, const juce::String& msg, const juce::String& okText,
                     const juce::String& defaultText, bool wantText, bool wantCancel,
                     std::function<void (bool, juce::String)> cb);
    juce::String tr (const char* ja, const char* en) const
    {
        return proc.getLanguage() == Lang::Japanese ? jp (ja) : juce::String (en);
    }

    struct Bar
    {
        FineSlider   slider;     // Shift + ドラッグで微調整
        juce::Label  label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    void setupBar (Bar& k, const char* id, const juce::String& name, const juce::String& suffix);
    void setStripPage (int page);

    DelayAnatomyLabProcessor& proc;
    theme::LookAndFeel lnf;

    juce::Label presetTitle, sampleTitle, modeTitle, machineTitle, syncLTitle, syncRTitle;
    ChoiceButtons presetButtons, sampleButtons, modeButtons, machineButtons, syncLButtons, syncRButtons, pitchModeButtons;
    juce::TextButton langBtn, infoBtn;

    FiguresPanel figures;
    ParamCompare compare;
    SidePanel side;
    HelpBox helpBox;
    TextBody presetBody, tipsBody;

    Bar timeL, timeR, feedback, hp, lp, rate, depth;              // BASICS（Delay Basics Lab と同じ並び）
    Bar wet;                                                      // Wet バランス（共通）
    Bar pitch, pitchFine, duck, duckRel;                          // FX
    juce::TextButton basicTab { "BASICS" }, fxTab { "FX" };
    juce::TextButton lfoBtn;                                      // LFO 波形（BASICS ページ）
    juce::Label      lfoLabel;
    int shownShape = -1;
    int stripPage = 0;                                            // 0 = BASICS, 1 = FX
    juce::TextButton delayOff { "Bypass" }, wetOnly { "Wet Only" }, wetLock { "Wet Lock" }, monoCheck { "Mono" };
    juce::TextButton reverse { "Reverse" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> offAtt, wetOnlyAtt, wetLockAtt, monoAtt, reverseAtt;

    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, saveBtn { "Save" }, manageBtn { "Manage" };
    PresetNameButton presetNameBtn;
    std::unique_ptr<juce::FileChooser> chooser;
    int shownSerial = -1;
    bool shownModified = false;

    bool rightOpen = true;
    int shownPreset = -1, shownMachine = -1;
    Lang shownLang = Lang::Japanese;
    juce::String readout;
    juce::Rectangle<int> stripPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayAnatomyLabView)
};

class DelayAnatomyLabEditor : public ScalingEditor<DelayAnatomyLabView, DelayAnatomyLabProcessor>
{
public:
    explicit DelayAnatomyLabEditor (DelayAnatomyLabProcessor& p)
        : ScalingEditor (p, DelayAnatomyLabView::baseWidthFor (p), DelayAnatomyLabView::fullH, "delayanatomylab.zoom") {}
};
