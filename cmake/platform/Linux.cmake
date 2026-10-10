# Linux: an X11 editor window, and SuperCollider's core UGens bundled next to the plugin.

find_package(X11 REQUIRED COMPONENTS Xcursor Xrandr Xext)
target_sources(ui_pugl PRIVATE ${PUGL_PATCHED_DIR}/src/x11.c ${PUGL_DIR}/src/x11_gl.c)
target_compile_definitions(ui_pugl PRIVATE USE_XCURSOR=1 USE_XRANDR=1 USE_XSYNC=1 _POSIX_C_SOURCE=200809L)
target_link_libraries(ui_pugl PUBLIC X11::X11 X11::Xcursor X11::Xrandr X11::Xext)

set(SC_PLUGIN_SUFFIX .so)
target_compile_definitions(scsynth_embedded PUBLIC "SC_PLUGIN_EXT=\"${SC_PLUGIN_SUFFIX}\"")
target_link_libraries(scsynth_embedded PUBLIC rt)
pkg_check_modules(FFTW3F REQUIRED IMPORTED_TARGET fftw3f)
target_compile_definitions(scsynth_embedded PUBLIC SC_FFT_FFTW)
target_link_libraries(scsynth_embedded PUBLIC PkgConfig::FFTW3F)

target_sources(supercollidaw_engine PRIVATE src/engine/InstalledSuperCollider_linux.cpp)
target_sources(supercollidaw_lang PRIVATE src/lang/SclangProcess_linux.cpp src/lang/StdinWriter_posix.cpp)
target_sources(supercollidaw_ui PRIVATE src/ui/EditorWindow_linux.cpp)
target_sources(supercollidaw_clap PRIVATE src/plugin/PluginGui_linux.cpp src/plugin/PluginPaths_linux.cpp src/plugin/reaper/ReaperTextField_linux.cpp)
target_link_options(supercollidaw_clap PRIVATE -Wl,--no-undefined -Wl,--exclude-libs,ALL)

get_target_property(PLUGIN_BINARY_DIR supercollidaw_clap BINARY_DIR)
bundle_ugens(supercollidaw_clap ${PLUGIN_BINARY_DIR}/SuperColliDAW/plugins ${SC_PLUGIN_SUFFIX} BUNDLED_UGENS)

add_executable(clap_host_test tests/clap_host_test.cpp src/plugin/state/PluginState.cpp src/plugin/params/ParameterSpec.cpp)
target_include_directories(clap_host_test PRIVATE ${CMAKE_SOURCE_DIR}/third_party/clap/include ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(clap_host_test PRIVATE ${CMAKE_DL_LIBS})
add_dependencies(clap_host_test supercollidaw_clap)
add_test(NAME clap_host_test COMMAND clap_host_test $<TARGET_FILE:supercollidaw_clap>)

add_executable(clap_gui_host tests/clap_gui_host.cpp src/plugin/state/PluginState.cpp src/plugin/params/ParameterSpec.cpp)
target_include_directories(clap_gui_host PRIVATE ${CMAKE_SOURCE_DIR}/third_party/clap/include ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(clap_gui_host PRIVATE X11::X11 ${CMAKE_DL_LIBS})
add_dependencies(clap_gui_host supercollidaw_clap)
