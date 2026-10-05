cmake_minimum_required(VERSION 3.22)
project(supercollidaw VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

include(cmake/patches.cmake)
include(cmake/scsynth.cmake)
include(cmake/ugens.cmake)
include(cmake/embed.cmake)
include(cmake/resources.cmake)
include(cmake/ui.cmake)

add_subdirectory(third_party/tiny-process-library EXCLUDE_FROM_ALL)

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)

include(cmake/platform/${CMAKE_SYSTEM_NAME}.cmake)
