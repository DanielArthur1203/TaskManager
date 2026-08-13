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

        //Returns working set size of one process in MiB
        SIZE_T getPhysicalMemoryUsage(const DWORD pid);
        //Returns working set size of all non PPL and PP-L processes in MiB
        SIZE_T getTotalPhysicalMemoryUsage();
        //Returns total usage of all non PPL and PP-L processes as a percentage
        unsigned int getPercentageMemoryUsage();
        //Returns total cached memory in MiB(takes at least 4 seconds to finish)
        unsigned long getTotalCachedMemory();
        //Returns system commit limit in GiB
        double getSystemCommitLimit();
        //Returns current system commit usage in GiB
        double getCurrentCommitUsage();
        //Returns current paged pool in MiB
        unsigned long getPagedPool();
        //Returns non paged pool in MiB
        unsigned long getNonPagedPool();
        //Returns configured memory clock speed in MT/s
        unsigned long getMemorySpeed();
        //Returns number of used RAM slots
        unsigned short getNumUsedRAMSlots();
        //Returns total number of RAM slots
        unsigned short getTotalRAMSlots();

};

#endif