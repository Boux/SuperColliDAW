# bundle_ugens(<target> <dest dir> <suffix> <out var>): builds SuperCollider's core UGens into <dest dir>, with the file
# suffix scsynth looks for, before <target> builds, and sets <out var> to their targets.
# Mirrors third_party/supercollider/server/plugins/CMakeLists.txt, without the supernova variants.

set(SC_UGENS_DIR ${SC_ROOT}/server/plugins)
# UIUGens is left out: its thread outlives the plugin when a host exits without unloading it, and std::terminate then kills the host.
set(SC_SINGLE_FILE_UGENS
    BinaryOpUGens ChaosUGens DelayUGens DemandUGens DemoUGens DynNoiseUGens FilterUGens GendynUGens GrainUGens IOUGens LFUGens
    MulAddUGens NoiseUGens OscUGens PanUGens PhysicalModelingUGens ReverbUGens TestUGens TriggerUGens UnaryOpUGens UnpackFFTUGens
)

function(bundle_ugens target dest suffix out_var)
    set(ugens)
    foreach(name ${SC_SINGLE_FILE_UGENS})
        add_library(${name} MODULE ${SC_UGENS_DIR}/${name}.cpp)
        list(APPEND ugens ${name})
    endforeach()
    add_library(FFT_UGens MODULE ${SC_UGENS_DIR}/FFTInterfaceTable.cpp ${SC_UGENS_DIR}/FFT_UGens.cpp ${SC_UGENS_DIR}/PV_UGens.cpp ${SC_UGENS_DIR}/PartitionedConvolution.cpp)
    add_library(PV_ThirdParty MODULE ${SC_UGENS_DIR}/Convolution.cpp ${SC_UGENS_DIR}/FFT2InterfaceTable.cpp ${SC_UGENS_DIR}/FeatureDetection.cpp ${SC_UGENS_DIR}/PV_ThirdParty.cpp)
    add_library(ML_UGens MODULE
        ${SC_UGENS_DIR}/ML.cpp
        ${SC_UGENS_DIR}/Loudness.cpp
        ${SC_UGENS_DIR}/BeatTrack.cpp
        ${SC_UGENS_DIR}/Onsets.cpp
        ${SC_UGENS_DIR}/onsetsds.c
        ${SC_UGENS_DIR}/KeyTrack.cpp
        ${SC_UGENS_DIR}/MFCC.cpp
        ${SC_UGENS_DIR}/BeatTrack2.cpp
        ${SC_UGENS_DIR}/ML_SpecStats.cpp
    )
    add_library(DiskIO_UGens MODULE ${SC_UGENS_DIR}/DiskIO_UGens.cpp)
    target_link_libraries(DiskIO_UGens PRIVATE PkgConfig::SNDFILE)
    list(APPEND ugens FFT_UGens PV_ThirdParty ML_UGens DiskIO_UGens)

    foreach(ugen ${ugens})
        target_include_directories(${ugen} PRIVATE ${SC_ROOT}/include/common ${SC_ROOT}/common ${SC_ROOT}/include/plugin_interface ${SC_EXT}/nova-simd ${SC_EXT}/boost)
        target_compile_definitions(${ugen} PRIVATE NOVA_SIMD BOOST_CHRONO_HEADER_ONLY BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE)
        # Upstream SuperCollider code; its warnings are not ours to fix.
        target_compile_options(${ugen} PRIVATE -w -fno-math-errno -fno-finite-math-only)
        set_target_properties(${ugen} PROPERTIES PREFIX "" SUFFIX ${suffix} LIBRARY_OUTPUT_DIRECTORY ${dest} CXX_STANDARD 17)
    endforeach()
    add_dependencies(${target} ${ugens})
    set(${out_var} ${ugens} PARENT_SCOPE)
endfunction()
