# All variables are also configurable from the command line (e.g. make TNL_CXX=clang++ )
# or from the environment (e.g. TNL_CXX=clang++ make ).
#
# If you are building targets in a subdirectory, you can create a local config.mk
# file in that subdirectory to override options from the global config.mk file.

# build type (can be 'Release' or 'Debug')
BUILD ?= Release

# compiler for the .cpp files
# Note that we have to use TNL_CXX instead of CXX to set the default value,
# because CXX is an implicit variable defined by GNU make itself. (Thank you!)
TNL_CXX ?= g++
CXX := $(TNL_CXX)

# compiler for the .cu files
CUDA_COMPILER ?= nvcc

# compiler for the host/CPU code used by nvcc
CUDA_HOST_COMPILER ?= $(CXX)

# path to the CUDA toolkit installation
# (autodetection is attempted, set it manually if it fails)
CUDA_PATH ?= $(abspath $(dir $(shell command -v nvcc))/..)
#$(info Detected CUDA_PATH: $(CUDA_PATH))

# CUDA GPU architecture (e.g. "sm_61" or "auto")
# (if you use "auto", tnl-cuda-arch must be installed in your $PATH)
CUDA_GPU_ARCH ?= auto
#CUDA_GPU_ARCH ?= sm_70
