#ifndef MEMORYINFO_HPP
#define MEMORYINFO_HPP

#include "processes.hpp"
#include <memory>
#include <windows.h>

class MemoryInfo{
    private:
        Processes processes;
    public:
        MemoryInfo(){processes = Processes();};

        //Returns working set size of one process in MB
        SIZE_T getPhysicalMemoryUsage(const DWORD pid);
        //Returns working set size of all non PPL and PP-L processes in MB
        SIZE_T getTotalPhysicalMemoryUsage();
};

#endif