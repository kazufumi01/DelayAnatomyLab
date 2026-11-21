#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

using APVTS = juce::AudioProcessorValueTreeState;

// =============================================================================
//  パラメーター定義
//  NormalisableRange の setSkewForCentre は「ノブの真ん中を何にするか」。
//  ms や Hz は対数的に聴こえるので、中央を小さめの値に寄せると扱いやすい。
// =============================================================================
APVTS::ParameterLayout DelayAnatomyLabProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto pid = [] (const char* id) { return juce::ParameterID { id, 1 }; };

    auto msRange = juce::NormalisableRange<float> (1.0f, 2000.0f, 0.1f);   // 1/2音符 @60BPM まで表示できる範囲
    msRange.setSkewForCentre (150.0f);
    auto hpRange = juce::NormalisableRange<float> (20.0f, 2000.0f, 1.0f);
    hpRange.setSkewForCentre (200.0f);
    auto lpRange = juce::NormalisableRange<float> (500.0f, 20000.0f, 1.0f);
    lpRange.setSkewForCentre (3500.0f);
    auto rateRange = juce::NormalisableRange<float> (0.05f, 6.0f, 0.01f);
    rateRange.setSkewForCentre (0.8f);

    // 新規に立ち上げた時の値＝プリセット「Default」の値（比較表が「= 初期値」から始まるように）
    // ※ Time の初期値はテンポ同期（1/4）中の 120BPM 換算。同期中は実際のテンポに追従する。
    const auto& d = factoryPresets().front();

    p.push_back (std::make_unique<juce::AudioParameterChoice> (pid (ParamID::mode), "Mode", modeNames(), (int) d.mode));
    p.push_back (std::make_unique<juce::AudioParameterFloat>  (pid (ParamID::timeL), "Time L", msRange,
                    (float) resolveTimeMs (d.syncL, d.timeL, 120.0),
                    juce::AudioParameterFloatAttributes().withLabel ("ms")));
    p.push_back (std::make_unique<juce::AudioParameterFloat>  (pid (ParamID::timeR), "Time R", msRange,
                    (float) resolveTimeMs (d.syncR, d.timeR, 120.0),
                    juce::AudioParameterFloatAttributes().withLabel ("ms")));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (pid (ParamID::syncL), "Sync L", syncNames(), d.syncL));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (pid (ParamID::syncR), "Sync R", syncNames(), d.syncR));
    p.push_back (std::make_unique<juce::AudioParameterFloat>  (pid (ParamID::feedback), "Feedback",
                    juce::NormalisableRange<float> (0.0f, 95.0f, 0.1f), d.feedbackPct,
                    juce::AudioParameterFloatAttributes().withLabel ("%")));
    p.push_back (std::make_unique<juce::AudioParameterFloat>  (pid (ParamID::hp), "High Pass", hpRange, d.hp,
                    juce::AudioParameterFloatAttributes().withLabel ("Hz")));
    p.push_back (std::make_unique<juce::AudioParameterFloat>  (pid (ParamID::lp), "Low Pass", lpRange, d.lp,
                    juce::AudioParameterFloatAttributes().withLabel ("Hz")));
    p.push_back (std::make_unique<juce::AudioParameterFloat>  (pid (ParamID::rate), "Mod Rate", rateRange, d.rate,
                    juce::AudioParameterFloatAttributes().withLabel ("Hz")));
    p.push_back (std::make_unique<juce::AudioParameterFloat>  (pid (ParamID::depth), "Mod Depth",
                    juce::NormalisableRange<float> (0.0f, 5.0f, 0.01f), d.depth,
                    juce::AudioParameterFloatAttributes().withLabel ("ms")));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (pid (ParamID::lfoShape), "LFO Shape", lfoShapeNames(), 0));
    p.push_back (std::make_unique<juce::AudioParameterFloat>  (pid (ParamID::wet), "Wet",
                    juce::NormalisableRange<float> (-30.0f, 0.0f, 0.1f), d.wetDb,
                    juce::AudioParameterFloatAttributes().withLabel ("dB")));
    p.push_back (std::make_unique<juce::AudioParameterBool>   (pid (ParamID::delayOff),  "Bypass",     false));
    p.push_back (std::make_unique<juce::AudioParameterBool>   (pid (ParamID::wetOnly),   "Wet Only",   false));
    p.push_back (std::make_unique<juce::AudioParameterBool>   (pid (ParamID::wetLock),   "Wet Lock",   false));
    p.push_back (std::make_unique<juce::AudioParameterBool>   (pid (ParamID::monoCheck), "Mono Check", false));
    // 入力の代わりに鳴らすサンプル音源（Off ならトラックの音をそのまま処理）
    p.push_back (std::make_unique<juce::AudioParameterChoice> (pid (ParamID::demo), "Sample Source", demoNames(), 0));

    // --- Analog Anatomy：機種・劣化・揺れ／ピッチ・リバース・ダッキング ---
    auto pct = [&] (const char* id, const char* name)
    {
        p.push_back (std::make_unique<juce::AudioParameterFloat> (pid (id), name, juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f,
                        juce::AudioParameterFloatAttributes().withLabel ("%")));
    };
    p.push_back (std::make_unique<juce::AudioParameterChoice> (pid (ParamID::machine), "Machine", machineNames(), d.fx.machine));
    pct (ParamID::age, "Age");
    pct (ParamID::drive, "Drive");
    pct (ParamID::wow, "Wow");
    pct (ParamID::flutter, "Flutter");
    p.push_back (std::make_unique<juce::AudioParameterFloat> (pid (ParamID::pitch), "Pitch", juce::NormalisableRange<float> (-12.0f, 12.0f, 1.0f), 0.0f,
                    juce::AudioParameterFloatAttributes().withLabel ("st")));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (pid (ParamID::pitchFine), "Pitch Fine", juce::NormalisableRange<float> (-50.0f, 50.0f, 1.0f), 0.0f,
                    juce::AudioParameterFloatAttributes().withLabel ("ct")));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (pid (ParamID::pitchMode), "Pitch Mode", pitchModeNames(), 0));
    p.push_back (std::make_unique<juce::AudioParameterBool>   (pid (ParamID::reverse), "Reverse", false));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (pid (ParamID::duck), "Duck", juce::NormalisableRange<float> (0.0f, 24.0f, 0.1f), 0.0f,
                    juce::AudioParameterFloatAttributes().withLabel ("dB")));
    auto relRange = juce::NormalisableRange<float> (50.0f, 1000.0f, 1.0f);
    relRange.setSkewForCentre (250.0f);
    p.push_back (std::make_unique<juce::AudioParameterFloat> (pid (ParamID::duckRel), "Duck Release", relRange, 250.0f,
                    juce::AudioParameterFloatAttributes().withLabel ("ms")));

    return { p.begin(), p.end() };
}

