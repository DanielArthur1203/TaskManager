#ifndef CPU_HPP
#define CPU_HPP

#include "processes.hpp"
#include <windows.h>
#include <vector>

class Cpu{
    private:
        Processes processes;
        //Helper that converts FILETIME fields to unsigned long long
        unsigned long long fileTimeToULL(const FILETIME& ft);
        //Helper that returns CPU base speed
        double getBaseSpeed();
    public:
        Cpu(){
            processes = Processes();
        };

        //Returns percent CPU usage
        double currentUsage();
        //Returns CPU clock speed in GHz by multiplying the usage ratio by the base speed
        double cpuClockSpeed();
};

#endif