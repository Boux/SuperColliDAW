add_executable(clap_host_test clap_host_test.cpp ${CMAKE_SOURCE_DIR}/src/plugin/state/PluginState.cpp)
target_include_directories(clap_host_test PRIVATE ${CMAKE_SOURCE_DIR}/third_party/clap/include ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(clap_host_test PRIVATE ${CMAKE_DL_LIBS})
add_dependencies(clap_host_test supercollidaw_clap)
add_test(NAME clap_host_test COMMAND clap_host_test $<TARGET_FILE:supercollidaw_clap>)

find_package(X11 REQUIRED)
add_executable(clap_gui_host clap_gui_host.cpp)
target_include_directories(clap_gui_host PRIVATE ${CMAKE_SOURCE_DIR}/third_party/clap/include)
target_link_libraries(clap_gui_host PRIVATE X11::X11 ${CMAKE_DL_LIBS})
add_dependencies(clap_gui_host supercollidaw_clap)

add_executable(code_region_test code_region_test.cpp ${CMAKE_SOURCE_DIR}/src/ui/CodeRegion.cpp)
target_include_directories(code_region_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME code_region_test COMMAND code_region_test)

add_executable(code_document_test code_document_test.cpp
    ${CMAKE_SOURCE_DIR}/src/plugin/code/CodeDocument.cpp
    ${CMAKE_SOURCE_DIR}/src/plugin/code/LinkedFile.cpp
    ${CMAKE_SOURCE_DIR}/src/plugin/state/PluginState.cpp
)
target_include_directories(code_document_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME code_document_test COMMAND code_document_test)
