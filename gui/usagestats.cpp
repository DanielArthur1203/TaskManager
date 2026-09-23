#include "usagestats.hpp"
#include <QDebug>
#include <iostream>
#include <chrono>

using namespace std::chrono;

UsageStats::UsageStats(int pid, bool isPPL){
    if(isPPL){
        data->cpuUsage = "N/A";
        data->memoryUsage = "N/A";
        data->diskUsage = "N/A";
    }
    else{
        try{
            this->pid = pid;
            //higher than it should be fml
            startCPUsageReport(this->pid);
            startMemoryUsageReport(this->pids);
            startDiskUsageReport(this->pids);
            //memoryUsage = memory->getNamePhysicalMemoryUsage(pid);
            //diskUsage = disk->allProcessNameDiskUsage(pid);
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
    }
}

UsageStats::UsageStats(int pid, bool isPPL, std::vector<DWORD>& pids){
    if(isPPL){
        data->cpuUsage = "N/A";
        data->memoryUsage = "N/A";
        data->diskUsage = "N/A";
    }
    else{
        try{
            this->pid = pid;
            this->pids = pids;
            startCPUsageReport(this->pid);
            //checkSharedException();
            startMemoryUsageReport(this->pids);
            startDiskUsageReport(this->pids);
            //memoryUsage = memory->getNamePhysicalMemoryUsage(pid);
            //diskUsage = disk->allProcessNameDiskUsage(pid);
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
    }
}

UsageStats::UsageStats(UsageStats &&other) noexcept{
    std::scoped_lock lock(other.mtx, other.mtx2, other.mtx3);

    sharedException = other.sharedException;
    other.sharedException = nullptr;

    report = std::move(other.report);
    memoryReport = std::move(other.memoryReport);
    diskReport = std::move(other.diskReport);

    pid = std::move(other.pid);
    pids = std::move(other.pids);
    cpu = std::move(other.cpu);
    memory = std::move(other.memory);
    disk = std::move(other.disk);

    data->cpuUsage = std::move(other.data->cpuUsage);
    data->memoryUsage = std::move(other.data->memoryUsage);
    data->diskUsage = std::move(other.data->diskUsage);
}

UsageStats &UsageStats::operator=(UsageStats &&other) noexcept{
    if(this != &other){
        data = std::move(other.data);
        report = std::move(other.report);
        memoryReport = std::move(other.memoryReport);
        diskReport = std::move(other.diskReport);
    }
    return *this;

}

UsageStats::~UsageStats(){
    qDebug() << "Object killed";
    stopThreads();
}

void UsageStats::startCPUsageReport(int pid){
    //report = std::jthread(&UsageStats::cpuReportStartHelper, this, pid);
    auto ptr = this->data;
    report = std::jthread([ptr, pid, this](std::stop_token tok){
        try{
            while(!tok.stop_requested()){
                {
                    std::lock_guard<std::mutex> lock(ptr->mtx);
                    this->cpu->processNameTotalUsage(pid, ptr->cpuUsage, std::ref(sharedException));
                }

                if (tok.stop_requested()) break;

                for (int i = 0; i < 20; ++i) {
                    if (tok.stop_requested()) break;
                    std::this_thread::sleep_for(milliseconds(50));
                }
            }
        }
        catch(const std::runtime_error& e){
            if(!std::string(e.what()).contains("5")){
                throw std::runtime_error(e.what());
            }
        }
        catch(const std::exception& e){
            throw std::runtime_error(e.what());
        }
    });
}

void UsageStats::startMemoryUsageReport(std::vector<DWORD> pids){
    //memoryReport = std::jthread(&UsageStats::memoryReportStartHelper, this, pids);
    auto ptr = this->data;
    memoryReport = std::jthread([ptr, pids, this](std::stop_token tok){
        try{
            while(!tok.stop_requested()){
                {
                    std::lock_guard<std::mutex> lock(ptr->mtx2);
                    this->memory->getNamePhysicalMemoryUsage(pids, ptr->memoryUsage, std::ref(sharedException));
                }
                if (tok.stop_requested()) break;

                for (int i = 0; i < 20; ++i) {
                    if (tok.stop_requested()) break;
                    std::this_thread::sleep_for(milliseconds(50));
                }
            }
        }
        catch(const std::runtime_error& e){
            if(!std::string(e.what()).contains("5")){
                throw std::runtime_error(e.what());
            }
        }
        catch(const std::exception& e){
            throw std::runtime_error(e.what());
        }
    });
}

void UsageStats::startDiskUsageReport(std::vector<DWORD> pids){
    //diskReport = std::jthread(&UsageStats::diskReportStartHelper, this, pids);
    auto ptr = this->data;
    diskReport = std::jthread([ptr, pids, this](std::stop_token tok){
        try{
            while(!tok.stop_requested()){
                {
                    std::lock_guard<std::mutex> lock(ptr->mtx3);
                    this->disk->allProcessNameDiskUsage(pids, ptr->diskUsage, std::ref(sharedException));
                }
                if (tok.stop_requested()) break;

                for (int i = 0; i < 20; ++i) {
                    if (tok.stop_requested()) break;
                    std::this_thread::sleep_for(milliseconds(50));
                }
            }
        }
        catch(const std::runtime_error& e){
            if(!std::string(e.what()).contains("5")){
                throw std::runtime_error(e.what());
            }
        }
        catch(const std::exception& e){
            throw std::runtime_error(e.what());
        }
    });
}

void UsageStats::checkSharedException(){
    if(sharedException){
        try{
            std::rethrow_exception(sharedException);
        }
        catch(const std::runtime_error& e){
            std::string msg = e.what();
            if(msg.contains("5") || msg.contains("87")){
                stopThreads();
            }
            else{
                throw std::runtime_error(e.what());
            }
        }
        catch(const std::exception& e){
            std::rethrow_exception(sharedException);
        }
    }
}

void UsageStats::stopThreads(){
    report.request_stop();
    memoryReport.request_stop(); 
    diskReport.request_stop();

    if (report.joinable()) report.join();
    if (memoryReport.joinable()) memoryReport.join();
    if (diskReport.joinable()) diskReport.join();
}

const std::variant<double, std::string> UsageStats::getCpuUsage() const noexcept
{
    return data->cpuUsage;
}

const std::variant<double, std::string> UsageStats::getMemoryUsage() const noexcept{
    return data->memoryUsage;
}

const std::variant<double, std::string> UsageStats::getDiskUsage() const noexcept{
    return data->diskUsage;
}
