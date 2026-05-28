cmake_minimum_required(VERSION 3.16)

project(RoomAcoustiCPP LANGUAGES CXX)

# Apple Clang doesn't ship with OpenMP; requires libomp installed via Homebrew.
if(APPLE)
    execute_process(
        COMMAND brew --prefix libomp
        OUTPUT_VARIABLE LIBOMP_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT LIBOMP_PREFIX)
        message(FATAL_ERROR "libomp not found. Install with: brew install libomp")
    endif()
    set(OpenMP_C_FLAGS "-Xpreprocessor -fopenmp -I${LIBOMP_PREFIX}/include")
    set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp -I${LIBOMP_PREFIX}/include")
    set(OpenMP_C_LIB_NAMES "omp")
    set(OpenMP_CXX_LIB_NAMES "omp")
    set(OpenMP_omp_LIBRARY "${LIBOMP_PREFIX}/lib/libomp.dylib")
endif()

find_package(OpenMP REQUIRED)


#
# 3DTI
#

# Generate a list of all source files in the 3dti_AudioToolkit directory and save it in TDTI_SOURCES variable
file(GLOB_RECURSE TDTI_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/3dti_AudioToolkit/3dti_Toolkit/BinauralSpatializer/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/3dti_AudioToolkit/3dti_Toolkit/Common/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/3dti_AudioToolkit/3dti_Toolkit/HAHLSimulation/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/3dti_AudioToolkit/3dti_Toolkit/ISM/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/3dti_AudioToolkit/3dti_ResourceManager/*.cpp"
)

# Do not compile bundled third-party example/sandbox sources.
list(FILTER TDTI_SOURCES EXCLUDE REGEX "/third_party_libraries/")

# Add the 3DTI library based on the source files defined in TDTI_SOURCES variable
add_library(3DTI STATIC ${TDTI_SOURCES})
set_target_properties(3DTI PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Include directories for 3DTI library to allow other targets to find the necessary header files
target_include_directories(3DTI
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/3dti_AudioToolkit"
        "${CMAKE_CURRENT_SOURCE_DIR}/3dti_AudioToolkit/3dti_Toolkit"
        "${CMAKE_CURRENT_SOURCE_DIR}/3dti_AudioToolkit/3dti_ResourceManager"
        "${CMAKE_CURRENT_SOURCE_DIR}/3dti_AudioToolkit/3dti_ResourceManager/third_party_libraries/boost_circular_buffer"
        "${CMAKE_CURRENT_SOURCE_DIR}/3dti_AudioToolkit/3dti_ResourceManager/third_party_libraries/cereal/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/3dti_AudioToolkit/3dti_ResourceManager/third_party_libraries/eigen"
        "${CMAKE_CURRENT_SOURCE_DIR}/3dti_AudioToolkit/3dti_ResourceManager/third_party_libraries/sofacoustics/libsofa/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/3dti_AudioToolkit/3dti_ResourceManager/third_party_libraries/sofacoustics/libsofa/dependencies/include"
        )

target_compile_features(3DTI PUBLIC cxx_std_17)

#
# RoomAcoustiCPP
#

# Generate a list of all source files in the RoomAcoustiCpp directory and save it in RAC_SOURCES variable
file(GLOB_RECURSE RAC_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/RoomAcoustiCpp/source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/RoomAcoustiCpp/codegen/dll/myNN/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/RoomAcoustiCpp/codegen/lib/myNN/source/*.cpp"
)

# Exclude MATLAB MEX/interface and example entry points from runtime library build.
list(FILTER RAC_SOURCES EXCLUDE REGEX "/interface/")
list(FILTER RAC_SOURCES EXCLUDE REGEX "/examples/")
list(FILTER RAC_SOURCES EXCLUDE REGEX "_coder_.*\\.cpp$")
list(FILTER RAC_SOURCES EXCLUDE REGEX ".*_mex\\.cpp$")

# Add the RoomAcoustiCPP library based on the source files defined in RAC_SOURCES variable
add_library(RoomAcoustiCPP STATIC ${RAC_SOURCES})
set_target_properties(RoomAcoustiCPP PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Matlab MEX interface files are excluded from compilation (filtered above),
# so Matlab linkage is optional — skip if not installed.
find_package(Matlab COMPONENTS MX_LIBRARY ENG_LIBRARY)

if(Matlab_FOUND)
    target_link_libraries(RoomAcoustiCPP PRIVATE Matlab::mx Matlab::eng)
    target_include_directories(RoomAcoustiCPP PRIVATE ${Matlab_INCLUDE_DIRS})
endif()

target_link_libraries(RoomAcoustiCPP
    PUBLIC
        3DTI
    OpenMP::OpenMP_CXX
)

target_include_directories(RoomAcoustiCPP
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/RoomAcoustiCpp/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/RoomAcoustiCpp/include/Spatialiser"
        "${CMAKE_CURRENT_SOURCE_DIR}/RoomAcoustiCpp/codegen/dll/myNN"
        "${CMAKE_CURRENT_SOURCE_DIR}/RoomAcoustiCpp/codegen/lib/myNN/include"
)

target_compile_features(RoomAcoustiCPP PUBLIC cxx_std_20)

# Apple's libc++ doesn't implement std::atomic<std::shared_ptr<T>> yet; force-include the polyfill.
if(APPLE)
    target_compile_options(RoomAcoustiCPP PRIVATE
        -include "${CMAKE_CURRENT_SOURCE_DIR}/RoomAcoustiCpp/include/Common/AtomicSharedPtr.h"
    )
endif()

if(WIN32)
    target_compile_definitions(RoomAcoustiCPP
        PRIVATE
            RPC_NO_WINDOWS_H
            WIN32_LEAN_AND_MEAN
    )
endif()

