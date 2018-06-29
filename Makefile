ifeq ("$(wildcard MeshBenchmarks.templates/)","")
$(shell python3 ./MeshBenchmarks.py)
endif

MESH_BENCHMARK_TEMPLATES_CPP = $(sort $(wildcard MeshBenchmarks.templates/MeshBenchmarks.t*.cpp))
MESH_BENCHMARK_TEMPLATES_CU = $(sort $(wildcard MeshBenchmarks.templates/MeshBenchmarks.t*.cu))

SOURCES = tnl-benchmark-mesh.cpp $(MESH_BENCHMARK_TEMPLATES_CPP)
CUDA_SOURCES = tnl-benchmark-mesh-cuda.cu $(MESH_BENCHMARK_TEMPLATES_CU)

# include general rules
include Makefile.base

CXXFLAGS += $(OPENMP_CXXFLAGS)
LDLIBS += $(OPENMP_LDLIBS)

# aggregation rules for tnl-benchmark-mesh
host: tnl-benchmark-mesh
tnl-benchmark-mesh: tnl-benchmark-mesh.o $(MESH_BENCHMARK_TEMPLATES_CPP:%.cpp=%.o)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)
cuda: tnl-benchmark-mesh-cuda
tnl-benchmark-mesh-cuda: tnl-benchmark-mesh-cuda.cu.o $(MESH_BENCHMARK_TEMPLATES_CU:%.cu=%.cu.o)
	$(CUDA_COMPILER) $(CUDA_LDFLAGS) -o $@ $^ $(CUDA_LDLIBS)

clean:
	$(RM) -r MeshBenchmarks.templates/

-include $(SOURCES:%.cpp=%.d)

ifeq ($(CUDA_COMPILER),nvcc)
# nvcc creates .cu.d with rubbish and .d with the content we need
-include $(CUDA_SOURCES:%.cu=%.d)
else
# clang creates .cu.d
-include $(CUDA_SOURCES:%.cu=%.cu.d)
endif
