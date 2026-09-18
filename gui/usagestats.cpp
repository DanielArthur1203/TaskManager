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
            data->cpuUsage = "N/A";
            data->memoryUsage = "N/A";
            data->diskUsage = "N/A";
            this->pid = pid;
            this->pids = pids;
            startCPUsageReport(this->pid);
            //startMemoryUsageReport(this->pids);
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

// void UsageStats::cpuReportStartHelper(std::stop_token tok, int pid){
//     try{
//         while(!tok.stop_requested()){
//             {
//                 std::lock_guard<std::mutex> lock(this->mtx);
//                 this->cpu->processNameTotalUsage(pid, this->cpuUsage);
//             }
//             //In the method is a 1 second sleep it shouldn't need this I hope
//             //std::this_thread::sleep_for(seconds(1));
//         }
//     }
//     catch(const std::runtime_error& e){
//         if(!std::string(e.what()).contains("5")){
//             throw std::runtime_error(e.what());
//         }
//     }
//     catch(const std::exception& e){
//         throw std::runtime_error(e.what());
//     }
// }

void UsageStats::startCPUsageReport(int pid){
    //report = std::jthread(&UsageStats::cpuReportStartHelper, this, pid);
    auto ptr = this->data;
    report = std::jthread([ptr, pid, this](std::stop_token tok){
        try{
            while(!tok.stop_requested()){
                {
                    std::lock_guard<std::mutex> lock(ptr->mtx);
                    this->cpu->processNameTotalUsage(pid, ptr->cpuUsage);
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

// void UsageStats::memoryReportStartHelper(std::stop_token tok, std::vector<DWORD> pids){
//     try{
//         while(!tok.stop_requested()){
//             {
//                 std::lock_guard<std::mutex> lock(this->mtx2);
//                 this->memory->getNamePhysicalMemoryUsage(pids);
//             }
//             std::this_thread::sleep_for(seconds(1));
//         }
//     }
//     catch(const std::runtime_error& e){
//         if(!std::string(e.what()).contains("5")){
//             throw std::runtime_error(e.what());
//         }
//     }
//     catch(const std::exception& e){
//         throw std::runtime_error(e.what());
//     }
// }

void UsageStats::startMemoryUsageReport(std::vector<DWORD> pids){
    //memoryReport = std::jthread(&UsageStats::memoryReportStartHelper, this, pids);
    auto ptr = this->data;
    memoryReport = std::jthread([ptr, pids, this](std::stop_token tok){
        try{
            while(!tok.stop_requested()){
                {
                    std::lock_guard<std::mutex> lock(ptr->mtx2);
                    this->memory->getNamePhysicalMemoryUsage(pids, ptr->memoryUsage);
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

// void UsageStats::diskReportStartHelper(std::stop_token tok, std::vector<DWORD> pids){
//     try{
//         while(!tok.stop_requested()){
//             {
//                 std::lock_guard<std::mutex> lock(this->mtx3);
//                 this->disk->allProcessNameDiskUsage(pids);
//             }
//             //std::this_thread::sleep_for(seconds(1));
//         }
//     }
//     catch(const std::runtime_error& e){
//         if(!std::string(e.what()).contains("5")){
//             throw std::runtime_error(e.what());
//         }
//     }
//     catch(const std::exception& e){
//         throw std::runtime_error(e.what());
//     }
// }

void UsageStats::startDiskUsageReport(std::vector<DWORD> pids){
    //diskReport = std::jthread(&UsageStats::diskReportStartHelper, this, pids);
    auto ptr = this->data;
    diskReport = std::jthread([ptr, pids, this](std::stop_token tok){
        try{
            while(!tok.stop_requested()){
                {
                    std::lock_guard<std::mutex> lock(ptr->mtx3);
                    this->disk->allProcessNameDiskUsage(pids, ptr->diskUsage);
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
