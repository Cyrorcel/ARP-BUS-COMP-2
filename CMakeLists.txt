cmake_minimum_required(VERSION 3.22)

set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

project(ARPBusComp VERSION 2.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_binary_data(ARPAssets SOURCES
    assets/bg.png assets/glass.png
    assets/led_on.png assets/led_off.png
    assets/tog_up.png assets/tog_down.png
    assets/skirtS.png assets/topS.png assets/skirtB.png assets/topB.png)

juce_add_plugin(ARPBusComp
    COMPANY_NAME "ARPEGIA"
    PLUGIN_MANUFACTURER_CODE Arpg
    PLUGIN_CODE Abc1
    FORMATS VST3 AU
    PRODUCT_NAME "ARP BUS COMP"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES Fx Dynamics
    AU_MAIN_TYPE kAudioUnitType_Effect)

target_sources(ARPBusComp PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp)

target_compile_definitions(ARPBusComp PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(ARPBusComp
    PRIVATE
        ARPAssets
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