DelayAnatomyLabProcessor::DelayAnatomyLabProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    apvts.addParameterListener (ParamID::delayOff, this);
}

DelayAnatomyLabProcessor::~DelayAnatomyLabProcessor()
{
    apvts.removeParameterListener (ParamID::delayOff, this);
    cancelPendingUpdate();
}

void DelayAnatomyLabProcessor::parameterChanged (const juce::String&, float newValue)
{
    if (newValue > 0.5f)          // オートメーションならオーディオスレッドから来るので非同期で処理
        triggerAsyncUpdate();
}

void DelayAnatomyLabProcessor::handleAsyncUpdate()
{
    for (auto* id : { ParamID::wetOnly, ParamID::wetLock })
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (0.0f);
}

// 入力はモノ／ステレオ、出力はステレオのみ（Haasやピンポンは2chが前提）
bool DelayAnatomyLabProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    const auto in  = layouts.getMainInputChannelSet();
    return out == juce::AudioChannelSet::stereo()
        && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void DelayAnatomyLabProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    demo.prepare (sampleRate);
    dryCopy.setSize (2, samplesPerBlock);
    bypassMix.reset (sampleRate, 0.02);
    bypassMix.setCurrentAndTargetValue (apvts.getRawParameterValue (ParamID::delayOff)->load() > 0.5f ? 1.0f : 0.0f);
}

