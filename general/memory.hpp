#ifndef MEMORYINFO_HPP
#define MEMORYINFO_HPP

#include "processes.hpp"
#include <memory>
#include <windows.h>
#include <pdh.h>

class MemoryInfo{
    private:
        Processes processes;

        unsigned long PDHQueryHelper(PDH_HQUERY& q, DWORD_PTR& dwP, LPCSTR path);
    public:
        MemoryInfo(){processes = Processes();};

        //Returns private working set size of one process in MiB
        double getPhysicalMemoryUsage(const DWORD pid);
        //Returns private working set size in MiB of all processes with the same exe name as the given PID
        double getNamePhysicalMemoryUsage(const DWORD pid);
        //Returns working set size of all non PPL and PP-L processes in MiB
        double getTotalPhysicalMemoryUsage();
        //Returns total usage of all non PPL and PP-L processes as a percentage
        double getPercentageMemoryUsage();
        //Returns total cached memory in MiB
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