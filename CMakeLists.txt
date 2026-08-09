cmake_minimum_required(VERSION 3.22)

project(PhatForge VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Pull in JUCE automatically. If you already have a local JUCE checkout,
# set -DFETCHCONTENT_SOURCE_DIR_JUCE=/path/to/JUCE when running cmake and it
# will be used instead of downloading anything.
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG         7.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(PhatForge
    COMPANY_NAME        "OpenTone Audio"
    PLUGIN_MANUFACTURER_CODE OpTn
    PLUGIN_CODE          Phtf
    FORMATS              VST3 AU Standalone
    PRODUCT_NAME         "PhatForge"
    IS_SYNTH             FALSE
    NEEDS_MIDI_INPUT     FALSE
    NEEDS_MIDI_OUTPUT    FALSE
    IS_MIDI_EFFECT       FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES      Fx Distortion
    AU_MAIN_TYPE         kAudioUnitType_Effect
)

target_sources(PhatForge PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/XYPad.cpp
    Source/PresetManager.cpp
)

target_compile_definitions(PhatForge PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(PhatForge PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
