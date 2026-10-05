# TODO(macos): pugl's mac*.m sources and frameworks, EditorWindow_mac.cpp, PluginGui_mac.cpp (CLAP_WINDOW_API_COCOA), bundled UGens,
# and SclangProcess_mac.cpp reading _NSGetEnviron(), because a bundle cannot link environ.
target_sources(supercollidaw_engine PRIVATE src/engine/InstalledSuperCollider_mac.cpp)
target_sources(supercollidaw_lang PRIVATE src/lang/StdinWriter_posix.cpp)
target_sources(supercollidaw_clap PRIVATE src/plugin/PluginPaths_mac.cpp)
