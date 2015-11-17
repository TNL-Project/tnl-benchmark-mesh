TARGETS = linear-solvers matrix-writer
SOURCES = linear-solvers.cpp matrix-writer.cpp
CUDA_TARGETS = linear-solvers-cuda
CUDA_SOURCES = linear-solvers-cuda.cu

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

-include $(SOURCES:%.cpp=%.d)

ifeq ($(CUDA_COMPILER),nvcc)
# nvcc creates .cu.d with rubbish and .d with the content we need
-include $(CUDA_SOURCES:%.cu=%.d)
else
# clang creates .cu.d
-include $(CUDA_SOURCES:%.cu=%.cu.d)
endif
