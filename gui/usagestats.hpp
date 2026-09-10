#ifndef USAGESTATS_HPP
#define USAGESTATS_HPP

#include "../general/cpu.hpp"
#include "../general/memory.hpp"
#include "../general/disk.hpp"
#include <memory>

class UsageStats{
    private:
        std::unique_ptr<Cpu> cpu;
        std::unique_ptr<MemoryInfo> memory;
        std::unique_ptr<Disk> disk;

        double cpuUsage = 0;
        double memoryUsage = 0;
        double diskUsage = 0;
    public:
        explicit UsageStats(int pid);

        const double getCpuUsage() noexcept;
        const double getMemoryUsage() noexcept;
        const double getDiskUsage() noexcept;
};

#endif