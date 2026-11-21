#include "PluginEditor.h"
#include "Presets.h"

namespace
{
    juce::StringArray presetLabels()
    {
        juce::StringArray s;
        for (int i = 0; i < kQuickPresets; ++i)
            s.add (factoryPresets()[(size_t) i].name);
        return s;
    }

    juce::StringArray shortSyncLabels()  { return { "ms", "1/16", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2" }; }
    juce::StringArray shortModeLabels()  { return { "Mono", "Stereo", "Ping-Pong", "Haas" }; }

    constexpr int headerH = 44;

    // 機種ボタンを押した時の「他社プラグインでの対応」（英語）
    const char* othersEn (int m)
    {
        static const char* t[] = {
            "Timeless 3: all FX off / EchoBoy: a clean Style / Valhalla: Digital, Age 0%",
            "Timeless 3: light Drive + 10 kHz high cut / EchoBoy: Studio Tape / Valhalla: HiFi, Chrome Tape",
            "Timeless 3: Drive + LFO to Time / EchoBoy: EchoPlex-type / Valhalla: Tape, ERA Past",
            "Timeless 3: 3 taps / EchoBoy: Space Echo + Rhythm / Valhalla: Tape + STYLE Quad",
            "Timeless 3: Lo-Fi + 4 kHz high cut + LFO / EchoBoy: Memory Man, DM-2 types / Valhalla: BBD, Analog",
            "Timeless 3: Diffuse + LFO / EchoBoy: TelRay / Valhalla: none, use DIFFUSION",
            "Timeless 3: Lo-Fi / EchoBoy: clean Style + high cut / Valhalla: Digital, Age 50%" };
        return t[juce::jlimit (0, 6, m)];
    }
}

DelayAnatomyLabView::DelayAnatomyLabView (DelayAnatomyLabProcessor& p)
    : proc (p),
      presetButtons (presetLabels(), 1001),
      sampleButtons (demoNames(), 1002),
      modeButtons   (shortModeLabels(), 1003),
      machineButtons (machineNames(), 1006),
      syncLButtons  (shortSyncLabels(), 1004),
      syncRButtons  (shortSyncLabels(), 1005),
      pitchModeButtons (pitchModeNames(), 1008),
      figures (p.apvts),
      compare (p.apvts)
{
    using APVTS = juce::AudioProcessorValueTreeState;

    // --- 配色（セッション → 前回使った配色 の順に復元） ---
    const int skin = proc.uiState.hasProperty ("skin") ? (int) proc.uiState.getProperty ("skin")
                                                       : ZoomPrefs::file().getIntValue ("delayanatomy.skin", 0);
    theme::setSkin (skin);
    lnf.apply();
    setLookAndFeel (&lnf);

    rightOpen = proc.uiState.hasProperty ("rightOpen") ? (bool) proc.uiState.getProperty ("rightOpen") : true;

    auto setupTitle = [this] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setFont (theme::mono (10.0f, true));
        l.setColour (juce::Label::textColourId, theme::muted);
        addAndMakeVisible (l);
    };
    setupTitle (presetTitle,  "PRESET");
    setupTitle (sampleTitle,  "SAMPLE");
    setupTitle (modeTitle,    "MODE");
    setupTitle (machineTitle, "MACHINE");
    setupTitle (syncLTitle,   "SYNC L");
    setupTitle (syncRTitle,   "SYNC R");

    for (auto* cb : { &presetButtons, &sampleButtons, &modeButtons, &machineButtons, &syncLButtons, &syncRButtons, &pitchModeButtons })
        addAndMakeVisible (*cb);

    presetButtons.onSelect = [this] (int index)
    {
        proc.setCurrentProgram (index);
        onPresetLoaded();
    };
    syncQuickButtons();

    sampleButtons.attachTo (*proc.apvts.getParameter (ParamID::demo));
    modeButtons.attachTo   (*proc.apvts.getParameter (ParamID::mode));
    syncLButtons.attachTo  (*proc.apvts.getParameter (ParamID::syncL));
    syncRButtons.attachTo  (*proc.apvts.getParameter (ParamID::syncR));
    pitchModeButtons.attachTo (*proc.apvts.getParameter (ParamID::pitchMode));

    // 機種ボタン：ホストのオートメーションと連動しつつ、手で押した時は「その機種らしい値」を Age / Drive / Wow / Flutter に入れる
    machineButtons.attachTo (*proc.apvts.getParameter (ParamID::machine));
    machineButtons.onSelect = [this] (int i) { proc.applyMachine (i); };


    // --- ヘッダー：画面サイズ・配色・言語・右パネルの開閉 ---
    addAndMakeVisible (zoom);
    for (int i = 0; i < theme::numSkins(); ++i)
        zoom.skinNames.add (theme::skinName (i));
    zoom.getSkin = [] { return theme::current; };
    zoom.setSkin = [this] (int i) { applySkin (i); };

    for (auto* b : { &prevBtn, &nextBtn, &saveBtn, &manageBtn })
        addAndMakeVisible (*b);
    addAndMakeVisible (presetNameBtn);
    prevBtn.onClick       = [this] { stepPreset (-1); };
    nextBtn.onClick       = [this] { stepPreset (+1); };
    presetNameBtn.onClick = [this] { showPresetMenu(); };
    saveBtn.onClick       = [this] { doSave(); };
    manageBtn.onClick     = [this] { showManageMenu(); };

    addAndMakeVisible (langBtn);
    langBtn.onClick = [this] { applyLanguage (proc.getLanguage() == Lang::Japanese ? Lang::English : Lang::Japanese); };
    addAndMakeVisible (infoBtn);
    infoBtn.onClick = [this] { setRightOpen (! rightOpen); };

    compare.setLanguage (proc.getLanguage());
    compare.setNarrow (true);
    figures.setLanguage (proc.getLanguage());

    // --- 右パネル ---
    side.addSection ("", &presetBody, [this] (int w) { return presetBody.fitHeight (w); });
    side.addSection ("", &tipsBody,   [this] (int w) { return tipsBody.fitHeight (w); });
    side.addSection ("", &compare,    [this] (int) { return compare.preferredHeight(); });
    side.setOpenMask (proc.uiState.hasProperty ("secMask2") ? (int) proc.uiState.getProperty ("secMask2") : 0b111);
    side.onMaskChanged = [this] (int m) { proc.uiState.setProperty ("secMask2", m, nullptr); };

    addAndMakeVisible (figures);
    addAndMakeVisible (helpBox);
    addAndMakeVisible (side);
    side.setVisible (rightOpen);

    // --- 値ストリップ（図に載せにくいもの） ---
    setupBar (timeL,     ParamID::timeL,     "Time L",    " ms");
    setupBar (timeR,     ParamID::timeR,     "Time R",    " ms");
    setupBar (feedback,  ParamID::feedback,  "Feedback",  " %");
    setupBar (hp,        ParamID::hp,        "High Pass", " Hz");
    setupBar (lp,        ParamID::lp,        "Low Pass",  " Hz");
    setupBar (rate,      ParamID::rate,      "Mod Rate",  " Hz");
    setupBar (depth,     ParamID::depth,     "Mod Depth", " ms");
    setupBar (wet,       ParamID::wet,       "Wet",       " dB");
    timeL.slider.onDragStart = [this] { releaseSync (ParamID::syncL); };      // 手で動かしたらテンポ同期は外れる
    timeR.slider.onDragStart = [this] { releaseSync (ParamID::syncR); };

    lfoLabel.setText ("LFO SHAPE", juce::dontSendNotification);
    lfoLabel.setFont (theme::mono (10.0f, true));
    lfoLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (lfoLabel);
    addAndMakeVisible (lfoBtn);
    lfoBtn.onClick = [this]
    {
        juce::PopupMenu m;
        const int cur = (int) proc.apvts.getRawParameterValue (ParamID::lfoShape)->load();
        for (int i = 0; i < lfoShapeNames().size(); ++i)
            m.addItem (i + 1, lfoShapeNames()[i], true, i == cur);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&lfoBtn).withMinimumWidth (lfoBtn.getWidth()),
                         [this] (int r)
                         {
                             if (r <= 0) return;
                             auto* p = proc.apvts.getParameter (ParamID::lfoShape);
                             p->beginChangeGesture();
                             p->setValueNotifyingHost (p->convertTo0to1 ((float) (r - 1)));
                             p->endChangeGesture();
                         });
    };
    setupBar (pitch,     ParamID::pitch,     "Pitch",   " st");
    setupBar (pitchFine, ParamID::pitchFine, "Fine",    " ct");
    setupBar (duck,      ParamID::duck,      "Duck",    " dB");
    setupBar (duckRel,   ParamID::duckRel,   "Release", " ms");

    for (auto* b : { &delayOff, &wetOnly, &wetLock, &monoCheck, &reverse })
    {
        b->setClickingTogglesState (true);
        addAndMakeVisible (*b);
    }
    offAtt     = std::make_unique<APVTS::ButtonAttachment> (proc.apvts, ParamID::delayOff,  delayOff);
    wetOnlyAtt = std::make_unique<APVTS::ButtonAttachment> (proc.apvts, ParamID::wetOnly,   wetOnly);
    wetLockAtt = std::make_unique<APVTS::ButtonAttachment> (proc.apvts, ParamID::wetLock,   wetLock);
    monoAtt    = std::make_unique<APVTS::ButtonAttachment> (proc.apvts, ParamID::monoCheck, monoCheck);
    reverseAtt = std::make_unique<APVTS::ButtonAttachment> (proc.apvts, ParamID::reverse,   reverse);

    compare.setView (2);
    for (auto* b : { &basicTab, &fxTab })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (5001);
        addAndMakeVisible (*b);
    }
    basicTab.onClick = [this] { if (basicTab.getToggleState()) setStripPage (0); };
    fxTab.onClick    = [this] { if (fxTab.getToggleState())    setStripPage (1); };
    stripPage = proc.uiState.hasProperty ("stripPage") ? (int) proc.uiState.getProperty ("stripPage") : 0;
    applyLanguage (proc.getLanguage());
    setStripPage (stripPage);
    addMouseListener (this, true);                       // 子部品の上のマウス移動も受けて、説明欄に出す
    { const auto h = uiHelp ("tip", proc.getLanguage()); helpBox.setHelp (h.title, h.body); }
    setSize (baseWidthFor (proc), fullH);
    startTimerHz (20);
