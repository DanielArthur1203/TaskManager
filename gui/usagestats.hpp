#ifndef USAGESTATS_HPP
#define USAGESTATS_HPP

#include "../general/cpu.hpp"
#include "../general/memory.hpp"
#include "../general/disk.hpp"
#include <memory>
#include <variant>
#include <string>
#include <mutex>
#include <thread>
#include <vector>
#include <array>

struct DataHolder{
    mutable std::mutex mtx;
    mutable std::mutex mtx2;
    mutable std::mutex mtx3;

    std::variant<double, std::string> cpuUsage = "N/A";
    std::variant<double, std::string> memoryUsage = "N/A";
    std::variant<double, std::string> diskUsage = "N/A";
};

class UsageStats{
    private:
        std::shared_ptr<DataHolder> data = std::make_shared<DataHolder>();
        mutable std::mutex mtx;
        mutable std::mutex mtx2;
        mutable std::mutex mtx3;

        int pid;
        std::vector<DWORD> pids;
        std::jthread report;
        std::jthread memoryReport;
        std::jthread diskReport;

        std::unique_ptr<Cpu> cpu = std::make_unique<Cpu>();
        std::unique_ptr<MemoryInfo> memory = std::make_unique<MemoryInfo>();
        std::unique_ptr<Disk> disk = std::make_unique<Disk>();

        //void cpuReportStartHelper(std::stop_token tok, int pid);
        void startCPUsageReport(int pid);
        // void memoryReportStartHelper(std::stop_token tok, std::vector<DWORD> pids);
        void startMemoryUsageReport(std::vector<DWORD> pids);
        //void diskReportStartHelper(std::stop_token tok, std::vector<DWORD> pids);
        void startDiskUsageReport(std::vector<DWORD> pids);
    public:
        UsageStats(int pid, bool isPPL);
        UsageStats(int pid, bool isPPL, std::vector<DWORD>& pids);
        UsageStats(const UsageStats&) = delete;
        UsageStats& operator=(const UsageStats&) = delete;

        UsageStats(UsageStats&& other) noexcept;
        UsageStats& operator=(UsageStats&& other) noexcept;
        ~UsageStats();

        void stopThreads();
        const std::variant<double, std::string> getCpuUsage() const noexcept;
        const std::variant<double, std::string> getMemoryUsage() const noexcept;
        const std::variant<double, std::string> getDiskUsage() const noexcept;
};

#endif