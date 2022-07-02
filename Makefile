ifeq ("$(wildcard MeshBenchmarksRunner.templates/)","")
$(shell python3 ./MeshBenchmarksRunner.py)
endif

MESH_BENCHMARK_TEMPLATES_CPP = $(sort $(wildcard MeshBenchmarksRunner.templates/MeshBenchmarksRunner.t*.cpp))
MESH_BENCHMARK_TEMPLATES_CU = $(sort $(wildcard MeshBenchmarksRunner.templates/MeshBenchmarksRunner.t*.cu))

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
tnl-benchmark-mesh-cuda: tnl-benchmark-mesh-cuda.cuo $(MESH_BENCHMARK_TEMPLATES_CU:%.cu=%.cuo)
	$(CXX) $(CUDA_LDFLAGS) -o $@ $^ $(CUDA_LDLIBS)

clean: clean_templates
.PHONY: clean_templates
clean_templates:
	$(RM) tnl-benchmark-mesh tnl-benchmark-mesh-cuda
	$(RM) -r MeshBenchmarksRunner.templates/

-include $(SOURCES:%.cpp=%.d)
-include $(CUDA_SOURCES:%.cu=%.d)
