# copy_resources(<target> <source dir> <glob> <dest dir>): copies the matching files, keeping subfolders, before <target> builds.
function(copy_resources target source glob dest)
    file(GLOB_RECURSE files CONFIGURE_DEPENDS RELATIVE ${source} ${source}/${glob})
    set(copies)
    foreach(file ${files})
        get_filename_component(dir ${dest}/${file} DIRECTORY)
        add_custom_command(
            OUTPUT ${dest}/${file}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${dir}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${source}/${file} ${dest}/${file}
            DEPENDS ${source}/${file}
            VERBATIM
        )
        list(APPEND copies ${dest}/${file})
    endforeach()
    get_filename_component(name ${dest} NAME)
    add_custom_target(${target}_${name} ALL DEPENDS ${copies})
    add_dependencies(${target} ${target}_${name})
endfunction()