DelaySettings DelayAnatomyLabProcessor::currentSettings() const
{
    auto v = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    const double bpm = hostBpm.load();

    DelaySettings s;
    s.mode      = (DelayMode) (int) v (ParamID::mode);
    s.timeLms   = (float) resolveTimeMs ((int) v (ParamID::syncL), v (ParamID::timeL), bpm);
    s.timeRms   = (float) resolveTimeMs ((int) v (ParamID::syncR), v (ParamID::timeR), bpm);
    if (s.mode == DelayMode::Mono)
        s.timeRms = s.timeLms;                       // Mono は L の時間だけを使う
    s.machine   = (Machine) (int) v (ParamID::machine);
    s.timeLms   = machineClampTime (s.machine, s.timeLms);     // 機種ごとに Time の範囲がある
    s.timeRms   = machineClampTime (s.machine, s.timeRms);
    s.age       = v (ParamID::age);
    s.drive     = v (ParamID::drive);
    s.wow       = v (ParamID::wow);
    s.flutter   = v (ParamID::flutter);
    s.pitchSemis = v (ParamID::pitch) + v (ParamID::pitchFine) / 100.0f;
    s.pitchInLoop = (int) v (ParamID::pitchMode) == 0;
    s.reverse   = v (ParamID::reverse) > 0.5f;
    s.duckDb    = v (ParamID::duck);
    s.duckRelMs = v (ParamID::duckRel);
    s.feedback  = v (ParamID::feedback) / 100.0f;
    s.hpHz      = v (ParamID::hp);
    s.lpHz      = v (ParamID::lp);
    s.rateHz    = v (ParamID::rate);
    s.depthMs   = v (ParamID::depth);
    s.lfoShape  = (int) v (ParamID::lfoShape);
    s.wetGain   = juce::Decibels::decibelsToGain (v (ParamID::wet));
    s.delayOff  = v (ParamID::delayOff)  > 0.5f;
    s.wetOnly   = v (ParamID::wetOnly)   > 0.5f;
    s.monoCheck = v (ParamID::monoCheck) > 0.5f;
    return s;
}

void DelayAnatomyLabProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;   // 減衰していく尾で CPU が跳ねるのを防ぐ

    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    // ホストのテンポを取得（取れない時は前回値のまま）
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                hostBpm.store (*bpm);

    // サンプル音源がONなら、入力をサンプル音で置き換えてからディレイへ
    const int demoSource = (int) apvts.getRawParameterValue (ParamID::demo)->load();
    if (demoSource != DemoSynth::Off)
        demo.render (buffer, demoSource, hostBpm.load());

    // Bypass 用に「処理前の音」を保存（モノ入力なら L を R にも複製）
    const int n = buffer.getNumSamples();
    if (dryCopy.getNumSamples() < n)
        dryCopy.setSize (2, n, false, false, true);
    const bool monoIn = (demoSource == DemoSynth::Off && numIn < 2);
    for (int ch = 0; ch < 2; ++ch)
        dryCopy.copyFrom (ch, 0, buffer, monoIn ? 0 : juce::jmin (ch, buffer.getNumChannels() - 1), 0, n);

    engine.process (buffer, demoSource != DemoSynth::Off ? 2 : numIn, currentSettings());

    // Bypass：Wet Only / Mono / Haas の配線に関係なく、入力音をそのまま返す
    bypassMix.setTargetValue (apvts.getRawParameterValue (ParamID::delayOff)->load() > 0.5f ? 1.0f : 0.0f);
    if (bypassMix.isSmoothing() || bypassMix.getTargetValue() > 0.5f)
    {
        for (int i = 0; i < n; ++i)
        {
            const float b = bypassMix.getNextValue();
            for (int ch = 0; ch < juce::jmin (2, buffer.getNumChannels()); ++ch)
            {
                auto* d = buffer.getWritePointer (ch);
                d[i] = d[i] * (1.0f - b) + dryCopy.getSample (ch, i) * b;
            }
        }
    }
}

// =============================================================================
//  プリセット
// =============================================================================
int DelayAnatomyLabProcessor::getNumPrograms()
{
    return (int) factoryPresets().size();
}

const juce::String DelayAnatomyLabProcessor::getProgramName (int index)
{
    if (juce::isPositiveAndBelow (index, getNumPrograms()))
        return factoryPresets()[(size_t) index].name;
    return {};
}

void DelayAnatomyLabProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, getNumPrograms()))
        return;

    currentProgram = index;
    userPresetName = {};
    applyValues (valuesFromFactory (index));
    ++presetSerial;
}

