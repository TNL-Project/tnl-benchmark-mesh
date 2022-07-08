cmake_minimum_required(VERSION 3.23)
project(tnl-benchmark-mesh
    LANGUAGES CXX CUDA
)

# Require C++14, and disable compiler-specific extensions (if possible).
foreach(lang CXX CUDA)
    set(CMAKE_${lang}_STANDARD 14)
    set(CMAKE_${lang}_STANDARD_REQUIRED ON)
    set(CMAKE_${lang}_EXTENSIONS OFF)
endforeach()

# Set build flags for CXX
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -Werror=vla")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_CXX_FLAGS_DEBUG}")

# Pass -rdynamic only in Debug mode
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS_DEBUG "-rdynamic")
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS_RELEASE "")
set(CMAKE_EXE_LINKER_FLAGS "")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-rdynamic")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")
set(CMAKE_SHARED_LINKER_FLAGS "")
set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "-rdynamic")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "")

# Force colorized output in continuous integration and Ninja
if( DEFINED ENV{CI_JOB_NAME} OR ${CMAKE_GENERATOR} STREQUAL "Ninja" )
    message(STATUS "Continuous integration or Ninja detected -- forcing compilers to produce colorized output.")
    if( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics" )
    elseif( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color" )
    endif()
endif()

# Enable link-time optimization
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# Set build flags for CUDA
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Wall -DHAVE_CUDA --expt-relaxed-constexpr --expt-extended-lambda --default-stream per-thread")
set(CMAKE_CUDA_FLAGS_DEBUG "-g")
set(CMAKE_CUDA_FLAGS_RELEASE "-O3 -DNDEBUG -Xcompiler -march=native -Xcompiler -mtune=native")
set(CMAKE_CUDA_FLAGS_RELWITHDEBINFO "${CMAKE_CUDA_FLAGS_RELEASE} ${CMAKE_CUDA_FLAGS_DEBUG} --generate-line-info")
# Disable false compiler warnings
#   reference for the -Xcudafe --diag_suppress and --display_error_number flags: https://stackoverflow.com/a/54142937
#   incomplete list of tokens: http://www.ssl.berkeley.edu/~jimm/grizzly_docs/SSL/opt/intel/cc/9.0/lib/locale/en_US/mcpcom.msg
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Wno-deprecated-gpu-targets -Xcudafe --diag_suppress=code_is_unreachable -Xcudafe --diag_suppress=loop_not_reachable -Xcudafe --diag_suppress=implicit_return_from_non_void_function -Xcudafe --diag_suppress=unsigned_compare_with_zero -Xcudafe --display_error_number")
# This diagnostic is just plain wrong in CUDA 9 and later, see https://github.com/kokkos/kokkos/issues/1470
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xcudafe --diag_suppress=esa_on_defaulted_function_ignored")

# Set CUDA architectures for which the executables will be compiled.
# See https://cmake.org/cmake/help/latest/prop_tgt/CUDA_ARCHITECTURES.html
if( NOT DEFINED CMAKE_CUDA_ARCHITECTURES )
    # NOTE: "all" and "all-major" are available since CMake 3.23
    set(CMAKE_CUDA_ARCHITECTURES "all-major")
    # NOTE: "native" is available since CMake 3.24
    #set(CMAKE_CUDA_ARCHITECTURES "native")
endif()

# Set up bundled dependencies
include_directories(SYSTEM libs/tnl/src libs/tnl/src/3rdparty)

# Find third-party dependencies
find_package(OpenMP COMPONENTS CXX)
find_package(ZLIB)
find_package(tinyxml2)

# Add executables
file( GLOB EXPLICIT_TEMPLATES_CPP MeshBenchmarksRunner.templates/*.cpp )
file( GLOB EXPLICIT_TEMPLATES_CU MeshBenchmarksRunner.templates/*.cu )
add_executable(tnl-benchmark-mesh tnl-benchmark-mesh.cpp ${EXPLICIT_TEMPLATES_CPP})
add_executable(tnl-benchmark-mesh-cuda tnl-benchmark-mesh-cuda.cu ${EXPLICIT_TEMPLATES_CU})

# Set target options
set(MY_TARGETS
    tnl-benchmark-mesh
    tnl-benchmark-mesh-cuda
)
foreach( target IN ITEMS ${MY_TARGETS} )
    target_link_libraries(${target} PUBLIC stdc++fs)
    if( OPENMP_FOUND )
        target_compile_definitions(${target} PUBLIC "-DHAVE_OPENMP")
        target_link_libraries(${target} PUBLIC OpenMP::OpenMP_CXX)
    endif()
    if( ZLIB_FOUND )
        target_compile_definitions(${target} PUBLIC "-DHAVE_ZLIB")
        target_include_directories(${target} PUBLIC ${ZLIB_INCLUDE_DIRS})
        target_link_libraries(${target} PUBLIC ${ZLIB_LIBRARIES})
    endif()
    if( tinyxml2_FOUND )
        target_compile_definitions(${target} PUBLIC "-DHAVE_TINYXML2")
        target_link_libraries(${target} PUBLIC tinyxml2::tinyxml2)
    endif()
endforeach()
