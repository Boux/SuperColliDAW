# A DAW loads the plugin from its CLAP folder, where Windows does not look for the DLLs the plugin depends on.
# Included after project(), because CMake's Windows-GNU platform module resets the library suffixes during project().
set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
add_link_options(-static)