void DelayAnatomyLabProcessor::applyValues (const PresetValues& v)
{
    auto set = [this] (const char* id, float value)
    {
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };

    set (ParamID::mode,     (float) v.mode);
    // SYNC 付きのプリセットは、ノブにも今のテンポでの ms を入れておく
    const double bpm = hostBpm.load();
    set (ParamID::timeL,    (float) juce::jlimit (1.0, 2000.0, resolveTimeMs (v.syncL, v.timeL, bpm)));
    set (ParamID::timeR,    (float) juce::jlimit (1.0, 2000.0, resolveTimeMs (v.syncR, v.timeR, bpm)));
    set (ParamID::syncL,    (float) v.syncL);
    set (ParamID::syncR,    (float) v.syncR);
    set (ParamID::feedback, v.feedback);
    set (ParamID::hp,       v.hp);
    set (ParamID::lp,       v.lp);
    set (ParamID::rate,     v.rate);
    set (ParamID::depth,    v.depth);
    set (ParamID::lfoShape, (float) v.lfoShape);
    set (ParamID::delayOff, 0.0f);

    // Machine / Age / Drive / Wow / Flutter：keepMachine のプリセットは今の設定のまま（Default だけ Clean に戻す）
    if (! v.keepMachine)
    {
        set (ParamID::machine, (float) v.machine);
        set (ParamID::age,     v.age);
        set (ParamID::drive,   v.drive);
        set (ParamID::wow,     v.wow);
        set (ParamID::flutter, v.flutter);
    }
    set (ParamID::pitch,     v.pitch);
    set (ParamID::pitchFine, v.pitchFine);
    set (ParamID::pitchMode, (float) v.pitchMode);
    set (ParamID::reverse,   v.reverse ? 1.0f : 0.0f);
    set (ParamID::duck,      v.duck);
    set (ParamID::duckRel,   v.duckRel);

    // Wet Lock が ON なら、Wet の量と Wet Only はプリセットを切り替えても今の設定のまま
    if (apvts.getRawParameterValue (ParamID::wetLock)->load() < 0.5f)
    {
        set (ParamID::wet,     v.wet);
        set (ParamID::wetOnly, v.wetOnly ? 1.0f : 0.0f);
    }
    if (v.mode != (int) DelayMode::Haas)
        set (ParamID::monoCheck, 0.0f);
}

PresetValues DelayAnatomyLabProcessor::captureValues() const
{
    auto g = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    PresetValues v;
    v.mode = (int) g (ParamID::mode);
    v.syncL = (int) g (ParamID::syncL);   v.syncR = (int) g (ParamID::syncR);
    v.timeL = g (ParamID::timeL);         v.timeR = g (ParamID::timeR);
    v.feedback = g (ParamID::feedback);
    v.hp = g (ParamID::hp);               v.lp = g (ParamID::lp);
    v.rate = g (ParamID::rate);           v.depth = g (ParamID::depth);
    v.lfoShape = (int) g (ParamID::lfoShape);
    v.wet = g (ParamID::wet);
    v.wetOnly = g (ParamID::wetOnly) > 0.5f;
    v.keepMachine = false;
    v.machine = (int) g (ParamID::machine);
    v.age = g (ParamID::age);             v.drive = g (ParamID::drive);
    v.wow = g (ParamID::wow);             v.flutter = g (ParamID::flutter);
    v.pitch = g (ParamID::pitch);         v.pitchFine = g (ParamID::pitchFine);
    v.pitchMode = (int) g (ParamID::pitchMode);
    v.reverse = g (ParamID::reverse) > 0.5f;
    v.duck = g (ParamID::duck);           v.duckRel = g (ParamID::duckRel);
    return v;
}

PresetValues DelayAnatomyLabProcessor::getBaseline() const
{
    return userPresetName.isNotEmpty() ? userBaseline : valuesFromFactory (currentProgram);
}

juce::String DelayAnatomyLabProcessor::getPresetDisplayName() const
{
    if (userPresetName.isNotEmpty())
        return userPresetName;
    return juce::String::fromUTF8 (factoryPresets()[(size_t) juce::jlimit (0, (int) factoryPresets().size() - 1, currentProgram)].name);
}

bool DelayAnatomyLabProcessor::isModified() const
{
    const bool wetLocked = apvts.getRawParameterValue (ParamID::wetLock)->load() > 0.5f;
    return presetstore::countDiff (getBaseline(), captureValues(), wetLocked) > 0;
}

bool DelayAnatomyLabProcessor::saveUserPreset (const juce::String& name)
{
    const auto n = presetstore::cleanName (name);
    if (n.isEmpty())
        return false;
    const auto v = captureValues();
    if (! presetstore::write (presetstore::fileFor (n), n, currentProgram, v))
        return false;
    userPresetName = n;
    userBaseline = v;
    ++presetSerial;
    return true;
}

bool DelayAnatomyLabProcessor::loadUserPreset (const juce::File& file)
{
    int basedOn = 0;
    PresetValues v;
    if (! presetstore::read (file, basedOn, v))
        return false;
    currentProgram = basedOn;
    userPresetName = file.getFileNameWithoutExtension();
    userBaseline = v;
    applyValues (v);
    ++presetSerial;
    return true;
}

