cmake_minimum_required(VERSION 3.22)
project(DPSV55Controller VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.0
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(DPSV55Controller
    COMPANY_NAME                "HomeStudio"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           TRUE
    IS_MIDI_EFFECT              TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     TRUE
    PLUGIN_MANUFACTURER_CODE    Hstu
    PLUGIN_CODE                 Dp55
    FORMATS                     VST3
    PRODUCT_NAME                "DPS-V55 Controller"
    DESCRIPTION                 "MIDI controller for Sony DPS-V55M Multi-Effect Processor"
    VST3_CATEGORIES             "Fx|Tools"
)

juce_generate_juce_header(DPSV55Controller)

target_sources(DPSV55Controller PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(DPSV55Controller
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(DPSV55Controller
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
