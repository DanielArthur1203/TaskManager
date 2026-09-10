#include "usagestats.hpp"

UsageStats::UsageStats(int pid){
    try{
        cpuUsage = cpu->processNameTotalUsage(pid);
        memoryUsage = memory->getNamePhysicalMemoryUsage(pid);
        diskUsage = disk->allProcessNameDiskUsage(pid);
    }
    catch(const std::runtime_error& e){
        throw std::runtime_error(e.what());
    }
    catch(const std::exception& e){
        throw std::runtime_error(e.what());
    }
    cpu.reset();
    memory.reset();
    disk.reset();
}

const double UsageStats::getCpuUsage() noexcept{
    return cpuUsage;
}

const double UsageStats::getMemoryUsage() noexcept{
    return memoryUsage;
}

const double UsageStats::getDiskUsage() noexcept{
    return diskUsage;
}