#if defined(DAL_SNAPSHOT)      // 開発用：画面を PNG に書き出して終了（通常ビルドでは無効）
    {
        auto step = std::make_shared<std::function<void (int)>>();
        juce::Component::SafePointer<DelayAnatomyLabView> sp (this);
        *step = [sp, step] (int n)
        {
            if (sp == nullptr) return;
            auto save = [&] (const char* name)
            {
                auto img = sp->createComponentSnapshot (sp->getLocalBounds(), true, 1.0f);
                juce::File f (juce::String (std::getenv ("DAL_SNAPSHOT")) + "/" + name);
                f.deleteFile();
                juce::FileOutputStream os (f);
                juce::PNGImageFormat png;
                png.writeImageToStream (img, os);
            };
            switch (n)
            {
                case 0: sp->applySkin (1); sp->proc.applyMachine (6); break;
                case 1: save ("1_lablight_digital.png"); sp->proc.applyMachine (5); break;
                case 2: save ("2_lablight_oilcan.png"); sp->applySkin (0); sp->proc.setCurrentProgram (36); sp->onPresetLoaded(); break;
                case 3: save ("3_default_echoplex.png"); sp->proc.applyMachine (4); break;
                case 4: save ("4_basic_view.png"); sp->setRightOpen (false); break;
                case 5: save ("5_panel_closed.png"); juce::JUCEApplicationBase::quit(); return;
                default: return;
            }
            juce::Timer::callAfterDelay (1300, [step, n] { (*step) (n + 1); });
        };
        juce::Timer::callAfterDelay (2000, [step] { (*step) (0); });
    }
