add_executable(code_region_test code_region_test.cpp ${CMAKE_SOURCE_DIR}/src/ui/CodeRegion.cpp ${CMAKE_SOURCE_DIR}/src/ui/CodeScanner.cpp)
target_include_directories(code_region_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME code_region_test COMMAND code_region_test)

add_executable(call_context_test call_context_test.cpp ${CMAKE_SOURCE_DIR}/src/ui/CallContext.cpp ${CMAKE_SOURCE_DIR}/src/ui/CodeScanner.cpp)
target_include_directories(call_context_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME call_context_test COMMAND call_context_test)

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

add_executable(settings_file_test settings_file_test.cpp ${CMAKE_SOURCE_DIR}/src/plugin/settings/SettingsFile.cpp ${CMAKE_SOURCE_DIR}/src/plugin/code/LinkedFile.cpp)
target_include_directories(settings_file_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME settings_file_test COMMAND settings_file_test)

add_executable(sample_clock_test sample_clock_test.cpp ${CMAKE_SOURCE_DIR}/src/engine/SampleClock.cpp)
target_include_directories(sample_clock_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME sample_clock_test COMMAND sample_clock_test)

find_program(SCLANG_EXECUTABLE sclang)
if(SCLANG_EXECUTABLE)
    foreach(name completion signatures)
        add_test(NAME ${name}_test COMMAND ${SCLANG_EXECUTABLE} --include-path ${CMAKE_BINARY_DIR}/src/SuperColliDAW/classes ${CMAKE_CURRENT_SOURCE_DIR}/${name}_test.scd)
        set_tests_properties(${name}_test PROPERTIES PASS_REGULAR_EXPRESSION "${name}: all passed" TIMEOUT 60)
    endforeach()
endif()
