MESH_BENCHMARK_TEMPLATES_CPP = $(sort $(wildcard MeshBenchmarks.templates/MeshBenchmarks.t*.cpp))
MESH_BENCHMARK_TEMPLATES_CU = $(sort $(wildcard MeshBenchmarks.templates/MeshBenchmarks.t*.cu))

TARGETS = linear-solvers matrix-writer
SOURCES = linear-solvers.cpp matrix-writer.cpp tnl-benchmark-mesh.cpp $(MESH_BENCHMARK_TEMPLATES_CPP)
CUDA_TARGETS = linear-solvers-cuda
CUDA_SOURCES = linear-solvers-cuda.cu tnl-benchmark-mesh-cuda.cu $(MESH_BENCHMARK_TEMPLATES_CU)

# include general rules
include ../Makefile.base

#CXXFLAGS += -DHAVE_UMFPACK -DHAVE_METIS -DHAVE_ARMADILLO $(OPENMP_CXXFLAGS)
#LDLIBS += -lmetis -larmadillo -lumfpack $(OPENMP_LDLIBS)
#CUDA_CXXFLAGS += -DHAVE_METIS $(OPENMP_CXXFLAGS)
#CUDA_LDLIBS += -lcusparse -lmetis $(OPENMP_LDLIBS)
CXXFLAGS += -DHAVE_UMFPACK -DHAVE_METIS $(OPENMP_CXXFLAGS)
LDLIBS += -lmetis -lumfpack $(OPENMP_LDLIBS)
CUDA_CXXFLAGS += -DHAVE_METIS
CUDA_LDLIBS += -lcusparse -lmetis

# aggregation rules for tnl-benchmark-mesh
host: tnl-benchmark-mesh
tnl-benchmark-mesh: tnl-benchmark-mesh.o $(MESH_BENCHMARK_TEMPLATES_CPP:%.cpp=%.o)
	$(CXX) -o $@ $^ $(LDFLAGS)
cuda: tnl-benchmark-mesh-cuda
tnl-benchmark-mesh-cuda: tnl-benchmark-mesh-cuda.cu.o $(MESH_BENCHMARK_TEMPLATES_CU:%.cu=%.cu.o)
	$(CUDA_COMPILER) $(CUDA_LDFLAGS) -o $@ $^

-include $(SOURCES:%.cpp=%.d)

ifeq ($(CUDA_COMPILER),nvcc)
# nvcc creates .cu.d with rubbish and .d with the content we need
-include $(CUDA_SOURCES:%.cu=%.d)
else
# clang creates .cu.d
-include $(CUDA_SOURCES:%.cu=%.cu.d)
endif
