#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Presets.h"

// =============================================================================
//  PresetStore：ユーザープリセット（.dalpreset の XML）の保存・読み込み・一覧
//   保存先  macOS : ~/Library/Application Support/JPK/Delay Anatomy Lab/Presets
//           Windows: %APPDATA%\JPK\Delay Anatomy Lab\Presets
//   ファイル名（拡張子を除く）＝プリセット名
// =============================================================================
namespace presetstore
{
inline juce::File folder (bool create = true)
{
    auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    base = base.getChildFile ("Application Support");     // JUCE の Mac は ~/Library を返す
   #endif
    auto f = base.getChildFile ("JPK").getChildFile ("Delay Anatomy Lab").getChildFile ("Presets");
    if (create && ! f.isDirectory())
        f.createDirectory();
    return f;
}

inline juce::String cleanName (const juce::String& n)
{
    return juce::File::createLegalFileName (n.trim()).trim().substring (0, 60);
}

inline juce::File fileFor (const juce::String& name)
{
    return folder().getChildFile (cleanName (name) + ".dalpreset");
}

inline juce::Array<juce::File> list()
{
    auto fs = folder (false).findChildFiles (juce::File::findFiles, false, "*.dalpreset");
    std::sort (fs.begin(), fs.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getFileName().compareIgnoreCase (b.getFileName()) < 0; });
    return fs;
}

inline juce::ValueTree toTree (const juce::String& name, int basedOn, const PresetValues& v)
{
    juce::ValueTree t ("DALPreset");
    auto set = [&t] (const char* k, const juce::var& x) { t.setProperty (k, x, nullptr); };
    set ("version", 1);
    set ("name", name);
    set ("basedOn", basedOn);
    set ("mode", v.mode);       set ("syncL", v.syncL);   set ("syncR", v.syncR);
    set ("timeL", v.timeL);     set ("timeR", v.timeR);
    set ("feedback", v.feedback);
    set ("hp", v.hp);           set ("lp", v.lp);
    set ("rate", v.rate);       set ("depth", v.depth);   set ("wet", v.wet);
    set ("lfoShape", v.lfoShape);
    set ("wetOnly", v.wetOnly ? 1 : 0);
    set ("machine", v.machine);
    set ("age", v.age);         set ("drive", v.drive);
    set ("wow", v.wow);         set ("flutter", v.flutter);
    set ("pitch", v.pitch);     set ("pitchFine", v.pitchFine);
    set ("pitchMode", v.pitchMode);
    set ("reverse", v.reverse ? 1 : 0);
    set ("duck", v.duck);       set ("duckRel", v.duckRel);
    return t;
}

inline bool fromTree (const juce::ValueTree& t, int& basedOn, PresetValues& v)
{
    if (! t.hasType ("DALPreset"))
        return false;
    auto f = [&t] (const char* k, float d) { return (float) (double) t.getProperty (k, d); };
    auto i = [&t] (const char* k, int d)   { return (int) t.getProperty (k, d); };
    basedOn = juce::jlimit (0, (int) factoryPresets().size() - 1, i ("basedOn", 0));
    v = PresetValues();
    v.mode = juce::jlimit (0, 3, i ("mode", 0));
    v.syncL = juce::jlimit (0, 8, i ("syncL", 0));
    v.syncR = juce::jlimit (0, 8, i ("syncR", 0));
    v.timeL = f ("timeL", 500.0f);  v.timeR = f ("timeR", 500.0f);
    v.feedback = f ("feedback", 25.0f);
    v.hp = f ("hp", 200.0f);        v.lp = f ("lp", 6000.0f);
    v.rate = f ("rate", 0.5f);      v.depth = f ("depth", 0.2f);   v.wet = f ("wet", -12.0f);
    v.lfoShape = juce::jlimit (0, 4, i ("lfoShape", 0));      // 古いプリセットには無いので Sine
    v.wetOnly = i ("wetOnly", 0) != 0;
    v.keepMachine = false;
    v.machine = juce::jlimit (0, 6, i ("machine", 0));
    v.age = f ("age", 0.0f);        v.drive = f ("drive", 0.0f);
    v.wow = f ("wow", 0.0f);        v.flutter = f ("flutter", 0.0f);
    v.pitch = f ("pitch", 0.0f);    v.pitchFine = f ("pitchFine", 0.0f);
    v.pitchMode = juce::jlimit (0, 1, i ("pitchMode", 0));
    v.reverse = i ("reverse", 0) != 0;
    v.duck = f ("duck", 0.0f);      v.duckRel = f ("duckRel", 250.0f);
    return true;
}

inline bool write (const juce::File& file, const juce::String& name, int basedOn, const PresetValues& v)
{
    file.getParentDirectory().createDirectory();
    if (auto xml = toTree (name, basedOn, v).createXml())
        return xml->writeTo (file);
    return false;
}

inline bool read (const juce::File& file, int& basedOn, PresetValues& v)
{
    if (auto xml = juce::XmlDocument::parse (file))
        return fromTree (juce::ValueTree::fromXml (*xml), basedOn, v);
    return false;
}

// プリセットの値と「いまの値」が何個違うか（ヘッダーの * 表示用）
inline int countDiff (const PresetValues& b, const PresetValues& n, bool wetLocked)
{
    int c = 0;
    auto f = [&c] (float x, float y, float tol) { if (std::abs (x - y) > tol) ++c; };
    if (b.mode != n.mode)   ++c;
    if (b.syncL != n.syncL) ++c;
    if (b.syncR != n.syncR) ++c;
    if (b.syncL == 0 && n.syncL == 0) f (b.timeL, n.timeL, 0.06f);
    if (b.syncR == 0 && n.syncR == 0) f (b.timeR, n.timeR, 0.06f);
    f (b.feedback, n.feedback, 0.06f);
    f (b.hp, n.hp, 0.6f);
    f (b.lp, n.lp, 0.6f);
    f (b.rate, n.rate, 0.006f);
    f (b.depth, n.depth, 0.006f);
    if (b.lfoShape != n.lfoShape) ++c;
    if (! wetLocked) f (b.wet, n.wet, 0.06f);
    if (! b.keepMachine)
    {
        if (b.machine != n.machine) ++c;
        f (b.age, n.age, 0.06f);   f (b.drive, n.drive, 0.06f);
        f (b.wow, n.wow, 0.06f);   f (b.flutter, n.flutter, 0.06f);
    }
    f (b.pitch, n.pitch, 0.5f);
    f (b.pitchFine, n.pitchFine, 0.5f);
    if (b.pitchMode != n.pitchMode) ++c;
    if (b.reverse != n.reverse)     ++c;
    f (b.duck, n.duck, 0.06f);
    f (b.duckRel, n.duckRel, 0.6f);
    return c;
}
} // namespace presetstore
