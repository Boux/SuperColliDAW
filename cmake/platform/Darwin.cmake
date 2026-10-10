# macOS: a Cocoa editor window, scsynth's FFT through Apple's vDSP as in SuperCollider's own build, and SuperCollider's core UGens bundled next to the plugin.

enable_language(OBJC OBJCXX)

target_sources(ui_pugl PRIVATE ${PUGL_DIR}/src/mac.m ${PUGL_DIR}/src/mac_gl.m)
target_compile_definitions(ui_pugl PUBLIC GL_SILENCE_DEPRECATION)
target_link_libraries(ui_pugl PUBLIC "-framework Cocoa" "-framework CoreVideo")

set_property(SOURCE ${SC_ROOT}/common/SC_Filesystem_macos.cpp PROPERTY COMPILE_OPTIONS -xobjective-c++)
target_sources(scsynth_embedded PRIVATE ${SC_ROOT}/common/SC_Apple.mm)
set_property(SOURCE ${SC_ROOT}/common/SC_Apple.mm PROPERTY COMPILE_OPTIONS -fobjc-exceptions)
target_compile_definitions(scsynth_embedded PUBLIC SC_FFT_VDSP)
target_link_libraries(scsynth_embedded PUBLIC "-framework Accelerate" "-framework AppKit" "-framework CoreServices" "-framework Foundation")

target_sources(supercollidaw_engine PRIVATE src/engine/InstalledSuperCollider_mac.cpp)
target_sources(supercollidaw_lang PRIVATE src/lang/SclangProcess_mac.cpp src/lang/StdinWriter_posix.cpp)
target_sources(supercollidaw_ui PRIVATE src/ui/EditorWindow_mac.cpp)
target_sources(supercollidaw_clap PRIVATE src/plugin/PluginGui_mac.cpp src/plugin/PluginPaths_mac.cpp src/plugin/reaper/ReaperTextField_mac.cpp)
set_target_properties(supercollidaw_clap PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION clap
    SUFFIX ""
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/packaging/macos/Info.plist.in
    MACOSX_BUNDLE_GUI_IDENTIFIER org.supercollidaw.supercollidaw
    MACOSX_BUNDLE_BUNDLE_NAME SuperColliDAW
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
)

get_target_property(PLUGIN_BINARY_DIR supercollidaw_clap BINARY_DIR)
bundle_ugens(supercollidaw_clap ${PLUGIN_BINARY_DIR}/SuperColliDAW/plugins .scx BUNDLED_UGENS)
foreach(ugen FFT_UGens PV_ThirdParty ML_UGens)
    target_link_libraries(${ugen} PRIVATE "-framework Accelerate")
endforeach()
target_link_libraries(DiskIO_UGens PRIVATE "-framework CoreServices")
