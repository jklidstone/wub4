cmake_minimum_required(VERSION 3.22)
project(WUB4 VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()

# Use a local JUCE checkout if given (-DJUCE_DIR=...), otherwise download JUCE 8.
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.10
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(WUB4_FORMATS VST3)
if(APPLE)
    list(APPEND WUB4_FORMATS AU)
endif()

juce_add_plugin(WUB4
    COMPANY_NAME "WUB4"
    BUNDLE_ID "com.wub4.riddimsynth"
    PRODUCT_NAME "WUB4"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Wubz
    PLUGIN_CODE Wb4r
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    FORMATS ${WUB4_FORMATS})

juce_generate_juce_header(WUB4)

target_sources(WUB4 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(WUB4 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

if(UNIX AND NOT APPLE)
    # Headless CI / test builds on Linux don't need these optional X11 extensions.
    target_compile_definitions(WUB4 PUBLIC JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XRENDER=0 JUCE_USE_XCURSOR=0)
endif()

target_link_libraries(WUB4
    PRIVATE
        juce::juce_audio_processors
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Offline render test (no JUCE needed)
add_executable(wub4_render_test tools/render_test.cpp)

# Optional: a tiny VST3 host used to smoke-test the built plugin (cmake -DWUB4_HOST_TEST=ON)
option(WUB4_HOST_TEST "Build the VST3 host smoke test" OFF)
if(WUB4_HOST_TEST)
    juce_add_console_app(wub4_host_test PRODUCT_NAME "wub4_host_test")
    target_sources(wub4_host_test PRIVATE tools/host_test/main.cpp)
    juce_generate_juce_header(wub4_host_test)
    target_compile_definitions(wub4_host_test PRIVATE JUCE_PLUGINHOST_VST3=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    if(UNIX AND NOT APPLE)
        target_compile_definitions(wub4_host_test PRIVATE JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
    endif()
    target_link_libraries(wub4_host_test PRIVATE juce::juce_audio_processors juce::juce_recommended_config_flags)
endif()
if(WUB4_HOST_TEST)
    juce_add_console_app(wub4_snapshot PRODUCT_NAME "wub4_snapshot")
    target_sources(wub4_snapshot PRIVATE tools/host_test/snapshot.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    juce_generate_juce_header(wub4_snapshot)
    target_compile_definitions(wub4_snapshot PRIVATE JucePlugin_Name="WUB4" JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(wub4_snapshot PRIVATE juce::juce_audio_processors juce::juce_recommended_config_flags)
endif()