#endif
}

DelayAnatomyLabView::~DelayAnatomyLabView()
{
    setLookAndFeel (nullptr);
}

void DelayAnatomyLabView::setupBar (Bar& k, const char* id, const juce::String& name, const juce::String& suffix)
{
    k.slider.setTextValueSuffix (suffix);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 18);
    k.label.setText (name.toUpperCase(), juce::dontSendNotification);
    k.label.setFont (theme::mono (10.0f, true));
    k.label.setColour (juce::Label::textColourId, theme::muted);
    k.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (k.slider);
    addAndMakeVisible (k.label);
    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.slider);
}

void DelayAnatomyLabView::setStripPage (int page)
{
    stripPage = juce::jlimit (0, 1, page);
    proc.uiState.setProperty ("stripPage", stripPage, nullptr);
    basicTab.setToggleState (stripPage == 0, juce::dontSendNotification);
    fxTab.setToggleState (stripPage == 1, juce::dontSendNotification);
    for (auto* k : { &timeL, &timeR, &feedback, &hp, &lp, &rate, &depth })
        { k->slider.setVisible (stripPage == 0); k->label.setVisible (stripPage == 0); }
    lfoBtn.setVisible (stripPage == 0);
    lfoLabel.setVisible (stripPage == 0);
    for (auto* k : { &pitch, &pitchFine, &duck, &duckRel })
        { k->slider.setVisible (stripPage == 1); k->label.setVisible (stripPage == 1); }
    reverse.setVisible (stripPage == 1);
    pitchModeButtons.setVisible (stripPage == 1);
    resized();
    repaint();
}

void DelayAnatomyLabView::applySkin (int index)
{
    theme::setSkin (index);
    ZoomPrefs::file().setValue ("delayanatomy.skin", theme::current);
    ZoomPrefs::file().saveIfNeeded();
    proc.uiState.setProperty ("skin", theme::current, nullptr);
    lnf.apply();
    refreshColours();
    sendLookAndFeelChange();
    repaint();
}

void DelayAnatomyLabView::refreshColours()
{
    for (auto* l : { &presetTitle, &sampleTitle, &modeTitle, &machineTitle, &syncLTitle, &syncRTitle })
        l->setColour (juce::Label::textColourId, theme::muted);
    for (auto* k : { &timeL, &timeR, &feedback, &hp, &lp, &rate, &depth, &wet, &pitch, &pitchFine, &duck, &duckRel })
        k->label.setColour (juce::Label::textColourId, theme::muted);
    lfoLabel.setColour (juce::Label::textColourId, theme::muted);
    for (auto* t : { &presetBody, &tipsBody })
        t->refreshColours();
    side.refreshColours();
    showText();
}

void DelayAnatomyLabView::setRightOpen (bool open)
{
    rightOpen = open;
    proc.uiState.setProperty ("rightOpen", open, nullptr);
    side.setVisible (open);
    infoBtn.setButtonText ((proc.getLanguage() == Lang::Japanese ? juce::String::fromUTF8 ("\xe8\xa7\xa3\xe8\xaa\xac") : juce::String ("INFO")) + (open ? " >" : " <"));
    if (requestBaseSize)
        requestBaseSize (baseWidthFor (proc), fullH);     // 左 960 はそのまま、右だけ広げる／畳む
    repaint();
}

void DelayAnatomyLabView::applyLanguage (Lang l)
{
    proc.setLanguage (l);
    compare.setLanguage (l);
    figures.setLanguage (l);
    langBtn.setButtonText (l == Lang::Japanese ? juce::String ("EN") : juce::String::fromUTF8 ("日本語"));
    infoBtn.setButtonText ((l == Lang::Japanese ? juce::String::fromUTF8 ("\xe8\xa7\xa3\xe8\xaa\xac") : juce::String ("INFO")) + (rightOpen ? " >" : " <"));
    side.setTitle (0, tr ("このプリセットの解説", "About this preset"));
    side.setTitle (1, tr ("設定のコツ（触ってみよう）", "Tips: try these"));
    side.setTitle (2, tr ("比較表（現在値は入力・ドラッグで変更可）", "Comparison table (type or drag to edit)"));
    refreshPresetBar();
    showText();
}

