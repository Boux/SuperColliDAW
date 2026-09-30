# embed_binary(<target> <symbol> <file>): compiles <file> into <target> as
#   extern const unsigned char <symbol>[]; extern const unsigned long <symbol>_size;
function(embed_binary target symbol file)
    set(out ${CMAKE_CURRENT_BINARY_DIR}/embedded/${symbol}.cpp)
    add_custom_command(
        OUTPUT ${out}
        COMMAND ${CMAKE_COMMAND} -DIN=${file} -DOUT=${out} -DSYMBOL=${symbol} -P ${CMAKE_SOURCE_DIR}/cmake/embed_file.cmake
        DEPENDS ${file} ${CMAKE_SOURCE_DIR}/cmake/embed_file.cmake
        VERBATIM
    )
    target_sources(${target} PRIVATE ${out})
endfunction()
