#ifndef CPU_HPP
#define CPU_HPP

#include "processes.hpp"
#include <windows.h>
#include <string>
#include <array>
#include <variant>
#include <stdexcept>
#include <unordered_map>
#include <mutex>
#include <memory>

struct ProcessCpuSample{
    unsigned long long totalCpuTime;
    unsigned long long creationTime;
    ULONGLONG sampledAtMs;
};

struct CpuUsageSample{
    unsigned long long totalUsageTime;
    unsigned long long idleTime;
    ULONGLONG sampleAtMs;
};

class Cpu{
    private:
        Processes processes;
        std::unordered_map<DWORD, ProcessCpuSample> previousSamples;
        std::unique_ptr<CpuUsageSample> pastSample;
        inline static std::once_flag initFlag;
        inline static std::string name = "";
        inline static unsigned short cores = -1;
        inline static std::array<double, 3> cache = std::array<double,3>();
        inline static unsigned int processors = -1;

        static void initialize(Cpu& cpu);
        //Helper that converts FILETIME fields to unsigned long long
        unsigned long long fileTimeToULL(const FILETIME& ft);
        //Helper that returns CPU base speed
        double getBaseSpeed();
        //Helper that returns vector of structs holder processor information
        std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> getProcessorInfo();
        //Helper that returns a snapshot of a processes CPU usage by its PID
        ProcessCpuSample getSnap(DWORD pid);
        bool sampleProcessUsage(DWORD pid, double& usage);
        std::string cpuName() noexcept;
        CpuUsageSample utilizationSnap();
        bool sampleUtilization(double& usage);
        //Returns # of CPU cores
        unsigned short coreCount();
        //Returns an array holding the sizes of L1, L2, L3 caches
        std::array<double, 3> cacheAmounts();
        //Returns number of logical processors
        unsigned int processorCount();
    public:
        Cpu(){
            processes = Processes();
            initialize(*this);
        };

        //Returns percent CPU usage
        double currentUsage();
        //Returns the CPU usage of all processes that have the same exe name as the given PID
        void processNameTotalUsage(std::vector<DWORD>& pids, std::variant<double, std::string>& usage, std::exception_ptr& ptr);
        void utilization(double& usage, std::exception_ptr& ptr);
        //Returns CPU clock speed in GHz by multiplying the usage ratio by the base speed
        double cpuClockSpeed();
        //Returns the number of CPU threads 
        unsigned int threadCount();
        //Returns number of handles that do not belong to PPL or PP-L processes
        //Sadly makes the return extremely off 
        unsigned int handleCount();
        //Returns CPU uptime in d:h:m:s format
        std::string upTime();

        std::string getName() const noexcept;
        unsigned short getCores() const noexcept;
        const std::array<double, 3> getCache() const noexcept;
        unsigned int getProcessors() const noexcept;
        bool virtualizationState();
};

#endif