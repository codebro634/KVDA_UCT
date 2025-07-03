#include "../../include/Utils/MemoryAnalysis.h"

#include <iostream>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#endif


namespace MEMORY
{

    void PrintUsedMemory()
    {
        #ifdef _WIN32
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            std::cout << "Used Memory: " << pmc.WorkingSetSize / (1024.0 * 1024.0) << " MB" << std::endl;
        } else {
            std::cerr << "Failed to retrieve used memory information." << std::endl;
        }
        #else
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
            std::cout << "Used Memory: " << usage.ru_maxrss / 1024.0 << " MB" << std::endl;
        } else {
            std::cerr << "Failed to retrieve used memory information." << std::endl;
        }
        #endif
    }

}