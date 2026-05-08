cmake_minimum_required(VERSION 3.22)

project(PitchEdit VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# ── JUCE 7.0.12 ───────────────────────────────────────────────────────────────
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ── ARA SDK 2.2.0 (required by JUCE 7 for ARA2 support) ─────────────────────
# We need just the ARA_Library headers; its CMakeLists has submodule deps we
# don't want.  A plain shallow git clone at configure time is the most
# reliable way to get header-only content without triggering CMake's update
# machinery on subsequent reconfigures.
set(ARA_SDK_ROOT "${CMAKE_BINARY_DIR}/ara_sdk_root")
set(ARA_LIB_DIR  "${ARA_SDK_ROOT}/ARA_Library")

if(NOT EXISTS "${ARA_LIB_DIR}/Debug/ARADebug.h")
    message(STATUS "Cloning ARA_Library 2.2.0…")
    file(MAKE_DIRECTORY "${ARA_SDK_ROOT}")
    execute_process(
        COMMAND git clone --branch releases/2.2.0 --depth 1 --no-local
                    https://github.com/Celemony/ARA_Library.git "${ARA_LIB_DIR}"
        RESULT_VARIABLE _r OUTPUT_QUIET ERROR_QUIET)
    if(NOT _r EQUAL 0)
        message(FATAL_ERROR "Failed to clone ARA_Library. "
            "Supply -DJUCE_GLOBAL_ARA_SDK_PATH=<path> instead.")
    endif()
endif()

# ARA_Library includes ARA_API/ARAInterface.h relative to ara_sdk_root.
set(ARA_API_DIR "${ARA_SDK_ROOT}/ARA_API")
if(NOT EXISTS "${ARA_API_DIR}/ARAInterface.h")
    message(STATUS "Cloning ARA_API 2.2.0…")
    execute_process(
        COMMAND git clone --branch releases/2.2.0 --depth 1 --no-local
                    https://github.com/Celemony/ARA_API.git "${ARA_API_DIR}"
        RESULT_VARIABLE _r OUTPUT_QUIET ERROR_QUIET)
    if(NOT _r EQUAL 0)
        message(FATAL_ERROR "Failed to clone ARA_API.")
    endif()
endif()

# Point JUCE to the root that contains ARA_Library/ (resolves <ARA_Library/…>)
juce_set_ara_sdk_path("${ARA_SDK_ROOT}")

# ── Plugin target ─────────────────────────────────────────────────────────────
juce_add_plugin(PitchEdit
    COMPANY_NAME             "PitchEdit Audio"
    BUNDLE_ID                "com.pitchedit.PitchEdit"
    PLUGIN_MANUFACTURER_CODE PEdt
    PLUGIN_CODE              Pedt
    FORMATS                  VST3
    PRODUCT_NAME             "PitchEdit"
    PLUGIN_DESCRIPTION       "ARA2 pitch and time editor"
    IS_ARA_EFFECT            TRUE
    VST3_CATEGORIES          "Fx" "Analyzer" "Pitch Shift"
    COPY_PLUGIN_AFTER_BUILD  FALSE
)

target_sources(PitchEdit
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/PitchDetector.cpp
        Source/PhaseVocoder.cpp
        Source/NoteSegmenter.cpp
        Source/ARADocumentController.cpp
        Source/NoteCanvas.cpp
)

target_include_directories(PitchEdit PRIVATE Source)

target_compile_definitions(PitchEdit
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(PitchEdit
    PRIVATE
        juce::juce_audio_utils      # includes juce_audio_formats (ARAAudioSourceReader)
        juce::juce_audio_processors # ARA plugin utilities
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
