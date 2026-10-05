# Windows: a Win32 editor window, and SuperCollider's core UGens bundled next to the plugin.

target_sources(ui_pugl PRIVATE ${PUGL_DIR}/src/win.c ${PUGL_DIR}/src/win_gl.c)
target_compile_definitions(ui_pugl PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN)
target_link_libraries(ui_pugl PUBLIC dwmapi gdi32 shell32 shlwapi user32)

target_sources(scsynth_embedded PRIVATE ${SC_ROOT}/common/SC_Win32Utils.cpp)
target_compile_definitions(scsynth_embedded PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0600 PRIVATE UNICODE _UNICODE)
target_link_libraries(scsynth_embedded PUBLIC ws2_32 mswsock winmm)

target_sources(supercollidaw_engine PRIVATE src/engine/InstalledSuperCollider_win.cpp)
target_sources(supercollidaw_lang PRIVATE src/lang/SclangProcess_win.cpp src/lang/StdinWriter_win.cpp)
target_sources(supercollidaw_ui PRIVATE src/ui/EditorWindow_win.cpp)
target_sources(supercollidaw_clap PRIVATE src/plugin/PluginGui_win.cpp src/plugin/PluginPaths_win.cpp)

get_target_property(PLUGIN_BINARY_DIR supercollidaw_clap BINARY_DIR)
bundle_ugens(supercollidaw_clap ${PLUGIN_BINARY_DIR}/SuperColliDAW/plugins BUNDLED_UGENS)
foreach(ugen ${BUNDLED_UGENS})
    target_compile_definitions(${ugen} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0600)
    target_link_libraries(${ugen} PRIVATE ws2_32)
endforeach()
