add_library(supercollidaw_engine OBJECT
    engine/Engine.cpp
    engine/InstalledSuperCollider.cpp
    engine/SC_PluginDriver.cpp
)
target_include_directories(supercollidaw_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(supercollidaw_engine PUBLIC scsynth_embedded)

add_library(supercollidaw_clap MODULE
    plugin/entry.cpp
    plugin/Plugin.cpp
    plugin/TestSynth.cpp
)
embed_binary(supercollidaw_clap supercollidaw_test_scsyndef ${CMAKE_SOURCE_DIR}/assets/supercollidaw_test.scsyndef)
target_include_directories(supercollidaw_clap PRIVATE ${CMAKE_SOURCE_DIR}/third_party/clap/include)
target_link_libraries(supercollidaw_clap PRIVATE supercollidaw_engine)
set_target_properties(supercollidaw_clap PROPERTIES OUTPUT_NAME SuperColliDAW PREFIX "" SUFFIX ".clap")
if(UNIX AND NOT APPLE)
    target_link_options(supercollidaw_clap PRIVATE -Wl,--no-undefined -Wl,--exclude-libs,ALL)
endif()
