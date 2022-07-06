# tnl-benchmark-mesh

Benchmarks for the unstructured mesh in TNL. Two algorithms (calculation of cell measures and
calculation of curve lengths/surface areas around vertices) were presented in the paper
[Configurable open-source data structure for distributed conforming unstructured homogeneous meshes
with GPU support](https://doi.org/10.1145/3536164).

## Quickstart

1. Make sure that [Git](https://git-scm.com/), [Git LFS](https://git-lfs.github.com/),
   [CMake](https://cmake.org/), [GNU Make](https://www.gnu.org/software/make/),
   [Python 3](https://www.python.org/), and
   [CUDA 11](https://developer.nvidia.com/cuda-toolkit) are installed on your Linux system.

2. Clone the repository and initialize the submodules:

       git clone --recurse-submodules https://gitlab.com/tnl-project/tnl-benchmark-mesh.git

3. Build the benchmark binaries:

       cd tnl-benchmark-mesh
       python3 MeshBenchmarksRunner.py
       cmake -B build -S . -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
       make -C build

4. Run the script to execute the benchmarks on the meshes included in the repository:

       ./run_benchmark