void DelayAnatomyLabView::showText()
{
    const int index = proc.getCurrentProgram();          // ユーザープリセットでは「元にしたプリセット」
    if (! juce::isPositiveAndBelow (index, (int) factoryPresets().size()))
        return;

    const auto lang = proc.getLanguage();
    const auto& pr  = factoryPresets()[(size_t) index];
    const bool  ja  = (lang == Lang::Japanese);
    const auto& en  = presetTextEN (index);
    const auto& tw  = presetTweak (index);
    const bool user = proc.isUserPreset();

    auto txt = [ja] (const char* jpText, const char* enText) { return ja ? jp (jpText) : juce::String (enText); };

    juce::String t;
    if (user)
        t = proc.getPresetDisplayName() + "  [" + tr ("ユーザー／元：", "User / based on ") + juce::String (pr.name) + "]\n\n";
    else
        t = juce::String (pr.name) + "  [" + txt (pr.tag, en.tag) + "]\n\n";

    t += ui (UiText::WhatHappens, lang)  + "\n" + txt (pr.what, en.what) + "\n\n"
       + ui (UiText::WhereToUse, lang)   + "\n" + txt (pr.use,  en.use)  + "\n\n"
       + ui (UiText::WhatToListen, lang) + "\n" + txt (pr.ear,  en.ear)  + "\n\n"
       + tr ("※ 入力が無い時は SAMPLE で Pluck を選ぶと、すぐ音で確かめられます。", "Note: with no input, set SAMPLE to Pluck to hear it right away.");
    if (user)
        t += "\n" + ui (UiText::UserNote, lang);

    presetBody.setBody (t);
    tipsBody.setBody (txt (tw.ja, tw.en));
    side.relayout();
    shownPreset = index;
    shownMachine = (int) proc.apvts.getRawParameterValue (ParamID::machine)->load();
    shownLang = lang;
}

void DelayAnatomyLabView::timerCallback()
{
    const auto s   = proc.currentSettings();
    const auto bpm = proc.getHostBpm();
    const double fs = proc.getSampleRate() > 0.0 ? proc.getSampleRate() : 48000.0;
    figures.update (s, bpm, fs);
    compare.update (proc.getBaseline(), proc.getPresetDisplayName(), bpm);

    const auto next = juce::String (bpm, 1) + " BPM";
    if (next != readout) { readout = next; repaint (0, 0, leftW, headerH); }

    followSync (ParamID::timeL, ParamID::syncL, bpm);
    followSync (ParamID::timeR, ParamID::syncR, bpm);
    syncRButtons.setAlpha (s.mode == DelayMode::Mono ? 0.4f : 1.0f);
    timeR.slider.setAlpha (s.mode == DelayMode::Mono ? 0.35f : 1.0f);
    if (s.lfoShape != shownShape)
    {
        shownShape = s.lfoShape;
        lfoBtn.setButtonText (lfoShapeNames()[juce::jlimit (0, 4, shownShape)]);
    }

    // ホスト側でのプリセット／機種の切替にも表示を追従
    if (proc.getPresetSerial() != shownSerial)
    {
        shownSerial = proc.getPresetSerial();
        syncQuickButtons();
        refreshPresetBar();
        showText();
    }
    else if ((int) s.machine != shownMachine)
        showText();
    if (proc.isModified() != shownModified)
        refreshPresetBar();                      // プリセット名の後ろの * を出し入れ
}

void DelayAnatomyLabView::paint (juce::Graphics& g)
{
    g.fillAll (theme::bg);

    g.setColour (theme::header); g.fillRect (0, 0, getWidth(), headerH);
    g.setColour (theme::line);   g.drawHorizontalLine (headerH, 0.0f, (float) getWidth());
    g.setColour (theme::text);   g.setFont (theme::mono (14.0f, true));
    g.drawText ("DELAY ANATOMY LAB", 14, 6, 190, 20, juce::Justification::centredLeft);
    g.setColour (theme::faint);  g.setFont (theme::ui (9.5f));
    g.drawText ("by Johan Plus Kei", 14, 24, 170, 14, juce::Justification::centredLeft);
    g.setColour (theme::muted);  g.setFont (theme::mono (11.0f));
    g.drawText ("HOST " + readout, 650, 12, 120, 20, juce::Justification::centredRight);

    if (! stripPanel.isEmpty())
    {
        g.setColour (theme::panel); g.fillRoundedRectangle (stripPanel.toFloat(), 6.0f);
        g.setColour (theme::line);  g.drawRoundedRectangle (stripPanel.toFloat().reduced (0.5f), 6.0f, 1.0f);
    }
}

