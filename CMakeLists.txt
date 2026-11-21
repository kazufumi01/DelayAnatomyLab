cmake_minimum_required(VERSION 3.22)

# ---- macOS 設定（project() より前に書く必要がある） ----------------------
# macOS 11 以降で動くバイナリを作る。Apple Silicon / Intel 両対応のユニバーサル。
# （Windows では無視される）
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures")

# ---- Windows 設定 ----------------------------------------------------------
# C++ ランタイムを静的リンク → ユーザーが VC++ 再頒布パッケージを入れなくても動く
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(DelayAnatomyLab VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ビルド後に自分の Plug-Ins フォルダへコピーするか（手元では ON、配布ビルドでは OFF）
option(DAL_COPY_AFTER_BUILD "Copy plugins to the system plug-in folders after build" ON)

# ---- JUCE の取得 -----------------------------------------------------------
# 手元の JUCE フォルダを使う場合（build.sh はこちら）：
#   cmake -B build -DJUCE_PATH=$HOME/Documents/jpkplugins/JUCE
# 指定しなければ GitHub から JUCE_GIT_TAG を取得（GitHub Actions はこちら）。
# ※ 手元と配布ビルドで JUCE のバージョンを揃えること（現在 9.0.4）
# ※ JUCE_DIR という名前は CMake の find_package が使う予約名なので使わない
set(JUCE_PATH "" CACHE PATH "Local JUCE folder (optional)")
set(JUCE_GIT_TAG "9.0.4" CACHE STRING "JUCE tag to fetch when JUCE_PATH is empty")
if(JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        ${JUCE_GIT_TAG}
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# ---- 出力フォーマット（AU は Mac のみ） ------------------------------------
if(APPLE)
    set(DAL_FORMATS AU VST3 Standalone)
else()
    set(DAL_FORMATS VST3 Standalone)
endif()

# ---- プラグイン本体 --------------------------------------------------------
juce_add_plugin(DelayAnatomyLab
    PRODUCT_NAME              "Delay Anatomy Lab"
    COMPANY_NAME              "JPK"
    BUNDLE_ID                 com.jpk.delayanatomylab
    PLUGIN_MANUFACTURER_CODE  Jpkl          # 4文字・大文字を1つ以上
    PLUGIN_CODE               Dal1          # 4文字・プラグインごとに固有
    FORMATS                   ${DAL_FORMATS}
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    AU_MAIN_TYPE              kAudioUnitType_Effect
    VST3_CATEGORIES           Fx Delay
    COPY_PLUGIN_AFTER_BUILD   ${DAL_COPY_AFTER_BUILD})

target_sources(DelayAnatomyLab PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(DelayAnatomyLab PRIVATE Source)

option(DAL_SNAPSHOT "Dev only: write UI screenshots and quit" OFF)
if(DAL_SNAPSHOT)
    target_compile_definitions(DelayAnatomyLab PRIVATE DAL_SNAPSHOT=1)
endif()

target_compile_definitions(DelayAnatomyLab PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

if(MSVC)
    target_compile_options(DelayAnatomyLab PRIVATE /utf-8 /bigobj)   # 日本語文字列のため
endif()

target_link_libraries(DelayAnatomyLab
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
