# Builds scsynth as a static library from the SuperCollider submodule, without any
# audio backend. src/engine provides SC_NewAudioDriver and the timing functions.
# Mirrors third_party/supercollider/server/scsynth/CMakeLists.txt.

set(SC_ROOT ${CMAKE_SOURCE_DIR}/third_party/supercollider)
set(SC_EXT ${SC_ROOT}/external_libraries)

file(STRINGS ${SC_ROOT}/SCVersion.txt sc_version_lines REGEX "^set\\(SC_VERSION_(MAJOR|MINOR|PATCH) ")
foreach(line ${sc_version_lines})
    string(REGEX REPLACE "^set\\((SC_VERSION_[A-Z]+) ([0-9]+)\\)$" "\\1;\\2" pair "${line}")
    list(GET pair 0 name)
    list(GET pair 1 value)
    set(${name} ${value})
endforeach()
set(SC_VERSION_TWEAK "")
set(GIT_REF_TYPE "tag")
set(GIT_BRANCH_OR_TAG "Version-${SC_VERSION_MAJOR}.${SC_VERSION_MINOR}.${SC_VERSION_PATCH}")
set(GIT_COMMIT_HASH "embedded")
set(SC_GENERATED_DIR ${CMAKE_BINARY_DIR}/sc_generated)
configure_file(${SC_ROOT}/common/SC_Version.hpp.in ${SC_GENERATED_DIR}/SC_Version.hpp)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SNDFILE REQUIRED IMPORTED_TARGET sndfile)
pkg_check_modules(FFTW3F REQUIRED IMPORTED_TARGET fftw3f)

add_library(sc_tlsf STATIC ${SC_EXT}/TLSF-2.4.6/src/tlsf.c)
target_compile_definitions(sc_tlsf PRIVATE TLSF_STATISTIC=1)
target_include_directories(sc_tlsf INTERFACE ${SC_EXT}/TLSF-2.4.6/src)

set(SC_PATCHED_DIR ${CMAKE_BINARY_DIR}/sc_patched)
patch_sources(${SC_ROOT} ${SC_PATCHED_DIR} ${CMAKE_SOURCE_DIR}/patches/supercollider server/scsynth/SC_World.cpp)

set(SCSYNTH_DIR ${SC_ROOT}/server/scsynth)
add_library(scsynth_embedded STATIC
    ${SCSYNTH_DIR}/SC_BufGen.cpp
    ${SCSYNTH_DIR}/SC_ComPort.cpp
    ${SCSYNTH_DIR}/SC_CoreAudio.cpp
    ${SCSYNTH_DIR}/SC_Graph.cpp
    ${SCSYNTH_DIR}/SC_GraphDef.cpp
    ${SCSYNTH_DIR}/SC_Group.cpp
    ${SCSYNTH_DIR}/SC_Lib_Cintf.cpp
    ${SCSYNTH_DIR}/SC_Lib.cpp
    ${SCSYNTH_DIR}/SC_MiscCmds.cpp
    ${SCSYNTH_DIR}/SC_Node.cpp
    ${SCSYNTH_DIR}/SC_Rate.cpp
    ${SCSYNTH_DIR}/SC_SequencedCommand.cpp
    ${SCSYNTH_DIR}/SC_Str4.cpp
    ${SCSYNTH_DIR}/SC_Unit.cpp
    ${SCSYNTH_DIR}/SC_UnitDef.cpp
    ${SC_PATCHED_DIR}/server/scsynth/SC_World.cpp
    ${SCSYNTH_DIR}/Rendezvous.cpp
    ${SC_ROOT}/common/SC_Filesystem_macos.cpp
    ${SC_ROOT}/common/SC_Filesystem_win.cpp
    ${SC_ROOT}/common/SC_Filesystem_unix.cpp
    ${SC_ROOT}/common/SC_Filesystem_iphone.cpp
    ${SC_ROOT}/common/SC_fftlib.cpp
    ${SC_ROOT}/common/SC_AllocPool.cpp
    ${SC_ROOT}/common/SC_Errors.cpp
    ${SC_ROOT}/common/SC_Reply.cpp
    ${SC_ROOT}/common/SC_StringBuffer.cpp
    ${SC_ROOT}/common/SC_StringParser.cpp
    ${SC_ROOT}/common/Samp.cpp
    ${SC_ROOT}/common/sc_popen.cpp
    ${SC_ROOT}/common/SC_ServerBootDelayWarning.cpp
)

target_include_directories(scsynth_embedded PUBLIC
    ${SC_ROOT}/include/common
    ${SC_ROOT}/common
    ${SC_ROOT}/include/server
    ${SC_ROOT}/include/plugin_interface
    ${SCSYNTH_DIR}
    ${SC_EXT}
    ${SC_EXT}/nova-simd
    ${SC_EXT}/nova-tt
    ${SC_EXT}/boost
    ${SC_EXT}/boost_sync/include
    ${SC_GENERATED_DIR}
)

# SC_AUDIO_API must not match any of the upstream backends in SC_CoreAudio.h,
# so none of their code paths are compiled in.
target_compile_definitions(scsynth_embedded PUBLIC
    SC_AUDIO_API_PLUGIN=100
    SC_AUDIO_API=SC_AUDIO_API_PLUGIN
    SC_MEMORY_ALIGNMENT=32
    NOVA_SIMD
    SC_FFT_FFTW
    BOOST_CHRONO_HEADER_ONLY
    BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE
    _REENTRANT
)
if(UNIX AND NOT APPLE)
    target_compile_definitions(scsynth_embedded PUBLIC "SC_PLUGIN_EXT=\".so\"")
endif()

target_link_libraries(scsynth_embedded PUBLIC sc_tlsf PkgConfig::SNDFILE PkgConfig::FFTW3F Threads::Threads ${CMAKE_DL_LIBS})
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_libraries(scsynth_embedded PUBLIC rt)
endif()
if(WIN32)
    target_sources(scsynth_embedded PRIVATE ${SC_ROOT}/common/SC_Win32Utils.cpp)
    target_compile_definitions(scsynth_embedded PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0600 PRIVATE UNICODE _UNICODE)
    target_link_libraries(scsynth_embedded PUBLIC ws2_32 mswsock winmm)
endif()

# Upstream SuperCollider code; its warnings are not ours to fix.
target_compile_options(scsynth_embedded PRIVATE -w)
set_target_properties(scsynth_embedded PROPERTIES CXX_STANDARD 17)