void DelayAnatomyLabView::resized()
{
    zoom.setBounds (178, 12, 46, 20);
    langBtn.setBounds (leftW - 12 - 64, 10, 64, 24);
    infoBtn.setBounds (leftW - 12 - 64 - 8 - 76, 10, 76, 24);
    {
        int x = 240;                                   // ◀ [ プリセット名 ▼ ] ▶  保存  管理
        prevBtn.setBounds (x, 10, 26, 24);        x += 30;
        presetNameBtn.setBounds (x, 10, 200, 24); x += 204;
        nextBtn.setBounds (x, 10, 26, 24);        x += 30;
        saveBtn.setBounds (x, 10, 54, 24);        x += 58;
        manageBtn.setBounds (x, 10, 72, 24);
    }

    // 左の領域は 980 幅で固定（右パネルの開閉に左右されない）
    auto r = juce::Rectangle<int> (0, headerH + 14, leftW, fullH - headerH - 14 - 16).reduced (14, 0);
    const int labelW = 64, rowH = 28, gap = 12;
    auto row = r.removeFromTop (rowH);                  // PRESET（11個）
    presetTitle.setBounds (row.removeFromLeft (labelW));
    presetButtons.setBounds (row);
    r.removeFromTop (gap);

    row = r.removeFromTop (rowH);                       // SAMPLE ＋ MODE
    auto left = row.removeFromLeft (row.getWidth() / 2 - 6);
    row.removeFromLeft (12);
    sampleTitle.setBounds (left.removeFromLeft (labelW));
    sampleButtons.setBounds (left);
    modeTitle.setBounds (row.removeFromLeft (labelW - 14));
    modeButtons.setBounds (row);
    r.removeFromTop (gap);

    row = r.removeFromTop (rowH);                       // MACHINE（7個）
    machineTitle.setBounds (row.removeFromLeft (labelW));
    machineButtons.setBounds (row);
    r.removeFromTop (gap);

    // 下端から積む：SYNC → 値ストリップ。残りは図
    auto syncRow = r.removeFromBottom (rowH);
    r.removeFromBottom (gap);
    stripPanel = r.removeFromBottom (158);
    r.removeFromBottom (gap);
    figures.setBounds (r);

    // 値ストリップ：左＝タブ（BASICS / FX）、中＝ノブ、右＝Wet ノブ（共通）＋スイッチ 4つ（共通）
    {
        auto in = stripPanel.reduced (16, 12);
        auto tabs = in.removeFromLeft (64);
        in.removeFromLeft (12);
        basicTab.setBounds (tabs.removeFromTop (30));
        tabs.removeFromTop (8);
        fxTab.setBounds (tabs.removeFromTop (30));

        auto btnCol = in.removeFromRight (104);
        in.removeFromRight (14);
        const int bh = (btnCol.getHeight() - 3 * 6) / 4;
        for (auto* b : { &delayOff, &wetOnly, &wetLock, &monoCheck })
        {
            b->setBounds (btnCol.removeFromTop (bh));
            btnCol.removeFromTop (6);
        }

        auto wetCell = in.removeFromRight (90);
        in.removeFromRight (14);
        wet.label.setBounds (wetCell.removeFromTop (16));
        wet.slider.setBounds (wetCell);

        auto place = [] (juce::Rectangle<int> area, std::initializer_list<Bar*> ks)
        {
            const int w = area.getWidth() / (int) ks.size();
            for (auto* k : ks)
            {
                auto cell = area.removeFromLeft (w);
                k->label.setBounds (cell.removeFromTop (16));
                k->slider.setBounds (cell);
            }
        };
        if (stripPage == 0)
        {
            auto lfoCell = in.removeFromRight (76);
            in.removeFromRight (6);
            lfoLabel.setBounds (lfoCell.removeFromTop (16));
            lfoBtn.setBounds (lfoCell.withSizeKeepingCentre (lfoCell.getWidth(), 28).translated (0, -8));
            place (in, { &timeL, &timeR, &feedback, &hp, &lp, &rate, &depth });
        }
        else
        {
            auto col = in.removeFromRight (130);
            in.removeFromRight (20);
            reverse.setBounds (col.removeFromTop (30));
            col.removeFromTop (10);
            pitchModeButtons.setBounds (col.removeFromTop (30));
            place (in, { &pitch, &pitchFine, &duck, &duckRel });
        }
    }

    // テンポ同期 L / R
    row = syncRow;
    left = row.removeFromLeft (row.getWidth() / 2 - 6);
    row.removeFromLeft (12);
    syncLTitle.setBounds (left.removeFromLeft (labelW));
    syncLButtons.setBounds (left);
    syncRTitle.setBounds (row.removeFromLeft (labelW - 14));
    syncRButtons.setBounds (row);

    // 右パネル
    helpBox.setBounds (leftW, headerH + 14, panelW - 14, 132);
    side.setBounds (leftW, headerH + 14 + 132 + 12, panelW - 14, fullH - headerH - 14 - 16 - 132 - 12);
}

void DelayAnatomyLabView::followSync (const char* timeId, const char* syncId, double bpm)
{
    const int sync = (int) proc.apvts.getRawParameterValue (syncId)->load();
    if (sync <= 0)
        return;

    auto* param  = proc.apvts.getParameter (timeId);
    const auto target = (float) juce::jlimit (1.0, 2000.0, resolveTimeMs (sync, 0.0, bpm));
    const auto now    = proc.apvts.getRawParameterValue (timeId)->load();
    if (std::abs (now - target) > 0.06f)
        param->setValueNotifyingHost (param->convertTo0to1 (target));
}

void DelayAnatomyLabView::releaseSync (const char* syncId)
{
    auto* param = proc.apvts.getParameter (syncId);
    if (param->getValue() <= 0.0f)
        return;
    param->beginChangeGesture();
    param->setValueNotifyingHost (0.0f);
    param->endChangeGesture();
}


// =============================================================================
//  プリセット欄（ヘッダー）・保存・管理
// =============================================================================
void DelayAnatomyLabView::onPresetLoaded()
{
    shownSerial = proc.getPresetSerial();
    syncQuickButtons();
    refreshPresetBar();
    showText();
}

