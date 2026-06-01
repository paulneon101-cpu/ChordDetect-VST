cmake_minimum_required(VERSION 3.22)
project(ChordDetect VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.9
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(ChordDetect
    COMPANY_NAME            "ChordDetectAudio"
    PLUGIN_MANUFACTURER_CODE Cdct
    PLUGIN_CODE             Chdt
    FORMATS                 VST3 Standalone
    PRODUCT_NAME            "ChordDetect"
    IS_MIDI_EFFECT          FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       TRUE
    IS_SYNTH                FALSE
    PLUGIN_VST3_CATEGORY    "Analyzer Fx"
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(ChordDetect)

target_sources(ChordDetect PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/ChordDetector.cpp
    Source/PitchDetector.cpp
    Source/MonoPitchDetector.cpp
    Source/HarmonyGenerator.cpp
    Source/AudioEffectChain.cpp
    Source/MidiRecorder.cpp
    Source/ChordAdvisor.cpp
)

target_include_directories(ChordDetect PRIVATE Source)

target_compile_definitions(ChordDetect
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(ChordDetect
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
