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

        std::unique_ptr<SIZE_T> getPhysicalMemoryUsage(const DWORD pid);
};

#endif