void DelayAnatomyLabView::syncQuickButtons()
{
    const int p = proc.getCurrentProgram();
    presetButtons.setSelected ((proc.isUserPreset() || p >= kQuickPresets) ? -1 : p);
}

void DelayAnatomyLabView::refreshPresetBar()
{
    shownModified = proc.isModified();
    juce::String name = proc.getPresetDisplayName();
    if (proc.isUserPreset())
        name = tr ("[ユーザー] ", "[User] ") + name;
    if (shownModified)
        name += " *";
    presetNameBtn.setButtonText (name);
    saveBtn.setButtonText (tr ("保存", "Save"));
    manageBtn.setButtonText (tr ("管理", "Manage"));
}

void DelayAnatomyLabView::loadEntry (int i, const juce::Array<juce::File>& files)
{
    const int n = proc.getNumPrograms();
    if (i < n)
        proc.setCurrentProgram (i);
    else if (i - n < files.size())
        proc.loadUserPreset (files[i - n]);
    onPresetLoaded();
}

void DelayAnatomyLabView::stepPreset (int dir)
{
    const int n = proc.getNumPrograms();
    const auto files = presetstore::list();
    const int total = n + files.size();
    int cur = proc.getCurrentProgram();
    if (proc.isUserPreset())
    {
        cur = 0;
        for (int i = 0; i < files.size(); ++i)
            if (files[i].getFileNameWithoutExtension().equalsIgnoreCase (proc.getPresetDisplayName()))
                cur = n + i;
    }
    loadEntry ((cur + dir + total) % total, files);
}

void DelayAnatomyLabView::showPresetMenu()
{
    const bool ja = proc.getLanguage() == Lang::Japanese;
    const auto files = presetstore::list();
    const bool user = proc.isUserPreset();
    const int cur = proc.getCurrentProgram();

    juce::PopupMenu m;
    for (auto& cat : presetCategories())
    {
        juce::PopupMenu sub;
        bool has = false;
        for (int id : cat.ids)
        {
            const bool on = ! user && id == cur;
            has = has || on;
            sub.addItem (1 + id, juce::String (factoryPresets()[(size_t) id].name), true, on);
        }
        m.addSubMenu (ja ? jp (cat.ja) : juce::String (cat.en), sub, true, nullptr, has);
    }
    m.addSeparator();

    juce::PopupMenu um;
    if (files.isEmpty())
        um.addItem (9999, tr ("（まだありません）", "(none yet)"), false);
    for (int i = 0; i < files.size(); ++i)
        um.addItem (1000 + i, files[i].getFileNameWithoutExtension(), true,
                    user && files[i].getFileNameWithoutExtension().equalsIgnoreCase (proc.getPresetDisplayName()));
    m.addSubMenu (tr ("ユーザープリセット", "User presets"), um, true, nullptr, user);

    juce::Component::SafePointer<DelayAnatomyLabView> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetNameBtn).withMinimumWidth (presetNameBtn.getWidth()),
                     [sp, files] (int r)
                     {
                         if (sp == nullptr || r <= 0 || r == 9999) return;
                         sp->loadEntry (r >= 1000 ? sp->proc.getNumPrograms() + (r - 1000) : r - 1, files);
                     });
}

void DelayAnatomyLabView::showManageMenu()
{
    const bool user = proc.isUserPreset();
    juce::PopupMenu m;
    m.addItem (1, tr ("上書き保存", "Save (overwrite)"), user);
    m.addItem (2, tr ("名前を付けて保存…", "Save as..."));
    m.addItem (3, tr ("名前を変更…", "Rename..."), user);
    m.addItem (4, tr ("削除…", "Delete..."), user);
    m.addSeparator();
    m.addItem (5, tr ("選んだプリセットの値に戻す", "Revert to preset values"));
    m.addSeparator();
    m.addItem (6, tr ("プリセットファイルを読み込む…", "Import preset file..."));
    m.addItem (7, tr ("今の設定を書き出す…", "Export current settings..."));
    m.addItem (8, tr ("プリセットのフォルダを開く", "Show preset folder"));

    juce::Component::SafePointer<DelayAnatomyLabView> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&manageBtn),
                     [sp] (int r)
                     {
                         if (sp == nullptr) return;
                         switch (r)
                         {
                             case 1: sp->doSave(); break;
                             case 2: sp->doSaveAs(); break;
                             case 3: sp->doRename(); break;
                             case 4: sp->doDelete(); break;
                             case 5: sp->proc.revertToPreset(); sp->onPresetLoaded(); break;
                             case 6: sp->doImport(); break;
                             case 7: sp->doExport(); break;
                             case 8: presetstore::folder().revealToUser(); break;
                             default: break;
                         }
                     });
}

void DelayAnatomyLabView::doSave()
{
    if (proc.isUserPreset())
    {
        proc.saveUserPreset (proc.getPresetDisplayName());      // 上書き
        onPresetLoaded();
    }
    else
        doSaveAs();                                             // ファクトリーは名前を付けて保存
}

