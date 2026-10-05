add_library(supercollidaw_text STATIC text/LineBuffer.cpp)
target_include_directories(supercollidaw_text PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(supercollidaw_engine OBJECT
    engine/Engine.cpp
    engine/InstalledSuperCollider.cpp
    engine/OscMessage.cpp
    engine/OscPort.cpp
    engine/SampleClock.cpp
    engine/SC_PluginDriver.cpp
    engine/ServerOutput.cpp
    engine/SynthDefControls.cpp
)
target_include_directories(supercollidaw_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(supercollidaw_engine PUBLIC scsynth_embedded supercollidaw_text)

add_library(supercollidaw_lang OBJECT
    lang/PostLog.cpp
    lang/SclangOutbox.cpp
    lang/SclangProcess.cpp
    lang/StdinWriter.cpp
)
target_include_directories(supercollidaw_lang PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(supercollidaw_lang PUBLIC scsynth_embedded tiny-process-library supercollidaw_text)

add_library(supercollidaw_ui OBJECT
    ui/CallContext.cpp
    ui/CodeCompletion.cpp
    ui/CodeEditor.cpp
    ui/CodeFlash.cpp
    ui/CodeRegion.cpp
    ui/CodeScanner.cpp
    ui/EditorView.cpp
    ui/EditorWindow.cpp
    ui/ExamplesPanel.cpp
    ui/PostWindow.cpp
    ui/PuglClipboard.cpp
    ui/PuglImGuiInput.cpp
    ui/SignatureHint.cpp
    ui/SuperColliderLanguage.cpp
    ${CMAKE_SOURCE_DIR}/third_party/ImGuiColorTextEdit/example/dejavu.cpp
)
target_include_directories(supercollidaw_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ${CMAKE_SOURCE_DIR}/third_party/ImGuiColorTextEdit/example)
target_link_libraries(supercollidaw_ui PUBLIC ui_pugl ui_imgui ui_texteditor)

add_library(supercollidaw_clap MODULE
    plugin/entry.cpp
    plugin/Plugin.cpp
    plugin/PluginGui.cpp
    plugin/PluginPaths.cpp
    plugin/code/CodeController.cpp
    plugin/code/CodeDocument.cpp
    plugin/code/LanguageReplies.cpp
    plugin/code/DefaultCode.cpp
    plugin/code/Examples.cpp
    plugin/code/FileDialog.cpp
    plugin/code/LinkedFile.cpp
    plugin/midi/MidiInput.cpp
    plugin/midi/MidiOutput.cpp
    plugin/params/ParameterBank.cpp
    plugin/params/ParameterEventReader.cpp
    plugin/params/ParameterExtension.cpp
    plugin/params/ParameterSpec.cpp
    plugin/params/ParameterWatcher.cpp
    plugin/state/ClapStreams.cpp
    plugin/state/PluginState.cpp
    plugin/transport/TransportBuses.cpp
    plugin/transport/TransportFollower.cpp
)
embed_binary(supercollidaw_clap supercollidaw_default_scd ${CMAKE_SOURCE_DIR}/assets/default.scd)
target_include_directories(supercollidaw_clap PRIVATE ${CMAKE_SOURCE_DIR}/third_party/clap/include ${CMAKE_SOURCE_DIR}/third_party/portable-file-dialogs)
target_link_libraries(supercollidaw_clap PRIVATE supercollidaw_engine supercollidaw_lang supercollidaw_ui)
set_target_properties(supercollidaw_clap PROPERTIES OUTPUT_NAME SuperColliDAW PREFIX "" SUFFIX ".clap")
copy_resources(supercollidaw_clap ${CMAKE_SOURCE_DIR}/sclang *.sc ${CMAKE_CURRENT_BINARY_DIR}/SuperColliDAW/classes)
copy_resources(supercollidaw_clap ${CMAKE_SOURCE_DIR}/assets/examples *.scd ${CMAKE_CURRENT_BINARY_DIR}/SuperColliDAW/examples)