bool DelayAnatomyLabProcessor::renameUserPreset (const juce::String& newName)
{
    if (userPresetName.isEmpty())
        return false;
    const auto n = presetstore::cleanName (newName);
    if (n.isEmpty())
        return false;
    const auto oldFile = presetstore::fileFor (userPresetName);
    const auto newFile = presetstore::fileFor (n);
    if (newFile.existsAsFile() && newFile != oldFile)
        return false;
    if (! presetstore::write (newFile, n, currentProgram, userBaseline))
        return false;
    if (oldFile != newFile)
        oldFile.deleteFile();
    userPresetName = n;
    ++presetSerial;
    return true;
}

bool DelayAnatomyLabProcessor::deleteUserPreset()
{
    if (userPresetName.isEmpty())
        return false;
    presetstore::fileFor (userPresetName).deleteFile();
    userPresetName = {};          // 音はそのまま。比較表は元にしたファクトリープリセットとの差になる
    ++presetSerial;
    return true;
}

bool DelayAnatomyLabProcessor::importPreset (const juce::File& file)
{
    int basedOn = 0;
    PresetValues v;
    if (! presetstore::read (file, basedOn, v))
        return false;
    const auto n = presetstore::cleanName (file.getFileNameWithoutExtension());
    if (n.isEmpty())
        return false;
    if (! presetstore::write (presetstore::fileFor (n), n, basedOn, v))
        return false;
    return loadUserPreset (presetstore::fileFor (n));
}

bool DelayAnatomyLabProcessor::exportCurrent (const juce::File& file) const
{
    return presetstore::write (file, file.getFileNameWithoutExtension(), currentProgram, captureValues());
}

void DelayAnatomyLabProcessor::revertToPreset()
{
    applyValues (getBaseline());
}

void DelayAnatomyLabProcessor::setParam (const char* id, float value)
{
    if (auto* param = apvts.getParameter (id))
        param->setValueNotifyingHost (param->convertTo0to1 (value));
}

void DelayAnatomyLabProcessor::applyMachine (int m)
{
    const auto d = machineDefaults (m);
    setParam (ParamID::machine, (float) m);
    setParam (ParamID::age, d.age);
    setParam (ParamID::drive, d.drive);
    setParam (ParamID::wow, d.wow);
    setParam (ParamID::flutter, d.flutter);
}

void DelayAnatomyLabProcessor::applyView (int view)
{
    if (view < 1)        // BASIC：機種は Clean
    {
        setParam (ParamID::machine, 0.0f);
        setParam (ParamID::age, 0.0f);   setParam (ParamID::drive, 0.0f);
        setParam (ParamID::wow, 0.0f);   setParam (ParamID::flutter, 0.0f);
    }
    if (view < 2)        // ANALOG 以下：Pitch / Reverse / Duck は無効
    {
        setParam (ParamID::pitch, 0.0f); setParam (ParamID::pitchFine, 0.0f);
        setParam (ParamID::reverse, 0.0f); setParam (ParamID::duck, 0.0f);
    }
}

// =============================================================================
//  状態の保存／復元
// =============================================================================
void DelayAnatomyLabProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    state.removeChild (state.getChildWithName (uiState.getType()), nullptr);
    state.addChild (uiState.createCopy(), -1, nullptr);
    state.removeChild (state.getChildWithName ("DALPreset"), nullptr);
    if (userPresetName.isNotEmpty())               // ユーザープリセットの元の値もプロジェクトに持たせる（ファイルが無くても比較できる）
        state.addChild (presetstore::toTree (userPresetName, currentProgram, userBaseline), -1, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void DelayAnatomyLabProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = juce::jlimit (0, getNumPrograms() - 1, (int) tree.getProperty ("program", 0));
            const auto savedUi = tree.getChildWithName (uiState.getType());
            if (savedUi.isValid())
            {
                uiState.copyPropertiesFrom (savedUi, nullptr);
                tree.removeChild (savedUi, nullptr);
            }
            userPresetName = {};
            const auto savedPreset = tree.getChildWithName ("DALPreset");
            if (savedPreset.isValid())
            {
                int basedOn = currentProgram;
                PresetValues v;
                if (presetstore::fromTree (savedPreset, basedOn, v))
                {
                    userBaseline = v;
                    userPresetName = savedPreset.getProperty ("name").toString();
                }
                tree.removeChild (savedPreset, nullptr);
            }
            apvts.replaceState (tree);
            ++presetSerial;
        }
}

juce::AudioProcessorEditor* DelayAnatomyLabProcessor::createEditor()
{
    return new DelayAnatomyLabEditor (*this);
}

// JUCE がプラグインを生成するときに呼ぶ関数
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelayAnatomyLabProcessor();
}
