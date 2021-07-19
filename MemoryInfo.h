#pragma once

// References:
// - https://stackoverflow.com/a/64166/4180822
// - https://lemire.me/blog/2020/03/03/calling-free-or-delete/
// - https://stackoverflow.com/questions/15529643/what-does-malloc-trim0-really-mean

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/sysinfo.h>
#include <malloc.h>

inline long
getTotalVirtualMemory()
{
    struct sysinfo memInfo;
    sysinfo (&memInfo);

    long totalVirtualMem = memInfo.totalram;
    // Add other values in next statement to avoid int overflow on right hand side...
    totalVirtualMem += memInfo.totalswap;
    totalVirtualMem *= memInfo.mem_unit;

    return totalVirtualMem;
}

inline long
getUsedVirtualMemory()
{
    struct sysinfo memInfo;
    sysinfo (&memInfo);

    long virtualMemUsed = memInfo.totalram - memInfo.freeram;
    // Add other values in next statement to avoid int overflow on right hand side...
    virtualMemUsed += memInfo.totalswap - memInfo.freeswap;
    virtualMemUsed *= memInfo.mem_unit;

    return virtualMemUsed;
}

inline long
parseLine(char* line)
{
    // This assumes that a digit will be found and the line ends in " kB".
    int i = strlen(line);
    const char* p = line;
    while (*p <'0' || *p > '9') p++;
    line[i-3] = '\0';
    return atol(p);
}

// virtual memory currently used by the calling process
inline long
getSelfVirtualMemory()
{
    // explicitly release unused memory
    malloc_trim(0);

    FILE* file = fopen("/proc/self/status", "r");
    long result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmSize:", 7) == 0){
            // convert from kB to B
            result = parseLine(line) * 1024;
            break;
        }
    }
    fclose(file);
    return result;
}

inline long
getTotalPhysicalMemory()
{
    struct sysinfo memInfo;
    sysinfo (&memInfo);

    long totalPhysMem = memInfo.totalram;
    //Multiply in next statement to avoid int overflow on right hand side...
    totalPhysMem *= memInfo.mem_unit;

    return totalPhysMem;
}

inline long
getUsedPhysicalMemory()
{
    struct sysinfo memInfo;
    sysinfo (&memInfo);

    long physMemUsed = memInfo.totalram - memInfo.freeram;
    //Multiply in next statement to avoid int overflow on right hand side...
    physMemUsed *= memInfo.mem_unit;

    return physMemUsed;
}

inline long
getSelfPhysicalMemory()
{
    // explicitly release unused memory
    malloc_trim(0);

    FILE* file = fopen("/proc/self/status", "r");
    long result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmRSS:", 6) == 0){
            // convert from kB to B
            result = parseLine(line) * 1024;
            break;
        }
    }
    fclose(file);
    return result;
}


#include <TNL/Benchmarks/Benchmarks.h>
#include <TNL/Config/ParameterContainer.h>
#include <TNL/Containers/StaticVector.h>

struct MemoryBenchmarkResult
: public TNL::Benchmarks::BenchmarkResult
{
   using HeaderElements = TNL::Benchmarks::Logging::HeaderElements;
   using RowElements = TNL::Benchmarks::Logging::RowElements;

   double memory = std::numeric_limits<double>::quiet_NaN();
   double memstddev = std::numeric_limits<double>::quiet_NaN();

   virtual HeaderElements getTableHeader() const override
   {
      return HeaderElements({ "time", "stddev", "stddev/time", "bandwidth", "speedup", "memory", "memstddev", "memstddev/memory" });
   }

   virtual RowElements getRowElements() const override
   {
      RowElements elements;
      elements << time << stddev << stddev / time << bandwidth;
      if( speedup != 0 )
         elements << speedup;
      else
         elements << "N/A";
      elements << memory << memstddev << memstddev / memory;
      return elements;
   }
};

template< typename Mesh >
MemoryBenchmarkResult
testMemoryUsage( const TNL::Config::ParameterContainer& parameters,
                 const Mesh& mesh )
{
    const long baseline = getSelfPhysicalMemory();
    const Mesh m1 = mesh;
    const long check1 = getSelfPhysicalMemory();
    const Mesh m2 = mesh;
    const long check2 = getSelfPhysicalMemory();
    const Mesh m3 = mesh;
    const long check3 = getSelfPhysicalMemory();
    const Mesh m4 = mesh;
    const long check4 = getSelfPhysicalMemory();
    const Mesh m5 = mesh;
    const long check5 = getSelfPhysicalMemory();
    const Mesh m6 = mesh;
    const long check6 = getSelfPhysicalMemory();
    const Mesh m7 = mesh;
    const long check7 = getSelfPhysicalMemory();
    const Mesh m8 = mesh;
    const long check8 = getSelfPhysicalMemory();
    const Mesh m9 = mesh;
    const long check9 = getSelfPhysicalMemory();
    const Mesh m10 = mesh;
    const long check10 = getSelfPhysicalMemory();

    TNL::Containers::StaticVector< 10, long > data;
    data[0] = check1 - baseline;
    data[1] = check2 - check1;
    data[2] = check3 - check2;
    data[3] = check4 - check3;
    data[4] = check5 - check4;
    data[5] = check6 - check5;
    data[6] = check7 - check6;
    data[7] = check8 - check7;
    data[8] = check9 - check8;
    data[9] = check10 - check9;

    const double mean = TNL::sum( data ) / (double) data.getSize();
    const double stddev = 1.0 / std::sqrt( data.getSize() - 1 ) * TNL::l2Norm( data - mean );

    MemoryBenchmarkResult result;
    result.memory = mean / 1024.0 / 1024.0;  // MiB
    result.memstddev = stddev / 1024.0 / 1024.0;  // MiB
    return result;
}
