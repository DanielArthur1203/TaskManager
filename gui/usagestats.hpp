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
#include <stdexcept>
#include <condition_variable>

struct DataHolder{
    int pid;
    std::vector<DWORD> pids;

    mutable std::mutex mtx;
    mutable std::mutex mtx2;
    mutable std::mutex mtx3;

    std::variant<double, std::string> cpuUsage = "N/A";
    std::variant<double, std::string> memoryUsage = "N/A";
    std::variant<double, std::string> diskUsage = "N/A";
};

class UsageStatsDaemon{
    public:
        static inline UsageStatsDaemon& instance(){
            static UsageStatsDaemon daemon;
            return daemon;
        }
        void registerHolder(const std::shared_ptr<DataHolder>& data);

        UsageStatsDaemon(const UsageStatsDaemon&) = delete;
        UsageStatsDaemon& operator=(const UsageStatsDaemon&) = delete;
        ~UsageStatsDaemon();
    private:
        inline UsageStatsDaemon(){
            for(std::size_t i = 0; i < 20; ++i){
                workers.emplace_back(
                    [this, i](std::stop_token stop){workerLoop(stop, i);}
                );
            }

            scheduler = std::jthread([this](std::stop_token stop){scheduleUpdate(stop);});
        };
        std::jthread scheduler;
        std::vector<std::jthread> workers;
        std::mutex recordsMutex;
        std::mutex cycleMutex;
        std::condition_variable cycleCondition;
        std::vector<std::weak_ptr<DataHolder>> holders;
        std::vector<std::shared_ptr<DataHolder>> currentHolders;
        std::uint64_t cycle = 0;
        std::size_t workersFinished = 0;

        void scheduleUpdate(std::stop_token stop);
        void updateUsageStats(const std::shared_ptr<DataHolder>& data, Cpu& cpu, Disk& disk);
        std::vector<std::shared_ptr<DataHolder>> getLiveHolders();
        void workerLoop(std::stop_token stop, std::size_t index);
};

class UsageStats{
    private:
        std::shared_ptr<DataHolder> data = std::make_shared<DataHolder>();
        std::exception_ptr sharedException = nullptr;

        std::unique_ptr<Cpu> cpu = std::make_unique<Cpu>();
        std::unique_ptr<MemoryInfo> memory = std::make_unique<MemoryInfo>();
        std::unique_ptr<Disk> disk = std::make_unique<Disk>();

    public:
        UsageStats(int pid, bool isPPL, std::vector<DWORD>& pids);
        UsageStats(const UsageStats&) = delete;
        UsageStats& operator=(const UsageStats&) = delete;

        UsageStats(UsageStats&& other) noexcept;
        UsageStats& operator=(UsageStats&& other) noexcept;
        ~UsageStats();

        const std::variant<double, std::string> getCpuUsage() const noexcept;
        const std::variant<double, std::string> getMemoryUsage() const noexcept;
        const std::variant<double, std::string> getDiskUsage() const noexcept;
};

#endif