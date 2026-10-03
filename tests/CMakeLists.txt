add_executable(clap_host_test clap_host_test.cpp ${CMAKE_SOURCE_DIR}/src/plugin/state/PluginState.cpp ${CMAKE_SOURCE_DIR}/src/plugin/params/ParameterSpec.cpp)
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
    ${CMAKE_SOURCE_DIR}/src/plugin/params/ParameterSpec.cpp
)
target_include_directories(code_document_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME code_document_test COMMAND code_document_test)

add_executable(synthdef_controls_test synthdef_controls_test.cpp ${CMAKE_SOURCE_DIR}/src/engine/SynthDefControls.cpp)
target_include_directories(synthdef_controls_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME synthdef_controls_test COMMAND synthdef_controls_test ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/params.scsyndef)

add_executable(parameter_spec_test parameter_spec_test.cpp ${CMAKE_SOURCE_DIR}/src/plugin/params/ParameterSpec.cpp)
target_include_directories(parameter_spec_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME parameter_spec_test COMMAND parameter_spec_test)

add_executable(server_output_test server_output_test.cpp)
target_link_libraries(server_output_test PRIVATE supercollidaw_engine)
add_test(NAME server_output_test COMMAND server_output_test)

add_executable(sample_clock_test sample_clock_test.cpp ${CMAKE_SOURCE_DIR}/src/engine/SampleClock.cpp)
target_include_directories(sample_clock_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME sample_clock_test COMMAND sample_clock_test)
