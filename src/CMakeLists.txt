add_library(supercollidaw_engine OBJECT
    engine/Engine.cpp
    engine/InstalledSuperCollider.cpp
    engine/OscPort.cpp
    engine/SC_PluginDriver.cpp
)
target_include_directories(supercollidaw_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(supercollidaw_engine PUBLIC scsynth_embedded)

add_library(supercollidaw_lang OBJECT
    lang/PostLog.cpp
    lang/SclangProcess.cpp
)
target_include_directories(supercollidaw_lang PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(supercollidaw_lang PUBLIC scsynth_embedded tiny-process-library)

add_library(supercollidaw_ui OBJECT
    ui/CodeRegion.cpp
    ui/EditorView.cpp
    ui/EditorWindow.cpp
    ui/PostWindow.cpp
    ui/PuglClipboard.cpp
    ui/PuglImGuiInput.cpp
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
    plugin/code/DefaultCode.cpp
    plugin/code/LinkedFile.cpp
)
embed_binary(supercollidaw_clap supercollidaw_default_scd ${CMAKE_SOURCE_DIR}/assets/default.scd)
target_include_directories(supercollidaw_clap PRIVATE ${CMAKE_SOURCE_DIR}/third_party/clap/include)
target_link_libraries(supercollidaw_clap PRIVATE supercollidaw_engine supercollidaw_lang supercollidaw_ui)
set_target_properties(supercollidaw_clap PROPERTIES OUTPUT_NAME SuperColliDAW PREFIX "" SUFFIX ".clap")
if(UNIX AND NOT APPLE)
    target_link_options(supercollidaw_clap PRIVATE -Wl,--no-undefined -Wl,--exclude-libs,ALL)
endif()
add_custom_command(TARGET supercollidaw_clap POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/sclang $<TARGET_FILE_DIR:supercollidaw_clap>/SuperColliDAW/classes
    VERBATIM
)
