#include "usagestats.hpp"
#include <QDebug>
#include <iostream>
#include <chrono>

using namespace std::chrono;

UsageStats::UsageStats(int pid, bool isPPL, std::vector<DWORD>& pids){
    if(isPPL){
        data->cpuUsage = "N/A";
        data->memoryUsage = "N/A";
        data->diskUsage = "N/A";
    }
    else{
        try{
            data->pid = pid;
            data->pids = pids;
        }
        catch(const std::runtime_error& e){
            std::string msg = e.what();
            if(!msg.contains("5")){ //Win32 Error Access Denied. Comes from PP-L Processes with a exe name
                throw std::runtime_error(e.what());
            }
            data->cpuUsage = "N/A";
            data->memoryUsage = "N/A";
            data->diskUsage = "N/A";
        }
        catch(const std::exception& e){
            throw std::runtime_error(e.what());
        }
        UsageStatsDaemon::instance().registerHolder(data);
    }
}

UsageStats::UsageStats(UsageStats &&other) noexcept{
    data = std::move(other.data);
    sharedException = std::move(other.sharedException);
    cpu = std::move(other.cpu);
    memory = std::move(other.memory);
    disk = std::move(other.disk);
}

UsageStats &UsageStats::operator=(UsageStats &&other) noexcept{
    if(this != &other){
        data = std::move(other.data);
        sharedException = std::move(other.sharedException);
        cpu = std::move(other.cpu);
        memory = std::move(other.memory);
        disk = std::move(other.disk);
    }
    return *this;

}

UsageStats::~UsageStats(){
    qDebug() << "Object killed";
}

const std::variant<double, std::string> UsageStats::getCpuUsage() const noexcept{
    {
        std::lock_guard lock(data->mtx);
        return data->cpuUsage;
    }
}

const std::variant<double, std::string> UsageStats::getMemoryUsage() const noexcept{
    {
        std::lock_guard lock(data->mtx2);
        return data->memoryUsage;
    }
}

const std::variant<double, std::string> UsageStats::getDiskUsage() const noexcept{
    {
        std::lock_guard lock(data->mtx3);
        return data->diskUsage;
    }
}

//New Daemon methods below
//__________________________________________________________________________________________

void UsageStatsDaemon::registerHolder(const std::shared_ptr<DataHolder> &data){
    {
        std::lock_guard lock(recordsMutex);

        std::weak_ptr<DataHolder> weak(data);
        holders.push_back(std::move(weak));
    }
}

UsageStatsDaemon::~UsageStatsDaemon(){
    scheduler.request_stop();

    for(auto& worker: workers){
        worker.request_stop();
    }
    cycleCondition.notify_all();

    if(scheduler.joinable()){
        scheduler.join();
    }

    for(auto& worker: workers){
        if(worker.joinable()){
            worker.join();
        }
    }
}

void UsageStatsDaemon::scheduleUpdate(std::stop_token stop){
    while(!stop.stop_requested()){
        auto live = getLiveHolders();

        std::unique_lock lock(cycleMutex);
        currentHolders = std::move(live);
        ++cycle;
        cycleCondition.notify_all();

        cycleCondition.wait(lock, [&]{
            return stop.stop_requested() || workersFinished == workers.size();
        });

        if(stop.stop_requested()){
            break;
        }
        workersFinished = 0;
        lock.unlock();
        std::this_thread::sleep_for(milliseconds(250));
    }
}

void UsageStatsDaemon::updateUsageStats(const std::shared_ptr<DataHolder> &data, Cpu& cpu, Disk& disk){
    std::exception_ptr error = nullptr;
    std::variant<double, std::string> cpuUsage = 0.0;
    std::variant<double, std::string> memoryUsage = 0.0;
    std::variant<double, std::string> diskUsage = 0.0;

    cpu.processNameTotalUsage(data->pids, cpuUsage, error);

    MemoryInfo memory;
    memory.getNamePhysicalMemoryUsage(data->pids, memoryUsage, error);

    disk.allProcessNameDiskUsage(data->pids, diskUsage, error);

    {
        std::scoped_lock lock(data->mtx, data->mtx2, data->mtx3);
        data->cpuUsage = std::move(cpuUsage);
        data->memoryUsage = std::move(memoryUsage);
        data->diskUsage = std::move(diskUsage);
    }

    if(error){
        try{
            std::rethrow_exception(error);
        }
        catch(const std::runtime_error& e){
            std::string msg = e.what();
            if(msg.contains("5") || msg.contains("87")){
                
            }
            else{
                throw std::runtime_error(e.what());
            }
        }
        catch(const std::exception& e){
            std::rethrow_exception(error);
        }
    }
}

std::vector<std::shared_ptr<DataHolder>> UsageStatsDaemon::getLiveHolders(){
    std::vector<std::shared_ptr<DataHolder>> live;

    std::lock_guard lock(recordsMutex);

    for(auto it = holders.begin(); it != holders.end();){
        if(auto data = it->lock()){
            live.push_back(std::move(data));
            ++it;
        }
        else{
            it = holders.erase(it);
        }
    }
    return live;
}

void UsageStatsDaemon::workerLoop(std::stop_token stop, std::size_t index){
    std::uint64_t last = 0;
    Cpu cpu;
    Disk disk;
    while(!stop.stop_requested()){
        std::vector<std::shared_ptr<DataHolder>> holdersForCycle;

        {
            std::unique_lock lock(cycleMutex);
            cycleCondition.wait(lock, [&]{
                return stop.stop_requested() || cycle != last;
            });

            if(stop.stop_requested()){
                return;
            }

            last = cycle;
            holdersForCycle = currentHolders;
        }
        for(std::size_t i = index; i < holdersForCycle.size(); i += workers.size()){
            try{
                updateUsageStats(holdersForCycle.at(i), cpu, disk);
            }
            catch(const std::runtime_error& e){
                qWarning() << "Update failed: " << e.what();
            }
            catch(const std::exception& e){
                qWarning() << "Update failed with unknown exception";
            }
            catch(...){
                qWarning() << "Unknown Error";
            }
        }
        {
            std::lock_guard lock(cycleMutex);
            ++workersFinished;
        }
        cycleCondition.notify_all();
    }

}