void DelayAnatomyLabView::doSaveAs()
{
    const auto def = proc.isUserPreset() ? proc.getPresetDisplayName() + " 2"
                                         : proc.getPresetDisplayName() + " (mine)";
    juce::Component::SafePointer<DelayAnatomyLabView> sp (this);
    openDialog (tr ("名前を付けて保存", "Save preset"),
                tr ("プリセット名を入力してください。いまのノブの値がそのまま保存されます。", "Enter a name. The current knob values are saved as they are."),
                tr ("保存", "Save"), def, true, true,
                [sp] (bool ok, juce::String name)
                {
                    if (sp == nullptr || ! ok) return;
                    const auto n = presetstore::cleanName (name);
                    if (n.isEmpty()) return;
                    const bool same = sp->proc.isUserPreset() && sp->proc.getPresetDisplayName().equalsIgnoreCase (n);
                    if (presetstore::fileFor (n).existsAsFile() && ! same)
                    {
                        sp->openDialog (sp->tr ("上書きの確認", "Overwrite?"),
                                        sp->tr ("同じ名前のプリセットがあります。上書きしますか？", "A preset with this name already exists. Overwrite it?"),
                                        sp->tr ("上書き", "Overwrite"), {}, false, true,
                                        [sp, n] (bool ok2, juce::String)
                                        {
                                            if (sp == nullptr || ! ok2) return;
                                            sp->proc.saveUserPreset (n);
                                            sp->onPresetLoaded();
                                        });
                    }
                    else
                    {
                        sp->proc.saveUserPreset (n);
                        sp->onPresetLoaded();
                    }
                });
}

void DelayAnatomyLabView::doRename()
{
    juce::Component::SafePointer<DelayAnatomyLabView> sp (this);
    openDialog (tr ("名前を変更", "Rename preset"), tr ("新しい名前を入力してください。", "Enter a new name."),
                tr ("変更", "Rename"), proc.getPresetDisplayName(), true, true,
                [sp] (bool ok, juce::String name)
                {
                    if (sp == nullptr || ! ok) return;
                    if (sp->proc.renameUserPreset (name))
                        sp->onPresetLoaded();
                    else
                        sp->openDialog (sp->tr ("名前を変更できません", "Cannot rename"),
                                        sp->tr ("同じ名前のプリセットが既にあるか、使えない名前です。", "A preset with that name already exists, or the name is not valid."),
                                        "OK", {}, false, false, [] (bool, juce::String) {});
                });
}

void DelayAnatomyLabView::doDelete()
{
    const auto name = proc.getPresetDisplayName();
    juce::Component::SafePointer<DelayAnatomyLabView> sp (this);
    const auto msg = proc.getLanguage() == Lang::Japanese
                         ? jp ("「") + name + jp ("」を削除しますか？（ファイルも消えます。いまの音はそのまま残ります）")
                         : "Delete \"" + name + "\"? The file is removed; the current sound stays as it is.";
    openDialog (tr ("プリセットの削除", "Delete preset"), msg, tr ("削除", "Delete"), {}, false, true,
                [sp] (bool ok, juce::String)
                {
                    if (sp == nullptr || ! ok) return;
                    sp->proc.deleteUserPreset();
                    sp->onPresetLoaded();
                });
}

void DelayAnatomyLabView::doImport()
{
    juce::Component::SafePointer<DelayAnatomyLabView> sp (this);
    chooser = std::make_unique<juce::FileChooser> (tr ("読み込むプリセットファイル", "Choose a preset file"), presetstore::folder(), "*.dalpreset");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [sp] (const juce::FileChooser& fc)
                          {
                              if (sp == nullptr) return;
                              const auto f = fc.getResult();
                              if (f.existsAsFile() && sp->proc.importPreset (f))
                                  sp->onPresetLoaded();
                          });
}

void DelayAnatomyLabView::doExport()
{
    juce::Component::SafePointer<DelayAnatomyLabView> sp (this);
    const auto start = juce::File::getSpecialLocation (juce::File::userDesktopDirectory)
                           .getChildFile (presetstore::cleanName (proc.getPresetDisplayName()) + ".dalpreset");
    chooser = std::make_unique<juce::FileChooser> (tr ("プリセットの書き出し先", "Export preset to"), start, "*.dalpreset");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [sp] (const juce::FileChooser& fc)
                          {
                              if (sp == nullptr) return;
                              auto f = fc.getResult();
                              if (f == juce::File())
                                  return;
                              if (! f.hasFileExtension ("dalpreset"))
                                  f = f.withFileExtension ("dalpreset");
                              sp->proc.exportCurrent (f);
                          });
}

