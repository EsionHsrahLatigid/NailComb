cmake_minimum_required(VERSION 3.22)

project(NailComb VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(NAILCOMB_BUILD_PLUGIN "Build the JUCE plug-in targets" ON)
option(NAILCOMB_BUILD_TESTS "Build the test targets" ON)
set(NAILCOMB_ARTIFACT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/artifacts" CACHE PATH "Stable product staging root")
set(EHL_JUCE_SOURCE_DIR "" CACHE PATH "Existing JUCE source checkout; avoids network when set")

add_library(nailcomb_dsp STATIC
    Source/dsp/NailCombDSP.cpp)
target_include_directories(nailcomb_dsp PUBLIC Source)
target_compile_features(nailcomb_dsp PUBLIC cxx_std_17)
if(MSVC)
    target_compile_options(nailcomb_dsp PRIVATE /W4)
else()
    target_compile_options(nailcomb_dsp PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(NAILCOMB_BUILD_TESTS)
    enable_testing()
    add_executable(nailcomb_dsp_tests Tests/DSPTests.cpp)
    target_link_libraries(nailcomb_dsp_tests PRIVATE nailcomb_dsp)
    add_test(NAME nailcomb_dsp_tests COMMAND nailcomb_dsp_tests)
endif()

if(NAILCOMB_BUILD_PLUGIN)
    include(FetchContent)
    if(EHL_JUCE_SOURCE_DIR)
        if(NOT EXISTS "${EHL_JUCE_SOURCE_DIR}/CMakeLists.txt")
            message(FATAL_ERROR "EHL_JUCE_SOURCE_DIR does not contain JUCE: ${EHL_JUCE_SOURCE_DIR}")
        endif()
        add_subdirectory("${EHL_JUCE_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/juce")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
        add_subdirectory(JUCE)
    else()
        FetchContent_Declare(
            JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG 91ad83ae34a81e0833b1a2b0866f54846370ae53
            GIT_SHALLOW FALSE)
        FetchContent_MakeAvailable(JUCE)
    endif()

    set(NAILCOMB_FORMATS VST3 Standalone)
    if(APPLE)
        list(APPEND NAILCOMB_FORMATS AU)
    endif()

    juce_add_plugin(NailComb
        VERSION ${PROJECT_VERSION}
        COMPANY_NAME "EsionHsrahLatigid"
        BUNDLE_ID "jp.ehl.nailcomb"
        PRODUCT_NAME "NailComb"
        DESCRIPTION "four-voice digital harsh-noise feedback comb filter"
        PLUGIN_MANUFACTURER_CODE EHL_
        PLUGIN_CODE NlCb
        FORMATS ${NAILCOMB_FORMATS}
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        MICROPHONE_PERMISSION_ENABLED TRUE
        MICROPHONE_PERMISSION_TEXT "NailComb processes live audio input."
        COPY_PLUGIN_AFTER_BUILD FALSE)

    target_sources(NailComb PRIVATE
        Source/ParameterIDs.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h)
    target_include_directories(NailComb PRIVATE Source)
    target_compile_definitions(NailComb PUBLIC
        JUCE_REPORT_APP_USAGE=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0)
    target_link_libraries(NailComb
        PRIVATE
            nailcomb_dsp
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

    target_sources(NailComb_Standalone PRIVATE Source/StandaloneApp.cpp)
    target_compile_definitions(NailComb_Standalone PRIVATE JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1)

    set(ehl_stage_platform "linux-x64")
    if(APPLE)
        set(ehl_stage_platform "macos-arm64")
    elseif(WIN32)
        set(ehl_stage_platform "windows-x64")
    endif()
    set(ehl_juce_products "${CMAKE_CURRENT_BINARY_DIR}/NailComb_artefacts/$<CONFIG>")
    set(ehl_staged_products "${NAILCOMB_ARTIFACT_ROOT}/plugin-release/${ehl_stage_platform}")

    set(ehl_stage_dependencies NailComb_VST3 NailComb_Standalone)
    set(ehl_stage_commands
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ehl_staged_products}/vst3"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ehl_staged_products}/standalone"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ehl_staged_products}/au")

    if(APPLE)
        list(APPEND ehl_stage_commands
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${ehl_staged_products}/vst3/nailcomb_vst3_plugin.vst3"
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${ehl_staged_products}/standalone/nailcomb_standalone_plugin.app"
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${ehl_staged_products}/au/nailcomb_au_plugin.component"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${ehl_juce_products}/VST3/NailComb.vst3" "${ehl_staged_products}/vst3/nailcomb_vst3_plugin.vst3"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${ehl_juce_products}/Standalone/NailComb.app" "${ehl_staged_products}/standalone/nailcomb_standalone_plugin.app"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${ehl_juce_products}/AU/NailComb.component" "${ehl_staged_products}/au/nailcomb_au_plugin.component"
            COMMAND codesign --force --deep --sign - "${ehl_staged_products}/vst3/nailcomb_vst3_plugin.vst3"
            COMMAND codesign --force --deep --sign - "${ehl_staged_products}/standalone/nailcomb_standalone_plugin.app"
            COMMAND codesign --force --deep --sign - "${ehl_staged_products}/au/nailcomb_au_plugin.component")
        list(APPEND ehl_stage_dependencies NailComb_AU)
    elseif(WIN32)
        list(APPEND ehl_stage_commands
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${ehl_staged_products}/vst3/nailcomb_vst3_plugin.vst3"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${ehl_juce_products}/VST3/NailComb.vst3" "${ehl_staged_products}/vst3/nailcomb_vst3_plugin.vst3"
            COMMAND ${CMAKE_COMMAND} -E copy "${ehl_juce_products}/Standalone/NailComb.exe" "${ehl_staged_products}/standalone/nailcomb_standalone_plugin.exe")
    else()
        list(APPEND ehl_stage_commands
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${ehl_juce_products}/VST3/NailComb.vst3" "${ehl_staged_products}/vst3/nailcomb_vst3_plugin.vst3"
            COMMAND ${CMAKE_COMMAND} -E copy "${ehl_juce_products}/Standalone/NailComb" "${ehl_staged_products}/standalone/nailcomb_standalone_plugin")
    endif()

    list(APPEND ehl_stage_commands
        COMMAND ${CMAKE_COMMAND}
            -DPRODUCT_NAME=NailComb
            -DSLUG=nailcomb
            -DBUNDLE_ID=jp.ehl.nailcomb
            -DPLUGIN_CODE=NlCb
            -DSTAGE_DIR=${ehl_staged_products}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WriteArtifactsManifest.cmake")

    add_custom_target(ehl_stage_products
        ${ehl_stage_commands}
        DEPENDS ${ehl_stage_dependencies}
        COMMENT "Staging NailComb products under ${NAILCOMB_ARTIFACT_ROOT}/plugin-release")

    if(NAILCOMB_BUILD_TESTS)
        add_executable(nailcomb_plugin_tests Tests/PluginTests.cpp)
        target_include_directories(nailcomb_plugin_tests PRIVATE Source)
        target_link_libraries(nailcomb_plugin_tests PRIVATE NailComb juce::juce_audio_utils)
        add_test(NAME nailcomb_plugin_tests COMMAND nailcomb_plugin_tests)

        add_executable(nailcomb_editor_tests Tests/EditorTests.cpp)
        target_include_directories(nailcomb_editor_tests PRIVATE Source)
        target_link_libraries(nailcomb_editor_tests PRIVATE NailComb juce::juce_audio_utils)
        add_test(NAME nailcomb_editor_tests COMMAND nailcomb_editor_tests)

        add_test(
            NAME nailcomb_artifact_contract
            COMMAND ${CMAKE_COMMAND}
                -DSTAGE_DIR=${ehl_staged_products}
                -DSLUG=nailcomb
                -DEXPECT_AU=$<BOOL:${APPLE}>
                -P "${CMAKE_CURRENT_SOURCE_DIR}/Tests/VerifyArtifacts.cmake")
    endif()
endif()
