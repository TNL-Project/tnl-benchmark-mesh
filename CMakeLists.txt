cmake_minimum_required(VERSION 3.24)
project(tnl-benchmark-mesh
    LANGUAGES CXX CUDA
)

# Require C++17, and disable compiler-specific extensions (if possible).
foreach(lang CXX CUDA)
    set(CMAKE_${lang}_STANDARD 17)
    set(CMAKE_${lang}_STANDARD_REQUIRED ON)
    set(CMAKE_${lang}_EXTENSIONS OFF)
endforeach()

# Set build flags for CXX
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -Werror=vla")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_CXX_FLAGS_DEBUG}")

# Set build flags for CUDA
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Wall")
set(CMAKE_CUDA_FLAGS_DEBUG "-g")
set(CMAKE_CUDA_FLAGS_RELEASE "-O3 -DNDEBUG -Xcompiler -march=native -Xcompiler -mtune=native")
set(CMAKE_CUDA_FLAGS_RELWITHDEBINFO "${CMAKE_CUDA_FLAGS_RELEASE} ${CMAKE_CUDA_FLAGS_DEBUG} --generate-line-info")

# Enable link-time optimization
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# Set CUDA architectures for which the executables will be compiled.
# See https://cmake.org/cmake/help/latest/prop_tgt/CUDA_ARCHITECTURES.html
if( NOT DEFINED CMAKE_CUDA_ARCHITECTURES )
    # NOTE: "all" and "all-major" are available since CMake 3.23, "native" is available since CMake 3.24
    set(CMAKE_CUDA_ARCHITECTURES "native")
endif()

# Set up bundled dependencies
add_subdirectory(libs/tnl)

# Find third-party dependencies
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
target_link_libraries(tnl-benchmark-mesh PUBLIC TNL::TNL_CXX)
target_link_libraries(tnl-benchmark-mesh-cuda PUBLIC TNL::TNL_CUDA)
foreach( target IN ITEMS ${MY_TARGETS} )
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
