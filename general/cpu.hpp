#ifndef CPU_HPP
#define CPU_HPP

#include "processes.hpp"
#include <windows.h>
#include <vector>
#include <string>
#include <array>

class Cpu{
    private:
        Processes processes;

        //Helper that converts FILETIME fields to unsigned long long
        unsigned long long fileTimeToULL(const FILETIME& ft);
        //Helper that returns CPU base speed
        double getBaseSpeed();
        //Helper that returns vector of structs holder processor information
        std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> getProcessorInfo();
        double getUsageByTime(HANDLE h, unsigned long long& lastK, unsigned long long& lastU, unsigned long long& lastS);
    public:
        Cpu(){
            processes = Processes();
        };

        //Returns percent CPU usage
        double currentUsage();
        //IDK if this is accurate this is really annoying
        double processUsage(DWORD pid);
        //Returns CPU clock speed in GHz by multiplying the usage ratio by the base speed
        double cpuClockSpeed();
        //Returns number of logical processors
        unsigned int processorCount();
        //Returns the number of CPU threads 
        unsigned int threadCount();
        //Returns number of handles that do not belong to PPL or PP-L processes
        //Sadly makes the return extremely off 
        unsigned int handleCount();
        //Returns CPU uptime in d:h:m:s format
        std::string upTime();
        //Returns an array holding the sizes of L1, L2, L30 caches
        std::array<double, 3> cacheAmounts();
        //Returns # of CPU cores
        unsigned short coreCount();
        //Inclined to believe that this works but since I have HyperV on it will always return false
        //bool virtualizationState();
};

#endif