// 名前入力／確認のダイアログ（OK = true）。名前入力が要らない時は wantText = false
void DelayAnatomyLabView::openDialog (const juce::String& title, const juce::String& msg, const juce::String& okText,
                                      const juce::String& defaultText, bool wantText, bool wantCancel,
                                      std::function<void (bool, juce::String)> cb)
{
    auto* w = new juce::AlertWindow (title, msg, juce::MessageBoxIconType::NoIcon);
    if (wantText)
        w->addTextEditor ("name", defaultText, {});
    w->addButton (okText, 1, juce::KeyPress (juce::KeyPress::returnKey));
    if (wantCancel)
        w->addButton (tr ("キャンセル", "Cancel"), 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->centreAroundComponent (this, w->getWidth(), w->getHeight());
    if (! w->isOnDesktop())
        w->addToDesktop (juce::ComponentPeer::windowHasDropShadow);

    juce::Component::SafePointer<juce::AlertWindow> wp (w);
    w->enterModalState (true,
                        juce::ModalCallbackFunction::create ([wp, cb, wantText] (int result)
                        {
                            juce::String text;
                            if (wp != nullptr && wantText)
                                text = wp->getTextEditorContents ("name");
                            if (cb)
                                cb (result == 1, text);
                        }),
                        true);
}


// =============================================================================
//  マウスオーバー説明：乗っている部品 → 右パネル上部の説明欄
// =============================================================================
void DelayAnatomyLabView::updateHelp (const juce::MouseEvent& e)
{
    const auto lang = proc.getLanguage();
    HelpText h;
    const juce::Component* prev = nullptr;
    for (auto* c = e.eventComponent; c != nullptr && c != this && ! h.valid(); prev = c, c = c->getParentComponent())
    {
        if      (c == &zoom)           h = uiHelp ("zoom", lang);
        else if (c == &langBtn)        h = uiHelp ("lang", lang);
        else if (c == &infoBtn)        h = uiHelp ("info", lang);
        else if (c == &prevBtn)        h = uiHelp ("prev", lang);
        else if (c == &nextBtn)        h = uiHelp ("next", lang);
        else if (c == &presetNameBtn)  h = uiHelp ("presetName", lang);
        else if (c == &saveBtn)        h = uiHelp ("save", lang);
        else if (c == &manageBtn)      h = uiHelp ("manage", lang);
        else if (c == &delayOff)       h = paramHelp (ParamID::delayOff, lang);
        else if (c == &wetOnly)        h = paramHelp (ParamID::wetOnly, lang);
        else if (c == &wetLock)        h = paramHelp (ParamID::wetLock, lang);
        else if (c == &monoCheck)      h = paramHelp (ParamID::monoCheck, lang);
        else if (c == &reverse)        h = paramHelp (ParamID::reverse, lang);
        else if (c == &pitchModeButtons) { const int i = pitchModeButtons.indexOf (prev); h = paramHelp (ParamID::pitchMode, lang);
                                           if (i >= 0) h.title += " : " + pitchModeNames()[i]; }
        else if (c == &presetButtons)  h = presetHelp (presetButtons.indexOf (prev), lang);
        else if (c == &sampleButtons)  h = paramHelp (ParamID::demo, lang);
        else if (c == &modeButtons)    { const int i = modeButtons.indexOf (prev); h = i >= 0 ? modeHelp (i, lang) : paramHelp (ParamID::mode, lang); }
        else if (c == &machineButtons) { const int i = machineButtons.indexOf (prev); h = i >= 0 ? machineHelp (i, lang) : paramHelp (ParamID::machine, lang); }
        else if (c == &syncLButtons)   { const int i = syncLButtons.indexOf (prev); h = i >= 0 ? syncHelp (i, lang) : paramHelp (ParamID::syncL, lang); }
        else if (c == &syncRButtons)   { const int i = syncRButtons.indexOf (prev); h = i >= 0 ? syncHelp (i, lang) : paramHelp (ParamID::syncR, lang); }
        else if (c == &basicTab)       h = uiHelp ("stripBasic", lang);
        else if (c == &fxTab)          h = uiHelp ("stripFx", lang);
        else if (c == &timeL.slider || c == &timeL.label)         h = paramHelp (ParamID::timeL, lang);
        else if (c == &timeR.slider || c == &timeR.label)         h = paramHelp (ParamID::timeR, lang);
        else if (c == &feedback.slider || c == &feedback.label)   h = paramHelp (ParamID::feedback, lang);
        else if (c == &hp.slider || c == &hp.label)               h = paramHelp (ParamID::hp, lang);
        else if (c == &lp.slider || c == &lp.label)               h = paramHelp (ParamID::lp, lang);
        else if (c == &rate.slider || c == &rate.label)           h = paramHelp (ParamID::rate, lang);
        else if (c == &depth.slider || c == &depth.label)         h = paramHelp (ParamID::depth, lang);
        else if (c == &wet.slider || c == &wet.label)             h = paramHelp (ParamID::wet, lang);
        else if (c == &lfoBtn || c == &lfoLabel)                  h = paramHelp (ParamID::lfoShape, lang);
        else if (c == &pitch.slider || c == &pitch.label)         h = paramHelp (ParamID::pitch, lang);
        else if (c == &pitchFine.slider || c == &pitchFine.label) h = paramHelp (ParamID::pitchFine, lang);
        else if (c == &duck.slider || c == &duck.label)           h = paramHelp (ParamID::duck, lang);
        else if (c == &duckRel.slider || c == &duckRel.label)     h = paramHelp (ParamID::duckRel, lang);
        else if (c == &presetTitle)    h = uiHelp ("presetName", lang);
        else if (c == &compare)
        {
            const auto p = e.getEventRelativeTo (&compare).getPosition();
            if (auto* id = compare.idAt (p)) h = paramHelp (id, lang);
            if (! h.valid()) h = uiHelp ("compare", lang);
        }
        else if (auto* hv = dynamic_cast<const HandleView*> (c))
        {
            const auto id = hv->hoveredParamId();
            if (id.isNotEmpty()) h = paramHelp (id, lang);
            if (! h.valid())
                h = uiHelp (dynamic_cast<const TimelineView*> (c) ? "timeline" : dynamic_cast<const DiagramView*> (c) ? "diagram" : "repeats", lang);
        }
    }
    if (! h.valid()) h = uiHelp ("tip", lang);
    helpBox.setHelp (h.title, h.body);
}
